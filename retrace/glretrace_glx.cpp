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


#include "glproc.hpp"
#include "retrace.hpp"
#include "glretrace.hpp"
#include "glretrace_glx.hpp"

#if !defined(HAVE_X11)

#define GLX_PBUFFER_HEIGHT 0x8040
#define GLX_PBUFFER_WIDTH 0x8041

#endif /* !HAVE_X11 */


using namespace glretrace;


glws::Drawable *
GLInterfaceGLX::getDrawable(unsigned long drawable_id) {
    if (drawable_id == 0) {
        return NULL;
    }

    DrawableMap::const_iterator it;
    it = drawable_map.find(drawable_id);
    if (it == drawable_map.end()) {
        return (drawable_map[drawable_id] = glws.createDrawable());
    }

    return it->second;
}

Context *
GLInterfaceGLX::getContext(unsigned long long context_ptr) {
    if (context_ptr == 0) {
        return NULL;
    }

    ContextMap::const_iterator it;
    it = context_map.find(context_ptr);
    if (it == context_map.end()) {
        return (context_map[context_ptr] = glws.createContext());
    }

    return it->second;
}

void GLInterfaceGLX::retrace_glXCreateContext(trace::Call &call) {
    unsigned long long orig_context = call.ret->toUIntPtr();
    if (!orig_context) {
        return;
    }

    Context *share_context = getContext(call.arg(2).toUIntPtr());

    Context *context = glws.createContext(share_context);
    context_map[orig_context] = context;
}

void GLInterfaceGLX::retrace_glXCreateContextAttribsARB(trace::Call &call) {
    unsigned long long orig_context = call.ret->toUIntPtr();
    if (!orig_context) {
        return;
    }

    Context *share_context = getContext(call.arg(2).toUIntPtr());

    const trace::Value * attrib_list = &call.arg(4);
    glfeatures::Profile profile = glws.parseContextAttribList(attrib_list);

    Context *context = glws.createContext(share_context, profile);
    context_map[orig_context] = context;
}

void GLInterfaceGLX::retrace_glXMakeCurrent(trace::Call &call) {
    if (call.ret && !call.ret->toBool()) {
        // If false was returned then any previously current rendering context
        // and drawable remain unchanged.
        return;
    }

    glws::Drawable *new_drawable = getDrawable(call.arg(1).toUInt());
    Context *new_context = getContext(call.arg(2).toUIntPtr());

    glws.makeCurrent(call, new_drawable, new_context);
}


void GLInterfaceGLX::retrace_glXDestroyContext(trace::Call &call) {
    ContextMap::iterator it;
    it = context_map.find(call.arg(1).toUIntPtr());
    if (it == context_map.end()) {
        return;
    }

    it->second->release();

    context_map.erase(it);
}

void GLInterfaceGLX::retrace_glXCopySubBufferMESA(trace::Call &call) {
    glws::Drawable *drawable = getDrawable(call.arg(1).toUInt());
    int x = call.arg(2).toSInt();
    int y = call.arg(3).toSInt();
    int width = call.arg(4).toSInt();
    int height = call.arg(5).toSInt();

    drawable->copySubBuffer(x, y, width, height);
}

void GLInterfaceGLX::retrace_glXSwapBuffers(trace::Call &call) {
    glws::Drawable *drawable = getDrawable(call.arg(1).toUInt());

    frame_complete(call);
    if (retrace::doubleBuffer) {
        if (drawable) {
            drawable->swapBuffers();
        }
    } else {
        glFlush();
    }
}

void GLInterfaceGLX::retrace_glXCreateNewContext(trace::Call &call) {
    unsigned long long orig_context = call.ret->toUIntPtr();
    if (!orig_context) {
        return;
    }

    Context *share_context = getContext(call.arg(3).toUIntPtr());

    Context *context = glws.createContext(share_context);
    context_map[orig_context] = context;
}

void GLInterfaceGLX::retrace_glXCreatePbuffer(trace::Call &call) {
    unsigned long long orig_drawable = call.ret->toUInt();
    if (!orig_drawable) {
        return;
    }

    const trace::Value *attrib_list = &call.arg(2);
    int width = glws.parseAttrib(attrib_list, GLX_PBUFFER_WIDTH, 0);
    int height = glws.parseAttrib(attrib_list, GLX_PBUFFER_HEIGHT, 0);
    glws::pbuffer_info pbInfo = {0, 0, false};

    glws::Drawable *drawable = glws.createPbuffer(width, height, &pbInfo);
    
    drawable_map[orig_drawable] = drawable;
}

