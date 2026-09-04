const views = [...document.querySelectorAll("[data-view]")];
const navButtons = [...document.querySelectorAll(".service-nav [data-target]")];
const validViews = new Set(views.map((view) => view.dataset.view));

function showView(name, updateUrl = true) {
  const targetName = validViews.has(name) ? name : "status";
  views.forEach((view) => {
    const active = view.dataset.view === targetName;
    view.hidden = !active;
    view.classList.toggle("is-active", active);
  });
  navButtons.forEach((button) => {
    const active = button.dataset.target === targetName;
    button.classList.toggle("is-active", active);
    button.setAttribute("aria-selected", String(active));
  });
  if (updateUrl) {
    const url = new URL(window.location.href);
    url.searchParams.set("screen", targetName);
    window.history.replaceState({}, "", url);
  }
  document.querySelector(".service-content").scrollTop = 0;
  window.scrollTo({top: 0, behavior: "auto"});
}

navButtons.forEach((button) => {
  button.addEventListener("click", () => showView(button.dataset.target));
});

document.querySelectorAll("[data-open-view]").forEach((button) => {
  button.addEventListener("click", () => showView(button.dataset.openView));
});

document.querySelectorAll(".template-row").forEach((row) => {
  row.addEventListener("click", () => {
    document.querySelectorAll(".template-row").forEach((candidate) => candidate.classList.remove("is-selected"));
    row.classList.add("is-selected");
  });
});

document.querySelectorAll(".switch").forEach((button) => {
  button.addEventListener("click", () => {
    const enabled = button.getAttribute("aria-pressed") === "true";
    button.setAttribute("aria-pressed", String(!enabled));
    button.classList.toggle("is-on", !enabled);
  });
});

const freezeButton = document.querySelector(".freeze-button");
freezeButton?.addEventListener("click", () => {
  const frozen = freezeButton.getAttribute("aria-pressed") === "true";
  freezeButton.setAttribute("aria-pressed", String(!frozen));
  freezeButton.textContent = frozen ? "화면 고정" : "고정됨";
});

const changedBits = new Set([2, 7, 12, 20, 35, 42, 57]);
const correlatedBits = new Set([12, 35]);
const selectedBits = new Set([12]);
const bitMap = document.querySelector("[data-bit-map]");

for (let byte = 0; byte < 8; byte += 1) {
  const row = document.createElement("div");
  row.className = "bit-row";
  const byteLabel = document.createElement("span");
  byteLabel.textContent = `B${byte}`;
  row.append(byteLabel);

  for (let displayBit = 7; displayBit >= 0; displayBit -= 1) {
    const bit = byte * 8 + displayBit;
    const cell = document.createElement("button");
    cell.type = "button";
    cell.className = "bit-cell";
    cell.title = `bit ${bit}`;
    cell.setAttribute("aria-label", `bit ${bit}`);
    cell.classList.toggle("is-changed", changedBits.has(bit));
    cell.classList.toggle("is-correlated", correlatedBits.has(bit));
    cell.classList.toggle("is-selected", selectedBits.has(bit));
    cell.addEventListener("click", () => {
      bitMap.querySelectorAll(".bit-cell").forEach((item) => item.classList.remove("is-selected"));
      cell.classList.add("is-selected");
    });
    row.append(cell);
  }
  bitMap.append(row);
}

document.querySelector("[data-capture-start]")?.addEventListener("click", (event) => {
  event.currentTarget.textContent = "장치 확인 중…";
  event.currentTarget.disabled = true;
  window.setTimeout(() => {
    event.currentTarget.textContent = "캡처 준비 완료";
  }, 500);
});

const requestedView = new URLSearchParams(window.location.search).get("screen");
showView(requestedView ?? "status", false);
