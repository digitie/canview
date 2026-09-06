/* 진단 웹의 오프라인 5뷰·조작·실패 회귀. 공통 runner는 runDiagnosticTests를 호출한다. */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs/promises");
const path = require("node:path");
const {pathToFileURL} = require("node:url");

async function runDiagnosticTests(browser, options = {}) {
  const url = options.url || pathToFileURL(path.resolve(__dirname, "../../ui/diagnostic-web/index.html")).href;
  const context = await browser.newContext({viewport: {width: 390, height: 844}, acceptDownloads: true, reducedMotion: "reduce"});
  const page = await context.newPage();
  const errors = [];
  const external = [];
  const passed = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("console", (message) => { if (message.type() === "error") errors.push(message.text()); });
  page.on("request", (request) => { if (!/^(file:|blob:|data:)/.test(request.url())) external.push(request.url()); });
  await context.setOffline(true);
  const text = (selector) => page.locator(selector).textContent();
  const select = (id, value) => page.locator("#" + id).selectOption(String(value));
  const click = (id) => page.locator("#" + id).click();
  const view = (name) => page.locator('[data-target="' + name + '"]').click();
  const waitState = (id, state) => page.waitForFunction(([elementId, value]) => document.getElementById(elementId).dataset.state === value, [id, state]);
  const load = async (screen = "status", demo = false) => {
    const target = new URL(url);
    target.searchParams.set("screen", screen);
    await page.goto(target.href);
    if (demo) await click("demo-toggle");
  };
  const step = async (name, action) => {
    try { await action(); passed.push(name); }
    catch (error) { error.message = name + ": " + error.message; throw error; }
  };
  try {
    await step("5개 직접 진입·미연결·권한 안내", async () => {
      for (const screen of ["status", "frames", "capture", "signals", "settings"]) {
        await load(screen);
        assert.equal(await page.locator("[data-view]:visible").count(), 1);
        assert.equal(await page.locator('[data-view="' + screen + '"]').isVisible(), true);
        assert.equal(await page.locator('[data-target="' + screen + '"]').getAttribute("aria-current"), "page");
        assert.match(await text(".vehicle-state"), /미연결.*속도 불명/);
        assert.match(await text(".prototype-notice"), /서버 없음/);
        assert.equal(await page.locator("#candidate-open").isDisabled(), true);
        assert.equal(await page.locator("#sample-quality").isDisabled(), true);
      }
      await load("unknown");
      assert.equal(await page.locator('[data-view="status"]').isVisible(), true);
      assert.equal(await page.locator(".bus-card").count(), 3);
      assert.doesNotMatch(await text(".bus-grid"), /ONLINE|500|642/);
      await click("permissions-open");
      assert.equal(await page.locator("#permissions-dialog").isVisible(), true);
      await page.keyboard.press("Escape");
      assert.equal(await page.locator("#permissions-dialog").isVisible(), false);
    });
    await step("장치 확인·capture timeout·취소·재시도", async () => {
      await click("connection-check");
      await waitState("connection-result", "pending");
      await waitState("connection-result", "rejected");
      assert.equal(await page.locator("#connection-check").isDisabled(), false);
      await view("capture");
      for (let attempt = 0; attempt < 2; attempt += 1) {
        await click("capture-start");
        await waitState("capture-status", "pending");
        await waitState("capture-status", "rejected");
        assert.match(await text("#capture-status"), /NO_BACKEND/);
        assert.equal(await page.locator("#capture-start").isDisabled(), false);
        assert.equal(await page.locator("#capture-mark").isDisabled(), true);
        assert.doesNotMatch(await text("#capture-status"), /준비 완료|성공|APPLIED/);
      }
      await click("capture-start");
      await click("capture-cancel");
      await page.waitForTimeout(650);
      assert.match(await text("#capture-status"), /연습 취소/);
    });
    await step("프레임 필터·ID 검증·정렬·freeze·선택 identity", async () => {
      await load("frames", true);
      assert.equal(await page.locator(".frame-row").count(), 6);
      await select("filter-bus", 3);
      assert.equal(await page.locator(".frame-row").count(), 1);
      assert.match(await text(".frame-row"), /0x593.*DLC 6/);
      await page.locator("#filter-id").fill("541");
      assert.equal(await page.locator(".frame-row").count(), 0);
      await click("filter-reset");
      for (const invalid of ["0x", "xyz", "0x20000000", "-1", "<svg>"]) {
        await page.locator("#filter-id").fill(invalid);
        assert.equal(await page.locator("#filter-id").getAttribute("aria-invalid"), "true");
        assert.equal(await page.locator(".frame-row").count(), 0);
      }
      await page.locator("#filter-id").fill("0x541");
      assert.equal(await page.locator(".frame-row").count(), 3);
      await select("filter-format", "extended");
      assert.equal(await page.locator(".frame-row").count(), 1);
      assert.match(await text(".frame-row"), /29-bit/);
      await page.locator(".frame-row").click();
      assert.match(await text("#signal-identity"), /0x00000541.*29-bit/);
      await view("frames");
      await click("filter-reset");
      await select("filter-state", "new");
      assert.equal(await page.locator(".frame-row").count(), 2);
      await select("filter-state", "changed");
      assert.equal(await page.locator(".frame-row").count(), 5);
      await select("filter-sort", "rate");
      assert.match(await page.locator(".frame-row").first().textContent(), /0x386/);
      await select("filter-sort", "id");
      assert.match(await page.locator(".frame-row").first().textContent(), /0x1EB/);
      await select("filter-sort", "recent");
      assert.match(await page.locator(".frame-row").first().textContent(), /0x386/);
      await click("filter-reset");
      const before = await page.locator(".frame-row code").first().textContent();
      await click("freeze");
      await click("sample-next");
      assert.equal(await page.locator(".frame-row code").first().textContent(), before);
      assert.match(await text("#frame-summary"), /표 고정/);
      await click("freeze");
      assert.notEqual(await page.locator(".frame-row code").first().textContent(), before);
      await page.locator('[data-frame-key="1:0:1345:2"]').click();
      assert.match(await text("#signal-identity"), /DLC 2/);
      assert.equal(await page.locator(".bit-cell").count(), 16);
    });
    await step("Intel·Motorola·64-bit·부호·scale·offset·단위 계산", async () => {
      await load("signals", true);
      assert.equal(await text("#decoded-value"), "0");
      assert.match(await text("#comparison-value"), /비교 샘플: 1 raw/);
      await click("signal-next");
      assert.equal(await text("#decoded-value"), "1");
      assert.match(await text("#comparison-value"), /비교 샘플: 0 raw/);
      await click("signal-next");
      await select("start-bit", 0); await select("bit-length", 8);
      assert.equal(await text("#decoded-value"), "58");
      await select("factor-preset", "0.5"); await select("offset-preset", "-40"); await select("unit", "°C");
      assert.equal(await text("#decoded-value"), "-11");
      await select("factor-preset", "1"); await select("offset-preset", "0"); await select("unit", "raw");
      await select("start-bit", 7); await select("bit-length", 16); await select("byte-order", "MOTOROLA");
      assert.equal(await text("#decoded-value"), "14848");
      assert.match(await text("#bit-range"), /7 → 6 → 5 → 4 → 3 → 2 → 1 → 0 → 15/);
      await select("start-bit", 0); await select("bit-length", 64); await select("byte-order", "INTEL");
      assert.equal(await text("#decoded-value"), BigInt("0x6f8c04201091003a").toString());
      await select("signal-frame", "1:0:1345:2"); await select("start-bit", 0); await select("bit-length", 8); await select("signed", "signed");
      assert.equal(await text("#decoded-value"), "-1");
      await select("bit-length", 16);
      assert.equal(await text("#decoded-value"), "-32513");
      await select("start-bit", 15); await select("bit-length", 2);
      assert.match(await text("#decoder-error"), /DLC 2 밖/);
      assert.equal(await page.locator("#candidate-open").isDisabled(), true);
      await select("byte-order", "MOTOROLA");
      assert.equal(await text("#decoded-value"), "-2");
      await select("start-bit", 8);
      assert.match(await text("#decoder-error"), /DLC 2 밖/);
    });
    await step("잘못된 decimal·bool·unit·bit 선택 복구", async () => {
      await load("signals", true);
      await select("start-bit", 0); await select("bit-length", 8);
      assert.equal(await page.locator('#decoder input[type="text"], #decoder input[type="number"]').count(), 0);
      for (const invalid of ["", "NaN", "Infinity", "1e3", "1000001", "0.1234567", "0", "<img>"]) {
        await page.evaluate((value) => { const select = document.getElementById("factor-preset"); select.add(new Option("malformed fixture", value)); select.value = value; select.dispatchEvent(new Event("change", {bubbles: true})); }, invalid);
        assert.notEqual(await text("#decoder-error"), "");
        assert.equal(await text("#decoded-value"), "—");
        assert.equal(await page.locator("#candidate-open").isDisabled(), true);
      }
      await select("factor-preset", "-0.5");
      await select("offset-preset", "-40");
      assert.equal(await text("#decoded-value"), "-69");
      await page.evaluate(() => { const select = document.getElementById("offset-preset"); select.add(new Option("malformed fixture", "Infinity")); select.value = "Infinity"; select.dispatchEvent(new Event("change", {bubbles: true})); });
      assert.match(await text("#decoder-error"), /Offset/);
      await select("factor-preset", "1"); await select("offset-preset", "0");
      await select("unit", "bool");
      assert.match(await text("#decoder-error"), /bool/);
      await select("bit-length", 1);
      assert.equal(await text("#decoder-error"), "");
      await page.locator('.bit-cell[aria-label$="시작 bit 12"]').click();
      assert.equal(await page.locator("#start-bit").inputValue(), "12");
      assert.equal(await page.locator('.bit-cell[aria-pressed="true"]').count(), 1);
      await select("start-bit", 63); await select("bit-length", 2);
      assert.match(await text("#decoder-error"), /DLC 8 밖/);
      await page.evaluate(() => { const unit = document.getElementById("unit"); unit.add(new Option("bad", "bad")); unit.value = "bad"; unit.dispatchEvent(new Event("change", {bubbles: true})); });
      await select("bit-length", 1);
      assert.match(await text("#decoder-error"), /단위/);
    });
    await step("로컬 캡처 marker·취소·신호 Lab 전후 값 연결", async () => {
      await load("capture", true);
      await page.locator('input[name="template"][value="door"]').check();
      await select("capture-frame", "2:0:491:8"); await select("capture-mode", "FILTERED_RAW");
      await select("capture-pre", 2); await select("capture-post", 5);
      assert.match(await text("#capture-scope"), /CAN 2.*0x1EB/);
      assert.match(await text("#capture-duration"), /FILTERED_RAW.*앞 2초.*뒤 5초/);
      await click("capture-start"); await waitState("capture-status", "pending");
      await click("capture-cancel"); await page.waitForTimeout(650);
      assert.match(await text("#capture-status"), /취소/);
      await click("capture-start"); await waitState("capture-status", "local");
      assert.equal(await page.locator("#capture-finish").isDisabled(), true);
      for (let i = 1; i <= 3; i += 1) { await click("capture-mark"); assert.match(await text("#capture-status"), new RegExp("marker " + i + "/3")); }
      assert.equal(await page.locator("#capture-mark").isDisabled(), true);
      await click("capture-finish");
      assert.match(await text("#capture-comparison"), /marker 3회.*bit 7/);
      await click("capture-analyze");
      assert.match(await text("#raw-age"), /캡처 연습의 전 샘플/);
      assert.equal(await text("#decoded-value"), "0");
      assert.match(await text("#comparison-value"), /비교 샘플: 1/);
      await click("signal-next");
      assert.equal(await text("#decoded-value"), "1");
      assert.match(await text("#raw-age"), /캡처 연습의 후 샘플/);
    });
    await step("후보 dialog·중복 방지·JSON whitelist·다운로드 실패·삭제", async () => {
      await load("signals", true);
      await click("candidate-open");
      await page.keyboard.press("Escape");
      assert.equal(await text("#candidate-count"), "0 / 20");
      for (let i = 0; i < 2; i += 1) { await click("candidate-open"); await click("candidate-save"); }
      assert.equal(await text("#candidate-count"), "1 / 20");
      assert.match(await text("#export-status"), /이미/);
      const [download] = await Promise.all([page.waitForEvent("download"), click("candidate-export")]);
      assert.equal(download.suggestedFilename(), "canview-demo-candidates.json");
      const bundle = JSON.parse(await fs.readFile(await download.path(), "utf8"));
      assert.equal(bundle.vehicle_evidence, false);
      assert.equal(bundle.source, "SYNTHETIC_DEMO");
      assert.equal(bundle.candidates.length, 1);
      assert.deepEqual(Object.keys(bundle.candidates[0]).sort(), ["status", "grade", "source", "quality", "bus_id", "can_id", "extended", "dlc", "start_bit", "bit_length", "byte_order", "signed", "factor", "offset", "unit", "evidence_ids"].sort());
      assert.equal(bundle.candidates[0].status, "CANDIDATE");
      assert.deepEqual(bundle.candidates[0].evidence_ids, []);
      assert.doesNotMatch(JSON.stringify(bundle), /password|token|secret|latitude|longitude|source_device|raw_data|VIN|VERIFIED|APPLIED/i);
      await page.evaluate(() => { window.savedCreateObjectURL = URL.createObjectURL; URL.createObjectURL = () => { throw new Error("download failure"); }; });
      await click("candidate-export");
      assert.match(await text("#export-status"), /REJECTED.*메모리에 유지/);
      assert.equal(await text("#candidate-count"), "1 / 20");
      await page.evaluate(() => { URL.createObjectURL = window.savedCreateObjectURL; delete window.savedCreateObjectURL; });
      await click("candidate-clear"); await page.keyboard.press("Escape");
      assert.equal(await text("#candidate-count"), "1 / 20");
      await click("candidate-clear"); await click("candidate-clear-confirm");
      assert.equal(await text("#candidate-count"), "0 / 20");
      assert.equal(await page.locator("#candidate-export").isDisabled(), true);
    });
    await step("stale·gap·malformed·중단 및 pending 무효화", async () => {
      await load("signals", true);
      for (const quality of ["stale", "gap", "malformed", "disconnected"]) {
        await select("sample-quality", quality);
        assert.equal(await page.locator("#candidate-open").isDisabled(), true);
        assert.match(await text("#source-status"), new RegExp(quality.toUpperCase()));
        if (quality !== "gap") assert.equal(await text("#decoded-value"), "—");
        if (["malformed", "disconnected"].includes(quality)) assert.equal(await text("#raw-bytes"), "—");
        await view("frames");
        assert.equal(await page.locator(".frame-row").count(), ["stale", "gap"].includes(quality) ? 6 : 0);
        await view("capture");
        await click("capture-start"); await waitState("capture-status", "rejected");
        assert.equal(await page.locator("#capture-mark").isDisabled(), true);
        await view("signals");
      }
      await select("sample-quality", "sample");
      await view("capture"); await click("capture-start");
      await select("sample-quality", "disconnected");
      await page.waitForTimeout(650);
      assert.match(await text("#capture-status"), /DISCONNECTED/);
      assert.equal(await page.locator("#capture-mark").isDisabled(), true);
      await select("sample-quality", "sample");
      await view("signals"); await click("candidate-open");
      // Dialog이 열린 동안 외부 상태가 바뀌는 경로도 저장 직전에 검사한다.
      await page.evaluate(() => { const quality = document.getElementById("sample-quality"); quality.value = "stale"; quality.dispatchEvent(new Event("change")); });
      assert.equal(await page.locator("#candidate-save").isDisabled(), true);
      assert.match(await text("#candidate-save-status"), /REJECTED/);
      await page.keyboard.press("Escape");
      await select("sample-quality", "sample");
      await click("candidate-open"); await click("candidate-save");
      await click("demo-toggle");
      assert.equal(await text("#candidate-count"), "1 / 20");
      assert.equal(await text("#decoded-value"), "—");
      await page.reload();
      assert.equal(await text("#candidate-count"), "0 / 20");
    });
    await step("설정 대상·초안·AP 회전 무응답·연습·dialog 취소", async () => {
      await load("settings");
      await select("setting-ap", 20); await click("settings-save");
      assert.match(await text("#settings-status"), /LOCAL ONLY.*20분.*장치 적용 없음/);
      await page.locator('[data-settings="controller"]').click();
      assert.equal(await page.locator("#settings-save").isDisabled(), true);
      assert.equal(await page.locator('[data-settings-panel="controller"]').isVisible(), true);
      await page.locator('[data-settings="stream"]').click();
      await select("setting-rate", 200); await select("setting-budget", 8); await click("settings-save");
      assert.match(await text("#stream-summary"), /200 f\/s.*8 kB\/s.*수락 —/);
      assert.match(await text("#settings-status"), /LOCAL ONLY.*stream/);
      await page.evaluate(() => { const rate = document.getElementById("setting-rate"); rate.add(new Option("999", "999")); rate.value = "999"; });
      await click("settings-save");
      assert.match(await text("#settings-status"), /REJECTED/);
      await page.locator('[data-settings="bridge"]').click();
      await click("ap-open");
      assert.equal(await page.locator("#ap-rehearse").isDisabled(), true);
      await click("ap-check"); await waitState("ap-status", "pending"); await waitState("ap-status", "rejected");
      assert.match(await text("#ap-status"), /NO_BACKEND.*변경되지/);
      await click("ap-check"); await page.keyboard.press("Escape"); await page.waitForTimeout(650);
      await click("ap-open");
      assert.match(await text("#ap-status"), /취소/);
      await page.keyboard.press("Escape");
      await click("demo-toggle"); await click("ap-open"); await click("ap-rehearse");
      assert.match(await text("#ap-demo-status"), /LOCAL DEMO.*1회.*암호 생성·회전 없음/);
      await page.keyboard.press("Escape");
    });
    await step("320–430px·큰 화면 5뷰·44px/24px·가로 overflow·오프라인", async () => {
      await load("status", true);
      for (const width of [320, 360, 390, 430, 768]) {
        await page.setViewportSize({width, height: 844});
        for (const screen of ["status", "frames", "capture", "signals", "settings"]) {
          await view(screen);
          const findings = await page.evaluate(() => {
            const failures = [];
            if (document.documentElement.scrollWidth > innerWidth + 1) failures.push("horizontal overflow");
            for (const element of document.querySelectorAll("button, select, input")) {
              if (!element.checkVisibility() || element.type === "checkbox" || element.type === "radio") continue;
              const box = element.getBoundingClientRect();
              const minimum = element.classList.contains("bit-cell") ? 24 : 44;
              if (box.height < minimum - 0.1 || box.width < minimum - 0.1) failures.push(element.id + ": " + box.width + "×" + box.height);
              if (!element.closest("label") && !element.getAttribute("aria-label") && !(element.textContent || "").trim()) failures.push("unlabelled: " + element.id);
            }
            return failures;
          });
          assert.deepEqual(findings, [], width + "px " + screen);
          if (options.screenshotDir && width === 390) {
            await fs.mkdir(options.screenshotDir, {recursive: true});
            await page.screenshot({path: path.join(options.screenshotDir, "diagnostic-" + screen + ".png"), fullPage: false});
          }
        }
      }
      assert.deepEqual(external, []);
      assert.deepEqual(errors, []);
    });
    return {suite: "diagnostic-browser", passed: passed.length, checks: passed, pageErrors: errors, externalRequests: external};
  } finally { await context.close(); }
}

module.exports = {runDiagnosticTests};
if (require.main === module) {
  (async () => {
    const {chromium} = require("playwright");
    const browser = await chromium.launch({channel: "msedge", headless: true});
    try { console.log(JSON.stringify(await runDiagnosticTests(browser, {screenshotDir: process.env.DIAGNOSTIC_SCREENSHOTS}), null, 2)); }
    finally { await browser.close(); }
  })().catch((error) => { console.error(error); process.exitCode = 1; });
}
