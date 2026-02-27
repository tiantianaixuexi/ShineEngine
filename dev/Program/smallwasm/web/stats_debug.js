// stats_debug.js (ES module)
// Optional debug-only stats UI: groups, history graphs, cmd console.
// Loaded on demand by index.js (keeps release payload small).

function hashColor(s) {
  let h = 2166136261;
  for (let i = 0; i < s.length; i++) h = Math.imul(h ^ s.charCodeAt(i), 16777619);
  const palette = ['#7aa2ff', '#9ece6a', '#ff9e64', '#f7768e', '#bb9af7', '#2ac3de', '#c0caf5', '#e0af68'];
  return palette[Math.abs(h) % palette.length];
}

class StatSeries {
  constructor(fullName, unit = '', color = null, historySize = 240) {
    this.fullName = fullName;
    this.unit = unit;
    this.color = color || hashColor(fullName);
    this.N = historySize | 0;
    this.buf = new Float32Array(this.N);
    this.head = 0;
    this.value = 0;
    this.hasAny = false;
  }
  push(v) {
    const fv = Number(v);
    this.value = fv;
    this.buf[this.head] = fv;
    this.head = (this.head + 1) % this.N;
    this.hasAny = true;
  }
  range() {
    if (!this.hasAny) return [0, 1];
    let mn = Infinity, mx = -Infinity;
    for (let i = 0; i < this.N; i++) {
      const x = this.buf[i];
      if (x < mn) mn = x;
      if (x > mx) mx = x;
    }
    if (!Number.isFinite(mn) || !Number.isFinite(mx)) return [0, 1];
    if (mx - mn < 1e-6) return [mn - 1, mx + 1];
    return [mn, mx];
  }
  at(k) {
    const idx = (this.head + k) % this.N;
    return this.buf[idx];
  }
}

class StatsSystem {
  constructor(perfCanvas) {
    this.enabled = true;
    this.groupsEnabled = new Map(); // prefix -> bool
    this.series = new Map(); // fullName -> StatSeries
    this.watch = []; // array of fullName
    this.perfCanvas = perfCanvas;
    this.ctx2d = perfCanvas ? perfCanvas.getContext('2d') : null;
    this._lastW = 0;
    this._lastH = 0;
    this._dpr = 1;
  }

  set(fullName, value, { unit = '', color = null } = {}) {
    if (!this.enabled) return;
    if (!this.isEnabledFor(fullName)) return;
    let s = this.series.get(fullName);
    if (!s) {
      s = new StatSeries(fullName, unit, color);
      this.series.set(fullName, s);
      if (this.watch.length === 0) this.watch.push(fullName);
    }
    if (unit && !s.unit) s.unit = unit;
    if (color && !s.color) s.color = color;
    s.push(value);
  }

  isEnabledFor(fullName) {
    let bestKey = '';
    let bestVal = true;
    for (const [k, v] of this.groupsEnabled) {
      if (!k) continue;
      if (fullName === k || fullName.startsWith(k + '/')) {
        if (k.length > bestKey.length) {
          bestKey = k;
          bestVal = !!v;
        }
      }
    }
    return bestVal;
  }

  setGroup(prefix, on) { this.groupsEnabled.set(String(prefix), !!on); }

  listGroups() {
    const out = new Set();
    for (const k of this.series.keys()) {
      const parts = k.split('/');
      let p = '';
      for (let i = 0; i < parts.length - 1; i++) {
        p = p ? (p + '/' + parts[i]) : parts[i];
        out.add(p);
      }
      out.add(parts[0]);
    }
    return Array.from(out).sort();
  }

  listStats(prefix = '') {
    const p = String(prefix || '');
    const out = [];
    for (const k of this.series.keys()) {
      if (!p || k === p || k.startsWith(p + '/')) out.push(k);
    }
    return out.sort();
  }

  watchAdd(fullName) {
    const k = String(fullName);
    if (!this.watch.includes(k)) this.watch.push(k);
  }
  watchRemove(fullName) {
    const k = String(fullName);
    this.watch = this.watch.filter(x => x !== k);
  }
  watchClear() { this.watch = []; }

  resize() {
    if (!this.perfCanvas || !this.ctx2d) return;
    const dpr = window.devicePixelRatio || 1;
    const w = Math.max(1, Math.floor(window.innerWidth * dpr));
    const h = Math.max(1, Math.floor(window.innerHeight * dpr));
    if (w !== this._lastW || h !== this._lastH || dpr !== this._dpr) {
      this._lastW = w; this._lastH = h; this._dpr = dpr;
      this.perfCanvas.width = w;
      this.perfCanvas.height = h;
    }
  }

