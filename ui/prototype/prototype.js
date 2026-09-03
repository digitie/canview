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

function setToggle(button, enabled) {
  button.setAttribute("aria-pressed", String(enabled));
  button.textContent = enabled ? "사용" : "끔";
}

document.querySelectorAll(".adaptive-toggle, .sport-toggle").forEach((button) => {
  button.addEventListener("click", () => {
    const next = button.getAttribute("aria-pressed") !== "true";
    setToggle(button, next);
    const target = button.classList.contains("adaptive-toggle") ? "noise" : "sport";
    const settingButton = document.querySelector(`[data-setting-toggle="${target}"]`);
    if (settingButton) setToggle(settingButton, next);
  });
});

document.querySelectorAll("[data-setting-toggle]").forEach((button) => {
  button.addEventListener("click", () => {
    const next = button.getAttribute("aria-pressed") !== "true";
    setToggle(button, next);
    if (button.dataset.settingToggle === "brightness") brightness.disabled = next;
    if (button.dataset.settingToggle === "noise") {
      const audioButton = document.querySelector(".adaptive-toggle");
      if (audioButton) setToggle(audioButton, next);
    }
    if (button.dataset.settingToggle === "sport") {
      const sportButton = document.querySelector(".sport-toggle");
      if (sportButton) setToggle(sportButton, next);
    }
  });
});

document.querySelectorAll("[data-cycle]").forEach((button) => {
  button.addEventListener("click", () => {
    const options = button.dataset.options.split("|");
    const current = options.indexOf(button.textContent.trim());
    button.textContent = options[(current + 1) % options.length];
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
brightness.disabled = document.querySelector('[data-setting-toggle="brightness"]')?.getAttribute("aria-pressed") === "true";
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

const requestedSection = new URL(window.location.href).searchParams.get("section");
if (requestedScreen === "settings" && requestedSection) {
  const target = document.querySelector(`#${requestedSection}-settings`)?.closest(".settings-group");
  const settingsPanel = document.querySelector(".settings-panel");
  if (target && settingsPanel) settingsPanel.scrollTop = Math.max(0, target.offsetTop - 8);
}
