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

#if LL_HMD_SUPPORTED

#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llview.h"
#include "llviewerdisplay.h"
#include "pipeline.h"
#include "llrendertarget.h"
#if LL_WINDOWS
    #include "llwindowwin32.h"
#elif LL_DARWIN
    #include "llwindowmacosx.h"
    #define IDCONTINUE 1        // Exist on Windows along "IDOK" and "IDCANCEL" but not on Mac
#endif

#include "CAPI/CAPI_HMDState.h"


LLHMDImplOculus::LLHMDImplOculus()
    //: mDeviceManager(NULL)
    : mHMD(NULL)
    //, mLastUpdate(0.0)
    , mFovSideTanLimit(0.0f)
    , mFovSideTanMax(0.0f)
    , mTrackingCaps(0)
    , mFPS(0.0f)
    , mSecondsPerFrame(0.0f)
    , mFrameCounter(0)
    , mTotalFrameCounter(0)
    , mLastFpsUpdate(0.0)
    , mLastTimewarpUpdate(0.0)
    , mCurrentHMDCount(0)
    //, mSensorFusion(NULL)
    //, mSensorDevice(NULL)
    //, mHeadRotationCorrection(LLQuaternion::DEFAULT)
    //, mHeadPitchCorrection(LLQuaternion::DEFAULT)
    //, mpDeviceStatusNotificationsQueue(NULL)
    //, mpLatencyTester(NULL)
    , mCurrentEye(LLHMD::CENTER_EYE)
{
    OVR::WorldAxes axesOculus(OVR::Axis_Right, OVR::Axis_Up, OVR::Axis_Out);
    OVR::WorldAxes axesLL(OVR::Axis_In, OVR::Axis_Left, OVR::Axis_Up);
    mConvOculusToLL = OVR::Matrix4f::AxisConversion(axesLL, axesOculus);
    mConvLLToOculus = OVR::Matrix4f::AxisConversion(axesOculus, axesLL);
    mEyeRPY[ovrEye_Left].set(0.0f, 0.0f, 0.0f);
    mEyeRPY[ovrEye_Right].set(0.0f, 0.0f, 0.0f);
    mEyePos[ovrEye_Left].set(0.0f, 0.0f, 0.0f);
    mEyePos[ovrEye_Right].set(0.0f, 0.0f, 0.0f);
    mEyeRT[LLHMD::CENTER_EYE] = NULL;
    mEyeRT[LLHMD::LEFT_EYE] = &gPipeline.mLeftEye;
    mEyeRT[LLHMD::RIGHT_EYE] = &gPipeline.mRightEye;
}


LLHMDImplOculus::~LLHMDImplOculus()
{
    shutdown();
}


BOOL LLHMDImplOculus::preInit()
{
    BOOL init = gHMD.isPreDetectionInitialized();
    if (!init && !gHMD.failedInit())
    {
        // Initializes LibOVR, and the Rift
        init = (BOOL)ovr_Initialize();
        gHMD.isPreDetectionInitialized(init);
        if (init)
        {
            LL_INFOS("HMD") << "HMD Preinit successful" << LL_ENDL;
            // note:  though initHMDDevice returns a success value, we ignore it here since all we care about is if the pre-init succeeded.  Initializing
            // an HMD device here is just for convenience and success or failure to detect a device does not affect the status of pre-init.
            initHMDDevice();
        }
    }
    return init;
    //if (!gHMD.isPreDetectionInitialized())
    //{
    //    return FALSE;
    //}
    //mHMD = ovrHmd_Create(0);
    //if (!mHMD && gHMD.isAdvancedMode())
    //{
    //    // no Rift device detected, but create one for debugging anyway
    //    mHMD = ovrHmd_CreateDebug(ovrHmd_DK2);
    //    if (mHMD)
    //    {
    //        gHMD.isUsingDebugHMD(TRUE);
    //    }
    //}
    //OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_None));

    //mSensorDevice = NULL;
    //mSensorFusion = new OVR::SensorFusion(NULL);
    //mpDeviceStatusNotificationsQueue = new OVR::Array<DeviceStatusNotificationDesc>();
    
    //mDeviceManager = *OVR::DeviceManager::Create();
    //if (!mDeviceManager)
    //{
    //    LL_INFOS("HMD") << "HMD Preinit abort: could not create Oculus Rift HMD device manager" << LL_ENDL;
    //    gHMD.isPreDetectionInitialized(FALSE);
    //    return FALSE;
    //}

    //mDeviceManager->SetMessageHandler(this);

    //mHMD = *mDeviceManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    //initHMDDevice();
    //mpLatencyTester = *(mDeviceManager->EnumerateDevices<OVR::LatencyTestDevice>().CreateDevice());
    //initHMDLatencyTester();

    // consider ourselves pre-initialized if we get here
    //LL_INFOS("HMD") << "HMD Preinit successful" << LL_ENDL;

    //gHMD.isPreDetectionInitialized(TRUE);
    //return TRUE;
}


