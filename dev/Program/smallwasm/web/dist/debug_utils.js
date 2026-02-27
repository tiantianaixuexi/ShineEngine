
// Encapsulates Debug logic (GPU Timing, HUD, Stats)
// Loaded dynamically only when debugging is enabled.

export class GpuTimer {
  constructor(gl, isWebGL2) {
    this.gl = gl;
    this.isWebGL2 = isWebGL2;
    this.ext = isWebGL2
      ? gl.getExtension('EXT_disjoint_timer_query_webgl2')
      : gl.getExtension('EXT_disjoint_timer_query');
    
    this.q = [];
    this.qWrite = 0;
    this.qRead = 0;
    this.active = false;
    this.gpuMs = 0;
    this.disjoint = false;

    if (this.ext) {
      const N = 6;
      for (let i = 0; i < N; i++) {
        const q = isWebGL2 ? gl.createQuery() : this.ext.createQueryEXT();
        this.q.push(q);
      }
    }
  }

  begin() {
    if (!this.ext || this.q.length === 0) return;
    // avoid overwriting unread queries
    const nextWrite = (this.qWrite + 1) % this.q.length;
    if (nextWrite === this.qRead) {
      this.active = false;
      return;
    }
    const q = this.q[this.qWrite];
    if (this.isWebGL2) this.gl.beginQuery(this.ext.TIME_ELAPSED_EXT, q);
    else this.ext.beginQueryEXT(this.ext.TIME_ELAPSED_EXT, q);
    this.active = true;
  }

  end() {
    if (!this.ext || this.q.length === 0 || !this.active) return;
    if (this.isWebGL2) this.gl.endQuery(this.ext.TIME_ELAPSED_EXT);
    else this.ext.endQueryEXT(this.ext.TIME_ELAPSED_EXT);
    this.qWrite = (this.qWrite + 1) % this.q.length;
    this.active = false;
  }

  tryConsume() {
    if (!this.ext || this.q.length === 0) return;
    const gl = this.gl;
    const ext = this.ext;

    // Disjoint check
    const disjoint = this.isWebGL2
      ? gl.getParameter(ext.GPU_DISJOINT_EXT)
      : gl.getParameter(ext.GPU_DISJOINT_EXT);
    this.disjoint = !!disjoint;
    if (disjoint) return;

    if (this.qRead === this.qWrite) return; // nothing pending
    const q = this.q[this.qRead];
    
    let available = false;
    if (this.isWebGL2) {
      available = !!gl.getQueryParameter(q, gl.QUERY_RESULT_AVAILABLE);
    } else {
      available = !!ext.getQueryObjectEXT(q, ext.QUERY_RESULT_AVAILABLE_EXT);
    }

    if (!available) return;

    let ns = 0;
    if (this.isWebGL2) ns = gl.getQueryParameter(q, gl.QUERY_RESULT);
    else ns = ext.getQueryObjectEXT(q, ext.QUERY_RESULT_EXT);
    
    const ms = Number(ns) / 1e6;
    // Simple smooth
    this.gpuMs = this.gpuMs * 0.8 + ms * 0.2;
    this.qRead = (this.qRead + 1) % this.q.length;
  }
}

export class DebugHud {
  constructor() {
    this.el = document.getElementById('hud');
    this.enabled = (localStorage.getItem('hud') !== '0');
    this.applyState();
    
    // Setup global access for toggle
    window.__hud = {
      on: () => this.setEnabled(true),
      off: () => this.setEnabled(false),
      toggle: () => this.toggle(),
      enabled: () => this.enabled,
    };
  }

  setEnabled(on) {
    this.enabled = !!on;
    try { localStorage.setItem('hud', this.enabled ? '1' : '0'); } catch {}
    this.applyState();
  }

  toggle() { this.setEnabled(!this.enabled); }

  applyState() {
    if (this.el) this.el.style.display = this.enabled ? '' : 'none';
  }

  update(info) {
    if (!this.enabled || !this.el) return;
    // Format the text content
    // info expects: { bytes, triCount, modeName, ctxId, inited, frameNo, drawCalls, cmdCount, vertCount, instCount, texCount, fps, cpuMs, gpuMs, gpuDisjoint, wasmBytes, jsHeapMB, heapUsed, heapCap, heapFree, heapAllocs, heapFrees, heapFails, appAddr, appSize, heapBase, heapPtr }
    
    const i = info;
    this.el.textContent =
        `demo (output.gl.wasm ${i.bytes} bytes)\n` +
        `tris=${i.triCount}  mode=${i.modeName}  ctx=${i.ctxId}  inited=${i.inited}  frame=${i.frameNo}\n` +
        `drawCalls=${i.drawCalls}  cmds=${i.cmdCount}  verts=${i.vertCount}  inst=${i.instCount}  tex=${i.texCount}\n` +
        `app@0x${(i.appAddr||0).toString(16)} +${i.appSize}  heapBase=0x${(i.heapBase||0).toString(16)} heapPtr=0x${(i.heapPtr||0).toString(16)}\n` +
        `fps≈${i.fps.toFixed(1)}  cpu≈${i.cpuMs.toFixed(2)}ms  gpu≈${i.gpuMs.toFixed(2)}ms${i.gpuDisjoint ? ' (disjoint)' : ''}\n` +
        `wasmMem=${(i.wasmBytes / (1024 * 1024)).toFixed(2)}MB  jsHeap≈${i.jsHeapMB.toFixed(1)}MB\n` +
        `heap used=${(i.heapUsed / (1024 * 1024)).toFixed(2)}MB / cap=${(i.heapCap / (1024 * 1024)).toFixed(2)}MB  freeList≈${(i.heapFree / (1024 * 1024)).toFixed(2)}MB\n` +
        `heap allocs=${i.heapAllocs} frees=${i.heapFrees} fails=${i.heapFails}` +
        (window.__cmdLast ? `\n---\n${window.__cmdLast}` : '');
  }
}

// Manages the stats_debug.js module
export class DebugStats {
  constructor() {
    this.instance = null;
    this.loadPromise = null;
  }

  async ensure(openCmd = false) {
    if (this.instance) {
      if (openCmd && this.instance.openCmd) this.instance.openCmd();
      return this.instance;
    }
    if (!this.loadPromise) {
      this.loadPromise = import('./stats_debug.js').then(mod => {
        this.instance = mod.createDebugStatsRuntime();
        return this.instance;
      });
    }
    const inst = await this.loadPromise;
    if (openCmd && inst.openCmd) inst.openCmd();
    return inst;
  }
  
  // Set a value if loaded
  set(name, val, opts) {
    if (this.instance && this.instance.set) this.instance.set(name, val, opts);
  }

  draw() {
    if (this.instance && this.instance.draw) this.instance.draw();
  }
  
  get toggleCmd() { return this.instance ? this.instance.toggleCmd : null; }
}
