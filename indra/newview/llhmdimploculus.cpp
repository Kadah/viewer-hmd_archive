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


LLHMDImplOculus::LLHMDImplOculus()
    : mDeviceManager(NULL)
    , mHMD(NULL)
    , mSensorFusion(NULL)
    , mSensorDevice(NULL)
    //, mHeadRotationCorrection(LLQuaternion::DEFAULT)
    //, mHeadPitchCorrection(LLQuaternion::DEFAULT)
    , mpDeviceStatusNotificationsQueue(NULL)
    , mpLatencyTester(NULL)
    , mEyePitch(0.0f)
    , mEyeRoll(0.0f)
    , mEyeYaw(0.0f)
    , mCurrentEye(OVR::Util::Render::StereoEye_Center)
{
}


LLHMDImplOculus::~LLHMDImplOculus()
{
    shutdown();
}


BOOL LLHMDImplOculus::preInit()
{
    if (gHMD.isPreDetectionInitialized())
    {
        return TRUE;
    }

    OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_None));

    mSensorDevice = NULL;
    mSensorFusion = new OVR::SensorFusion(NULL);
    mpDeviceStatusNotificationsQueue = new OVR::Array<DeviceStatusNotificationDesc>();
    
    mDeviceManager = *OVR::DeviceManager::Create();
    if (!mDeviceManager)
    {
        LL_INFOS("HMD") << "HMD Preinit abort: could not create Oculus Rift HMD device manager" << LL_ENDL;
        gHMD.isPreDetectionInitialized(FALSE);
        return FALSE;
    }

    mDeviceManager->SetMessageHandler(this);

    mHMD = *mDeviceManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    initHMDDevice(TRUE);
    mpLatencyTester = *(mDeviceManager->EnumerateDevices<OVR::LatencyTestDevice>().CreateDevice());
    initHMDLatencyTester();

    // consider ourselves pre-initialized if we get here
    LL_INFOS("HMD") << "HMD Preinit successful" << LL_ENDL;

    gHMD.isPreDetectionInitialized(TRUE);
    return TRUE;
}


void LLHMDImplOculus::initHMDDevice(BOOL initSensor)
{
    gHMD.isHMDConnected(FALSE);
    if (mHMD)
    {
        OVR::HMDInfo info;
        bool validInfo = !mHMD->IsDisconnected() && mHMD->GetDeviceInfo(&info) && info.HResolution > 0;
        if (validInfo)
        {
            // Retrieve relevant profile settings if available, otherwise use saved settings
            OVR::Profile* pUserProfile = mHMD->GetProfile();
            if (pUserProfile)
            {
                info.InterpupillaryDistance = pUserProfile->GetIPD();
            }
            else
            {
                info.InterpupillaryDistance = gSavedSettings.getF32("HMDInterpupillaryDistance");
            }
            info.EyeToScreenDistance = gSavedSettings.getF32("HMDEyeToScreenDistance");
            mStereoConfig.SetHMDInfo(info);
            gHMD.isHMDConnected(TRUE);

            // *** Configure Stereo settings.
            mStereoConfig.SetFullViewport(OVR::Util::Render::Viewport(0,0, info.HResolution, info.VResolution));
            mStereoConfig.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);

            // Configure proper Distortion Fit.
            // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
            // screen on the top (saves on rendering cost).
            // For smaller screens (5.5"), fit to the top.
            if (info.HScreenSize > 0.0f)
            {
                if (info.HScreenSize > 0.140f) // 7"
                {
                    mStereoConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
                }
                else
                {
                    mStereoConfig.SetDistortionFitPointVP(0.0f, 1.0f);
                }
            }

            if (info.DisplayDeviceName[0])
            {
                LL_INFOS("HMD") << "HMD initHMDDevice: Found connected HMD device " << info.DisplayDeviceName << "[" << info.DisplayId << "]" << LL_ENDL;
            }
            else
            {
                LL_INFOS("HMD") << "HMD initHMDDevice: Found connected HMD device with ID [" << info.DisplayId << "]" << LL_ENDL;
            }
        }
        else
        {
            LL_INFOS("HMD") << "HMD initHMDDevice: Could not find connected HMD device" << LL_ENDL;
        }
    }
    else
    {
        // check for HMD window and destroy if it exists
        if (gHMD.isPostDetectionInitialized())
        {
            LLWindow* pWin = gViewerWindow ? gViewerWindow->getWindow() : NULL;
            if (pWin)
            {
                pWin->destroyHMDWindow();
            }
            gHMD.isPostDetectionInitialized(FALSE);
        }
    }
    if (initSensor)
    {
        if (mHMD)
        {
            mSensorDevice = *(mHMD->GetSensor());
        }
        else
        {
            LL_INFOS("HMD") << "HMD initHMDDevice: could not create Oculus Rift HMD device" << LL_ENDL;
            mSensorDevice = *mDeviceManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
        }
        initHMDSensor();
    }
}


