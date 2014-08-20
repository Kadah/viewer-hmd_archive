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
#include "OVR_Stereo.h"
#include "Util/Util_Render_Stereo.h"
//#if defined(OVR_OS_WIN32)
//#include "win32_pl
//#elif defined(OVR_OS_MAC) && !defined(OVR_MAC_X11)
//#else
//#endif
//#include "Kernel/OVR_Timer.h"

class LLHMDImplOculus : public LLHMDImpl //, OVR::MessageHandler
{
public:
    LLHMDImplOculus();
    ~LLHMDImplOculus();

    BOOL preInit();
    BOOL postDetectionInit();
    void initHMDDevice();
    //void initHMDSensor();
    //void initHMDLatencyTester();
    //void handleMessages();
    //bool isReady() { return mHMD && mSensorDevice && gHMD.isHMDConnected() && gHMD.isHMDSensorConnected(); }
    bool isReady() { return mHMD && gHMD.isHMDConnected() && gHMD.isHMDSensorConnected() && gHMD.isHMDDisplayEnabled(); }
    void shutdown();
    void onIdle();
    U32 getCurrentEye() const { return mCurrentEye; }
    U32 getCurrentOVREye() const { return mCurrentEye == LLHMD::RIGHT_EYE ? 1 : 0; }
    void setCurrentEye(U32 eye)
    {
        mCurrentEye = llclamp(eye, (U32)OVR::StereoEye_Center, (U32)OVR::StereoEye_Right);
        //mCurrentEyeParams = mStereoConfig.GetEyeRenderParams((OVR::StereoEye)mCurrentEye);
    }
    virtual void getViewportInfo(S32& x, S32& y, S32& w, S32& h);
    virtual void getViewportInfo(S32 vp[4]);
    S32 getHMDWidth() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.w : kDefaultHResolution; }
    S32 getHMDEyeWidth() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.w / 2.0f : (kDefaultHResolution / 2); }
    S32 getHMDHeight() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.h : kDefaultVResolution; }
    S32 getHMDUIWidth() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.w : kDefaultHResolution; }
    S32 getHMDUIHeight() const { return gHMD.isPostDetectionInitialized() ? mHMD->Resolution.h : kDefaultVResolution; }
    F32 getPhysicalScreenWidth() const { return gHMD.isPostDetectionInitialized() ? mScreenSizeInMeters.w : kDefaultHScreenSize; }
    F32 getPhysicalScreenHeight() const { return gHMD.isPostDetectionInitialized() ? mScreenSizeInMeters.h : kDefaultVScreenSize; }
    F32 getInterpupillaryOffset() const { return gHMD.isPostDetectionInitialized() ? mInterpupillaryDistance : getInterpupillaryOffsetDefault(); }
    void setInterpupillaryOffset(F32 f) { if (gHMD.isPostDetectionInitialized()) { mInterpupillaryDistance = f; } }
    F32 getLensSeparationDistance() const { return gHMD.isPostDetectionInitialized() ? mLensSeparationInMeters : kDefaultLenSeparationDistance; }
    F32 getEyeToScreenDistance() const { return gHMD.isPostDetectionInitialized() ? mEyeToScreenDistance : getEyeToScreenDistanceDefault(); }
    void setEyeToScreenDistance(F32 f) { if (gHMD.isPostDetectionInitialized()) { mEyeToScreenDistance = f; } }
    F32 getVerticalFOV() { return gHMD.isPostDetectionInitialized() ? mFOVRadians.h : kDefaultVerticalFOVRadians; }
    F32 getAspect() { return gHMD.isPostDetectionInitialized() ? mAspect : kDefaultAspect; }

    LLVector4 getDistortionConstants() const;
    F32 getXCenterOffset() const { return /* gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.pDistortion->XCenterOffset : */ kDefaultXCenterOffset; }
    F32 getYCenterOffset() const { return /* gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.pDistortion->YCenterOffset : */ kDefaultYCenterOffset; }
    F32 getDistortionScale() const { return /* gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.pDistortion->Scale : */ kDefaultDistortionScale; }

    BOOL useMotionPrediction() { return useMotionPredictionDefault(); } // gHMD.isPostDetectionInitialized() ? mSensorFusion->IsPredictionEnabled() : useMotionPredictionDefault(); }
    BOOL useMotionPredictionDefault() const { return TRUE; }
    void useMotionPrediction(BOOL b) {} //  if (gHMD.isPostDetectionInitialized()) { mSensorFusion->SetPredictionEnabled(b); } }
    F32 getMotionPredictionDelta() { return getMotionPredictionDeltaDefault(); } // gHMD.isPostDetectionInitialized() ? mSensorFusion->GetPredictionDelta() : getMotionPredictionDeltaDefault(); }
    F32 getMotionPredictionDeltaDefault() const { return 0.03f; }
    void setMotionPredictionDelta(F32 f) {} //  if (gHMD.isPostDetectionInitialized()) { mSensorFusion->SetPrediction(f); } }

    LLQuaternion getHMDOrient() const
    {
        LLQuaternion q;
        q.setEulerAngles(mEyeRoll, mEyePitch, mEyeYaw);
        return q;
    }
    F32 getRoll() const { return mEyeRoll; }
    F32 getPitch() const { return mEyePitch; }
    F32 getYaw() const { return mEyeYaw; }
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = mEyeRoll; pitch = mEyePitch; yaw = mEyeYaw; }
    //virtual LLQuaternion getHeadRotationCorrection() const { return mHeadRotationCorrection; }
    //virtual void addHeadRotationCorrection(LLQuaternion quat) { mHeadRotationCorrection *= quat; mHeadRotationCorrection.normalize(); }
    //virtual void resetHeadRotationCorrection() { mHeadRotationCorrection = LLQuaternion::DEFAULT; }
    //virtual LLQuaternion getHeadPitchCorrection() const { return mHeadPitchCorrection; }
    //virtual void addHeadPitchCorrection(LLQuaternion quat) { mHeadPitchCorrection *= quat; mHeadPitchCorrection.normalize(); }
    //virtual void resetHeadPitchCorrection() { mHeadPitchCorrection = LLQuaternion::DEFAULT; }

    void resetOrientation() { if (gHMD.isPostDetectionInitialized()) { /* mSensorFusion->Reset(); */ } }

    F32 getOrthoPixelOffset() const { return gHMD.isPostDetectionInitialized() ? mOrthoPixelOffset[mCurrentEye] : (kDefaultOrthoPixelOffset * (mCurrentEye == (U32)OVR::StereoEye_Left ? 1.0f : -1.0f)); }

    //const char* getLatencyTesterResults() { return ""; } // if (gHMD.isPostDetectionInitialized() && mLatencyUtil.HasDevice()) { return mLatencyUtil.GetResultsString(); } else { return ""; } }

    // OVR::MessageHandler override
    //virtual void OnMessage(const OVR::Message& msg);

    // DK2
    virtual BOOL beginFrame();
    virtual BOOL endFrame();
    virtual U32 getCurrentEyeTextureWidth();
    virtual U32 getCurrentEyeTextureHeight();

