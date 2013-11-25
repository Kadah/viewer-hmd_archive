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
#include "llui.h"
#include "llview.h"
#include "lltool.h"

#if LL_HMD_SUPPORTED
#include "OVR.h"
#include "Kernel/OVR_Timer.h"
#include "Util/Util_MagCalibration.h"

#if LL_WINDOWS
    #include "llwindowwin32.h"
#elif LL_DARWIN
    #include "llwindowmacosx.h"
    #define IDCONTINUE 1        // Exist on Windows along "IDOK" and "IDCANCEL" but not on Mac
#endif

#include "llhmdimploculus.h"

LLHMD gHMD;

LLHMD::LLHMD()
    : mRenderMode(RenderMode_None)
    , mMainWindowSize(LLHMDImpl::kDefaultHResolution, LLHMDImpl::kDefaultVResolution)
    , mMainClientSize(LLHMDImpl::kDefaultHResolution, LLHMDImpl::kDefaultVResolution)
    , mEyeDepth(0.075f)
    , mUIMagnification(600.0f)
    , mUIEyeDepth(0.6f)
    , mMouseWorldSizeMult(5.0f)
    , mPresetAspect(0.8f)
    , mPresetUIAspect(1.6f)
    , mCalibrateBackgroundTexture(NULL)
    , mCalibrateForegroundTexture(NULL)
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
    if (gHMD.isPreDetectionInitialized())
    {
        return TRUE;
    }
    else if (gHMD.failedInit())
    {
        return FALSE;
    }

    gSavedSettings.getControl("OculusDebugMode")->getSignal()->connect(boost::bind(&onChangeOculusDebugMode));
    onChangeOculusDebugMode();
    gSavedSettings.getControl("OculusUISurfaceArc")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("OculusUISurfaceToroidRadius")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("OculusUISurfaceCrossSectionRadius")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("OculusUISurfaceOffsets")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    onChangeUISurfaceSavedParams();
    gSavedSettings.getControl("OculusEyeDepth")->getSignal()->connect(boost::bind(&onChangeEyeDepth));
    onChangeEyeDepth();
    gSavedSettings.getControl("OculusUIMagnification")->getSignal()->connect(boost::bind(&onChangeUIMagnification));
    onChangeUIMagnification();
    gSavedSettings.getControl("OculusWorldCursorSizeMult")->getSignal()->connect(boost::bind(&onChangeWorldCursorSizeMult));
    onChangeWorldCursorSizeMult();
    gSavedSettings.getControl("OculusUseCalculatedAspect")->getSignal()->connect(boost::bind(&onChangeUseCalculatedAspect));
    onChangeUseCalculatedAspect();

    BOOL preInitResult = mImpl->preInit();
    if (preInitResult)
    {
        // load textures
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

            mCalibrateBackgroundTexture = LLViewerTextureManager::getFetchedTextureFromFile("hmd/test_pattern_bkg.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI);
            mCalibrateForegroundTexture = LLViewerTextureManager::getFetchedTextureFromFile("hmd/cube_test-01.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI);
        }
    }
    else
    {
        gHMD.isPreDetectionInitialized(FALSE);
        gHMD.isPostDetectionInitialized(FALSE);
        gHMD.isInitialized(FALSE);
        gHMD.failedInit(TRUE);  // if we fail pre-init, we're done forever (or at least until client is restarted).
    }

    return preInitResult;
}

void LLHMD::onChangeOculusDebugMode() { gHMD.isDebugMode(gSavedSettings.getBOOL("OculusDebugMode")); }
void LLHMD::onChangeEyeDepth() { gHMD.mEyeDepth = gSavedSettings.getF32("OculusEyeDepth") * .001f; }
void LLHMD::onChangeUIMagnification()
{
    gHMD.mUIMagnification = gSavedSettings.getF32("OculusUIMagnification");
    gHMD.calculateUIEyeDepth();
}