BOOL LLHMDImplOculus::initHMDDevice()
{
    if (!mHMD)
    {
        mHMD = ovrHmd_Create(0);
        if (!mHMD && gHMD.isAdvancedMode())
        {
            // no Rift device detected, but create one for debugging anyway
            mHMD = ovrHmd_CreateDebug(ovrHmd_DK2);
            if (mHMD)
            {
                gHMD.isUsingDebugHMD(TRUE);
            }
        }
    }
    gHMD.isHMDConnected(mHMD && (mHMD->HmdCaps & ovrHmdCap_Present) != 0);
    gHMD.isHMDSensorConnected(mHMD && (mHMD->HmdCaps & (ovrHmdCap_Available | ovrHmdCap_Captured)) != 0);
    gHMD.isHMDDisplayEnabled(mHMD && mHMD->ProductName && mHMD->ProductName[0] != 0);
    gHMD.isUsingAppWindow(mHMD && (mHMD->HmdCaps & ovrHmdCap_ExtendDesktop) == 0);
    //mStereoConfig.SetStereoMode(OVR::Util::Render::StereoConfig::Stereo_LeftRight_Multipass);
    // if mHMD is gone and we've already created a HMDWindow, then destroy it.
    if (!mHMD && gHMD.isPostDetectionInitialized())
    {
        removeHMDDevice();
    }
    else if (mHMD)
    {
        gHMD.renderSettingsChanged(TRUE);
    }
    BOOL res = mHMD != NULL && !gHMD.isUsingDebugHMD();
    mCurrentHMDCount = res ? 1 : 0;
    return res;
}


void LLHMDImplOculus::removeHMDDevice()
{
    if (gHMD.isHMDMode())
    {
        gHMD.setRenderMode(LLHMD::RenderMode_None);
    }
    if (mHMD)
    {
        ovrHmd_Destroy(mHMD);
        mHMD = NULL;
    }
    if (gHMD.isPostDetectionInitialized() && !gHMD.isUsingAppWindow())
    {
        gViewerWindow->getWindow()->destroyHMDWindow();
    }
    gHMD.releaseAllEyeRT();
    gHMD.isPostDetectionInitialized(FALSE);
    gHMD.isUsingDebugHMD(FALSE);
    gHMD.isHMDConnected(FALSE);
    gHMD.isHMDSensorConnected(FALSE);
    gHMD.isHMDMirror(FALSE);
    gHMD.isHMDDisplayEnabled(FALSE);
    gHMD.isUsingAppWindow(FALSE);
    gHMD.isPositionTrackingEnabled(FALSE);
    mCurrentHMDCount = 0;
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
    //const OVR::HMDInfo& info = mStereoConfig.GetHMDInfo();
    BOOL isMirror = FALSE;
#if defined(LL_WINDOWS) || defined(LL_DARWIN)
    if (gHMD.isUsingAppWindow())
    {
        isMirror = TRUE;
        ovrHmd_AttachToWindow(mHMD, pWin->getPlatformWindow(), NULL, NULL);
    }
    else
#endif
#if LL_WINDOWS
    if (!pWin->initHMDWindow(mHMD->WindowsPos.x, mHMD->WindowsPos.y, mHMD->Resolution.w, mHMD->Resolution.h, isMirror))
#elif LL_DARWIN
    if (!pWin->initHMDWindow(info.DisplayId, 0, mHMD->Resolution.w, mHMD->Resolution.h, isMirror))
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

    setCurrentEye(OVR::StereoEye_Center);

    LL_INFOS("HMD") << "HMD Post-Detection Init Success" << LL_ENDL;

    return TRUE;
}


void LLHMDImplOculus::shutdown()
{
    if (!gHMD.isPreDetectionInitialized())
    {
        return;
    }
    removeHMDDevice();
    ovr_Shutdown();
    mEyeRT[0] = mEyeRT[1] = mEyeRT[2] = NULL;

    // make sure if/when we call shutdown again, we don't try to deallocate things twice.
    gHMD.isPreDetectionInitialized(FALSE);
}


