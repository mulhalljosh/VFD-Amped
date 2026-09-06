(function () {
  const $ = (sel, root) => (root || document).querySelector(sel);
  const $$ = (sel, root) => Array.from((root || document).querySelectorAll(sel));

  const state = {
    key: localStorage.getItem("amped_api_key") || "",
    mockFault: { 1: false, 2: false },
    busy: false,
  };

  function headers(json) {
    const h = {};
    if (json) h["Content-Type"] = "application/json";
    if (state.key) h["X-Api-Key"] = state.key;
    return h;
  }

  async function api(path, opt) {
    const res = await fetch(path, opt);
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || res.statusText);
    return data;
  }

  function fmt(n, d) {
    return Number(n).toFixed(d);
  }

  function applyStatus(s) {
    $("#fw-ver").textContent = s.version || "0.1.0";
    $("#badge-mock").classList.toggle("hidden", !s.mock);
    $$(".mock-only").forEach((el) => el.classList.toggle("hidden", !s.mock));
    $("#badge-hb").className = "pill " + (s.heartbeat_ok ? "ok" : "bad");
    $("#badge-hb").textContent = s.heartbeat_ok ? "HEARTBEAT" : "HB LOST";
    $("#badge-net").className = "pill " + (s.network_ok ? "ok" : "bad");
    $("#uptime").textContent = "up " + Math.floor((s.uptime_ms || 0) / 1000) + "s";

    (s.temps || []).forEach((t) => {
      const card = document.querySelector('.temp-card[data-id="' + t.id + '"]');
      if (!card) return;
      $("h3", card).textContent = t.label;
      const el = $(".reading", card);
      el.innerHTML = (t.valid ? fmt(t.celsius, 1) : "—") + "<small>°C</small>";
    });

    (s.vfds || []).forEach((v) => {
      const panel = document.querySelector('.vfd[data-ch="' + v.channel + '"]');
      if (!panel) return;
      panel.classList.toggle("faulted", !!v.fault);
      const flag = $(".fault-flag", panel);
      flag.hidden = !v.fault;
      $$(".mode button", panel).forEach((b) => {
        b.classList.toggle("on", b.dataset.mode === v.mode);
      });
      const slider = $("input[type=range]", panel);
      if (document.activeElement !== slider) slider.value = String(Math.round(v.speed_pct));
      $(".pct", panel).textContent = String(Math.round(v.commanded_speed_pct));
      $(".volts", panel).textContent = fmt(v.ao_volts, 3) + " V";
      $(".ma", panel).textContent = fmt(v.ao_ma, 3) + " mA";
      const runBtn = $(".run", panel);
      runBtn.classList.toggle("live", !!v.commanded_run);
      runBtn.textContent = v.commanded_run ? "Running" : "Run";
      let msg = v.commanded_run ? "Output live" : "Stopped";
      if (v.fault) msg = "Fault — run dropped";
      if (s.interlock_blocking_cooler && v.channel === 2) msg = "Blocked by pump interlock";
      $(".state", panel).textContent = msg;
    });
  }

  async function postVfd(ch, body) {
    if (state.busy) return;
    state.busy = true;
    try {
      const s = await api("/api/vfd/" + ch, {
        method: "POST",
        headers: headers(true),
        body: JSON.stringify(body),
      });
      applyStatus(s);
    } catch (err) {
      $("#settings-msg").textContent = err.message;
    } finally {
      state.busy = false;
    }
  }

  function bindPanel(panel) {
    const ch = Number(panel.dataset.ch);
    $$(".mode button", panel).forEach((btn) => {
      btn.addEventListener("click", () => {
        postVfd(ch, { mode: btn.dataset.mode });
      });
    });
    const slider = $("input[type=range]", panel);
    slider.addEventListener("change", () => {
      postVfd(ch, { speed_pct: Number(slider.value) });
    });
    slider.addEventListener("input", () => {
      $(".pct", panel).textContent = slider.value;
    });
    $(".run", panel).addEventListener("click", () => {
      const live = $(".run", panel).classList.contains("live");
      postVfd(ch, { run: !live, speed_pct: Number(slider.value) });
    });
  }

  $$(".vfd").forEach(bindPanel);

  async function refresh() {
    try {
      const s = await api("/api/status", { headers: headers(false) });
      applyStatus(s);
    } catch (err) {
      $("#badge-hb").className = "pill bad";
      $("#badge-hb").textContent = "HB LOST";
      $("#settings-msg").textContent = err.message;
    }
  }

  async function loadSettings() {
    try {
      const s = await api("/api/settings", { headers: headers(false) });
      const f = $("#settings-form");
      f.heartbeat_timeout_ms.value = s.heartbeat_timeout_ms;
      f.interlock_cooler_requires_pump.checked = !!s.interlock_cooler_requires_pump;
      (s.channels || []).forEach((c) => {
        f["failsafe_" + c.channel].value = c.failsafe;
        f["preset_pct_" + c.channel].value = c.preset_pct;
        f["analog_path_" + c.channel].value = c.analog_path;
      });
      f.outdoor_low_c.value = s.outdoor_low_c;
      f.outdoor_high_c.value = s.outdoor_high_c;
      f.water_low_c.value = s.water_low_c;
      f.water_high_c.value = s.water_high_c;
      f.client_api_key.value = state.key;
    } catch (err) {
      $("#settings-msg").textContent = err.message;
    }
  }

  $("#settings-form").addEventListener("submit", async (ev) => {
    ev.preventDefault();
    const f = ev.target;
    state.key = f.client_api_key.value.trim();
    localStorage.setItem("amped_api_key", state.key);
    const body = {
      heartbeat_timeout_ms: Number(f.heartbeat_timeout_ms.value),
      interlock_cooler_requires_pump: f.interlock_cooler_requires_pump.checked,
      failsafe_1: f.failsafe_1.value,
      failsafe_2: f.failsafe_2.value,
      preset_pct_1: Number(f.preset_pct_1.value),
      preset_pct_2: Number(f.preset_pct_2.value),
      analog_path_1: f.analog_path_1.value,
      analog_path_2: f.analog_path_2.value,
      outdoor_low_c: Number(f.outdoor_low_c.value),
      outdoor_high_c: Number(f.outdoor_high_c.value),
      water_low_c: Number(f.water_low_c.value),
      water_high_c: Number(f.water_high_c.value),
    };
    try {
      await api("/api/settings", {
        method: "POST",
        headers: headers(true),
        body: JSON.stringify(body),
      });
      $("#settings-msg").textContent = "Settings saved.";
    } catch (err) {
      $("#settings-msg").textContent = err.message;
    }
  });

  async function toggleFault(ch) {
    state.mockFault[ch] = !state.mockFault[ch];
    try {
      const s = await api("/api/mock/fault", {
        method: "POST",
        headers: headers(true),
        body: JSON.stringify({ channel: ch, fault: state.mockFault[ch] }),
      });
      applyStatus(s);
    } catch (err) {
      $("#settings-msg").textContent = err.message;
    }
  }

  $("#btn-fault-1").addEventListener("click", () => toggleFault(1));
  $("#btn-fault-2").addEventListener("click", () => toggleFault(2));

  loadSettings();
  refresh();
  setInterval(refresh, 1000);
})();