  draw() {
    if (!this.enabled) return;
    if (!this.ctx2d) return;
    this.resize();
    const g = this.ctx2d;
    const W = this.perfCanvas.width, H = this.perfCanvas.height;

    // Important: on some browsers, pixels outside the drawn region may appear as
    // uninitialized garbage if we only clear a small panel. Clear the whole canvas.
    g.clearRect(0, 0, W, H);

    const pad = 12 * this._dpr;
    const desiredW = Math.floor(520 * this._dpr);
    const desiredH = Math.floor(260 * this._dpr);
    const panelW = Math.max(200 * this._dpr, Math.min(desiredW, W - pad * 2));
    const panelH = Math.max(160 * this._dpr, Math.min(desiredH, H - pad * 2));
    const x0 = Math.floor(W - pad - panelW);
    const y0 = pad;

    g.fillStyle = 'rgba(0,0,0,0.35)';
    g.strokeStyle = 'rgba(255,255,255,0.12)';
    g.lineWidth = 1;
    g.beginPath();
    g.rect(x0, y0, panelW, panelH);
    g.fill();
    g.stroke();

    const innerPad = 10 * this._dpr;
    const px0 = x0 + innerPad;
    const px1 = x0 + panelW - innerPad;

    const watched = this.watch.map(k => this.series.get(k)).filter(Boolean);
    if (watched.length === 0) return;

    const legendLines = Math.min(6, watched.length);
    const legendLineH = 16 * this._dpr;
    const legendH = legendLines * legendLineH;

    g.fillStyle = 'rgba(220,240,255,0.9)';
    g.font = `${Math.floor(12 * this._dpr)}px ui-monospace, SFMono-Regular, Menlo, Consolas, monospace`;
    const titleY = y0 + innerPad + 12 * this._dpr;
    g.fillText('stats (cmd: ` then "stat help")', px0, titleY);

    const plotTop = y0 + innerPad + 22 * this._dpr;
    const legendTop = y0 + panelH - innerPad - legendH;
    const plotBottom = Math.max(plotTop + 20 * this._dpr, legendTop - 8 * this._dpr);
    const py0 = plotTop;
    const py1 = plotBottom;
    const plotW = px1 - px0;
    const plotH = py1 - py0;

    g.strokeStyle = 'rgba(255,255,255,0.08)';
    g.lineWidth = 1;
    g.beginPath();
    for (let i = 0; i <= 4; i++) {
      const y = py0 + (plotH * i) / 4;
      g.moveTo(px0, y);
      g.lineTo(px1, y);
    }
    g.stroke();

    let mn = Infinity, mx = -Infinity;
    for (const s of watched) {
      const [a, b] = s.range();
      if (a < mn) mn = a;
      if (b > mx) mx = b;
    }
    if (!Number.isFinite(mn) || !Number.isFinite(mx) || mx - mn < 1e-6) { mn = 0; mx = 1; }

    for (const s of watched) {
      g.strokeStyle = s.color;
      g.lineWidth = Math.max(1, Math.floor(2 * this._dpr));
      g.beginPath();
      const N = s.N;
      for (let i = 0; i < N; i++) {
        const v = s.at(i);
        const t = i / (N - 1);
        const x = px0 + t * plotW;
        const yn = (v - mn) / (mx - mn);
        const y = py1 - yn * plotH;
        if (i === 0) g.moveTo(x, y);
        else g.lineTo(x, y);
      }
      g.stroke();
    }

    // Legend at bottom (prevents overlap with title)
    let ly = legendTop + 12 * this._dpr;
    const lx = px0;
    g.font = `${Math.floor(11 * this._dpr)}px ui-monospace, SFMono-Regular, Menlo, Consolas, monospace`;
    for (const s of watched.slice(0, 6)) {
      g.fillStyle = s.color;
      g.fillText(`${s.fullName}: ${s.value.toFixed(2)}${s.unit ? ' ' + s.unit : ''}`, lx, ly);
      ly += legendLineH;
    }
  }
}

function injectDomOnce() {
  if (document.getElementById('perf')) return;

  const style = document.createElement('style');
  style.textContent = `
    #perf { position: fixed; left: 0; top: 0; width: 100%; height: 100%; pointer-events: none; }
    #cmd { position: fixed; left: 12px; bottom: 12px; right: 12px; display: none; gap: 8px; align-items: center; background: rgba(0,0,0,0.65); border: 1px solid rgba(255,255,255,0.15); padding: 8px 10px; border-radius: 10px; }
    #cmd.show { display: flex; }
    #cmdLabel { color: #9cc; font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; }
    #cmdInput { flex: 1; font: inherit; padding: 6px 10px; border-radius: 8px; border: 1px solid #444; background: #0f0f0f; color: #ddd; outline: none; }
  `;
  document.head.appendChild(style);

  const perf = document.createElement('canvas');
  perf.id = 'perf';
  document.body.appendChild(perf);

  const cmd = document.createElement('div');
  cmd.id = 'cmd';
  cmd.innerHTML = `<div id="cmdLabel">cmd</div><input id="cmdInput" autocomplete="off" spellcheck="false" placeholder="try: stat help" />`;
  document.body.appendChild(cmd);
}

