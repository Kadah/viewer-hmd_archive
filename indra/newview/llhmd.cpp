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
#include "pipeline.h"
#include "llagentcamera.h"
#include "llviewertexture.h"

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

    F32 getPhysicalScreenWidth() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().HScreenSize : kDefaultHScreenSize; }
    F32 getPhysicalScreenHeight() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().VScreenSize : kDefaultVScreenSize; }
    F32 getInterpupillaryOffset() const { return gHMD.isInitialized() ? mStereoConfig.GetIPD() : getInterpupillaryOffsetDefault(); }
    F32 getInterpupillaryOffsetDefault() const { return kDefaultInterpupillaryOffset; }
    void setInterpupillaryOffset(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetIPD(f); } }
    F32 getLensSeparationDistance() const { return gHMD.isInitialized() ? mStereoConfig.GetHMDInfo().LensSeparationDistance : kDefaultLenSeparationDistance; }
    F32 getEyeToScreenDistance() const { return gHMD.isInitialized() ? mStereoConfig.GetEyeToScreenDistance() : getEyeToScreenDistanceDefault(); }
    F32 getEyeToScreenDistanceDefault() const { return kDefaultEyeToScreenDistance; }
    void setEyeToScreenDistance(F32 f) { if (gHMD.isInitialized()) { mStereoConfig.SetEyeToScreenDistance(f); } }

    //F32 getVerticalFOV() { return gHMD.isInitialized() ? mStereoConfig.GetYFOVRadians() : kDefaultVerticalFOVRadians; }
    F32 getVerticalFOV() { return kDefaultVerticalFOVRadians; }

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

    BOOL useMotionPrediction() { return gHMD.isInitialized() ? mSFusion.IsPredictionEnabled() : useMotionPredictionDefault(); }
    BOOL useMotionPredictionDefault() const { return TRUE; }
    void useMotionPrediction(BOOL b) { if (gHMD.isInitialized()) { mSFusion.SetPredictionEnabled(b); } }
    F32 getMotionPredictionDelta() { return gHMD.isInitialized() ? mSFusion.GetPredictionDelta() : getMotionPredictionDeltaDefault(); }
    F32 getMotionPredictionDeltaDefault() const { return 0.03f; }
    void setMotionPredictionDelta(F32 f) { if (gHMD.isInitialized()) { mSFusion.SetPrediction(f); } }

    LLQuaternion getHMDOrient() const
    {
        LLQuaternion q;
        q.setEulerAngles(mEyeRoll, mEyePitch, mEyeYaw);
        return q;
    }
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = mEyeRoll; pitch = mEyePitch; yaw = mEyeYaw; }

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

