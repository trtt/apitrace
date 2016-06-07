/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
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

#include "retrace.hpp"
#include "glws.hpp"
#include "glretrace_ws.hpp"


namespace glretrace {


class GLInterfaceGLX : public GLInterface
{
public:
    inline GLInterfaceGLX(GLWs* glws) : GLInterface(glws) {}

    ~GLInterfaceGLX();

private:
    typedef std::map<unsigned long, glws::Drawable *> DrawableMap;
    typedef std::map<unsigned long long, Context *> ContextMap;
    DrawableMap drawable_map;
    ContextMap context_map;

    glws::Drawable *
        getDrawable(unsigned long drawable_id);

    Context *
        getContext(unsigned long long context_ptr);

    void registerCallbacks(retrace::Retracer &retracer);

    void retrace_glXCreateContext(trace::Call &call);

    void retrace_glXCreateContextAttribsARB(trace::Call &call);

    void retrace_glXMakeCurrent(trace::Call &call);

    void retrace_glXDestroyContext(trace::Call &call);

    void retrace_glXCopySubBufferMESA(trace::Call &call);

    void retrace_glXSwapBuffers(trace::Call &call);

    void retrace_glXCreateNewContext(trace::Call &call);

    void retrace_glXCreatePbuffer(trace::Call &call);

    void retrace_glXDestroyPbuffer(trace::Call &call);

    void retrace_glXMakeContextCurrent(trace::Call &call);
};

} /* namespace glretrace */
