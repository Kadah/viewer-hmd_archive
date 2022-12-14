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
#elif LL_WINDOWS
    #include "llwindowwin32.h"
#endif

//#include "OVR.h"

#if LL_DARWIN
// hack around an SDK warning that becomes an error with our compilation settings
#define __gl_h_
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#endif

#if LL_WINDOWS
#include "OVR_CAPI_GL.h"
#endif

#include "llviewerdisplay.h"
#include "pipeline.h"
#include "raytrace.h"

// external methods
void drawBox(const LLVector3& c, const LLVector3& r);
void push_state_gl();
void push_state_gl_identity();
void pop_state_gl();

const S32 LLHMDImpl::kDefaultHResolution          = 1280;
const S32 LLHMDImpl::kDefaultVResolution          = 1600;
const F32 LLHMDImpl::kDefaultInterpupillaryOffset = 0.064f;
const F32 LLHMDImpl::kDefaultEyeToScreenDistance  = 0.047f;  // A lens = 0.047f, B lens = 0.044f, C lens = 0.040f.   Default of 0.041f from the SDK is just WRONG and will cause visual distortion (Rift-46)
const F32 LLHMDImpl::kDefaultVerticalFOVRadians   = 2.196863;
const F32 LLHMDImpl::kDefaultAspect               = kDefaultHResolution / kDefaultVResolution;

LLHMD gHMD;

LLHMD::LLHMD()
    : mImpl(NULL)
    , mFlags(0)
    , mRenderMode(RenderMode_Normal)
    , mInterpupillaryDistance(0.064f)
    , mUIEyeDepth(0.65f)
    , mUIShapePreset(0)
    , mNextUserPresetIndex(1)
    , mMouseWorldSizeMult(5.0f)
    , mMouselookControlMode(0)
    , mMouselookRotThreshold(45.0f * DEG_TO_RAD)
    , mMouselookRotMax(30.0f * DEG_TO_RAD)
    , mMouselookTurnSpeedMax(0.1f)
    , mMonoCameraPosition(LLVector3())
{
    memset(&mUIShape, 0, sizeof(UISurfaceShapeSettings));
    mUIShape.mPresetType = (U32)LLHMD::kCustom;
    mUIShape.mPresetTypeIndex = 0;
    // "Custom" preset is always in index 0
    mUIPresetValues.push_back(mUIShape);
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
    // You had your shot at VR stardom, and you missed.    
    if (gHMD.isFailedInit())
    {
        return FALSE;
    }
    // Saul Goodman. Carry on.
    else if (gHMD.isInitialized())
    {
        return TRUE;
    }

    BOOL initResult = FALSE;

#if LL_HMD_OPENVR_SUPPORTED
    if (!mImpl)
    {
        mImpl = new LLHMDImplOpenVR();
    }
#endif

#if LL_HMD_OCULUS_SUPPORTED
    if (!mImpl)
    {
        mImpl = new LLHMDImplOculus();
    }
#endif

    gSavedSettings.getControl("HMDPixelDensity")->getSignal()->connect(boost::bind(&onChangeRenderSettings));
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
    gSavedSettings.getControl("HMDMouselookYawOnly")->getSignal()->connect(boost::bind(&onChangeMouselookSettings));
    onChangeMouselookSettings();
    gSavedSettings.getControl("HMDMouselookControlMode")->getSignal()->connect(boost::bind(&onChangeMouselookControlMode));
    onChangeMouselookControlMode();
    gSavedSettings.getControl("HMDAllowTextRoll")->getSignal()->connect(boost::bind(&onChangeAllowTextRoll));
    onChangeAllowTextRoll();
    initResult = mImpl->init();
    if (initResult)
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
        isInitialized(TRUE);
    }
    else
    {
        isInitialized(FALSE);
        isHMDConnected(FALSE);
    }

    return initResult;
}

void LLHMD::onChangeUIMagnification()
{
    gHMD.setUIMagnification(gSavedSettings.getF32("HMDUIMagnification"));
}

void LLHMD::onChangeUISurfaceSavedParams()
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

void LLHMD::onChangeUIShapePreset()
{
    gHMD.setUIShapePresetIndex(gSavedSettings.getS32("HMDUIShapePreset"));
}

void LLHMD::onChangeWorldCursorSizeMult()
{    
    gHMD.mMouseWorldSizeMult = gSavedSettings.getF32("HMDWorldCursorSizeMult");
}

void LLHMD::onChangeMouselookControlMode()
{
    gHMD.setMouselookControlMode(gSavedSettings.getS32("HMDMouselookControlMode"));
}

void LLHMD::onChangeAllowTextRoll()
{
    gHMD.allowTextRoll(gSavedSettings.getBOOL("HMDAllowTextRoll"));
}