private:
    OVR::Util::MagCalibration mMagCal;
    SensorFusion mSFusion;
    Util::LatencyTest mLatencyUtil;
    OVR::Util::Render::StereoConfig mStereoConfig;
    OVR::Ptr<DeviceManager> mpDeviceMgr;
    OVR::Ptr<SensorDevice> mpSensor;
    OVR::Ptr<HMDDevice> mpHMD;
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
const F32 LLHMDImpl::kDefaultVerticalFOVRadians = 1.5707963f; // -0.1775f Right Eye


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
        mDisplayId = 0;
        mpHMD  = *(mpDeviceMgr->EnumerateDevices<OVR::HMDDevice>().CreateDevice());
        if (mpHMD)
        {
            mpSensor = *(mpHMD->GetSensor());

            // This will initialize HMDInfo with information about configured IPD,
            // screen size and other variables needed for correct projection.
            // We pass HMD DisplayDeviceName into the renderer to select the
            // correct monitor in full-screen mode.
            HMDInfo hmd_info;
            if (mpHMD->GetDeviceInfo(&hmd_info))
            {
                mDisplayName = utf8str_to_utf16str(hmd_info.DisplayDeviceName);
                mDisplayId = hmd_info.DisplayId;
                mStereoConfig.SetHMDInfo(hmd_info);
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
        detectionMessage += (mStereoConfig.GetHMDInfo().DisplayDeviceName[0] != '\0') ? "; HMD Display EDID detected" : "; HMD display EDID not detected";
        LL_INFOS("Oculus") << detectionMessage.c_str() << LL_ENDL;

        if (!mpHMD || !mpSensor || mStereoConfig.GetHMDInfo().DisplayDeviceName[0] == '\0')
        {
            // TODO: possibly put up a retry/cancel dialog here?
            LL_WARNS("Oculus") << detectionMessage << LL_ENDL;
            gHMD.failedInit(TRUE);
            return FALSE;
        }
    } while (detectionResult != IDCONTINUE);

    if (mStereoConfig.GetHMDInfo().HResolution == 0 || mStereoConfig.GetHMDInfo().VResolution == 0)
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
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_WAIT  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_HAND  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        //UI_CURSOR_IBEAM  (TODO: Need actual texture)
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
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
    mpSensor.Clear();
    mpHMD.Clear();
    mCursorTextures.clear();

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
        OVR::Quatf orient = useMotionPrediction() ? mSFusion.GetPredictedOrientation() : mSFusion.GetOrientation();
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
// LLHMD
/////////////////////////////////////////////////////////////////////////////////////////////////////

LLHMD gHMD;

LLHMD::LLHMD()
    : mXCenterOffsetMod(0.0f)
    , mRenderMode(RenderMode_None)
    , mMainWindowWidth(LLHMD::kHMDWidth)
    , mMainWindowHeight(LLHMD::kHMDHeight)
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
    
    if (gHMD.isInitialized())
    {
        delete mImpl;
    }
}


BOOL LLHMD::init()
{
    BOOL res = mImpl->init();
    //if (res)
    //{
	    gSavedSettings.getControl("OculusXCenterOffsetModifier")->getSignal()->connect(boost::bind(&onChangeXCenterOffsetModifier));
        onChangeXCenterOffsetModifier();
        gSavedSettings.getControl("OculusOptWindowRaw")->getSignal()->connect(boost::bind(&onChangeWindowRaw));
        onChangeWindowRaw();
        gSavedSettings.getControl("OculusOptWindowScaled")->getSignal()->connect(boost::bind(&onChangeWindowScaled));
        onChangeWindowScaled();
        gSavedSettings.getControl("OculusOptWorldViewRaw")->getSignal()->connect(boost::bind(&onChangeWorldViewRaw));
        onChangeWorldViewRaw();
        gSavedSettings.getControl("OculusOptWorldViewScaled")->getSignal()->connect(boost::bind(&onChangeWorldViewScaled));
        onChangeWorldViewScaled();
        gSavedSettings.getControl("OculusTestCalibration")->getSignal()->connect(boost::bind(&onChangeTestCalibration));
        onChangeTestCalibration();
        gSavedSettings.getControl("Oculus2DUICurvedSurface")->getSignal()->connect(boost::bind(&onChangeRender2DUICurvedSurface));
        onChangeRender2DUICurvedSurface();
        gSavedSettings.getControl("OculusUISurfaceFudge")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
        gSavedSettings.getControl("OculusUISurfaceX")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
        gSavedSettings.getControl("OculusUISurfaceY")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
        gSavedSettings.getControl("OculusUISurfaceRadius")->getSignal()->connect(boost::bind(&onChangeUISurfaceShape));
        onChangeUISurfaceShape();
        gSavedSettings.getControl("OculusUIFlatSurfaceScale")->getSignal()->connect(boost::bind(&onChangeUIFlatSurfaceScale));
        onChangeUIFlatSurfaceScale();
    //}
    return res;
}
void LLHMD::onChangeXCenterOffsetModifier() { gHMD.mXCenterOffsetMod = gSavedSettings.getF32("OculusXCenterOffsetModifier") * 0.1f; }
void LLHMD::onChangeWindowRaw() { gHMD.mOptWindowRaw = gSavedSettings.getS32("OculusOptWindowRaw"); }
void LLHMD::onChangeWindowScaled() { gHMD.mOptWindowScaled = gSavedSettings.getS32("OculusOptWindowScaled"); }
void LLHMD::onChangeWorldViewRaw() { gHMD.mOptWorldViewRaw = gSavedSettings.getS32("OculusOptWorldViewRaw"); }
void LLHMD::onChangeWorldViewScaled() { gHMD.mOptWorldViewScaled = gSavedSettings.getS32("OculusOptWorldViewScaled"); }
void LLHMD::onChangeTestCalibration() { gHMD.shouldShowDepthUI(gSavedSettings.getBOOL("OculusTestCalibration")); }
void LLHMD::onChangeRender2DUICurvedSurface() { gHMD.shouldRender2DUICurvedSurface(gSavedSettings.getBOOL("Oculus2DUICurvedSurface")); }
void LLHMD::onChangeUIFlatSurfaceScale() { gHMD.mUIFlatSurfaceScale = gSavedSettings.getVector3("OculusUIFlatSurfaceScale"); }

