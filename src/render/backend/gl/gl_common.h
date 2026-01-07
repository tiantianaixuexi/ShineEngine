#pragma once

#include <GL/glew.h>

namespace shine::render::backend::gl
{
    struct ViewportInfo {
        GLuint fbo{0};
        GLuint color{0};
        GLuint depth{0};
        int width{0};
        int height{0};

        ViewportInfo() = default;
        ViewportInfo(GLuint f, GLuint c, GLuint d, int w, int h)
            : fbo(f), color(c), depth(d), width(w), height(h) {}
        
        // Rule of 5 defaults
        ViewportInfo(const ViewportInfo&) = default;
        ViewportInfo(ViewportInfo&&) = default;
        ViewportInfo& operator=(const ViewportInfo&) = default;
        ViewportInfo& operator=(ViewportInfo&&) = default;
    };
}