void LLHMD::onChangeUISurfaceSavedParams()
{
    gHMD.mUICurvedSurfaceArc.set(gSavedSettings.getVector3("OculusUISurfaceArc").mV);
    gHMD.mUICurvedSurfaceArc *= F_PI;
    LLVector3 tr = gSavedSettings.getVector3("OculusUISurfaceToroidRadius");
    LLVector3 csr = gSavedSettings.getVector3("OculusUISurfaceCrossSectionRadius");
    gHMD.mUICurvedSurfaceRadius.set(tr[0], tr[1], csr[1], csr[0]);
    gHMD.mUICurvedSurfaceOffsets = gSavedSettings.getVector3("OculusUISurfaceOffsets");
    onChangeUISurfaceShape();
}

void LLHMD::onChangeWorldCursorSizeMult() { gHMD.mMouseWorldSizeMult = gSavedSettings.getF32("OculusWorldCursorSizeMult"); }
void LLHMD::onChangeUseCalculatedAspect()
{
    BOOL oldVal = gHMD.useCalculatedAspect();
    BOOL newVal = gSavedSettings.getBOOL("OculusUseCalculatedAspect");
    if (oldVal != newVal)
    {
        gHMD.useCalculatedAspect(newVal);
        if (gHMD.isHMDMode())
        {
            onChangeUISurfaceShape();
            gHMD.mPresetAspect = (F32)gHMD.getHMDEyeWidth() / (F32)gHMD.getHMDHeight();
            gHMD.mPresetUIAspect = (F32)gHMD.getHMDUIWidth() / (F32)gHMD.getHMDUIHeight();
            LLViewerCamera::getInstance()->setAspect(gHMD.getAspect());
            gViewerWindow->reshape(gHMD.getHMDWidth(), gHMD.getHMDHeight());
        }
    }
}

void LLHMD::saveSettings()
{
    // TODO: when we have correct defaults and settings.xml param names, enable this block.  For now, changes to these settings will revert to defaults after each session.
    //gSavedSettings.setF32("OculusInterpupillaryDistance", gHMD.getInterpupillaryOffset());
    //gSavedSettings.setF32("OculusEyeToScreenDistance", gHMD.getEyeToScreenDistance());
    //gSavedSettings.setBOOL("OculusUseMotionPrediction", gHMD.useMotionPrediction());
    //gSavedSettings.setVector3("OculusUISurfaceToroidRadius", LLVector3(mUICurvedSurfaceRadius[VX], mUICurvedSurfaceRadius[VY], 0.0f));
    //gSavedSettings.setVector3("OculusUISurfaceCrossSectionRadius", LLVector3(mUICurvedSurfaceRadius[VW], mUICurvedSurfaceRadius[VZ], 0.0f));
    //gSavedSettings.setVector3("OculusUISurfaceArc", LLVector3(mUICurvedSurfaceArc[VX] / F_PI, mUICurvedSurfaceArc[VY] / F_PI, 0.0f));
    //gSavedSettings.setVector3("OculusUISurfaceOffsets", mUICurvedSurfaceOffsets);
    //gSavedSettings.setF32("OculusEyeDepth", gHMD.getEyeDepth() * 1000.0f);
    //gSavedSettings.setF32("OculusUIMagnification", gHMD.getUIMagnification());
}

void LLHMD::onChangeUISurfaceShape()
{
    gPipeline.mOculusUISurface = NULL;
}

void LLHMD::shutdown()
{
    mImpl->shutdown();
    mCursorTextures.clear();
}

void LLHMD::onIdle() { mImpl->onIdle(); }

