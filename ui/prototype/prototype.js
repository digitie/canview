/* 브라우저 전용 데모: CAN·RTC 전송 없이 화면 상태만 검토한다. 장치 APPLIED가 아니다. */
"use strict";
const app = document.querySelector(".app");
const stack = document.querySelector(".screen-stack");
const panels = [...document.querySelectorAll(".screen-panel")];
const navButtons = [...document.querySelectorAll(".nav-button")];
const query = new URLSearchParams(location.search);
const offline = query.get("state") === "offline";
const candidate = query.get("state") === "candidate";
const unavailable = new Set((query.get("stale") ?? "").split(","));
const valid = (name) => !offline && !candidate && !unavailable.has(name);
const parked = query.get("parked") === "1";
const speed = parked ? 0 : 82;
const mode = ["normal", "sport", "eco"].includes(query.get("mode")) ? query.get("mode") : "normal";
const feedback = document.querySelector(".feedback");
let idleTimer;
let feedbackTimer;
function notify(message) {
  feedback.textContent = message;
  feedback.hidden = false;
  clearTimeout(feedbackTimer);
  feedbackTimer = setTimeout(() => { feedback.hidden = true; }, 1800);
}
function showScreen(name) {
  const selected = panels.find((panel) => panel.dataset.screen === name) ?? panels[0];
  panels.forEach((panel) => {
    const active = panel === selected;
    panel.classList.toggle("is-active", active);
    panel.hidden = !active;
  });
  navButtons.forEach((button) => button.setAttribute("aria-selected", String(button.dataset.target === selected.dataset.screen)));
  stack.dataset.currentScreen = selected.dataset.screen;
  stack.classList.toggle("is-touch-screen", ["audio", "automation", "settings"].includes(selected.dataset.screen));
}
function resetIdle() {
  app.classList.remove("is-idle");
  clearTimeout(idleTimer);
  const seconds = Number.parseInt(document.querySelector("#idle-timeout").value, 10);
  idleTimer = setTimeout(() => {
    app.classList.add("is-idle");
    showScreen("drive");
  }, ([15, 30, 60, 120].includes(seconds) ? seconds : 30) * 1000);
}
function paint(selector, name, value) {
  document.querySelectorAll(selector).forEach((node) => {
    node.textContent = valid(name) ? value : "—";
    node.dataset.quality = valid(name) ? "demo" : "unavailable";
  });
}
// 필드별 품질은 독립이다. 실차 신호 승격은 profile/evidence adapter의 책임이다.
paint("[data-speed-value], [data-signal='speed'], .round-gauge:first-child strong", "speed", speed);
paint(".round-gauge:nth-child(2) strong", "rpm", parked ? "0.8" : "1.8");
paint("[data-signal='rpm']", "rpm", parked ? "800" : "1840");
paint(".vehicle-instant strong", "fuel", parked ? "—" : "17.4");
paint(".rear-coupling", "coupling", "R 38%");
paint(".dpf-card > strong", "dpf", "");
paint(".dpf-card > span:last-child", "dpf", "34%");
paint(".drive-aux-metrics span:nth-child(1) b", "battery", "12.6 V");
paint(".drive-aux-metrics span:nth-child(2) b", "lock", "ON");
paint(".drive-aux-metrics span:nth-child(3) b", "temperature", "86 °C");
paint(".volume-card > strong", "volume", "18");
paint(".volume-card b", "volume", "+2");
paint(".chart-metrics > div:first-child strong", "fft", "1.25 kHz");
paint(".chart-metrics > div:last-child strong", "fft", "−24.6 dBFS");
document.querySelectorAll(".spectrum-chart").forEach((node) => node.setAttribute("aria-label", valid("fft") ? "실내 FFT 데모, 피크 1.25킬로헤르츠, 디지털 상대 레벨 마이너스 24.6 dBFS" : "FFT 수신값 없음"));
document.querySelectorAll(".spectrum-card").forEach((node) => { node.dataset.quality = valid("fft") ? "demo" : "unavailable"; });
// 1/3 옥타브 중심 50…8000 Hz, 14번 bin=1250 Hz. 축·막대·peak가 같은 좌표를 쓴다.
const spectrumHeights = [12, 18, 22, 20, 26, 33, 40, 32, 28, 36, 48, 56, 68, 81, 100, 76, 51, 39, 30, 23, 17, 12, 8];
document.querySelectorAll(".spectrum-chart").forEach((chart) => {
  const baseline = chart.viewBox.baseVal.height === 260 ? 243 : 120;
  const maximum = baseline === 243 ? 198 : 99;
  chart.querySelectorAll(".spectrum-bars rect").forEach((bar, index) => {
    const height = maximum * spectrumHeights[index] / 100;
    bar.setAttribute("y", String(baseline - height));
    bar.setAttribute("height", String(height));
    bar.classList.toggle("is-peak", index === 14);
  });
  chart.querySelector(".peak-marker")?.setAttribute("d", "M174.5 38V248");
});
document.querySelectorAll(".wheel-meter").forEach((node, index) => {
  const key = ["fl", "fr", "rl", "rr"][index];
  const available = valid("torque") && valid("torque-" + key);
  node.dataset.quality = available ? "demo" : "unavailable";
  if (!available) node.querySelector("em").textContent = "—";
  if (!valid("tpms") || !valid("tpms-" + key)) node.querySelector("strong").textContent = "— psi";
});
document.querySelectorAll(".round-gauge").forEach((node, index) => {
  const field = index === 0 ? "speed" : "rpm";
  if (!valid(field)) node.style.setProperty("--gauge", "0%");
  else if (parked) node.style.setProperty("--gauge", index === 0 ? "0%" : "12%");
  node.setAttribute("aria-label", field + " " + node.querySelector("strong").textContent + " " + node.querySelector("small").textContent);
});
if (!valid("dpf")) document.querySelector(".dpf-meter i").style.setProperty("--dpf", "0%");
document.querySelector(".dpf-card").setAttribute("aria-label", valid("dpf") ? "DPF 데모 상태" : "DPF 수신값 없음");
document.querySelector(".vehicle-instant").setAttribute("aria-label", "순간 연비 " + document.querySelector(".vehicle-instant strong").textContent + " km/L");
document.querySelector(".volume-card").setAttribute("aria-label", "현재 음량 " + document.querySelector(".volume-card > strong").textContent);
document.querySelector(".persistent-speed").setAttribute("aria-label", "현재 차속 " + (valid("speed") ? speed : "수신값 없음") + " km/h");
document.querySelector(".link-summary").textContent = offline ? "연결 끊김" : candidate ? "미확정" : "샘플";
document.querySelector(".link-summary").setAttribute("aria-label", "실차 연결이 아닌 화면 검토용 데이터");
paint(".mode-line strong, .mode-field strong", "mode", mode.toUpperCase());
document.querySelector(".sport-card").dataset.driveMode = valid("mode") ? mode : "unknown";
document.querySelector(".mode-line strong").className = valid("mode") ? "mode-" + mode : "";
paint(".sport-return strong", "mode", mode === "sport" ? "ECO" : "—");
paint(".sport-head b", "mode", mode === "sport" ? "활성" : "대기");
const hasLimit = query.get("limit") === "1" || query.get("warning") === "1";
const speedWarning = hasLimit && query.get("warning") === "1" && valid("speed") && valid("limit") && speed >= 77;
document.querySelector(".speed-limit-overlay").hidden = !hasLimit || !valid("limit");
app.classList.toggle("has-limit", hasLimit && valid("limit"));
app.classList.toggle("is-speed-warning", speedWarning);
document.querySelector(".headlamp-warning-overlay").hidden = !(query.get("headlamp") === "1" && valid("lights") && valid("rtc") && valid("solar") && !speedWarning);
// 경고 query는 상태기계 출력의 fixture다. 실시간 과속/일몰 판정 코드가 아니다.
if (query.get("night") === "1" && valid("lights")) app.style.setProperty("--preview-brightness", ".72");
navButtons.forEach((button) => button.addEventListener("click", () => showScreen(button.dataset.target)));
navButtons.forEach((button, index) => button.addEventListener("keydown", (event) => {
  if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
  event.preventDefault();
  const next = event.key === "Home" ? 0 : event.key === "End" ? 4 : (index + (event.key === "ArrowRight" ? 1 : 4)) % 5;
  navButtons[next].focus();
  showScreen(navButtons[next].dataset.target);
}));
function setToggle(button, enabled) {
  button.setAttribute("aria-pressed", String(enabled));
  if (button.dataset.settingToggle) button.textContent = enabled ? "사용" : "끔";
  if (button.classList.contains("sport-toggle")) button.textContent = enabled ? "자동 전환 사용" : "자동 전환 꺼짐";
}
document.querySelectorAll("[aria-pressed]").forEach((button) => {
  if (button.dataset.profile) setToggle(button, false);
  button.addEventListener("click", () => {
    if (offline || candidate) { notify("장치 연결 필요"); return; }
    const enabled = button.getAttribute("aria-pressed") !== "true";
    if (button.dataset.profile) document.querySelectorAll("[data-profile]").forEach((node) => setToggle(node, false));
    setToggle(button, enabled);
    if (button.dataset.settingToggle === "sport" || button.classList.contains("sport-toggle")) {
      document.querySelectorAll("[data-setting-toggle='sport'], .sport-toggle").forEach((node) => setToggle(node, enabled));
    }
    notify("데모 설정 변경");
  });
});
const brightness = document.querySelector("#brightness");
brightness.addEventListener("input", () => { document.querySelector(".brightness-value").value = brightness.value + "%"; });
const hour = document.querySelector("#clock-hour");
const minute = document.querySelector("#clock-minute");
const year = document.querySelector("#clock-year");
const month = document.querySelector("#clock-month");
const day = document.querySelector("#clock-day");
function options(select, first, last, suffix = "") {
  select.replaceChildren(...Array.from({length: last - first + 1}, (_, index) => {
    const value = String(index + first).padStart(2, "0");
    return new Option(value + suffix, value);
  }));
}
options(hour, 0, 23); options(minute, 0, 59);
options(year, 2000, 2099, "년"); options(month, 1, 12, "월");
year.value = "2026"; month.value = "09"; hour.value = "12"; minute.value = "00";
function updateDays() {
  const previous = Number(day.value) || 6;
  const count = new Date(Date.UTC(Number(year.value), Number(month.value), 0)).getUTCDate();
  options(day, 1, count, "일");
  day.value = String(Math.min(previous, count)).padStart(2, "0");
}
year.addEventListener("change", updateDays); month.addEventListener("change", updateDays); updateDays();
const clockValue = document.querySelector(".rtc-value");
clockValue.value = valid("rtc") ? "12:00 · 데모" : "—";
document.querySelector("#clock-apply").addEventListener("click", () => {
  clockValue.value = hour.value + ":" + minute.value + " · 데모";
  notify("시각 미리보기 적용");
});
// 시각 선택은 draft이며 실제 제품은 RTC write/readback 결과 후 적용한다.
const locked = !parked || !valid("speed");
document.querySelector(".settings-lock").hidden = !locked;
document.querySelectorAll(".settings-panel button, .settings-panel select, .settings-panel input, .sport-toggle").forEach((node) => { node.disabled = locked; });
document.querySelectorAll(".settings-panel select").forEach((node) => node.addEventListener("change", () => {
  if (!node.id.startsWith("clock-")) notify("데모 설정 변경");
}));
document.querySelector("#idle-timeout").addEventListener("change", resetIdle);
document.addEventListener("pointerdown", resetIdle, {passive: true});
document.addEventListener("keydown", resetIdle);
showScreen(query.get("screen") ?? "drive");
if (query.get("scroll") === "bottom") document.querySelector("#screen-settings").scrollTop = 10000;
resetIdle();