void LLHMDImplOculus::onIdle()
{
    if (!gHMD.isPreDetectionInitialized())
    {
        return;
    }

    //if (!mHMD || gHMD.isUsingDebugHMD())
    {
        // for some unknown reason, Oculus completely got rid of their Message system that would notify when a device was attached or not, so now we have to poll
        // every frame to see if the status has changed.  WTF?
        // Since trying to actually create an HMD device is the only way to actually test whether anything is connected, and I'm pretty sure that's too slow to just
        // call every frame, we're basically forced to restart the viewer to detect an HMD or re-detect only when the user tells us to.  Grrrr.  WHY, Oculus, WHYYYYYY???
        // TODO: as a temporary hack, every second or so, call ovrHmd_Detect() and compare the result against the previous.  If they differ, then an HMD has been
        //       attached/detached and we then call ovrHmd_Create or ovrHmdDestroy
        static const U32 kFramePollInterval = 100;
        if (gFrameCount > 0 && (gFrameCount % kFramePollInterval) == 0)
        {
            // clamp value to [0,1] as we only care about the first HMD connected
            S32 numHMD = llmax(llmin(ovrHmd_Detect(), 0), 1);
            if (numHMD != mCurrentHMDCount)
            {
                removeHMDDevice();
                if (numHMD > 0)
                {
                    initHMDDevice();
                }
            }
        }
    }
    if (mHMD)
    {
        bool wasHMDConnected = gHMD.isHMDConnected();
        //bool wasHMDSensorConnected = gHMD.isHMDSensorConnected();
        gHMD.isHMDConnected(mHMD && (mHMD->HmdCaps & ovrHmdCap_Present) != 0);
        //gHMD.isHMDSensorConnected(mHMD && (mHMD->HmdCaps & (ovrHmdCap_Available | ovrHmdCap_Captured)) != 0);
        // ovrHmdCap_Available and ovrHmdCap_Captured don't seem to be getting set by the SDK, ignoring for now
        gHMD.isHMDSensorConnected(mHMD && (mHMD->HmdCaps & ovrHmdCap_Present) != 0);
        gHMD.isHMDDisplayEnabled(mHMD && mHMD->ProductName && mHMD->ProductName[0] != 0);
        //if ((!wasHMDConnected && gHMD.isHMDConnected()) || (!wasHMDSensorConnected && gHMD.isHMDSensorConnected()))
        //{
        //    LL_INFOS("HMD") << "HMD Device Added" << LL_ENDL;
        //    initHMDDevice();
        //}
        //else
        if ((wasHMDConnected && !gHMD.isHMDConnected())
            //|| (wasHMDSensorConnected && !gHMD.isHMDSensorConnected())
            )
        {
            LL_INFOS("HMD") << "HMD Device Not Detected" << LL_ENDL;
            if (gHMD.isHMDMode())
            {
                gHMD.setRenderMode(LLHMD::RenderMode_None);
            }
        }
    }

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

    if (gHMD.renderSettingsChanged())
    {
        if (!calculateViewportSettings())
        {
            return;
        }
    }

    if (gHMD.isHMDMode())
    {
        // This is a weird place to call this, but unfortunately, with the new OVR SDK, you cannot call the tracking functions
        // unless you've called ovrHmd_BeginFrame.   Since the orientation values are used for camera positioning, etc, we have
        // to call beginFrame before we do that even though this probably messes up the OVR frame timer a bit and makes frames
        // seem to be taking longer to render than they actually are.  Ideally, this should be at the top of
        // LLViewerDisplay::display() to get correct timing.  Oh well.
        gHMD.beginFrame();
    }
}


LLVector4 LLHMDImplOculus::getDistortionConstants() const
{
//    if (gHMD.isPostDetectionInitialized())
//    {
//        return LLVector4(   mCurrentEyeParams.pDistortion->K[0],
//                            mCurrentEyeParams.pDistortion->K[1],
//                            mCurrentEyeParams.pDistortion->K[2],
//                            mCurrentEyeParams.pDistortion->K[3]);
//    }
//    else
    {
        return LLVector4(   kDefaultDistortionConstant0,
                            kDefaultDistortionConstant1,
                            kDefaultDistortionConstant2,
                            kDefaultDistortionConstant3);
    }
}