void LLHMD::setRenderMode(U32 mode, bool setFocusWindow)
{
#if LL_HMD_SUPPORTED
    U32 newRenderMode = llclamp(mode, (U32)RenderMode_None, (U32)RenderMode_Last);
    if (newRenderMode != mRenderMode)
    {
        LLWindow* windowp = gViewerWindow->getWindow();
        if (!windowp || (newRenderMode == RenderMode_HMD && (!isInitialized() || !isHMDConnected())))
        {
            return;
        }
        U32 oldMode = mRenderMode;
        mRenderMode = newRenderMode;
        switch (oldMode)
        {
        case RenderMode_HMD:
        case RenderMode_ScreenStereo:
            {
                switch (mRenderMode)
                {
                case RenderMode_HMD:
                    // switching from ScreenStereo to HMD
                    // not currently possible, but could change, so might as well handle it
                    {
                        // first ensure that we CAN render to the HMD (i.e. it's initialized, we have a valid window,
                        // the HMD is still connected, etc.
                        if (!gHMD.isPostDetectionInitialized() || !gHMD.isHMDConnected())
                        {
                            // can't render to the HMD window, so abort
                            mRenderMode = RenderMode_ScreenStereo;
                            return;
                        }
                        gViewerWindow->reshape(mImpl->getHMDWidth(), mImpl->getHMDHeight());
                        if (!setRenderWindowHMD())
                        {
                            // Somehow, we've lost the HMD window, so just recreate it
                            setRenderWindowMain();
                            if (!mImpl->postDetectionInit() || !setRenderWindowHMD())
                            {
                                // still can't create the window - abort
                                setRenderMode(RenderMode_ScreenStereo, setFocusWindow);
                                return;
                            }
                        }
                    }
                    break;
                case RenderMode_ScreenStereo:
                    // switching from HMD to ScreenStereo
                    // not much to do here except resize the main window
                    {
                        setRenderWindowMain();
                        windowp->setSize(getHMDClientSize());
                    }
                    break;
                case RenderMode_None:
                default:
                    // switching from isHMDMode() to !isHMDMode()
                    {
                        if (oldMode == RenderMode_HMD)
                        {
                            setRenderWindowMain();
                        }
                        LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
                        windowp->setPosition(getMainWindowPos());
                        windowp->setSize(getMainClientSize());
                        if (gAgentCamera.cameraFirstPerson())
                        {
                            gAgentCamera.changeCameraToMouselook();
                        }
                        if (shouldShowCalibrationUI())
                        {
                            LLUI::getRootView()->getChildView("menu_stack")->setVisible(!shouldShowDepthVisual());
                        }
                    }
                    break;
                }
            }
            break;
        case RenderMode_None:
        default:
            {
                // clear the main window and save off size settings
                windowp->getSize(&mMainWindowSize);
                windowp->getSize(&mMainClientSize);
                windowp->getPosition(&mMainWindowPos);
                renderUnusedMainWindow();
                mPresetAspect = ((F32)getHMDEyeWidth() / (F32)getHMDHeight());
                mPresetUIAspect = (F32)gHMD.getHMDUIWidth() / (F32)gHMD.getHMDUIHeight();
                switch (mRenderMode)
                {
                case RenderMode_HMD:
                    // switching from Normal to HMD
                    {
                        // first ensure that we CAN render to the HMD (i.e. it's initialized, we have a valid window,
                        // the HMD is still connected, etc.
                        if (!gHMD.isPostDetectionInitialized() || !gHMD.isHMDConnected())
                        {
                            // can't render to the HMD window, so abort
                            mRenderMode = RenderMode_None;
                            return;
                        }
                        LLViewerCamera::getInstance()->setDefaultFOV(gHMD.getVerticalFOV());
                        gViewerWindow->reshape(mImpl->getHMDWidth(), mImpl->getHMDHeight());
                        if (!setRenderWindowHMD())
                        {
                            // Somehow, we've lost the HMD window, so just recreate it
                            setRenderWindowMain();
                            if (!mImpl->postDetectionInit() || !setRenderWindowHMD())
                            {
                                // still can't create the window - abort
                                setRenderMode(RenderMode_ScreenStereo, setFocusWindow);
                                return;
                            }
                        }
                    }
                    break;
                case RenderMode_ScreenStereo:
                    // switching from Normal to ScreenStereo
                    {
                        LLViewerCamera::getInstance()->setDefaultFOV(gHMD.getVerticalFOV());
                        windowp->setSize(getHMDClientSize());
                    }
                    break;
                }
                if (gAgentCamera.cameraMouselook())
                {
                    gAgentCamera.changeCameraToFirstPerson();
                }
                if (gHMD.shouldShowCalibrationUI())
                {
                    LLUI::getRootView()->getChildView("menu_stack")->setVisible(!gHMD.shouldShowDepthVisual() && gHMD.isCalibrated());
                }
            }
            break;
        }
        if (setFocusWindow)
        {
            if (mRenderMode == RenderMode_HMD)
            {
                setFocusWindowHMD();
            }
            else
            {
                setFocusWindowMain();
            }
        }
        if (isInitialized() && isHMDMode() && !isCalibrated())
        {
            mImpl->BeginManualCalibration();
        }
    }
#else
    mRenderMode = RenderMode_None;
#endif
}


