/** 
* @file llhmd.cpp
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
#include "llhmd.h"

#include "llfloaterreg.h"


#include "llviewerwindow.h"
#include "llagent.h"
#include "llviewercamera.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "pipeline.h"
#include "llagentcamera.h"
#include "llviewertexture.h"
#include "raytrace.h"

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

// TODO_VR: Add support for non-supported platforms. Currently waiting for Oculus SDK to support Linux.

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
    static const F32 kDefaultVerticalFOVRadians;
    static const F32 kDefaultAspect;
    static const F32 kDefaultAspectMult;
    static const F32 kUIEyeDepthMult;

public:
    LLHMDImpl();
    ~LLHMDImpl();



///////////////////////////////////////////////////////////////////
public:
    BOOL init();
    BOOL preInit();
    BOOL postDetectionInit();
    void handleMessages();
    bool isReady() { return mHMD && mSensorDevice && mHMDConnected; }
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

    F32 getPhysicalScreenWidth() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().HScreenSize : kDefaultHScreenSize; }
    F32 getPhysicalScreenHeight() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().VScreenSize : kDefaultVScreenSize; }
    F32 getInterpupillaryOffset() const { return gHMD.isInitialized() ? mStereoConfig.GetIPD() : getInterpupillaryOffsetDefault(); }
    F32 getInterpupillaryOffsetDefault() const { return kDefaultInterpupillaryOffset; }
    void setInterpupillaryOffset(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetIPD(f); } }
    F32 getLensSeparationDistance() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().LensSeparationDistance : kDefaultLenSeparationDistance; }
    F32 getEyeToScreenDistance() const { return gHMD.isInitialized() ? mStereoConfig.GetEyeToScreenDistance() : getEyeToScreenDistanceDefault(); }
    F32 getEyeToScreenDistanceDefault() const { return kDefaultEyeToScreenDistance; }
    void setEyeToScreenDistance(F32 f)
    {
        if (gHMD.isInitialized())
        {
            mStereoConfig.SetEyeToScreenDistance(f);
            gHMD.isUIEyeDepthCalculated(FALSE);
            gHMD.onChangeUISurfaceShape();
            //LL_INFOS("Oculus") << "EyeToScreenDistance: " << std::fixed << std::setprecision(4) << getEyeToScreenDistance() << ", default: " << LLHMDImpl::kDefaultEyeToScreenDistance << LL_ENDL;
            //LL_INFOS("Oculus") << "Vertical FOV: " << std::fixed << std::setprecision(4) << mStereoConfig.GetYFOVRadians() << ", default: " << kDefaultVerticalFOVRadians << LL_ENDL;
        }
    }

    F32 getVerticalFOV() { return gHMD.isInitialized() ? mStereoConfig.GetYFOVRadians() : kDefaultVerticalFOVRadians; }
    F32 getAspect() { return gHMD.isInitialized() ? mStereoConfig.GetAspect() : kDefaultAspect; }
    F32 getAspectMultiplier() { return gHMD.isInitialized() ? mStereoConfig.GetAspectMultiplier() : kDefaultAspectMult; }
    void setAspectMultiplier(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetAspectMultiplier(f); } }

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
    //LLMatrix4 getViewAdjustMatrix() const { return matrixOVRtoLL(mCurrentEyeParams.ViewAdjust); }
    //LLMatrix4 getProjectionMatrix() const { return matrixOVRtoLL(mCurrentEyeParams.Projection); }
    //LLMatrix4 getOrthoProjectionMatrix() const { return matrixOVRtoLL(mCurrentEyeParams.OrthoProjection); }

    F32 getOrthoPixelOffset() const { return gHMD.isInitialized() ? mCurrentEyeParams.OrthoProjection.M[0][3] : (kDefaultOrthoPixelOffset * (mCurrentEye == (U32)OVR::Util::Render::StereoEye_Left ? 1.0f : -1.0f)); }

    BOOL isManuallyCalibrating() const { return gHMD.isInitialized() ? mMagCal.IsManuallyCalibrating() : FALSE; }
    const std::string& getCalibrationText() const { return mCalibrationText; }

    LLViewerTexture* getCursorImage(U32 idx) { return (idx < mCursorTextures.size()) ? mCursorTextures[idx].get() : NULL; }

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

private:
    OVR::Util::MagCalibration mMagCal;
    Util::LatencyTest mLatencyUtil;
    OVR::Ptr <OVR::DeviceManager> mDeviceManager;
    OVR::Ptr <OVR::HMDDevice> mHMD;
    OVR::HMDInfo mHMDInfo;
    OVR::SensorFusion mSensorFusion;
    OVR::Ptr <OVR::SensorDevice> mSensorDevice;
    OVR::Util::Render::StereoConfig mStereoConfig;
    bool mHMDConnected;
    LLQuaternion mHeadRotationCorrection;

    struct DeviceStatusNotificationDesc
    {
        OVR::DeviceHandle Handle;
        OVR::MessageType Action;

        DeviceStatusNotificationDesc():Action (OVR::Message_None) { }

        DeviceStatusNotificationDesc(OVR::MessageType mt, const OVR::DeviceHandle& dev) :
            Handle (dev),
            Action (mt)
        {
        }
    };
    OVR::Array <DeviceStatusNotificationDesc> DeviceStatusNotificationsQueue;

    OVR::Ptr<LatencyTestDevice> mpLatencyTester;
    llutf16string mDisplayName;     // Identity of the Oculus on Windows
    long mDisplayId;                // Identity of the Oculus on Mac
    LLVector3 mRawHMDRollPitchYaw;
    F32 mEyePitch;
    F32 mEyeRoll;
    F32 mEyeYaw;
    U32 mCurrentEye;
    OVR::Util::Render::StereoEyeParams mCurrentEyeParams;
    LLRect mHMDRect;
    S32 mLastCalibrationStep;
    std::vector<LLPointer<LLViewerTexture> > mCursorTextures;
    std::string mCalibrationText;
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
const F32 LLHMDImpl::kDefaultVerticalFOVRadians = 1.5707963f;
const F32 LLHMDImpl::kDefaultAspect = 0.8f;
const F32 LLHMDImpl::kDefaultAspectMult = 1.0f;
const F32 LLHMDImpl::kUIEyeDepthMult = -14.285714f;
// formula:  X = N / ((DESD - MESD) * 1000.0)
// WHERE
//  X = result 
//  N = desired UIEyeDepth when at default Eye-To-Screen-Distance
//  DESD = default_eye_to_screen_dist
//  MESD = eye_to_screen_dist_where_X_should_be_zero
//  in the current case:
//  -14.285714 = 600 / ((0.041 - 0.083) * 1000)



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
    if ( gHMD.isPreDetectionInitialized())
        return TRUE;

    OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_None));

    mDeviceManager = *OVR::DeviceManager::Create();
    if (! mDeviceManager)
    {
        gHMD.isInitialized(FALSE);
        gHMD.isPreDetectionInitialized(FALSE);
        return false;
    }

    mDeviceManager->SetMessageHandler(this);

    mHMD = *mDeviceManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    if (! mHMD)
    {
        gHMD.isPreDetectionInitialized(TRUE); // consider ourselves pre-initialized if we get here
        gHMD.isInitialized(FALSE);
        return false;
    }

    if (mHMD)
    {
        mHMD->GetDeviceInfo(&mHMDInfo);
        mDisplayName = utf8str_to_utf16str(mHMDInfo.DisplayDeviceName);
        mDisplayId = mHMDInfo.DisplayId;
        mStereoConfig.SetHMDInfo(mHMDInfo);
        mHMDConnected = true;
    }

    mSensorDevice = 0;
    
    gHMD.isInitialized(TRUE);
    gHMD.isPreDetectionInitialized(TRUE); 

    return true;
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
                break;
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
                        if (! mSensorDevice)
                        {
                            mSensorDevice = *desc.Handle.CreateDeviceTyped<OVR::SensorDevice>();
                            mSensorFusion.AttachToSensor(mSensorDevice);
                            mSensorFusion.SetDelegateMessageHandler(this);
                            mSensorFusion.SetPredictionEnabled(true);
                        }
                        else
                        if (! was_already_created )
                        {
                            // A new sensor has been detected, but it is not currently used.
                        }
                    }
                    break;

                case OVR::Device_HMD:
                {
                    OVR::HMDInfo info;
                    desc.Handle.GetDeviceInfo(&info);

                    if (strlen(info.DisplayDeviceName) > 0 && (! mHMD || ! info.IsSameDisplay(mHMDInfo)))
                    {
                        if ( ! mHMD || !desc.Handle.IsDevice(mHMD))
                        {
                            mHMD = *desc.Handle.CreateDeviceTyped<OVR::HMDDevice>();
                        }

                        if (mHMD && mHMD->GetDeviceInfo(&mHMDInfo))
                        {
                            mStereoConfig.SetHMDInfo(mHMDInfo);
                            mDisplayName = utf8str_to_utf16str(mHMDInfo.DisplayDeviceName);
                            mDisplayId = mHMDInfo.DisplayId;
                        }

                        mHMDConnected = true;
                    }
                    break;
                }
                default:
                {
                }
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
                mHMDConnected = false;

                if (mHMD && ! mHMD->IsDisconnected())
                {
                    mHMD = mHMD->Disconnect(mSensorDevice);

                    if (mHMD && mHMD->GetDeviceInfo(&mHMDInfo))
                    {
                        mStereoConfig.SetHMDInfo(mHMDInfo);
                        mDisplayName = utf8str_to_utf16str(mHMDInfo.DisplayDeviceName);
                        mDisplayId = mHMDInfo.DisplayId;
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
                {
                    // DeviceManager reported unknown action.
                }
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
    LLWindow* pWin = gViewerWindow->getWindow();
    pWin->getDisplayInfo(mDisplayName, mDisplayId, mHMDRect, dummy);
    pWin->getRenderWindow(mainFullScreen);
    gHMD.isMainFullScreen(mainFullScreen);
    if (!pWin->initHMDWindow(mHMDRect.mLeft, mHMDRect.mTop, mHMDRect.mRight - mHMDRect.mLeft, mHMDRect.mBottom - mHMDRect.mTop))
    {
        gHMD.failedInit(TRUE);
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

    // load cursor textures
    {
        mCursorTextures.clear();
        //UI_CURSOR_ARROW  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_WAIT  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_waiting.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_HAND  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_hand.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_IBEAM  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_ibeam.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_CROSS  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_SIZENWSE  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_SIZENESW  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_SIZEWE  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_SIZENS  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_NO  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_WORKING  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLGRAB
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolgrab.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLLAND
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolland.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLFOCUS
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolfocus.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLCREATE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolcreate.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_ARROWDRAG
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowdrag.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_ARROWCOPY
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowcop.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_ARROWDRAGMULTI
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowdragmulti.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_ARROWCOPYMULTI
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowcopmulti.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_NOLOCKED
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llnolocked.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_ARROWLOCKED
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llarrowlocked.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_GRABLOCKED
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llgrablocked.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLTRANSLATE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltooltranslate.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLROTATE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolrotate.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLSCALE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolscale.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLCAMERA
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolcamera.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPAN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpan.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLZOOMIN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolzoomin.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPICKOBJECT3
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolpickobject3.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPLAY
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolplay.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPAUSE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolpause.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLMEDIAOPEN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolmediaopen.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_PIPETTE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolpipette.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLSIT
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolsit.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLBUY
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolbuy.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLOPEN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolopen.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPATHFINDING
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfinding.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPATHFINDING_PATH_START
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathstart.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathstartadd.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPATHFINDING_PATH_END
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathend.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathendadd.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_TOOLNO
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llno.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
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
    mCursorTextures.clear();

    // This causes a deadlock.  No idea why.   Disabling it as it doesn't seem to be necessary unless we're actually RE-initializing the HMD
    // without shutting down.   For now, to init HMD, we have to restart the viewer.
    //OVR::System::Destroy();
}


void LLHMDImpl::onIdle()
{
    static LLCachedControl<bool> debug_hmd(gSavedSettings, "DebugHMDEnable");
    if ( ! debug_hmd && ! gHMD.shouldRender() )
    {
        return;
    }

    if ( ! gHMD.isPreDetectionInitialized() )
    {
        return;
    }

    handleMessages();

    // still waiting for device to initialize
    if (! isReady())
    {
        return;
    }

    if ( ! gHMD.isPostDetectionInitialized())
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
    // We extract Yaw, Pitch, Roll instead of directly using the orientation
    // to allow "additional" yaw manipulation with mouse/controller.
    if (mSensorDevice)
    {
        OVR::Quatf orient = useMotionPrediction() ? mSensorFusion.GetPredictedOrientation() : mSensorFusion.GetOrientation();
        orient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&mEyeYaw, &mEyePitch, &mEyeRoll);
        mEyeRoll = -mEyeRoll;
        mEyePitch = -mEyePitch;
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
            LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
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
            LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
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
            LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
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
            LL_INFOS("Oculus") << "Magnetometer Calibration\n" << mCalibrationText << LL_ENDL;
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
            Vector3f mc = mMagCal.GetMagCenter();
            LL_INFOS("Oculus") << "Magnetometer Calibration Complete   \nCenter: " << mc.x << "," << mc.y << "," << mc.z << LL_ENDL;
            mLastCalibrationStep = 4;
            gHMD.isCalibrated(TRUE);
            mCalibrationText = "";

            setCurrentEye(OVR::Util::Render::StereoEye_Left);
            LL_INFOS("Oculus") << "Left Eye:" << LL_ENDL;
            LL_INFOS("Oculus") << "HScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenWidth() << ", default: " << LLHMDImpl::kDefaultHScreenSize << LL_ENDL;
            LL_INFOS("Oculus") << "VScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenHeight() << ", default: " << LLHMDImpl::kDefaultVScreenSize << LL_ENDL;
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
            LL_INFOS("Oculus") << "HScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenWidth() << ", default: " << LLHMDImpl::kDefaultHScreenSize << LL_ENDL;
            LL_INFOS("Oculus") << "VScreenSize: " << std::fixed << std::setprecision(4) << getPhysicalScreenHeight() << ", default: " << LLHMDImpl::kDefaultVScreenSize << LL_ENDL;
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
// LLHMD
/////////////////////////////////////////////////////////////////////////////////////////////////////

LLHMD gHMD;

LLHMD::LLHMD()
    : mRenderMode(RenderMode_None)
    , mMainWindowWidth(LLHMD::kHMDWidth)
    , mMainWindowHeight(LLHMD::kHMDHeight)
    , mOrthoPixelOffsetMult(1.0f)
    , mEyeDepth(0.075f)
    , mUIEyeDepth(0.6f)
    , mUIEyeDepthMod(0.0f)
{
    mImpl = new LLHMDImpl();
}


LLHMD::~LLHMD()
{
    if (gHMD.isInitialized())
    {
        delete mImpl;
    }
}


BOOL LLHMD::init()
{
    gSavedSettings.getControl("OculusUISurfaceArc")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
    gSavedSettings.getControl("OculusUISurfaceToroidRadius")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
    gSavedSettings.getControl("OculusUISurfaceCrossSectionRadius")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
    gSavedSettings.getControl("OculusUISurfaceOffsets")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
    onChangeUISurfaceShape();
    gSavedSettings.getControl("OculusOrthoPixelOffsetMult")->getSignal()->connect(boost::bind(&onChangeOrthoPixelOffsetMult));
    onChangeOrthoPixelOffsetMult();
    gSavedSettings.getControl("OculusEyeDepth")->getSignal()->connect(boost::bind(&onChangeEyeDepth));
    onChangeEyeDepth();
    gSavedSettings.getControl("OculusUIEyeDepth")->getSignal()->connect(boost::bind(&onChangeUIEyeDepth));
    onChangeUIEyeDepth();

    return mImpl->preInit();    
}

void LLHMD::onChangeOrthoPixelOffsetMult() { gHMD.mOrthoPixelOffsetMult = gSavedSettings.getF32("OculusOrthoPixelOffsetMult"); }
void LLHMD::onChangeEyeDepth() { gHMD.mEyeDepth = gSavedSettings.getF32("OculusEyeDepth") * .001f; }
void LLHMD::onChangeUIEyeDepth() { gHMD.mUIEyeDepthMod = gSavedSettings.getF32("OculusUIEyeDepth") * .001f; }
void LLHMD::onChangeUISurfaceShape()
{
    gHMD.mUICurvedSurfaceArc.set(gSavedSettings.getVector3("OculusUISurfaceArc").mV);
    gHMD.mUICurvedSurfaceArc *= F_PI;
    LLVector3 tr = gSavedSettings.getVector3("OculusUISurfaceToroidRadius");
    LLVector3 csr = gSavedSettings.getVector3("OculusUISurfaceCrossSectionRadius");
    gHMD.mUICurvedSurfaceRadius.set(tr[0], tr[1], csr[1], csr[0]);
    gHMD.mUICurvedSurfaceOffsets = gSavedSettings.getVector3("OculusUISurfaceOffsets");
    gPipeline.mOculusUISurface = NULL;
}

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
        BOOL oldShouldRender = shouldRender();
        mRenderMode = newRenderMode;
        if (!oldShouldRender && shouldRender())
        {
            if (gAgentCamera.cameraMouselook())
            {
                gAgentCamera.changeCameraToFirstPerson();
            }
            LLCoordWindow windowSize;
            gViewerWindow->getWindow()->getSize(&windowSize);
            mMainWindowWidth = windowSize.mX;
            mMainWindowHeight = windowSize.mY;
            gViewerWindow->reshape(LLHMD::kHMDEyeWidth, LLHMD::kHMDHeight);
        }
        else if (oldShouldRender && !shouldRender())
        {
            if (gAgentCamera.cameraFirstPerson())
            {
                gAgentCamera.changeCameraToMouselook();
            }
            gViewerWindow->reshape(mMainWindowWidth, mMainWindowHeight);
        }
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
    BOOL shouldRender = gHMD.shouldRender();
    gViewerWindow->getWindow()->setFocusWindow(0, shouldRender, shouldRender ? LLHMD::kHMDUIWidth : 0, shouldRender ? LLHMD::kHMDUIHeight : 0);
    if (gHMD.shouldRender())
    {
        gViewerWindow->hideCursor();
    }
    else
    {
        gViewerWindow->showCursor();
    }
}

void LLHMD::setFocusWindowHMD()
{
    gViewerWindow->getWindow()->setFocusWindow(1, TRUE, LLHMD::kHMDUIWidth, LLHMD::kHMDUIHeight);
    gViewerWindow->hideCursor();
}

U32 LLHMD::getCurrentEye() const { return mImpl->getCurrentEye(); }
void LLHMD::setCurrentEye(U32 eye) { mImpl->setCurrentEye(eye); }
void LLHMD::getViewportInfo(S32& x, S32& y, S32& w, S32& h) { mImpl->getViewportInfo(x, y, w, h); }
F32 LLHMD::getPhysicalScreenWidth() const { return mImpl->getPhysicalScreenWidth(); }
F32 LLHMD::getPhysicalScreenHeight() const { return mImpl->getPhysicalScreenHeight(); }
F32 LLHMD::getInterpupillaryOffset() const { return mImpl->getInterpupillaryOffset(); }
void LLHMD::setInterpupillaryOffset(F32 f) { mImpl->setInterpupillaryOffset(f); }
F32 LLHMD::getInterpupillaryOffsetDefault() const { return mImpl->getInterpupillaryOffsetDefault(); }
F32 LLHMD::getLensSeparationDistance() const { return mImpl->getLensSeparationDistance(); }
F32 LLHMD::getEyeToScreenDistance() const { return mImpl->getEyeToScreenDistance(); }
F32 LLHMD::getEyeToScreenDistanceDefault() const { return mImpl->getEyeToScreenDistanceDefault(); }
void LLHMD::setEyeToScreenDistance(F32 f) { mImpl->setEyeToScreenDistance(f); }
F32 LLHMD::getUIEyeDepth() { return (isUIEyeDepthCalculated() ? mUIEyeDepth : calculateUIEyeDepth()) + mUIEyeDepthMod; }
LLVector4 LLHMD::getDistortionConstants() const { return mImpl->getDistortionConstants(); }
F32 LLHMD::getXCenterOffset() const { return mImpl->getXCenterOffset(); }
F32 LLHMD::getYCenterOffset() const { return mImpl->getYCenterOffset(); }
F32 LLHMD::getDistortionScale() const { return mImpl->getDistortionScale(); }
LLQuaternion LLHMD::getHMDOrient() const { return mImpl->getHMDOrient(); }
LLQuaternion LLHMD::getHeadRotationCorrection() const { return mImpl->getHeadRotationCorrection(); }
void LLHMD::addHeadRotationCorrection(LLQuaternion quat) { return mImpl->addHeadRotationCorrection(quat); }
void LLHMD::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { mImpl->getHMDRollPitchYaw(roll, pitch, yaw); }
F32 LLHMD::getVerticalFOV() const { return mImpl->getVerticalFOV(); }
BOOL LLHMD::useMotionPrediction() const { return mImpl->useMotionPrediction(); }
void LLHMD::useMotionPrediction(BOOL b) { mImpl->useMotionPrediction(b); }
BOOL LLHMD::useMotionPredictionDefault() const { return mImpl->useMotionPredictionDefault(); }
F32 LLHMD::getMotionPredictionDelta() const { return mImpl->getMotionPredictionDelta(); }
F32 LLHMD::getMotionPredictionDeltaDefault() const { return mImpl->getMotionPredictionDeltaDefault(); }
void LLHMD::setMotionPredictionDelta(F32 f) { mImpl->setMotionPredictionDelta(f); }
//LLVector4 LLHMD::getChromaticAberrationConstants() const { return mImpl->getChromaticAberrationConstants(); }
F32 LLHMD::getOrthoPixelOffset() const { return mImpl->getOrthoPixelOffset() * mOrthoPixelOffsetMult; }
void LLHMD::BeginManualCalibration() { isCalibrated(FALSE); mImpl->BeginManualCalibration(); }
const std::string& LLHMD::getCalibrationText() const { return mImpl->getCalibrationText(); }
LLViewerTexture* LLHMD::getCursorImage(U32 cursorType) { return mImpl->getCursorImage(cursorType); }

F32 LLHMD::calculateUIEyeDepth()
{
    mUIEyeDepth = ((mImpl->getEyeToScreenDistance() - 0.083f) * LLHMDImpl::kUIEyeDepthMult);
    gHMD.isUIEyeDepthCalculated(TRUE);
    return mUIEyeDepth;
}


// Creates a surface that is part of an outer shell of a torus.
// Results are in local-space with -z forward, y up (i.e. standard OpenGL)
// The center of the toroid section (assuming that xa and ya are centered at 0), will
// be at [offset[X], offset[Y], offset[Z] - (r[0] + r[2])]
//
// mUICurvedSurfaceArc = how much arc of the toroid surface is used
//     [X] = length of horizontal arc
//     [Y] = length of vertical arc (all in radians)
// mUICurvedSurfaceRadius = radius of toroid.
//     [0] = Center of toroid to center of cross-section (width)
//     [1] = Center of toroid to center of cross-section (depth)
//     [2] = cross-section vertical (height)
//     [3] = cross-section horizontal (width/depth)
// mUICurvedSurfaceOffsets = offsets to add to final coordinates (x,y,z)
LLVertexBuffer* LLHMD::createUISurface()
{
    LLVertexBuffer* buff = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, GL_STATIC_DRAW_ARB);
    U32 resX = 32;
    U32 resY = 16;
    U32 numVerts = resX * resY;
    U32 numIndices = 6 * (resX - 1) * (resY - 1);
    buff->allocateBuffer(numVerts, numIndices, true);
    LLStrider<LLVector4a> v;
    LLStrider<LLVector2> tc;
    buff->getVertexStrider(v);
    buff->getTexCoord0Strider(tc);
    F32 dx = mUICurvedSurfaceArc[VX] / ((F32)resX - 1.0f);
    F32 dy = mUICurvedSurfaceArc[VY] / ((F32)resY - 1.0f);
    //LL_INFOS("Oculus")  << "XA: [" << xa[0] << "," << xa[1] << "], "
    //                    << "YA: [" << ya[0] << "," << ya[1] << "], "
    //                    << "r: [" << r[0] << "," << r[1] << "], "
    //                    << "offsets: [" << offsets[0] << "," << offsets[1] << "," << offsets[2] << "]"
    //                    << LL_ENDL;
    //LL_INFOS("Oculus")  << "dX: " << dx << ", "
    //                    << "dY: " << dy
    //                    << LL_ENDL;

    LLVector4a t;
    LLVector2 uv;
    for (U32 i = 0; i < resY; ++i)
    {
        F32 va = (F_PI - (mUICurvedSurfaceArc[VY] * 0.5f)) + ((F32)i * dy);
        for (U32 j = 0; j < resX; ++j)
        {
            F32 ha = (mUICurvedSurfaceArc[VX] * -0.5f) + ((F32)j * dx);
            LLVector4 t2;
            getUISurfaceCoordinates(ha, va, t2, uv);
            t.loadua(t2.mV);
            *v++ = t;
            *tc++ = uv;
        }
    }

    LLStrider<U16> idx;
    buff->getIndexStrider(idx);
    for (S32 i = resY - 2; i >= 0; --i)
    {
        for (S32 j = 0; j < resX-1; ++j)
        {
            U16 cur_idx = (i * resX) + j;
            U16 right = cur_idx + 1;
            U16 bottom = cur_idx + resX;
            U16 bottom_right = bottom+1;
            *idx++ = cur_idx;
            *idx++ = bottom;
            *idx++ = right;
            *idx++ = right;
            *idx++ = bottom;
            *idx++ = bottom_right;
            //LL_INFOS("Oculus")  << "Vtx " << ((i * resX) + j) << " [" << i << "," << j << "]: "
            //                    << "[" << cur_idx << "," << bottom << "," << right << "], "
            //                    << "[" << right << "," << bottom << "," << bottom_right << "]"
            //                    << LL_ENDL;
        }
    }

    if (gSavedSettings.getBOOL("DebugHMDEnable"))
    {
        LLVector2 uv;
        F32 hha = mUICurvedSurfaceArc[VX] * 0.5f;
        F32 hva = mUICurvedSurfaceArc[VY] * 0.5f;

        LLVector4 ul;
        getUISurfaceCoordinates(-hha, F_PI + hva, ul, uv);
        LLVector4 cl;
        getUISurfaceCoordinates(-hha, F_PI, cl, uv);
        LLVector4 ll;
        getUISurfaceCoordinates(-hha, F_PI - hva, ll, uv);
        LLVector4 uc;
        getUISurfaceCoordinates(0.0f, F_PI + hva, uc, uv);
        LLVector4 cc;
        getUISurfaceCoordinates(0.0f, F_PI, cc, uv);
        LLVector4 lc;
        getUISurfaceCoordinates(0.0f, F_PI - hva, lc, uv);
        LLVector4 ur;
        getUISurfaceCoordinates(hha, F_PI + hva, ur, uv);
        LLVector4 cr;
        getUISurfaceCoordinates(hha, F_PI, cr, uv);
        LLVector4 lr;
        getUISurfaceCoordinates(hha, F_PI - hva, lr, uv);

        LL_INFOS("Oculus")  << "UISurface Bounds:" << LL_ENDL;
        LL_INFOS("Oculus")  << "UpperLeft : " << ul << ",\tUpperCenter: " << uc << ",\tUpperRight : " << ur << LL_ENDL;
        LL_INFOS("Oculus")  << "MiddleLeft: " << cl << ",\t     Center: " << cc << ",\tMiddleRight: " << cr << LL_ENDL;
        LL_INFOS("Oculus")  << "LowerLeft : " << ll << ",\tLowerCenter: " << lc << ",\tLowerRight : " << lr << LL_ENDL;
    }

    buff->flush();
    return buff;
}


void LLHMD::getUISurfaceCoordinates(F32 ha, F32 va, LLVector4& pos, LLVector2& uv)
{
    F32 cva = cos(va);
    pos.set(    (sin(ha) * (mUICurvedSurfaceRadius[0] - (cva * mUICurvedSurfaceRadius[3]))) + mUICurvedSurfaceOffsets[VX],
                (mUICurvedSurfaceRadius[2] * -sin(va)) + mUICurvedSurfaceOffsets[VY],
                (-1.0f * ((mUICurvedSurfaceRadius[1] * cos(ha)) - (cva * mUICurvedSurfaceRadius[3]))) + mUICurvedSurfaceOffsets[VZ],
                1.0f);
    uv.set((ha - (mUICurvedSurfaceArc[VX] * -0.5f))/mUICurvedSurfaceArc[VX], (va - (F_PI - (mUICurvedSurfaceArc[VY] * 0.5f)))/mUICurvedSurfaceArc[VY]);
}


BOOL LLHMD::getWorldMouseCoordinatesFromUIScreen(S32 ui_x, S32 ui_y, S32& world_x, S32& world_y)
{
    if (!shouldRender())
    {
        world_x = ui_x;
        world_y = ui_y;

        //LLViewerCamera* pCamera = LLViewerCamera::getInstance();
        //LLVector3 origin = pCamera->getOrigin();
        //LLMatrix4 m1 = pCamera->getModelview();
        //LLMatrix4 proj = pCamera->getProjection();

        //GLdouble x, y, z;
        //F64 mdlv[16];
        //F64 prj[16];
        //for (U32 i = 0; i < 16; i++)
        //{
        //    mdlv[i] = (F64)(*(((GLfloat*)m1.mMatrix) + i));
        //    prj[i] = (F64)(*(((GLfloat*)proj.mMatrix) + i));
        //}
        //GLint vp[4] = {0,0,gViewerWindow->getWorldViewWidthScaled(), gViewerWindow->getWorldViewHeightScaled()};
        //gluUnProject(
        //    GLdouble(world_x), GLdouble(world_y), 0.0,
        //    mdlv, prj, vp,
        //    &x, &y, &z );
        //LLVector4 worldPos(x, y, z, 1.0f);
        ////pos_agent->setVec( (F32)x, (F32)y, (F32)z );

        //LL_INFOS("Oculus") << "Mouse2D: [" << ui_x << "," << ui_y << "], Mouse3D [" << world_x << "," << world_y << std::setprecision(6) << "], origin: " << origin << ", worldPos: " << worldPos << LL_ENDL;
        ////LL_INFOS("Oculus") << "spp: " << screenSpacePos << ", Flat spp: " << scrPos << ", world: " << worldPos << LL_ENDL;

        return FALSE;
    }

    // 1. determine horizontal and vertical angle on toroid based on ui_x, ui_y
    F32 nx = llclamp((F32)ui_x / (F32)LLHMD::kHMDUIWidth, 0.0f, 1.0f);
    F32 ny = llclamp((F32)ui_y / (F32)LLHMD::kHMDUIHeight, 0.0f, 1.0f);
    F32 ha = ((mUICurvedSurfaceArc[VX] * -0.5f) + (nx * mUICurvedSurfaceArc[VX]));
    F32 va = (F_PI - (mUICurvedSurfaceArc[VY] * 0.5f)) + (ny * mUICurvedSurfaceArc[VY]);
    // 2. determine clip-space x,y,z for ha, va (-z forward/depth)
    LLVector4 screenSpacePos;
    LLVector2 uv;
    getUISurfaceCoordinates(ha, va, screenSpacePos, uv);
    // 3. Convert clip-space to screen coordinates (as if you were looking straight at the center of the UI surface)
    //LLMatrix4 t;
    //t.translate(LLVector3(0.0f, 0.0f, getUIEyeDepth()));
    LLMatrix4 m1(mUIModelView);
    //m1 *= t;
    LLMatrix4 proj(mBaseProjection);
    LLVector4 scrPos = screenSpacePos * proj;
    LLVector4 sp2(scrPos[VX] / scrPos[VZ], scrPos[VY] / scrPos[VZ], 0.0f, 1.0f);
    scrPos[VX] = ((sp2[VX] + 1.0f) * 0.5f) * (F32)kHMDUIWidth;
    scrPos[VY] = ((sp2[VY] + 1.0f) * 0.5f) * (F32)kHMDUIHeight;
    S32 intermediate_wx = llround(scrPos[VX]);
    S32 intermediate_wy = llround(scrPos[VY]);
    // 4. Find the world coordinates for those screen coordinates (still based on the looking-straight-at-UI-center viewpoint)
	GLdouble x, y, z;
	F64 mdlv[16];
	F64 prj[16];
	for (U32 i = 0; i < 16; i++)
	{
		mdlv[i] = (F64)(*(((GLfloat*)m1.mMatrix) + i));
		prj[i] = (F64)(*(((GLfloat*)proj.mMatrix) + i));
	}
    GLint vp[4] = {0,0,LLHMD::kHMDEyeWidth,LLHMD::kHMDHeight};
	gluUnProject(GLdouble(intermediate_wx), GLdouble(intermediate_wy), 0.5f, mdlv, prj, vp, &x, &y, &z);
    LLVector4 worldPos(x, y, z, 1.0f);
    // 5. Adjust for actual camera position to find clip-space coordinates of the actual position on the screen with the mouse cursor
    LLMatrix4 m2(mBaseModelViewInv);
    LLVector4 adjWorldPos = worldPos * m2;
    LLVector4 adjWp2 = adjWorldPos * proj;
    adjWp2 /= adjWp2[VW];
    LLMatrix3 m3 = m2.getMat3();
    m3.invert();
    LLVector3 adjScrPos((F32)intermediate_wx, (F32)intermediate_wy, 1.0f);
    adjScrPos = adjScrPos * m3;

    LL_INFOS("Oculus") << "Mouse2D: [" << ui_x << "," << ui_y << "], IntMouse3D [" << intermediate_wx << "," << intermediate_wy << "], Final2D: " << adjScrPos << LL_ENDL;
    //LL_INFOS("Oculus") << std::setprecision(6) << "spp: " << screenSpacePos << ", Flat spp: " << scrPos << LL_ENDL;
    LL_INFOS("Oculus") << std::setprecision(6) << "origin: " << LLViewerCamera::getInstance()->getOrigin() << ", world: " << worldPos << ", adjWorldPos: " << adjWorldPos << ", adjWp2: " << adjWp2 << LL_ENDL;

    return TRUE;
}
