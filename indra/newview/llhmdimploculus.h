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
#include "Kernel/OVR_Timer.h"

class LLHMDImplOculus : public LLHMDImpl, OVR::MessageHandler
{
public:
    LLHMDImplOculus();
    ~LLHMDImplOculus();

    BOOL preInit();
    BOOL postDetectionInit();
    void handleMessages();
    bool isReady() { return mHMD && mSensorDevice && gHMD.isHMDConnected(); }
    void shutdown();
    void onIdle();
    U32 getCurrentEye() const { return mCurrentEye; }
    void setCurrentEye(U32 eye)
    {
        mCurrentEye = llclamp(eye, (U32)OVR::Util::Render::StereoEye_Center, (U32)OVR::Util::Render::StereoEye_Right);
        mCurrentEyeParams = mStereoConfig.GetEyeRenderParams((OVR::Util::Render::StereoEye)mCurrentEye);
    }

    void getViewportInfo(S32& x, S32& y, S32& w, S32& h)
    {
        x = mCurrentEyeParams.VP.x;
        y = mCurrentEyeParams.VP.y;
        w = mCurrentEyeParams.VP.w;
        h = mCurrentEyeParams.VP.h;
    }
    S32 getHMDWidth() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().HResolution : kDefaultHResolution; }
    S32 getHMDEyeWidth() const { return gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.VP.w : (kDefaultHResolution / 2); }
    S32 getHMDHeight() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().VResolution : kDefaultVResolution; }
    S32 getHMDUIWidth() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().HResolution : kDefaultHResolution; }
    S32 getHMDUIHeight() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().VResolution : kDefaultVResolution; }
    F32 getPhysicalScreenWidth() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().HScreenSize : kDefaultHScreenSize; }
    F32 getPhysicalScreenHeight() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().VScreenSize : kDefaultVScreenSize; }
    F32 getInterpupillaryOffset() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetIPD() : getInterpupillaryOffsetDefault(); }
    void setInterpupillaryOffset(F32 f) { if (gHMD.isPostDetectionInitialized()) { mStereoConfig.SetIPD(f); } }
    F32 getLensSeparationDistance() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetHMDInfo().LensSeparationDistance : kDefaultLenSeparationDistance; }
    F32 getEyeToScreenDistance() const { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetEyeToScreenDistance() : getEyeToScreenDistanceDefault(); }
    void setEyeToScreenDistance(F32 f) { if (gHMD.isPostDetectionInitialized()) { mStereoConfig.SetEyeToScreenDistance(f); } }
    F32 getVerticalFOV() { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetYFOVRadians() : kDefaultVerticalFOVRadians; }
    F32 getAspect() { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetAspect() : kDefaultAspect; }
    F32 getAspectMultiplier() { return gHMD.isPostDetectionInitialized() ? mStereoConfig.GetAspectMultiplier() : kDefaultAspectMult; }
    void setAspectMultiplier(F32 f) { if (gHMD.isPostDetectionInitialized()) { mStereoConfig.SetAspectMultiplier(f); } }

    LLVector4 getDistortionConstants() const;
    F32 getXCenterOffset() const { return gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.pDistortion->XCenterOffset : kDefaultXCenterOffset; }
    F32 getYCenterOffset() const { return gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.pDistortion->YCenterOffset : kDefaultYCenterOffset; }
    F32 getDistortionScale() const { return gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.pDistortion->Scale : kDefaultDistortionScale; }

    BOOL useMotionPrediction() { return gHMD.isPostDetectionInitialized() ? mSensorFusion->IsPredictionEnabled() : useMotionPredictionDefault(); }
    BOOL useMotionPredictionDefault() const { return TRUE; }
    void useMotionPrediction(BOOL b) { if (gHMD.isPostDetectionInitialized()) { mSensorFusion->SetPredictionEnabled(b); } }
    F32 getMotionPredictionDelta() { return gHMD.isPostDetectionInitialized() ? mSensorFusion->GetPredictionDelta() : getMotionPredictionDeltaDefault(); }
    F32 getMotionPredictionDeltaDefault() const { return 0.03f; }
    void setMotionPredictionDelta(F32 f) { if (gHMD.isPostDetectionInitialized()) { mSensorFusion->SetPrediction(f); } }

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
    LLQuaternion getHeadRotationCorrection() const { return mHeadRotationCorrection; }
    void addHeadRotationCorrection(LLQuaternion quat) { mHeadRotationCorrection *= quat; }

    F32 getOrthoPixelOffset() const { return gHMD.isPostDetectionInitialized() ? mCurrentEyeParams.OrthoProjection.M[0][3] : (kDefaultOrthoPixelOffset * (mCurrentEye == (U32)OVR::Util::Render::StereoEye_Left ? 1.0f : -1.0f)); }

    void resetOrientation() { if (gHMD.isPostDetectionInitialized()) { mSensorFusion->Reset(); } }

    // OVR::MessageHandler override
    virtual void OnMessage(const OVR::Message& msg);

private:
    struct DeviceStatusNotificationDesc
    {
        OVR::DeviceHandle Handle;
        OVR::MessageType Action;

        DeviceStatusNotificationDesc():Action (OVR::Message_None) {}
        DeviceStatusNotificationDesc(OVR::MessageType mt, const OVR::DeviceHandle& dev) : Handle (dev), Action (mt) {}
    };

    OVR::Ptr <OVR::DeviceManager> mDeviceManager;
    OVR::Ptr <OVR::HMDDevice> mHMD;
    OVR::SensorFusion* mSensorFusion;
    OVR::Ptr <OVR::SensorDevice> mSensorDevice;
    OVR::Util::Render::StereoConfig mStereoConfig;
    LLQuaternion mHeadRotationCorrection;
    OVR::Array<DeviceStatusNotificationDesc>* mpDeviceStatusNotificationsQueue;

    OVR::Util::LatencyTest mLatencyUtil;
    OVR::Ptr<OVR::LatencyTestDevice> mpLatencyTester;
    OVR::Util::Render::StereoEyeParams mCurrentEyeParams;
    llutf16string mDisplayName;     // Identity of the Oculus on Windows
    long mDisplayId;                // Identity of the Oculus on Mac
    F32 mEyePitch;
    F32 mEyeRoll;
    F32 mEyeYaw;
    U32 mCurrentEye;
};
#endif // LL_HMD_SUPPORTED
#endif // LL_LLHMDIMPL_OCULUS_H
