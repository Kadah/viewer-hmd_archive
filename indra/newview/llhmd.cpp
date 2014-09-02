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
#include "llhmdimploculus.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llfloatercamera.h"
#include "llfloaterreg.h"
#include "llmoveview.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "lltoolgun.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llui.h"
#include "llview.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewershadermgr.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwindow.h"
#if LL_DARWIN
    #include "llwindowmacosx.h"
#endif
#include "llviewerdisplay.h"
#include "pipeline.h"
#include "raytrace.h"

// external methods
void drawBox(const LLVector3& c, const LLVector3& r);

const F32 LLHMDImpl::kDefaultHScreenSize = 0.14976f;
const F32 LLHMDImpl::kDefaultVScreenSize = 0.0936f;
const F32 LLHMDImpl::kDefaultInterpupillaryOffset = 0.064f;
const F32 LLHMDImpl::kDefaultLenSeparationDistance = 0.0635f;
const F32 LLHMDImpl::kDefaultEyeToScreenDistance = 0.047f;  // A lens = 0.047f, B lens = 0.044f, C lens = 0.040f.   Default of 0.041f from the SDK is just WRONG and will cause visual distortion (Rift-46)
const F32 LLHMDImpl::kDefaultDistortionConstant0 = 1.0f;
const F32 LLHMDImpl::kDefaultDistortionConstant1 = 0.22f;
const F32 LLHMDImpl::kDefaultDistortionConstant2 = 0.24f;
const F32 LLHMDImpl::kDefaultDistortionConstant3 = 0.0f;
const F32 LLHMDImpl::kDefaultXCenterOffset = 0.152f;
const F32 LLHMDImpl::kDefaultYCenterOffset = 0.0f;
const F32 LLHMDImpl::kDefaultDistortionScale = 1.7146f;
const F32 LLHMDImpl::kDefaultOrthoPixelOffset = 0.1775f; // -0.1775f Right Eye
const F32 LLHMDImpl::kDefaultVerticalFOVRadians = 2.196863;
const F32 LLHMDImpl::kDefaultAspect = 0.8f;


LLHMD gHMD;

LLHMD::LLHMD()
    : mImpl(NULL)
    , mFlags(0)
    , mRenderMode(RenderMode_None)
    , mInterpupillaryDistance(0.064f)
    , mUIEyeDepth(0.65f)
    , mUIShapePreset(0)
    , mNextUserPresetIndex(1)
    , mMainWindowFOV(DEFAULT_FIELD_OF_VIEW)
    , mMainWindowAspect(1.658793f)
    , mMouseWorldSizeMult(5.0f)
    , mMouselookControlMode(0)
    , mMouselookRotThreshold(45.0f * DEG_TO_RAD)
    , mMouselookRotMax(30.0f * DEG_TO_RAD)
    , mMouselookTurnSpeedMax(0.1f)
    , mStereoCameraFOV(DEFAULT_FIELD_OF_VIEW)
    , mStereoCullCameraFOV(0.0f)
    , mStereoCullCameraAspect(0.0f)
    , mTimewarpIntervalSeconds(0.0001f)
{
    memset(&mUIShape, 0, sizeof(UISurfaceShapeSettings));
    mUIShape.mPresetType = (U32)LLHMD::kCustom;
    mUIShape.mPresetTypeIndex = 0;
    // "Custom" preset is always in index 0
    mUIPresetValues.push_back(mUIShape);
    memset(mProjectionOffset, 0, sizeof(F32) * 4 * 4);
}


LLHMD::~LLHMD()
{
    if (mImpl)
    {
        delete mImpl;
        mImpl = NULL;
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
    BOOL preInitResult = FALSE;

#if LL_HMD_SUPPORTED
    if (!mImpl)
    {
        mImpl = new LLHMDImplOculus();
    }

    gSavedSettings.getControl("HMDAdvancedMode")->getSignal()->connect(boost::bind(&onChangeHMDAdvancedMode));
    onChangeHMDAdvancedMode();
    gSavedSettings.getControl("HMDLowPersistence")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDPixelLuminanceOverdrive")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDUseMotionPrediction")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDTimewarp")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDTimewarpIntervalSeconds")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDTimewarpNoJit")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDEnablePositionalTracking")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    gSavedSettings.getControl("HMDPixelDensity")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
    onChangeRenderSettings();
    gSavedSettings.getControl("HMDDynamicResolutionScaling")->getSignal()->connect(boost::bind(&onChangeDynamicResolutionScaling));
    onChangeDynamicResolutionScaling();
    gSavedSettings.getControl("HMDUISurfaceArcHorizontal")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("HMDUISurfaceArcVertical")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("HMDUISurfaceToroidWidth")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("HMDUISurfaceToroidDepth")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("HMDUISurfaceToroidCSWidth")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("HMDUISurfaceToroidCSHeight")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    gSavedSettings.getControl("HMDUISurfaceOffsets")->getSignal()->connect(boost::bind(&onChangeUISurfaceSavedParams));
    onChangeUISurfaceSavedParams();
    gSavedSettings.getControl("HMDUIMagnification")->getSignal()->connect(boost::bind(&onChangeUIMagnification));
    onChangeUIMagnification();
    gSavedSettings.getControl("HMDUIPresetValues")->getSignal()->connect(boost::bind(&onChangePresetValues));
    onChangePresetValues();
    gSavedSettings.getControl("HMDUIShapePreset")->getSignal()->connect(boost::bind(&onChangeUIShapePreset));
    onChangeUIShapePreset();
    gSavedSettings.getControl("HMDWorldCursorSizeMult")->getSignal()->connect(boost::bind(&onChangeWorldCursorSizeMult));
    onChangeWorldCursorSizeMult();
    gSavedSettings.getControl("HMDMouselookRotThreshold")->getSignal()->connect(boost::bind(&onChangeMouselookSettings));
    gSavedSettings.getControl("HMDMouselookRotMax")->getSignal()->connect(boost::bind(&onChangeMouselookSettings));
    gSavedSettings.getControl("HMDMouselookTurnSpeedMax")->getSignal()->connect(boost::bind(&onChangeMouselookSettings));
    onChangeMouselookSettings();
    gSavedSettings.getControl("HMDMouselookControlMode")->getSignal()->connect(boost::bind(&onChangeMouselookControlMode));
    onChangeMouselookControlMode();
    gSavedSettings.getControl("HMDAllowTextRoll")->getSignal()->connect(boost::bind(&onChangeAllowTextRoll));
    onChangeAllowTextRoll();

    
    preInitResult = mImpl->preInit();
    if (preInitResult)
    {
        // load textures
        mCursorTextures.clear();
        //UI_CURSOR_ARROW
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_arrow.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_WAIT
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_waiting.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_HAND
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_hand.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_IBEAM
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_ibeam.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_CROSS
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_cross.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_SIZENWSE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_sizenwse.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_SIZENESW
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_sizenesw.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_SIZEWE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_sizewe.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_SIZENS
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_sizens.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_NO
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_no.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_WORKING
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/cursor_working.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLGRAB
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolgrab.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLLAND
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolland.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLFOCUS
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolfocus.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLCREATE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolcreate.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_ARROWDRAG
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowdrag.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_ARROWCOPY
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowcop.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_ARROWDRAGMULTI
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llarrowdragmulti.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_ARROWCOPYMULTI
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/arrowcopmulti.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_NOLOCKED
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llnolocked.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_ARROWLOCKED
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llarrowlocked.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_GRABLOCKED
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llgrablocked.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLTRANSLATE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltooltranslate.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLROTATE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolrotate.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLSCALE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolscale.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLCAMERA
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolcamera.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLPAN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpan.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLZOOMIN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolzoomin.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLPICKOBJECT3
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolpickobject3.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLPLAY
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolplay.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLPAUSE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolpause.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_TOOLMEDIAOPEN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolmediaopen.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.0f, 0.0f));
        //UI_CURSOR_PIPETTE
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolpipette.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLSIT
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolsit.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLBUY
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolbuy.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLOPEN
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/toolopen.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLPATHFINDING
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfinding.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLPATHFINDING_PATH_START
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathstart.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathstartadd.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLPATHFINDING_PATH_END
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathend.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/lltoolpathfindingpathendadd.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));
        //UI_CURSOR_TOOLNO
        mCursorTextures.push_back(LLViewerTextureManager::getFetchedTextureFromFile("hmd/llno.tga", FTT_LOCAL_FILE, FALSE, LLViewerFetchedTexture::BOOST_UI));
        mCursorHotSpotOffsets.push_back(LLVector2(0.5f, 0.5f));

        gHMD.isHMDAllowed(gPipeline.getUseVertexShaders());
    }
    else