BOOL LLHMD::setRenderWindowMain()
{
    return gViewerWindow->getWindow()->setRenderWindow(0, gHMD.isMainFullScreen());
}


BOOL LLHMD::setRenderWindowHMD()
{
    // TODO: check to see if HMD window is valid, has not been destroyed, etc.  If it has,
    // re-initialize it here
    return gViewerWindow->getWindow()->setRenderWindow(1, TRUE);
}


void LLHMD::setFocusWindowMain()
{
    isChangingRenderContext(TRUE);
    if (isHMDMode())
    {
        gViewerWindow->getWindow()->setFocusWindow(0, TRUE, mImpl->getHMDWidth(), mImpl->getHMDHeight());
    }
    else
    {
        gViewerWindow->getWindow()->setFocusWindow(0, FALSE, 0, 0);
    }
}


void LLHMD::setFocusWindowHMD()
{
    // TODO: check to see if HMD window is valid, has not been destroyed, etc.  If it has,
    // re-initialize it here
    isChangingRenderContext(TRUE);
    gViewerWindow->getWindow()->setFocusWindow(1, TRUE, mImpl->getHMDWidth(), mImpl->getHMDHeight());
}


void LLHMD::onAppFocusGained()
{
    if (isChangingRenderContext())
    {
        if (isHMDMode())
        {
            gViewerWindow->getWindow()->setMouseClipping(TRUE);
        }
        isChangingRenderContext(FALSE);
    }
    else
    {
        // if we've tried to focus on the HMD window while not intentionally swapping contexts, we'll likely get a BSOD.
        // Note that since the HMD window has no task-bar icon, this should not be possible, but users will find a way...
        if (mRenderMode == (U32)RenderMode_HMD)
        {
            setRenderMode(RenderMode_None);
            // TODO: kill HMD window
        }
        else if (mRenderMode == (U32)RenderMode_ScreenStereo)
        {
            gViewerWindow->getWindow()->setMouseClipping(TRUE);
        }
    }
}


void LLHMD::onAppFocusLost()
{
    if (!isChangingRenderContext() && mRenderMode == (U32)RenderMode_HMD)
    {
        // Make sure we change the render window to main so that we avoid BSOD in the graphics drivers when
        // it tries to render to a (now) invalid renderContext.
        setRenderWindowMain();
        setRenderMode(RenderMode_None, false);
        // TODO: destroy HMD window
    }
}


void LLHMD::renderUnusedMainWindow()
{
    if (gHMD.getRenderMode() == LLHMD::RenderMode_HMD
        && gHMD.isInitialized()
        && gHMD.isHMDConnected()
        && gViewerWindow
        && gViewerWindow->getWindow()
#if LL_DARWIN
        && !gSavedSettings.getBOOL("OculusUseMirroring")
#endif
       )
    {
        if (gHMD.setRenderWindowMain())
        {
            gViewerWindow->getWindowViewportRaw(gGLViewport, gHMD.getMainWindowWidth(), gHMD.getMainWindowHeight());
            glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            gViewerWindow->getWindow()->swapBuffers();
        }
    }
}


