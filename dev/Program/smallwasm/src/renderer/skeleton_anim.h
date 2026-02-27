#pragma once

#include "../math/Matrix4.h"

namespace shine {
namespace renderer {

struct BoneTransform {
    float tx = 0.0f;
    float ty = 0.0f;
    float tz = 0.0f;
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;
    float qw = 1.0f;
    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;
};

struct Keyframe {
    float t = 0.0f;
    BoneTransform tr;
};

struct Track {
    const Keyframe* keys = nullptr;
    int keyCount = 0;
};

struct AnimationClip {
    const Track* tracks = nullptr;
    int trackCount = 0;
    float duration = 0.0f;
};

struct Skeleton {
    int boneCount = 0;
    const int* parent = nullptr;      // length=boneCount, parent index or -1
    const Matrix4* invBind = nullptr; // length=boneCount
};

static inline Matrix4 compose_trs(const BoneTransform& tr) {
    return Matrix4::fromQuatScaleTranslation(
        tr.qx, tr.qy, tr.qz, tr.qw,
        tr.sx, tr.sy, tr.sz,
        tr.tx, tr.ty, tr.tz
    );
}

static inline void normalize_quat(float& x, float& y, float& z, float& w) {
    float len2 = x * x + y * y + z * z + w * w;
    if (len2 <= 0.0f) { x = y = z = 0.0f; w = 1.0f; return; }
    float inv = 1.0f / sqrt(len2);
    x *= inv; y *= inv; z *= inv; w *= inv;
}

static inline BoneTransform lerp_tr(const BoneTransform& a, const BoneTransform& b, float t) {
    BoneTransform r;
    r.tx = a.tx + (b.tx - a.tx) * t;
    r.ty = a.ty + (b.ty - a.ty) * t;
    r.tz = a.tz + (b.tz - a.tz) * t;
    r.sx = a.sx + (b.sx - a.sx) * t;
    r.sy = a.sy + (b.sy - a.sy) * t;
    r.sz = a.sz + (b.sz - a.sz) * t;
    float dot = a.qx * b.qx + a.qy * b.qy + a.qz * b.qz + a.qw * b.qw;
    float bx = b.qx, by = b.qy, bz = b.qz, bw = b.qw;
    if (dot < 0.0f) { bx = -bx; by = -by; bz = -bz; bw = -bw; }
    r.qx = a.qx + (bx - a.qx) * t;
    r.qy = a.qy + (by - a.qy) * t;
    r.qz = a.qz + (bz - a.qz) * t;
    r.qw = a.qw + (bw - a.qw) * t;
    normalize_quat(r.qx, r.qy, r.qz, r.qw);
    return r;
}

static inline void sample_track(const Track& track, float t, BoneTransform& out) {
    if (!track.keys || track.keyCount <= 0) return;
    if (track.keyCount == 1) { out = track.keys[0].tr; return; }
    int idx = 0;
    for (int i = 0; i < track.keyCount - 1; ++i) {
        if (t >= track.keys[i].t && t <= track.keys[i + 1].t) { idx = i; break; }
    }
    const Keyframe& k0 = track.keys[idx];
    const Keyframe& k1 = track.keys[idx + 1];
    float dt = (k1.t - k0.t);
    float a = (dt > 0.0f) ? (t - k0.t) / dt : 0.0f;
    out = lerp_tr(k0.tr, k1.tr, a);
}

static inline void sample_clip(const AnimationClip& clip, float t, BoneTransform* outLocal, int boneCount) {
    if (!outLocal || boneCount <= 0 || !clip.tracks) return;
    float time = t;
    if (clip.duration > 0.0f) {
        while (time > clip.duration) time -= clip.duration;
        while (time < 0.0f) time += clip.duration;
    }
    const int n = (boneCount < clip.trackCount) ? boneCount : clip.trackCount;
    for (int i = 0; i < n; ++i) {
        sample_track(clip.tracks[i], time, outLocal[i]);
    }
}

static inline void build_skin_matrices(const Skeleton& skel, const BoneTransform* local, Matrix4* outSkin) {
    if (!local || !outSkin || skel.boneCount <= 0 || !skel.parent || !skel.invBind) return;
    for (int i = 0; i < skel.boneCount; ++i) {
        Matrix4 localM = compose_trs(local[i]);
        int p = skel.parent[i];
        if (p >= 0) {
            outSkin[i] = Matrix4::multiply(outSkin[p], localM);
        } else {
            outSkin[i] = localM;
        }
        outSkin[i] = Matrix4::multiply(outSkin[i], skel.invBind[i]);
    }
}

// Pack skin matrices into a float array suitable for bone texture upload.
// Layout: width=4, height=boneCount, each row is 4 vec4 columns (mat4).
static inline void pack_skin_matrices(const Matrix4* mats, int boneCount, float* out) {
    if (!mats || !out || boneCount <= 0) return;
    for (int i = 0; i < boneCount; ++i) {
        const float* m = mats[i].e;
        for (int j = 0; j < 16; ++j) out[i * 16 + j] = m[j];
    }
}

} // namespace renderer
} // namespace shine
