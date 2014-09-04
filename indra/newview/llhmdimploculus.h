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

#include "OVR.h"
#if LL_DARWIN
    // hack around an SDK warning that becomes an error with our compilation settings
    #define __gl_h_
    #define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#endif
#include "OVR_CAPI_GL.h"

class LLRenderTarget;


class LLHMDImplOculus : public LLHMDImpl //, OVR::MessageHandler
{
public:
    LLHMDImplOculus();
    ~LLHMDImplOculus();

    virtual BOOL preInit();
    virtual BOOL postDetectionInit();
    BOOL initHMDDevice();
    void removeHMDDevice();
    virtual BOOL detectHMDDevice(BOOL force);
    bool isReady() { return mHMD && gHMD.isHMDConnected() && gHMD.isHMDDisplayEnabled(); }
    virtual void shutdown();
    virtual void onIdle();
    virtual U32 getCurrentEye() const { return mCurrentEye; }
    U32 getCurrentOVREye() const { return mCurrentEye == LLHMD::RIGHT_EYE ? 1 : 0; }
    virtual void setCurrentEye(U32 eye) { mCurrentEye = llclamp(eye, (U32)LLHMD::CENTER_EYE, (U32)LLHMD::RIGHT_EYE); }
    virtual void getViewportInfo(S32& x, S32& y, S32& w, S32& h) const;
    virtual void getViewportInfo(S32 vp[4]) const;
    virtual S32 getViewportWidth() const;
    virtual S32 getViewportHeight() const;
    virtual S32 getHMDWidth() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.w : kDefaultHResolution; }
    virtual S32 getHMDEyeWidth() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.w / 2 : (kDefaultHResolution / 2); }
    virtual S32 getHMDHeight() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.h : kDefaultVResolution; }
    virtual S32 getHMDUIWidth() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.w : kDefaultHResolution; }
    virtual S32 getHMDUIHeight() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.h : kDefaultVResolution; }
    virtual F32 getInterpupillaryOffset() const { return gHMD.isPostDetectionInitialized() ? mInterpupillaryDistance : getInterpupillaryOffsetDefault(); }
    virtual void setInterpupillaryOffset(F32 f) { if (gHMD.isPostDetectionInitialized()) { mInterpupillaryDistance = f; } }
    virtual F32 getEyeToScreenDistance() const { return gHMD.isPostDetectionInitialized() ? mEyeToScreenDistance : getEyeToScreenDistanceDefault(); }
    virtual F32 getVerticalFOV() { return gHMD.isPostDetectionInitialized() ? mFOVRadians.h : kDefaultVerticalFOVRadians; }
    virtual F32 getAspect() const;

    virtual BOOL useMotionPrediction() { return useMotionPredictionDefault(); }
    virtual BOOL useMotionPredictionDefault() const { return TRUE; }
    virtual void useMotionPrediction(BOOL b) {}

    virtual F32 getRoll() const { return gHMD.isPostDetectionInitialized() ? mEyeRPY[LLHMD::ROLL] : 0.0f; }
    virtual F32 getPitch() const { return gHMD.isPostDetectionInitialized() ? mEyeRPY[LLHMD::PITCH] : 0.0f; }
    virtual F32 getYaw() const { return gHMD.isPostDetectionInitialized() ? mEyeRPY[LLHMD::YAW] : 0.0f; }
    virtual void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const;
    virtual LLQuaternion getHMDRotation() const { return mEyeRotation; }

    virtual void resetOrientation();

    virtual F32 getOrthoPixelOffset() const { return gHMD.isPostDetectionInitialized() ? mOrthoPixelOffset[mCurrentEye] : (kDefaultOrthoPixelOffset * (mCurrentEye == (U32)LLHMD::LEFT_EYE ? 1.0f : -1.0f)); }

    // DK2
    virtual BOOL beginFrame();
    virtual BOOL endFrame();
    virtual void getCurrentEyeProjectionOffset(F32 p[4][4]) const;
    virtual LLVector3 getStereoCullCameraForwards() const;
    virtual LLVector3 getCurrentEyeCameraOffset() const;
    virtual LLVector3 getCurrentEyeOffset(const LLVector3& centerPos) const;
    virtual LLVector3 getHeadPosition() const;

    virtual LLRenderTarget* getCurrentEyeRT();
    virtual LLRenderTarget* getEyeRT(U32 eye);
    virtual void onViewChange(S32 oldMode);
    virtual void showHSW(BOOL show);

private:
    BOOL calculateViewportSettings();
    void applyDynamicResolutionScaling(double curTime);

private:
    ovrHmd mHMD;
    ovrFrameTiming mFrameTiming;
    ovrTrackingState mTrackingState;
    OVR::Sizei mEyeRenderSize[ovrEye_Count];
    ovrGLTexture mEyeTexture[ovrEye_Count];
    ovrEyeRenderDesc mEyeRenderDesc[ovrEye_Count];
    ovrPosef mEyeRenderPose[ovrEye_Count];
    U32 mTrackingCaps;
    // Note: OVR matrices are RH, row-major with OGL axes (-Z forward, Y up, X Right)
    OVR::Matrix4f mProjection[ovrEye_Count];
    //OVR::Matrix4f mOrthoProjection[ovrEye_Count];      // TODO: needed?
    //OVR::Matrix4f mConvOculusToLL;  // convert from OGL to LL (RH, Row-Major, X Forward, Z Up, Y Left)
    //OVR::Matrix4f mConvLLToOculus;  // convert from LL to OGL
    //F32 mFPS;               // TODO: needed?
    //F32 mSecondsPerFrame;   // TODO: needed?
    //S32 mFrameCounter;      // TODO: needed?
    //S32 mTotalFrameCounter; // TODO: needed?
    //double mLastFpsUpdate;
    double mLastTimewarpUpdate;
    F32 mInterpupillaryDistance;
    F32 mEyeToScreenDistance;
    OVR::Sizef mFOVRadians;
    F32 mAspect;
    F32 mOrthoPixelOffset[3];
    S32 mCurrentHMDCount;
    LLRenderTarget* mEyeRT[3];
    U32 mCurrentEye;
    LLVector3 mEyeRPY;
    LLVector3 mHeadPos;
    LLQuaternion mEyeRotation;
};
#endif // LL_HMD_SUPPORTED
#endif // LL_LLHMDIMPL_OCULUS_H
