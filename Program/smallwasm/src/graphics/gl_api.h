#pragma once

// graphics/gl_api.h
// Raw WebGL wrappers imported from JS.

using uintptr_t = __UINTPTR_TYPE__;

extern "C" {

// Context / Texture
int js_create_context(int canvas_id_ptr, int canvas_id_len); // returns ctxId
int js_create_texture_checker(int ctx, int size); // returns texId
void js_tex_load_url(int ctx, int url_ptr, int url_len, int reqId);
void js_tex_load_dataurl(int ctx, int data_ptr, int data_len, int reqId);
void js_tex_load_base64(int ctx, int mime_ptr, int mime_len, int b64_ptr, int b64_len, int reqId);
int  js_tex_load_url_sync(int ctx, int url_ptr, int url_len);           // returns texId if cached else 0
int  js_tex_load_dataurl_sync(int ctx, int data_ptr, int data_len);     // returns texId if cached else 0
int  js_tex_load_base64_sync(int ctx, int mime_ptr, int mime_len, int b64_ptr, int b64_len); // cached else 0
int  js_tex_get_wh(int ctx, int texId); // returns packed (w<<16)|h, 0 if unknown

// Shader / Program
int  gl_create_shader(int ctx, int type, int src_ptr, int src_len);
int  gl_create_program(int ctx, int vs, int fs);
int  gl_create_program_instanced(int ctx, int vs, int fs);
void gl_use_program(int ctx, int prog);
int  gl_get_uniform_location(int ctx, int prog, int name_ptr, int name_len); // returns locId

// Uniforms
void gl_uniform1i(int ctx, int loc, int v);
void gl_uniform1f(int ctx, int loc, float v);
void gl_uniform2f(int ctx, int loc, float x, float y);
void gl_uniform4f(int ctx, int loc, float x, float y, float z, float w);

// Buffers / VAO
int  gl_create_buffer(int ctx);
void gl_bind_buffer(int ctx, int target, int buf);
void gl_buffer_data_f32(int ctx, int target, int ptr, int float_count, int usage);
void gl_enable_attribs(int ctx);
int  gl_create_vertex_array(int ctx);
void gl_bind_vertex_array(int ctx, int vao);
void gl_setup_attribs_basic(int ctx, int vbo);
void gl_setup_attribs_instanced(int ctx, int baseVbo, int instVbo);

// Drawing / State
void gl_viewport(int ctx, int x, int y, int w, int h);
void gl_clear_color(int ctx, float r, float g, float b, float a);
void gl_clear(int ctx, int mask);
void gl_active_texture(int ctx, int unit);
void gl_bind_texture(int ctx, int target, int tex);
void gl_draw_arrays(int ctx, int mode, int first, int count);
void gl_draw_arrays_instanced(int ctx, int mode, int first, int count, int instCount);

// Command Buffer Submit
void gl_submit(int ctx, int cmd_ptr, int cmd_count);

} // extern "C"

// Constants
static constexpr int GL_ARRAY_BUFFER = 0x8892;
static constexpr int GL_DYNAMIC_DRAW = 0x88E8;
static constexpr int GL_COLOR_BUFFER_BIT = 0x00004000;
static constexpr int GL_TRIANGLES = 0x0004;
static constexpr int GL_VERTEX_SHADER = 0x8B31;
static constexpr int GL_FRAGMENT_SHADER = 0x8B30;
static constexpr int GL_TEXTURE_2D = 0x0DE1;
