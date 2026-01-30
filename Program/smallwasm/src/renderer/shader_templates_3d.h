#pragma once

#include "../util/wasm_compat.h"

// Minimal GLSL templates for UBO + bone texture skinning.
// std140 layout: mat4/vec4 are 16-byte aligned; avoid vec3 without padding.
// These are templates; you can string-replace defines as needed.

SHINE_INLINE_VAR SHINE_CONSTINIT const char kVS_Skinned_UBO[] =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNrm;\n"
    "layout(location=2) in vec2 aUV;\n"
    "layout(location=3) in vec4 aJoints;\n"
    "layout(location=4) in vec4 aWeights;\n"
    "layout(std140, binding=0) uniform FrameUBO { mat4 uView; mat4 uProj; vec4 uLightDir; vec4 uLightColor; };\n"
    "uniform sampler2D uBoneTex;\n"
    "uniform int uBoneCount;\n"
    "out vec2 vUV;\n"
    "mat4 loadBone(int idx){\n"
    "  vec4 c0 = texelFetch(uBoneTex, ivec2(0, idx), 0);\n"
    "  vec4 c1 = texelFetch(uBoneTex, ivec2(1, idx), 0);\n"
    "  vec4 c2 = texelFetch(uBoneTex, ivec2(2, idx), 0);\n"
    "  vec4 c3 = texelFetch(uBoneTex, ivec2(3, idx), 0);\n"
    "  return mat4(c0,c1,c2,c3);\n"
    "}\n"
    "void main(){\n"
    "  vUV = aUV;\n"
    "  mat4 skin = mat4(0.0);\n"
    "  skin += loadBone(int(aJoints.x)) * aWeights.x;\n"
    "  skin += loadBone(int(aJoints.y)) * aWeights.y;\n"
    "  skin += loadBone(int(aJoints.z)) * aWeights.z;\n"
    "  skin += loadBone(int(aJoints.w)) * aWeights.w;\n"
    "  vec4 wpos = skin * vec4(aPos,1.0);\n"
    "  gl_Position = uProj * uView * wpos;\n"
    "}\n";

SHINE_INLINE_VAR SHINE_CONSTINIT const char kFS_Lit_UBO[] =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 vUV;\n"
    "layout(std140, binding=0) uniform FrameUBO { mat4 uView; mat4 uProj; vec4 uLightDir; vec4 uLightColor; };\n"
    "uniform sampler2D uTex;\n"
    "uniform vec4 uColor;\n"
    "out vec4 outColor;\n"
    "void main(){\n"
    "  vec4 base = texture(uTex, vUV) * uColor;\n"
    "  outColor = base;\n"
    "}\n";

// Quaternion + translation bone texture layout:
// width=2, height=boneCount
// texel(0, i) = vec4(qx,qy,qz,qw)
// texel(1, i) = vec4(tx,ty,tz,1)
SHINE_INLINE_VAR SHINE_CONSTINIT const char kVS_SkinnedQuat_UBO[] =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNrm;\n"
    "layout(location=2) in vec2 aUV;\n"
    "layout(location=3) in vec4 aJoints;\n"
    "layout(location=4) in vec4 aWeights;\n"
    "layout(std140, binding=0) uniform FrameUBO { mat4 uView; mat4 uProj; vec4 uLightDir; vec4 uLightColor; };\n"
    "uniform sampler2D uBoneTex;\n"
    "out vec2 vUV;\n"
    "vec3 qrotate(vec4 q, vec3 v){\n"
    "  vec3 t = 2.0 * cross(q.xyz, v);\n"
    "  return v + q.w * t + cross(q.xyz, t);\n"
    "}\n"
    "void main(){\n"
    "  vUV = aUV;\n"
    "  vec3 p = vec3(0.0);\n"
    "  for (int i = 0; i < 4; ++i) {\n"
    "    int j = int(aJoints[i]);\n"
    "    float w = aWeights[i];\n"
    "    vec4 q = texelFetch(uBoneTex, ivec2(0, j), 0);\n"
    "    vec3 t = texelFetch(uBoneTex, ivec2(1, j), 0).xyz;\n"
    "    p += (qrotate(q, aPos) + t) * w;\n"
    "  }\n"
    "  gl_Position = uProj * uView * vec4(p, 1.0);\n"
    "}\n";