private:
    BOOL calculateViewportSettings();
    void applyDynamicResolutionScaling(double curTime);

private:
    ovrHmd mHMD;
    ovrFrameTiming mFrameTiming;
    ovrTrackingState mTrackingState;
    //double mLastUpdate;
    float mFovSideTanLimit; // TODO: make this local?
    float mFovSideTanMax; // TODO: make this local?
    OVR::Sizei mEyeRenderSize[ovrEye_Count];
    ovrTexture mEyeTexture[ovrEye_Count];
    ovrEyeRenderDesc mEyeRenderDesc[ovrEye_Count];
    ovrPosef mEyeRenderPose[ovrEye_Count];
    U32 mTrackingCaps;
    // Note: OVR matrices are RH, row-major with OGL axes (-Z forward, Y up, X Right)
    OVR::Matrix4f mProjection[ovrEye_Count];
    OVR::Matrix4f mOrthoProjection[ovrEye_Count];      // TODO: needed?
    OVR::Matrix4f mView[ovrEye_Count];
    OVR::Matrix4f mConvOculusToLL;  // convert from OGL to LL (RH, Row-Major, X Forward, Z Up, Y Left)
    OVR::Matrix4f mConvLLToOculus;  // convert from LL to OGL
    F32 mFPS;               // TODO: needed?
    F32 mSecondsPerFrame;   // TODO: needed?
    S32 mFrameCounter;      // TODO: needed?
    S32 mTotalFrameCounter; // TODO: needed?
    double mLastFpsUpdate;
    double mLastTimewarpUpdate;
    OVR::Sizef mScreenSizeInMeters;
    F32 mInterpupillaryDistance;
    F32 mLensSeparationInMeters;
    F32 mEyeToScreenDistance;
    OVR::Sizef mFOVRadians;
    F32 mAspect;
    F32 mOrthoPixelOffset[3];

    //struct DeviceStatusNotificationDesc
    //{
    //    OVR::DeviceHandle Handle;
    //    OVR::MessageType Action;

    //    DeviceStatusNotificationDesc():Action (OVR::Message_None) {}
    //    DeviceStatusNotificationDesc(OVR::MessageType mt, const OVR::DeviceHandle& dev) : Handle (dev), Action (mt) {}
    //};

    //OVR::Ptr <OVR::DeviceManager> mDeviceManager;
    //OVR::Ptr <OVR::HMDDevice> mHMD;
    //OVR::SensorFusion* mSensorFusion;
    //OVR::Ptr <OVR::SensorDevice> mSensorDevice;
    //OVR::Util::Render::StereoConfig mStereoConfig;
    //LLQuaternion mHeadRotationCorrection;
    //LLQuaternion mHeadPitchCorrection;
    //OVR::Array<DeviceStatusNotificationDesc>* mpDeviceStatusNotificationsQueue;

    //OVR::Util::LatencyTest mLatencyUtil;
    //OVR::Ptr<OVR::LatencyTestDevice> mpLatencyTester;
    //OVR::StereoEyeParams mCurrentEyeParams;
    F32 mEyePitch;  // TODO: remove
    F32 mEyeRoll;   // TODO: remove
    F32 mEyeYaw;    // TODO: remove
    U32 mCurrentEye;
    LLVector3 mEyeRPY[ovrEye_Count];
    LLVector3 mEyePos[ovrEye_Count];
};
#endif // LL_HMD_SUPPORTED
#endif // LL_LLHMDIMPL_OCULUS_H