#endif
    {
        gHMD.isHMDAllowed(FALSE);
        gHMD.isPreDetectionInitialized(FALSE);
        gHMD.isPostDetectionInitialized(FALSE);
        gHMD.failedInit(TRUE);  // if we fail pre-init, we're done forever (or at least until client is restarted).
    }

    return preInitResult;
}


void LLHMD::onChangeHMDAdvancedMode() { gHMD.isAdvancedMode(gSavedSettings.getBOOL("HMDAdvancedMode")); }
void LLHMD::onChangeUIMagnification() { if (!gHMD.isSavingSettings()) { gHMD.setUIMagnification(gSavedSettings.getF32("HMDUIMagnification")); } }


void LLHMD::onChangeUISurfaceSavedParams()
{
    if (!gHMD.isSavingSettings())
    {
        gHMD.mUIShape.mPresetType = (U32)LLHMD::kCustom;
        gHMD.mUIShape.mPresetTypeIndex = 1;
        gHMD.mUIShape.mArcHorizontal = gSavedSettings.getF32("HMDUISurfaceArcHorizontal");
        gHMD.mUIShape.mArcVertical = gSavedSettings.getF32("HMDUISurfaceArcVertical");
        gHMD.mUIShape.mToroidRadiusWidth = gSavedSettings.getF32("HMDUISurfaceToroidWidth");
        gHMD.mUIShape.mToroidRadiusDepth = gSavedSettings.getF32("HMDUISurfaceToroidDepth");
        gHMD.mUIShape.mToroidCrossSectionRadiusWidth = gSavedSettings.getF32("HMDUISurfaceToroidCSWidth");
        gHMD.mUIShape.mToroidCrossSectionRadiusHeight = gSavedSettings.getF32("HMDUISurfaceToroidCSHeight");
        LLVector3 offsets = gSavedSettings.getVector3("HMDUISurfaceOffsets");
        gHMD.mUIShape.mOffsetX = offsets[VX];
        gHMD.mUIShape.mOffsetY = offsets[VY];
        gHMD.mUIShape.mOffsetZ = offsets[VZ];
        onChangeUISurfaceShape();
    }
}


void LLHMD::onChangePresetValues()
{
    LLSD raw = gSavedSettings.getLLSD("HMDUIPresetValues");
    gHMD.mUIPresetValues.clear();
    {
        // "Custom" preset is always in index 0
        UISurfaceShapeSettings settings;
        memset(&settings, 0, sizeof(UISurfaceShapeSettings));
        settings.mPresetType = (U32)LLHMD::kCustom;
        settings.mPresetTypeIndex = 1;
        gHMD.mUIPresetValues.push_back(settings);
    }
    U32 nextDefault = 1;
    gHMD.mNextUserPresetIndex = 1;
    for (LLSD::array_const_iterator it1 = raw.beginArray(), it1End = raw.endArray(); it1 != it1End; ++it1)
    {
        const LLSD& entry = *it1;
        UISurfaceShapeSettings settings;
        memset(&settings, 0, sizeof(UISurfaceShapeSettings));
        for (LLSD::map_const_iterator it2 = entry.beginMap(), it2End = entry.endMap(); it2 != it2End; ++it2)
        {
            const std::string& key = it2->first;
            if (!strncasecmp(key.c_str(), "PresetType", 32))
            {
                if (!strncasecmp(it2->second.asString().c_str(), "Default", 16))
                {
                    settings.mPresetType = (U32)LLHMD::kDefault;
                    settings.mPresetTypeIndex = nextDefault++;
                }
                else
                {
                    settings.mPresetType = (U32)LLHMD::kUser;
                    settings.mPresetTypeIndex = gHMD.mNextUserPresetIndex++;
                }
            }
            else if (!strncasecmp(key.c_str(), "ToroidRadiusWidth", 32))
            {
                settings.mToroidRadiusWidth = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "ToroidRadiusDepth", 32))
            {
                settings.mToroidRadiusDepth = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "ToroidCSRadiusWidth", 32))
            {
                settings.mToroidCrossSectionRadiusWidth = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "ToroidCSRadiusHeight", 32))
            {
                settings.mToroidCrossSectionRadiusHeight = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "ArcHorizontal", 32))
            {
                settings.mArcHorizontal = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "ArcVertical", 32))
            {
                settings.mArcVertical = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "UIMagnification", 32))
            {
                settings.mUIMagnification = it2->second.asReal();
            }
            else if (!strncasecmp(key.c_str(), "Offsets", 32))
            {
                settings.mOffsetX = it2->second[0].asReal();
                settings.mOffsetY = it2->second[1].asReal();
                settings.mOffsetZ = it2->second[2].asReal();
            }
        }
        gHMD.mUIPresetValues.push_back(settings);
    }
}


void LLHMD::onChangeUIShapePreset() { if (!gHMD.isSavingSettings()) { gHMD.setUIShapePresetIndex(gSavedSettings.getS32("HMDUIShapePreset")); } }
void LLHMD::onChangeWorldCursorSizeMult() { if (!gHMD.isSavingSettings()) { gHMD.mMouseWorldSizeMult = gSavedSettings.getF32("HMDWorldCursorSizeMult"); } }
void LLHMD::onChangeMouselookControlMode() { if (!gHMD.isSavingSettings()) { gHMD.setMouselookControlMode(gSavedSettings.getS32("HMDMouselookControlMode")); } }
void LLHMD::onChangeDynamicResolutionScaling() { if (!gHMD.isSavingSettings()) { gHMD.useDynamicResolutionScaling(gSavedSettings.getBOOL("HMDDynamicResolutionScaling")); } }
void LLHMD::onChangeAllowTextRoll() { if (!gHMD.isSavingSettings()) { gHMD.allowTextRoll(gSavedSettings.getBOOL("HMDAllowTextRoll")); } }


void LLHMD::onChangeMouselookSettings()
{
    if (!gHMD.isSavingSettings())
    { 
        gHMD.mMouselookRotThreshold = llclamp(gSavedSettings.getF32("HMDMouselookRotThreshold") * DEG_TO_RAD, (10.0f * DEG_TO_RAD), (80.0f * DEG_TO_RAD));
        gHMD.mMouselookRotMax = llclamp(gSavedSettings.getF32("HMDMouselookRotMax") * DEG_TO_RAD, (1.0f * DEG_TO_RAD), (90.0f * DEG_TO_RAD));
        gHMD.mMouselookTurnSpeedMax = llclamp(gSavedSettings.getF32("HMDMouselookTurnSpeedMax"), 0.01f, 0.5f);
    }
}


void LLHMD::onChangeUISurfaceShape() { gPipeline.mHMDUISurface = NULL; }


void LLHMD::onChangeRenderSettings()
{
    if (!gHMD.isSavingSettings())
    {
        gHMD.useLowPersistence(gSavedSettings.getBOOL("HMDLowPersistence"));
        gHMD.usePixelLuminanceOverdrive(gSavedSettings.getBOOL("HMDPixelLuminanceOverdrive"));
        gHMD.useMotionPrediction(gSavedSettings.getBOOL("HMDUseMotionPrediction"));
        gHMD.isTimewarpEnabled(gSavedSettings.getBOOL("HMDTimewarp"));
        gHMD.setTimewarpIntervalSeconds(gSavedSettings.getF32("HMDTimewarpIntervalSeconds"));
        gHMD.useDynamicResolutionScaling(gSavedSettings.getBOOL("HMDDynamicResolutionScaling"));
        gHMD.renderSettingsChanged(TRUE);
    }
}


void LLHMD::shutdown()
{
    if (mImpl)
    {
        delete mImpl;
        mImpl = NULL;
    }
    mCursorTextures.clear();
    isPreDetectionInitialized(FALSE);
    isPostDetectionInitialized(FALSE);
    failedInit(FALSE);
}


void LLHMD::onIdle()
{
    if (mImpl)
    {
        mLastRollPitchYaw.setVec(mImpl->getRoll(), mImpl->getPitch(), mImpl->getYaw());
        mImpl->onIdle();
        if (isHMDMode() && gAgentCamera.cameraMouselook() && getMouselookControlMode() == (S32)kMouselookControl_Linked)
        {
            LLVector3 atLeveled = gAgent.getAtAxis();
            atLeveled[VZ] = 0.0f;
            atLeveled.normalize();
            gAgent.resetAxes(atLeveled);

            gAgent.pitch(mImpl->getPitch());

            LLVector3 skyward = gAgent.getReferenceUpVector();
            F32 yaw = mImpl->getYaw();
            F32 dy = yaw - mLastRollPitchYaw[VZ];
            if (yaw < -mMouselookRotThreshold || yaw > mMouselookRotThreshold)
            {
                F32 yt = llclamp(llabs(yaw) - mMouselookRotThreshold, 0.0f, mMouselookRotMax) * ((yaw >= 0.0f) ? 1.0f : -1.0f);
                F32 v = llclamp(gAgent.getVelocity().lengthSquared(), 0.0f, 16.0f);
                if (v <= 1.0f)
                {
                    // rotate faster when stopped or moving very slowly.
                    yt *= 2.0f;
                }
                dy += (mMouselookTurnSpeedMax * ((1.0f / mMouselookRotMax) * yt));
            }
            gAgent.rotate(dy, skyward[VX], skyward[VY], skyward[VZ]);
        }
    }
}