BOOL LLHMDImplOculus::calculateViewportSettings()
{
    llassert(mHMD);

    // Initialize FovSideTanMax, which allows us to change all Fov sides at once - Fov
    // starts at default and is clamped to this value.
    mFovSideTanLimit = OVR::FovPort::Max(mHMD->MaxEyeFov[ovrEye_Left], mHMD->MaxEyeFov[ovrEye_Right]).GetMaxSideTan();
    mFovSideTanMax = OVR::FovPort::Max(mHMD->DefaultEyeFov[ovrEye_Left], mHMD->DefaultEyeFov[ovrEye_Right]).GetMaxSideTan();

    // Initialize eye rendering information for ovrHmd_Configure.
    // The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
    // Clamp Fov based on our dynamically adjustable FovSideTanMax.
    // Most apps should use the default, but reducing Fov does reduce rendering cost.
    ovrFovPort eyeFov[ovrEye_Count];
    eyeFov[ovrEye_Left] = OVR::FovPort::Min(mHMD->DefaultEyeFov[ovrEye_Left], OVR::FovPort::FovPort(mFovSideTanMax));
    eyeFov[ovrEye_Right] = OVR::FovPort::Min(mHMD->DefaultEyeFov[ovrEye_Right], OVR::FovPort::FovPort(mFovSideTanMax));

    // Configure Stereo settings. Default pixel density is 1.0f.
    float desiredPixelDensity = llmin(1.5f, llmax(0.5f, gSavedSettings.getF32("HMDPixelDensity")));
    OVR::Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(mHMD, ovrEye_Left,  eyeFov[ovrEye_Left], desiredPixelDensity);
    OVR::Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(mHMD, ovrEye_Right, eyeFov[ovrEye_Right], desiredPixelDensity);

    OVR::Sizei tex0Size = OVR::Sizei::Max(OVR::Sizei::Min(recommenedTex0Size, OVR::Sizei(4096)), OVR::Sizei(64));
    OVR::Sizei tex1Size = OVR::Sizei::Max(OVR::Sizei::Min(recommenedTex1Size, OVR::Sizei(4096)), OVR::Sizei(64));

    mEyeRenderSize[ovrEye_Left] = OVR::Sizei::Min(tex0Size, recommenedTex0Size);
    mEyeRenderSize[ovrEye_Right] = OVR::Sizei::Min(tex1Size, recommenedTex1Size);

    // Store texture pointers and viewports that will be passed for rendering.
    mEyeTexture[ovrEye_Left].OGL.Header.API            = ovrRenderAPI_OpenGL;
    mEyeTexture[ovrEye_Left].OGL.Header.TextureSize    = tex0Size;
    mEyeTexture[ovrEye_Left].OGL.Header.RenderViewport = OVR::Recti(mEyeRenderSize[0]);
    mEyeTexture[ovrEye_Left].OGL.TexId = 0;
    mEyeTexture[ovrEye_Right].OGL.Header.API            = ovrRenderAPI_OpenGL;
    mEyeTexture[ovrEye_Right].OGL.Header.TextureSize    = tex1Size;
    mEyeTexture[ovrEye_Right].OGL.Header.RenderViewport = OVR::Recti(mEyeRenderSize[1]);
    mEyeTexture[ovrEye_Right].OGL.TexId = 0;

    for (S32 i = (S32)LLHMD::LEFT_EYE; i <= (S32)LLHMD::RIGHT_EYE; ++i)
    {
        if (!mEyeRT[i])
        {
            LL_WARNS() << "could not find Eye RenderTarget for HMD" << LL_ENDL;
            removeHMDDevice();
            return FALSE;
        }
        U32 eye = i == LLHMD::LEFT_EYE ? ovrEye_Left : ovrEye_Right;
        U32 w = (U32)mEyeTexture[eye].OGL.Header.TextureSize.w;
        U32 h = (U32)mEyeTexture[eye].OGL.Header.TextureSize.h;
        bool needsRealloc = !mEyeRT[i]->isComplete() || mEyeRT[i]->getWidth() != w || mEyeRT[i]->getHeight() != h;
        if (needsRealloc)
        {
            if (mEyeRT[i]->isComplete())
            {
                mEyeRT[i]->release();
            }
            if (!mEyeRT[i]->allocate(w, h, GL_RGB, false, false, LLTexUnit::TT_TEXTURE, true))
            {
                LL_WARNS() << "could not allocate Eye RenderTarget for HMD" << LL_ENDL;
                removeHMDDevice();
                return FALSE;
            }
        }
        mEyeTexture[eye].OGL.TexId = mEyeRT[i]->isComplete() ? mEyeRT[i]->getTexture() : 0;
    }

    // Calculate HMD Hardware Settings
    U32 hmdCaps = gSavedSettings.getBOOL("DisableVerticalSync") ? ovrHmdCap_NoVSync : 0;
    hmdCaps |= gSavedSettings.getBOOL("HMDLowPersistence") ? ovrHmdCap_LowPersistence : 0;
    hmdCaps |= gSavedSettings.getBOOL("HMDUseMotionPrediction") ? ovrHmdCap_DynamicPrediction : 0;
    hmdCaps |= gHMD.isHMDDisplayEnabled() ? 0 : ovrHmdCap_DisplayOff;
    hmdCaps |= (gHMD.isUsingAppWindow() && !gHMD.isHMDMirror()) ? ovrHmdCap_NoMirrorToWindow : 0;
    ovrHmd_SetEnabledCaps(mHMD, hmdCaps);

    ovrRenderAPIConfig config;
    config.Header.API = ovrRenderAPI_OpenGL;
    config.Header.RTSize.w = gHMD.isUsingAppWindow() ? gViewerWindow->getWindowWidthRaw() : mHMD->Resolution.w;
    config.Header.RTSize.h = gHMD.isUsingAppWindow() ? gViewerWindow->getWindowHeightRaw() : mHMD->Resolution.h;
    config.Header.Multisample = gGLManager.mHasTextureMultisample;

    U32 distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette;
    distortionCaps |= gHMD.isUsingAppWindow() ? ovrDistortionCap_SRGB : 0;
    distortionCaps |= gSavedSettings.getBOOL("HMDPixelLuminanceOverdrive") ? ovrDistortionCap_Overdrive : 0;
    distortionCaps |= gHMD.isTimewarpEnabled() ? ovrDistortionCap_TimeWarp : 0;
    distortionCaps |= gHMD.isTimewarpEnabled() && gSavedSettings.getBOOL("HMDTimewarpNoJit") ? ovrDistortionCap_ProfileNoTimewarpSpinWaits : 0;

    if (!ovrHmd_ConfigureRendering(mHMD, &config, distortionCaps, eyeFov, mEyeRenderDesc))
    {
        return FALSE;
    }

    U32 sensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;
    sensorCaps |= (gSavedSettings.getBOOL("HMDEnablePositionalTracking") && (mHMD->TrackingCaps & ovrTrackingCap_Position) != 0) ? ovrTrackingCap_Position : 0;
    if (mTrackingCaps != sensorCaps)
    {
        ovrHmd_ConfigureTracking(mHMD, sensorCaps, 0);
        mTrackingCaps = sensorCaps;
    }    

    // Calculate projections
    F32 n = LLViewerCamera::getInstance()->getNear();
    F32 f = LLViewerCamera::getInstance()->getFar();
    mProjection[ovrEye_Left] = ovrMatrix4f_Projection(mEyeRenderDesc[ovrEye_Left].Fov, n, f, true);
    mProjection[ovrEye_Right] = ovrMatrix4f_Projection(mEyeRenderDesc[ovrEye_Right].Fov, n, f, true);

    float orthoDistance = 0.8f; // 2D is 0.8 meter from camera..  TODO: Can I get this value from somewhere else (LLViewerCamera perhaps?)
    OVR::Vector2f orthoScale0 = OVR::Vector2f(1.0f) / OVR::Vector2f(mEyeRenderDesc[ovrEye_Left].PixelsPerTanAngleAtCenter);
    OVR::Vector2f orthoScale1 = OVR::Vector2f(1.0f) / OVR::Vector2f(mEyeRenderDesc[ovrEye_Right].PixelsPerTanAngleAtCenter);

    mOrthoProjection[ovrEye_Left] = ovrMatrix4f_OrthoSubProjection(mProjection[ovrEye_Left], orthoScale0, orthoDistance, mEyeRenderDesc[ovrEye_Left].ViewAdjust.x);
    mOrthoProjection[ovrEye_Right] = ovrMatrix4f_OrthoSubProjection(mProjection[ovrEye_Right], orthoScale1, orthoDistance, mEyeRenderDesc[ovrEye_Right].ViewAdjust.x);

    OVR::CAPI::HMDState* hmdState = (OVR::CAPI::HMDState*)mHMD->Handle;
    OVR::CAPI::HMDRenderState* renderState = hmdState ? &(hmdState->RenderState) : NULL;
    OVR::HmdRenderInfo* renderInfo = renderState ? &(renderState->RenderInfo) : NULL;
    mScreenSizeInMeters = renderInfo ? renderInfo->ScreenSizeInMeters : OVR::Sizef(kDefaultHScreenSize, kDefaultVScreenSize);
    mLensSeparationInMeters = renderInfo ? renderInfo->LensSeparationInMeters : kDefaultLenSeparationDistance;
    mEyeToScreenDistance = renderInfo ? (0.5f * (renderInfo->EyeLeft.ReliefInMeters + renderInfo->EyeRight.ReliefInMeters)) : getEyeToScreenDistanceDefault();
    mInterpupillaryDistance = ovrHmd_GetFloat(mHMD, OVR_KEY_IPD, getInterpupillaryOffsetDefault());

    OVR::FovPort l = mEyeRenderDesc[ovrEye_Left].Fov;
    OVR::FovPort r = mEyeRenderDesc[ovrEye_Right].Fov;
    mFOVRadians.w = (l.GetHorizontalFovRadians() + r.GetHorizontalFovRadians()) * 0.5f;
    mFOVRadians.h = (l.GetVerticalFovRadians() + r.GetVerticalFovRadians()) * 0.5f;
    mAspect = mFOVRadians.w / mFOVRadians.h;
    // TODO: alternatively:
    // mAspect = ((F32)mHMD->Resolution.w / 2.0f) / (F32)mHMD->Resolution.h;
    mOrthoPixelOffset[(U32)OVR::StereoEye_Center] = 0.0f;
    mOrthoPixelOffset[(U32)OVR::StereoEye_Left] = mOrthoProjection[ovrEye_Left].M[0][3];
    mOrthoPixelOffset[(U32)OVR::StereoEye_Right] = mOrthoProjection[ovrEye_Right].M[0][3];

    gHMD.renderSettingsChanged(FALSE);
    return TRUE;
}


