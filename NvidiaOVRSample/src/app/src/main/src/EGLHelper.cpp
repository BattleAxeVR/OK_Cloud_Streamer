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

#include <stdlib.h>
#include <unistd.h>
#include "EGLHelper.h"

#define LOG_TAG "EGLHelper"
#include "CloudXRLog.h"

//-----------------------------------------------------------------------------
// static member variables..
EGLHelper::Handle EGLHelper::mDisplay = 0;

//-----------------------------------------------------------------------------
bool EGLHelper::Initialize()
{
    if (mContext != 0)
        return true; // already initialized

    const EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglBindAPI(EGL_OPENGL_ES_API);
    eglInitialize(display, 0, 0);

    EGLConfig bestConfig;
    EGLHelper::HelperEGLConfig questConfig(8, 8, 8, 8, EGL_DEPTH_SIZE, 16, 4, EGL_OPENGL_ES3_BIT);
    if (!ChooseConfig(display, questConfig, bestConfig))
    {
        CXR_LOGE("EGLHelper failed to choose a display config!");
        return false;
    }

    EGLint format;
    if (!eglGetConfigAttrib(display, bestConfig, EGL_NATIVE_VISUAL_ID, &format))
    {
        CXR_LOGE("EGLHelper failed to get native visual format!");
        return false;
    }

    static EGLint contextAttrs[] =
    {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE,
    };

    EGLContext context = eglCreateContext(display, bestConfig, EGL_NO_CONTEXT, contextAttrs);
    if (context == nullptr)
    {
        CXR_LOGE("EGLHelper: CreateContext failed.");
        return false; // early exit, nothing to clean up.
    }

    mDisplay = (Handle)display;
    mConfig = (Handle)bestConfig;
    mContext = (Handle)context;

    static EGLint pbufferAttrs[32] =
    {
            EGL_WIDTH, 4,
            EGL_HEIGHT, 4,
            EGL_NONE,
    };

    const EGLSurface surface = eglCreatePbufferSurface(display, bestConfig, pbufferAttrs);
    if (surface == nullptr)
    {
        CXR_LOGE("EGLHelper unable to create pbuffer surface");
        eglDestroyContext(display, context);
        return false;
    }

    CXR_LOGV("EGLHelper using pbuffer context");
    mSurface = (Handle) surface;

    MakeCurrent();

    return true;
}

//-----------------------------------------------------------------------------
void EGLHelper::Release()
{
    eglMakeCurrent((EGLDisplay)mDisplay,
                   (EGLSurface)NULL, (EGLSurface)NULL, (EGLContext)NULL);

    if (mContext)
    {
        eglDestroyContext((EGLDisplay)mDisplay, (EGLContext)mContext);
        mContext = 0;
    }

    if (mSurface)
    {
        eglDestroySurface((EGLDisplay)mDisplay, (EGLSurface)mSurface);
        mSurface = 0;
    }

    if (mDisplay)
    {
        eglTerminate((EGLDisplay)mDisplay);
        mDisplay = 0;
    }
}

//-----------------------------------------------------------------------------
void EGLHelper::MakeCurrent()
{
    if (GetCurrent() == mContext)
        return;

    eglMakeCurrent((EGLDisplay)mDisplay,
                   (EGLSurface)mSurface, (EGLSurface)mSurface, (EGLContext)mContext);
}

//-----------------------------------------------------------------------------
EGLHelper::Handle EGLHelper::GetCurrent()
{
    return reinterpret_cast<Handle>(eglGetCurrentContext());
}