void LLHMD::setRenderMode(U32 mode, bool setFocusWindow)
{
#if LL_HMD_SUPPORTED
    U32 newRenderMode = llclamp(mode, (U32)RenderMode_None, (U32)RenderMode_Last);
    if (newRenderMode != mRenderMode)
    {
        LLWindow* windowp = gViewerWindow->getWindow();
        if (!windowp || (newRenderMode == RenderMode_HMD && (!isPostDetectionInitialized() || !isHMDConnected() || !isHMDSensorConnected())))
        {
            return;
        }

        LLViewerCamera* pCamera = LLViewerCamera::getInstance();
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
                        if (!mImpl || !gHMD.isPostDetectionInitialized() || !gHMD.isHMDConnected() || !gHMD.isHMDSensorConnected())
                        {
                            // can't render to the HMD window, so abort
                            mRenderMode = RenderMode_ScreenStereo;
                            return;
                        }
#if LL_DARWIN
                        // resize main window to be the size of the HMD
                        // to handle cursor positioning in HMD mode
                        if (!isMainFullScreen())
                        {
                            windowp->setSize(getHMDClientSize());
                        }
#endif
                        if (!setRenderWindowHMD())
                        {
                            // Somehow, we've lost the HMD window, so just recreate it
                            setRenderWindowMain();
                            gHMD.isPostDetectionInitialized(FALSE);
                            if (!mImpl->postDetectionInit() || !setRenderWindowHMD())
                            {
                                // still can't create the window - abort
                                setRenderMode(RenderMode_ScreenStereo, setFocusWindow);
                                return;
                            }
                        }
                        windowp->enableVSync(TRUE);
                        windowp->setHMDMode(TRUE, gHMD.isUsingAppWindow(), isMainFullScreen(), (U32)mImpl->getHMDWidth(), (U32)mImpl->getHMDHeight());
                        onViewChange(oldMode);
                    }
                    break;
                case RenderMode_ScreenStereo:
                    // switching from HMD to ScreenStereo
                    // not much to do here except resize the main window
                    {
                        setRenderWindowMain();
                        windowp->setHMDMode(TRUE, isHMDMirror(), isMainFullScreen(), (U32)mImpl->getHMDWidth(), (U32)mImpl->getHMDHeight());
                        if (isMainFullScreen())
                        {
                            onViewChange(oldMode);
                        }
                        else
                        {
                            windowp->setSize(getHMDClientSize());
                            windowp->setPosition(mMainWindowPos);
                        }
                        windowp->enableVSync(!gSavedSettings.getBOOL("DisableVerticalSync"));
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
                        windowp->setHMDMode(FALSE, isHMDMirror(), isMainFullScreen(), gSavedSettings.getU32("MinWindowWidth"), gSavedSettings.getU32("MinWindowHeight"));
#if LL_DARWIN
                        if (isHMDMirror())
                        {
                            LLWindowMacOSX* w = dynamic_cast<LLWindowMacOSX*>(windowp);
                            if (w)
                            {
                                w->exitFullScreen(mMainWindowPos, mMainClientSize);
                            }
                        }
                        else
#endif
                        if (!isMainFullScreen())
                        {
                            onViewChange(oldMode);
#if LL_DARWIN
                            windowp->setSize(mMainClientSize);
#else
                            windowp->setSize(mMainWindowSize);
#endif
                            windowp->setPosition(mMainWindowPos);
                        }
                        if (oldMode == RenderMode_HMD)
                        {
                            windowp->enableVSync(!gSavedSettings.getBOOL("DisableVerticalSync"));
                        }
                        LLFloaterCamera::onHMDChange();
                        LLFloaterReg::setBlockInstance(false, "snapshot");
                        pCamera->setAspect(mMainWindowAspect);
                        gSavedSettings.setF32("CameraAngle", mMainWindowFOV);
                        pCamera->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
                        gViewerWindow->reshape(mMainClientSize.mX, mMainClientSize.mY);
                    }
                    break;
                }
            }
            break;
        case RenderMode_None:
        default:
            {
                // clear the main window and save off size settings
                windowp->getFramePos(&mMainWindowPos);
                windowp->getSize(&mMainWindowSize);
                windowp->getSize(&mMainClientSize);
                mMainWindowFOV = gSavedSettings.getF32("CameraAngle");
                mMainWindowAspect = pCamera->getAspect();
                isMainFullScreen(windowp->getFullscreen());
                renderUnusedMainWindow();

                // snapshots are disabled in HMD mode due to problems with always rendering UI and sometimes
                // rendering black screen before saving.  This is probably a solvable issue, but not in the 
                // time constraints given right now, so disabling them until someone has a chance to fix
                // the issues.
                LLFloaterReg::setBlockInstance(true, "snapshot");
                switch (mRenderMode)
                {
                case RenderMode_HMD:
                    // switching from Normal to HMD
                    {
                        // first ensure that we CAN render to the HMD (i.e. it's initialized, we have a valid window,
                        // the HMD is still connected, etc.
                        if (!mImpl || !gHMD.isPostDetectionInitialized() || !gHMD.isHMDConnected() || !gHMD.isHMDSensorConnected())
                        {
                            // can't render to the HMD window, so abort
                            mRenderMode = RenderMode_None;
                            return;
                        }
#if LL_DARWIN
                        // resize main window to be the size of the HMD
                        // to handle cursor positioning in HMD mode
                        if (!isMainFullScreen())
                        {
                            windowp->setSize(getHMDClientSize(), TRUE);
                        }
#endif
                        if (!setRenderWindowHMD())
                        {
                            // Somehow, we've lost the HMD window, so just recreate it
                            setRenderWindowMain(); 
                            gHMD.isPostDetectionInitialized(FALSE);
                            if (!mImpl->postDetectionInit() || !setRenderWindowHMD())
                            {
                                // still can't create the window - abort
                                setRenderMode(RenderMode_ScreenStereo, setFocusWindow);
                                return;
                            }
                        }
                        windowp->enableVSync(TRUE);
                        windowp->setHMDMode(TRUE, gHMD.isUsingAppWindow(), isMainFullScreen(), (U32)mImpl->getHMDWidth(), (U32)mImpl->getHMDHeight());
                        pCamera->setAspect(gHMD.getAspect());
                        pCamera->setDefaultFOV(gHMD.getVerticalFOV());
                        gSavedSettings.setF32("CameraAngle", gHMD.getVerticalFOV());
                        onViewChange(oldMode);
                    }
                    break;
                case RenderMode_ScreenStereo:
                    // switching from Normal to ScreenStereo
                    {
                        windowp->setHMDMode(TRUE, isHMDMirror(), isMainFullScreen(), (U32)mImpl->getHMDWidth(), (U32)mImpl->getHMDHeight());
                        pCamera->setAspect(gHMD.getAspect());
                        pCamera->setDefaultFOV(gHMD.getVerticalFOV());
                        gSavedSettings.setF32("CameraAngle", gHMD.getVerticalFOV());
                        if (isMainFullScreen() || isHMDMirror())
                        {
                            onViewChange(oldMode);
                        }
                        else
                        {
                            windowp->setSize(getHMDClientSize(), TRUE);
                            windowp->setPosition(mMainWindowPos);
                        }
                    }
                    break;
                }
                LLFloaterCamera::onHMDChange();
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
        if (mImpl && isPostDetectionInitialized() && isHMDConnected() && isHMDSensorConnected() && isHMDMode())
        {
            mImpl->resetOrientation();
        }
    }
#else
    mRenderMode = RenderMode_None;
#endif
}


BOOL LLHMD::setRenderWindowMain()
{
    return gHMD.isUsingAppWindow() || gViewerWindow->getWindow()->setRenderWindow(0, gHMD.isMainFullScreen());
}


