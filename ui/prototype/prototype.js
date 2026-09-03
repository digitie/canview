const screens = [...document.querySelectorAll("[data-screen]")];
const tabs = [...document.querySelectorAll("[data-target]")];

function selectScreen(name, updateUrl = true) {
  const next = screens.find((screen) => screen.dataset.screen === name) ?? screens[0];
  screens.forEach((screen) => {
    const active = screen === next;
    screen.classList.toggle("is-active", active);
    screen.hidden = !active;
  });
  tabs.forEach((tab) => tab.setAttribute("aria-selected", String(tab.dataset.target === next.dataset.screen)));
  if (updateUrl) {
    const url = new URL(window.location.href);
    url.searchParams.set("screen", next.dataset.screen);
    window.history.replaceState({}, "", url);
  }
}

tabs.forEach((tab) => tab.addEventListener("click", () => selectScreen(tab.dataset.target)));

document.querySelectorAll("[data-profile]").forEach((button) => {
  button.addEventListener("click", () => {
    const next = button.getAttribute("aria-pressed") !== "true";
    document.querySelectorAll("[data-profile]").forEach((item) => item.setAttribute("aria-pressed", "false"));
    button.setAttribute("aria-pressed", String(next));
  });
});

document.querySelectorAll(".adaptive-toggle, .sport-toggle, .setting-toggle").forEach((button) => {
  button.addEventListener("click", () => {
    const next = button.getAttribute("aria-pressed") !== "true";
    button.setAttribute("aria-pressed", String(next));
    button.textContent = next ? "사용" : "끔";
    if (button.classList.contains("setting-toggle")) {
      brightness.disabled = next;
    }
  });
});

let volume = 18;
const volumeValue = document.querySelector("[data-volume-value]");
document.querySelectorAll("[data-volume-step]").forEach((button) => {
  button.addEventListener("click", () => {
    volume = Math.max(0, Math.min(40, volume + Number(button.dataset.volumeStep)));
    volumeValue.textContent = String(volume);
  });
});

const brightness = document.querySelector("#brightness");
const brightnessValue = document.querySelector(".brightness-value");
brightness.addEventListener("input", () => {
  brightnessValue.value = `${brightness.value}%`;
});

document.querySelectorAll("[data-unit]").forEach((button) => {
  button.addEventListener("click", () => {
    document.querySelectorAll("[data-unit]").forEach((item) => item.setAttribute("aria-pressed", String(item === button)));
    const metric = button.dataset.unit === "metric";
    document.querySelectorAll("[data-speed-value]").forEach((item) => { item.textContent = metric ? "82" : "51"; });
    document.querySelectorAll("[data-speed-unit]").forEach((item) => { item.textContent = metric ? "km/h" : "mph"; });
  });
});

const requestedScreen = new URL(window.location.href).searchParams.get("screen");
selectScreen(requestedScreen, false);
