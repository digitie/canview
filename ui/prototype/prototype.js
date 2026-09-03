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

document.querySelectorAll(".adaptive-toggle, .sport-toggle").forEach((button) => {
  button.addEventListener("click", () => {
    const next = button.getAttribute("aria-pressed") !== "true";
    button.setAttribute("aria-pressed", String(next));
    button.textContent = button.classList.contains("sport-toggle")
      ? (next ? "감시 중" : "사용 안 함")
      : (next ? "사용 중" : "꺼짐");
  });
});

const focusPuck = document.querySelector(".sound-focus");
let focusX = 0;
let focusY = 0;
document.querySelectorAll("[data-direction]").forEach((button) => {
  button.addEventListener("click", () => {
    const direction = button.dataset.direction;
    if (direction === "left") {
      focusX = Math.max(-18, focusX - 6);
    } else if (direction === "right") {
      focusX = Math.min(18, focusX + 6);
    } else if (direction === "up") {
      focusY = Math.max(-18, focusY - 6);
    } else if (direction === "down") {
      focusY = Math.min(18, focusY + 6);
    }
    focusPuck.style.transform = `translate(${focusX}px, ${focusY}px)`;
  });
});

const requestedScreen = new URL(window.location.href).searchParams.get("screen");
selectScreen(requestedScreen, false);