void LLHMDImplOculus::initHMDSensor()
{
    if (mSensorDevice)
    {
        gHMD.isHMDSensorConnected(TRUE);
        if (mSensorFusion->AttachToSensor(mSensorDevice))
        {
            LL_INFOS("HMD") << "HMD Sensor device found and successfully attached to SensorFusion" << LL_ENDL;
        }
        else
        {
            LL_INFOS("HMD") << "HMD Sensor device found, but could not attach to SensorFusion" << LL_ENDL;
        }
        mSensorFusion->SetDelegateMessageHandler(this);
        mSensorFusion->SetPredictionEnabled(gSavedSettings.getBOOL("HMDUseMotionPrediction"));
    }
    else
    {
        gHMD.isHMDSensorConnected(FALSE);
        mSensorFusion->AttachToSensor(NULL);
    }
}


void LLHMDImplOculus::initHMDLatencyTester()
{
    if (mpLatencyTester)
    {
        mLatencyUtil.SetDevice(mpLatencyTester);
        gHMD.isLatencyTesterConnected(TRUE);
    }
    else
    {
        mLatencyUtil.SetDevice(NULL);
        gHMD.isLatencyTesterConnected(FALSE);
    }
}


void LLHMDImplOculus::handleMessages()
{
    bool queue_is_empty = false;

    while (! queue_is_empty)
    {
        DeviceStatusNotificationDesc desc;
        {
            OVR::Lock::Locker lock(mDeviceManager->GetHandlerLock());
            if (!mpDeviceStatusNotificationsQueue || mpDeviceStatusNotificationsQueue->GetSize() == 0)
            {
                break;
            }
            desc = mpDeviceStatusNotificationsQueue->Front();

            mpDeviceStatusNotificationsQueue->RemoveAt(0);
            queue_is_empty = (mpDeviceStatusNotificationsQueue->GetSize() == 0);
        }
        
        if (desc.Action == OVR::Message_DeviceAdded)
        {
            switch(desc.Handle.GetType())
            {
            case OVR::Device_Sensor:
                if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated() && !mSensorDevice)
                {
                    mSensorDevice = *desc.Handle.CreateDeviceTyped<OVR::SensorDevice>();
                    initHMDSensor();
                    LL_INFOS("HMD") << "HMD Sensor Device Added" << LL_ENDL;
                }
                break;
            case OVR::Device_LatencyTester:
                if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated() && !mpLatencyTester)
                {
                    mpLatencyTester = *desc.Handle.CreateDeviceTyped<OVR::LatencyTestDevice>();
                    initHMDLatencyTester();
                    LL_INFOS("HMD") << "HMD Latency Tester Device Added" << LL_ENDL;
                }
                break;
            case OVR::Device_HMD:
                {
                    OVR::HMDInfo info;
                    bool validInfo = desc.Handle.GetDeviceInfo(&info) && info.HResolution > 0;
                    if (validInfo &&
                        (!mHMD || !info.IsSameDisplay(mStereoConfig.GetHMDInfo())))
                    {
                        if (!mHMD || !desc.Handle.IsDevice(mHMD))
                        {
                            mHMD = *desc.Handle.CreateDeviceTyped<OVR::HMDDevice>();
                        }
                        initHMDDevice(FALSE);
                        LL_INFOS("HMD") << "HMD Device Added" << LL_ENDL;
                    }
                }
                break;
            default:
                break;
            }
        }
        else if (desc.Action == OVR::Message_DeviceRemoved)
        {
            if (desc.Handle.IsDevice(mSensorDevice))
            {
                if (gHMD.isHMDMode())
                {
                    gHMD.setRenderMode(LLHMD::RenderMode_None);
                }
                mSensorDevice.Clear();
                initHMDSensor();
                LL_INFOS("HMD") << "HMD Sensor Device Removed" << LL_ENDL;
            }
            else if (desc.Handle.IsDevice(mpLatencyTester))
            {
                mpLatencyTester.Clear();
                initHMDLatencyTester();
                LL_INFOS("HMD") << "HMD Latency Tester Device Removed" << LL_ENDL;
            }
            else if (desc.Handle.IsDevice(mHMD))
            {
                if (gHMD.isHMDMode())
                {
                    gHMD.setRenderMode(LLHMD::RenderMode_None);
                }
                if (mHMD && !mHMD->IsDisconnected())
                {
                    mHMD = mHMD->Disconnect(mSensorDevice);
                    LL_INFOS("HMD") << "HMD Device Removed" << LL_ENDL;
                }
                gHMD.isHMDConnected(FALSE);
            }
        }
    }
}


