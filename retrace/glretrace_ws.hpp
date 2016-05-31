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

#pragma once

#include <map>

#include "os_thread.hpp"
#include "glws.hpp"


namespace glretrace {

class Context
{
public:
    Context(glws::Context* context)
        : wsContext(context)
    {
    }

private:
    unsigned refCount = 1;
    ~Context();

public:
    inline void
    aquire(void) {
        assert(refCount);
        ++refCount;
    }

    inline void
    release(void) {
        assert(refCount);
        if (--refCount == 0) {
            delete this;
        }
    }

    glws::Context* wsContext;

    // Bound drawable
    glws::Drawable *drawable = nullptr;

    // Active program (unswizzled) for profiling
    GLuint currentUserProgram = 0;

    // Current program (swizzled) for uniform swizzling
    GLuint currentProgram = 0;
    GLuint currentPipeline = 0;

    bool insideBeginEnd = false;
    bool insideList = false;
    bool needsFlush = false;

    bool used = false;

    bool KHR_debug = false;
    GLsizei maxDebugMessageLength = 0;

    inline glfeatures::Profile
    profile(void) const {
        return wsContext->profile;
    }

    inline glfeatures::Profile
    actualProfile(void) const {
        return wsContext->actualProfile;
    }

    inline const glfeatures::Features &
    features(void) const {
        return wsContext->actualFeatures;
    }

    inline bool
    hasExtension(const char *extension) const {
        return wsContext->hasExtension(extension);
    }
};


class GLWs
{
private:
    std::map<glfeatures::Profile, glws::Visual *> visuals;

    glws::Drawable *
    createDrawableHelper(glfeatures::Profile profile, int width = 32, int height = 32,
                         const glws::pbuffer_info *pbInfo = NULL);

    static OS_THREAD_LOCAL Context * currentContextPtr;

    inline glws::Visual * getVisual(glfeatures::Profile profile) {
        std::map<glfeatures::Profile, glws::Visual *>::iterator it = visuals.find(profile);
        if (it == visuals.end()) {
            glws::Visual *visual = NULL;
            unsigned samples = retrace::samples;
            /* The requested number of samples might not be available, try fewer until we succeed */
            while (!visual && samples > 0) {
                visual = glws::createVisual(retrace::doubleBuffer, samples, profile);
                if (!visual) {
                    samples--;
                }
            }
            if (!visual) {
                std::cerr << "error: failed to create OpenGL visual\n";
                exit(1);
            }
            if (samples != retrace::samples) {
                std::cerr << "warning: Using " << samples << " samples instead of the requested " << retrace::samples << "\n";
            }
            visuals[profile] = visual;
            return visual;
        }
        return it->second;
    }

public:
    inline Context *
    getCurrentContext(void) {
        return currentContextPtr;
    }

    glws::Drawable *
        createDrawable(glfeatures::Profile profile);

    glws::Drawable *
        createDrawable(void);

    glws::Drawable *
        createPbuffer(int width, int height, const glws::pbuffer_info *pbInfo);

    Context *
        createContext(Context *shareContext, glfeatures::Profile profile);

    Context *
        createContext(Context *shareContext = 0);

    bool
        makeCurrent(trace::Call &call, glws::Drawable *drawable, Context *context);

    /**
     * Grow the current drawble.
     *
     * We need to infer the drawable size from GL calls because the drawable sizes
     * are specified by OS specific calls which we do not trace.
     */
    void
        updateDrawable(int width, int height);

    int
        parseAttrib(const trace::Value *attribs, int param, int default_ = 0, int terminator = 0);

    /**
     * Parse GLX/WGL_ARB_create_context attribute list.
     */
    glfeatures::Profile
        parseContextAttribList(const trace::Value *attribs);

    // WGL_ARB_render_texture / wglBindTexImageARB()
    bool
        bindTexImage(glws::Drawable *pBuffer, int iBuffer);

    // WGL_ARB_render_texture / wglReleaseTexImageARB()
    bool
        releaseTexImage(glws::Drawable *pBuffer, int iBuffer);

    // WGL_ARB_render_texture / wglSetPbufferAttribARB()
    bool
        setPbufferAttrib(glws::Drawable *pBuffer, const int *attribs);
};


} /* namespace glretrace */