//-----------------------------------------------------------------------------
bool EGLHelper::ChooseConfig(EGLDisplay disp, const struct HelperEGLConfig &wanted, EGLConfig &bestConfig)
{

    EGLint count = 0;
    if (!eglGetConfigs(disp, NULL, 0, &count))
    {
        CXR_LOGE("ChooseConfig cannot query count of all configs");
        return false;
    }
    CXR_LOGV("ChooseConfig EGL config count = %d", count);

    EGLConfig* configs = new EGLConfig[count];
    if (!eglGetConfigs(disp, configs, count, &count))
    {
        CXR_LOGE("ChooseConfig cannot query all configs");
        return false;
    }

    int32_t bestMatch = 1<<30;
    int32_t bestIndex = -1;

    int32_t i;
    for (i = 0; i < count; i++)
    {
        EGLint surfaceType = 0;
        EGLint blueBits = 0;
        EGLint greenBits = 0;
        EGLint redBits = 0;
        EGLint alphaBits = 0;
        EGLint depthBits = 0;
        EGLint stencilBits = 0;
        EGLint renderableFlags = 0;
        EGLint msaaSamples = 0;

        eglGetConfigAttrib(disp, configs[i], EGL_SURFACE_TYPE, &surfaceType);
        eglGetConfigAttrib(disp, configs[i], EGL_BLUE_SIZE, &blueBits);
        eglGetConfigAttrib(disp, configs[i], EGL_GREEN_SIZE, &greenBits);
        eglGetConfigAttrib(disp, configs[i], EGL_RED_SIZE, &redBits);
        eglGetConfigAttrib(disp, configs[i], EGL_ALPHA_SIZE, &alphaBits);
        eglGetConfigAttrib(disp, configs[i], EGL_DEPTH_SIZE, &depthBits);
        eglGetConfigAttrib(disp, configs[i], EGL_STENCIL_SIZE, &stencilBits);
        eglGetConfigAttrib(disp, configs[i], EGL_RENDERABLE_TYPE, &renderableFlags);
        eglGetConfigAttrib(disp, configs[i], EGL_SAMPLES, &msaaSamples);

        // Currently, this NEEDS to be a window-compatible config
        if ((surfaceType & EGL_WINDOW_BIT) == 0)
            continue;
        // also want pbuffer-compat config
        if ((surfaceType & EGL_PBUFFER_BIT) == 0)
            continue;
        // Require the requested API level
        if ((renderableFlags & wanted.apiBit) == 0)
            continue;

        // Used when a feature is big mismatch to desired value
        const int32_t MAJOR_PENALTY = 50;
        // Used when a feature is small difference from desired value
        const int32_t MINOR_PENALTY = 1;
        // we track penalty 'cost' per config and look for smallest
        int32_t penalty = 0;

        if (wanted.depthBits > 0)
        {
            // If the user requests a depth buffer, then we want to match what they
            // request _or_ better.  Larger will be a minor penalty, smaller is major,
            // none is a showstopper.  Same for stencil.
            if (depthBits == 0)
                continue;

            if (depthBits < wanted.depthBits)
            {
                penalty += MAJOR_PENALTY;
            }
            else if (depthBits > wanted.depthBits)
            {
                // buffer deeper than we want - penalty point for each extra 8 bits
                penalty += MINOR_PENALTY * ((depthBits - wanted.depthBits) >> 3);
            }
        }
        else
        {
            // If the user requests _no_ depth buffer, then it is a minor penalty to have
            // one, based on size of it
            if (depthBits > 0)
            {
                // buffer deeper than we want - penalty point for each extra 8 bits
                penalty += MINOR_PENALTY * (depthBits >> 3);
            }
        }

        if (wanted.stencilBits > 0)
        {
            // If the user requests a stencil buffer, then we want to match what they
            // request _or_ better.  Larger will be a minor penalty, smaller is major,
            // none is a showstopper.
            if (stencilBits == 0)
                continue;

            if (stencilBits < wanted.stencilBits)
            {
                penalty += MAJOR_PENALTY;
            }
            else if (stencilBits > wanted.stencilBits)
            {
                // buffer deeper than we want - penalty point for each extra 8 bits
                penalty += MINOR_PENALTY * ((stencilBits - wanted.stencilBits) >> 3);
            }
        }
        else
        {
            // If the user requests _no_ stencil buffer, then it is a minor penalty to have
            // one, based on size of it
            if (stencilBits > 0)
            {
                // buffer deeper than we want - penalty point for each extra 8 bits
                penalty += MINOR_PENALTY * (stencilBits >> 3);
            }
        }

        // MSAA cannot be a complete requirement, so we never filter out.  But asking
        // for it should net _some_ # of samples
        if (wanted.msaaSamples > 1)
        {
            if (msaaSamples <= 1)
            {
                // We wanted MSAA, we got none...
                penalty += MAJOR_PENALTY;
            }
            else if (msaaSamples < wanted.msaaSamples)
            {
                // fewer samples than we want; 2 minor penalties
                penalty += 2*MINOR_PENALTY;
            }
            else if (msaaSamples > wanted.msaaSamples)
            {
                // more samples than we want; minor penalty, unless we want 0 then it's major.
                if (wanted.msaaSamples==0)
                    penalty += MAJOR_PENALTY;
                else
                    penalty += MINOR_PENALTY;
            }
        }
        else // asked for no MSAA
        {
            if (msaaSamples)
            {
                // make bigger penalty as we didn't want msaa
                penalty += 5*MINOR_PENALTY * msaaSamples;
            }
        }

        // Color is handled as one item, so as not to overwhelm, except for
        // destination alpha, which has its own penalty
        const int requestedRGB = wanted.redBits + wanted.greenBits + wanted.blueBits;
        const int RGB = redBits + greenBits + blueBits;
        if (requestedRGB > RGB)
        {
            // major penalty for having fewer bits than requested
            penalty += MAJOR_PENALTY;
        }
        else if (requestedRGB < RGB)
        {
            // minor penalty for having more bits than requested, scaled by how much more
            penalty += MINOR_PENALTY * (RGB - requestedRGB);
        }

        // Now handle alpha, as this is an important "feature" if requested
        if (wanted.alphaBits > alphaBits)
        {
            // major penalty for having fewer bits than requested
            penalty += MAJOR_PENALTY;
        }
        else if (wanted.alphaBits < alphaBits)
        {
            // minor penalty for having more bits than requested
            penalty += MINOR_PENALTY * (alphaBits - wanted.alphaBits);
        }

        CXR_LOGV("Config[%d]: R%dG%dB%dA%d D%dS%d MSAA=%d  Type=%04x Render=%04x (penalties: %d)",
             i, redBits, greenBits, blueBits, alphaBits, depthBits, stencilBits, msaaSamples, surfaceType, renderableFlags, penalty);

        if ((penalty < bestMatch) || (bestIndex == -1))
        {
            bestMatch = penalty;
            bestIndex = i;
            CXR_LOGV("Config[%d] is the new best config", i);
        }
    }

    if (bestIndex < 0)
    {
        delete[] configs;
        return false;
    }

    bestConfig = configs[bestIndex];
    delete[] configs;
    return true;
}

//-----------------------------------------------------------------------------
EGLHelper::Handle EGLHelper::PushFence()
{
    return reinterpret_cast<Handle>(
            eglCreateSyncKHR((EGLDisplay)mDisplay, EGL_SYNC_FENCE_KHR, nullptr));
}

//-----------------------------------------------------------------------------
void EGLHelper::WaitFence(Handle fence, bool onClient)
{
    const auto eglFence = reinterpret_cast<EGLSyncKHR>(fence);
    const auto eglDisplay = reinterpret_cast<EGLDisplay>(mDisplay);

    if (onClient)
    {
        eglClientWaitSyncKHR(eglDisplay, eglFence,
                             EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER_KHR);
    }
    else
    {
        eglWaitSyncKHR(eglDisplay, eglFence, 0);
    }
}

//-----------------------------------------------------------------------------
void EGLHelper::ReleaseFence(Handle fence)
{
    eglDestroySyncKHR((EGLDisplay)mDisplay, reinterpret_cast<EGLSyncKHR>(fence));
}
