/**************************************************************************
 *
 * Copyright 2011-2012 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/


/**
 * Window system helpers for glretrace.
 */


#include <string.h>

#include <algorithm>
#include <map>

#include "os_thread.hpp"
#include "retrace.hpp"
#include "glproc.hpp"
#include "glstate.hpp"
#include "glretrace.hpp"


using namespace glretrace;


OS_THREAD_LOCAL Context * GLWs::currentContextPtr = nullptr;


GLWs::~GLWs() {
    for (auto &v : visuals) {
        delete v.second;
    }
}

glws::Drawable *
GLWs::createDrawableHelper(glfeatures::Profile profile, int width, int height,
                     const glws::pbuffer_info *pbInfo) {
    glws::Visual *visual = getVisual(profile);
    glws::Drawable *draw = glws::createDrawable(visual, width, height, pbInfo);
    if (!draw) {
        std::cerr << "error: failed to create OpenGL drawable\n";
        exit(1);
    }

    if (pbInfo)
        draw->pbInfo = *pbInfo;

    return draw;
}


glws::Drawable *
GLWs::createDrawable(glfeatures::Profile profile) {
    return createDrawableHelper(profile);
}


glws::Drawable *
GLWs::createDrawable(void) {
    return createDrawable(defaultProfile);
}


glws::Drawable *
GLWs::createPbuffer(int width, int height, const glws::pbuffer_info *pbInfo) {
    // Zero area pbuffers are often accepted, but given we create window
    // drawables instead, they should have non-zero area.
    width  = std::max(width,  1);
    height = std::max(height, 1);

    return createDrawableHelper(defaultProfile, width, height, pbInfo);
}


Context *
GLWs::createContext(Context *shareContext, glfeatures::Profile profile) {
    glws::Visual *visual = getVisual(profile);
    glws::Context *shareWsContext = shareContext ? shareContext->wsContext : NULL;
    glws::Context *ctx = glws::createContext(visual, shareWsContext, retrace::debug);
    if (!ctx) {
        std::cerr << "error: failed to create " << profile << " context.\n";
        exit(1);
    }

    return new Context(ctx);
}


Context *
GLWs::createContext(Context *shareContext) {
    return createContext(shareContext, defaultProfile);
}


Context::~Context()
{
    //assert(this != getCurrentContext());
    if (this != getCurrentContext()) {
        delete wsContext;
    }
}


bool
GLWs::makeCurrent(trace::Call &call, glws::Drawable *drawable, Context *context)
{
    Context *currentContext = currentContextPtr;
    glws::Drawable *currentDrawable = currentContext ? currentContext->drawable : NULL;

    if (drawable == currentDrawable && context == currentContext) {
        return true;
    }

    if (currentContext) {
        glFlush();
        currentContext->needsFlush = false;
        if (!retrace::doubleBuffer) {
            frame_complete(call);
        }
    }

    flushQueries();

    beforeContextSwitch();

    bool success = glws::makeCurrent(drawable, context ? context->wsContext : NULL);

    if (!success) {
        std::cerr << "error: failed to make current OpenGL context and drawable\n";
        exit(1);
    }

    if (context != currentContext) {
        if (context) {
            context->aquire();
        }
        currentContextPtr = context;
        if (currentContext) {
            currentContext->release();
        }
    }

    if (drawable && context) {
        context->drawable = drawable;
        
        if (!context->used) {
            initContext();
            context->used = true;
        }
    }

    afterContextSwitch();

    return true;
}


/**
 * Grow the current drawble.
 *
 * We need to infer the drawable size from GL calls because the drawable sizes
 * are specified by OS specific calls which we do not trace.
 */
