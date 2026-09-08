/* Diagnostic Bridge shell browser contract. The server is a local fault-injection fixture. */
"use strict";

const assert = require("node:assert/strict");
const crypto = require("node:crypto");
const fs = require("node:fs");
const http = require("node:http");
const path = require("node:path");

const root = path.resolve(__dirname, "../..");
const shell = fs.readFileSync(path.join(root, "ui/diagnostic-web/bridge-shell.html"));

function sendJson(response, status, value) {
  const body = Buffer.from(JSON.stringify(value));
  response.writeHead(status, {
    "Content-Type": "application/json; charset=utf-8",
    "Cache-Control": "no-store",
    "Content-Length": body.length,
  });
  response.end(body);
}

async function readRequest(request) {
  const chunks = [];
  for await (const chunk of request) chunks.push(chunk);
  return Buffer.concat(chunks);
}

async function listen(server) {
  const chromiumUnsafePorts = new Set([1, 7, 9, 11, 13, 15, 17, 19, 20, 21, 22, 23, 25, 37, 42, 43,
    53, 67, 68, 69, 70, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 99, 110, 111, 119,
    123, 135, 139, 143, 161, 179, 389, 427, 465, 512, 513, 514, 515, 526, 530, 531, 532, 540,
    548, 554, 556, 563, 587, 601, 636, 993, 995, 2049, 3659, 4045, 5060, 6000, 6566, 6665,
    6666, 6667, 6668, 6669, 6697, 10080, 27017]);
  for (let attempt = 0; attempt < 8; attempt += 1) {
    const port = await new Promise((resolve, reject) => {
      const onError = (error) => { server.removeListener("error", onError); reject(error); };
      server.once("error", onError);
      server.listen(0, "127.0.0.1", () => {
        server.removeListener("error", onError);
        resolve(server.address().port);
      });
    });
    if (!chromiumUnsafePorts.has(port)) return port;
    await new Promise((resolve) => server.close(resolve));
  }
  throw new Error("could not allocate a Chromium-safe local port");
}

async function closeServer(server, sockets) {
  for (const socket of sockets) socket.destroy();
  await new Promise((resolve) => server.close(resolve));
}

async function runBridgeShellTests(browser, options = {}) {
  const sockets = new Set();
  let sessionRequests = 0;
  let systemRequests = 0;
  const server = http.createServer(async (request, response) => {
    if (request.url === "/bridge-shell.html" && request.method === "GET") {
      response.writeHead(200, {"Content-Type": "text/html; charset=utf-8", "Content-Length": shell.length});
      response.end(shell);
      return;
    }
    if (request.url === "/api/v1/bootstrap" && request.method === "GET") {
      sendJson(response, 200, {schema_version: 1, snapshot_revision: 1, challenge: "fixture-challenge"});
      return;
    }
    if (request.url === "/api/v1/session" && request.method === "POST") {
      await readRequest(request);
      sessionRequests += 1;
      sendJson(response, 200, {token: "AAAAAAAAAAAAAAAAAAAAAA", snapshot_revision: 2});
      return;
    }
    if (request.url === "/api/v1/system" && request.method === "GET") {
      systemRequests += 1;
      if (systemRequests === 1) {
        sendJson(response, 200, {snapshot_revision: 2, read_only: true, control_scope: 0, vehicle_tx: false});
      } else {
        sendJson(response, 401, {error: "expired"});
      }
      return;
    }
    if (request.url === "/favicon.ico" && request.method === "GET") {
      response.writeHead(204);
      response.end();
      return;
    }
    response.writeHead(404);
    response.end();
  });
  server.on("upgrade", (request, socket) => {
    sockets.add(socket);
    socket.once("close", () => sockets.delete(socket));
    const protocols = String(request.headers["sec-websocket-protocol"] || "")
      .split(",").map((value) => value.trim());
    const key = request.headers["sec-websocket-key"];
    if (request.url !== "/api/v1/live" || !key || !protocols.includes("canview-session")) {
      socket.destroy();
      return;
    }
    const accept = crypto.createHash("sha1")
      .update(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
      .digest("base64");
    socket.write("HTTP/1.1 101 Switching Protocols\r\n" +
      "Upgrade: websocket\r\n" +
      "Connection: Upgrade\r\n" +
      "Sec-WebSocket-Accept: " + accept + "\r\n" +
      "Sec-WebSocket-Protocol: canview-session\r\n\r\n");
    setTimeout(() => socket.end(), 50);
  });

  const port = await listen(server);
  const baseUrl = "http://127.0.0.1:" + port;
  const context = await browser.newContext({viewport: {width: 390, height: 844}, reducedMotion: "reduce"});
  const page = await context.newPage();
  const errors = [];
  const external = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("request", (request) => {
    if (!request.url().startsWith(baseUrl)) external.push(request.url());
  });
  try {
    await page.goto(baseUrl + "/bridge-shell.html");
    await page.locator("#pin").fill("123456");
    await page.locator("#submit").click();
    await page.waitForFunction(() => document.getElementById("login").hidden, null, {timeout: 2000});
    await page.waitForFunction(() => {
      const login = document.getElementById("login");
      return !login.hidden && document.getElementById("status").textContent.includes("세션 만료");
    }, null, {timeout: 5000});
    assert.equal(sessionRequests, 1, "session request count");
    assert.equal(systemRequests, 2, "snapshot then expiry request");
    assert.equal(await page.locator("#login").isVisible(), true, "expired session returns to login");
    assert.equal(await page.locator("#pin").inputValue(), "", "expired token input is cleared");
    assert.equal(await page.locator("#submit").isDisabled(), false, "re-authentication is enabled");
    await page.waitForTimeout(1200);
    assert.equal(systemRequests, 2, "expired session cancels reconnect loop");
    assert.equal(errors.length, 0, "bridge shell browser errors: " + errors.join(", "));
    assert.equal(external.length, 0, "external bridge shell requests: " + external.join(", "));
    if (options.screenshotDir) {
      await page.screenshot({path: path.join(options.screenshotDir, "bridge-shell-expired.png"), animations: "disabled"});
    }
    return {suite: "diagnostic-bridge-shell", checks: 8, errors, externalRequests: external.length};
  } finally {
    await context.close();
    await closeServer(server, sockets);
  }
}

module.exports = {runBridgeShellTests};
