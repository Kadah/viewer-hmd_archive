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
#include "Util/Util_MagCalibration.h"

class LLHMDImpl : public OVR::MessageHandler
{
public:
    static const S32 kDefaultHResolution = 1280;
    static const S32 kDefaultVResolution = 800;
    static const F32 kDefaultHScreenSize;
    static const F32 kDefaultVScreenSize;
    static const F32 kDefaultInterpupillaryOffset;
    static const F32 kDefaultLenSeparationDistance;
    static const F32 kDefaultEyeToScreenDistance;
    static const F32 kDefaultDistortionConstant0;
    static const F32 kDefaultDistortionConstant1;
    static const F32 kDefaultDistortionConstant2;
    static const F32 kDefaultDistortionConstant3;
    static const F32 kDefaultXCenterOffset;
    static const F32 kDefaultYCenterOffset;
    static const F32 kDefaultDistortionScale;
    static const F32 kDefaultOrthoPixelOffset;
    static const F32 kDefaultVerticalFOVRadians;
    static const F32 kDefaultAspect;
    static const F32 kDefaultAspectMult;

public:
    LLHMDImpl();
    ~LLHMDImpl();

    BOOL init();
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
    S32 getHMDWidth() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().HResolution : kDefaultHResolution; }
    S32 getHMDEyeWidth() const { return gHMD.isInitialized() ? mCurrentEyeParams.VP.w : (kDefaultHResolution / 2); }
    S32 getHMDHeight() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().VResolution : kDefaultVResolution; }
    S32 getHMDUIWidth() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().HResolution : kDefaultHResolution; }
    S32 getHMDUIHeight() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().VResolution : kDefaultVResolution; }
    F32 getPhysicalScreenWidth() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().HScreenSize : kDefaultHScreenSize; }
    F32 getPhysicalScreenHeight() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().VScreenSize : kDefaultVScreenSize; }
    F32 getInterpupillaryOffset() const { return gHMD.isInitialized() ? mStereoConfig.GetIPD() : getInterpupillaryOffsetDefault(); }
    F32 getInterpupillaryOffsetDefault() const { return kDefaultInterpupillaryOffset; }
    void setInterpupillaryOffset(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetIPD(f); } }
    F32 getLensSeparationDistance() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().LensSeparationDistance : kDefaultLenSeparationDistance; }
    F32 getEyeToScreenDistance() const { return gHMD.isInitialized() ? mStereoConfig.GetEyeToScreenDistance() : getEyeToScreenDistanceDefault(); }
    F32 getEyeToScreenDistanceDefault() const { return kDefaultEyeToScreenDistance; }
    void setEyeToScreenDistance(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetEyeToScreenDistance(f); } }
    F32 getVerticalFOV() { return gHMD.isInitialized() ? mStereoConfig.GetYFOVRadians() : kDefaultVerticalFOVRadians; }
    F32 getAspect() { return gHMD.isInitialized() ? mStereoConfig.GetAspect() : kDefaultAspect; }
    F32 getAspectMultiplier() { return gHMD.isInitialized() ? mStereoConfig.GetAspectMultiplier() : kDefaultAspectMult; }
    void setAspectMultiplier(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetAspectMultiplier(f); } }

    LLVector4 getDistortionConstants() const;
    F32 getXCenterOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.pDistortion->XCenterOffset : kDefaultXCenterOffset; }
    F32 getYCenterOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.pDistortion->YCenterOffset : kDefaultYCenterOffset; }
    F32 getDistortionScale() const { return gHMD.isInitialized() ? mCurrentEyeParams.pDistortion->Scale : kDefaultDistortionScale; }

    BOOL useMotionPrediction() { return gHMD.isInitialized() ? mSensorFusion.IsPredictionEnabled() : useMotionPredictionDefault(); }
    BOOL useMotionPredictionDefault() const { return TRUE; }
    void useMotionPrediction(BOOL b) { if (gHMD.isInitialized()) { mSensorFusion.SetPredictionEnabled(b); } }
    F32 getMotionPredictionDelta() { return gHMD.isInitialized() ? mSensorFusion.GetPredictionDelta() : getMotionPredictionDeltaDefault(); }
    F32 getMotionPredictionDeltaDefault() const { return 0.03f; }
    void setMotionPredictionDelta(F32 f) { if (gHMD.isInitialized()) { mSensorFusion.SetPrediction(f); } }

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

    LLQuaternion getHeadRotationCorrection() const
    {
        return mHeadRotationCorrection;
    }

    void addHeadRotationCorrection(LLQuaternion quat)
    {
        mHeadRotationCorrection *= quat;
    }

    //LLVector4 getChromaticAberrationConstants() const
    //{
    //    return LLVector4(   mCurrentEyeParams.pDistortion->ChromaticAberration[0],
    //                        mCurrentEyeParams.pDistortion->ChromaticAberration[1],
    //                        mCurrentEyeParams.pDistortion->ChromaticAberration[2],
    //                        mCurrentEyeParams.pDistortion->ChromaticAberration[3]);
    //}

    F32 getOrthoPixelOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.OrthoProjection.M[0][3] : (kDefaultOrthoPixelOffset * (mCurrentEye == (U32)OVR::Util::Render::StereoEye_Left ? 1.0f : -1.0f)); }

    BOOL isManuallyCalibrating() const { return gHMD.isInitialized() ? mMagCal.IsManuallyCalibrating() : FALSE; }
    const std::string& getCalibrationText() const { return mCalibrationText; }

    // OVR::Application overrides
    //virtual int  OnStartup(int argc, const char** argv);
    //virtual void OnIdle();

    //virtual void OnMouseMove(int x, int y, int modifiers);
    //virtual void OnKey(KeyCode key, int chr, bool down, int modifiers);
    //virtual void OnGamepad(const GamepadState& pad);
    //virtual void OnResize(int width, int height);

    // OVR::MessageHandler overrides
    virtual void OnMessage(const OVR::Message& msg);

    void BeginAutoCalibration()
    {
        mSensorFusion.Reset();
        mMagCal.BeginAutoCalibration(mSensorFusion);
    }

    void BeginManualCalibration()
    {
        mMagCal.ClearCalibration(mSensorFusion);
        mSensorFusion.Reset();
        mMagCal.BeginManualCalibration(mSensorFusion);
    }