BOOL LLHMD::setRenderWindowHMD()
{
    BOOL res = FALSE;
#if LL_HMD_SUPPORTED
    res = gHMD.isUsingAppWindow() || gViewerWindow->getWindow()->setRenderWindow(1, TRUE);
#endif // LL_HMD_SUPPORTED
    return res;
}


void LLHMD::setFocusWindowMain()
{
#if LL_HMD_SUPPORTED
    isChangingRenderContext(TRUE);
    BOOL res = gViewerWindow->getWindow()->setFocusWindow(0);
    if (res)
    {
        if (isHMDMode() && isHMDMirror())
        {
            // in this case, appFocusGained is not called because we're not changing windows,
            // so just call manually
            onAppFocusGained();
            if (useMirrorHack() && !gViewerWindow->isMouseInWindow())
            {
                gViewerWindow->moveCursorToCenter();
            }
        }
        else if (!isHMDMode())
        {
#if !LL_DARWIN
            // setFocusWindow on Mac does not call FocusGained or FocusLost.  In order to make things behave,
            // we always need to call them directly here, whether the display is mirrored or not.  On non-Mac platforms
            // we only need to call onAppFocusGained directly if we're in Mirroring mode.
            if (isHMDMirror())
#endif // LL_DARWIN
            {
                onAppFocusGained();
                //// this is handled by the appfocuslost message in windows, but since that doesn't get called on Mac, we have to
                //// handle the critical parts here instead.
                //gViewerWindow->showCursor();
                if (useMirrorHack() && !gViewerWindow->isMouseInWindow())
                {
                    gViewerWindow->moveCursorToCenter();
                }
            }
            // in the case of switching from debug HMD mode to normal mode, no appfocus message is sent since 
            // we're already focused on the main window, so we have to manually disable mouse clipping.  In the case
            // where we are switching from HMD to normal mode, then this is just a redundant call, but doesn't hurt
            // anything.
            gViewerWindow->getWindow()->setMouseClipping(FALSE);
        }
    }
    else
    {
        isChangingRenderContext(FALSE);
    }
#endif // LL_HMD_SUPPORTED
}


void LLHMD::setFocusWindowHMD()
{
#if LL_HMD_SUPPORTED
    if (!gViewerWindow->isMouseInWindow())
    {
        gViewerWindow->moveCursorToCenter();
    }
    isChangingRenderContext(TRUE);
    if (!gViewerWindow->getWindow()->setFocusWindow(1))
    {
        isChangingRenderContext(FALSE);
    }
#if LL_DARWIN
    else
    {
        // setFocusWindow on Mac does not call FocusGained or FocusLost, except for calling FocusLost the first time.
        // WHY that is, I have no idea.   But in order to make things behave, we need to call them directly here.
        onAppFocusGained();
    }
#endif // LL_DARWIN
#endif // LL_HMD_SUPPORTED
}


void LLHMD::onAppFocusGained()
{
#if LL_HMD_SUPPORTED
    if (isChangingRenderContext())
    {
        if (isHMDMode())
        {
            gViewerWindow->getWindow()->setMouseClipping(TRUE);
        }
        else
        {
            gViewerWindow->getWindow()->setMouseClipping(FALSE);
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
        }
        else if (mRenderMode == (U32)RenderMode_ScreenStereo)
        {
            gViewerWindow->getWindow()->setMouseClipping(TRUE);
        }
        else
        {
            gViewerWindow->getWindow()->setMouseClipping(FALSE);
        }
    }
#endif // LL_HMD_SUPPORTED
}


void LLHMD::onAppFocusLost()
{
#if LL_HMD_SUPPORTED
    if (!isChangingRenderContext() && mRenderMode == (U32)RenderMode_HMD)
    {
        // Make sure we change the render window to main so that we avoid BSOD in the graphics drivers when
        // it tries to render to a (now) invalid renderContext.
        setRenderWindowMain();
        setRenderMode(RenderMode_None, false);
    }
#endif // LL_HMD_SUPPORTED
}


void LLHMD::renderUnusedMainWindow()
{
//#if LL_HMD_SUPPORTED
//    if (gHMD.getRenderMode() == LLHMD::RenderMode_HMD
//        && gHMD.isPostDetectionInitialized()
//        && gHMD.isHMDConnected()
//        && gHMD.isHMDSensorConnected()
//        && !gHMD.isUsingAppWindow()
//        && gViewerWindow
//        && gViewerWindow->getWindow()
//       )
//    {
//        if (gHMD.setRenderWindowMain())
//        {
//            gViewerWindow->getWindowViewportRaw(gGLViewport, gHMD.getMainWindowWidth(), gHMD.getMainWindowHeight());
//            glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
//            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//            glClear(GL_COLOR_BUFFER_BIT);
//            LLViewerDisplay::swap(TRUE, LLViewerDisplay::gDisplaySwapBuffers);
//        }
//    }
//#endif
}


void LLHMD::renderUnusedHMDWindow()
{
#if LL_HMD_SUPPORTED
    if (gHMD.isPostDetectionInitialized()
        && gHMD.isHMDConnected()
        && gHMD.isHMDSensorConnected()
        && gHMD.getRenderMode() != LLHMD::RenderMode_HMD
        && !gHMD.isUsingAppWindow()
        && gViewerWindow
        && gViewerWindow->getWindow())
    {
        if (gHMD.setRenderWindowHMD())
        {
            gHMD.getViewportInfo(gGLViewport);
            //gViewerWindow->getWindowViewportRaw(gGLViewport, gHMD.getHMDWidth(), gHMD.getHMDHeight());
            glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            // write text "press CTRL-SHIFT-D to switch to HMD"
            gViewerWindow->getWindow()->swapBuffers();
            //LLViewerDisplay::swap(TRUE, LLViewerDisplay::gDisplaySwapBuffers);
        }
    }
#endif
}


U32 LLHMD::suspendHMDMode()
{
    U32 oldMode = getRenderMode();
    if (isHMDMode())
    {
        setRenderMode(RenderMode_None);
    }
    return oldMode;
}


void LLHMD::resumeHMDMode(U32 prevRenderMode)
{
    if (prevRenderMode != RenderMode_None)
    {
        setRenderMode(prevRenderMode);
    }
}


U32 LLHMD::getCurrentEye() const { return mImpl ? mImpl->getCurrentEye() : 0; }


void LLHMD::setCurrentEye(U32 eye)
{
    if (mImpl)
    {
        mImpl->setCurrentEye(eye);
    }
    if (eye == CENTER_EYE)
    {
        mCameraOffset = LLVector3::zero;
    }
}


void LLHMD::getViewportInfo(S32& x, S32& y, S32& w, S32& h) { if (mImpl) { mImpl->getViewportInfo(x, y, w, h); } }
void LLHMD::getViewportInfo(S32 vp[4]) { if (mImpl) { mImpl->getViewportInfo(vp); } else { vp[0] = vp[1] = vp[2] = vp[3] = 0; } }
S32 LLHMD::getHMDWidth() const { return mImpl ? mImpl->getHMDWidth() : 0; }
S32 LLHMD::getHMDEyeWidth() const { return mImpl ? mImpl->getHMDEyeWidth() : 0; }
S32 LLHMD::getHMDHeight() const { return mImpl ? mImpl->getHMDHeight() : 0; }
S32 LLHMD::getHMDUIWidth() const { return mImpl ? mImpl->getHMDUIWidth() : 0; }
S32 LLHMD::getHMDUIHeight() const { return mImpl ? mImpl->getHMDUIHeight() : 0; }
S32 LLHMD::getHMDViewportWidth() const { return mImpl ? mImpl->getViewportWidth() : 0; }
S32 LLHMD::getHMDViewportHeight() const { return mImpl ? mImpl->getViewportHeight() : 0; }
F32 LLHMD::getInterpupillaryOffset() const { return mImpl ? mImpl->getInterpupillaryOffset() : 0.0f; }
F32 LLHMD::getInterpupillaryOffsetDefault() const { return mImpl ? mImpl->getInterpupillaryOffsetDefault() : 0.0f; }
F32 LLHMD::getEyeToScreenDistance() const { return mImpl ? mImpl->getEyeToScreenDistance() : 0.0f; }
F32 LLHMD::getEyeToScreenDistanceDefault() const { return mImpl ? mImpl->getEyeToScreenDistanceDefault() : 0.0f; }
void LLHMD::setEyeToScreenDistance(F32 f) { calculateUIEyeDepth(); gPipeline.mHMDUISurface = NULL; }


void LLHMD::setUIMagnification(F32 f)
{
    if (f != mUIShape.mUIMagnification)
    {
        mUIShape.mUIMagnification = f;
        if (gHMD.mUIShape.mPresetType != (U32)LLHMD::kUser)
        {
            mUIShapePreset = 0;
            gHMD.mUIShape.mPresetType = (U32)LLHMD::kCustom;
            gHMD.mUIShape.mPresetTypeIndex = 1;
        }
        calculateUIEyeDepth();
    }
}


