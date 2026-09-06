/* 오프라인 브라우저 UI 회귀. Playwright와 Edge/Chromium은 Windows 개발환경에서 제공한다. */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const {pathToFileURL} = require("node:url");
const {chromium} = require("playwright");
const root = path.resolve(__dirname, "../..");
const screenshots = process.argv.includes("--screenshots");
const output = path.join(root, screenshots ? "docs/images" : ".tools/ui-check");
fs.mkdirSync(output, {recursive: true});
let checks = 0;
async function run() {
  const browser = await chromium.launch({channel: process.env.CANVIEW_BROWSER_CHANNEL || "msedge", headless: true});
  try {
    const page = await browser.newPage({viewport: {width: 320, height: 480}, reducedMotion: "reduce"});
    const errors = [];
    const external = [];
    page.on("pageerror", (error) => errors.push(error.message));
    await page.route(/^https?:/, (route) => { external.push(route.request().url()); return route.abort(); });
    await page.clock.install({time: new Date("2026-09-06T03:00:00Z")});
    async function open(params) {
      await page.goto(pathToFileURL(path.join(root, "ui/prototype/index.html")).href + "?" + params);
    }
    function check(value, message) { assert.ok(value, message); checks += 1; }
    for (const screen of ["drive", "audio", "fft", "automation", "settings"]) {
      await open("screen=" + screen + (screen === "settings" ? "&parked=1" : ""));
      check(await page.locator(".screen-panel.is-active").getAttribute("data-screen") === screen, screen + " 선택");
      check(await page.locator(".speed-limit-overlay").isHidden(), "기본 경고 없음");
      check(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth), "가로 overflow 없음");
      check(await page.locator("input[type='text'], input[type='number']").count() === 0, "자유 숫자 입력 없음");
      check(await page.locator(".brand").innerText().then((value) => value.includes("DEMO")), "실차와 데모 구분");
      const badTargets = await page.locator("button, select, input").evaluateAll((nodes) => nodes.filter((node) => {
        const box = node.getBoundingClientRect();
        return box.width && box.height && (box.width < 44 || box.height < 44);
      }).map((node) => node.id || node.className));
      check(badTargets.length === 0, "44px 조작 영역: " + badTargets.join(","));
      await page.screenshot({path: path.join(output, "ui-" + screen + ".png"), animations: "disabled"});
      if (screen === "settings") {
        await page.locator(".settings-panel").evaluate((node) => { node.scrollTop = node.scrollHeight; });
        await page.screenshot({path: path.join(output, "ui-settings-automation.png"), animations: "disabled"});
      }
    }
    await open("screen=fft");
    check(await page.locator("[data-signal='speed']").innerText() === "82", "FFT 차속");
    check(await page.locator("[data-signal='rpm']").innerText() === "1840", "FFT RPM");
    check(await page.locator(".spectrum-card--full .is-peak").getAttribute("x") === "171", "1.25 kHz 좌표");
    check(await page.locator(".spectrum-card--full .chart-metrics").innerText().then((value) => value.includes("dBFS")), "보정되지 않은 SPL을 사칭하지 않음");
    await open("screen=drive&stale=torque-fl,tpms-fr,rpm");
    check(await page.locator(".wheel-meter--fl em").innerText() === "—", "FL 구동 unavailable");
    check(await page.locator(".wheel-meter--fl strong").innerText().then((value) => value.includes("34.2")), "FL 압력은 독립 유지");
    check(await page.locator(".wheel-meter--fr strong").innerText() === "— psi", "FR 압력 unavailable");
    check(await page.locator(".round-gauge:nth-child(2) strong").innerText() === "—", "RPM unavailable");
    for (const state of ["offline", "candidate"]) {
      await open("screen=fft&state=" + state + "&warning=1&headlamp=1");
      check(await page.locator("[data-speed-value]").innerText() === "—", "invalid 차속 숨김");
      check(await page.locator(".speed-limit-overlay").isHidden(), "invalid 제한 경고 숨김");
      check(await page.locator(".headlamp-warning-overlay").isHidden(), "invalid 전조등 경고 숨김");
      check(await page.locator(".spectrum-card--full").getAttribute("data-quality") === "unavailable", "invalid FFT 비움");
    }
    for (const mode of ["normal", "sport", "eco"]) {
      await open("screen=automation&mode=" + mode);
      check(await page.locator(".mode-field strong").innerText() === mode.toUpperCase(), "실제 모드 공유");
      check(await page.locator(".mode-line strong").innerText() === mode.toUpperCase(), "탭간 모드 동일");
    }
    await open("screen=audio&warning=1&headlamp=1");
    check(await page.locator(".app").getAttribute("class").then((value) => value.includes("is-speed-warning")), "과속 fixture");
    check(await page.locator(".headlamp-warning-overlay").isHidden(), "과속 우선");
    check(await page.locator(".speed-limit-overlay").evaluate((node) => getComputedStyle(node).pointerEvents) === "none", "경고 터치 통과");
    await page.locator("[data-profile='quiet']").click();
    await page.locator("[data-profile='rear']").click();
    check(await page.locator("[data-profile][aria-pressed='true']").count() === 1, "프로필 상호 배타");
    await page.locator("[data-target='drive']").click();
    check(await page.locator("#screen-drive").isVisible(), "경고 중 탐색");
    check(await page.locator("[data-target='drive']").getAttribute("aria-selected") === "true", "경고 중 주행 탭 선택");
    // 조작 toast와 가상 clock의 탭 진입 animation을 검토용 사진에 섞지 않는다.
    await open("screen=drive&warning=1");
    await page.screenshot({path: path.join(output, "ui-drive-warning.png"), animations: "disabled"});
    check(await page.locator("#screen-drive").evaluate((node) => getComputedStyle(node).opacity) === "1", "경고 뒤 본문 불투명 표시");
    await open("screen=drive&warning=1&stale=speed");
    check(!(await page.locator(".app").getAttribute("class")).includes("is-speed-warning"), "stale 차속 과속 금지");
    await open("screen=settings&parked=1");
    check(await page.locator("#clock-minute option").count() === 60, "0..59분");
    await page.locator("#clock-year").selectOption("2028");
    await page.locator("#clock-month").selectOption("02");
    check(await page.locator("#clock-day option").count() === 29, "윤년 29일");
    await page.locator("#clock-day").selectOption("29");
    await page.locator("#clock-year").selectOption("2027");
    check(await page.locator("#clock-day").inputValue() === "28", "날짜 경계 clamp");
    await page.locator("#clock-minute").selectOption("59");
    check(await page.locator(".rtc-value").innerText() === "12:00 · 데모", "draft는 적용 전 상태 불변");
    await page.locator("#clock-apply").click();
    check(await page.locator(".rtc-value").innerText() === "12:59 · 데모", "명시적 미리보기 적용");
    await page.locator("[data-setting-toggle='sport']").click();
    check(await page.locator(".sport-toggle").getAttribute("aria-pressed") === "false", "자동화 설정 동기화");
    await open("screen=settings");
    check(await page.locator("#clock-apply").isDisabled(), "주행 중 설정 금지");
    await open("screen=settings&parked=1&stale=speed");
    check(await page.locator("#clock-apply").isDisabled(), "속도 미수신도 설정 잠금");
    await open("screen=audio");
    await page.clock.runFor(30001);
    check(await page.locator(".app").getAttribute("class").then((value) => value.includes("is-idle")), "30초 idle");
    check(await page.locator("#screen-drive").isVisible(), "idle 기본 화면 복귀");
    await page.locator("[data-target='fft']").click();
    check(!(await page.locator(".app").getAttribute("class")).includes("is-idle"), "터치 밝기 복귀");
    check(errors.length === 0, "JS 예외: " + errors.join(", "));
    await open("screen=drive");
    check(await page.locator(".dpf-card > strong").innerText() === "", "lamp off를 전체 DPF 정상으로 승격하지 않음");
    check(external.length === 0, "외부 폰트·네트워크 의존: " + external.join(", "));
    console.log(JSON.stringify({suite: "driver-browser", checks, errors, externalRequests: external.length, screenshots: output}));
    const {runDiagnosticTests} = require('../../tests/ui/diagnostic-browser.cjs');
    console.log(JSON.stringify(await runDiagnosticTests(browser, {screenshotDir: output})));
  } finally { await browser.close(); }
}
run().catch((error) => { console.error(error); process.exitCode = 1; });