BOOL LLHMDImplOculus::beginFrame()
{
    gHMD.isFrameInProgress( mHMD &&
                            gHMD.isPreDetectionInitialized() &&
                            gHMD.isPostDetectionInitialized() &&
                            (gHMD.isUsingDebugHMD() || (gHMD.isHMDConnected() && gHMD.isHMDSensorConnected() && gHMD.isHMDDisplayEnabled())));
    if (gHMD.isFrameInProgress())
    {
        double curTime = ovr_GetTimeInSeconds();

        mFrameTiming = ovrHmd_BeginFrame(mHMD, 0);
        mTrackingState = ovrHmd_GetTrackingState(mHMD, mFrameTiming.ScanoutMidpointSeconds);
        gHMD.isPositionTrackingEnabled((mTrackingState.StatusFlags & (ovrStatus_PositionConnected | ovrStatus_PositionTracked)) == (ovrStatus_PositionConnected | ovrStatus_PositionTracked));
        
        mFrameCounter++;
        mTotalFrameCounter++;
        if (mLastFpsUpdate == 0.0f)
        {
            mLastFpsUpdate = curTime;
        }
        float dtFPS = (float)(curTime - mLastFpsUpdate);
        if (dtFPS >= 1.0f)
        {
            mSecondsPerFrame = (float)(curTime - mLastFpsUpdate) / (float)mFrameCounter;
            mFPS = 1.0f / mSecondsPerFrame;
            mLastFpsUpdate = curTime;
            mFrameCounter = 0;
        }

        gHMD.isFrameTimewarped(FALSE);
        double dtTimewarp = curTime - mLastTimewarpUpdate;
        if (gHMD.isTimewarpEnabled())
        {
            if ((dtTimewarp < 0.0) || ((float)dtTimewarp > gHMD.getTimewarpIntervalSeconds()))
            {
                // This allows us to do "fractional" speeds, e.g. 45fps rendering on a 60fps display.
                mLastTimewarpUpdate += gHMD.getTimewarpIntervalSeconds();
                if (dtTimewarp > 5.0)
                {
                    // renderInterval is probably tiny (i.e. "as fast as possible")
                    mLastTimewarpUpdate = curTime;
                }
            }
            else
            {
                gHMD.isFrameTimewarped(TRUE);
            }
        }
        if (!gHMD.isFrameTimewarped())
        {
            applyDynamicResolutionScaling(curTime);
            for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
            {
                ovrEyeType eye = mHMD->EyeRenderOrder[eyeIndex];
                mEyeRenderPose[eye] = ovrHmd_GetEyePose(mHMD, eye);

                // Note that the LL coord system uses X forward, Y left, and Z up whereas the Oculus SDK uses the
                // OpenGL coord system of -Z forward, X right, Y up.  To compensate, we retrieve the angles in the Oculus
                // coord system, but change the axes to ours, then negate X and Z to account for the forward left axes
                // being positive in LL, but negative in Oculus.
                // LL X = Oculus -Z, LL Y = Oculus -X, and LL Z = Oculus Y
                // Yaw = rotation around the "up" axis          (LL  Z, Oculus  Y)
                // Pitch = rotation around the left/right axis  (LL -Y, Oculus  X)
                // Roll = rotation around the forward axis      (LL  X, Oculus -Z)
                OVR::Quatf orient = mEyeRenderPose[eye].Orientation;
                float r, p, y;
                orient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&y, &p, &r);
                mEyeRPY[eyeIndex].set(-r, -p, y);
                mEyePos[eyeIndex].set(-mEyeRenderPose[ovrEye_Left].Position.z, -mEyeRenderPose[ovrEye_Left].Position.x, mEyeRenderPose[ovrEye_Left].Position.y);

                OVR::Matrix4f rpy = OVR::Matrix4f(mEyeRenderPose[eye].Orientation);
                OVR::Vector3f up = rpy.Transform(OVR::Vector3f(0.0f, 1.0f, 0.0f));
                OVR::Vector3f fwd = rpy.Transform(OVR::Vector3f(0.0f, 0.0f, -1.0f));
                OVR::Vector3f pos = rpy.Transform(mEyeRenderPose[eye].Position);
                OVR::Matrix4f v = OVR::Matrix4f::LookAtRH(pos, pos + fwd, up);
                mView[eye] = OVR::Matrix4f::Translation(mEyeRenderDesc[eye].ViewAdjust) * v;
            }
        }
    }
    return gHMD.isFrameInProgress();
}


