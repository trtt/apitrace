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
#include "retrace.hpp"
#include "glretrace.hpp"
#include "os.hpp"
#include "eglsize.hpp"
#include "glretrace_egl.hpp"

using namespace glretrace;


glws::Drawable *
GLInterfaceEGL::getDrawable(unsigned long long surface_ptr) {
    if (surface_ptr == 0) {
        return NULL;
    }

    DrawableMap::const_iterator it;
    it = drawable_map.find(surface_ptr);
    if (it == drawable_map.end()) {
        // In Fennec we get the egl window surface from Java which isn't
        // traced, so just create a drawable if it doesn't exist in here
        createDrawable(0, surface_ptr);
        it = drawable_map.find(surface_ptr);
        assert(it != drawable_map.end());
    }

    return (it != drawable_map.end()) ? it->second : NULL;
}

Context *
GLInterfaceEGL::getContext(unsigned long long context_ptr) {
    if (context_ptr == 0) {
        return NULL;
    }

    ContextMap::const_iterator it;
    it = context_map.find(context_ptr);

    return (it != context_map.end()) ? it->second : NULL;
}

void GLInterfaceEGL::createDrawable(unsigned long long orig_config, unsigned long long orig_surface)
{
    ProfileMap::iterator it = profile_map.find(orig_config);
    glfeatures::Profile profile;

    // If the requested config is associated with a profile, use that
    // profile. Otherwise, assume that the last used profile is what
    // the user wants.
    if (it != profile_map.end()) {
        profile = it->second;
    } else {
        profile = last_profile;
    }

    glws::Drawable *drawable = glws.createDrawable(profile);
    drawable_map[orig_surface] = drawable;
}

void GLInterfaceEGL::retrace_eglChooseConfig(trace::Call &call) {
    if (!call.ret->toSInt()) {
        return;
    }

    trace::Array *attrib_array = call.arg(1).toArray();
    trace::Array *config_array = call.arg(2).toArray();
    trace::Array *num_config_ptr = call.arg(4).toArray();
    if (!attrib_array || !config_array || !num_config_ptr) {
        return;
    }

    glfeatures::Profile profile;
    unsigned renderableType = glws.parseAttrib(attrib_array, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT);
    std::cerr << "renderableType = " << renderableType << "\n";
    if (renderableType & EGL_OPENGL_BIT) {
        profile = glfeatures::Profile(glfeatures::API_GL, 1, 0);
    } else {
        profile.api = glfeatures::API_GLES;
        if (renderableType & EGL_OPENGL_ES3_BIT) {
            profile.major = 3;
        } else if (renderableType & EGL_OPENGL_ES2_BIT) {
            profile.major = 2;
        } else {
            profile.major = 1;
        }
    }

    unsigned num_config = num_config_ptr->values[0]->toUInt();
    for (unsigned i = 0; i < num_config; ++i) {
        unsigned long long orig_config = config_array->values[i]->toUIntPtr();
        profile_map[orig_config] = profile;
    }
}

void GLInterfaceEGL::retrace_eglCreateWindowSurface(trace::Call &call) {
    unsigned long long orig_config = call.arg(1).toUIntPtr();
    unsigned long long orig_surface = call.ret->toUIntPtr();
    createDrawable(orig_config, orig_surface);
}

void GLInterfaceEGL::retrace_eglCreatePbufferSurface(trace::Call &call) {
    unsigned long long orig_config = call.arg(1).toUIntPtr();
    unsigned long long orig_surface = call.ret->toUIntPtr();
    createDrawable(orig_config, orig_surface);
    // TODO: Respect the pbuffer dimensions too
}

void GLInterfaceEGL::retrace_eglDestroySurface(trace::Call &call) {
    unsigned long long orig_surface = call.arg(1).toUIntPtr();

    DrawableMap::iterator it;
    it = drawable_map.find(orig_surface);

    if (it != drawable_map.end()) {
        glretrace::Context *currentContext = glretrace::getCurrentContext();
        if (!currentContext || it->second != currentContext->drawable) {
            // TODO: reference count
            delete it->second;
        }
        drawable_map.erase(it);
    }
}

void GLInterfaceEGL::retrace_eglBindAPI(trace::Call &call) {
    if (!call.ret->toBool()) {
        return;
    }

    current_api = call.arg(0).toUInt();
}

void GLInterfaceEGL::retrace_eglCreateContext(trace::Call &call) {
    unsigned long long orig_context = call.ret->toUIntPtr();
    unsigned long long orig_config = call.arg(1).toUIntPtr();
    Context *share_context = getContext(call.arg(2).toUIntPtr());
    trace::Array *attrib_array = call.arg(3).toArray();
    glfeatures::Profile profile;

    switch (current_api) {
    case EGL_OPENGL_API:
        profile.api = glfeatures::API_GL;
        profile.major = glws.parseAttrib(attrib_array, EGL_CONTEXT_MAJOR_VERSION, 1);
        profile.minor = glws.parseAttrib(attrib_array, EGL_CONTEXT_MINOR_VERSION, 0);
        if (profile.versionGreaterOrEqual(3,2)) {
             int profileMask = glws.parseAttrib(attrib_array, EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);
             if (profileMask & EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR) {
                 profile.core = true;
             }
             int contextFlags = glws.parseAttrib(attrib_array, EGL_CONTEXT_FLAGS_KHR, 0);
             if (contextFlags & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR) {
                 profile.forwardCompatible = true;
             }
        }
        break;
    case EGL_OPENGL_ES_API:
    default:
        profile.api = glfeatures::API_GLES;
        profile.major = glws.parseAttrib(attrib_array, EGL_CONTEXT_MAJOR_VERSION, 1);
        profile.minor = glws.parseAttrib(attrib_array, EGL_CONTEXT_MINOR_VERSION, 0);
        break;
    }


    Context *context = glws.createContext(share_context, profile);
    assert(context);

    context_map[orig_context] = context;
    profile_map[orig_config] = profile;
    last_profile = profile;
}

