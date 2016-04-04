/** 
* @file   llhmd.h
* @brief  Header file for llhmd
* @author voidpointer@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/
#ifndef LL_LLHMDIMPL_OCULUS_H
#define LL_LLHMDIMPL_OCULUS_H

#include "llhmd.h"

#if LL_HMD_SUPPORTED
#include "llpointer.h"
#include "v4math.h"
#include "llviewertexture.h"

class LLRenderTarget;
class LLHMDImplOculus : public LLHMDImpl
{
public:
    LLHMDImplOculus();
    ~LLHMDImplOculus();

    virtual BOOL init();

    virtual void destroy();
    virtual void shutdown();

    virtual void resetOrientation();

    virtual S32 getViewportWidth() const;
    virtual S32 getViewportHeight() const;

    virtual F32  getInterpupillaryOffset() const { return mInterpupillaryDistance; }
    virtual void setInterpupillaryOffset(F32 f)  { mInterpupillaryDistance = f;    }

    virtual F32 getEyeToScreenDistance() const { return mEyeToScreenDistance; }

    virtual F32 getVerticalFOV() const;
    virtual F32 getAspect() const;

    virtual const LLQuaternion  getHMDRotation()  const { return mEyeRotation; }
    virtual LLVector3           getHeadPosition() const;
    virtual void                getEyeOffset(int whichEye, LLVector3& offsetOut) const;
    virtual void                getEyeProjection(int whichEye, glh::matrix4f& proj, float zNear, float zFar) const;

    virtual BOOL beginFrame();
    virtual BOOL copyToEyeRenderTarget(int which_eye, LLRenderTarget& source, int mask);
    virtual BOOL bindEyeRenderTarget(int which_eye);
    virtual BOOL flushEyeRenderTarget(int which_eye);
    virtual BOOL releaseEyeRenderTarget(int which_eye);
    virtual BOOL endFrame();
    virtual BOOL postSwap();

    // Release to OS...
    virtual BOOL releaseAllEyeRenderTargets();

    virtual void resetFrameIndex();
    virtual U32  getFrameIndex();    

private:
    virtual void incrementFrameIndex();
    virtual BOOL calculateViewportSettings();

    BOOL initSwapChains();
    BOOL initSwapChain(int eyeIndex);
    void destroySwapChains();

    LLVector3    mHeadPos;
    LLQuaternion mEyeRotation;

    U32 mFrameIndex;
    U32 mTrackingCaps;
    F32 mInterpupillaryDistance;
    F32 mEyeToScreenDistance;
    F32 mVerticalFovRadians;
    F32 mAspect;

    // Targets w/ depth in which to render eye data
    LLRenderTarget* mEyeRenderTarget[2][3];
    
    GLint mSwapTexture[2][3];

    // wraps Oculus FBO so we can copy without screwing up
    // the oh so delicate static var ecosystem of LLRenderTarget
    LLRenderTarget* mMirrorRT;

    struct OculusData;
    OculusData* mOculus;
    BOOL mNsightDebugMode;
};
#endif // LL_HMD_SUPPORTED
#endif // LL_LLHMDIMPL_OCULUS_H