void LLHMD::setUISurfaceParam(F32* p, F32 f)
{
    if (!p)
    {
        return;
    }
    if (!is_approx_equal(f, *p))
    {
        *p = f;
        if (gHMD.mUIShape.mPresetType != (U32)LLHMD::kUser)
        {
            mUIShapePreset = 0;
            mUIShape.mPresetType = (U32)LLHMD::kCustom;
            mUIShape.mPresetTypeIndex = 1;
        }
        onChangeUISurfaceShape();
    }
}


void LLHMD::resetOrientation() { if (mImpl) { mImpl->resetOrientation(); } }
void LLHMD::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { if (mImpl) { mImpl->getHMDRollPitchYaw(roll, pitch, yaw); } else { roll = pitch = yaw = 0.0f; } }
void LLHMD::getHMDLastRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = mLastRollPitchYaw[VX]; pitch = mLastRollPitchYaw[VY], yaw = mLastRollPitchYaw[VZ]; }


void LLHMD::getHMDDeltaRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const
{
    if (mImpl)
    {
        roll = mImpl->getRoll() - mLastRollPitchYaw[VX];
        pitch = mImpl->getPitch() - mLastRollPitchYaw[VY];
        yaw = mImpl->getYaw() - mLastRollPitchYaw[VZ];
    }
    else
    {
        roll = pitch = yaw = 0.0f;
    }
}


F32 LLHMD::getHMDRoll() const { return mImpl ? mImpl->getRoll() : 0.0f; }
F32 LLHMD::getHMDLastRoll() const { return mLastRollPitchYaw[VX]; }
F32 LLHMD::getHMDDeltaRoll() const { if (mImpl) { return mImpl->getRoll() - mLastRollPitchYaw[VX]; } else { return 0.0f; } }
F32 LLHMD::getHMDPitch() const { return mImpl ? mImpl->getPitch() : 0.0f; }
F32 LLHMD::getHMDLastPitch() const { return mLastRollPitchYaw[VY]; }
F32 LLHMD::getHMDDeltaPitch() const { if (mImpl) { return mImpl->getPitch() - mLastRollPitchYaw[VY]; } else { return 0.0f; } }
F32 LLHMD::getHMDYaw() const { return mImpl ? mImpl->getYaw() : 0.0f; }
F32 LLHMD::getHMDLastYaw() const { return mLastRollPitchYaw[VZ]; }
F32 LLHMD::getHMDDeltaYaw() const { if (mImpl) { return mImpl->getYaw() - mLastRollPitchYaw[VZ]; } else { return 0.0f; } }
LLVector3 LLHMD::getHeadPosition() const { if (mImpl) { return mImpl->getHeadPosition(); } else { return LLVector3::zero; } }

F32 LLHMD::getVerticalFOV() const { return mImpl ? mImpl->getVerticalFOV() : 0.0f; }
F32 LLHMD::getAspect() { return mImpl ? mImpl->getAspect() : 0.0f; }
BOOL LLHMD::useMotionPrediction() const { return mImpl ? mImpl->useMotionPrediction() : FALSE; }
void LLHMD::useMotionPrediction(BOOL b) { if (mImpl) { mImpl->useMotionPrediction(b); } }
BOOL LLHMD::useMotionPredictionDefault() const { return mImpl ? mImpl->useMotionPredictionDefault() : FALSE; }
F32 LLHMD::getMotionPredictionDelta() const { return mImpl ? mImpl->getMotionPredictionDelta() : 0.0f; }
F32 LLHMD::getMotionPredictionDeltaDefault() const { return mImpl ? mImpl->getMotionPredictionDeltaDefault() : 0.0f; }
void LLHMD::setMotionPredictionDelta(F32 f) { if (mImpl) { mImpl->setMotionPredictionDelta(f); } }
F32 LLHMD::getOrthoPixelOffset() const { return mImpl ? mImpl->getOrthoPixelOffset() : 0.0f; }


std::string LLHMD::getUIShapeName() const
{
    if (mUIShape.mPresetType >= (U32)LLHMD::kDefault)
    {
        LLStringUtil::format_map_t args;
        args["[INDEX]"] = llformat ("%u", mUIShape.mPresetTypeIndex);
        return LLTrans::getString(mUIShape.mPresetType == (U32)LLHMD::kDefault ? "HMDPresetDefault" : "HMDPresetUser", args);
    }
    return LLTrans::getString("HMDPresetCustom");
}


void LLHMD::calculateUIEyeDepth()
{
    if (mImpl)
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
        F32 eyeDepthMult = mUIShape.mUIMagnification / ((getEyeToScreenDistanceDefault() - 0.083f) * 1000.0f);
        mUIEyeDepth = ((mImpl->getEyeToScreenDistance() - 0.083f) * eyeDepthMult);
    }
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


void LLHMD::setUIShapePresetIndex(S32 idx)
{
    if (idx < 0 || idx >= (S32)mUIPresetValues.size())
    {
        idx = 0;
    }
    mUIShapePreset = idx;
    if (idx == 0)
    {
        mUIShape.mPresetType = (U32)LLHMD::kCustom;
        mUIShape.mPresetTypeIndex = 1;
    }
    else // if (idx > 0)
    {
        mUIShape = mUIPresetValues[idx];
        calculateUIEyeDepth();
        onChangeUISurfaceShape();
    }
}


LLHMD::UISurfaceShapeSettings LLHMD::getUIShapePreset(S32 idx)
{
    // Note: intentionally NOT returning a const ref here.  We do a copy because the contents of the vector can
    // change and if it does, any refs held by others will become invalid and could cause crashes/memory corruption
    // if accessed.
    if (idx < 0 || idx >= (S32)mUIPresetValues.size())
    {
        idx = 0;
    }
    return mUIPresetValues[idx];
}


BOOL LLHMD::addPreset()
{
    if (mUIShape.mPresetType != (U32)LLHMD::kCustom)
    {
        return FALSE;
    }
    // verify that the current settings are different than all other saved presets
    U32 numPresets = mUIPresetValues.size();
    for (U32 i = 1; i < numPresets; ++i)
    {
        const UISurfaceShapeSettings& settings = mUIPresetValues[i];
        BOOL isSame = TRUE;
        isSame = isSame && (is_approx_equal(mUIShape.mOffsetX, settings.mOffsetX));
        isSame = isSame && (is_approx_equal(mUIShape.mOffsetY, settings.mOffsetY));
        isSame = isSame && (is_approx_equal(mUIShape.mOffsetZ, settings.mOffsetZ));
        isSame = isSame && (is_approx_equal(mUIShape.mToroidRadiusWidth, settings.mToroidRadiusWidth));
        isSame = isSame && (is_approx_equal(mUIShape.mToroidRadiusDepth, settings.mToroidRadiusDepth));
        isSame = isSame && (is_approx_equal(mUIShape.mToroidCrossSectionRadiusWidth, settings.mToroidCrossSectionRadiusWidth));
        isSame = isSame && (is_approx_equal(mUIShape.mToroidCrossSectionRadiusHeight, settings.mToroidCrossSectionRadiusHeight));
        isSame = isSame && (is_approx_equal(mUIShape.mArcHorizontal, settings.mArcHorizontal));
        isSame = isSame && (is_approx_equal(mUIShape.mArcVertical, settings.mArcVertical));
        isSame = isSame && (is_approx_equal(mUIShape.mUIMagnification, settings.mUIMagnification));
        if (isSame)
        {
            setUIShapePresetIndex(i);
            return TRUE;
        }
    }

    // the current values are different than all existing presets, so add a new preset
    mUIShape.mPresetType = (U32)LLHMD::kUser;
    mUIShape.mPresetTypeIndex = mNextUserPresetIndex++;
    mUIPresetValues.push_back(mUIShape);
    setUIShapePresetIndex(numPresets);
    return TRUE;
}


BOOL LLHMD::updatePreset()
{
    if (mUIShapePreset < 1 || mUIShapePreset >= (S32)mUIPresetValues.size())
    {
        return FALSE;
    }
    if (mUIPresetValues[mUIShapePreset].mPresetType != (U32)LLHMD::kUser)
    {
        return FALSE;
    }
    memcpy(&(mUIPresetValues[mUIShapePreset]), &mUIShape, sizeof(UISurfaceShapeSettings));
    return TRUE;
}


