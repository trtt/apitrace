/**************************************************************************
 *
 * Copyright 2016 VMware, Inc.
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


#pragma once

#include "glimports.hpp"

#include "glretrace.hpp"


namespace glretrace {


class GLInterfaceWGL : public GLInterface
{
public:
    inline GLInterfaceWGL(GLWs* glws) : GLInterface(glws) {}

    ~GLInterfaceWGL() {}

    void registerCallbacks(retrace::Retracer &retracer);

private:
    struct Bitmap {
        GLsizei width;
        GLsizei height;
        GLfloat xorig;
        GLfloat yorig;
        GLfloat xmove;
        GLfloat ymove;
        const char * pixels;
    };

    static const Bitmap wglSystemFontBitmaps[256];

    typedef std::map<unsigned long long, glws::Drawable *> DrawableMap;
    typedef std::map<unsigned long long, Context *> ContextMap;
    DrawableMap drawable_map;
    DrawableMap pbuffer_map;
    ContextMap context_map;


    glws::Drawable *
        getDrawable(unsigned long long hdc);

    Context *
        getContext(unsigned long long context_ptr);

    void retrace_wglCreateContext(trace::Call &call);

    void retrace_wglDeleteContext(trace::Call &call);

    void retrace_wglMakeCurrent(trace::Call &call);

    void retrace_wglSwapBuffers(trace::Call &call);

    void retrace_wglShareLists(trace::Call &call);

    void retrace_wglCreateLayerContext(trace::Call &call);

    void retrace_wglSwapLayerBuffers(trace::Call &call);

    void retrace_wglCreatePbufferARB(trace::Call &call);

    void retrace_wglGetPbufferDCARB(trace::Call &call);

    void retrace_wglCreateContextAttribsARB(trace::Call &call);

    GLenum
        wgl_buffer_to_enum(int iBuffer);

    void retrace_wglBindTexImageARB(trace::Call &call);

    void retrace_wglReleaseTexImageARB(trace::Call &call);

    void retrace_wglSetPbufferAttribARB(trace::Call &call);

    void retrace_wglUseFontBitmapsAW(trace::Call &call);
};

} /* namespace glretrace */