void LLHMD::renderUnusedHMDWindow()
{
    if (gHMD.isInitialized()
        && gHMD.isHMDConnected()
        && gHMD.getRenderMode() != LLHMD::RenderMode_HMD
        && gViewerWindow
        && gViewerWindow->getWindow())
    {
        if (gHMD.setRenderWindowHMD())
        {
            gViewerWindow->getWindowViewportRaw(gGLViewport, gHMD.getHMDWidth(), gHMD.getHMDHeight());
            glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            // write text "press CTRL-SHIFT-D to switch to HMD"
            gViewerWindow->getWindow()->swapBuffers();
        }
    }
}


U32 LLHMD::getCurrentEye() const { return mImpl->getCurrentEye(); }
void LLHMD::setCurrentEye(U32 eye) { mImpl->setCurrentEye(eye); }
void LLHMD::getViewportInfo(S32& x, S32& y, S32& w, S32& h) { mImpl->getViewportInfo(x, y, w, h); }
S32 LLHMD::getHMDWidth() const { return mImpl->getHMDWidth(); }
S32 LLHMD::getHMDEyeWidth() const { return mImpl->getHMDEyeWidth(); }
S32 LLHMD::getHMDHeight() const { return mImpl->getHMDHeight(); }
S32 LLHMD::getHMDUIWidth() const { return mImpl->getHMDUIWidth(); }
S32 LLHMD::getHMDUIHeight() const { return mImpl->getHMDUIHeight(); }
F32 LLHMD::getPhysicalScreenWidth() const { return mImpl->getPhysicalScreenWidth(); }
F32 LLHMD::getPhysicalScreenHeight() const { return mImpl->getPhysicalScreenHeight(); }
F32 LLHMD::getInterpupillaryOffset() const { return mImpl->getInterpupillaryOffset(); }
void LLHMD::setInterpupillaryOffset(F32 f) { mImpl->setInterpupillaryOffset(f); }
F32 LLHMD::getInterpupillaryOffsetDefault() const { return mImpl->getInterpupillaryOffsetDefault(); }
F32 LLHMD::getLensSeparationDistance() const { return mImpl->getLensSeparationDistance(); }
F32 LLHMD::getEyeToScreenDistance() const { return mImpl->getEyeToScreenDistance(); }
F32 LLHMD::getEyeToScreenDistanceDefault() const { return mImpl->getEyeToScreenDistanceDefault(); }
void LLHMD::setEyeToScreenDistance(F32 f)
{
    mImpl->setEyeToScreenDistance(f);
    calculateUIEyeDepth();
    gPipeline.mOculusUISurface = NULL;
}
LLVector4 LLHMD::getDistortionConstants() const { return mImpl->getDistortionConstants(); }
F32 LLHMD::getXCenterOffset() const { return mImpl->getXCenterOffset(); }
F32 LLHMD::getYCenterOffset() const { return mImpl->getYCenterOffset(); }
F32 LLHMD::getDistortionScale() const { return mImpl->getDistortionScale(); }
LLQuaternion LLHMD::getHMDOrient() const { return mImpl->getHMDOrient(); }
LLQuaternion LLHMD::getHeadRotationCorrection() const { return mImpl->getHeadRotationCorrection(); }
void LLHMD::addHeadRotationCorrection(LLQuaternion quat) { return mImpl->addHeadRotationCorrection(quat); }
void LLHMD::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { mImpl->getHMDRollPitchYaw(roll, pitch, yaw); }
F32 LLHMD::getVerticalFOV() const { return mImpl->getVerticalFOV(); }
F32 LLHMD::getAspect() { return useCalculatedAspect() ? mImpl->getAspect() : mPresetAspect; }
BOOL LLHMD::useMotionPrediction() const { return mImpl->useMotionPrediction(); }
void LLHMD::useMotionPrediction(BOOL b) { mImpl->useMotionPrediction(b); }
BOOL LLHMD::useMotionPredictionDefault() const { return mImpl->useMotionPredictionDefault(); }
F32 LLHMD::getMotionPredictionDelta() const { return mImpl->getMotionPredictionDelta(); }
F32 LLHMD::getMotionPredictionDeltaDefault() const { return mImpl->getMotionPredictionDeltaDefault(); }
void LLHMD::setMotionPredictionDelta(F32 f) { mImpl->setMotionPredictionDelta(f); }
//LLVector4 LLHMD::getChromaticAberrationConstants() const { return mImpl->getChromaticAberrationConstants(); }
F32 LLHMD::getOrthoPixelOffset() const { return mImpl->getOrthoPixelOffset(); }
void LLHMD::BeginManualCalibration() { isCalibrated(FALSE); mImpl->BeginManualCalibration(); }
const std::string& LLHMD::getCalibrationText() const { return mImpl->getCalibrationText(); }

