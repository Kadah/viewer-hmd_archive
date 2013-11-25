/** 
* @file llhmdimploculus.cpp
* @brief Implementation of llhmd
* @author voidpointer@lindenlab.com
* @author callum@lindenlab.com
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
#include "llhmdimploculus.h"

#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llview.h"

#if LL_HMD_SUPPORTED

#if LL_WINDOWS
    #include "llwindowwin32.h"
#elif LL_DARWIN
    #include "llwindowmacosx.h"
    #define IDCONTINUE 1        // Exist on Windows along "IDOK" and "IDCANCEL" but not on Mac
#endif

using namespace OVR;

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
const F32 LLHMDImpl::kDefaultVerticalFOVRadians = 1.5707963f;
const F32 LLHMDImpl::kDefaultAspect = 0.8f;
const F32 LLHMDImpl::kDefaultAspectMult = 1.0f;


LLHMDImpl::LLHMDImpl()
    : mEyePitch(0.0f)
    , mEyeRoll(0.0f)
    , mEyeYaw(0.0f)
    , mCurrentEye(OVR::Util::Render::StereoEye_Center)
    , mLastCalibrationStep(-1)
    , mCalibrationText("")
{
}


LLHMDImpl::~LLHMDImpl()
{
    shutdown();
}


BOOL LLHMDImpl::preInit()
{
    if (gHMD.isPreDetectionInitialized())
    {
        return TRUE;
    }

    OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_None));

    mDeviceManager = *OVR::DeviceManager::Create();
    if (!mDeviceManager)
    {
        gHMD.isInitialized(FALSE);
        gHMD.isPreDetectionInitialized(FALSE);
        return FALSE;
    }

    mDeviceManager->SetMessageHandler(this);

    mHMD = *mDeviceManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    if (!mHMD)
    {
        gHMD.isPreDetectionInitialized(TRUE); // consider ourselves pre-initialized if we get here
        gHMD.isInitialized(FALSE);
        return FALSE;
    }

    if (mHMD)
    {
        OVR::HMDInfo info;
        bool validInfo = mHMD->GetDeviceInfo(&info) && info.HResolution > 0;
        if (validInfo)
        {
            mDisplayName = utf8str_to_utf16str(info.DisplayDeviceName);
            mDisplayId = info.DisplayId;
            info.InterpupillaryDistance = gSavedSettings.getF32("OculusInterpupillaryDistance");
            info.EyeToScreenDistance = gSavedSettings.getF32("OculusEyeToScreenDistance");
            mStereoConfig.SetHMDInfo(info);
            gHMD.isHMDConnected(TRUE);
        }
    }

    mSensorDevice = 0;
    
    gHMD.isInitialized(TRUE);
    gHMD.isPreDetectionInitialized(TRUE); 

    return TRUE;
}


void LLHMDImpl::handleMessages()
{
    bool queue_is_empty = false;

    while (! queue_is_empty)
    {
        DeviceStatusNotificationDesc desc;
        {
            OVR::Lock::Locker lock(mDeviceManager->GetHandlerLock());
            if (DeviceStatusNotificationsQueue.GetSize() == 0)
            {
                break;
            }
            desc = DeviceStatusNotificationsQueue.Front();

            DeviceStatusNotificationsQueue.RemoveAt(0);
            queue_is_empty = (DeviceStatusNotificationsQueue.GetSize() == 0);
        }
        
        bool was_already_created = desc.Handle.IsCreated();
        
        if (desc.Action == OVR::Message_DeviceAdded)
        {
            switch(desc.Handle.GetType())
            {
                case OVR::Device_Sensor:
                    if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated())
                    {
                        if (!mSensorDevice)
                        {
                            mSensorDevice = *desc.Handle.CreateDeviceTyped<OVR::SensorDevice>();
                            mSensorFusion.AttachToSensor(mSensorDevice);
                            mSensorFusion.SetDelegateMessageHandler(this);
                            mSensorFusion.SetPredictionEnabled(gSavedSettings.getBOOL("OculusUseMotionPrediction"));
                        }
                        else
                        if (!was_already_created )
                        {
                            // A new sensor has been detected, but it is not currently used.
                        }
                    }
                    break;

                case OVR::Device_HMD:
                {
                    OVR::HMDInfo info;
                    bool validInfo = desc.Handle.GetDeviceInfo(&info) && info.HResolution > 0;
                    if (validInfo &&
                        info.DisplayDeviceName[0] &&
                        //strlen(info.DisplayDeviceName) > 0 &&
                        (!mHMD || !info.IsSameDisplay(mStereoConfig.GetHMDInfo())))
                    {
                        if (!mHMD || !desc.Handle.IsDevice(mHMD))
                        {
                            mHMD = *desc.Handle.CreateDeviceTyped<OVR::HMDDevice>();
                        }
                        if (mHMD)
                        {
                            info.InterpupillaryDistance = gSavedSettings.getF32("OculusInterpupillaryDistance");
                            info.EyeToScreenDistance = gSavedSettings.getF32("OculusEyeToScreenDistance");
                            mDisplayName = utf8str_to_utf16str(info.DisplayDeviceName);
                            mDisplayId = info.DisplayId;
                            mStereoConfig.SetHMDInfo(info);
                            gHMD.isHMDConnected(TRUE);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else 
        if (desc.Action == OVR::Message_DeviceRemoved)
        {
            if (desc.Handle.IsDevice(mSensorDevice))
            {
                mSensorFusion.AttachToSensor(NULL);
                mSensorDevice.Clear();
            }
            else if (desc.Handle.IsDevice(mHMD))
            {
                gHMD.isHMDConnected(FALSE);
                if (mHMD && !mHMD->IsDisconnected())
                {
                    mHMD = mHMD->Disconnect(mSensorDevice);
                    OVR::HMDInfo info;
                    if (mHMD && mHMD->GetDeviceInfo(&info) && info.HResolution > 0)
                    {
                        info.InterpupillaryDistance = gSavedSettings.getF32("OculusInterpupillaryDistance");
                        info.EyeToScreenDistance = gSavedSettings.getF32("OculusEyeToScreenDistance");
                        mDisplayName = utf8str_to_utf16str(info.DisplayDeviceName);
                        mDisplayId = info.DisplayId;
                        mStereoConfig.SetHMDInfo(info);
                    }
                }
            }
        }
        else
        {
            // unknown action - TODO: what do we do here if anything?
        }
    }
}


// virtual
void LLHMDImpl::OnMessage(const OVR::Message& msg)
{
    if (msg.Type == OVR::Message_DeviceAdded || msg.Type == OVR::Message_DeviceRemoved)
    {
        if (msg.pDevice == mDeviceManager)
        {
            const OVR::MessageDeviceStatus& statusMsg = static_cast<const OVR::MessageDeviceStatus&>(msg);
            {
                OVR::Lock::Locker lock(mDeviceManager->GetHandlerLock());
                DeviceStatusNotificationsQueue.PushBack(DeviceStatusNotificationDesc(statusMsg.Type, statusMsg.Handle));
            }

            switch (statusMsg.Type)
            {
                case OVR::Message_DeviceAdded:
                    break;

                case OVR::Message_DeviceRemoved:
                    break;

                default:
                    // DeviceManager reported unknown action.
                    break;
            }
        }
    }
}


BOOL LLHMDImpl::postDetectionInit()
{
    mpLatencyTester = *(mDeviceManager->EnumerateDevices<LatencyTestDevice>().CreateDevice());
    if (mpLatencyTester)
    {
        mLatencyUtil.SetDevice(mpLatencyTester);
    }

    // get device's "monitor" info
    BOOL dummy;
    BOOL mainFullScreen = FALSE;
    LLRect r;
    LLWindow* pWin = gViewerWindow->getWindow();
    pWin->getDisplayInfo(mDisplayName, mDisplayId, r, dummy);
    S32 rcIdx = pWin->getRenderWindow(mainFullScreen);
    if (rcIdx == 0)
    {
        gHMD.isMainFullScreen(mainFullScreen);
    }
    LL_INFOS("Oculus") << "Got HMD Display Info: " << utf16str_to_utf8str(mDisplayName) << " [" << rcIdx << "] is " << (mainFullScreen ? " " : "NOT") << " fullscreen" << LL_ENDL;
    if (!pWin->initHMDWindow(r.mLeft, r.mTop, r.mRight - r.mLeft, r.mBottom - r.mTop))
    {
        LL_INFOS("Oculus") << "HMD Window init with rect " << r << " Failed!" << LL_ENDL;
        return FALSE;
    }

    // *** Configure Stereo settings.
    mStereoConfig.SetFullViewport(OVR::Util::Render::Viewport(0,0, mStereoConfig.GetHMDInfo().HResolution, mStereoConfig.GetHMDInfo().VResolution));
    mStereoConfig.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);

    // Configure proper Distortion Fit.
    // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
    // screen on the top (saves on rendering cost).
    // For smaller screens (5.5"), fit to the top.
    if (mStereoConfig.GetHMDInfo().HScreenSize > 0.0f)
    {
        if (mStereoConfig.GetHMDInfo().HScreenSize > 0.140f) // 7"
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

    // This causes a deadlock.  No idea why.   Disabling it as it doesn't seem to be necessary unless we're actually RE-initializing the HMD
    // without shutting down.   For now, to init HMD, we have to restart the viewer.
    //OVR::System::Destroy();
}


void LLHMDImpl::onIdle()
{
    if (!gHMD.isPreDetectionInitialized())
    {
        return;
    }

    handleMessages();

    // still waiting for device to initialize
    if (!isReady())
    {
        return;
    }

    if (!gHMD.isPostDetectionInitialized())
    {
        BOOL result = postDetectionInit();
        gHMD.isPostDetectionInitialized(result);
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
            mMagCal.UpdateAutoCalibration(mSensorFusion);
            // if (mMagCal.IsCalibrated())
            // {
            //    Vector3f mc = mMagCal.GetMagCenter();
            //    SetAdjustMessage("   Magnetometer Calibration Complete   \nCenter: %f %f %f",mc.x,mc.y,mc.z);
            // }
            // SetAdjustMessage("Mag has been successfully calibrated");
        }
    }

    // Handle Sensor motion.
    if (mSensorDevice)
    {
        // Note that the LL coord system uses X forward, Y left, and Z up whereas the Oculus SDK uses the
        // OpenGL coord system of -Z forward, X right, Y up.  To compensate, we retrieve the angles in the Oculus
        // coord system, but change the axes to ours, then negate X and Z to account for the forward left axes
        // being positive in LL, but negative in Oculus.
        // LL X = Oculus -Z, LL Y = Oculus -X, and LL Z = Oculus Y
        // Yaw = rotation around the "up" axis          (LL  Z, Oculus  Y)
        // Pitch = rotation around the left/right axis  (LL -Y, Oculus  X)
        // Roll = rotation around the forward axis      (LL  X, Oculus -Z)
        OVR::Quatf orient = useMotionPrediction() ? mSensorFusion.GetPredictedOrientation() : mSensorFusion.GetOrientation();
        orient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&mEyeYaw, &mEyePitch, &mEyeRoll);
        mEyeRoll = -mEyeRoll;
        mEyePitch = -mEyePitch;
    }
}


LLVector4 LLHMDImpl::getDistortionConstants() const
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

    
void LLHMDImpl::updateManualMagCalibration()
{
    F32 yaw, pitch, roll;
    OVR::Quatf hmdOrient = mSensorFusion.GetOrientation();
    hmdOrient.GetEulerAngles<Axis_X, Axis_Z, Axis_Y>(&pitch, &roll, &yaw);
    OVR::Vector3f mag = mSensorFusion.GetMagnetometer();
    float dtr = OVR::Math<float>::DegreeToRadFactor;

    switch(mMagCal.NumberOfSamples())
    {
    case 0:
        if (mLastCalibrationStep != 0)
        {
            mCalibrationText = "** Step 1: Please Look Forward **";
            //LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
            mLastCalibrationStep = 0;
        }
        if ((fabs(yaw) < 10.0f * dtr) && (fabs(pitch) < 10.0f * dtr))
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 1:
        if (mLastCalibrationStep != 1)
        {
            mCalibrationText = "** Step 2: Please Look Up **";
            //LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
            mLastCalibrationStep = 1;
        }
        if (pitch > 30.0f * dtr)
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 2:
        if (mLastCalibrationStep != 2)
        {
            mCalibrationText = "** Step 3: Please Look Left **";
            //LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
            mLastCalibrationStep = 2;
        }
        if (yaw > 30.0f * dtr)
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 3:
        if (mLastCalibrationStep != 3)
        {
            mCalibrationText = "** Step 4: Please Look Right **";
            //LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
            mLastCalibrationStep = 3;
        }
        if (yaw < -30.0f * dtr)
        {
            mMagCal.InsertIfAcceptable(hmdOrient, mag);
        }
        break;
    case 4:
        if (!mMagCal.IsCalibrated()) 
        {
            mMagCal.SetCalibration(mSensorFusion);
            //Vector3f mc = mMagCal.GetMagCenter();
            //LL_INFOS("Oculus") << "Magnetometer Calibration Complete   \nCenter: " << mc.x << "," << mc.y << "," << mc.z << LL_ENDL;
            mLastCalibrationStep = 4;
            gHMD.isCalibrated(TRUE);
            mCalibrationText = "";
            if (gHMD.isHMDMode() && gHMD.shouldShowCalibrationUI() && !gHMD.shouldShowDepthVisual())
            {
                LLUI::getRootView()->getChildView("menu_stack")->setVisible(TRUE);
            }
            //setCurrentEye(OVR::Util::Render::StereoEye_Left);
            //LL_INFOS("Oculus") << "Left Eye:" << LL_ENDL;
            //LL_INFOS("Oculus") << "HScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenWidth() << ", default: " << LLHMDImpl::kDefaultHScreenSize << LL_ENDL;
            //LL_INFOS("Oculus") << "VScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenHeight() << ", default: " << LLHMDImpl::kDefaultVScreenSize << LL_ENDL;
            //LL_INFOS("Oculus") << "InterpupillaryOffset: " << std::fixed << std::setprecision(4) << getInterpupillaryOffset() << ", default: " << LLHMDImpl::kDefaultInterpupillaryOffset << LL_ENDL;
            //LL_INFOS("Oculus") << "LensSeparationDistance: " << std::fixed << std::setprecision(4) << getLensSeparationDistance() << ", default: " << LLHMDImpl::kDefaultLenSeparationDistance << LL_ENDL;
            //LL_INFOS("Oculus") << "EyeToScreenDistance: " << std::fixed << std::setprecision(4) << getEyeToScreenDistance() << ", default: " << LLHMDImpl::kDefaultEyeToScreenDistance << LL_ENDL;
            //LLVector4 dc = getDistortionConstants();
            //LL_INFOS("Oculus") << "Distortion Constants: [" << std::fixed << std::setprecision(4) << dc.mV[0] << "," << dc.mV[1] << "," << dc.mV[2] << "," << dc.mV[3] << "]" << LL_ENDL;
            //LL_INFOS("Oculus") << "Default Dist Cnstnts: [" << std::fixed << std::setprecision(4) << LLHMDImpl::kDefaultDistortionConstant0 << "," << LLHMDImpl::kDefaultDistortionConstant1 << "," << LLHMDImpl::kDefaultDistortionConstant2 << "," << LLHMDImpl::kDefaultDistortionConstant3 << "]" << LL_ENDL;
            //LL_INFOS("Oculus") << "XCenterOffset: " << std::fixed << std::setprecision(4) << getXCenterOffset() << ", default: " << LLHMDImpl::kDefaultXCenterOffset << LL_ENDL;
            //LL_INFOS("Oculus") << "YCenterOffset: " << std::fixed << std::setprecision(4) << getYCenterOffset() << ", default: " << LLHMDImpl::kDefaultYCenterOffset << LL_ENDL;
            //LL_INFOS("Oculus") << "DistortionScale: " << std::fixed << std::setprecision(4) << getDistortionScale() << ", default: " << LLHMDImpl::kDefaultDistortionScale << LL_ENDL;
            //LL_INFOS("Oculus") << "OrthoPixelOffset: " << std::fixed << std::setprecision(4) << getOrthoPixelOffset() << ", default: " << LLHMDImpl::kDefaultOrthoPixelOffset << LL_ENDL;

            //setCurrentEye(OVR::Util::Render::StereoEye_Right);
            //LL_INFOS("Oculus") << "Right Eye:" << LL_ENDL;
            //LL_INFOS("Oculus") << "HScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenWidth() << ", default: " << LLHMDImpl::kDefaultHScreenSize << LL_ENDL;
            //LL_INFOS("Oculus") << "VScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenHeight() << ", default: " << LLHMDImpl::kDefaultVScreenSize << LL_ENDL;
            //LL_INFOS("Oculus") << "InterpupillaryOffset: " << std::fixed << std::setprecision(4) << getInterpupillaryOffset() << ", default: " << LLHMDImpl::kDefaultInterpupillaryOffset << LL_ENDL;
            //LL_INFOS("Oculus") << "LensSeparationDistance: " << std::fixed << std::setprecision(4) << getLensSeparationDistance() << ", default: " << LLHMDImpl::kDefaultLenSeparationDistance << LL_ENDL;
            //LL_INFOS("Oculus") << "EyeToScreenDistance: " << std::fixed << std::setprecision(4) << getEyeToScreenDistance() << ", default: " << LLHMDImpl::kDefaultEyeToScreenDistance << LL_ENDL;
            //dc = getDistortionConstants();
            //LL_INFOS("Oculus") << "Distortion Constants: [" << std::fixed << std::setprecision(4) << dc.mV[0] << "," << dc.mV[1] << "," << dc.mV[2] << "," << dc.mV[3] << "]" << LL_ENDL;
            //LL_INFOS("Oculus") << "Default Dist Cnstnts: [" << std::fixed << std::setprecision(4) << LLHMDImpl::kDefaultDistortionConstant0 << "," << LLHMDImpl::kDefaultDistortionConstant1 << "," << LLHMDImpl::kDefaultDistortionConstant2 << "," << LLHMDImpl::kDefaultDistortionConstant3 << "]" << LL_ENDL;
            //LL_INFOS("Oculus") << "XCenterOffset: " << std::fixed << std::setprecision(4) << getXCenterOffset() << ", default: " << LLHMDImpl::kDefaultXCenterOffset << LL_ENDL;
            //LL_INFOS("Oculus") << "YCenterOffset: " << std::fixed << std::setprecision(4) << getYCenterOffset() << ", default: " << LLHMDImpl::kDefaultYCenterOffset << LL_ENDL;
            //LL_INFOS("Oculus") << "DistortionScale: " << std::fixed << std::setprecision(4) << getDistortionScale() << ", default: " << LLHMDImpl::kDefaultDistortionScale << LL_ENDL;
            //LL_INFOS("Oculus") << "OrthoPixelOffset: " << std::fixed << std::setprecision(4) << getOrthoPixelOffset() << ", default: " << (-1.0f * LLHMDImpl::kDefaultOrthoPixelOffset) << LL_ENDL;

            setCurrentEye(OVR::Util::Render::StereoEye_Center);
        }
    }
}

#endif // LL_HMD_SUPPORTED