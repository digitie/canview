const app = document.querySelector(".app");
const stack = document.querySelector(".screen-stack");
const panels = [...document.querySelectorAll(".screen-panel")];
const navButtons = [...document.querySelectorAll(".nav-button")];
const touchScreens = new Set(["audio", "automation", "settings"]);
let idleTimer;

function showScreen(name) {
  const selected = panels.find((panel) => panel.dataset.screen === name) ?? panels[0];
  panels.forEach((panel) => {
    const active = panel === selected;
    panel.classList.toggle("is-active", active);
    panel.hidden = !active;
  });
  navButtons.forEach((button) => {
    button.setAttribute("aria-selected", String(button.dataset.target === selected.dataset.screen));
  });
  stack.dataset.currentScreen = selected.dataset.screen;
  stack.classList.toggle("is-touch-screen", touchScreens.has(selected.dataset.screen));
}

function resetIdle() {
  app.classList.remove("is-idle");
  window.clearTimeout(idleTimer);
  const selected = document.querySelector("#idle-timeout option:checked");
  const delay = Number.parseInt(selected?.textContent ?? "30", 10) * 1000;
  idleTimer = window.setTimeout(() => {
    app.classList.add("is-idle");
    showScreen("drive");
  }, delay);
}

navButtons.forEach((button) => button.addEventListener("click", () => showScreen(button.dataset.target)));

document.querySelectorAll("[aria-pressed]").forEach((button) => {
  button.addEventListener("click", () => {
    const pressed = button.getAttribute("aria-pressed") === "true";
    button.setAttribute("aria-pressed", String(!pressed));
    if (button.matches("[data-setting-toggle]")) {
      button.textContent = pressed ? "끔" : "사용";
    }
    if (button.classList.contains("sport-toggle")) {
      button.textContent = pressed ? "자동 전환 꺼짐" : "자동 전환 사용";
    }
  });
});

const brightness = document.querySelector("#brightness");
brightness.addEventListener("input", () => {
  document.querySelector(".brightness-value").value = `${brightness.value}%`;
});

document.addEventListener("pointerdown", resetIdle, {passive: true});
document.querySelector("#idle-timeout").addEventListener("change", resetIdle);

const query = new URLSearchParams(window.location.search);
const requestedScreen = query.get("screen");
showScreen(requestedScreen ?? "drive");
if (query.get("scroll") === "bottom") {
  document.querySelector("#screen-settings").scrollTop = 1000;
}
resetIdle();