void LLHMD::onChangeMouselookSettings()
{
    gHMD.mMouselookRotThreshold = llclamp(gSavedSettings.getF32("HMDMouselookRotThreshold") * DEG_TO_RAD, (10.0f * DEG_TO_RAD), (80.0f * DEG_TO_RAD));
    gHMD.mMouselookRotMax = llclamp(gSavedSettings.getF32("HMDMouselookRotMax") * DEG_TO_RAD, (1.0f * DEG_TO_RAD), (90.0f * DEG_TO_RAD));
    gHMD.mMouselookTurnSpeedMax = llclamp(gSavedSettings.getF32("HMDMouselookTurnSpeedMax"), 0.01f, 0.5f);
    gHMD.isMouselookYawOnly(gSavedSettings.getBOOL("HMDMouselookYawOnly"));
}

void LLHMD::onChangeUISurfaceShape()
{
    gPipeline.mHMDUISurface = NULL;
}

void LLHMD::onChangeRenderSettings()
{
    F32 pixelDensity = gSavedSettings.getF32("HMDPixelDensity");
    gHMD.setPixelDensity(pixelDensity);
}

void LLHMD::shutdown()
{
    if (mImpl)
    {
        delete mImpl;
        mImpl = NULL;
    }
    mCursorTextures.clear();
    mFlags = 0;
}

void LLHMD::setRenderMode(U32 mode, bool setFocusWindow)
{
    U32 newRenderMode = llclamp(mode, (U32)RenderMode_Normal, (U32)RenderMode_Last);
    if (newRenderMode != mRenderMode)
    {
        LLWindow* windowp = gViewerWindow->getWindow();

        U32 oldMode = mRenderMode;
        mRenderMode = newRenderMode;

        switch (oldMode)
        {
            case RenderMode_HMD:
            {
                switch (mRenderMode)
                {
                case RenderMode_HMD:
                    break;

                case RenderMode_Normal:
                default:
                    windowp->enableVSync(FALSE);
                    break;
                }
            }
            break;

            case RenderMode_Normal:
            default:
            {               
                switch (mRenderMode)
                {
                    case RenderMode_HMD:
                    {
                        windowp->enableVSync(TRUE);
                    }
                    break;
                }

                LLFloaterCamera::onHMDChange();
            }
            break;
        }

        if (mImpl && isHMDMode())
        {
            mImpl->resetOrientation();
        }
		//RIFT TODO:  is Y of screen < HMD height, the display in the rift is bad but mirroring looks okay. 
		gViewerWindow->reshape(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
    }
}

F32 LLHMD::getPixelDensity() const { return mImpl ? mImpl->getPixelDensity() : 1.0f; }
void LLHMD::setPixelDensity(F32 pixelDensity) { if (mImpl) { mImpl->setPixelDensity(pixelDensity); } }
S32 LLHMD::getViewportWidth() const { return mImpl ? mImpl->getViewportWidth() : 0; }
S32 LLHMD::getViewportHeight() const { return mImpl ? mImpl->getViewportHeight() : 0; }
F32 LLHMD::getInterpupillaryOffset() const { return mImpl ? mImpl->getInterpupillaryOffset() : 0.0f; }
F32 LLHMD::getEyeToScreenDistance() const { return mImpl ? mImpl->getEyeToScreenDistance() : 0.0f; }
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

void LLHMD::resetOrientation()
{
    if (mImpl)
    {
        mImpl->resetOrientation();
    }
}

LLVector3 LLHMD::getHeadPosition() const
{
    if (mImpl)
    {
        return mImpl->getHeadPosition();
    }

    return LLVector3::zero;
}

F32 LLHMD::getVerticalFOV() const
{
    return mImpl ? mImpl->getVerticalFOV() : 0.0f;
}

F32 LLHMD::getAspect()
{
    return mImpl ? mImpl->getAspect() : 0.0f;
}

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
        //F32 eyeDepthMult = mUIShape.mUIMagnification / ((getEyeToScreenDistance() - 0.083f) * 1000.0f);
        //mUIEyeDepth = ((mImpl->getEyeToScreenDistance() - 0.083f) * eyeDepthMult);
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
        setUIShapePresetIndex(getUIShapePresetIndexDefault());
    }
    return TRUE;
}

