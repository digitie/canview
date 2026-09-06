/* 오프라인 합성 샘플 전용. 네트워크·영속 저장·차량 송신 경로 없음. */
(() => {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const all = (selector) => [...document.querySelectorAll(selector)];
  const views = all("[data-view]");
  const qualityText = {
    sample: "SAMPLE · 합성 데이터 · 실제 장치 미연결",
    stale: "STALE · 마지막 샘플 30초 경과 예시 · 현재값·저장 잠금",
    gap: "GAP · 샘플 2건 누락 예시 · coverage 불완전 · 저장 잠금",
    malformed: "REJECTED · MALFORMED · DLC와 payload 길이 불일치",
    disconnected: "DISCONNECTED · 샘플 공급 중단 · 현재 데이터 없음"
  };
  // 동일 ID라도 버스·11/29-bit·DLC variant를 합치지 않는다.
  const fixtures = [
    {bus: 1, id: 0x541, extended: false, data: [0x3A, 0, 0x91, 0x10, 0x20, 4, 0x8C, 0x6F], flip: 12, rate: 20, age: 18, isNew: false},
    {bus: 1, id: 0x386, extended: false, data: [0x1B, 0x32, 0x1C, 0x34, 0x1A, 0xF0, 0x1B, 1], flip: 20, rate: 50, age: 8, isNew: false},
    {bus: 2, id: 0x1EB, extended: false, data: [0, 0x44, 0x10, 0, 0x0C, 0, 0x21, 0x9A], flip: 35, rate: 10, age: 42, isNew: true},
    {bus: 3, id: 0x593, extended: false, data: [0x23, 0x22, 0x22, 0x23, 0, 0x18], flip: 2, rate: 1, age: 310, isNew: false},
    {bus: 1, id: 0x541, extended: true, data: [0x80, 1, 2, 3, 4, 5, 6, 7], flip: null, rate: 2, age: 120, isNew: true},
    {bus: 1, id: 0x541, extended: false, data: [0xFF, 0x80], flip: 7, rate: 5, age: 50, isNew: false}
  ].map((frame) => ({...frame, dlc: frame.data.length}));
  const frameKey = (frame) => [frame.bus, frame.extended ? 1 : 0, frame.id, frame.dlc].join(":");
  const hexId = (frame) => "0x" + frame.id.toString(16).toUpperCase().padStart(frame.extended ? 8 : 3, "0");
  const identity = (frame) => "CAN " + frame.bus + " · " + hexId(frame) + " · " + (frame.extended ? "29-bit" : "11-bit") + " · DLC " + frame.dlc;
  const rawText = (data) => data.map((byte) => byte.toString(16).toUpperCase().padStart(2, "0")).join(" ");
  const state = {demo: false, quality: "disconnected", tick: 0, frozenTick: null, frame: frameKey(fixtures[0]), analysisPair: null, candidates: [], settingsTarget: "bridge", apCount: 0};
  const capture = {phase: "idle", marks: 0, frame: null, template: null, before: [], after: []};
  const timers = new Map();
  const units = new Set(["raw", "bool", "%", "km/h", "rpm", "°C", "V"]);
  const templates = {lights: "미등·전조등", door: "문·잠금", lock: "4WD LOCK", pedal: "가속·속도"};

  function status(id, phase, message) {
    $(id).dataset.state = phase;
    $(id).textContent = message;
  }
  function showView(name, updateUrl = true) {
    const activeName = views.some((view) => view.dataset.view === name) ? name : "status";
    views.forEach((view) => { view.hidden = view.dataset.view !== activeName; });
    all(".service-nav [data-target]").forEach((button) => {
      if (button.dataset.target === activeName) button.setAttribute("aria-current", "page");
      else button.removeAttribute("aria-current");
    });
    if (updateUrl) {
      const url = new URL(location.href);
      url.searchParams.set("screen", activeName);
      history.replaceState({}, "", url);
      document.querySelector('[data-view="' + activeName + '"] h1').focus({preventScroll: true});
      window.scrollTo({top: document.querySelector('[data-view="' + activeName + '"]').offsetTop, behavior: "auto"});
    }
  }
  function cancelTimer(key) {
    if (timers.has(key)) clearTimeout(timers.get(key));
    timers.delete(key);
  }
  function later(key, callback) {
    cancelTimer(key);
    timers.set(key, setTimeout(() => { timers.delete(key); callback(); }, 500));
  }
  // 대상 DOM을 동기 시점에 보관한다. event를 비동기 callback에 넘기지 않는다.
  function checkUnavailable(buttonId, resultId) {
    const button = $(buttonId);
    button.disabled = true;
    status(resultId, "pending", "PENDING · 응답 확인 절차 · 실제 요청은 전송되지 않음");
    later(resultId, () => {
      button.disabled = false;
      status(resultId, "rejected", "REJECTED · NO_BACKEND · 장치 응답 없음. 변경되지 않았습니다.");
    });
  }
  function dataAvailable() { return state.demo && ["sample", "stale", "gap"].includes(state.quality); }
  function usable() { return state.demo && state.quality === "sample"; }
  function sampleAt(frame, tick = state.tick) {
    const data = [...frame.data];
    if (frame.flip !== null && tick % 2) data[Math.floor(frame.flip / 8)] ^= 1 << (frame.flip % 8);
    return {...frame, data};
  }
  function validateFrame(frame) {
    if (!frame || !Number.isInteger(frame.bus) || frame.bus < 1 || frame.bus > 3 ||
        typeof frame.extended !== "boolean" || !Number.isInteger(frame.id) || frame.id < 0 ||
        frame.id > (frame.extended ? 0x1FFFFFFF : 0x7FF) ||
        !Number.isInteger(frame.dlc) || frame.dlc < 1 || frame.dlc > 8 ||
        !Array.isArray(frame.data) || frame.data.length !== frame.dlc ||
        frame.data.some((byte) => !Number.isInteger(byte) || byte < 0 || byte > 255)) {
      throw new Error("잘못된 frame · ID·DLC·payload 형식을 확인하세요.");
    }
    return frame;
  }
  function selectedFrame() {
    if (!dataAvailable()) return null;
    const base = fixtures.find((frame) => frameKey(frame) === state.frame);
    if (!base) return null;
    return validateFrame({...base, data: analysisData(base, state.tick)});
  }
  function analysisData(frame, tick) {
    if (state.analysisPair?.key === frameKey(frame)) {
      return [...(tick % 2 ? state.analysisPair.after : state.analysisPair.before)];
    }
    return sampleAt(fixtures.find((base) => frameKey(base) === frameKey(frame)), tick).data;
  }
  function fillFrameSelectors() {
    ["signal-frame", "capture-frame"].forEach((id) => {
      const previous = $(id).value;
      $(id).replaceChildren();
      if (!dataAvailable()) {
        $(id).append(new Option("샘플 없음 · 상단에서 샘플 열기", ""));
      } else {
        fixtures.forEach((frame) => $(id).append(new Option(identity(frame), frameKey(frame))));
        $(id).value = id === "signal-frame" ? state.frame : (fixtures.some((frame) => frameKey(frame) === previous) ? previous : state.frame);
      }
      $(id).disabled = !dataAvailable();
    });
  }
  function resetCapture(message) {
    cancelTimer("capture");
    capture.phase = "idle";
    capture.marks = 0;
    capture.frame = null;
    $("capture-result").hidden = true;
    status("capture-status", usable() ? "local" : "disconnected", message || (usable() ? "LOCAL ONLY · 합성 샘플 연습을 시작할 수 있습니다." : "DISCONNECTED · 실제 캡처 실행 불가"));
    renderCaptureControls();
  }
  function setSource() {
    state.analysisPair = null;
    state.quality = state.demo ? $("sample-quality").value : "disconnected";
    if (!Object.hasOwn(qualityText, state.quality)) state.quality = "disconnected";
    state.frozenTick = null;
    $("freeze").setAttribute("aria-pressed", "false");
    $("freeze").textContent = "화면 고정";
    $("source-status").textContent = qualityText[state.quality];
    $("sample-quality").disabled = !state.demo;
    $("demo-toggle").setAttribute("aria-pressed", String(state.demo));
    $("demo-toggle").textContent = state.demo ? "샘플 닫기 · 로컬 후보 유지" : "로컬 샘플 열기";
    if (state.quality === "malformed") {
      try { validateFrame({...fixtures[0], dlc: 7}); }
      catch (error) { $("source-status").textContent += " · " + error.message; }
    }
    fillFrameSelectors();
    resetCapture(state.quality === "sample" ? undefined : qualityText[state.quality] + " · 연습 중단");
    renderFrames();
    renderSignal();
    $("ap-rehearse").disabled = !usable();
    // 진행 중 저장 dialog의 조건도 무효화한다.
    if ($("candidate-dialog").open) {
      $("candidate-save").disabled = true;
      status("candidate-save-status", "rejected", "REJECTED · 데이터 출처가 바뀌었습니다. 닫고 후보를 다시 확인하세요.");
    }
  }
  function renderFrames() {
    const idInput = $("filter-id").value.trim();
    let exactId = null;
    let error = "";
    if (idInput) {
      if (!/^(?:0x)?[0-9a-f]{1,8}$/i.test(idInput) || parseInt(idInput.replace(/^0x/i, ""), 16) > 0x1FFFFFFF) {
        error = "ID는 0x000–0x1FFFFFFF 범위의 완전한 16진수여야 합니다.";
      } else exactId = parseInt(idInput.replace(/^0x/i, ""), 16);
    }
    $("filter-error").textContent = error;
    $("filter-id").setAttribute("aria-invalid", String(Boolean(error)));
    const list = $("frame-list");
    list.replaceChildren();
    let rows = dataAvailable() && !error ? fixtures.map((frame) => sampleAt(frame, state.frozenTick ?? state.tick)) : [];
    rows = rows.filter((frame) =>
      ($("filter-bus").value === "all" || String(frame.bus) === $("filter-bus").value) &&
      ($("filter-format").value === "all" || (frame.extended ? "extended" : "standard") === $("filter-format").value) &&
      ($("filter-state").value === "all" || ($("filter-state").value === "new" ? frame.isNew : frame.flip !== null)) &&
      (exactId === null || frame.id === exactId));
    const sorts = {
      id: (a, b) => a.id - b.id || a.bus - b.bus || Number(a.extended) - Number(b.extended) || a.dlc - b.dlc,
      rate: (a, b) => b.rate - a.rate,
      change: (a, b) => Number(b.flip !== null) - Number(a.flip !== null),
      recent: (a, b) => a.age - b.age
    };
    rows.sort(sorts[$("filter-sort").value] || sorts.id);
    rows.forEach((frame) => {
      const button = document.createElement("button");
      button.type = "button";
      button.className = "frame-row" + (state.quality === "stale" ? " is-stale" : "");
      button.dataset.frameKey = frameKey(frame);
      const title = document.createElement("strong");
      title.textContent = identity(frame);
      const meta = document.createElement("span");
      meta.className = "frame-meta";
      meta.textContent = "SAMPLE · " + frame.rate + " Hz · age " + (state.quality === "stale" ? "30초 이상" : frame.age + " ms 예시");
      const bytes = document.createElement("code");
      bytes.textContent = rawText(frame.data);
      const change = document.createElement("small");
      change.textContent = state.quality.toUpperCase() + " · " + (frame.flip === null ? "변화 없음" : "합성 bit " + frame.flip + " 변화") + (frame.isNew ? " · 신규" : "");
      button.append(title, meta, bytes, change);
      button.addEventListener("click", () => {
        state.analysisPair = null;
        state.frame = frameKey(frame);
        $("signal-frame").value = state.frame;
        // 고정된 inventory 행을 선택하면 그 snapshot을 그대로 분석한다.
        if (state.frozenTick !== null) state.tick = state.frozenTick;
        renderSignal(true);
        showView("signals");
      });
      list.append(button);
    });
    if (!rows.length) {
      const empty = document.createElement("p");
      empty.className = "empty-state";
      empty.textContent = error ? "검색 형식을 고치면 다시 표시합니다." : !dataAvailable() ? qualityText[state.quality] : "일치하는 샘플이 없습니다. 필터를 초기화해 보세요.";
      list.append(empty);
    }
    $("frame-summary").textContent = rows.length + "개 샘플 · " + (state.frozenTick === null ? "수동 snapshot #" + state.tick : "표 고정 #" + state.frozenTick + " · 다음 샘플 #" + state.tick + " 대기") + " · 실제 stream 없음";
    $("sample-next").disabled = !usable();
    $("freeze").disabled = !dataAvailable();
  }
  function decimal(text, label) {
    if (!/^-?\d{1,7}(?:\.\d{1,6})?$/.test(text) || !Number.isFinite(Number(text)) || Math.abs(Number(text)) > 1000000) {
      throw new Error(label + ": 유한 십진수, 소수 6자리, 절댓값 1,000,000 이하로 입력하세요.");
    }
    const [integer, fraction = ""] = text.replace(/^-/, "").split(".");
    const micros = (BigInt(integer) * 1000000n + BigInt(fraction.padEnd(6, "0"))) * (text.startsWith("-") ? -1n : 1n);
    return {value: Number(text), micros};
  }
  function descriptor(frame) {
    validateFrame(frame);
    const start = Number($("start-bit").value);
    const length = Number($("bit-length").value);
    const order = $("byte-order").value;
    const signedChoice = $("signed").value;
    if ($("start-bit").value === "" || $("bit-length").value === "" || !Number.isInteger(start) || start < 0 || start > 63 ||
        !Number.isInteger(length) || length < 1 || length > 64 || !["INTEL", "MOTOROLA"].includes(order) ||
        !["unsigned", "signed"].includes(signedChoice)) throw new Error("시작 bit·길이·Endian·부호 선택을 확인하세요.");
    const bits = [];
    let bit = start;
    for (let i = 0; i < length; i += 1) {
      if (bit < 0 || bit >= frame.dlc * 8) throw new Error("비트 범위가 DLC " + frame.dlc + " 밖입니다. 시작 bit나 길이를 줄이세요.");
      bits.push(bit);
      bit = order === "INTEL" ? bit + 1 : bit % 8 === 0 ? bit + 15 : bit - 1;
    }
    const factor = decimal($("factor-preset").value, "Factor");
    const offset = decimal($("offset-preset").value, "Offset");
    if (factor.micros === 0n) throw new Error("Factor는 0일 수 없습니다.");
    const unit = $("unit").value;
    if (!units.has(unit)) throw new Error("허용된 단위 후보를 선택하세요.");
    const signed = signedChoice === "signed";
    if (unit === "bool" && (length !== 1 || signed || factor.value !== 1 || offset.value !== 0)) {
      throw new Error("bool은 Unsigned 1 bit, Factor 1, Offset 0에서만 사용하세요.");
    }
    return {start_bit: start, bit_length: length, byte_order: order, signed, factor: factor.value, offset: offset.value, unit, bits, factorMicros: factor.micros, offsetMicros: offset.micros};
  }
  function decode(data, config) {
    let raw = 0n;
    config.bits.forEach((bit, index) => {
      const digit = BigInt((data[Math.floor(bit / 8)] >> (bit % 8)) & 1);
      if (config.byte_order === "INTEL") raw |= digit << BigInt(index);
      else raw = (raw << 1n) | digit;
    });
    if (config.signed && (raw & (1n << BigInt(config.bit_length - 1)))) raw -= 1n << BigInt(config.bit_length);
    const physical = raw * config.factorMicros + config.offsetMicros;
    const absolute = physical < 0n ? -physical : physical;
    const fraction = (absolute % 1000000n).toString().padStart(6, "0").replace(/0+$/, "");
    return {raw: raw.toString(), value: (physical < 0n ? "-" : "") + (absolute / 1000000n).toString() + (fraction ? "." + fraction : "")};
  }
  function renderSignal(resetRange = false) {
    const frame = selectedFrame();
    $("signal-identity").textContent = frame ? identity(frame) : "선택 샘플 없음";
    $("raw-quality").textContent = state.quality.toUpperCase();
    $("raw-bytes").textContent = frame ? rawText(frame.data) : "—";
    $("raw-age").textContent = frame ? (state.quality === "stale" ? "마지막 합성 샘플 · 30초 경과 예시" : state.analysisPair ? "캡처 연습의 " + (state.tick % 2 ? "후" : "전") + " 샘플 · 로컬 marker " + state.analysisPair.marks + "회 · 실차 증거 없음" : "수동 합성 snapshot #" + state.tick + " · 실측 age 아님") : qualityText[state.quality];
    if (resetRange) {
      $("start-bit").value = String(Math.min(frame?.flip ?? 0, (frame?.dlc ?? 1) * 8 - 1));
      $("bit-length").value = "1";
    }
    $("decoder").disabled = !frame;
    let config = null;
    let error = "";
    try { if (frame) config = descriptor(frame); }
    catch (problem) { error = problem.message; }
    $("decoder-error").textContent = error;
    $("bit-range").textContent = config ? (config.byte_order === "INTEL" ? "LSB → MSB: " : "MSB → LSB: ") + config.bits.join(" → ") : "비트 선택 범위를 확인하세요.";
    $("bit-map").replaceChildren();
    if (frame) {
      for (let byte = 0; byte < frame.dlc; byte += 1) {
        const row = document.createElement("div");
        row.className = "bit-row";
        const byteLabel = document.createElement("span");
        byteLabel.textContent = "B" + byte;
        row.append(byteLabel);
        for (let bit = 7; bit >= 0; bit -= 1) {
          const index = byte * 8 + bit;
          const cell = document.createElement("button");
          cell.type = "button";
          cell.className = "bit-cell" + (frame.flip === index ? " is-changed" : "");
          cell.textContent = String((frame.data[byte] >> bit) & 1);
          cell.setAttribute("aria-label", "byte " + byte + " bit " + bit + " · 시작 bit " + index);
          cell.setAttribute("aria-pressed", String(Boolean(config?.bits.includes(index))));
          cell.addEventListener("click", () => {
            $("start-bit").value = String(index);
            renderSignal();
            // 재생성한 보조 버튼으로 키보드 focus를 돌려준다.
            $("bit-map").querySelector('[aria-label="byte ' + byte + ' bit ' + bit + ' · 시작 bit ' + index + '"]')?.focus({preventScroll: true});
          });
          row.append(cell);
        }
        $("bit-map").append(row);
      }
    }
    const canDecode = config && state.quality !== "stale";
    const current = canDecode ? decode(frame.data, config) : null;
    $("decoded-value").textContent = current ? current.value : "—";
    $("decoded-unit").textContent = config?.unit ?? "raw";
    $("decoded-detail").textContent = current ? "raw " + current.raw + " × " + config.factor + " + " + config.offset + " · " + state.quality.toUpperCase() : (error || qualityText[state.quality]);
    $("comparison-value").textContent = current ? "비교 샘플: " + decode(analysisData(frame, state.tick + 1), config).value + " " + config.unit + " · 합성 변화, 상관 증거 없음" : "현재값·비교값을 확정할 수 없습니다.";
    $("signal-next").disabled = !usable();
    $("candidate-open").disabled = !(config && usable());
    if ($("candidate-dialog").open) $("candidate-save").disabled = !(config && usable());
  }
  function advanceSample() {
    if (!usable()) return;
    state.tick += 1;
    renderFrames();
    renderSignal();
  }
  function renderCapturePlan() {
    const frame = fixtures.find((item) => frameKey(item) === $("capture-frame").value);
    $("capture-scope").textContent = frame ? identity(frame) + " · exact 1개" : "대상 없음";
    $("capture-duration").textContent = $("capture-mode").value + " · 앞 " + $("capture-pre").value + "초 / 뒤 " + $("capture-post").value + "초";
  }
  function renderCaptureControls() {
    const busy = ["pending", "mark"].includes(capture.phase);
    $("capture-plan").disabled = busy;
    $("capture-start").disabled = busy;
    $("capture-start").textContent = usable() ? "로컬 캡처 절차 연습" : "장치 응답 확인";
    $("capture-mark").disabled = capture.phase !== "mark" || capture.marks >= 3;
    $("capture-finish").disabled = capture.phase !== "mark" || capture.marks < 1;
    $("capture-cancel").disabled = !busy;
    all("[data-step]").forEach((step) => {
      const active = capture.phase === "mark" ? "mark" : capture.phase === "result" ? "result" : "plan";
      if (step.dataset.step === active) step.setAttribute("aria-current", "step");
      else step.removeAttribute("aria-current");
    });
    renderCapturePlan();
  }
  function beginCapture() {
    if (["pending", "mark"].includes(capture.phase)) return;
    $("capture-result").hidden = true;
    capture.phase = "pending";
    capture.marks = 0;
    renderCaptureControls();
    status("capture-status", "pending", usable() ? "PENDING · 합성 기준 샘플 준비 중 · 실제 캡처 없음" : "PENDING · 장치 응답 확인 절차 · 실제 요청 없음");
    const local = usable();
    later("capture", () => {
      if (!local || !usable()) {
        capture.phase = "idle";
        status("capture-status", "rejected", "REJECTED · " + (state.quality === "sample" || !state.demo ? "NO_BACKEND" : state.quality.toUpperCase()) + " · 실제 캡처·준비 확인 없음");
      } else {
        const frame = fixtures.find((item) => frameKey(item) === $("capture-frame").value);
        const template = document.querySelector('input[name="template"]:checked')?.value;
        if (!frame || !Object.hasOwn(templates, template)) {
          capture.phase = "idle";
          status("capture-status", "rejected", "REJECTED · 대상 샘플·행동 선택을 확인하세요.");
        } else {
          capture.frame = sampleAt(frame);
          capture.template = template;
          capture.before = [...capture.frame.data];
          capture.phase = "mark";
          status("capture-status", "local", "LOCAL DEMO · 합성 기준값 " + rawText(capture.before) + " · MARK로 전후 비교 연습");
        }
      }
      renderCaptureControls();
    });
  }
  function saveCandidate() {
    if (!usable()) {
      status("candidate-save-status", "rejected", "REJECTED · 유효한 합성 샘플에서만 후보를 저장할 수 있습니다.");
      return;
    }
    try {
      const frame = selectedFrame();
      const config = descriptor(frame);
      const record = {
        status: "CANDIDATE", grade: "C", source: "SYNTHETIC_DEMO", quality: "SAMPLE",
        bus_id: frame.bus, can_id: frame.id, extended: frame.extended, dlc: frame.dlc,
        start_bit: config.start_bit, bit_length: config.bit_length, byte_order: config.byte_order,
        signed: config.signed, factor: config.factor, offset: config.offset, unit: config.unit,
        evidence_ids: []
      };
      const duplicate = state.candidates.some((candidate) => JSON.stringify(candidate) === JSON.stringify(record));
      if (!duplicate && state.candidates.length >= 20) throw new Error("로컬 후보는 최대 20개입니다. JSON을 내보내고 후보를 비우세요.");
      if (!duplicate) state.candidates.push(record);
      renderCandidates();
      $("candidate-dialog").close();
      status("export-status", "local", duplicate ? "LOCAL ONLY · 같은 후보가 이미 저장되어 있습니다." : "LOCAL ONLY · 후보 C를 이 탭 메모리에 저장했습니다. 장치 저장 없음.");
    } catch (error) {
      status("candidate-save-status", "rejected", "REJECTED · " + error.message);
    }
  }
  function renderCandidates() {
    $("candidate-count").textContent = state.candidates.length + " / 20";
    $("candidate-list").replaceChildren();
    state.candidates.forEach((candidate, index) => {
      const item = document.createElement("li");
      const label = document.createElement("strong");
      label.textContent = "후보 " + (index + 1) + " · C · SYNTHETIC DEMO";
      const summary = document.createElement("span");
      summary.textContent = identity({bus: candidate.bus_id, id: candidate.can_id, extended: candidate.extended, dlc: candidate.dlc}) + " · bit " + candidate.start_bit + " / " + candidate.bit_length + " · " + candidate.byte_order + " · " + (candidate.signed ? "signed" : "unsigned") + " · ×" + candidate.factor + " +" + candidate.offset + " " + candidate.unit;
      item.append(label, summary);
      $("candidate-list").append(item);
    });
    $("candidate-export").disabled = !state.candidates.length;
    $("candidate-clear").disabled = !state.candidates.length;
  }
  function exportCandidates() {
    if (!state.candidates.length) return;
    let url = null;
    let anchor = null;
    try {
      // 명시적 허용 필드만 복사. DOM text, 입력 원문, raw bytes, 설정·비밀은 제외한다.
      const candidates = state.candidates.map((record) => ({
        status: "CANDIDATE", grade: "C", source: "SYNTHETIC_DEMO", quality: "SAMPLE",
        bus_id: record.bus_id, can_id: record.can_id, extended: record.extended, dlc: record.dlc,
        start_bit: record.start_bit, bit_length: record.bit_length, byte_order: record.byte_order,
        signed: record.signed, factor: record.factor, offset: record.offset, unit: record.unit, evidence_ids: []
      }));
      const bundle = {format: "canview-local-candidates", version: 1, prototype: true, operational: false, source: "SYNTHETIC_DEMO", vehicle_evidence: false, candidates};
      url = URL.createObjectURL(new Blob([JSON.stringify(bundle, null, 2)], {type: "application/json"}));
      anchor = document.createElement("a");
      anchor.href = url;
      anchor.download = "canview-demo-candidates.json";
      document.body.append(anchor);
      anchor.click();
      status("export-status", "local", "LOCAL ONLY · JSON 다운로드를 요청했습니다. 브라우저 다운로드 목록에서 확인하세요.");
    } catch {
      status("export-status", "rejected", "REJECTED · JSON 다운로드를 시작하지 못했습니다. 후보는 메모리에 유지됩니다. 다시 시도하세요.");
    } finally {
      anchor?.remove();
      if (url) setTimeout(() => URL.revokeObjectURL(url), 1000);
    }
  }
  function settingsSummary() {
    $("stream-summary").textContent = "로컬 초안 " + $("setting-rate").value + " f/s · " + $("setting-budget").value + " kB/s · 장치 수락 —";
  }
  function saveSettings() {
    if (state.settingsTarget === "controller") {
      status("settings-status", "rejected", "REJECTED · Controller schema·revision·정차 확인 없음.");
      return;
    }
    const ids = state.settingsTarget === "bridge" ? ["setting-ap", "setting-refresh", "setting-retention"] : ["setting-stats", "setting-rate", "setting-budget"];
    const allowed = {"setting-ap": ["5", "10", "20", "30"], "setting-refresh": ["5", "10"], "setting-retention": ["5", "10", "20"], "setting-stats": ["250", "500", "1000", "2000"], "setting-rate": ["20", "50", "100", "150", "200"], "setting-budget": ["2", "4", "8", "12", "20"]};
    if (ids.some((id) => !allowed[id].includes($(id).value))) {
      status("settings-status", "rejected", "REJECTED · 허용된 설정 선택값을 확인하세요.");
      return;
    }
    state[state.settingsTarget + "Draft"] = Object.fromEntries(ids.map((id) => [id, Number($(id).value)]));
    status("settings-status", "local", "LOCAL ONLY · " + state.settingsTarget + " 초안 [" + ids.map((id) => $(id).selectedOptions[0].textContent).join(" · ") + "] 저장 · 이 탭에서만 유지 · 장치 적용 없음");
  }

  all("[data-target]").forEach((button) => button.addEventListener("click", () => showView(button.dataset.target)));
  all("[data-open-view]").forEach((button) => button.addEventListener("click", () => showView(button.dataset.openView)));
  window.addEventListener("popstate", () => showView(new URLSearchParams(location.search).get("screen"), false));
  $("demo-toggle").addEventListener("click", () => { state.demo = !state.demo; if (state.demo) $("sample-quality").value = "sample"; setSource(); });
  $("sample-quality").addEventListener("change", setSource);
  $("connection-check").addEventListener("click", () => checkUnavailable("connection-check", "connection-result"));
  ["filter-bus", "filter-format", "filter-state", "filter-sort"].forEach((id) => $(id).addEventListener("change", renderFrames));
  $("filter-id").addEventListener("input", renderFrames);
  $("filter-reset").addEventListener("click", () => {
    ["filter-bus", "filter-format", "filter-state"].forEach((id) => { $(id).value = "all"; });
    $("filter-sort").value = "change"; $("filter-id").value = ""; renderFrames();
  });
  $("freeze").addEventListener("click", () => {
    if (!dataAvailable()) return;
    state.frozenTick = state.frozenTick === null ? state.tick : null;
    $("freeze").setAttribute("aria-pressed", String(state.frozenTick !== null));
    $("freeze").textContent = state.frozenTick === null ? "화면 고정" : "고정 해제";
    renderFrames();
  });
  ["sample-next", "signal-next"].forEach((id) => $(id).addEventListener("click", advanceSample));
  for (let bit = 0; bit < 64; bit += 1) $("start-bit").append(new Option(String(bit), String(bit)));
  for (let length = 1; length <= 64; length += 1) $("bit-length").append(new Option(length + " bit", String(length)));
  $("start-bit").value = "12";
  $("signal-frame").addEventListener("change", () => { state.analysisPair = null; state.frame = $("signal-frame").value; renderSignal(true); });
  $("decoder").addEventListener("change", () => renderSignal());
  $("capture-plan").addEventListener("change", renderCapturePlan);
  $("capture-start").addEventListener("click", beginCapture);
  $("capture-cancel").addEventListener("click", () => resetCapture("LOCAL ONLY · 연습 취소 · 실제 캡처 없음"));
  $("capture-mark").addEventListener("click", () => {
    if (!usable() || capture.phase !== "mark" || capture.marks >= 3) return;
    capture.marks += 1;
    capture.after = [...capture.before];
    const templateBit = {lights: 12, door: 7, lock: 2, pedal: 0}[capture.template];
    capture.after[Math.floor(templateBit / 8)] ^= 1 << (templateBit % 8);
    status("capture-status", "local", "LOCAL DEMO · " + templates[capture.template] + " marker " + capture.marks + "/3 · 합성 전후 쌍 생성 · 실제 기록 없음");
    renderCaptureControls();
  });
  $("capture-finish").addEventListener("click", () => {
    if (!usable() || capture.phase !== "mark" || !capture.marks) return;
    capture.phase = "result";
    const changed = [];
    capture.before.forEach((byte, index) => { for (let bit = 0; bit < 8; bit += 1) if ((byte ^ capture.after[index]) & (1 << bit)) changed.push(index * 8 + bit); });
    $("capture-comparison").textContent = identity(capture.frame) + " · " + templates[capture.template] + " · 로컬 marker " + capture.marks + "회 · 합성 변화 bit " + changed.join(", ") + "\n전: " + rawText(capture.before) + "\n후: " + rawText(capture.after);
    $("capture-result").hidden = false;
    status("capture-status", "local", "LOCAL DEMO · 비교 화면 생성 · 실차 capture 결과 없음");
    renderCaptureControls();
  });
  $("capture-analyze").addEventListener("click", () => {
    if (!usable() || capture.phase !== "result" || !capture.frame) return;
    state.frame = frameKey(capture.frame);
    state.analysisPair = {key: state.frame, before: [...capture.before], after: [...capture.after], marks: capture.marks};
    state.tick = 0;
    $("signal-frame").value = state.frame;
    renderSignal(true);
    $("start-bit").value = String({lights: 12, door: 7, lock: 2, pedal: 0}[capture.template]);
    renderSignal();
    showView("signals");
  });
  $("candidate-open").addEventListener("click", () => {
    if (!usable() || $("candidate-open").disabled) return;
    const frame = selectedFrame();
    $("candidate-preview").textContent = identity(frame) + " · bit " + $("start-bit").value + " / " + $("bit-length").value + " · " + $("byte-order").value;
    $("candidate-save-status").textContent = "";
    $("candidate-save").disabled = false;
    $("candidate-dialog").showModal();
  });
  $("candidate-save").addEventListener("click", saveCandidate);
  $("candidate-export").addEventListener("click", exportCandidates);
  $("candidate-clear").addEventListener("click", () => $("clear-dialog").showModal());
  $("candidate-clear-confirm").addEventListener("click", () => {
    state.candidates = []; renderCandidates(); $("clear-dialog").close();
    status("export-status", "local", "LOCAL ONLY · 이 탭의 후보를 비웠습니다. 다운로드 파일은 유지됩니다.");
  });
  $("permissions-open").addEventListener("click", () => $("permissions-dialog").showModal());
  $("ap-open").addEventListener("click", () => $("ap-dialog").showModal());
  $("ap-check").addEventListener("click", () => checkUnavailable("ap-check", "ap-status"));
  $("ap-rehearse").addEventListener("click", () => {
    if (!usable()) return;
    state.apCount += 1;
    $("ap-demo-status").textContent = "LOCAL DEMO · 물리 확인 안내 연습 " + state.apCount + "회 · 실제 버튼 확인·암호 생성·회전 없음. 실제 구현은 새 암호로 재접속해야 합니다.";
  });
  $("ap-dialog").addEventListener("close", () => {
    if (timers.has("ap-status")) {
      cancelTimer("ap-status"); $("ap-check").disabled = false;
      status("ap-status", "disconnected", "DISCONNECTED · 응답 확인 취소 · 실제 암호 미변경");
    }
  });
  all("[data-close-dialog]").forEach((button) => button.addEventListener("click", () => button.closest("dialog").close()));
  all("[data-settings]").forEach((button) => button.addEventListener("click", () => {
    state.settingsTarget = button.dataset.settings;
    all("[data-settings]").forEach((item) => item.setAttribute("aria-pressed", String(item === button)));
    all("[data-settings-panel]").forEach((panel) => { panel.hidden = panel.dataset.settingsPanel !== state.settingsTarget; });
    $("settings-save").disabled = state.settingsTarget === "controller";
    status("settings-status", state.settingsTarget === "controller" ? "rejected" : "local", state.settingsTarget === "controller" ? "REJECTED · Controller schema·revision 없음" : "LOCAL ONLY · " + state.settingsTarget + " 초안 · 장치 수락값 없음");
  }));
  all('[data-settings-panel] select').forEach((select) => select.addEventListener("change", () => {
    settingsSummary(); status("settings-status", "pending", "LOCAL DRAFT · 저장 전 선택값 · 실제 요청·적용 없음");
  }));
  $("settings-save").addEventListener("click", saveSettings);
  setSource();
  renderCandidates();
  settingsSummary();
  showView(new URLSearchParams(location.search).get("screen"), false);
})();
