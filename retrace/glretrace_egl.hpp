/**************************************************************************
 *
 * Copyright 2011 LunarG, Inc.
 * All Rights Reserved.
 *
 * Based on glretrace_glx.cpp, which has
 *
 *   Copyright 2011 Jose Fonseca
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


#include "glproc.hpp"
#include "glretrace.hpp"
#include "eglsize.hpp"
#include "glws.hpp"

#ifndef EGL_OPENGL_ES_API
#define EGL_OPENGL_ES_API		0x30A0
#define EGL_OPENVG_API			0x30A1
#define EGL_OPENGL_API			0x30A2
#define EGL_CONTEXT_CLIENT_VERSION	0x3098
#endif


namespace glretrace {


class GLInterfaceEGL : public GLInterface
{
public:
    inline GLInterfaceEGL(GLWs* glws) : GLInterface(glws),
                            last_profile(glfeatures::API_GLES, 2, 0) {}

    ~GLInterfaceEGL() {}

    void registerCallbacks(retrace::Retracer &retracer);

private:
    typedef std::map<unsigned long long, glws::Drawable *> DrawableMap;
    typedef std::map<unsigned long long, Context *> ContextMap;
    typedef std::map<unsigned long long, glfeatures::Profile> ProfileMap;
    DrawableMap drawable_map;
    ContextMap context_map;
    ProfileMap profile_map;

    /* FIXME: This should be tracked per thread. */
    unsigned int current_api = EGL_OPENGL_ES_API;

    /*
     * FIXME: Ideally we would defer the context creation until the profile was
     * clear, as explained in https://github.com/apitrace/apitrace/issues/197 ,
     * instead of guessing.  For now, start with a guess of ES2 profile, which
     * should be the most common case for EGL.
     */
    glfeatures::Profile last_profile;

    glws::Drawable *null_drawable = NULL;


    void
        createDrawable(unsigned long long orig_config, unsigned long long orig_surface);

    glws::Drawable *
        getDrawable(unsigned long long surface_ptr);

    Context *
        getContext(unsigned long long context_ptr);

    void retrace_eglChooseConfig(trace::Call &call);

    void retrace_eglCreateWindowSurface(trace::Call &call);

    void retrace_eglCreatePbufferSurface(trace::Call &call);

    void retrace_eglDestroySurface(trace::Call &call);

    void retrace_eglBindAPI(trace::Call &call);

    void retrace_eglCreateContext(trace::Call &call);

    void retrace_eglDestroyContext(trace::Call &call);

    void retrace_eglMakeCurrent(trace::Call &call);


    void retrace_eglSwapBuffers(trace::Call &call);
};

} /* namespace glretrace */