BOOL LLHMDImplOculus::endFrame()
{
    BOOL res = gHMD.isFrameInProgress();
    if (res)
    {
        ovrHmd_EndFrame(mHMD, mEyeRenderPose, const_cast<ovrTexture*>(reinterpret_cast<const ovrTexture*>(mEyeTexture)));
    }
    gHMD.isFrameInProgress(FALSE);
    return res;
}


void LLHMDImplOculus::applyDynamicResolutionScaling(double curTime)
{
    if (!gHMD.useDynamicResolutionScaling())
    {
        // Restore viewport rectangle in case dynamic res scaling was enabled before.
        mEyeTexture[ovrEye_Left].OGL.Header.RenderViewport.Size = mEyeRenderSize[ovrEye_Left];
        mEyeTexture[ovrEye_Right].OGL.Header.RenderViewport.Size = mEyeRenderSize[ovrEye_Right];
        return;
    }

    // Demonstrate dynamic-resolution rendering.
    // This demo is too simple to actually have a framerate that varies that much, so we'll
    // just pretend this is trying to cope with highly dynamic rendering load.
    float dynamicRezScale = 1.0f;

    {
        // Hacky stuff to make up a scaling...
        // This produces value oscillating as follows: 0 -> 1 -> 0.        
        static double dynamicRezStartTime = curTime;
        float dynamicRezPhase = (float)(curTime - dynamicRezStartTime);
        const float dynamicRezTimeScale = 4.0f;

        dynamicRezPhase /= dynamicRezTimeScale;
        if (dynamicRezPhase < 1.0f)
        {
            dynamicRezScale = dynamicRezPhase;
        }
        else if (dynamicRezPhase < 2.0f)
        {
            dynamicRezScale = 2.0f - dynamicRezPhase;
        }
        else
        {
            // Reset it to prevent creep.
            dynamicRezStartTime = curTime;
            dynamicRezScale = 0.0f;
        }

        // Map oscillation: 0.5 -> 1.0 -> 0.5
        dynamicRezScale = dynamicRezScale * 0.5f + 0.5f;
    }

    OVR::Sizei sizeLeft  = mEyeRenderSize[ovrEye_Left];
    OVR::Sizei sizeRight = mEyeRenderSize[ovrEye_Right];

    // This viewport is used for rendering and passed into ovrHmd_EndEyeRender.
    mEyeTexture[ovrEye_Left].OGL.Header.RenderViewport.Size = OVR::Sizei(int(sizeLeft.w  * dynamicRezScale), int(sizeLeft.h  * dynamicRezScale));
    mEyeTexture[ovrEye_Right].OGL.Header.RenderViewport.Size = OVR::Sizei(int(sizeRight.w * dynamicRezScale), int(sizeRight.h * dynamicRezScale));
}