void GLInterfaceEGL::retrace_eglDestroyContext(trace::Call &call) {
    unsigned long long orig_context = call.arg(1).toUIntPtr();

    ContextMap::iterator it;
    it = context_map.find(orig_context);

    if (it != context_map.end()) {
        glretrace::Context *currentContext = glretrace::getCurrentContext();
        if (it->second != currentContext) {
            // TODO: reference count
            it->second->release();
        }
        context_map.erase(it);
    }
}

void GLInterfaceEGL::retrace_eglMakeCurrent(trace::Call &call) {
    if (!call.ret->toSInt()) {
        // Previously current rendering context and surfaces (if any) remain
        // unchanged.
        return;
    }

    glws::Drawable *new_drawable = getDrawable(call.arg(1).toUIntPtr());
    Context *new_context = getContext(call.arg(3).toUIntPtr());

    // Try to support GL_OES_surfaceless_context by creating a dummy drawable.
    if (new_context && !new_drawable) {
        if (!null_drawable) {
            null_drawable = glws.createDrawable(last_profile);
        }
        new_drawable = null_drawable;
    }

    glws.makeCurrent(call, new_drawable, new_context);
}


void GLInterfaceEGL::retrace_eglSwapBuffers(trace::Call &call) {
    glws::Drawable *drawable = getDrawable(call.arg(1).toUIntPtr());

    frame_complete(call);

    if (retrace::doubleBuffer) {
        if (drawable) {
            drawable->swapBuffers();
        }
    } else {
        glFlush();
    }
}

void GLInterfaceEGL::registerCallbacks(retrace::Retracer &retracer) {
    using namespace std::placeholders;

    #define wrap(func) std::bind(&GLInterfaceEGL::func, this, _1)

    const retrace::Entry egl_callbacks[] = {
        {"eglGetError", retrace::ignore},
        {"eglGetDisplay", retrace::ignore},
        {"eglInitialize", retrace::ignore},
        {"eglTerminate", retrace::ignore},
        {"eglQueryString", retrace::ignore},
        {"eglGetConfigs", retrace::ignore},
        {"eglChooseConfig", wrap(retrace_eglChooseConfig)},
        {"eglGetConfigAttrib", retrace::ignore},
        {"eglCreateWindowSurface", wrap(retrace_eglCreateWindowSurface)},
        {"eglCreatePbufferSurface", wrap(retrace_eglCreatePbufferSurface)},
        //{"eglCreatePixmapSurface", retrace::ignore},
        {"eglDestroySurface", wrap(retrace_eglDestroySurface)},
        {"eglQuerySurface", retrace::ignore},
        {"eglBindAPI", wrap(retrace_eglBindAPI)},
        {"eglQueryAPI", retrace::ignore},
        //{"eglWaitClient", retrace::ignore},
        //{"eglReleaseThread", retrace::ignore},
        //{"eglCreatePbufferFromClientBuffer", retrace::ignore},
        //{"eglSurfaceAttrib", retrace::ignore},
        //{"eglBindTexImage", retrace::ignore},
        //{"eglReleaseTexImage", retrace::ignore},
        {"eglSwapInterval", retrace::ignore},
        {"eglCreateContext", wrap(retrace_eglCreateContext)},
        {"eglDestroyContext", wrap(retrace_eglDestroyContext)},
        {"eglMakeCurrent", wrap(retrace_eglMakeCurrent)},
        {"eglGetCurrentContext", retrace::ignore},
        {"eglGetCurrentSurface", retrace::ignore},
        {"eglGetCurrentDisplay", retrace::ignore},
        {"eglQueryContext", retrace::ignore},
        {"eglWaitGL", retrace::ignore},
        {"eglWaitNative", retrace::ignore},
        {"eglReleaseThread", retrace::ignore},
        {"eglSwapBuffers", wrap(retrace_eglSwapBuffers)},
        {"eglSwapBuffersWithDamageEXT", wrap(retrace_eglSwapBuffers)},  // ignores additional params
        {"eglSwapBuffersWithDamageKHR", wrap(retrace_eglSwapBuffers)},  // ignores additional params
        //{"eglCopyBuffers", retrace::ignore},
        {"eglGetProcAddress", retrace::ignore},
        {"eglCreateImageKHR", retrace::ignore},
        {"eglDestroyImageKHR", retrace::ignore},
        {"glEGLImageTargetTexture2DOES", retrace::ignore},
        {NULL, NULL},
    };
    #undef wrap

    retracer.addCallbacks(egl_callbacks);
}
