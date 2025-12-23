const decoder = new TextDecoder('utf-8');

// Debug stats UI is heavy; load it on demand.
let __dbgStats = null;
async function ensureDbgStats(openCmd = false) {
  if (__dbgStats) {
    if (openCmd && __dbgStats.openCmd) __dbgStats.openCmd();
    return __dbgStats;
  }
  const mod = await import('./stats_debug.js');
  __dbgStats = mod.createDebugStatsRuntime();
  if (openCmd && __dbgStats.openCmd) __dbgStats.openCmd();
  return __dbgStats;
}

// -------------------------------
// HUD toggle (debug overlay text)
// -------------------------------
let __hudEnabled = (localStorage.getItem('hud') !== '0');
function __setHudEnabled(on) {
  __hudEnabled = !!on;
  try { localStorage.setItem('hud', __hudEnabled ? '1' : '0'); } catch {}
  const el = document.getElementById('hud');
  if (el) el.style.display = __hudEnabled ? '' : 'none';
}
function __toggleHud() { __setHudEnabled(!__hudEnabled); }
window.__hud = {
  on: () => __setHudEnabled(true),
  off: () => __setHudEnabled(false),
  toggle: () => __toggleHud(),
  enabled: () => __hudEnabled,
};
// Apply initial state
__setHudEnabled(__hudEnabled);

function cstr(mem, ptr) {
  const u8 = new Uint8Array(mem.buffer);
  let s = "";
  while (u8[ptr]) s += String.fromCharCode(u8[ptr++]);
  return s;
}

function strSlice(mem, ptr, len) {
  return decoder.decode(new Uint8Array(mem.buffer, ptr, len));
}

function compile(gl, type, src) {
  const s = gl.createShader(type);
  gl.shaderSource(s, src);
  gl.compileShader(s);
  if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
    throw new Error(gl.getShaderInfoLog(s) || 'shader compile failed');
  }
  return s;
}

function resizeCanvasToWindow(canvas) {
  const dpr = window.devicePixelRatio || 1;
  const w = Math.max(1, Math.floor(window.innerWidth * dpr));
  const h = Math.max(1, Math.floor(window.innerHeight * dpr));
  // console.log(`resizeCanvasToWindow: inner ${window.innerWidth}x${window.innerHeight} dpr ${dpr} -> ${w}x${h}`);
  // console.log(`resizeCanvasToWindow: inner ${window.innerWidth}x${window.innerHeight} dpr ${dpr} -> ${w}x${h}`);
  if (canvas.width !== w || canvas.height !== h) {
    canvas.width = w;
    canvas.height = h;
  }
  return { w, h };
}

async function loadBitmapFromUrl(url) {
  const resp = await fetch(url);
  if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
  const blob = await resp.blob();
  return await createImageBitmap(blob);
}


function makeDataUrl(mime, b64) {
  const m = (mime && mime.length) ? mime : 'image/png';
  return `data:${m};base64,${b64}`;
}