void
GLWs::updateDrawable(int width, int height) {
    Context *currentContext = getCurrentContext();
    if (!currentContext) {
        return;
    }

    glws::Drawable *currentDrawable = currentContext->drawable;
    if (!currentDrawable) {
        return;
    }

    if (currentDrawable->pbuffer) {
        return;
    }

    if (currentDrawable->visible &&
        width  <= currentDrawable->width &&
        height <= currentDrawable->height) {
        return;
    }

    // Ignore zero area viewports
    if (width == 0 || height == 0) {
        return;
    }

    width  = std::max(width,  currentDrawable->width);
    height = std::max(height, currentDrawable->height);

    // Check for bound framebuffer last, as this may have a performance impact.
    if (currentContext->features().framebuffer_object) {
        GLint framebuffer = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
        if (framebuffer != 0) {
            return;
        }
    }

    currentDrawable->resize(width, height);
    currentDrawable->show();

    // Ensure the drawable dimensions, as perceived by glstate match.
    if (retrace::debug) {
        GLint newWidth = 0;
        GLint newHeight = 0;
        if (glstate::getDrawableBounds(&newWidth, &newHeight) &&
            (newWidth != width || newHeight != height)) {
            std::cerr
                << "error: drawable failed to resize: "
                << "expected " << width << "x" << height << ", "
                << "got " << newWidth << "x" << newHeight << "\n";
        }
    }

    glScissor(0, 0, width, height);
}


int
GLWs::parseAttrib(const trace::Value *attribs, int param, int default_, int terminator) {
    const trace::Array *attribs_ = attribs ? attribs->toArray() : NULL;

    if (attribs_) {
        for (size_t i = 0; i + 1 < attribs_->values.size(); i += 2) {
            int param_i = attribs_->values[i]->toSInt();
            if (param_i == terminator) {
                break;
            }

            if (param_i == param) {
                int value = attribs_->values[i + 1]->toSInt();
                return value;
            }
        }
    }

    return default_;
}


/**
 * Parse GLX/WGL_ARB_create_context attribute list.
 */
glfeatures::Profile
GLWs::parseContextAttribList(const trace::Value *attribs)
{
    // {GLX,WGL}_CONTEXT_MAJOR_VERSION_ARB
    int major_version = parseAttrib(attribs, 0x2091, 1);

    // {GLX,WGL}_CONTEXT_MINOR_VERSION_ARB
    int minor_version = parseAttrib(attribs, 0x2092, 0);

    int profile_mask = parseAttrib(attribs, GL_CONTEXT_PROFILE_MASK, GL_CONTEXT_CORE_PROFILE_BIT);

    bool core_profile = profile_mask & GL_CONTEXT_CORE_PROFILE_BIT;
    if (major_version < 3 ||
        (major_version == 3 && minor_version < 2)) {
        core_profile = false;
    }

    int context_flags = parseAttrib(attribs, GL_CONTEXT_FLAGS, 0);

    bool forward_compatible = context_flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT;
    if (major_version < 3) {
        forward_compatible = false;
    }

    // {GLX,WGL}_CONTEXT_ES_PROFILE_BIT_EXT
    bool es_profile = profile_mask & 0x0004;

    glfeatures::Profile profile;
    if (es_profile) {
        profile.api = glfeatures::API_GLES;
    } else {
        profile.api = glfeatures::API_GL;
        profile.core = core_profile;
        profile.forwardCompatible = forward_compatible;
    }

    profile.major = major_version;
    profile.minor = minor_version;

    return profile;
}


// WGL_ARB_render_texture / wglBindTexImageARB()
bool
GLWs::bindTexImage(glws::Drawable *pBuffer, int iBuffer) {
    return glws::bindTexImage(pBuffer, iBuffer);
}

// WGL_ARB_render_texture / wglReleaseTexImageARB()
bool
GLWs::releaseTexImage(glws::Drawable *pBuffer, int iBuffer) {
    return glws::releaseTexImage(pBuffer, iBuffer);
}

// WGL_ARB_render_texture / wglSetPbufferAttribARB()
bool
GLWs::setPbufferAttrib(glws::Drawable *pBuffer, const int *attribs) {
    return glws::setPbufferAttrib(pBuffer, attribs);
}