// virtual
void LLHMDImplOculus::OnMessage(const OVR::Message& msg)
{
    if ((msg.Type == OVR::Message_DeviceAdded || msg.Type == OVR::Message_DeviceRemoved) && msg.pDevice == mDeviceManager)
    {
        const OVR::MessageDeviceStatus& statusMsg = static_cast<const OVR::MessageDeviceStatus&>(msg);
        if (mpDeviceStatusNotificationsQueue)
        {
            OVR::Lock::Locker lock(mDeviceManager->GetHandlerLock());
            mpDeviceStatusNotificationsQueue->PushBack(DeviceStatusNotificationDesc(statusMsg.Type, statusMsg.Handle));
        }
    }
}


BOOL LLHMDImplOculus::postDetectionInit()
{
    LLWindow* pWin = gViewerWindow->getWindow();
    BOOL mainFullScreen = FALSE;
    S32 rcIdx = pWin->getRenderWindow(mainFullScreen);
    if (rcIdx == 0)
    {
        gHMD.isMainFullScreen(mainFullScreen);
    }
    const OVR::HMDInfo& info = mStereoConfig.GetHMDInfo();
    BOOL isMirror = FALSE;
#if LL_WINDOWS
    if (!pWin->initHMDWindow(info.DesktopX, info.DesktopY, info.HResolution, info.VResolution, isMirror))
#elif LL_DARWIN
    if (!pWin->initHMDWindow(info.DisplayId, 0, info.HResolution, info.VResolution, isMirror))
#else
    if (FALSE)
#endif
    {
        LL_INFOS("HMD") << "HMD Window init failed!" << LL_ENDL;
        return FALSE;
    }

    gHMD.isPostDetectionInitialized(TRUE);
    gHMD.failedInit(FALSE);
    gHMD.isHMDMirror(isMirror);

    setCurrentEye(OVR::Util::Render::StereoEye_Center);

    LL_INFOS("HMD") << "HMD Post-Detection Init Success" << LL_ENDL;

    return TRUE;
}


void LLHMDImplOculus::shutdown()
{
    if (!gHMD.isPreDetectionInitialized())
    {
        return;
    }
    if (gHMD.isPostDetectionInitialized())
    {
        gViewerWindow->getWindow()->destroyHMDWindow();
    }
    RemoveHandlerFromDevices();
    mLatencyUtil.SetDevice(NULL);
    mpLatencyTester.Clear();
    if (mSensorFusion)
    {
        mSensorFusion->AttachToSensor(NULL);
    }
    mSensorDevice.Clear();
    if (mpDeviceStatusNotificationsQueue)
    {
        mpDeviceStatusNotificationsQueue->ClearAndRelease();
        delete mpDeviceStatusNotificationsQueue;
        mpDeviceStatusNotificationsQueue = NULL;
    }
    mHMD.Clear();
    mDeviceManager.Clear();
    delete mSensorFusion;
    mSensorFusion = NULL;

    OVR::System::Destroy();

    // make sure if/when we call shutdown again, we don't try to deallocate things twice.
    gHMD.isPreDetectionInitialized(FALSE);
    gHMD.isPostDetectionInitialized(FALSE);
}


void LLHMDImplOculus::onIdle()
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
        postDetectionInit();
        // give the HMD a frame to internally initialize before trying to access it
        return;
    }

    // Process latency tester results.
    // Have to place this as close as possible to where the HMD orientation is read.
    mLatencyUtil.ProcessInputs();

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
        OVR::Quatf orient = useMotionPrediction() ? mSensorFusion->GetPredictedOrientation() : mSensorFusion->GetOrientation();
        orient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&mEyeYaw, &mEyePitch, &mEyeRoll);
        mEyeRoll = -mEyeRoll;
        mEyePitch = -mEyePitch;
    }
}


LLVector4 LLHMDImplOculus::getDistortionConstants() const
{
    if (gHMD.isPostDetectionInitialized())
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

#endif // LL_HMD_SUPPORTED