async function runApp(canvas, hud) {
  // Context table (like Emscripten's GL contexts table idea)
  const contexts = [null];
  function ctx(id) {
    const c = contexts[id];
    if (!c) throw new Error(`bad ctxId ${id}`);
    return c;
  }

  let mem;
  let wasmExports = null;

  // Debug stats UI loads on demand (press ` or add ?stat=1).
  const wantStat = (new URLSearchParams(location.search)).get('stat');
  if (wantStat === '1' || wantStat === 'true') {
    // Fire and forget; stats will show when loaded.
    ensureDbgStats(false).catch(() => {});
  }

  window.addEventListener('keydown', (ev) => {
    if (ev.key === '`') {
      ev.preventDefault();
      // First press loads it, later presses toggle cmd.
      if (!__dbgStats) ensureDbgStats(true).catch(() => {});
      else if (__dbgStats.toggleCmd) __dbgStats.toggleCmd();
    }
  });

  function smooth(oldV, newV, a) { return oldV * (1 - a) + newV * a; }

  function normalizeUrlForFetch(u) {
    // Accept Windows-ish paths from C/C++ code, and make them browser-fetch friendly.
    // - trim accidental whitespace
    // - convert backslashes to slashes
    // - encode non-ascii characters (e.g. 中文文件名)
    const raw = String(u ?? '').trim().replace(/\\/g, '/');
    try {
      return encodeURI(raw);
    } catch {
      // If it's already partially encoded with bad '%' sequences, just fall back.
      return raw;
    }
  }

  async function loadBitmap(kind, value) {
    if (kind === 'url') {
      return await loadBitmapFromUrl(value);
    }
    // data url
    return await loadBitmapFromUrl(value);
    }

 

  function createTextureFromBitmap(c, bmp) {
    const gl = c.gl;
    const tex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, bmp);
    c.textures.push(tex);
    const texId = c.textures.length - 1;
    c.texInfo[texId] = { w: bmp.width | 0, h: bmp.height | 0 };
    return texId;
  }

  function texNotifyOk(reqId, texId, w, h) {
    if (!wasmExports || !wasmExports.on_tex_loaded) {
        console.warn('texNotifyOk: no wasmExports.on_tex_loaded');
        return;
    }
    wasmExports.on_tex_loaded(reqId | 0, texId | 0, w | 0, h | 0);
  }
  function texNotifyFail(reqId, errCode) {
    if (!wasmExports || !wasmExports.on_tex_failed) return;
    wasmExports.on_tex_failed(reqId | 0, errCode | 0);
  }

  function texGet(c, key) {
    const hit = c.texCache.get(key);
    return (hit && hit.state === 'ready') ? hit : null;
  }

  function texEnsureLoad(c, key, kind, src) {
    const hit = c.texCache.get(key);
    if (hit && hit.state === 'ready') {
        console.warn('texEnsureLoad hit');
        return hit;
    }
      if (hit && hit.state === 'pending') {
          console.warn('texEnsureLoad pending');
        return null;
      }

    c.texCache.set(key, { state: 'pending', reqIds: [] });
    (async () => {
      try {
        const bmp = await loadBitmap(kind, src);
        const texId = createTextureFromBitmap(c, bmp);
        const w = bmp.width | 0;
        const h = bmp.height | 0;
        bmp.close && bmp.close();

        const pending = c.texCache.get(key);
        const reqs = pending && pending.reqIds ? pending.reqIds.slice() : [];
          c.texCache.set(key, { state: 'ready', texId, w, h });
          console.info('texEnsureLoad ready');
        for (const r of reqs) texNotifyOk(r, texId, w, h);
      } catch (e) {
        const pending = c.texCache.get(key);
        const reqs = pending && pending.reqIds ? pending.reqIds.slice() : [];
        c.texCache.delete(key);
        for (const r of reqs) texNotifyFail(r, -1);
        console.warn('texEnsureLoad failed', e);
      }
    })();
    return null;
  }


  const imports = {
    env: {


      js_console_log(msgPtr, len) {
        console.log(strSlice(mem, msgPtr, len));
      },
      js_console_log_int(labelPtr, labelLen, val) {
        console.log(strSlice(mem, labelPtr, labelLen), val);
      },

      js_log0: (fmt) => console.log(cstr(mem, fmt)),
      js_log1: (fmt, a) => console.log(cstr(mem, fmt), a),
      js_log2: (fmt, a, b) => console.log(cstr(mem, fmt), a, b),
      js_log3: (fmt, a, b, c) => console.log(cstr(mem, fmt), a, b, c),
      js_log4: (fmt, a, b, c, d) => console.log(cstr(mem, fmt), a, b, c, d),

      js_logs1: (fmt, s) => console.log(cstr(mem, fmt), cstr(mem, s)),
      js_logs2: (fmt, s1, s2) =>
        console.log(cstr(mem, fmt), cstr(mem, s1), cstr(mem, s2)),
      js_logis: (fmt, a, s) =>
        console.log(cstr(mem, fmt), a, cstr(mem, s)),

      js_debug: (fmt, file, line) =>
        console.debug(`[${cstr(mem, file)}:${line}] ${cstr(mem, fmt)}`),
      js_warn: (fmt, file, line) =>
        console.warn(`[${cstr(mem, file)}:${line}] ${cstr(mem, fmt)}`),
      js_error: (fmt, file, line) =>
        console.error(`[${cstr(mem, file)}:${line}] ${cstr(mem, fmt)}`),

      // Optional debug stats injection from WASM (DEBUG builds).
      // If stats_debug.js isn't loaded, these are cheap no-ops.
      js_stat_f32(namePtr, nameLen, value, unitPtr, unitLen) {
        if (!__dbgStats || !__dbgStats.set) return;
        const name = strSlice(mem, namePtr, nameLen);
        const unit = (unitLen | 0) > 0 ? strSlice(mem, unitPtr, unitLen) : '';
        __dbgStats.set(name, value, unit ? { unit } : undefined);
      },
      js_stat_i32(namePtr, nameLen, value, unitPtr, unitLen) {
        if (!__dbgStats || !__dbgStats.set) return;
        const name = strSlice(mem, namePtr, nameLen);
        const unit = (unitLen | 0) > 0 ? strSlice(mem, unitPtr, unitLen) : '';
        __dbgStats.set(name, value | 0, unit ? { unit } : undefined);
      },

      js_create_context(canvasIdPtr, canvasIdLen) {
        const id = strSlice(mem, canvasIdPtr, canvasIdLen);
        const el = document.getElementById(id);
        if (!el) throw new Error(`canvas id not found: ${id}`);
        const gl = el.getContext('webgl2');
        if (!gl) throw new Error('WebGL2 not supported');
        // Enable derivatives for rounded-rect AA shader.
        gl.getExtension('OES_standard_derivatives');
        // Enable alpha blending (needed for shadows/rounded AA).
        gl.disable(gl.DEPTH_TEST);
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

        const c = {
          gl,
          shaders: [null],
          programs: [null],
          buffers: [null],
          textures: [null],
          texInfo: [null], // texId -> {w,h}
          uniforms: [null],
          texCache: new Map(),
          // monitor / gpu timing
          isWebGL2: (typeof WebGL2RenderingContext !== 'undefined') && (gl instanceof WebGL2RenderingContext),
          qExt: null,
          q: [],
          qWrite: 0,
          qRead: 0,
          // instancing / VAO helpers
          instExt: null,
          vaoExt: null,
          vaos: [null],
        };

        // Instancing support (WebGL2 or ANGLE_instanced_arrays)
        if (!c.isWebGL2) {
          c.instExt = gl.getExtension('ANGLE_instanced_arrays');
          c.vaoExt = gl.getExtension('OES_vertex_array_object');
        }

        // Setup GPU timer query extension if possible.
        // WebGL2: EXT_disjoint_timer_query_webgl2
        // WebGL1: EXT_disjoint_timer_query (ANGLE)
        if (c.isWebGL2) {
          c.qExt = gl.getExtension('EXT_disjoint_timer_query_webgl2');
        } else {
          c.qExt = gl.getExtension('EXT_disjoint_timer_query');
        }
        if (c.qExt) {
          // pre-allocate a small ring of queries
          const N = 6;
          for (let i = 0; i < N; i++) {
            const q = c.isWebGL2 ? gl.createQuery() : c.qExt.createQueryEXT();
            c.q.push(q);
          }
        }

        contexts.push(c);
        return contexts.length - 1;
      },

      // Async texture load (callback mode) + cache.
      js_tex_load_url(ctxId, urlPtr, urlLen, reqId) {
        const c = ctx(ctxId);
        const url = normalizeUrlForFetch(strSlice(mem, urlPtr, urlLen));
        const key = `url:${url}`;

        const hit = texGet(c, key);
        if (hit) {
          queueMicrotask(() => texNotifyOk(reqId, hit.texId, hit.w, hit.h));
          return;
        }
        const pend = c.texCache.get(key);
        if (pend && pend.state === 'pending') {
          pend.reqIds.push(reqId | 0);
          return;
        }

        c.texCache.set(key, { state: 'pending', reqIds: [reqId | 0] });
        texEnsureLoad(c, key, 'url', url);
      },

      js_tex_load_dataurl(ctxId, dataPtr, dataLen, reqId) {
        const c = ctx(ctxId);
        const dataUrl = strSlice(mem, dataPtr, dataLen);
        const key = `data:${dataUrl}`;

        const hit = texGet(c, key);
        if (hit) {
          queueMicrotask(() => texNotifyOk(reqId, hit.texId, hit.w, hit.h));
          return;
        }
        const pend = c.texCache.get(key);
        if (pend && pend.state === 'pending') {
          pend.reqIds.push(reqId | 0);
          return;
        }

        c.texCache.set(key, { state: 'pending', reqIds: [reqId | 0] });
        texEnsureLoad(c, key, 'data', dataUrl);
      },

      // base64 (raw, no data: prefix)
      js_tex_load_base64(ctxId, mimePtr, mimeLen, b64Ptr, b64Len, reqId) {
        const c = ctx(ctxId);
        const mime = strSlice(mem, mimePtr, mimeLen);
        const b64 = strSlice(mem, b64Ptr, b64Len);
        const dataUrl = makeDataUrl(mime, b64);
        const key = `data:${dataUrl}`;

        const hit = texGet(c, key);
        if (hit) {
          queueMicrotask(() => texNotifyOk(reqId, hit.texId, hit.w, hit.h));
          return;
        }
        const pend = c.texCache.get(key);
        if (pend && pend.state === 'pending') {
          pend.reqIds.push(reqId | 0);
          return;
        }
        c.texCache.set(key, { state: 'pending', reqIds: [reqId | 0] });
        texEnsureLoad(c, key, 'data', dataUrl);
      },

      // Sync (non-blocking): returns cached texId immediately, otherwise starts async load and returns 0.
      js_tex_load_url_sync(ctxId, urlPtr, urlLen) {
        const c = ctx(ctxId);
        const url = normalizeUrlForFetch(strSlice(mem, urlPtr, urlLen));
        const key = `url:${url}`;
        const hit = texGet(c, key);
        if (hit) return hit.texId | 0;
        texEnsureLoad(c, key, 'url', url);
        return 0;
      },

      js_tex_load_dataurl_sync(ctxId, dataPtr, dataLen) {
        const c = ctx(ctxId);
        const dataUrl = strSlice(mem, dataPtr, dataLen);
        const key = `data:${dataUrl}`;
        const hit = texGet(c, key);
        if (hit) return hit.texId | 0;
        texEnsureLoad(c, key, 'data', dataUrl);
        return 0;
      },
      js_tex_load_base64_sync(ctxId, mimePtr, mimeLen, b64Ptr, b64Len) {
        const c = ctx(ctxId);
        const mime = strSlice(mem, mimePtr, mimeLen);
        const b64 = strSlice(mem, b64Ptr, b64Len);
        const dataUrl = makeDataUrl(mime, b64);
        const key = `data:${dataUrl}`;
        const hit = texGet(c, key);
        if (hit) return hit.texId | 0;
        texEnsureLoad(c, key, 'data', dataUrl);
        return 0;
      },

      js_tex_get_wh(ctxId, texId) {
        const c = ctx(ctxId);
        const info = c.texInfo[texId | 0];
        if (!info) return 0;
        const w = (info.w | 0) & 0xffff;
        const h = (info.h | 0) & 0xffff;
        return (w << 16) | h;
      },

      js_create_texture_checker(ctxId, size) {
        const c = ctx(ctxId);
        const gl = c.gl;
        const tex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, tex);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

        const s = Math.max(2, Math.min(256, size | 0));
        const pixels = new Uint8Array(s * s * 4);
        for (let y = 0; y < s; y++) {
          for (let x = 0; x < s; x++) {
            const on = (((x >> 3) ^ (y >> 3)) & 1) !== 0;
            const i = (y * s + x) * 4;
            pixels[i + 0] = on ? 240 : 30;
            pixels[i + 1] = on ? 240 : 30;
            pixels[i + 2] = on ? 240 : 30;
            pixels[i + 3] = 255;
          }
        }
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, s, s, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels);

        c.textures.push(tex);
        const texId = c.textures.length - 1;
        c.texInfo[texId] = { w: s, h: s };
        return texId;
      },

      gl_create_shader(ctxId, type, src_ptr, src_len) {
        const c = ctx(ctxId);
        const src = strSlice(mem, src_ptr, src_len);
        const sh = compile(c.gl, type, src);
        c.shaders.push(sh);
        return c.shaders.length - 1;
      },

      gl_create_program(ctxId, vs_id, fs_id) {
        const c = ctx(ctxId);
        const vs = c.shaders[vs_id];
        const fs = c.shaders[fs_id];
        const p = c.gl.createProgram();
        c.gl.attachShader(p, vs);
        c.gl.attachShader(p, fs);
        c.gl.bindAttribLocation(p, 0, 'aPos');
        c.gl.bindAttribLocation(p, 1, 'aCol');
        c.gl.linkProgram(p);
        if (!c.gl.getProgramParameter(p, c.gl.LINK_STATUS)) {
          const log = c.gl.getProgramInfoLog(p) || 'program link failed';
          c.gl.deleteProgram(p);
          throw new Error(log);
        }
        c.programs.push(p);
        return c.programs.length - 1;
      },

      // Program with 4 fixed attrib locations (for instancing demo):
      // 0:aPos 1:aCol 2:aOffsetScale 3:aICol
      gl_create_program_instanced(ctxId, vs_id, fs_id) {
        const c = ctx(ctxId);
        const vs = c.shaders[vs_id];
        const fs = c.shaders[fs_id];
        const p = c.gl.createProgram();
        c.gl.attachShader(p, vs);
        c.gl.attachShader(p, fs);
        c.gl.bindAttribLocation(p, 0, 'aPos');
        c.gl.bindAttribLocation(p, 1, 'aCol');
        c.gl.bindAttribLocation(p, 2, 'aOffsetScale');
        c.gl.bindAttribLocation(p, 3, 'aICol');
        c.gl.linkProgram(p);
        if (!c.gl.getProgramParameter(p, c.gl.LINK_STATUS)) {
          const log = c.gl.getProgramInfoLog(p) || 'program link failed';
          c.gl.deleteProgram(p);
          throw new Error(log);
        }
        c.programs.push(p);
        return c.programs.length - 1;
      },

      gl_use_program(ctxId, prog_id) {
        const c = ctx(ctxId);
        c.gl.useProgram(c.programs[prog_id]);
      },

      gl_get_uniform_location(ctxId, prog_id, name_ptr, name_len) {
        const c = ctx(ctxId);
        const name = decoder.decode(new Uint8Array(mem.buffer, name_ptr, name_len));
        const loc = c.gl.getUniformLocation(c.programs[prog_id], name);
        c.uniforms.push(loc);
        return c.uniforms.length - 1;
      },

      gl_uniform1i(ctxId, loc_id, v) {
        const c = ctx(ctxId);
        c.gl.uniform1i(c.uniforms[loc_id], v);
      },
      gl_uniform1f(ctxId, loc_id, v) {
        const c = ctx(ctxId);
        c.gl.uniform1f(c.uniforms[loc_id], v);
      },
      gl_uniform2f(ctxId, loc_id, x, y) {
        const c = ctx(ctxId);
        c.gl.uniform2f(c.uniforms[loc_id], x, y);
      },
      gl_uniform4f(ctxId, loc_id, x, y, z, w) {
        const c = ctx(ctxId);
        c.gl.uniform4f(c.uniforms[loc_id], x, y, z, w);
      },

      gl_create_buffer(ctxId) {
        const c = ctx(ctxId);
        const b = c.gl.createBuffer();
        c.buffers.push(b);
        return c.buffers.length - 1;
      },

      gl_bind_buffer(ctxId, target, buf_id) {
        const c = ctx(ctxId);
        c.gl.bindBuffer(target, c.buffers[buf_id]);
      },

      gl_buffer_data_f32(ctxId, target, ptr, float_count, usage) {
        const c = ctx(ctxId);
        // console.log(`bufferData ptr=${ptr} count=${float_count}`);
        if (ptr === 0) {
          // Allocate empty buffer of given size (in bytes)
          c.gl.bufferData(target, float_count * 4, usage);
        } else {
          // Upload data from WASM memory
          const a = new Float32Array(mem.buffer, ptr, float_count);
          c.gl.bufferData(target, a, usage);
        }
      },

      gl_enable_attribs(ctxId) {
        const c = ctx(ctxId);
        const stride = 5 * 4; // x,y,r,g,b (floats)
        c.gl.enableVertexAttribArray(0); // aPos
        c.gl.vertexAttribPointer(0, 2, c.gl.FLOAT, false, stride, 0);
        c.gl.enableVertexAttribArray(1); // aCol
        c.gl.vertexAttribPointer(1, 3, c.gl.FLOAT, false, stride, 2 * 4);
      },

      // VAO helpers (WebGL2 or OES_vertex_array_object)
      gl_create_vertex_array(ctxId) {
        const c = ctx(ctxId);
        const gl = c.gl;
        let vao = null;
        if (c.isWebGL2) vao = gl.createVertexArray();
        else if (c.vaoExt) vao = c.vaoExt.createVertexArrayOES();
        c.vaos.push(vao);
        return c.vaos.length - 1;
      },
      gl_bind_vertex_array(ctxId, vaoId) {
        const c = ctx(ctxId);
        const gl = c.gl;
        const vao = c.vaos[vaoId | 0] || null;
        if (c.isWebGL2) gl.bindVertexArray(vao);
        else if (c.vaoExt) c.vaoExt.bindVertexArrayOES(vao);
        // if no VAO support, ignore (we'll still work but need re-setup; demo assumes VAO exists)
      },

      // Setup base (non-instanced) attribs into current VAO:
      // layout: [x,y,r,g,b] floats, stride=20 bytes.
      gl_setup_attribs_basic(ctxId, vboId) {
        const c = ctx(ctxId);
        const gl = c.gl;
        const stride = 5 * 4;
        gl.bindBuffer(gl.ARRAY_BUFFER, c.buffers[vboId | 0]);
        gl.enableVertexAttribArray(0);
        gl.vertexAttribPointer(0, 2, gl.FLOAT, false, stride, 0);
        gl.enableVertexAttribArray(1);
        gl.vertexAttribPointer(1, 3, gl.FLOAT, false, stride, 2 * 4);
        // Ensure divisors are 0 (in case instancing was enabled).
        if (c.isWebGL2) {
          gl.vertexAttribDivisor(0, 0);
          gl.vertexAttribDivisor(1, 0);
          gl.vertexAttribDivisor(2, 0);
          gl.vertexAttribDivisor(3, 0);
        } else if (c.instExt) {
          c.instExt.vertexAttribDivisorANGLE(0, 0);
          c.instExt.vertexAttribDivisorANGLE(1, 0);
          c.instExt.vertexAttribDivisorANGLE(2, 0);
          c.instExt.vertexAttribDivisorANGLE(3, 0);
        }
      },

      // Setup instanced attribs into current VAO:
      // baseVbo: [x,y,_,_,_] stride 20 bytes
      // instVbo: per-instance [offX,offY,scale, r,g,b] => 6 floats stride 24 bytes
      gl_setup_attribs_instanced(ctxId, baseVboId, instVboId) {
        const c = ctx(ctxId);
        const gl = c.gl;
        const baseStride = 5 * 4;
        gl.bindBuffer(gl.ARRAY_BUFFER, c.buffers[baseVboId | 0]);
        gl.enableVertexAttribArray(0);
        gl.vertexAttribPointer(0, 2, gl.FLOAT, false, baseStride, 0);
        gl.enableVertexAttribArray(1);
        gl.vertexAttribPointer(1, 3, gl.FLOAT, false, baseStride, 2 * 4);

        const instStride = 6 * 4;
        gl.bindBuffer(gl.ARRAY_BUFFER, c.buffers[instVboId | 0]);
        gl.enableVertexAttribArray(2);
        gl.vertexAttribPointer(2, 3, gl.FLOAT, false, instStride, 0);
        gl.enableVertexAttribArray(3);
        gl.vertexAttribPointer(3, 3, gl.FLOAT, false, instStride, 3 * 4);

        if (c.isWebGL2) {
          gl.vertexAttribDivisor(2, 1);
          gl.vertexAttribDivisor(3, 1);
        } else if (c.instExt) {
          c.instExt.vertexAttribDivisorANGLE(2, 1);
          c.instExt.vertexAttribDivisorANGLE(3, 1);
        }
      },

      gl_viewport(ctxId, x, y, w, h) { ctx(ctxId).gl.viewport(x, y, w, h); },
      gl_clear_color(ctxId, r, g, b, a) { ctx(ctxId).gl.clearColor(r, g, b, a); },
      gl_clear(ctxId, mask) { ctx(ctxId).gl.clear(mask); },
      gl_active_texture(ctxId, unit) {
        const c = ctx(ctxId);
        c.gl.activeTexture(c.gl.TEXTURE0 + unit);
      },
      gl_bind_texture(ctxId, target, tex_id) {
        const c = ctx(ctxId);
        c.gl.bindTexture(target, c.textures[tex_id] || null);
      },
      gl_draw_arrays(ctxId, mode, first, count) { ctx(ctxId).gl.drawArrays(mode, first, count); },

      // Command buffer submit: reduce wasm->JS boundary overhead.
      // cmd is a packed i32 array: each command is 8 i32:
      // [op, a, b, c, d, e, f, g]
      gl_submit(ctxId, cmdPtr, cmdCount) {
        const c = ctx(ctxId);
        const gl = c.gl;
        const i32 = new Int32Array(mem.buffer, cmdPtr, (cmdCount | 0) * 8);

        // float decode helper
        const dv = imports.env._dv4 || (imports.env._dv4 = new DataView(new ArrayBuffer(4)));
        function i2f(i) { dv.setInt32(0, i, true); return dv.getFloat32(0, true); }

        // tiny state cache (inside this submit)
        let curProg = -1;
        let curBuf = -1;
        let curTex = -1;
        let curActiveTex = 0;
        let curVao = -1;

        for (let i = 0, o = 0; i < (cmdCount | 0); i++, o += 8) {
          const op = i32[o + 0] | 0;
          const a0 = i32[o + 1] | 0;
          const a1 = i32[o + 2] | 0;
          const a2 = i32[o + 3] | 0;
          const a3 = i32[o + 4] | 0;
          const a4 = i32[o + 5] | 0;
          const a5 = i32[o + 6] | 0;
          const a6 = i32[o + 7] | 0;

          switch (op | 0) {
            case 0: { // nop
              break;
            }
            case 1: { // viewport: x,y,w,h
              // console.log(`viewport ${a0},${a1} ${a2}x${a3}`);
              gl.viewport(a0, a1, a2, a3);
              break;
            }
            case 2: { // clearColor: r,g,b,a (f32 bits)
              // console.log(`clearColor`);
              gl.clearColor(i2f(a0), i2f(a1), i2f(a2), i2f(a3));
              break;
            }
            case 3: { // clear: mask
              // console.log(`clear ${a0}`);
              gl.clear(a0);
              break;
            }
            case 4: { // useProgram: prog_id
              // console.log(`useProgram ${a0}`);
              if (curProg !== a0) {
                gl.useProgram(c.programs[a0]);
                curProg = a0;
              }
              break;
            }
            case 16: { // bindVertexArray: vao_id
              // console.log(`bindVAO ${a0}`);
              if (curVao !== a0) {
                const vao = c.vaos[a0] || null;
                if (c.isWebGL2) gl.bindVertexArray(vao);
                else if (c.vaoExt) c.vaoExt.bindVertexArrayOES(vao);
                curVao = a0;
              }
              break;
            }
            case 5: { // bindBuffer: target, buf_id
              if (curBuf !== a1) {
                gl.bindBuffer(a0, c.buffers[a1]);
                curBuf = a1;
              }
              break;
            }
            case 6: { // bufferDataF32: target, ptr, floatCount, usage
              // console.log(`bufferData ptr=${a1} count=${a2}`);
              const arr = new Float32Array(mem.buffer, a1, a2);
              gl.bufferData(a0, arr, a3);
              break;
            }
            case 7: { // bufferSubDataF32: target, offsetBytes, ptr, floatCount
              const arr = new Float32Array(mem.buffer, a2, a3);
              gl.bufferSubData(a0, a1, arr);
              break;
            }
            case 8: { // drawArrays: mode, first, count
              // console.log(`drawArrays mode=${a0} first=${a1} count=${a2} prog=${curProg} vao=${curVao}`);
              gl.drawArrays(a0, a1, a2);
              break;
            }
            case 15: { // drawArraysInstanced: mode, first, count, instanceCount
              // console.log(`drawInstanced count=${a2} inst=${a3}`);
              if (c.isWebGL2) gl.drawArraysInstanced(a0, a1, a2, a3);
              else if (c.instExt) c.instExt.drawArraysInstancedANGLE(a0, a1, a2, a3);
              break;
            }
            case 9: { // activeTexture: unit
              if (curActiveTex !== a0) {
                gl.activeTexture(gl.TEXTURE0 + a0);
                curActiveTex = a0;
              }
              break;
            }
            case 10: { // bindTexture: target, tex_id
              if (curTex !== a1) {
                gl.bindTexture(a0, c.textures[a1] || null);
                curTex = a1;
              }
              break;
            }
            case 11: { // uniform1i: loc_id, v
              gl.uniform1i(c.uniforms[a0], a1);
              break;
            }
            case 12: { // uniform1f: loc_id, f (bits)
              gl.uniform1f(c.uniforms[a0], i2f(a1));
              break;
            }
            case 13: { // uniform2f: loc_id, fx, fy (bits)
              // console.log(`uniform2f loc=${a0} x=${i2f(a1)} y=${i2f(a2)}`);
              gl.uniform2f(c.uniforms[a0], i2f(a1), i2f(a2));
              break;
            }
            case 14: { // uniform4f: loc_id, f0,f1,f2,f3 (bits)
              gl.uniform4f(c.uniforms[a0], i2f(a1), i2f(a2), i2f(a3), i2f(a4));
              break;
            }
            case 17: { // setupViewSampler2D: viewLoc, wBits, hBits, samplerLoc, unit
              gl.uniform2f(c.uniforms[a0], i2f(a1), i2f(a2));
              if (curActiveTex !== a4) {
                gl.activeTexture(gl.TEXTURE0 + a4);
                curActiveTex = a4;
              }
              gl.uniform1i(c.uniforms[a3], a4);
              break;
            }
            default:
              // unknown op: ignore
              break;
          }
        }
      },
    }
  };

  // Cache-bust to avoid stale wasm during live-reload / SW caching.
  const wasmUrl = `dist/output.gl.wasm?v=${Date.now()}`;
  const resp = await fetch(wasmUrl, { cache: 'no-store' });
  if (!resp.ok) throw new Error(`wasm HTTP ${resp.status}`);
  const buf = await resp.arrayBuffer();
  const bytes = buf.byteLength;
  const { instance } = await WebAssembly.instantiate(buf, imports);
  mem = instance.exports.memory;
  if (!mem) throw new Error('expected exported memory');
  const e = instance.exports;
  wasmExports = e;


  // init demo
  const TRI_COUNT = 1500;
  e.init(TRI_COUNT);
  // Quick sanity logs (helps when HUD shows zeros)
  try {
    console.log('wasmUrl', wasmUrl, 'bytes', bytes);
    if (e.get_context_handle) console.log('wasm ctx', e.get_context_handle() | 0);
    else if (e.get_ctx_id) console.log('wasm ctx', e.get_ctx_id() | 0);
    
    if (e.is_inited) console.log('wasm inited', e.is_inited() | 0);
    else if (e.get_inited) console.log('wasm inited', e.get_inited() | 0);
  } catch {}

  // pointer events -> wasm (NDC coords)
  let ptrDown = 0;
  function toNDC(ev) {
    const r = canvas.getBoundingClientRect();
    const x = ((ev.clientX - r.left) / Math.max(1, r.width)) * 2 - 1;
    const y = 1 - ((ev.clientY - r.top) / Math.max(1, r.height)) * 2;
    return { x, y };
  }
  canvas.addEventListener('pointerdown', (ev) => {
    ptrDown = 1;
    canvas.setPointerCapture(ev.pointerId);
    const p = toNDC(ev);
    if (e.pointer) e.pointer(p.x, p.y, 1);
  });
  canvas.addEventListener('pointermove', (ev) => {
    const p = toNDC(ev);
    if (e.pointer) e.pointer(p.x, p.y, ptrDown);
  });
  canvas.addEventListener('pointerup', (ev) => {
    ptrDown = 0;
    const p = toNDC(ev);
    if (e.pointer) e.pointer(p.x, p.y, 0);
  });
  window.addEventListener('resize', () => {
    const { w, h } = resizeCanvasToWindow(canvas);
    if (frame._lastW !== w || frame._lastH !== h) {
      frame._lastW = w;
      frame._lastH = h;
      if (e.resize) e.resize(w, h);
      else if (e.on_resize) e.on_resize(w, h);
    }
  });
  {
    const { w, h } = resizeCanvasToWindow(canvas);
    frame._lastW = w;
    frame._lastH = h;
    if (e.resize) e.resize(w, h);
    else if (e.on_resize) e.on_resize(w, h);
  }

  let lastT = performance.now();
  let fps = 0;
  let cpuMs = 0;
  let gpuMs = 0;
  let gpuDisjoint = false;

  function tryConsumeGpuTiming(c) {
    if (!c || !c.qExt || c.q.length === 0) return;
    const gl = c.gl;
    const ext = c.qExt;
    // Disjoint flag: if true, timing data invalid (GPU freq changed, etc.)
    const disjoint = c.isWebGL2
      ? gl.getParameter(ext.GPU_DISJOINT_EXT)
      : gl.getParameter(ext.GPU_DISJOINT_EXT);
    gpuDisjoint = !!disjoint;
    if (disjoint) return;

    // Read back oldest available query without blocking.
    if (c.qRead === c.qWrite) return; // nothing pending
    const q = c.q[c.qRead];
    let available = false;
    if (c.isWebGL2) {
      available = !!gl.getQueryParameter(q, gl.QUERY_RESULT_AVAILABLE);
      if (!available) return;
      const ns = gl.getQueryParameter(q, gl.QUERY_RESULT);
      const ms = Number(ns) / 1e6;
      gpuMs = smooth(gpuMs, ms, 0.2);
      c.qRead = (c.qRead + 1) % c.q.length;
    } else {
      available = !!ext.getQueryObjectEXT(q, ext.QUERY_RESULT_AVAILABLE_EXT);
      if (!available) return;
      const ns = ext.getQueryObjectEXT(q, ext.QUERY_RESULT_EXT);
      const ms = Number(ns) / 1e6;
      gpuMs = smooth(gpuMs, ms, 0.2);
      c.qRead = (c.qRead + 1) % c.q.length;
    }
  }

  function beginGpuTiming(c) {
    if (!c || !c.qExt || c.q.length === 0) return;
    // avoid overwriting unread queries
    const nextWrite = (c.qWrite + 1) % c.q.length;
    if (nextWrite === c.qRead) {
      // queue full => skip begin; make endGpuTiming a no-op this frame
      c._gpuTimingActive = false;
      return;
    }
    const gl = c.gl;
    const ext = c.qExt;
    const q = c.q[c.qWrite];
    if (c.isWebGL2) gl.beginQuery(ext.TIME_ELAPSED_EXT, q);
    else ext.beginQueryEXT(ext.TIME_ELAPSED_EXT, q);
    c._gpuTimingActive = true;
  }

  function endGpuTiming(c) {
    if (!c || !c.qExt || c.q.length === 0) return;
    if (!c._gpuTimingActive) return;
    const gl = c.gl;
    const ext = c.qExt;
    if (c.isWebGL2) gl.endQuery(ext.TIME_ELAPSED_EXT);
    else ext.endQueryEXT(ext.TIME_ELAPSED_EXT);
    c.qWrite = (c.qWrite + 1) % c.q.length;
    c._gpuTimingActive = false;
  }

  function frame(now) {
    const dt = now - lastT;
    lastT = now;
    fps = fps * 0.9 + (1000 / Math.max(1, dt)) * 0.1;

    const { w, h } = resizeCanvasToWindow(canvas);
    // Keep JS->WASM to ~1 call per frame: only notify resize when it changes.
    if (frame._lastW !== w || frame._lastH !== h) {
      // console.log(`JS resize: ${w}x${h} (last: ${frame._lastW}x${frame._lastH})`);
      frame._lastW = w;
      frame._lastH = h;
      if (e.resize) e.resize(w, h);
      else if (e.on_resize) e.on_resize(w, h);
      else console.warn("WASM resize export not found!");
    }

    const c = ctx(1); // we create exactly one context in init(); its id is 1.
    tryConsumeGpuTiming(c);
    const cpuStart = performance.now();
    beginGpuTiming(c);
    e.frame(now * 0.001);
    endGpuTiming(c);
    const cpuEnd = performance.now();
    cpuMs = smooth(cpuMs, (cpuEnd - cpuStart), 0.2);

    // memory stats
    const wasmBytes = mem && mem.buffer ? mem.buffer.byteLength : 0;
    let heapUsed = 0, heapCap = 0, heapFree = 0, heapAllocs = 0, heapFrees = 0, heapFails = 0;
    if (e.wasm_heap_used_bytes) {
      heapUsed = e.wasm_heap_used_bytes() >>> 0;
      heapCap = (e.wasm_heap_capacity_bytes ? (e.wasm_heap_capacity_bytes() >>> 0) : 0);
      heapFree = (e.wasm_heap_free_list_bytes ? (e.wasm_heap_free_list_bytes() >>> 0) : 0);
      heapAllocs = (e.wasm_heap_alloc_count ? (e.wasm_heap_alloc_count() >>> 0) : 0);
      heapFrees = (e.wasm_heap_free_count ? (e.wasm_heap_free_count() >>> 0) : 0);
      heapFails = (e.wasm_heap_alloc_fail_count ? (e.wasm_heap_alloc_fail_count() >>> 0) : 0);
    }
    let jsHeapMB = 0;
    const pm = performance && performance.memory;
    if (pm && typeof pm.usedJSHeapSize === 'number') jsHeapMB = pm.usedJSHeapSize / (1024 * 1024);

    const mode = (e.get_render_mode ? (e.get_render_mode() | 0) : 0);
    const modeName = (mode === 1) ? 'instanced' : 'tri-grid';
    const drawCalls = e.get_draw_calls ? (e.get_draw_calls() | 0) : 0;
    const vertCount = e.get_vertex_count ? (e.get_vertex_count() | 0) : 0;
    const instCount = e.get_instance_count ? (e.get_instance_count() | 0) : 0;
    const texCount = (c && c.textures) ? Math.max(0, (c.textures.length | 0) - 1) : 0;

    // Feed debug stats system (customizable + graphs) if loaded
    if (__dbgStats && __dbgStats.set) {
      __dbgStats.set('Frame/fps', fps, { unit: 'fps' });
      __dbgStats.set('Frame/cpuMs', cpuMs, { unit: 'ms' });
      __dbgStats.set('Frame/gpuMs', gpuMs, { unit: 'ms' });
      __dbgStats.set('Render/mode', mode, { unit: '' });
      __dbgStats.set('Render/drawCalls', drawCalls, { unit: '' });
      __dbgStats.set('Render/vertices', vertCount, { unit: '' });
      __dbgStats.set('Render/instances', instCount, { unit: '' });
      __dbgStats.set('Render/textures', texCount, { unit: '' });
      __dbgStats.set('Memory/wasmMemMB', wasmBytes / (1024 * 1024), { unit: 'MB' });
      __dbgStats.set('Memory/jsHeapMB', jsHeapMB, { unit: 'MB' });
      __dbgStats.set('Memory/heapUsedMB', heapUsed / (1024 * 1024), { unit: 'MB' });
      __dbgStats.set('Memory/heapCapMB', heapCap / (1024 * 1024), { unit: 'MB' });
      __dbgStats.set('Memory/heapFreeListMB', heapFree / (1024 * 1024), { unit: 'MB' });
      __dbgStats.set('Memory/heapAllocs', heapAllocs, { unit: '' });
      __dbgStats.set('Memory/heapFrees', heapFrees, { unit: '' });
      __dbgStats.set('Memory/heapFails', heapFails, { unit: '' });
    }

    if (__hudEnabled && hud) {
      const cmdCount = (e.get_cmd_count ? (e.get_cmd_count() | 0) : 0);
      const ctxId = (e.get_context_handle ? (e.get_context_handle() | 0) : (e.get_ctx_id ? (e.get_ctx_id() | 0) : -1));
      const inited = (e.is_inited ? (e.is_inited() | 0) : (e.get_inited ? (e.get_inited() | 0) : -1));
      const frameNo = (e.get_frame_no ? (e.get_frame_no() | 0) : -1);
      const appAddr = (e.get_app_addr ? (e.get_app_addr() >>> 0) : 0);
      const appSize = (e.get_app_size ? (e.get_app_size() | 0) : 0);
      const heapBase = (e.wasm_heap_base_addr ? (e.wasm_heap_base_addr() >>> 0) : 0);
      const heapPtr = (e.wasm_heap_ptr_addr ? (e.wasm_heap_ptr_addr() >>> 0) : 0);
      hud.textContent =
        `demo (output.gl.wasm ${bytes} bytes)\n` +
        `tris=${TRI_COUNT}  mode=${modeName}  ctx=${ctxId}  inited=${inited}  frame=${frameNo}\n` +
        `drawCalls=${drawCalls}  cmds=${cmdCount}  verts=${vertCount}  inst=${instCount}  tex=${texCount}\n` +
        `app@0x${appAddr.toString(16)} +${appSize}  heapBase=0x${heapBase.toString(16)} heapPtr=0x${heapPtr.toString(16)}\n` +
        `fps≈${fps.toFixed(1)}  cpu≈${cpuMs.toFixed(2)}ms  gpu≈${gpuMs.toFixed(2)}ms${gpuDisjoint ? ' (disjoint)' : ''}\n` +
        `wasmMem=${(wasmBytes / (1024 * 1024)).toFixed(2)}MB  jsHeap≈${jsHeapMB.toFixed(1)}MB\n` +
        `heap used=${(heapUsed / (1024 * 1024)).toFixed(2)}MB / cap=${(heapCap / (1024 * 1024)).toFixed(2)}MB  freeList≈${(heapFree / (1024 * 1024)).toFixed(2)}MB\n` +
        `heap allocs=${heapAllocs} frees=${heapFrees} fails=${heapFails}` +
        (window.__cmdLast ? `\n---\n${window.__cmdLast}` : '');
    }

    if (__dbgStats && __dbgStats.draw) __dbgStats.draw();
    requestAnimationFrame(frame);
  }

  requestAnimationFrame(frame);
}

async function main() {
  const hud = document.getElementById('hud');
  const canvas = document.getElementById('c');
  await runApp(canvas, hud);
}

main().catch((e) => {
  console.error(e);
  const hud = document.getElementById('hud');
  if (hud) hud.textContent = String(e);
});