void GLInterfaceGLX::retrace_glXDestroyPbuffer(trace::Call &call) {
    glws::Drawable *drawable = getDrawable(call.arg(1).toUInt());

    if (!drawable) {
        return;
    }

    delete drawable;
}

void GLInterfaceGLX::retrace_glXMakeContextCurrent(trace::Call &call) {
    if (call.ret && !call.ret->toBool()) {
        // If false was returned then any previously current rendering context
        // and drawable remain unchanged.
        return;
    }

    glws::Drawable *new_drawable = getDrawable(call.arg(1).toUInt());
    Context *new_context = getContext(call.arg(3).toUIntPtr());

    glws.makeCurrent(call, new_drawable, new_context);
}

void GLInterfaceGLX::registerCallbacks(retrace::Retracer &retracer) {
    using namespace std::placeholders;

    #define wrap(func) std::bind(&GLInterfaceGLX::func, this, _1)

    const retrace::Entry glx_callbacks[] = {
        //{"glXBindChannelToWindowSGIX", wrap(retrace_glXBindChannelToWindowSGIX)},
        //{"glXBindSwapBarrierNV", wrap(retrace_glXBindSwapBarrierNV)},
        //{"glXBindSwapBarrierSGIX", wrap(retrace_glXBindSwapBarrierSGIX)},
        {"glXBindTexImageEXT", retrace::ignore},
        //{"glXChannelRectSGIX", wrap(retrace_glXChannelRectSGIX)},
        //{"glXChannelRectSyncSGIX", wrap(retrace_glXChannelRectSyncSGIX)},
        {"glXChooseFBConfig", retrace::ignore},
        {"glXChooseFBConfigSGIX", retrace::ignore},
        {"glXChooseVisual", retrace::ignore},
        //{"glXCopyContext", wrap(retrace_glXCopyContext)},
        //{"glXCopyImageSubDataNV", wrap(retrace_glXCopyImageSubDataNV)},
        {"glXCopySubBufferMESA", wrap(retrace_glXCopySubBufferMESA)},
        {"glXCreateContextAttribsARB", wrap(retrace_glXCreateContextAttribsARB)},
        {"glXCreateContext", wrap(retrace_glXCreateContext)},
        //{"glXCreateContextWithConfigSGIX", wrap(retrace_glXCreateContextWithConfigSGIX)},
        //{"glXCreateGLXPbufferSGIX", wrap(retrace_glXCreateGLXPbufferSGIX)},
        //{"glXCreateGLXPixmap", wrap(retrace_glXCreateGLXPixmap)},
        //{"glXCreateGLXPixmapWithConfigSGIX", wrap(retrace_glXCreateGLXPixmapWithConfigSGIX)},
        {"glXCreateNewContext", wrap(retrace_glXCreateNewContext)},
        {"glXCreatePbuffer", wrap(retrace_glXCreatePbuffer)},
        {"glXCreatePixmap", retrace::ignore},
        //{"glXCreateWindow", wrap(retrace_glXCreateWindow)},
        //{"glXCushionSGI", wrap(retrace_glXCushionSGI)},
        {"glXDestroyContext", wrap(retrace_glXDestroyContext)},
        //{"glXDestroyGLXPbufferSGIX", wrap(retrace_glXDestroyGLXPbufferSGIX)},
        //{"glXDestroyGLXPixmap", wrap(retrace_glXDestroyGLXPixmap)},
        {"glXDestroyPbuffer", wrap(retrace_glXDestroyPbuffer)},
        {"glXDestroyPixmap", retrace::ignore},
        //{"glXDestroyWindow", wrap(retrace_glXDestroyWindow)},
        //{"glXFreeContextEXT", wrap(retrace_glXFreeContextEXT)},
        {"glXGetAGPOffsetMESA", retrace::ignore},
        {"glXGetClientString", retrace::ignore},
        {"glXGetConfig", retrace::ignore},
        {"glXGetContextIDEXT", retrace::ignore},
        {"glXGetCurrentContext", retrace::ignore},
        {"glXGetCurrentDisplayEXT", retrace::ignore},
        {"glXGetCurrentDisplay", retrace::ignore},
        {"glXGetCurrentDrawable", retrace::ignore},
        {"glXGetCurrentReadDrawable", retrace::ignore},
        {"glXGetCurrentReadDrawableSGI", retrace::ignore},
        {"glXGetFBConfigAttrib", retrace::ignore},
        {"glXGetFBConfigAttribSGIX", retrace::ignore},
        {"glXGetFBConfigFromVisualSGIX", retrace::ignore},
        {"glXGetFBConfigs", retrace::ignore},
        {"glXGetMscRateOML", retrace::ignore},
        {"glXGetProcAddressARB", retrace::ignore},
        {"glXGetProcAddress", retrace::ignore},
        {"glXGetSelectedEvent", retrace::ignore},
        {"glXGetSelectedEventSGIX", retrace::ignore},
        {"glXGetSwapIntervalMESA", retrace::ignore},
        {"glXGetSyncValuesOML", retrace::ignore},
        {"glXGetVideoSyncSGI", retrace::ignore},
        {"glXGetVisualFromFBConfig", retrace::ignore},
        {"glXGetVisualFromFBConfigSGIX", retrace::ignore},
        //{"glXImportContextEXT", wrap(retrace_glXImportContextEXT)},
        {"glXIsDirect", retrace::ignore},
        //{"glXJoinSwapGroupNV", wrap(retrace_glXJoinSwapGroupNV)},
        //{"glXJoinSwapGroupSGIX", wrap(retrace_glXJoinSwapGroupSGIX)},
        {"glXMakeContextCurrent", wrap(retrace_glXMakeContextCurrent)},
        //{"glXMakeCurrentReadSGI", wrap(retrace_glXMakeCurrentReadSGI)},
        {"glXMakeCurrent", wrap(retrace_glXMakeCurrent)},
        {"glXQueryChannelDeltasSGIX", retrace::ignore},
        {"glXQueryChannelRectSGIX", retrace::ignore},
        {"glXQueryContextInfoEXT", retrace::ignore},
        {"glXQueryContext", retrace::ignore},
        {"glXQueryDrawable", retrace::ignore},
        {"glXQueryExtension", retrace::ignore},
        {"glXQueryExtensionsString", retrace::ignore},
        {"glXQueryFrameCountNV", retrace::ignore},
        {"glXQueryGLXPbufferSGIX", retrace::ignore},
        {"glXQueryMaxSwapBarriersSGIX", retrace::ignore},
        {"glXQueryMaxSwapGroupsNV", retrace::ignore},
        {"glXQueryServerString", retrace::ignore},
        {"glXQuerySwapGroupNV", retrace::ignore},
        {"glXQueryVersion", retrace::ignore},
        //{"glXReleaseBuffersMESA", wrap(retrace_glXReleaseBuffersMESA)},
        {"glXReleaseTexImageEXT", retrace::ignore},
        //{"glXResetFrameCountNV", wrap(retrace_glXResetFrameCountNV)},
        //{"glXSelectEvent", wrap(retrace_glXSelectEvent)},
        //{"glXSelectEventSGIX", wrap(retrace_glXSelectEventSGIX)},
        //{"glXSet3DfxModeMESA", wrap(retrace_glXSet3DfxModeMESA)},
        //{"glXSwapBuffersMscOML", wrap(retrace_glXSwapBuffersMscOML)},
        {"glXSwapBuffers", wrap(retrace_glXSwapBuffers)},
        {"glXSwapIntervalEXT", retrace::ignore},
        {"glXSwapIntervalMESA", retrace::ignore},
        {"glXSwapIntervalSGI", retrace::ignore},
        //{"glXUseXFont", wrap(retrace_glXUseXFont)},
        {"glXWaitForMscOML", retrace::ignore},
        {"glXWaitForSbcOML", retrace::ignore},
        {"glXWaitGL", retrace::ignore},
        {"glXWaitVideoSyncSGI", retrace::ignore},
        {"glXWaitX", retrace::ignore},
        {NULL, NULL},
    };
    #undef wrap

    retracer.addCallbacks(glx_callbacks);
}

