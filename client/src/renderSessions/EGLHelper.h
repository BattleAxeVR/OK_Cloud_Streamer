/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef CLIENT_APP_OVR_EGLHELPER_H
#define CLIENT_APP_OVR_EGLHELPER_H

#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

class EGLHelper
{
public:
    typedef intptr_t Handle;

    bool Initialize();
    void Release();
    bool IsValid() { return mContext != 0 && mSurface != 0; }

    void MakeCurrent();
    void Kickoff();

    Handle GetDisplay() {return mDisplay;}
    Handle GetContext() {return mContext;}

    // Cross-context fence
    static Handle PushFence();
    static void WaitFence(Handle fence, bool onClient=false);
    static void ReleaseFence(Handle fence);

    static Handle GetCurrent();

private:
    static Handle mDisplay;
    Handle mContext = 0;
    Handle mSurface = 0;
    Handle mConfig = 0;

    struct HelperEGLConfig
    {
        HelperEGLConfig( uint32_t r = 8, uint32_t g = 8, uint32_t b = 8, uint32_t a = 8,
                            uint32_t d = 0, uint32_t s = 0, uint32_t msaa = 0,
                            int32_t esApiBit = EGL_OPENGL_ES3_BIT
            ) : apiBit(esApiBit), redBits(r), greenBits(g), blueBits(b), alphaBits(a),
                depthBits(d), stencilBits(s), msaaSamples(msaa) {}

        int32_t apiBit;
        uint32_t redBits;
        uint32_t greenBits;
        uint32_t blueBits;
        uint32_t alphaBits;
        uint32_t depthBits;
        uint32_t stencilBits;
        uint32_t msaaSamples;
    };

    bool ChooseConfig(EGLDisplay disp, const struct HelperEGLConfig &wanted, EGLConfig &bestConfig);
};

#endif //CLIENT_APP_OVR_EGLHELPER_H
