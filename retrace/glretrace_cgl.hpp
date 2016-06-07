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

#include "glproc.hpp"
#include "glretrace.hpp"


namespace glretrace {


struct PixelFormat
{
    glfeatures::Profile profile;

    inline PixelFormat() :
        profile(glfeatures::API_GL, 1, 0)
    {}
};


class GLInterfaceCGL : public GLInterface
{
public:
    inline GLInterfaceCGL(GLWs* glws) : GLInterface(glws) {}

    ~GLInterfaceCGL() {}

    void registerCallbacks(retrace::Retracer &retracer);

private:
    typedef std::map<unsigned long long, glws::Drawable *> DrawableMap;
    typedef std::map<unsigned long long, Context *> ContextMap;

    // sid -> Drawable* map
    DrawableMap drawable_map;

    // ctx -> Context* map
    ContextMap context_map;

    Context *sharedContext = NULL;


    glws::Drawable *
        getDrawable(unsigned long drawable_id, glfeatures::Profile profile);

    Context *
        getContext(unsigned long long ctx);

    void retrace_CGLChoosePixelFormat(trace::Call &call);

    void retrace_CGLDestroyPixelFormat(trace::Call &call);

    void retrace_CGLCreateContext(trace::Call &call);

    void retrace_CGLDestroyContext(trace::Call &call);

    void retrace_CGLSetSurface(trace::Call &call);

    void retrace_CGLClearDrawable(trace::Call &call);

    void retrace_CGLSetCurrentContext(trace::Call &call);

    void retrace_CGLFlushDrawable(trace::Call &call);

    void retrace_CGLSetVirtualScreen(trace::Call &call);

    void retrace_CGLTexImageIOSurface2D(trace::Call &call);
};

} /* namespace glretrace */
