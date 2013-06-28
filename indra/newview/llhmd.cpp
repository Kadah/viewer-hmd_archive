/** 
* @file llhmd.cpp
* @brief Implementation of llhmd
* @author Lee@lindenlab.com
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


#include "llviewerprecompiledheaders.h"
#include "llhmd.h"

#include "llfloaterreg.h"


#include "llviewerwindow.h"
#include "llagent.h"
#include "llviewercamera.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"

#include "OVR.h"
#include "Kernel/OVR_Timer.h"
#include "Util/Util_MagCalibration.h"

#if LL_WINDOWS
#include "llwindowwin32.h"
#elif LL_DARWIN
#include "llwindowmacosx.h"
#define IDCONTINUE 1        // Exist on Windows along "IDOK" and "IDCANCEL" but not on Mac
#else
// We do not support the Oculus Rift on LL_LINUX for the moment
#error unsupported platform
#endif // LL_WINDOWS

// TODO_VR: add support for non-Windows platforms.  Currently waiting for Oculus SDK to support Linux/Mac

/////////////////////////////////////////////////////////////////////////////////////////////////////
// LLHMDImpl
/////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace OVR;

class LLHMDImpl : public OVR::MessageHandler
{
public:
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

public:
    LLHMDImpl();
    ~LLHMDImpl();

    BOOL init();
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

    F32 getHScreenSize() const { return gHMD.isInitialized() ? mHMDInfo.HScreenSize : kDefaultHScreenSize; }
    F32 getVScreenSize() const { return gHMD.isInitialized() ? mHMDInfo.VScreenSize : kDefaultVScreenSize; }
    F32 getInterpupillaryOffset() const { return gHMD.isInitialized() ? mHMDInfo.InterpupillaryDistance : kDefaultInterpupillaryOffset; }
    F32 getLensSeparationDistance() const { return gHMD.isInitialized() ? mHMDInfo.LensSeparationDistance : kDefaultLenSeparationDistance; }
    F32 getEyeToScreenDistance() const { return gHMD.isInitialized() ? mHMDInfo.EyeToScreenDistance : kDefaultEyeToScreenDistance; }
    void setEyeToScreenDistance(F32 esd) { if (gHMD.isInitialized()) { mHMDInfo.EyeToScreenDistance = esd; } }

    LLVector4 getDistortionConstants() const
    {
        if (gHMD.isInitialized())
        {
            return LLVector4(   mCurrentEyeParams.pDistortion->K[0],
                                mCurrentEyeParams.pDistortion->K[1],
                                mCurrentEyeParams.pDistortion->K[2],
                                mCurrentEyeParams.pDistortion->K[3]);
        }
        else
        {
            return LLVector4(   kDefaultDistortionConstant0,
                                kDefaultDistortionConstant1,
                                kDefaultDistortionConstant2,
                                kDefaultDistortionConstant3);
        }
    }
    F32 getXCenterOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.pDistortion->XCenterOffset : kDefaultXCenterOffset; }
    F32 getYCenterOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.pDistortion->YCenterOffset : kDefaultYCenterOffset; }
    F32 getDistortionScale() const { return gHMD.isInitialized() ? mCurrentEyeParams.pDistortion->Scale : kDefaultDistortionScale; }

    LLQuaternion getHMDOrient() const
    {
        LLQuaternion q;
        q.setEulerAngles(mEyeRoll, mEyePitch, mEyeYaw);
        return q;
    }
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = mEyeRoll; pitch = mEyePitch; yaw = mEyeYaw; }
    const LLVector3& getRawHMDRollPitchYaw() const { return mRawHMDRollPitchYaw; }

    //F32 getFOV() const { return mStereoConfig.GetYFOVDegrees();
    void setFOV(F32 fov) { mStereoConfig.Set2DAreaFov(DegreeToRad(fov)); }

    //LLVector4 getChromaticAberrationConstants() const
    //{
    //    return LLVector4(   mCurrentEyeParams.pDistortion->ChromaticAberration[0],
    //                        mCurrentEyeParams.pDistortion->ChromaticAberration[1],
    //                        mCurrentEyeParams.pDistortion->ChromaticAberration[2],
    //                        mCurrentEyeParams.pDistortion->ChromaticAberration[3]);
    //}
    //LLMatrix4 getViewAdjustMatrix() const { return matrixOVRtoLL(mCurrentEyeParams.ViewAdjust); }
    //LLMatrix4 getProjectionMatrix() const { return matrixOVRtoLL(mCurrentEyeParams.Projection); }
    //LLMatrix4 getOrthoProjectionMatrix() const { return matrixOVRtoLL(mCurrentEyeParams.OrthoProjection); }

    // TODO: this returns an incorrect value.  need more info from SDK though to get correct value, however since the data needed
    // is private and has no accessors currently.  Hoping to not modify SDK, but may be forced to if we need this value.
    F32 getOrthoPixelOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.OrthoProjection.M[0][3] : (kDefaultOrthoPixelOffset * (mCurrentEye == (U32)OVR::Util::Render::StereoEye_Left ? 1.0f : -1.0f)); }

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
        mSFusion.Reset();
        mMagCal.BeginAutoCalibration(mSFusion);
    }

    void BeginManualCalibration()
    {
        mMagCal.BeginManualCalibration(mSFusion);
    }

    static LLMatrix4 matrixOVRtoLL(const OVR::Matrix4f& m2)
    {
        LLMatrix4 m;
        memcpy(m.mMatrix, m2.M, sizeof(F32) * 16); 
        return m;
    }

    static OVR::Matrix4f matrixLLtoOVR(const LLMatrix4& m2)
    {
        OVR::Matrix4f m;
        memcpy(m.M, m2.mMatrix, sizeof(F32) * 16); 
        return m;
    }


protected:
    void updateManualMagCalibration();
    S32 GetDisplayCount();
    BOOL getDisplayInfo(const llutf16string& displayName, LLRect& rcWork, BOOL& isPrimary);

private:
    OVR::Util::MagCalibration mMagCal;
    SensorFusion mSFusion;
    HMDInfo mHMDInfo;
    Util::LatencyTest mLatencyUtil;
    OVR::Util::Render::StereoConfig mStereoConfig;
    OVR::Ptr<DeviceManager> mpDeviceMgr;
    OVR::Ptr<SensorDevice> mpSensor;
    OVR::Ptr<HMDDevice> mpHMD;
    OVR::Ptr<LatencyTestDevice> mpLatencyTester;
    llutf16string mDisplayName;
    LLVector3 mRawHMDRollPitchYaw;
    F32 mEyePitch;
    F32 mEyeRoll;
    F32 mEyeYaw;
    U32 mCurrentEye;
    OVR::Util::Render::StereoEyeParams mCurrentEyeParams;
    LLRect mHMDRect;
    S32 mLastCalibrationStep;
};

const F32 LLHMDImpl::kDefaultHScreenSize = 0.14976f;
const F32 LLHMDImpl::kDefaultVScreenSize = 0.0936f;
const F32 LLHMDImpl::kDefaultInterpupillaryOffset = 0.064f;
const F32 LLHMDImpl::kDefaultLenSeparationDistance = 0.0635f;
const F32 LLHMDImpl::kDefaultEyeToScreenDistance = 0.041f;
const F32 LLHMDImpl::kDefaultDistortionConstant0 = 1.0f;
const F32 LLHMDImpl::kDefaultDistortionConstant1 = 0.22f;
const F32 LLHMDImpl::kDefaultDistortionConstant2 = 0.24f;
const F32 LLHMDImpl::kDefaultDistortionConstant3 = 0.0f;
const F32 LLHMDImpl::kDefaultXCenterOffset = 0.152f;
const F32 LLHMDImpl::kDefaultYCenterOffset = 0.0f;
const F32 LLHMDImpl::kDefaultDistortionScale = 1.7146f;
const F32 LLHMDImpl::kDefaultOrthoPixelOffset = 0.1775f; // -0.1775f Right Eye


LLHMDImpl::LLHMDImpl()
    : mEyePitch(0.0f)
    , mEyeRoll(0.0f)
    , mEyeYaw(0.0f)
    , mCurrentEye(OVR::Util::Render::StereoEye_Center)
    , mLastCalibrationStep(-1)
{
}


LLHMDImpl::~LLHMDImpl()
{
    shutdown();
}


BOOL LLHMDImpl::init()
{
    if (gHMD.isInitialized())
    {
        return TRUE;
    }

    OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));

    if (!mpDeviceMgr)
    {
        mpDeviceMgr = *OVR::DeviceManager::Create();
    }
	mpDeviceMgr->SetMessageHandler(this);

    S32 detectionResult = IDCONTINUE;
    std::string detectionMessage;

    do 
    {
        // Release Sensor/HMD in case this is a retry.
        mpSensor.Clear();
        mpHMD.Clear();
        mDisplayName.clear();
        mpHMD  = *(mpDeviceMgr->EnumerateDevices<OVR::HMDDevice>().CreateDevice());
        if (mpHMD)
        {
            mpSensor = *(mpHMD->GetSensor());

            // This will initialize HMDInfo with information about configured IPD,
            // screen size and other variables needed for correct projection.
            // We pass HMD DisplayDeviceName into the renderer to select the
            // correct monitor in full-screen mode.
            if (mpHMD->GetDeviceInfo(&mHMDInfo))
            {
                mDisplayName = utf8str_to_utf16str(mHMDInfo.DisplayDeviceName);
                //mHMDInfo.EyeToScreenDistance = kDefaultEyeToScreenDistance;
                mStereoConfig.SetHMDInfo(mHMDInfo);
            }
        }
#if LL_DEBUG
        else
        {            
            // If we didn't detect an HMD, try to create the sensor directly.
            // This is useful for debugging sensor interaction; it is not needed in
            // a shipping app.
            mpSensor = *(mpDeviceMgr->EnumerateDevices<SensorDevice>().CreateDevice());
        }
#endif

        // Create the Latency Tester device and assign it to the LatencyTesterUtil object.
        mpLatencyTester = *(mpDeviceMgr->EnumerateDevices<LatencyTestDevice>().CreateDevice());
        if (mpLatencyTester)
        {
            mLatencyUtil.SetDevice(mpLatencyTester);
        }

        // If there was a problem detecting the Rift, display appropriate message.
        detectionResult  = IDCONTINUE;

        detectionMessage = mpHMD ? "Oculus HMD Display detected" : "Oculus HMD Display not detected";
        detectionMessage += mpSensor ? "; Oculus Sensor detected" : "; Oculus Sensor not detected";
        detectionMessage += (mHMDInfo.DisplayDeviceName[0] != '\0') ? "; HMD Display EDID detected" : "; HMD display EDID not detected";
        LL_INFOS("Oculus") << detectionMessage.c_str() << LL_ENDL;

        if (!mpHMD || !mpSensor || mHMDInfo.DisplayDeviceName[0] == '\0')
        {
            // TODO: possibly put up a retry/cancel dialog here?
            LL_WARNS("Oculus") << detectionMessage << LL_ENDL;
            gHMD.failedInit(TRUE);
            return FALSE;
        }
    } while (detectionResult != IDCONTINUE);

    if (mHMDInfo.HResolution == 0 || mHMDInfo.VResolution == 0)
    {
        gHMD.failedInit(TRUE);
        return FALSE;
    }
    
    if (mpSensor)
    {
        // We need to attach sensor to SensorFusion object for it to receive 
        // body frame messages and update orientation. mSFusion.GetOrientation() 
        // is used in OnIdle() to orient the view.
        mSFusion.AttachToSensor(mpSensor);
        mSFusion.SetDelegateMessageHandler(this);
        mSFusion.SetPredictionEnabled(true);
    }

    // get device's "monitor" info
    BOOL dummy;
    getDisplayInfo(mDisplayName, mHMDRect, dummy);
    BOOL mainFullScreen = FALSE;
    LLWindow* pWin = gViewerWindow->getWindow();
    pWin->getRenderWindow(mainFullScreen);
    gHMD.isMainFullScreen(mainFullScreen);
    if (!pWin->initHMDWindow(mHMDRect.mLeft, mHMDRect.mTop, mHMDRect.mRight - mHMDRect.mLeft, mHMDRect.mBottom - mHMDRect.mTop))
    {
        gHMD.failedInit(TRUE);
        return FALSE;
    }

    // *** Configure Stereo settings.
    mStereoConfig.SetFullViewport(OVR::Util::Render::Viewport(0,0, mHMDInfo.HResolution, mHMDInfo.VResolution));
    mStereoConfig.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);

    // Configure proper Distortion Fit.
    // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
    // screen on the top (saves on rendering cost).
    // For smaller screens (5.5"), fit to the top.
    if (mHMDInfo.HScreenSize > 0.0f)
    {
        if (mHMDInfo.HScreenSize > 0.140f) // 7"
        {
            mStereoConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
        }
        else
        {
            mStereoConfig.SetDistortionFitPointVP(0.0f, 1.0f);
        }
    }

    gHMD.isInitialized(TRUE);
    gHMD.failedInit(FALSE);
    gHMD.isCalibrated(FALSE);

    setCurrentEye(OVR::Util::Render::StereoEye_Center);

    return TRUE;
}


void LLHMDImpl::shutdown()
{
    if (!gHMD.isInitialized())
    {
        return;
    }
    gHMD.isInitialized(FALSE);
    gViewerWindow->getWindow()->destroyHMDWindow();
    RemoveHandlerFromDevices();
    mpSensor.Clear();
    mpHMD.Clear();

    // This causes a deadlock.  No idea why.   Disabling it as it doesn't seem to be necessary unless we're actually RE-initializing the HMD
    // without shutting down.   For now, to init HMD, we have to restart the viewer.
    //OVR::System::Destroy();
}


void LLHMDImpl::onIdle()
{
	static LLCachedControl<bool> debug_hmd(gSavedSettings, "DebugHMDEnable");
    if (!gHMD.isInitialized() || (!debug_hmd && !gHMD.shouldRender()))
    {
        return;
    }

    // Process latency tester results.
    const char* results = mLatencyUtil.GetResultsString();
    if (results != NULL)
    {
        LL_DEBUGS("Oculus") << "LATENCY TESTER:" << results << LL_ENDL;
    }

    // Have to place this as close as possible to where the HMD orientation is read.
    mLatencyUtil.ProcessInputs();

    if (!gHMD.isCalibrated())
    {
        // Magnetometer calibration procedure
        if (mMagCal.IsManuallyCalibrating())
        {
            updateManualMagCalibration();
        }
        else if (mMagCal.IsAutoCalibrating()) 
        {
            mMagCal.UpdateAutoCalibration(mSFusion);
            //if (mMagCal.IsCalibrated())
            //{
            //    Vector3f mc = MagCal.GetMagCenter();
            //    SetAdjustMessage("   Magnetometer Calibration Complete   \nCenter: %f %f %f",mc.x,mc.y,mc.z);
            //}
            //SetAdjustMessage("Mag has been successfully calibrated");
        }
    }

    // Handle Sensor motion.
    // We extract Yaw, Pitch, Roll instead of directly using the orientation
    // to allow "additional" yaw manipulation with mouse/controller.
    if (mpSensor)
    {
        //OVR::Quatf orient = mSFusion.GetOrientation();
        OVR::Quatf orient = mSFusion.GetPredictedOrientation();
        orient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&mEyeYaw, &mEyePitch, &mEyeRoll);
        mEyeRoll = -mEyeRoll;
        mEyePitch = -mEyePitch;
    }    
}


void LLHMDImpl::updateManualMagCalibration()
{
    F32 yaw, pitch, roll;
    OVR::Quatf hmdOrient = mSFusion.GetOrientation();
    hmdOrient.GetEulerAngles<Axis_X, Axis_Z, Axis_Y>(&pitch, &roll, &yaw);
    OVR::Vector3f mag = mSFusion.GetMagnetometer();
    float dtr = OVR::Math<float>::DegreeToRadFactor;

    switch(mMagCal.NumberOfSamples())
    {
    case 0:
        if (mLastCalibrationStep != 0)
        {
            LL_INFOS("Oculus") << "Magnetometer Calibration\n** Step 1: Please Look Forward **" << LL_ENDL;
            mLastCalibrationStep = 0;
        }
        //SetAdjustMessage("Magnetometer Calibration\n** Step 1: Please Look Forward **");
        if ((fabs(yaw) < 10.0f * dtr) && (fabs(pitch) < 10.0f * dtr))
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 1:
        if (mLastCalibrationStep != 1)
        {
            LL_INFOS("Oculus") << "Magnetometer Calibration\n** Step2: Please Look Up **" << LL_ENDL;
            mLastCalibrationStep = 1;
        }
        //SetAdjustMessage("Magnetometer Calibration\n** Step 2: Please Look Up **");
        if (pitch > 30.0f * dtr)
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 2:
        if (mLastCalibrationStep != 2)
        {
            LL_INFOS("Oculus") << "Magnetometer Calibration\n** Step3: Please Look Left **" << LL_ENDL;
            mLastCalibrationStep = 2;
        }
        //SetAdjustMessage("Magnetometer Calibration\n** Step 3: Please Look Left **");
        if (yaw > 30.0f * dtr)
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 3:
        if (mLastCalibrationStep != 3)
        {
            LL_INFOS("Oculus") << "Magnetometer Calibration\n** Step4: Please Look Right **" << LL_ENDL;
            mLastCalibrationStep = 3;
        }
        //SetAdjustMessage("Magnetometer Calibration\n** Step 4: Please Look Right **");
        if (yaw < -30.0f * dtr)
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 4:
        if (!mMagCal.IsCalibrated()) 
        {
            mMagCal.SetCalibration(mSFusion);
            Vector3f mc = mMagCal.GetMagCenter();
            LL_INFOS("Oculus") << "Magnetometer Calibration Complete   \nCenter: " << mc.x << "," << mc.y << "," << mc.z << LL_ENDL;
            mLastCalibrationStep = 4;
            gHMD.isCalibrated(TRUE);
            //SetAdjustMessage("   Magnetometer Calibration Complete   \nCenter: %f %f %f",mc.x,mc.y,mc.z);

            setCurrentEye(OVR::Util::Render::StereoEye_Left);
            LL_INFOS("Oculus") << "Left Eye:" << LL_ENDL;
            LL_INFOS("Oculus") << "HScreenSize: " << std::fixed << std::setprecision(4) << getHScreenSize() << ", default: " << LLHMDImpl::kDefaultHScreenSize << LL_ENDL;
            LL_INFOS("Oculus") << "VScreenSize: " << std::fixed << std::setprecision(4) << getVScreenSize() << ", default: " << LLHMDImpl::kDefaultVScreenSize << LL_ENDL;
            LL_INFOS("Oculus") << "InterpupillaryOffset: " << std::fixed << std::setprecision(4) << getInterpupillaryOffset() << ", default: " << LLHMDImpl::kDefaultInterpupillaryOffset << LL_ENDL;
            LL_INFOS("Oculus") << "LensSeparationDistance: " << std::fixed << std::setprecision(4) << getLensSeparationDistance() << ", default: " << LLHMDImpl::kDefaultLenSeparationDistance << LL_ENDL;
            LL_INFOS("Oculus") << "EyeToScreenDistance: " << std::fixed << std::setprecision(4) << getEyeToScreenDistance() << ", default: " << LLHMDImpl::kDefaultEyeToScreenDistance << LL_ENDL;
            LLVector4 dc = getDistortionConstants();
            LL_INFOS("Oculus") << "Distortion Constants: [" << std::fixed << std::setprecision(4) << dc.mV[0] << "," << dc.mV[1] << "," << dc.mV[2] << "," << dc.mV[3] << "]" << LL_ENDL;
            LL_INFOS("Oculus") << "Default Dist Cnstnts: [" << std::fixed << std::setprecision(4) << LLHMDImpl::kDefaultDistortionConstant0 << "," << LLHMDImpl::kDefaultDistortionConstant1 << "," << LLHMDImpl::kDefaultDistortionConstant2 << "," << LLHMDImpl::kDefaultDistortionConstant3 << "]" << LL_ENDL;
            LL_INFOS("Oculus") << "XCenterOffset: " << std::fixed << std::setprecision(4) << getXCenterOffset() << ", default: " << LLHMDImpl::kDefaultXCenterOffset << LL_ENDL;
            LL_INFOS("Oculus") << "YCenterOffset: " << std::fixed << std::setprecision(4) << getYCenterOffset() << ", default: " << LLHMDImpl::kDefaultYCenterOffset << LL_ENDL;
            LL_INFOS("Oculus") << "DistortionScale: " << std::fixed << std::setprecision(4) << getDistortionScale() << ", default: " << LLHMDImpl::kDefaultDistortionScale << LL_ENDL;
            LL_INFOS("Oculus") << "OrthoPixelOffset: " << std::fixed << std::setprecision(4) << getOrthoPixelOffset() << ", default: " << LLHMDImpl::kDefaultOrthoPixelOffset << LL_ENDL;

            setCurrentEye(OVR::Util::Render::StereoEye_Right);
            LL_INFOS("Oculus") << "Right Eye:" << LL_ENDL;
            LL_INFOS("Oculus") << "HScreenSize: " << std::fixed << std::setprecision(4) << getHScreenSize() << ", default: " << LLHMDImpl::kDefaultHScreenSize << LL_ENDL;
            LL_INFOS("Oculus") << "VScreenSize: " << std::fixed << std::setprecision(4) << getVScreenSize() << ", default: " << LLHMDImpl::kDefaultVScreenSize << LL_ENDL;
            LL_INFOS("Oculus") << "InterpupillaryOffset: " << std::fixed << std::setprecision(4) << getInterpupillaryOffset() << ", default: " << LLHMDImpl::kDefaultInterpupillaryOffset << LL_ENDL;
            LL_INFOS("Oculus") << "LensSeparationDistance: " << std::fixed << std::setprecision(4) << getLensSeparationDistance() << ", default: " << LLHMDImpl::kDefaultLenSeparationDistance << LL_ENDL;
            LL_INFOS("Oculus") << "EyeToScreenDistance: " << std::fixed << std::setprecision(4) << getEyeToScreenDistance() << ", default: " << LLHMDImpl::kDefaultEyeToScreenDistance << LL_ENDL;
            dc = getDistortionConstants();
            LL_INFOS("Oculus") << "Distortion Constants: [" << std::fixed << std::setprecision(4) << dc.mV[0] << "," << dc.mV[1] << "," << dc.mV[2] << "," << dc.mV[3] << "]" << LL_ENDL;
            LL_INFOS("Oculus") << "Default Dist Cnstnts: [" << std::fixed << std::setprecision(4) << LLHMDImpl::kDefaultDistortionConstant0 << "," << LLHMDImpl::kDefaultDistortionConstant1 << "," << LLHMDImpl::kDefaultDistortionConstant2 << "," << LLHMDImpl::kDefaultDistortionConstant3 << "]" << LL_ENDL;
            LL_INFOS("Oculus") << "XCenterOffset: " << std::fixed << std::setprecision(4) << getXCenterOffset() << ", default: " << LLHMDImpl::kDefaultXCenterOffset << LL_ENDL;
            LL_INFOS("Oculus") << "YCenterOffset: " << std::fixed << std::setprecision(4) << getYCenterOffset() << ", default: " << LLHMDImpl::kDefaultYCenterOffset << LL_ENDL;
            LL_INFOS("Oculus") << "DistortionScale: " << std::fixed << std::setprecision(4) << getDistortionScale() << ", default: " << LLHMDImpl::kDefaultDistortionScale << LL_ENDL;
            LL_INFOS("Oculus") << "OrthoPixelOffset: " << std::fixed << std::setprecision(4) << getOrthoPixelOffset() << ", default: " << (-1.0f * LLHMDImpl::kDefaultOrthoPixelOffset) << LL_ENDL;

            setCurrentEye(OVR::Util::Render::StereoEye_Center);
        }
    }
}


void LLHMDImpl::OnMessage(const OVR::Message& msg)
{
    // TODO: take init out of llappviewer::idle() and add calls here so that init
    // is dynamic based on adding/removing the HMD
    if (msg.Type == Message_DeviceAdded && msg.pDevice == mpDeviceMgr)
    {
        LL_INFOS("Oculus") << "DeviceManager reported device added" << LL_ENDL;
    }
    else if (msg.Type == Message_DeviceRemoved && msg.pDevice == mpDeviceMgr)
    {
        LL_INFOS("Oculus") << "DeviceManager reported device removed" << LL_ENDL;
    }
    else if (msg.Type == Message_DeviceAdded && msg.pDevice == mpSensor)
    {
        LL_INFOS("Oculus") << "Sensor reported device added.\n" << LL_ENDL;
    }
    else if (msg.Type == Message_DeviceRemoved && msg.pDevice == mpSensor)
    {
        LL_INFOS("Oculus") << "Sensor reported device removed.\n" << LL_ENDL;
    }
}


#if LL_WINDOWS
// Used to capture all the active monitor handles
struct MonitorSet
{
    enum { MaxMonitors = 8 };
    HMONITOR Monitors[MaxMonitors];
    S32      MonitorCount;
    S32      PrimaryCount;
};


BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData)
{
    MonitorSet* monitorSet = (MonitorSet*)dwData;
    if (monitorSet->MonitorCount > MonitorSet::MaxMonitors)
    {
        return FALSE;
    }

    monitorSet->Monitors[monitorSet->MonitorCount] = hMonitor;
    monitorSet->MonitorCount++;
    return TRUE;
};

S32 LLHMDImpl::GetDisplayCount()
{
    // Get all the monitor handles
    MonitorSet monitors;
    monitors.MonitorCount = 0;
    ::EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);
    
    // Count the primary monitors
    int primary = 0;
    MONITORINFOEX info;
    for (S32 i = 0; i < monitors.MonitorCount; i++)
    {
        info.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(monitors.Monitors[i], &info);
        if (info.dwFlags & MONITORINFOF_PRIMARY)
        {
            primary++;
        }
    }
    
    if (primary > 1)
    {
        // Regard mirrored displays as a single screen
        return 1;
    }
    else
    {
        return monitors.MonitorCount;  // Return all extended displays
    }
}

BOOL LLHMDImpl::getDisplayInfo(const llutf16string& displayName, LLRect& rcWork, BOOL& isPrimary)
{
    MonitorSet monitors;
    monitors.MonitorCount = 0;
    ::EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);
    MONITORINFOEX info;
    for (S32 i = 0; i < monitors.MonitorCount; i++)
    {
        info.cbSize = sizeof(MONITORINFOEX);
        if (::GetMonitorInfo(monitors.Monitors[i], &info) && info.szDevice[0])
        {
            llutf16string displayNameTest1(info.szDevice);
            llutf16string displayNameTest2(info.szDevice);
            // for some reason, the library return seems to sometimes add "\\Monitor0" to the display name it gives.
            // however, the display names returned by GetMonitorInfo do not have that.  So we check for both
            // forms.
            displayNameTest2.append(L"\\Monitor0");
            if (!displayName.compare(displayNameTest1) || !displayName.compare(displayNameTest2))
            {
                isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) ? TRUE : FALSE;
                rcWork.set(info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom);
                return TRUE;
            }
        }
    }
    
    return FALSE;
}
#elif LL_DARWIN
// Need to implement the relevant monitor enumeration functions for Mac here
S32 LLHMDImpl::GetDisplayCount()
{
    return 1;
}
BOOL LLHMDImpl::getDisplayInfo(const llutf16string& displayName, LLRect& rcWork, BOOL& isPrimary)
{
    return FALSE;
}
#endif // LL_WINDOWS


/////////////////////////////////////////////////////////////////////////////////////////////////////
// LLHMD
/////////////////////////////////////////////////////////////////////////////////////////////////////

LLHMD gHMD;

LLHMD::LLHMD()
    : mInterpupillaryMod(0.0f)
    , mLensSepMod(0.0f)
    , mEyeToScreenMod(0.0f)
    , mXCenterOffsetMod(0.0f)
    , mRenderMode(RenderMode_None)
{
    mImpl = new LLHMDImpl();
}


LLHMD::~LLHMD()
{
    //if (isInitialized())
    //{
    //	gSavedSettings.getControl("OculusInterpupillaryOffsetModifier")->getSignal()->disconnect_all_slots();
	   // gSavedSettings.getControl("OculusLensSeparationDistanceModifier")->getSignal()->disconnect_all_slots();
	   // gSavedSettings.getControl("OculusEyeToScreenDistanceModifier")->getSignal()->disconnect_all_slots();
	   // gSavedSettings.getControl("OculusXCenterOffsetModifier")->getSignal()->disconnect_all_slots();
    //    gSavedSettings.getControl("OculusUIRenderSkip")->getSignal()->disconnect_all_slots();
    //}
    delete mImpl;
}


BOOL LLHMD::init()
{
    BOOL res = mImpl->init();
    //if (res)
    //{
    	gSavedSettings.getControl("OculusInterpupillaryOffsetModifier")->getSignal()->connect(boost::bind(&onChangeInterpupillaryOffsetModifer));
        onChangeInterpupillaryOffsetModifer();
	    gSavedSettings.getControl("OculusLensSeparationDistanceModifier")->getSignal()->connect(boost::bind(&onChangeLensSeparationDistanceModifier));
        onChangeLensSeparationDistanceModifier();
	    gSavedSettings.getControl("OculusEyeToScreenDistanceModifier")->getSignal()->connect(boost::bind(&onChangeEyeToScreenDistanceModifier));
        onChangeEyeToScreenDistanceModifier();
	    gSavedSettings.getControl("OculusXCenterOffsetModifier")->getSignal()->connect(boost::bind(&onChangeXCenterOffsetModifier));
        onChangeXCenterOffsetModifier();
    //}
    return res;
}

void LLHMD::onChangeInterpupillaryOffsetModifer() { gHMD.mInterpupillaryMod = gSavedSettings.getF32("OculusInterpupillaryOffsetModifier") * 0.1f; }
void LLHMD::onChangeLensSeparationDistanceModifier() { gHMD.mLensSepMod = gSavedSettings.getF32("OculusLensSeparationDistanceModifier") * 0.1f; }
void LLHMD::onChangeEyeToScreenDistanceModifier() { gHMD.mEyeToScreenMod = gSavedSettings.getF32("OculusEyeToScreenDistanceModifier") * 0.1f; }
void LLHMD::onChangeXCenterOffsetModifier() { gHMD.mXCenterOffsetMod = gSavedSettings.getF32("OculusXCenterOffsetModifier") * 0.1f; }

void LLHMD::shutdown() { mImpl->shutdown(); }
void LLHMD::onIdle() { mImpl->onIdle(); }

void LLHMD::setRenderMode(U32 mode)
{
    U32 newRenderMode = llclamp(mode, (U32)RenderMode_None, (U32)RenderMode_Last);
    if (newRenderMode != mRenderMode)
    {
        LLWindow* windowp = gViewerWindow->getWindow();
        if (!windowp || (newRenderMode == RenderMode_HMD && !isInitialized()))
        {
            return;
        }
        mRenderMode = newRenderMode;
        if (mRenderMode == RenderMode_HMD)
        {
            setFocusWindowHMD();
        }
        else
        {
            setFocusWindowMain();
        }
        if (isInitialized() && shouldRender() && !isCalibrated())
        {
            mImpl->BeginManualCalibration();
        }
    }
}


void LLHMD::setRenderWindowMain()
{
    gViewerWindow->getWindow()->setRenderWindow(0, gHMD.isMainFullScreen());
}


void LLHMD::setRenderWindowHMD()
{
    gViewerWindow->getWindow()->setRenderWindow(1, TRUE);
}


void LLHMD::setFocusWindowMain()
{
    gViewerWindow->getWindow()->setFocusWindow(0);
}


void LLHMD::setFocusWindowHMD()
{
    gViewerWindow->getWindow()->setFocusWindow(1);
}

U32 LLHMD::getCurrentEye() const { return mImpl->getCurrentEye(); }
void LLHMD::setCurrentEye(U32 eye) { mImpl->setCurrentEye(eye); }
void LLHMD::getViewportInfo(S32& x, S32& y, S32& w, S32& h) { mImpl->getViewportInfo(x, y, w, h); }
F32 LLHMD::getHScreenSize() const { return mImpl->getHScreenSize(); }
F32 LLHMD::getVScreenSize() const { return mImpl->getVScreenSize(); }
F32 LLHMD::getInterpupillaryOffset() const { return mInterpupillaryMod + mImpl->getInterpupillaryOffset(); }
F32 LLHMD::getLensSeparationDistance() const { return mLensSepMod + mImpl->getLensSeparationDistance(); }
F32 LLHMD::getEyeToScreenDistance() const { return mEyeToScreenMod + mImpl->getEyeToScreenDistance(); }
LLVector4 LLHMD::getDistortionConstants() const { return mImpl->getDistortionConstants(); }
F32 LLHMD::getXCenterOffset() const { return mXCenterOffsetMod + mImpl->getXCenterOffset(); }
F32 LLHMD::getYCenterOffset() const { return mImpl->getYCenterOffset(); }
F32 LLHMD::getDistortionScale() const { return mImpl->getDistortionScale(); }
LLQuaternion LLHMD::getHMDOrient() const { return mImpl->getHMDOrient(); }
void LLHMD::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { mImpl->getHMDRollPitchYaw(roll, pitch, yaw); }
const LLVector3& LLHMD::getRawHMDRollPitchYaw() const { return mImpl->getRawHMDRollPitchYaw(); }
void LLHMD::setFOV(F32 fov) { mImpl->setFOV(fov); }
//LLVector4 LLHMD::getChromaticAberrationConstants() const { return mImpl->getChromaticAberrationConstants(); }
//LLMatrix4 LLHMD::getViewAdjustMatrix() const { return mImpl->getViewAdjustMatrix(); }
//LLMatrix4 LLHMD::getProjectionMatrix() const { return mImpl->getProjectionMatrix(); }
//LLMatrix4 LLHMD::getOrthoProjectionMatrix() const { return mImpl->getOrthoProjectionMatrix(); }
F32 LLHMD::getOrthoPixelOffset() const { return mImpl->getOrthoPixelOffset(); }