void LLHMD::onChangeUISurfaceShape()
{
    gHMD.mUISurface_Fudge = gSavedSettings.getF32("OculusUISurfaceFudge");
    gHMD.mUISurface_R = gSavedSettings.getF32("OculusUISurfaceRadius");
    LLVector3 csx = gSavedSettings.getVector3("OculusUISurfaceX");
    LLVector3 csy = gSavedSettings.getVector3("OculusUISurfaceY");
    gHMD.mUICurvedSurfaceX.set(F_PI * (csx[VX] - (csx[VY] * csx[VZ])), F_PI * (csx[VX] + (csx[VY] * (1.0f - csx[VZ]))));
    gHMD.mUICurvedSurfaceY.set(F_PI * (csy[VX] - (csy[VY] * csy[VZ])), F_PI * (csy[VX] + (csy[VY] * (1.0f - csy[VZ]))));
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
    gViewerWindow->getWindow()->setFocusWindow(0, gHMD.shouldRender());
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
    gViewerWindow->getWindow()->setFocusWindow(1, TRUE);
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
LLVector4 LLHMD::getDistortionConstants() const { return mImpl->getDistortionConstants(); }
F32 LLHMD::getXCenterOffset() const { return mXCenterOffsetMod + mImpl->getXCenterOffset(); }
F32 LLHMD::getYCenterOffset() const { return mImpl->getYCenterOffset(); }
F32 LLHMD::getDistortionScale() const { return mImpl->getDistortionScale(); }
LLQuaternion LLHMD::getHMDOrient() const { return mImpl->getHMDOrient(); }
void LLHMD::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { mImpl->getHMDRollPitchYaw(roll, pitch, yaw); }
F32 LLHMD::getVerticalFOV() const { return mImpl->getVerticalFOV(); }
BOOL LLHMD::useMotionPrediction() const { return mImpl->useMotionPrediction(); }
void LLHMD::useMotionPrediction(BOOL b) { mImpl->useMotionPrediction(b); }
BOOL LLHMD::useMotionPredictionDefault() const { return mImpl->useMotionPredictionDefault(); }
F32 LLHMD::getMotionPredictionDelta() const { return mImpl->getMotionPredictionDelta(); }
F32 LLHMD::getMotionPredictionDeltaDefault() const { return mImpl->getMotionPredictionDeltaDefault(); }
void LLHMD::setMotionPredictionDelta(F32 f) { mImpl->setMotionPredictionDelta(f); }
//LLVector4 LLHMD::getChromaticAberrationConstants() const { return mImpl->getChromaticAberrationConstants(); }
F32 LLHMD::getOrthoPixelOffset() const { return mImpl->getOrthoPixelOffset(); }
LLViewerTexture* LLHMD::getCursorImage(U32 cursorType) { return mImpl->getCursorImage(cursorType); }