F32 LLHMDImplOculus::getAspect() const
{
    return (gHMD.isPostDetectionInitialized() && mCurrentEye != (U32)LLHMD::CENTER_EYE) ? mAspect : LLViewerCamera::getInstance()->getAspect();
}


LLQuaternion LLHMDImplOculus::getHMDOrient() const
{
    LLQuaternion q(0.0f, 0.0f, 0.0f, 1.0f);
    if (gHMD.isPostDetectionInitialized() && mCurrentEye != (U32)LLHMD::CENTER_EYE)
    {
        U32 eye = getCurrentOVREye();
        q.setEulerAngles(mEyeRPY[eye][LLHMD::ROLL], mEyeRPY[eye][LLHMD::PITCH], mEyeRPY[eye][LLHMD::YAW]);
    }
    return q;
}


void LLHMDImplOculus::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const
{
    if (gHMD.isPostDetectionInitialized())
    {
        U32 eye = getCurrentOVREye();
        roll = mEyeRPY[eye][LLHMD::ROLL];
        pitch = mEyeRPY[eye][LLHMD::PITCH];
        yaw = mEyeRPY[eye][LLHMD::YAW];
    }
    else
    {
        roll = pitch = yaw = 0.0f;
    }
}


void LLHMDImplOculus::getCurrentEyeProjectionOffset(F32 p[4][4]) const
{
    if (mCurrentEye == (U32)LLHMD::CENTER_EYE)
    {
        memset(p, 0, sizeof(F32) * 4 * 4);
    }
    else
    {
        memcpy(p, mProjection[getCurrentOVREye()].M, sizeof(F32) * 4 * 4);
    }
    //U32 eye = getCurrentOVREye();
    //for (S32 row = 0; row < 4; ++row)
    //{
    //    for (S32 col = 0; col < 4; ++col)
    //    {
    //        p[row][col] = mProjection[eye].M[row][col];
    //    }
    //}
}