void LLHMD::saveSettings()
{
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
    gSavedSettings.setBOOL("HMDMouselookYawOnly", gHMD.isMouselookYawOnly());
    gSavedSettings.setBOOL("HMDAllowTextRoll", gHMD.allowTextRoll());
    gSavedSettings.setF32("HMDPixelDensity", gHMD.getPixelDensity());

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

void LLHMD::calculateMouseToroidIntersectWorldSpace(S32 mouse_x, S32 mouse_y, LLVector3& intersection)
{
    if (!isHMDMode())
    {
        memset(&intersection, 0xFF, sizeof(LLVector3));
        return;
    }

    if (gAgentCamera.cameraMouselook())
    {
        LLViewerCamera* camera = LLViewerCamera::getInstance();
        intersection.set(camera->getOrigin() + (camera->getAtAxis() * camera->getNear()));
    }
    else
    {
        // 1. determine horizontal and vertical percentage within toroidal UI surface based on mouse_x, mouse_y
        F32 uiw = (F32)gHMD.getViewportWidth();
        F32 uih = (F32)gHMD.getViewportHeight();
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
		intersection.set(w[VX], w[VY], w[VZ]);
    }
}


void LLHMD::updateHMDMouseInfo()
{
    calculateMouseToroidIntersectWorldSpace(gViewerWindow->getCurrentMouse().mX, gViewerWindow->getCurrentMouse().mY, mMouseToroidIntersect);
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
            if (tool->hasMouseIntersectGlobal())
            {
                LLVector3 newMouseIntersect = gAgent.getPosAgentFromGlobal(tool->getMouseIntersectGlobal());
                gHMD.cursorIntersectsWorld(TRUE);
                gHMD.setMouseWorldRaycastIntersection(newMouseIntersect);
            }
        }
        return TRUE;
    }

    return FALSE;
}

void LLHMD::setupStereoValues()
{
    mMonoCameraPosition = LLViewerCamera::getInstance()->getOrigin();
}

void LLHMD::setupStereoCullFrustum()
{
    LLViewerCamera* cam = LLViewerCamera::getInstance();
    LLVector3 headPosWorld = getHeadPosition() + mMonoCameraPosition;

    cam->setOrigin(headPosWorld);
    cam->setPerspective(!FOR_SELECTION, 0, 0, getViewportWidth() * 2, getViewportHeight(), FALSE, cam->getNear(), cam->getFar());
}

void LLHMD::setup3DViewport(S32 x_offset, S32 y_offset)
{
    S32 w = getViewportWidth();
    S32 h = getViewportHeight();
    glViewport(x_offset, y_offset, w, h);
}

void LLHMD::setup3DRender(int which_eye)
{
    LLViewerCamera* cam = LLViewerCamera::getInstance();

    mEyeOffset[which_eye].setZero();
    mEyeProjection[which_eye].make_identity();

    mImpl->getEyeOffset(which_eye, mEyeOffset[which_eye]);
    mImpl->getEyeProjection(which_eye, mEyeProjection[which_eye], cam->getNear(), cam->getFar());

    LLVector3 eyePos = mMonoCameraPosition + getHeadPosition() + mEyeOffset[which_eye];

    // nope, ossm matrix management still screws up returning to 2D from HMD mode
    //cam->setView(mImpl->getVerticalFOV(), FALSE);

    cam->setOrigin(eyePos);    
    cam->setAspect(mImpl->getAspect());
    cam->setProjectionMatrix(mEyeProjection[which_eye]);
}

void LLHMD::setup2DRender()
{
    S32 w = getViewportWidth();
	S32 h = gViewerWindow->getWindowHeightRaw();
	

	gl_state_for_2d(w, h, 0, 0);
	glViewport(0, 0, w, h);
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

    BOOL beginFrameResult = FALSE;

    if (mImpl && isHMDMode())
    {
        beginFrameResult = mImpl->beginFrame();
    }

    return beginFrameResult;
}

BOOL LLHMD::bindEyeRenderTarget(int which_eye)
{
    if (mImpl)
    {
        return mImpl->bindEyeRenderTarget(which_eye);
    }

    return FALSE;
}

BOOL LLHMD::flushEyeRenderTarget(int which_eye)
{
    if (mImpl)
    {
        return mImpl->flushEyeRenderTarget(which_eye);
    }

    return FALSE;
}

BOOL LLHMD::copyToEyeRenderTarget(int which_eye, LLRenderTarget& source, int mask)
{
    if (mImpl)
    {
        return mImpl->copyToEyeRenderTarget(which_eye, source, mask);
    }

    return FALSE;
}

BOOL LLHMD::releaseEyeRenderTarget(int which_eye)
{
    if (mImpl)
    {
        return mImpl->releaseEyeRenderTarget(which_eye);
    }

    return FALSE;
}

BOOL LLHMD::endFrame()
{
    return mImpl ? mImpl->endFrame() : FALSE;
}

BOOL LLHMD::postSwap()
{
    return mImpl ? mImpl->postSwap() : FALSE;
}

