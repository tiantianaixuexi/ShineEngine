const decoder = new TextDecoder('utf-8');

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
    if (!wasmExports || !wasmExports.on_tex_loaded) return;
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
    if (hit && hit.state === 'ready') return hit;
    if (hit && hit.state === 'pending') return null;
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

      js_create_context(canvasIdPtr, canvasIdLen) {
        const id = strSlice(mem, canvasIdPtr, canvasIdLen);
        const el = document.getElementById(id);
        if (!el) throw new Error(`canvas id not found: ${id}`);
        const gl = el.getContext('webgl');
        if (!gl) throw new Error('WebGL not supported');
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
        };
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
        const a = new Float32Array(mem.buffer, ptr, float_count);
        c.gl.bufferData(target, a, usage);
      },

      gl_enable_attribs(ctxId) {
        const c = ctx(ctxId);
        const stride = 5 * 4; // x,y,r,g,b (floats)
        c.gl.enableVertexAttribArray(0); // aPos
        c.gl.vertexAttribPointer(0, 2, c.gl.FLOAT, false, stride, 0);
        c.gl.enableVertexAttribArray(1); // aCol
        c.gl.vertexAttribPointer(1, 3, c.gl.FLOAT, false, stride, 2 * 4);
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
    }
  };

  const resp = await fetch('dist/output.gl.wasm');
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
    if (e.ui_pointer) e.ui_pointer(p.x, p.y, 1);
  });
  canvas.addEventListener('pointermove', (ev) => {
    const p = toNDC(ev);
    if (e.ui_pointer) e.ui_pointer(p.x, p.y, ptrDown);
  });
  canvas.addEventListener('pointerup', (ev) => {
    ptrDown = 0;
    const p = toNDC(ev);
    if (e.ui_pointer) e.ui_pointer(p.x, p.y, 0);
  });

  let lastT = performance.now();
  let fps = 0;

  function frame(now) {
    const dt = now - lastT;
    lastT = now;
    fps = fps * 0.9 + (1000 / Math.max(1, dt)) * 0.1;

    const { w, h } = resizeCanvasToWindow(canvas);
    e.on_resize(w, h);

    e.frame(now * 0.001);


    hud.textContent = `demo (output.gl.wasm ${bytes} bytes)\ntris=${TRI_COUNT}\nfps≈${fps.toFixed(1)}`;
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