BOOL LLHMD::removePreset(S32 idx)
{
    if (idx < 1 || idx >= (S32)mUIPresetValues.size())
    {
        return FALSE;
    }
    if (mUIPresetValues[idx].mPresetType != (U32)LLHMD::kUser)
    {
        return FALSE;
    }
    mUIPresetValues.erase(mUIPresetValues.begin() + idx);
    if (mUIShapePreset == idx)
    {
        mUIShapePreset = 0;
        mUIShape.mPresetType = (U32)LLHMD::kCustom;
        mUIShape.mPresetTypeIndex = 1;
    }
    return TRUE;
}


void LLHMD::saveSettings()
{
    isSavingSettings(TRUE);
    if (mUIShapePreset == 0)
    {
        // These SHOULD already be set to these values, but just in case..
        mUIShape.mPresetType = (U32)LLHMD::kCustom;
        mUIShape.mPresetTypeIndex = 1;

        addPreset();
    }
    else if (mUIShape.mPresetType == (U32)LLHMD::kUser)
    {
        updatePreset();
    }

    gSavedSettings.setF32("HMDUISurfaceArcHorizontal", gHMD.getUISurfaceArcHorizontal());
    gSavedSettings.setF32("HMDUISurfaceArcVertical", gHMD.getUISurfaceArcVertical());
    gSavedSettings.setF32("HMDUISurfaceToroidWidth", gHMD.getUISurfaceToroidRadiusWidth());
    gSavedSettings.setF32("HMDUISurfaceToroidDepth", gHMD.getUISurfaceToroidRadiusDepth());
    gSavedSettings.setF32("HMDUISurfaceToroidCSWidth", gHMD.getUISurfaceToroidCrossSectionRadiusWidth());
    gSavedSettings.setF32("HMDUISurfaceToroidCSHeight", gHMD.getUISurfaceToroidCrossSectionRadiusHeight());
    LLVector3 offsets(mUIShape.mOffsetX, mUIShape.mOffsetY, mUIShape.mOffsetZ);
    gSavedSettings.setVector3("HMDUISurfaceOffsets", offsets);
    gSavedSettings.setF32("HMDUIMagnification", gHMD.getUIMagnification());
    gSavedSettings.setS32("HMDUIShapePreset", gHMD.getUIShapePresetIndex());

    gSavedSettings.setBOOL("HMDLowPersistence", gHMD.useLowPersistence());
    gSavedSettings.setBOOL("HMDPixelLuminanceOverdrive", gHMD.usePixelLuminanceOverdrive());
    gSavedSettings.setBOOL("HMDUseMotionPrediction", gHMD.useMotionPrediction());
    gSavedSettings.setBOOL("HMDTimewarp", gHMD.isTimewarpEnabled());
    gSavedSettings.setF32("HMDTimewarpIntervalSeconds", gHMD.getTimewarpIntervalSeconds());
    gSavedSettings.setBOOL("HMDDynamicResolutionScaling", gHMD.useDynamicResolutionScaling());

    //gSavedSettings.setBOOL("HMDAdvancedMode", gHMD.isAdvancedMode());
    //gSavedSettings.setBOOL("HMDEnablePositionalTracking", gHMD.isPositionTrackingEnabled());
    //gSavedSettings.setBOOL("HMDAllowTextRoll", gHMD.allowTextRoll());
    //gSavedSettings.setF32("HMDWorldCursorSizeMult", gHMD.getWorldCursorSizeMult());
    //gSavedSettings.setS32("HMDMouselookControlMode", gHMD.getMouselookControlMode());
    //gSavedSettings.setF32("HMDMouselookRotThreshold", mMouselookRotThreshold);
    //gSavedSettings.setF32("HMDMouselookRotMax", mMouselookRotMax);
    //gSavedSettings.setF32("HMDMouselookTurnSpeedMax", mMouselookTurnSpeedMax);
    //gSavedSettings.setBOOL("HMDTimewarpNoJit", gHMD.useMotionPrediction());
    //gSavedSettings.setF32("HMDPixelDensity", gHMD.useMotionPrediction());

    isSavingSettings(FALSE);

    static const char* kPresetTypeStrings[] = { "Custom", "Default", "User" };
    LLSDArray entries;
    U32 numPresets = mUIPresetValues.size();
    for (U32 i = 1; i < numPresets; ++i)
    {
        const UISurfaceShapeSettings& settings = mUIPresetValues[i];
        LLSDMap entry;
        entry("PresetType", kPresetTypeStrings[settings.mPresetType]);
        entry("ToroidRadiusWidth", settings.mToroidRadiusWidth);
        entry("ToroidRadiusDepth", settings.mToroidRadiusDepth);
        entry("ToroidCSRadiusWidth", settings.mToroidCrossSectionRadiusWidth);
        entry("ToroidCSRadiusHeight", settings.mToroidCrossSectionRadiusHeight);
        entry("ArcHorizontal", settings.mArcHorizontal);
        entry("ArcVertical", settings.mArcVertical);
        entry("UIMagnification", settings.mUIMagnification);
        entry("Offsets", LLSDArray(settings.mOffsetX)(settings.mOffsetY)(settings.mOffsetZ));
        entries(entry);
    }
    gSavedSettings.setLLSD("HMDUIPresetValues", entries);
}