LLQuaternion LLHMD::getHMDRotation() const
{
    return mImpl ? mImpl->getHMDRotation() : LLQuaternion();
}

U32 LLHMD::suspendHMDMode()
{
    U32 oldMode = getRenderMode();
    if (isHMDMode())
    {
        setRenderMode(RenderMode_Normal);
    }
    return oldMode;
}

void LLHMD::resumeHMDMode(U32 prevRenderMode)
{
    if (prevRenderMode != RenderMode_Normal)
{
        setRenderMode(prevRenderMode);
    }
}

BOOL LLHMD::releaseAllEyeRenderTargets()
{
    if (mImpl)
    {
        return mImpl->releaseAllEyeRenderTargets();
    }

    return FALSE;
}

void LLHMD::renderCursor2D()
{
    if (isHMDMode() && (!gAgentCamera.cameraMouselook() || gAgent.leftButtonGrabbed()))
    {
        /*LLWindow* window = gViewerWindow->getWindow();
        if (!window || window->isCursorHidden())
        {
            return;
        }*/

        gGL.pushMatrix();
        gGL.pushUIMatrix();
        if (LLGLSLShader::sNoFixedFunction)
        {
            gUIProgram.bind();
        }
        S32 mx = gViewerWindow->getCurrentMouseX();
        S32 my = gViewerWindow->getCurrentMouseY();

        F32 ssx = (F32)mx / (F32)gViewerWindow->getWindowRectRaw().getWidth();
        F32 ssy = (F32)my / (F32)gViewerWindow->getWindowRectRaw().getHeight();

        F32 viewX = ssx * (F32)gHMD.getViewportWidth();
        F32 viewY = ssy * (F32)gHMD.getViewportHeight();

        mx = S32(viewX);
        my = S32(viewY);

        U32 cursorType = (U32)gViewerWindow->getWindow()->getCursor();
        LLViewerTexture* pCursorTexture = getCursorImage(cursorType);
        if (pCursorTexture)
        {
            const LLVector2& curoff = getCursorHotspotOffset(cursorType);
            S32 offx = ll_round(-32.0f * curoff[VX]);
            S32 offy = -32 + ll_round(32.0f * curoff[VY]);
            gl_draw_scaled_image(mx + offx, my + offy, 32, 32, pCursorTexture);
        }
        else
        {
            gl_line_2d(mx - 16, my, mx + 16, my, LLColor4(1.0f, 0.0f, 0.0f));
            gl_line_2d(mx, my - 16, mx, my + 16, LLColor4(0.0f, 1.0f, 0.0f));
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

void LLHMD::renderUserInterfaceInVR(int which_eye, LLRenderTarget& uiScreenRenderTarget)
{
    push_state_gl();

    if (gPipeline.mHMDUISurface.isNull())
    {
        gPipeline.mHMDUISurface = createUISurface();
    }
    
    gOneTextureNoColorProgram.bind();

    {
        static F32 depth = -gHMD.getUIEyeDepth();

        LLMatrix4 zTranslateMat(0.0f, 0.0f, 0.0f, LLVector4(0.0f, 0.0f, depth, 1.0f));
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix((GLfloat*)zTranslateMat.mMatrix);

        gGL.setColorMask(true, true);

        uiScreenRenderTarget.bindTexture(0, 0);

        LLVertexBuffer* buff = gPipeline.mHMDUISurface;
        {
            LLGLDisable cull(GL_CULL_FACE);
            LLGLEnable  blend(GL_BLEND);
            LLGLDisable noDepth(GL_DEPTH_TEST);
            buff->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0);
            buff->drawRange(LLRender::TRIANGLES, 0, buff->getNumVerts() - 1, buff->getNumIndices(), 0);
        }
    }

    gOneTextureNoColorProgram.unbind();

    if (gAgentCamera.cameraMouselook())
    {
        LLTool* toolBase = LLToolMgr::getInstance()->getCurrentTool();
        LLToolCompGun* tool = LLToolCompGun::getInstance();
        if (toolBase != NULL && toolBase != gToolNull && toolBase == tool && !tool->isInGrabMode())
        {
            LLToolGun* gun = tool->getToolGun();
            LLGLSDefault gls_default;
            LLGLSUIDefault gls_ui;
            gPipeline.disableLights();
            gGL.setColorMask(true, false);
            gGL.color4f(1, 1, 1, 1);

            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.bind();
            }
            gHMD.setup2DRender();
            gun->drawCrosshairs((gHMD.getViewportWidth() / 2), gHMD.getViewportHeight() / 2);
            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.unbind();
            }
        }
    }

    pop_state_gl();    
}