void LLHMD::calculateUIEyeDepth()
{
    //const F32 LLHMDImpl::kUIEyeDepthMult = -14.285714f;
    //// formula:  X = N / ((DESD - MESD) * 1000.0)
    //// WHERE
    ////  X = result 
    ////  N = desired UIEyeDepth when at default Eye-To-Screen-Distance
    ////  DESD = default_eye_to_screen_dist
    ////  MESD = eye_to_screen_dist_where_X_should_be_zero
    ////  in the current case:
    ////  -14.285714 = 600 / ((0.041 - 0.083) * 1000)
    F32 eyeDepthMult = mUIMagnification / ((getEyeToScreenDistanceDefault() - 0.083f) * 1000.0f);
    mUIEyeDepth = ((mImpl->getEyeToScreenDistance() - 0.083f) * eyeDepthMult);
}

void LLHMD::setBaseModelView(F32* m)
{
    glh::matrix4f bmvInv(m);
    bmvInv = bmvInv.inverse();
    for (int i = 0; i < 16; ++i)
    {
        mBaseModelView[i] = m[i];
        mBaseModelViewInv[i] = bmvInv.m[i];
    }
}

void LLHMD::setUIModelView(F32* m)
{
    glh::matrix4f uivInv(m);
    uivInv = uivInv.inverse();
    for (int i = 0; i < 16; ++i)
    {
        mUIModelView[i] = m[i];
        mUIModelViewInv[i] = uivInv.m[i];
    }
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
    const U32 resX = 32;
    const U32 resY = 16;
    const U32 numVerts = resX * resY;
    const U32 numIndices = 6 * (resX - 1) * (resY - 1);
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
    LLVector4 t2;
    LLVector2 uv;
    for (U32 i = 0; i < resY; ++i)
    {
        F32 va = (F_PI - (mUICurvedSurfaceArc[VY] * 0.5f)) + ((F32)i * dy);
        for (U32 j = 0; j < resX; ++j)
        {
            F32 ha = (mUICurvedSurfaceArc[VX] * -0.5f) + ((F32)j * dx);
            getUISurfaceCoordinates(ha, va, t2, &uv);
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

    buff->flush();
    return buff;
}

void LLHMD::getUISurfaceCoordinates(F32 ha, F32 va, LLVector4& pos, LLVector2* uv)
{
    F32 cva = cos(va);
    pos.set(    (sin(ha) * (mUICurvedSurfaceRadius[0] - (cva * mUICurvedSurfaceRadius[3]))) + mUICurvedSurfaceOffsets[VX],
                (mUICurvedSurfaceRadius[2] * -sin(va)) + mUICurvedSurfaceOffsets[VY],
                (-1.0f * ((mUICurvedSurfaceRadius[1] * cos(ha)) - (cva * mUICurvedSurfaceRadius[3]))) + mUICurvedSurfaceOffsets[VZ],
                1.0f);
    if (uv)
    {
        uv->set((ha - (mUICurvedSurfaceArc[VX] * -0.5f))/mUICurvedSurfaceArc[VX], (va - (F_PI - (mUICurvedSurfaceArc[VY] * 0.5f)))/mUICurvedSurfaceArc[VY]);
    }
}

void LLHMD::updateHMDMouseInfo(S32 ui_x, S32 ui_y)
{
    if (!isHMDMode())
    {
        mMouseWorld.set(0.0f, 0.0f, 0.0f);
        return;
    }

    // 1. determine horizontal and vertical angle on toroid based on ui_x, ui_y
    F32 uiw = (F32)gViewerWindow->getWorldViewRectScaled().getWidth();
    F32 uih = (F32)gViewerWindow->getWorldViewRectScaled().getHeight();
    F32 nx = llclamp((F32)ui_x / (F32)uiw, 0.0f, 1.0f);
    F32 ny = llclamp((F32)ui_y / (F32)uih, 0.0f, 1.0f);
    F32 ha = ((mUICurvedSurfaceArc[VX] * -0.5f) + (nx * mUICurvedSurfaceArc[VX]));
    F32 va = (F_PI - (mUICurvedSurfaceArc[VY] * 0.5f)) + (ny * mUICurvedSurfaceArc[VY]);

    // 2. determine eye-space x,y,z for ha, va (-z forward/depth)
    LLVector4 eyeSpacePos;
    getUISurfaceCoordinates(ha, va, eyeSpacePos);

    // 3. convert eye-space to world coordinates (taking into account the ui-magnification that essentially 
    //    translates the view forward (or backward, depending on the mag level) along the axis going into the 
    //    center of the UI surface).  
    glh::matrix4f uivInv(mUIModelViewInv);
    glh::vec4f w(eyeSpacePos.mV);
    uivInv.mult_matrix_vec(w);
    mMouseWorld.set(w[VX], w[VY], w[VZ]);

    //// Note the below ... ALMOST works, but because the UI surface is not a regular curve, it is somewhat off.
    //// c'est la vie.  Luckily, it turns out we only need mMouseWorld.
    //// 4. Finally, take head-rotations and the camera's base pitch into account to project the world-coord from
    ////    step 3 into screen space.
    //glh::matrix4f bmv(mBaseModelView);
    //glh::matrix4f proj(mBaseProjection);
    //glh::matrix4f mvp = proj * bmv;
    //glh::vec4f asp(w[VX], w[VY], w[VZ], 1.0f);
    //mvp.mult_matrix_vec(asp);
    //asp[VX] /= asp[VW];
    //asp[VY] /= asp[VW];
    ////asp[VZ] /= asp[VW];

    //F32 center_x = uiw * 0.5f;
    //F32 center_y = uih * 0.5f;
    //mMouseWin.mX = llround(((asp[VX] + 1.0f) * 0.5f * uiw) + (center_x * asp[VX] * 0.25f));
    //mMouseWin.mY = llround(((asp[VY] + 1.0f) * 0.5f * uih) + (center_y * asp[VY] * 2.0f));
}


BOOL LLHMD::handleMouseIntersectOverride(LLMouseHandler* mh)
{
    if (!mh || !gHMD.isHMDMode())
    {
        return FALSE;
    }

    LLView* ui_view = dynamic_cast<LLView*>(mh);
    if (ui_view)
    {
        gHMD.cursorIntersectsUI(TRUE);
        return TRUE;
    }

    LLTool* tool = dynamic_cast<LLTool*>(mh);
    if (tool && tool->hasMouseIntersectOverride())
    {
        if (tool->isMouseIntersectInUISpace())
        {
            gHMD.cursorIntersectsUI(TRUE);
        }
        else
        {
            gHMD.cursorIntersectsWorld(TRUE);
            if (tool->hasMouseIntersectGlobal())
            {
                LLVector3 newMouseIntersect = gAgent.getPosAgentFromGlobal(tool->getMouseIntersectGlobal());
                gHMD.setMouseWorldRaycastIntersection(newMouseIntersect);
            }
        }
        return TRUE;
    }

    return FALSE;
}

#endif // LL_HMD_SUPPORTED