void LLHMD::onViewChange(S32 oldMode)
{
    if (mImpl)
    {
        mImpl->onViewChange(oldMode);
    }
    if (gHMD.isHMDMode())
    {
        gViewerWindow->reshape(mImpl->getViewportWidth(), mImpl->getViewportHeight());
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
    F32 dx = mUIShape.mArcHorizontal / ((F32)resX - 1.0f);
    F32 dy = mUIShape.mArcVertical / ((F32)resY - 1.0f);
    //LL_INFOS("HMD")  << "XA: [" << xa[0] << "," << xa[1] << "], "
    //                    << "YA: [" << ya[0] << "," << ya[1] << "], "
    //                    << "r: [" << r[0] << "," << r[1] << "], "
    //                    << "offsets: [" << offsets[0] << "," << offsets[1] << "," << offsets[2] << "]"
    //                    << LL_ENDL;
    //LL_INFOS("HMD")  << "dX: " << dx << ", "
    //                    << "dY: " << dy
    //                    << LL_ENDL;

    LLVector4a t;
    LLVector4 t2;
    LLVector2 uv;
    for (U32 i = 0; i < resY; ++i)
    {
        F32 va = (F_PI - (mUIShape.mArcVertical * 0.5f)) + ((F32)i * dy);
        for (U32 j = 0; j < resX; ++j)
        {
            F32 ha = (mUIShape.mArcHorizontal * -0.5f) + ((F32)j * dx);
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
            //LL_INFOS("HMD")  << "Vtx " << ((i * resX) + j) << " [" << i << "," << j << "]: "
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
    pos.set(    (sin(ha) * (mUIShape.mToroidRadiusWidth - (cva * mUIShape.mToroidCrossSectionRadiusWidth))) + mUIShape.mOffsetX,
                (mUIShape.mToroidCrossSectionRadiusHeight * -sin(va)) + mUIShape.mOffsetY,
                (-1.0f * ((mUIShape.mToroidRadiusDepth * cos(ha)) - (cva * mUIShape.mToroidCrossSectionRadiusWidth))) + mUIShape.mOffsetZ,
                1.0f);
    if (uv)
    {
        uv->set((ha - (mUIShape.mArcHorizontal * -0.5f)) / mUIShape.mArcHorizontal, (va - (F_PI - (mUIShape.mArcVertical * 0.5f))) / mUIShape.mArcVertical);
    }
}


void LLHMD::calculateMouseWorld(S32 mouse_x, S32 mouse_y, LLVector3& world)
{
    if (!isHMDMode())
    {
        world.set(0.0f, 0.0f, 0.0f);
        return;
    }

    if (gAgentCamera.cameraMouselook())
    {
        GLdouble x, y, z;
        F64 mdlv[16], proj[16];
        S32 vp[4];
        gViewerWindow->getWorldViewportRaw(vp);
        for (U32 i = 0; i < 16; i++)
        {
            mdlv[i] = (F64)mBaseModelView[i];
            proj[i] = (F64)mBaseProjection[i];
        }
        gluUnProject(   GLdouble(mouse_x), GLdouble(mouse_y), 0.0,
                        mdlv, proj, (GLint*)gGLViewport,
                        &x, &y, &z);
        world.set((F32)x, (F32)y, (F32)z);
    }
    else
    {
        // 1. determine horizontal and vertical percentage within toroidal UI surface based on mouse_x, mouse_y
        F32 uiw = (F32)gHMD.getHMDUIWidth();
        F32 uih = (F32)gHMD.getHMDUIHeight();
        F32 nx = llclamp((F32)mouse_x / (F32)uiw, 0.0f, 1.0f);
        F32 ny = llclamp((F32)mouse_y / (F32)uih, 0.0f, 1.0f);

        // 2. determine horizontal and vertical angle on toroid based on nx, ny
        F32 ha = ((mUIShape.mArcHorizontal * -0.5f) + (nx * mUIShape.mArcHorizontal));
        F32 va = (F_PI - (mUIShape.mArcVertical * 0.5f)) + (ny * mUIShape.mArcVertical);

        // 3. determine eye-space x,y,z for ha, va (-z forward/depth)
        LLVector4 eyeSpacePos;
        getUISurfaceCoordinates(ha, va, eyeSpacePos);

        // 4. convert eye-space to world coordinates (taking into account the ui-magnification that essentially 
        //    translates the view forward (or backward, depending on the mag level) along the axis going into the 
        //    center of the UI surface).
        glh::matrix4f uivInv(mUIModelViewInv);
        glh::vec4f w(eyeSpacePos.mV);
        uivInv.mult_matrix_vec(w);
        world.set(w[VX], w[VY], w[VZ]);
    }
}


void LLHMD::updateHMDMouseInfo()
{
#if LL_HMD_SUPPORTED
    calculateMouseWorld(gViewerWindow->getCurrentMouse().mX, gViewerWindow->getCurrentMouse().mY, mMouseWorld);
#endif // LL_HMD_SUPPORTED
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


void LLHMD::setupStereoValues()
{
    // Remember default mono camera details.
    LLViewerCamera* cam = LLViewerCamera::getInstance();
    mStereoCameraFOV = cam->getView();
    mStereoCameraPosition = cam->getOrigin();

    // Stereo culling frustum camera parameters.
    mStereoCullCameraDeltaForwards = mImpl->getStereoCullCameraForwards();
    mStereoCullCameraFOV = mStereoCameraFOV;
    mStereoCullCameraAspect = mImpl->getAspect();
}


void LLHMD::setupStereoCullFrustum()
{
    mCameraOffset = LLVector3::zero;
    mProjectionOffset[0][2] = 0.0f;
    LLViewerCamera* cam = LLViewerCamera::getInstance();
    cam->setView(mStereoCullCameraFOV, TRUE);
    cam->setAspect(mStereoCullCameraAspect);
    LLVector3 new_position = mStereoCameraPosition + mStereoCullCameraDeltaForwards;
    cam->setOrigin(new_position);
}


void LLHMD::setupEye()
{
    mCameraOffset = mImpl->getCurrentEyeCameraOffset();
    mImpl->getCurrentEyeProjectionOffset(mProjectionOffset);

    LLViewerCamera* cam = LLViewerCamera::getInstance();
    cam->setView(mStereoCameraFOV, getCurrentEye() == (U32)LLHMD::CENTER_EYE ? TRUE : FALSE);
    cam->setAspect(mImpl->getAspect());
    cam->setOrigin(mImpl->getCurrentEyeOffset(mStereoCameraPosition));
}


LLRenderTarget* LLHMD::getCurrentEyeRT()
{
    return mImpl ? mImpl->getCurrentEyeRT() : NULL;
}


void LLHMD::bindCurrentEyeRT()
{
    LLRenderTarget* target = getCurrentEyeRT();
    if (target)
    {
        target->bindTarget();
    }
}


void LLHMD::flushCurrentEyeRT()
{
    LLRenderTarget* target = getCurrentEyeRT();
    if (target)
    {
        target->flush();
    }
}


void LLHMD::releaseAllEyeRT()
{
    if (mImpl)
    {
        for (U32 i = (U32)LLHMD::CENTER_EYE; i <= (U32)LLHMD::RIGHT_EYE; ++i)
        {
            LLRenderTarget* rt = mImpl->getEyeRT(i);
            if (rt)
            {
                rt->release();
            }
        }
    }
}


void LLHMD::setup3DViewport(S32 x_offset, S32 y_offset, BOOL forEye)
{
    S32 x = 0, y = 0, w = 0, h = 0;
    if (forEye)
    {
        mImpl->getViewportInfo(x, y, w, h);
    }
    x += x_offset;
    y += y_offset;
    gViewerWindow->getWorldViewportRaw(gGLViewport, w, h, x, y);
	glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
}


void LLHMD::setup2DRender()
{
    gl_state_for_2d(gHMD.getHMDViewportWidth(), gHMD.getHMDViewportHeight(), 0, gHMD.getOrthoPixelOffset());
    gViewerWindow->getWindowViewportRaw(gGLViewport);
    glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
}


void LLHMD::renderCursor2D()
{
    if (isHMDMode() && (!gAgentCamera.cameraMouselook() || gAgent.leftButtonGrabbed()))
    {
        LLWindow* window = gViewerWindow->getWindow();
        if (!window || window->isCursorHidden())
        {
            return;
        }

        if (cursorIntersectsUI())
        {
            gGL.pushMatrix();
            gGL.pushUIMatrix();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.bind();
            }
            S32 mx = gViewerWindow->getCurrentMouseX();
            S32 my = gViewerWindow->getCurrentMouseY();
            U32 cursorType = (U32)gViewerWindow->getWindow()->getCursor();
            LLViewerTexture* pCursorTexture = getCursorImage(cursorType);
            if (pCursorTexture)
            {
                const LLVector2& curoff = getCursorHotspotOffset(cursorType);
                S32 offx = llround(-32.0f * curoff[VX]);
                S32 offy = -32 + llround(32.0f * curoff[VY]);
                gl_draw_scaled_image(mx + offx, my + offy, 32, 32, pCursorTexture);
            }
            else
            {
                gl_line_2d(mx - 10, my, mx + 10, my, LLColor4(1.0f, 0.0f, 0.0f));
                gl_line_2d(mx, my - 10, mx, my + 10, LLColor4(0.0f, 1.0f, 0.0f));
            }
            gGL.popUIMatrix();
            gGL.popMatrix();
            gGL.flush();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.unbind();
            }
        }
    }
}


void LLHMD::renderCursor3D()
{
    if (isHMDMode() && !cursorIntersectsUI() && (!gAgentCamera.cameraMouselook() || gAgent.leftButtonGrabbed()))
    {
        LLWindow* window = gViewerWindow->getWindow();
        if (!window || window->isCursorHidden())
        {
            return;
        }

        LLViewerCamera* camera = LLViewerCamera::getInstance();
        LLVector3 origin = camera->getOrigin();
        LLVector3 pt;
        if (cursorIntersectsWorld())
        {
            pt.set(mMouseWorldRaycastIntersection.getF32ptr());
        }
        else
        {
            pt.set(mMouseWorldEnd.getF32ptr());
        }
        LLVector3 delta = pt - origin;
        F32 dist = delta.magVec();
        F32 tanA = tanf(DEG_TO_RAD * mMouseWorldSizeMult);
        F32 scalingFactor = dist * tanA;

        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();
        gGL.loadMatrix(gGLModelView);

        LLVertexBuffer::unbind();
        LLGLSUIDefault s1;
        LLGLDisable fog(GL_FOG);
        gPipeline.disableLights();
        U32 cursorType = (U32)gViewerWindow->getWindow()->getCursor();

        LLViewerTexture* pCursorTexture = getCursorImage(cursorType);
        if (pCursorTexture)
        {
            if (LLGLSLShader::sNoFixedFunction)
            {
                gOneTextureNoColorProgram.bind();
            }
            gGL.setColorMask(true, false);
            gGL.color4f(1.0f,1.0f,1.0f,1.0f);
            gGL.getTexUnit(0)->bind(pCursorTexture);

            const LLVector2& curoff = getCursorHotspotOffset(cursorType);
            LLVector3 l	= camera->getLeftAxis() * scalingFactor;
            LLVector3 u	= camera->getUpAxis()   * scalingFactor;
            LLVector3 co = (curoff[VX] * l) + (curoff[VY] * u);
            // mouse-pointer hotspot is at the intersection point
            LLVector3 bottomLeft	= pt - u + co;
            LLVector3 bottomRight	= pt - l - u + co;
            LLVector3 topLeft		= pt + co;
            LLVector3 topRight		= pt - l + co;
            // only go to 0.98 of the texture width so that annoying lines on right side and annoying dot in lower left
            // are not drawn.  Even though the textures are blank in these areas, we still render some weird artifacts
            // for no apparent reason.  Only going to 0.98 of the texture width seems to solve this problem and since
            // no mouse cursor textures go all the way to the right or bottom sides anyway, there's no loss of quality.
            const F32 kMinTC = 0.02f;
            const F32 kMaxTC = 0.98f;
            gGL.begin( LLRender::TRIANGLE_STRIP );
                gGL.texCoord2f( 0.0f,   kMinTC ); gGL.vertex3fv( bottomLeft.mV );
                gGL.texCoord2f( kMaxTC, kMinTC ); gGL.vertex3fv( bottomRight.mV );
                gGL.texCoord2f( 0.0f,   1.0f   ); gGL.vertex3fv( topLeft.mV );
            gGL.end();
            gGL.begin( LLRender::TRIANGLE_STRIP );
                gGL.texCoord2f( kMaxTC, kMinTC ); gGL.vertex3fv( bottomRight.mV );
                gGL.texCoord2f( kMaxTC, 1.0f   ); gGL.vertex3fv( topRight.mV );
                gGL.texCoord2f( 0.0f,   1.0f   ); gGL.vertex3fv( topLeft.mV );
            gGL.end();
            gGL.flush();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gOneTextureNoColorProgram.unbind();
            }
        }
        else
        {
            gGL.translatef( mMouseWorldRaycastIntersection.getScalarAt<0>().getF32(),
                            mMouseWorldRaycastIntersection.getScalarAt<1>().getF32(),
                            mMouseWorldRaycastIntersection.getScalarAt<2>().getF32());
            LLVector4a debug_binormal;
            debug_binormal.setCross3(mMouseWorldRaycastNormal, mMouseWorldRaycastTangent);
            debug_binormal.mul(mMouseWorldRaycastTangent.getScalarAt<3>().getF32());
            LLVector3 normal(mMouseWorldRaycastNormal.getF32ptr());
            LLVector3 binormal(debug_binormal.getF32ptr());

            LLCoordFrame orient;
            orient.lookDir(normal, binormal);
            LLMatrix4 rotation;
            orient.getRotMatrixToParent(rotation);
            gGL.multMatrix((float*)rotation.mMatrix);

            if (LLGLSLShader::sNoFixedFunction)
            {
                gDebugProgram.bind();
            }
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

            gGL.diffuseColor4f(1,0,0,0.5f);
            drawBox(LLVector3::zero, LLVector3(0.1f * scalingFactor, 0.022f * scalingFactor, 0.022f * scalingFactor));
            gGL.diffuseColor4f(0,1,0,0.5f);
            drawBox(LLVector3::zero, LLVector3(0.021f * scalingFactor, 0.1f * scalingFactor, 0.021f * scalingFactor));
            gGL.diffuseColor4f(0,0,1,0.5f);
            drawBox(LLVector3::zero, LLVector3(0.02f * scalingFactor, 0.02f * scalingFactor, 0.1f * scalingFactor));

            gGL.flush();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gDebugProgram.unbind();
            }
        }
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }
}