export function createDebugStatsRuntime() {
  injectDomOnce();

  const perfCanvas = document.getElementById('perf');
  const stats = new StatsSystem(perfCanvas);

  const cmdEl = document.getElementById('cmd');
  const cmdInput = document.getElementById('cmdInput');
  let cmdOpen = false;

  function setCmdOpen(on) {
    cmdOpen = !!on;
    if (cmdEl) cmdEl.classList.toggle('show', cmdOpen);
    if (cmdOpen && cmdInput) {
      cmdInput.value = '';
      cmdInput.focus();
    } else if (cmdInput) {
      cmdInput.blur();
    }
  }

  function cmdPrint(lines) {
    const msg = Array.isArray(lines) ? lines.join('\n') : String(lines);
    window.__cmdLast = msg;
  }

  function runCmd(line) {
    const s = String(line || '').trim();
    if (!s) return;
    const parts = s.split(/\s+/g);
    const root = parts[0].toLowerCase();
    if (root === 'hud') {
      const sub = (parts[1] || 'toggle').toLowerCase();
      if (sub === 'on') { window.__hud && window.__hud.on && window.__hud.on(); cmdPrint('hud on'); return; }
      if (sub === 'off') { window.__hud && window.__hud.off && window.__hud.off(); cmdPrint('hud off'); return; }
      if (sub === 'toggle') { window.__hud && window.__hud.toggle && window.__hud.toggle(); cmdPrint(`hud ${window.__hud && window.__hud.enabled && window.__hud.enabled() ? 'on' : 'off'}`); return; }
      cmdPrint('usage: hud on|off|toggle');
      return;
    }
    if (root !== 'stat' && root !== 'stats') {
      cmdPrint(`unknown cmd: ${root} (try: stat help)`);
      return;
    }
    const sub = (parts[1] || '').toLowerCase();
    if (!sub || sub === 'toggle') {
      stats.enabled = !stats.enabled;
      cmdPrint(`stat ${stats.enabled ? 'on' : 'off'}`);
      return;
    }
    if (sub === 'on' || sub === 'off') {
      stats.enabled = (sub === 'on');
      cmdPrint(`stat ${sub}`);
      return;
    }
    if (sub === 'help') {
      cmdPrint([
        'stat on|off|toggle',
        'hud on|off|toggle',
        'stat groups',
        'stat list [prefix]',
        'stat group <prefix> on|off',
        'stat watch <fullName>',
        'stat unwatch <fullName>',
        'stat watch clear',
        'examples:',
        '  stat watch Frame/cpuMs',
        '  stat group Memory off',
        '  hud off',
      ]);
      return;
    }
    if (sub === 'groups') {
      cmdPrint(stats.listGroups().join('\n') || '(no groups yet)');
      return;
    }
    if (sub === 'list') {
      const pfx = parts[2] || '';
      cmdPrint(stats.listStats(pfx).join('\n') || '(no stats yet)');
      return;
    }
    if (sub === 'group') {
      const pfx = parts[2] || '';
      const onoff = (parts[3] || '').toLowerCase();
      if (!pfx || (onoff !== 'on' && onoff !== 'off')) {
        cmdPrint('usage: stat group <prefix> on|off');
        return;
      }
      stats.setGroup(pfx, onoff === 'on');
      cmdPrint(`group ${pfx} => ${onoff}`);
      return;
    }
    if (sub === 'watch') {
      const arg = parts.slice(2).join(' ');
      if (!arg) { cmdPrint('usage: stat watch <fullName> | stat watch clear'); return; }
      if (arg.toLowerCase() === 'clear') { stats.watchClear(); cmdPrint('watch cleared'); return; }
      stats.watchAdd(arg);
      cmdPrint(`watch + ${arg}`);
      return;
    }
    if (sub === 'unwatch') {
      const arg = parts.slice(2).join(' ');
      if (!arg) { cmdPrint('usage: stat unwatch <fullName>'); return; }
      stats.watchRemove(arg);
      cmdPrint(`watch - ${arg}`);
      return;
    }
    cmdPrint(`unknown stat subcmd: ${sub} (try: stat help)`);
  }

  // Expose for manual custom stat injection.
  window.STAT = {
    set: (name, v, opts) => stats.set(String(name), v, opts),
    watch: (name) => stats.watchAdd(String(name)),
    unwatch: (name) => stats.watchRemove(String(name)),
    groupOn: (p) => stats.setGroup(String(p), true),
    groupOff: (p) => stats.setGroup(String(p), false),
    on: () => { stats.enabled = true; },
    off: () => { stats.enabled = false; },
    list: (p = '') => stats.listStats(String(p)),
    groups: () => stats.listGroups(),
  };
  window.cmd = runCmd;

  if (cmdInput) {
    cmdInput.addEventListener('keydown', (ev) => {
      if (ev.key === 'Enter') {
        ev.preventDefault();
        const line = cmdInput.value;
        runCmd(line);
        cmdInput.value = '';
      } else if (ev.key === 'Escape') {
        ev.preventDefault();
        setCmdOpen(false);
      }
    });
  }

  return {
    set: (name, v, opts) => stats.set(String(name), v, opts),
    draw: () => stats.draw(),
    openCmd: () => setCmdOpen(true),
    toggleCmd: () => setCmdOpen(!cmdOpen),
    isEnabled: () => stats.enabled,
  };
}

