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

#include "glws.hpp"
#include "retrace.hpp"
#include "metric_backend.hpp"
#include "metric_writer.hpp"
#include "glretrace_ws.hpp"

#include "os_thread.hpp"


namespace glretrace {

class GLInterface
{
protected:
    GLWs &glws;

public:
    GLInterface(GLWs* glws) : glws(*glws) {}
    virtual ~GLInterface() {}

    virtual void registerCallbacks(retrace::Retracer &retracer) = 0;
};

extern std::vector<std::unique_ptr<GLInterface>> interfaces;
extern GLWs* glws;

extern bool metricBackendsSetup;
extern bool profilingContextAcquired;
extern bool profilingBoundaries[QUERY_BOUNDARY_LIST_END];
extern unsigned profilingBoundariesIndex[QUERY_BOUNDARY_LIST_END];
extern std::vector<MetricBackend*> metricBackends;
extern MetricBackend* curMetricBackend;
extern MetricWriter profiler;

extern glfeatures::Profile defaultProfile;

extern bool supportsARBShaderObjects;


static inline Context *
getCurrentContext(void) {
    return glws->getCurrentContext();
}


void
checkGlError(trace::Call &call);

void
insertCallMarker(trace::Call &call, Context *currentContext);


extern const retrace::Entry gl_callbacks[];
//extern const retrace::Entry cgl_callbacks[];
//extern const retrace::Entry glx_callbacks[];
//extern const retrace::Entry wgl_callbacks[];
//extern const retrace::Entry egl_callbacks[];

void frame_complete(trace::Call &call);
void initContext();
void beforeContextSwitch();
void afterContextSwitch();


void flushQueries();
void beginProfile(trace::Call &call, bool isDraw);
void endProfile(trace::Call &call, bool isDraw);

MetricBackend* getBackend(std::string backendName);

bool isLastPass();

void listMetricsCLI();

void enableMetricsFromCLI(const char* metrics, QueryBoundary pollingRule);

GLenum
blockOnFence(trace::Call &call, GLsync sync, GLbitfield flags);

GLenum
clientWaitSync(trace::Call &call, GLsync sync, GLbitfield flags, GLuint64 timeout);


} /* namespace glretrace */


