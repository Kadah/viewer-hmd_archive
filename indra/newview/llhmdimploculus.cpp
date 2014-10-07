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
#include "llnotificationsutil.h"
#include "lltimer.h"
#if LL_WINDOWS
    #include "llwindowwin32.h"
#elif LL_DARWIN
    #include "llwindowmacosx.h"
    #define IDCONTINUE 1        // Exist on Windows along "IDOK" and "IDCANCEL" but not on Mac
#endif

#include "CAPI/CAPI_HMDState.h"


LLHMDImplOculus::LLHMDImplOculus()
    : mHMD(NULL)
    , mTrackingCaps(0)
    , mLastTimewarpUpdate(0.0)
    , mCurrentHMDCount(0)
    , mCurrentEye(LLHMD::CENTER_EYE)
{
    OVR::WorldAxes axesOculus(OVR::Axis_Right, OVR::Axis_Up, OVR::Axis_Out);
    OVR::WorldAxes axesLL(OVR::Axis_In, OVR::Axis_Left, OVR::Axis_Up);
    //mConvOculusToLL = OVR::Matrix4f::AxisConversion(axesLL, axesOculus);
    //mConvLLToOculus = OVR::Matrix4f::AxisConversion(axesOculus, axesLL);
    mEyeRPY.set(0.0f, 0.0f, 0.0f);
    mHeadPos.set(0.0f, 0.0f, 0.0f);
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

    if (mHMD)
    {
        mTrackingCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;
        mTrackingCaps |= (gSavedSettings.getBOOL("HMDEnablePositionalTracking") && (mHMD->TrackingCaps & ovrTrackingCap_Position) != 0) ? ovrTrackingCap_Position : 0;
        // must be called every time an HMD is initialized, or in some cases, orientation/position tracking will not be possible.
        ovrHmd_ConfigureTracking(mHMD, mTrackingCaps, 0);

        // Note: (mHMD->HmdCaps & ovrHmdCap_Present) is ALWAYS true as of OVR SDK 0.4.1b.  sigh.   The only reliable way to truly tell if a Rift is connected is by using the tracking state
        //BOOL hmdConnected = mHMD && (mHMD->HmdCaps & ovrHmdCap_Present) != 0;
        //gHMD.isHMDConnected(hmdConnected);
        mTrackingState = ovrHmd_GetTrackingState(mHMD, 0.0);
        BOOL isHMDConnected = (mTrackingState.StatusFlags & ovrStatus_HmdConnected) != 0;
        gHMD.isHMDConnected(gHMD.isUsingDebugHMD() || isHMDConnected);
        gHMD.isPositionTrackingEnabled((mTrackingState.StatusFlags & (ovrStatus_PositionConnected | ovrStatus_PositionTracked)) == (ovrStatus_PositionConnected | ovrStatus_PositionTracked));

        // as of OVR SDK 0.4.1b, ovrHmdCap_Available and ovrHmdCap_Captured do not seem to ever be set to valid values.
        //BOOL hmdSensor
        //gHMD.isHMDSensorConnected(mHMD && (mHMD->HmdCaps & (ovrHmdCap_Available | ovrHmdCap_Captured)) != 0);

        BOOL hmdDisplayEnabled = mHMD && mHMD->ProductName && mHMD->ProductName[0] != 0;
        gHMD.isHMDDisplayEnabled(hmdDisplayEnabled);
        BOOL hmdUsingAppWindow = mHMD && (mHMD->HmdCaps & ovrHmdCap_ExtendDesktop) == 0;
        gHMD.isHMDDirectMode(hmdUsingAppWindow);

        gHMD.renderSettingsChanged(TRUE);
    }
    else
    {
        // if mHMD is gone and we've already created a HMDWindow, then destroy it.
        if (gHMD.isPostDetectionInitialized())
        {
            removeHMDDevice();
        }
    }

    BOOL res = mHMD != NULL; //  && !gHMD.isUsingDebugHMD();
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
    if (gHMD.isPostDetectionInitialized() && !gHMD.isHMDDirectMode() && !gHMD.useMirrorHack())
    {
        gViewerWindow->getWindow()->destroyHMDWindow();
    }
    gHMD.releaseAllEyeRT();
    gHMD.isPostDetectionInitialized(FALSE);
    gHMD.isUsingDebugHMD(FALSE);
    gHMD.isHMDConnected(FALSE);
    gHMD.isHMDMirror(FALSE);
    gHMD.isHMDDisplayEnabled(FALSE);
    gHMD.isHMDDirectMode(FALSE);
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

    // NOTE:  as of OVR SDK 0.4.1b:
    //   - direct mode is detected, but will not render to the HMD with opengl
    //   - extended mode cannot render to a secondary window
    //   Thus, the only way things work is in extended mode and to render to the main window (i.e. mirroring) and then move the window to the HMD.  *sigh*
    gHMD.useMirrorHack(TRUE);
    //gHMD.useMirrorHack(gHMD.isUsingAppWindow());
    BOOL isMirror = FALSE;
    if (!pWin->initHMDWindow(mHMD->WindowsPos.x, mHMD->WindowsPos.y, mHMD->Resolution.w, mHMD->Resolution.h, gHMD.useMirrorHack(), isMirror))
    {
        LL_INFOS("HMD") << "HMD Window init failed!" << LL_ENDL;
        LLNotificationsUtil::add("HMDModeErrorNoWindow");
        return FALSE;
    }

#if defined(LL_WINDOWS) || defined(LL_DARWIN)
    BOOL attach = FALSE;
    if (isMirror)
    {
        attach = (BOOL)ovrHmd_AttachToWindow(mHMD, pWin->getPlatformWindow(0), NULL, NULL);
    }
    else
    {
        attach = (BOOL)ovrHmd_AttachToWindow(mHMD, pWin->getPlatformWindow(1), NULL, NULL);
    }
#endif
    if (attach)
    {
        gHMD.isPostDetectionInitialized(TRUE);
        gHMD.failedInit(FALSE);
        gHMD.isHMDMirror(isMirror);
        gHMD.isHSWShowing(TRUE);

        setCurrentEye(OVR::StereoEye_Center);

        LL_INFOS("HMD") << "HMD Post-Detection Init Success" << LL_ENDL;
    }
    else
    {
        LL_INFOS("HMD") << "HMD Post-Detection attach to window failed!" << LL_ENDL;
        LLNotificationsUtil::add("HMDModeErrorNoWindow");
    }
    return attach;
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


BOOL LLHMDImplOculus::detectHMDDevice(BOOL force)
{
    // for some unknown reason, Oculus completely got rid of their Message system that would notify when a device was attached or not, so now we have to poll
    // every frame to see if the status has changed.  WTF?
    // Polling is not really a good option because ovrHmd_Detect() causes huge lag spikes when no Rift is connected, thus ensuring a bad user experience for those
    // without Rift hardware attached.
    // Thus, the only feasible option is to force a manual detection when the user requests it (which could be implicit when attempting to enter HMD mode).
    // Grrrr.  WHY, Oculus, WHYYYYYY???

    // clamp value to [0,1] as we only care about the first HMD connected
    S32 numHMD = llmax(llmin(ovrHmd_Detect(), 0), 1);
    if (force || numHMD != mCurrentHMDCount)
    {
        removeHMDDevice();
        if ((numHMD > 0 || gHMD.isAdvancedMode()))
        {
            if (initHMDDevice())
            {
                //// We need to give the Oculus camera time to initialize or it will not feed us orientation/positional data.  Sigh.
                //ms_sleep(1500);
                // Need to make sure post-detection init is run properly so that setRenderMode can change immediately after this call.
                gHMD.onIdle();
                gHMD.onIdle();
            }
        }
    }
    return mCurrentHMDCount > 0 && isReady();
}


void LLHMDImplOculus::onIdle()
{
    if (!gHMD.isPreDetectionInitialized())
    {
        return;
    }

    if (mHMD)
    {
        // RIFT-158: The camera jitters a lot when you move the camera with the HMD
        // Apparently, calling ovrHmd_GetTrackingState with a second param of 0 causes a lot of jitter.   Weird.
        //mTrackingState = ovrHmd_GetTrackingState(mHMD, 0.0);
        bool wasHMDConnected = gHMD.isHMDConnected();
        BOOL isHMDConnected = (mTrackingState.StatusFlags & ovrStatus_HmdConnected) != 0;
        gHMD.isHMDConnected(gHMD.isUsingDebugHMD() || isHMDConnected);
        gHMD.isPositionTrackingEnabled((mTrackingState.StatusFlags & (ovrStatus_PositionConnected | ovrStatus_PositionTracked)) == (ovrStatus_PositionConnected | ovrStatus_PositionTracked));
        if (wasHMDConnected && !gHMD.isHMDConnected())
        {
            LL_INFOS("HMD") << "HMD Device Not Detected" << LL_ENDL;
            if (gHMD.isHMDMode())
            {
                removeHMDDevice();
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

    if (gHMD.isHMDMode())
    {
        if (gHMD.renderSettingsChanged())
        {
            if (!calculateViewportSettings())
            {
                return;
            }
        }

        // This is a weird place to call this, but unfortunately, with the new OVR SDK, you cannot call the tracking functions
        // and get accurate predicted pose info unless you've called ovrHmd_BeginFrame.   Since the orientation values are used
        // for camera positioning, etc, we have to call beginFrame before we do that even though this probably messes up the
        // OVR frame timer a bit and makes frames seem to be taking longer to render than they actually are.  Ideally, this
        // should be at the top of LLViewerDisplay::display() to get correct timing.  Oh well.
        gHMD.beginFrame();
    }
}


BOOL LLHMDImplOculus::calculateViewportSettings()
{
    llassert(mHMD);

    // Initialize FovSideTanMax, which allows us to change all Fov sides at once - Fov
    // starts at default and is clamped to this value.
    //F32 fovSideTanLimit = OVR::FovPort::Max(mHMD->MaxEyeFov[ovrEye_Left], mHMD->MaxEyeFov[ovrEye_Right]).GetMaxSideTan();
    F32 fovSideTanMax = OVR::FovPort::Max(mHMD->DefaultEyeFov[ovrEye_Left], mHMD->DefaultEyeFov[ovrEye_Right]).GetMaxSideTan();

    // Initialize eye rendering information for ovrHmd_Configure.
    // The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
    // Clamp Fov based on our dynamically adjustable FovSideTanMax.
    // Most apps should use the default, but reducing Fov does reduce rendering cost.
    ovrFovPort eyeFov[ovrEye_Count];
    eyeFov[ovrEye_Left] = OVR::FovPort::Min(mHMD->DefaultEyeFov[ovrEye_Left], OVR::FovPort::FovPort(fovSideTanMax));
    eyeFov[ovrEye_Right] = OVR::FovPort::Min(mHMD->DefaultEyeFov[ovrEye_Right], OVR::FovPort::FovPort(fovSideTanMax));

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
    mEyeTexture[ovrEye_Left].OGL.TexId = 0;
    mEyeTexture[ovrEye_Right].OGL.Header.API            = ovrRenderAPI_OpenGL;
    mEyeTexture[ovrEye_Right].OGL.Header.TextureSize    = tex1Size;
    mEyeTexture[ovrEye_Right].OGL.TexId = 0;
    mEyeTexture[ovrEye_Left].OGL.Header.RenderViewport = OVR::Recti(OVR::Sizei(mEyeRenderSize[ovrEye_Left].w, mEyeRenderSize[ovrEye_Left].h));
    mEyeTexture[ovrEye_Right].OGL.Header.RenderViewport = OVR::Recti(OVR::Sizei(mEyeRenderSize[ovrEye_Right].w, mEyeRenderSize[ovrEye_Right].h));

    BOOL useSRGB = (gHMD.useSRGBDistortion() || gHMD.isHMDDirectMode()) ? GL_SRGB_ALPHA : GL_RGBA;

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
            U32 colorFormat = useSRGB ? GL_SRGB_ALPHA : GL_RGBA;
            if (!mEyeRT[i]->allocate(w, h, colorFormat, true, false, LLTexUnit::TT_TEXTURE, true))
            {
                LL_WARNS() << "could not allocate Eye RenderTarget for HMD" << LL_ENDL;
                removeHMDDevice();
                return FALSE;
            }
        }
        mEyeTexture[eye].OGL.TexId = mEyeRT[i]->isComplete() ? mEyeRT[i]->getTexture() : 0;
        mEyeTexture[eye].OGL.Header.TextureSize = mEyeRT[i]->isComplete() ? OVR::Sizei(mEyeRT[i]->getWidth(), mEyeRT[i]->getHeight()) : OVR::Sizei(0, 0);
    }

    // Calculate HMD Hardware Settings
    U32 hmdCaps = 0; // gSavedSettings.getBOOL("DisableVerticalSync") ? ovrHmdCap_NoVSync : 0;
    hmdCaps |= gHMD.useLowPersistence() ? ovrHmdCap_LowPersistence : 0;
    hmdCaps |= gHMD.useMotionPrediction() ? ovrHmdCap_DynamicPrediction : 0;
    hmdCaps |= gHMD.isHMDDisplayEnabled() ? 0 : ovrHmdCap_DisplayOff;
    hmdCaps |= (gHMD.isHMDDirectMode() && !gHMD.isHMDMirror()) ? ovrHmdCap_NoMirrorToWindow : 0;
    ovrHmd_SetEnabledCaps(mHMD, hmdCaps);

    ovrGLConfig config;
    config.OGL.Header.API = ovrRenderAPI_OpenGL;
    config.OGL.Header.RTSize.w = (gHMD.isHMDDirectMode() && !gHMD.isUsingDebugHMD()) ? gViewerWindow->getWindowWidthRaw() : mHMD->Resolution.w;
    config.OGL.Header.RTSize.h = (gHMD.isHMDDirectMode() && !gHMD.isUsingDebugHMD()) ? gViewerWindow->getWindowHeightRaw() : mHMD->Resolution.h;
    config.OGL.Header.Multisample = gGLManager.mHasTextureMultisample;
#if LL_WINDOWS
    // undocumented, but necessary to get Windows rendering working even in extended mode.  If this is not done, rendering will still sometimes
    // work, but it depends on a windows command to get the "main" window - which sometimes returns NULL and thus rendering won't work because
    // there's no attached window (at least according to the SDK).   This is actually something of a Windows issue, but since the SDK can easily
    // work around it, it seems like this fix should be documented somewhere in the SDK docs or examples.  Oh well.
    config.OGL.Window = (HWND)gViewerWindow->getWindow()->getPlatformWindow((!gHMD.isHMDMirror() && !gHMD.isHMDDirectMode()) ? 1 : 0);
    config.OGL.DC = NULL;
#endif

    U32 distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_NoRestore;
    distortionCaps |= useSRGB ? ovrDistortionCap_SRGB : 0;
    distortionCaps |= gHMD.usePixelLuminanceOverdrive() ? ovrDistortionCap_Overdrive : 0;
    distortionCaps |= gHMD.isTimewarpEnabled() ? (ovrDistortionCap_TimeWarp | ovrDistortionCap_ProfileNoTimewarpSpinWaits) : 0;

    if (!ovrHmd_ConfigureRendering(mHMD, reinterpret_cast<ovrRenderAPIConfig*>(&config), distortionCaps, eyeFov, mEyeRenderDesc))
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

    OVR::CAPI::HMDState* hmdState = (OVR::CAPI::HMDState*)mHMD->Handle;
    OVR::CAPI::HMDRenderState* renderState = hmdState ? &(hmdState->RenderState) : NULL;
    OVR::HmdRenderInfo* renderInfo = renderState ? &(renderState->RenderInfo) : NULL;
    mEyeToScreenDistance = renderInfo ? (0.5f * (renderInfo->EyeLeft.ReliefInMeters + renderInfo->EyeRight.ReliefInMeters)) : getEyeToScreenDistanceDefault();
    mInterpupillaryDistance = ovrHmd_GetFloat(mHMD, OVR_KEY_IPD, getInterpupillaryOffsetDefault());

    OVR::FovPort l = mEyeRenderDesc[ovrEye_Left].Fov;
    OVR::FovPort r = mEyeRenderDesc[ovrEye_Right].Fov;
    mFOVRadians.w = (l.GetHorizontalFovRadians() + r.GetHorizontalFovRadians()) * 0.5f;
    mFOVRadians.h = (l.GetVerticalFovRadians() + r.GetVerticalFovRadians()) * 0.5f;
    mAspect = mFOVRadians.w / mFOVRadians.h;
    // TODO: alternatively:
    // mAspect = ((F32)mHMD->Resolution.w / 2.0f) / (F32)mHMD->Resolution.h;

    OVR::Matrix4f orthoProjection[ovrEye_Count];
    orthoProjection[ovrEye_Left] = ovrMatrix4f_OrthoSubProjection(mProjection[ovrEye_Left], orthoScale0, orthoDistance, mEyeRenderDesc[ovrEye_Left].ViewAdjust.x);
    orthoProjection[ovrEye_Right] = ovrMatrix4f_OrthoSubProjection(mProjection[ovrEye_Right], orthoScale1, orthoDistance, mEyeRenderDesc[ovrEye_Right].ViewAdjust.x);
    mOrthoPixelOffset[(U32)OVR::StereoEye_Center] = 0.0f;
    mOrthoPixelOffset[(U32)OVR::StereoEye_Left] = orthoProjection[ovrEye_Left].M[0][3];
    mOrthoPixelOffset[(U32)OVR::StereoEye_Right] = orthoProjection[ovrEye_Right].M[0][3];

    if (gHMD.isHMDMode())
    {
        LLViewerCamera* pCamera = LLViewerCamera::getInstance();
        pCamera->setAspect(getAspect());
        pCamera->setDefaultFOV(getVerticalFOV());
        gSavedSettings.setF32("CameraAngle", getVerticalFOV());
    }
    gHMD.calculateUIEyeDepth();
    gHMD.onChangeUISurfaceShape();

    gHMD.renderSettingsChanged(FALSE);
    return TRUE;
}


BOOL LLHMDImplOculus::beginFrame()
{
    gHMD.isFrameInProgress(isReady() && gHMD.isPostDetectionInitialized());
    if (gHMD.isFrameInProgress())
    {
        mFrameTiming = ovrHmd_BeginFrame(mHMD, 0);
        double curTime = ovr_GetTimeInSeconds();
        mTrackingState = ovrHmd_GetTrackingState(mHMD, mFrameTiming.ScanoutMidpointSeconds);

        gHMD.isFrameTimewarped(FALSE);
        if (gHMD.isTimewarpEnabled())
        {
            double dtTimewarp = curTime - mLastTimewarpUpdate;
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

            //LL_INFOS("HMD") << std::setprecision(6)
            //    << "[" << gFrameCount << ": " << mFrameTiming.ThisFrameSeconds << " (" << mFrameTiming.DeltaSeconds << ")] TW:" 
            //    << (gHMD.isFrameTimewarped() ? "T" : "F")
            //    << ", dtTimewarp: " << std::setprecision(10) << dtTimewarp
            //    //<< ", Rot: {" << mEyeRPY[VX] << "," << mEyeRPY[VY] << "," << mEyeRPY[VZ] << "}" 
            //    //<< ", Pos: {" << -mHeadPos[VZ] << "," << -mHeadPos[VX] << "," << mHeadPos[VY] << "}"
            //    << LL_ENDL;
        }
        if (!gHMD.isFrameTimewarped())
        {
            OVR::Posef pose = mTrackingState.HeadPose.ThePose;

            // OpenGL coord system of -Z forward, X right, Y up. 
            mEyeRotation.set(pose.Rotation.x, pose.Rotation.y, pose.Rotation.z, pose.Rotation.w);
            if (gHMD.isPositionTrackingEnabled())
            {
                mHeadPos.set(pose.Translation.x, pose.Translation.y, pose.Translation.z);
            }
            else
            {
                mHeadPos = LLVector3::zero;
            }

            for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
            {
                ovrEyeType eye = mHMD->EyeRenderOrder[eyeIndex];
                mEyeRenderPose[eye] = ovrHmd_GetEyePose(mHMD, eye);
            }

            // These need to be in LL (CFR) coord system
            // Note that the LL coord system uses X forward, Y left, and Z up whereas the Oculus SDK uses the
            // OpenGL coord system of -Z forward, X right, Y up.  To compensate, we retrieve the angles in the Oculus
            // coord system, but change the axes to ours, then negate X and Z to account for the forward left axes
            // being positive in LL, but negative in Oculus.
            // LL X = Oculus -Z, LL Y = Oculus -X, and LL Z = Oculus Y
            // Yaw = rotation around the "up" axis          (LL  Z, Oculus  Y)
            // Pitch = rotation around the left/right axis  (LL -Y, Oculus  X)
            // Roll = rotation around the forward axis      (LL  X, Oculus -Z)
            float r, p, y;
            pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&y, &p, &r);
            mEyeRPY.set(-r, -p, y);
        }
    }
    return gHMD.isFrameInProgress();
}


BOOL LLHMDImplOculus::endFrame()
{
    BOOL res = gHMD.isFrameInProgress();
    if (res)
    {
        ovrTexture* t = const_cast<ovrTexture*>(reinterpret_cast<const ovrTexture*>(mEyeTexture));
        ovrHmd_EndFrame(mHMD, mEyeRenderPose, t);
    }
    gHMD.isFrameInProgress(FALSE);
    return res;
}


F32 LLHMDImplOculus::getAspect() const
{
    return (gHMD.isPostDetectionInitialized() && gHMD.isHMDMode()) ? mAspect : LLViewerCamera::getInstance()->getAspect();
}


void LLHMDImplOculus::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const
{
    if (gHMD.isPostDetectionInitialized())
    {
        roll = mEyeRPY[LLHMD::ROLL];
        pitch = mEyeRPY[LLHMD::PITCH];
        yaw = mEyeRPY[LLHMD::YAW];
    }
    else
    {
        roll = pitch = yaw = 0.0f;
    }
}


void LLHMDImplOculus::resetOrientation()
{
    if (gHMD.isPostDetectionInitialized())
    {
        ovrHmd_RecenterPose(mHMD);
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


S32 LLHMDImplOculus::getViewportWidth() const
{
    return gHMD.isPostDetectionInitialized() ? mEyeTexture[getCurrentOVREye()].OGL.Header.RenderViewport.Size.w : 0;
}


S32 LLHMDImplOculus::getViewportHeight() const
{
    return gHMD.isPostDetectionInitialized() ? mEyeTexture[getCurrentOVREye()].OGL.Header.RenderViewport.Size.h : 0;
}


LLVector3 LLHMDImplOculus::getCurrentEyeCameraOffset() const
{
    if (gHMD.isPostDetectionInitialized() && mCurrentEye != (U32)LLHMD::CENTER_EYE)
    {
        U32 eye = getCurrentOVREye();
        return LLVector3(mEyeRenderDesc[eye].ViewAdjust.x, mEyeRenderDesc[eye].ViewAdjust.y, mEyeRenderDesc[eye].ViewAdjust.z);
    }
    else
    {
        return LLVector3::zero;
    }
}


LLVector3 LLHMDImplOculus::getCurrentEyeOffset(const LLVector3& centerPos) const
{
    LLVector3 ret = centerPos;
    if (gHMD.isPostDetectionInitialized() && mCurrentEye != (U32)LLHMD::CENTER_EYE)
    {
        U32 eye = getCurrentOVREye();
        LLViewerCamera* camera = LLViewerCamera::getInstance();
        LLQuaternion quat = LLQuaternion(camera->getModelview());
        
        LLVector3 trans(mEyeRenderDesc[eye].ViewAdjust.x, mEyeRenderDesc[eye].ViewAdjust.y, mEyeRenderDesc[eye].ViewAdjust.z);
        
        trans *= ~quat;

        ret -= trans;
    }
    return ret;
}


LLVector3 LLHMDImplOculus::getHeadPosition() const
{
    return (gHMD.isPostDetectionInitialized()) ? mHeadPos : LLVector3::zero;
}


LLRenderTarget* LLHMDImplOculus::getCurrentEyeRT()
{
    return mEyeRT[mCurrentEye];
}


LLRenderTarget* LLHMDImplOculus::getEyeRT(U32 eye)
{
    return (eye >= (U32)LLHMD::CENTER_EYE && eye <= (U32)LLHMD::RIGHT_EYE) ? mEyeRT[eye] : NULL;
}


void LLHMDImplOculus::onViewChange(S32 oldMode)
{
    if (mHMD && !gHMD.isHMDDirectMode() && gHMD.isHMDMirror() && gHMD.useMirrorHack())
    {
        LLWindow* windowp = gViewerWindow ? gViewerWindow->getWindow() : NULL;
        if (!windowp)
        {
            return;
        }
        if (gHMD.isHMDMode())
        {
            // HACK!  Move main window to HMD
            windowp->setBorderStyle(FALSE, 0);  // remove title bar
            LLCoordScreen c(mHMD->WindowsPos.x, mHMD->WindowsPos.y);
            windowp->setPosition(c);
            windowp->maximize();
            calculateViewportSettings();
        }
        else
        {
            windowp->setBorderStyle(TRUE, 0);   // re-add title bar
        }
    }
}


void LLHMDImplOculus::showHSW(BOOL show)
{
    if (show && !gHMD.isHSWShowing())
    {
        if (mHMD)
        {
            ovrhmd_EnableHSWDisplaySDKRender(mHMD, FALSE);
        }
        gHMD.isHSWShowing(TRUE);
    }
    if (!show && gHMD.isHSWShowing())
    {
        if (mHMD)
        {
            ovrHmd_DismissHSWDisplay(mHMD);
        }
        gHMD.isHSWShowing(FALSE);
    }
}

#endif // LL_HMD_SUPPORTED