LLVector3 LLHMDImplOculus::getStereoCullCameraForwards() const
{
    return gHMD.isPostDetectionInitialized() ? (-mEyeRenderDesc[ovrEye_Left].ViewAdjust.x / mHMD->DefaultEyeFov[ovrEye_Left].LeftTan) * LLViewerCamera::getInstance()->getXAxis() : LLVector3::zero;
}


void LLHMDImplOculus::getViewportInfo(S32& x, S32& y, S32& w, S32& h) const
{
    if (gHMD.isPostDetectionInitialized())
    {
        U32 eye = getCurrentOVREye();
        x = mEyeTexture[eye].OGL.Header.RenderViewport.Pos.x;
        y = mEyeTexture[eye].OGL.Header.RenderViewport.Pos.y;
        w = mEyeTexture[eye].OGL.Header.RenderViewport.Size.w;
        h = mEyeTexture[eye].OGL.Header.RenderViewport.Size.h;
    }
    else
    {
        x = y = w = h = 0;
    }
}


void LLHMDImplOculus::getViewportInfo(S32 vp[4]) const
{
    if (gHMD.isPostDetectionInitialized())
    {
        U32 eye = getCurrentOVREye();
        vp[0] = mEyeTexture[eye].OGL.Header.RenderViewport.Pos.x;
        vp[1] = mEyeTexture[eye].OGL.Header.RenderViewport.Pos.y;
        vp[2] = mEyeTexture[eye].OGL.Header.RenderViewport.Size.w;
        vp[3] = mEyeTexture[eye].OGL.Header.RenderViewport.Size.h;
    }
    else
    {
        memset(vp, 0, sizeof(S32) * 4);
    }
}


F32 LLHMDImplOculus::getCurrentEyeCameraOffset() const
{
    return (gHMD.isPostDetectionInitialized() && mCurrentEye != (U32)LLHMD::CENTER_EYE) ? -mEyeRenderDesc[getCurrentOVREye()].ViewAdjust.x : 0.0f;
}


LLVector3 LLHMDImplOculus::getCurrentEyePosition(const LLVector3& centerPos) const
{
    return (gHMD.isPostDetectionInitialized() && mCurrentEye != (U32)LLHMD::CENTER_EYE) ? (centerPos - (-mEyeRenderDesc[getCurrentOVREye()].ViewAdjust.x * LLViewerCamera::getInstance()->getYAxis())) : centerPos;
}


LLRenderTarget* LLHMDImplOculus::getCurrentEyeRT()
{
    return mEyeRT[mCurrentEye];
}

LLRenderTarget* LLHMDImplOculus::getEyeRT(U32 eye)
{
    return (eye >= (U32)LLHMD::CENTER_EYE && eye <= (U32)LLHMD::RIGHT_EYE) ? mEyeRT[eye] : NULL;
}


#endif // LL_HMD_SUPPORTED