void LLHMD::render3DUI()
{
    LLViewerDisplay::pop_state_gl();

    render_hud_elements();  // in-world text, labels, nametags
    if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        //LLFastTimer t(FTM_RENDER_UI);
        LLViewerDisplay::render_ui_3d(FALSE);
        LLGLState::checkStates();
    }

    renderCursor3D();

    if (gPipeline.mUIScreen.isComplete())
    {
        if (gPipeline.mHMDUISurface.isNull())
        {
            gPipeline.mHMDUISurface = createUISurface();
        }
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();
        LLMatrix4 m1(mUIModelViewInv);
        LLMatrix4 m2(mBaseModelView);
        LLMatrix4 mt(0.0f, 0.0f, 0.0f, LLVector4(0.0f, 0.0f, gHMD.getUIEyeDepth(), 1.0f));
        m2 *= mt;
        m1 *= m2;
        gGL.loadMatrix((GLfloat*)m1.mMatrix);
        gOneTextureNoColorProgram.bind();
        gGL.setColorMask(true, true);
        gGL.getTexUnit(0)->bind(&gPipeline.mUIScreen);
        LLVertexBuffer* buff = gPipeline.mHMDUISurface;
        {
            LLGLDisable cull(GL_CULL_FACE);
            LLGLEnable blend(GL_BLEND);
            buff->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0);
            buff->drawRange(LLRender::TRIANGLES, 0, buff->getNumVerts()-1, buff->getNumIndices(), 0);
        }
        gOneTextureNoColorProgram.unbind();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }
    else if (gHMD.getCurrentEye() == LLHMD::RIGHT_EYE)
    {
        if (!gPipeline.mUIScreen.allocate(gHMD.getHMDUIWidth(), gHMD.getHMDUIHeight(), GL_RGBA, FALSE, FALSE, LLTexUnit::TT_TEXTURE, TRUE))
        {
            LL_WARNS() << "could not allocate UI buffer for HMD render mode" << LL_ENDL;
            return;
        }
    }

    if (gAgentCamera.cameraMouselook() && gHMD.getCurrentEye() != LLHMD::CENTER_EYE)
    {
        LLTool* toolBase = LLToolMgr::getInstance()->getCurrentTool();
        LLToolCompGun* tool =  LLToolCompGun::getInstance();
        if (toolBase != NULL && toolBase != gToolNull && toolBase == tool && !tool->isInGrabMode())
        {
            LLToolGun* gun = tool->getToolGun();
            LLGLSDefault gls_default;
            LLGLSUIDefault gls_ui;
            gPipeline.disableLights();
            gGL.setColorMask(true, false);
            gGL.color4f(1,1,1,1);

            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.bind();
            }
            LLViewerDisplay::push_state_gl();
            gHMD.setup2DRender();
            gun->drawCrosshairs((gHMD.getHMDViewportWidth() / 2), gHMD.getHMDViewportHeight() / 2);
            LLViewerDisplay::pop_state_gl();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.unbind();
            }
        }
    }

    LLViewerDisplay::push_state_gl_identity();
}


void LLHMD::prerender2DUI()
{
    if (gHMD.getCurrentEye() == LLHMD::RIGHT_EYE)
    {
        gPipeline.mUIScreen.bindTarget();
        gGL.setColorMask(true, true);
        glClearColor(0.0f,0.0f,0.0f,0.0f);
        gPipeline.mUIScreen.clear();
        gGL.color4f(1,1,1,1);
        LLUI::setDestIsRenderTarget(TRUE);
        // this is necessary even though it theoretically already is using that blend type due to
        // setting the DestIsRenderTarget flag
        gGL.setSceneBlendType(LLRender::BT_ALPHA);
        gViewerWindow->reshape(gHMD.getHMDUIWidth(), gHMD.getHMDUIHeight(), TRUE);
    }
}


void LLHMD::postRender2DUI()
{
    //if (getCurrentEye() == LLHMD::LEFT_EYE && LLViewerDisplay::gDisplaySwapBuffers)
    //{
    //    LLViewerDisplay::gDisplaySwapBuffers = FALSE;
    //}
    //else 
    if (getCurrentEye() == LLHMD::RIGHT_EYE)
    {
        gHMD.renderCursor2D();
        gPipeline.mUIScreen.flush();
        if (LLRenderTarget::sUseFBO)
        {
            //copy depth buffer from mScreen to framebuffer
            LLRenderTarget::copyContentsToFramebuffer(gPipeline.mScreen, 0, 0, gPipeline.mScreen.getWidth(), gPipeline.mScreen.getHeight(), 
                0, 0, gPipeline.mScreen.getWidth(), gPipeline.mScreen.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        }
        LLUI::setDestIsRenderTarget(FALSE);
        gViewerWindow->reshape(mImpl->getViewportWidth(), mImpl->getViewportHeight(), TRUE);
    }
}


BOOL LLHMD::beginFrame()
{
    mAgentRot = gAgent.getFrameAgent().getQuaternion();
    if (isAgentAvatarValid() && gAgentAvatarp->getParent())
    {
        LLViewerObject* rootObject = (LLViewerObject*)gAgentAvatarp->getRoot();
        if (rootObject && !rootObject->flagCameraDecoupled())
        {
            mAgentRot *= ((LLViewerObject*)(gAgentAvatarp->getParent()))->getRenderRotation();
        }
    }

    return mImpl ? mImpl->beginFrame() : FALSE;
}


BOOL LLHMD::endFrame()
{
    return mImpl ? mImpl->endFrame() : FALSE;
}


void LLHMD::showHSW(BOOL show)
{
    if (mImpl)
    {
        mImpl->showHSW(show);
    }
}


LLQuaternion LLHMD::getHMDRotation() const
{
    return mImpl ? mImpl->getHMDRotation() : LLQuaternion();
}


LLVector3 LLHMD::getCurrentEyeCameraOffset() const
{
    return mImpl ? mImpl->getCurrentEyeCameraOffset() : LLVector3::zero;
}