protected:
    void updateManualMagCalibration();

private:
    OVR::Util::MagCalibration mMagCal;
    OVR::Util::LatencyTest mLatencyUtil;
    OVR::Ptr <OVR::DeviceManager> mDeviceManager;
    OVR::Ptr <OVR::HMDDevice> mHMD;
    OVR::SensorFusion mSensorFusion;
    OVR::Ptr <OVR::SensorDevice> mSensorDevice;
    OVR::Util::Render::StereoConfig mStereoConfig;
    LLQuaternion mHeadRotationCorrection;

    struct DeviceStatusNotificationDesc
    {
        OVR::DeviceHandle Handle;
        OVR::MessageType Action;

        DeviceStatusNotificationDesc():Action (OVR::Message_None) {}
        DeviceStatusNotificationDesc(OVR::MessageType mt, const OVR::DeviceHandle& dev) : Handle (dev), Action (mt) {}
    };
    OVR::Array <DeviceStatusNotificationDesc> DeviceStatusNotificationsQueue;

    OVR::Ptr<OVR::LatencyTestDevice> mpLatencyTester;
    OVR::Util::Render::StereoEyeParams mCurrentEyeParams;
    llutf16string mDisplayName;     // Identity of the Oculus on Windows
    long mDisplayId;                // Identity of the Oculus on Mac
    F32 mEyePitch;
    F32 mEyeRoll;
    F32 mEyeYaw;
    U32 mCurrentEye;
    S32 mLastCalibrationStep;
    std::string mCalibrationText;
};

#else // HMD_SUPPORTED

// dummmy class to satisfy API requirements on platforms which we don't support HMD on
class LLHMDImpl
{
public:
    static const S32 kDefaultHResolution = 1280;
    static const S32 kDefaultVResolution = 800;

public:
    LLHMDImpl() : mCalibrationText("") {}
    ~LLHMDImpl() {}

public:
    BOOL init() { return preInit(); }
    BOOL preInit() { return FALSE; }
    void shutdown() {}
    void onIdle() {}
    U32 getCurrentEye() const { return LLViewerCamera::CENTER_EYE; }
    void setCurrentEye(U32 eye) {}
    void getViewportInfo(S32& x, S32& y, S32& w, S32& h) { x = y = w = h = 0; }
    S32 getHMDWidth() const { return 0; }
    S32 getHMDEyeWidth() const { return 0; }
    S32 getHMDHeight() const { return 0; }
    S32 getHMDUIWidth() const { return 0; }
    S32 getHMDUIHeight() const { return 0; }
    F32 getPhysicalScreenWidth() const { return 0.0f; }
    F32 getPhysicalScreenHeight() const { return 0.0f; }
    F32 getInterpupillaryOffset() const { return 0.0f; }
    F32 getInterpupillaryOffsetDefault() const { return 0.0f; }
    void setInterpupillaryOffset(F32 f) {}
    F32 getLensSeparationDistance() const { return 0.0f; }
    F32 getEyeToScreenDistance() const { return 0.0f; }
    F32 getEyeToScreenDistanceDefault() const { return 0.0f; }
    void setEyeToScreenDistance(F32 f) {}
    F32 getVerticalFOV() { return 0.0f; }
    F32 getAspect() { return 0.0f; }
    F32 getAspectMultiplier() { return 0.0f; }
    void setAspectMultiplier(F32 f) {}

    LLVector4 getDistortionConstants() const { return LLVector4::zero; }

    F32 getXCenterOffset() const { return 0.0f; }
    F32 getYCenterOffset() const { return 0.0f; }
    F32 getDistortionScale() const { return 0.0f; }

    BOOL useMotionPrediction() { return FALSE; }
    BOOL useMotionPredictionDefault() const { return FALSE; }
    void useMotionPrediction(BOOL b) {}
    F32 getMotionPredictionDelta() { return 0.0f; }
    F32 getMotionPredictionDeltaDefault() const { return 0.03f; }
    void setMotionPredictionDelta(F32 f) {}

    LLQuaternion getHMDOrient() const { return LLQuaternion::DEFAULT; }

    F32 getRoll() const { return 0.0f; }
    F32 getPitch() const { return 0.0f; }
    F32 getYaw() const { return 0.0f; }
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = pitch = yaw = 0.0f; }

    LLQuaternion getHeadRotationCorrection() const { return LLQuaternion::DEFAULT; }
    void addHeadRotationCorrection(LLQuaternion quat) {}

    F32 getOrthoPixelOffset() const { return 0.0f; }

    BOOL isManuallyCalibrating() const { return FALSE; }
    const std::string& getCalibrationText() const { return mCalibrationText; }

    void BeginManualCalibration() {}

private:
    std::string mCalibrationText;
};

#endif // LL_HMD_SUPPORTED
#endif // LL_LLHMDIMPL_OCULUS_H
