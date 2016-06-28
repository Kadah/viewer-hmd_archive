/** 
* @file llhmdimploculus.cpp
* @brief Implementation of llhmd
* @author voidpointer@lindenlab.com
* @author callum@lindenlab.com
* @author graham@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2016, Linden Research, Inc.
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

#if LL_HMD_OCULUS_SUPPORTED
#include "llhmdimploculus.h"
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

#include "OVR_CAPI_GL.h"

// Disables use of Oculus API elements that prevent using nVidia nSight debugging.
// Performs exactly the same eye target rendering and bounce to default framebuffer,
// but doesn't submit frames to Oculus' compositor which uses DX, which causes
// poor nSight to give up the ghost upon seeing DX and GL used at the same time.
// nV support says this won't be fixed any time soon... :(
#define NSIGHT_DEBUG 0

struct LLHMDImplOculus::OculusData
{
    LLHMDImplOculus::OculusData()
    {
        mMirrorTexture = 0;
    }

    // Oculus-Specific data structures
    ovrSession          mHMD;
    ovrHmdDesc          mHMDDesc;
    ovrGraphicsLuid     mLUID;
    ovrSizei            mViewport;
    ovrPosef            mEyeRenderPose[ovrEye_Count];
    ovrMatrix4f         mProjection[ovrEye_Count];
    ovrEyeRenderDesc    mEyeRenderDesc[ovrEye_Count];
    ovrTextureSwapChain mSwapChain[ovrEye_Count];    
    ovrMirrorTexture    mMirrorTexture;
};

LLHMDImplOculus::LLHMDImplOculus()
: mOculus(NULL)
, mFrameIndex(0)
, mTrackingCaps(0)
, mEyeToScreenDistance(kDefaultEyeToScreenDistance)
, mInterpupillaryDistance(kDefaultInterpupillaryOffset)
, mVerticalFovRadians(kDefaultVerticalFOVRadians)
, mAspect(kDefaultAspect)
, mMirrorRT(NULL)
{
    mOculus = new LLHMDImplOculus::OculusData;
    mEyeRotation.setEulerAngles(0.0f, 10.0f, 30.0f); // "haters gonna hate" sane defaults (better than NaNs)
    mHeadPos.set(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < ovrEye_Count; ++i)
    {
        for (int t = 0; t < 3; ++t)
        {
            mEyeRenderTarget[i][t] = nullptr;
            mSwapTexture[i][t] = 0;
        }
    }
}

LLHMDImplOculus::~LLHMDImplOculus()
{
    shutdown();
    delete mOculus;
    mOculus = NULL;
}


// Oculus method for probing headset presence without DLL dep.
// Not something to be called per frame as it's kinda 'spensive.
bool HasHeadMountedDisplay()
{    
    const int cTimeoutMs = 512;
    ovrDetectResult result = ovr_Detect(cTimeoutMs);
    if (result.IsOculusServiceRunning)
    {
        return result.IsOculusHMDConnected;
    }
    return false;
}

BOOL LLHMDImplOculus::init()
{    
    ovrInitParams params;
    params.Flags                 = ovrInit_Debug;
    params.RequestedMinorVersion = OVR_MINOR_VERSION;
    params.LogCallback           = NULL;
    params.UserData              = NULL;
    params.ConnectionTimeoutMS   = 0;

    // Initializes LibOVR, and the Rift
    ovrResult init_result = ovr_Initialize(&params);
    if (OVR_FAILURE(init_result))
    {
        LL_INFOS("HMD") << "HMD Init Failed. Possibly missing VR runtime?" << LL_ENDL;
        gHMD.isInitialized(FALSE);
        gHMD.isFailedInit(TRUE);
        return FALSE;       
    }

    ovrGraphicsLuid luid;

    memset(&luid, 0, sizeof(ovrGraphicsLuid));

    ovrResult ovr_result = ovr_Create(&mOculus->mHMD, &luid);

    if (OVR_FAILURE(ovr_result))
    {
        LL_INFOS("Oculus") << "ovr_Create failed." << LL_ENDL;
        return FALSE;
    }

    mOculus->mHMDDesc = ovr_GetHmdDesc(mOculus->mHMD);

    mTrackingCaps = ovr_GetTrackingState(mOculus->mHMD, 0.0, ovrTrue).StatusFlags;
    
    gHMD.isHMDConnected(true);

    calculateViewportSettings();

    return TRUE;
}

void LLHMDImplOculus::destroy()
{
    gHMD.isHMDConnected(false);
    gHMD.setRenderMode(LLHMD::RenderMode_Normal);

    destroySwapChains();

    if (mOculus->mHMD)
    {
        ovr_Destroy(mOculus->mHMD);        
    }    
    mOculus->mHMD = NULL;
}

void LLHMDImplOculus::shutdown()
{
    if (!gHMD.isInitialized())
    {
        return;
    }

    // make sure if/when we call shutdown again, we don't try to deallocate things twice.
    gHMD.isInitialized(FALSE);

    destroy();

    ovr_Shutdown();
}

BOOL LLHMDImplOculus::calculateViewportSettings()
{
    for (int i = 0; i < ovrEye_Count; ++i)
    {
        mOculus->mEyeRenderDesc[i] = ovr_GetRenderDesc(mOculus->mHMD, (ovrEyeType)i, mOculus->mHMDDesc.DefaultEyeFov[i]);
    }

    mOculus->mViewport = ovr_GetFovTextureSize(mOculus->mHMD, ovrEye_Left, mOculus->mEyeRenderDesc[ovrEye_Left].Fov, getPixelDensity());

    ovrFovPort l = mOculus->mEyeRenderDesc[ovrEye_Left ].Fov;
    ovrFovPort r = mOculus->mEyeRenderDesc[ovrEye_Right].Fov;

    float totalFovRadiansV = (fabs(l.DownTan)  + fabs(r.DownTan)  + fabs(l.UpTan)   + fabs(r.UpTan))   * 0.5f;
    float totalFovRadiansH = (fabs(l.RightTan) + fabs(r.RightTan) + fabs(l.LeftTan) + fabs(r.LeftTan)) * 0.5f;

    mVerticalFovRadians = totalFovRadiansV;
    mAspect             = totalFovRadiansH / totalFovRadiansV;

    return TRUE;
}

void LLHMDImplOculus::destroySwapChains()
{
    for (int i = 0; i < ovrEye_Count; ++i)
    {
        // Release Oculus' swap textures from our RTs before we destroy them (twice!).
        for (int t = 0; t < 3; ++t)
        {
            if (mEyeRenderTarget[i][t])
            {
                delete mEyeRenderTarget[i][t];
                mEyeRenderTarget[i][t] = nullptr;
            }

            ovr_DestroyTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[i]);
        }
    }

    if (mMirrorRT)
    {
        mMirrorRT->setAttachment(0, 0); // remove ref to oculus mirror texture...

        if (mOculus->mMirrorTexture)
        {
            ovr_DestroyMirrorTexture(mOculus->mHMD, mOculus->mMirrorTexture);
        }
        mOculus->mMirrorTexture = 0;

        delete mMirrorRT;
    }

    mMirrorRT = NULL;
}

BOOL LLHMDImplOculus::initSwapChains()
{
    mNsightDebugMode = gSavedSettings.getBOOL("NsightDebug");

    BOOL success = TRUE;

    for (int i = 0; success && i < ovrEye_Count; ++i)
    {
        success &= initSwapChain(i);
    }

    return success;
}

void dumpOculusError(const char* caller)
{
    ovrErrorInfo errorInfo;
    ovr_GetLastErrorInfo(&errorInfo);
    LL_ERRS() << caller << " failed: " << errorInfo.ErrorString << LL_ENDL;
}

BOOL LLHMDImplOculus::initSwapChain(int eyeIndex)
{    
    if (mNsightDebugMode)
    {
        for (int texIndex = 0; texIndex < 3; ++texIndex)
        {
            mEyeRenderTarget[eyeIndex][texIndex] = new LLRenderTarget();
            mEyeRenderTarget[eyeIndex][texIndex]->allocate(mOculus->mViewport.w, mOculus->mViewport.h, GL_RGBA, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, TRUE, 1);
        }
    }
    else
    {
        ovrTextureSwapChainDesc swapChainDesc;
        swapChainDesc.Type          = ovrTexture_2D;
        swapChainDesc.ArraySize     = 1;
        swapChainDesc.Width         = mOculus->mViewport.w;
        swapChainDesc.Height        = mOculus->mViewport.h;
        swapChainDesc.MipLevels     = 1;
        swapChainDesc.Format        = OVR_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleCount   = 1;
        swapChainDesc.StaticImage   = ovrFalse;
        swapChainDesc.MiscFlags     = ovrTextureMisc_None;
        swapChainDesc.BindFlags     = ovrTextureBind_None;

        ovrResult result = ovr_CreateTextureSwapChainGL(mOculus->mHMD, &swapChainDesc, &mOculus->mSwapChain[eyeIndex]);
        if (!OVR_SUCCESS(result))
        {
            dumpOculusError("ovr_CreateTextureSwapChainGL");
            return FALSE;
        }

        int length = 0;

        result = ovr_GetTextureSwapChainLength(mOculus->mHMD, mOculus->mSwapChain[eyeIndex], &length);
        if (!OVR_SUCCESS(result))
        {
            dumpOculusError("ovr_GetTextureSwapChainLength");
            return FALSE;
        }

        llassert(length <= 3);
        for (int texIndex = 0; texIndex < length; ++texIndex)
        {
            GLuint swapChainTextureId;

            result = ovr_GetTextureSwapChainBufferGL(mOculus->mHMD, mOculus->mSwapChain[eyeIndex], texIndex, &swapChainTextureId);
            if (!OVR_SUCCESS(result))
            {
                dumpOculusError("ovr_GetTextureSwapChainBufferGL");
                return FALSE;
            }

            mEyeRenderTarget[eyeIndex][texIndex] = new LLRenderTarget();
            mEyeRenderTarget[eyeIndex][texIndex]->allocate(mOculus->mViewport.w, mOculus->mViewport.h, GL_RGBA, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, TRUE, 1);
            mSwapTexture[eyeIndex][texIndex] = swapChainTextureId;
        }
    }

    return true;
}

void LLHMDImplOculus::resetFrameIndex()
{
    mFrameIndex = 0;
}

U32 LLHMDImplOculus::getFrameIndex()
{
    return mFrameIndex;
}

void LLHMDImplOculus::incrementFrameIndex()
{
    ++mFrameIndex;
}

BOOL LLHMDImplOculus::beginFrame()
{
    if (!mEyeRenderTarget[0][0])
    {
        if (!initSwapChains())
        {
            return false;
        }
    }

    ovrVector3f eyeOffsets[ovrEye_Count] =
    {
        mOculus->mEyeRenderDesc[ovrEye_Left ].HmdToEyeOffset,
        mOculus->mEyeRenderDesc[ovrEye_Right].HmdToEyeOffset
    };

    const double predictedDisplayTime      = ovr_GetPredictedDisplayTime(mOculus->mHMD, getFrameIndex());
    const ovrTrackingState  tracking_state = ovr_GetTrackingState(mOculus->mHMD, predictedDisplayTime, ovrTrue);

    int texIndex = getFrameIndex() % 3;

    ovr_CalcEyePoses(tracking_state.HeadPose.ThePose, eyeOffsets, mOculus->mEyeRenderPose);

    mTrackingCaps = tracking_state.StatusFlags;

    mEyeRotation.set(
        mOculus->mEyeRenderPose[0].Orientation.x,
        mOculus->mEyeRenderPose[0].Orientation.y,
        mOculus->mEyeRenderPose[0].Orientation.z,
        mOculus->mEyeRenderPose[0].Orientation.w);

    mHeadPos.set(tracking_state.HeadPose.ThePose.Position.x, tracking_state.HeadPose.ThePose.Position.y, tracking_state.HeadPose.ThePose.Position.z);

    glClearColor(0.5, 0, 0, 1);
    mEyeRenderTarget[0][texIndex]->bindTarget();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mEyeRenderTarget[0][texIndex]->getFBO());
    mEyeRenderTarget[0][texIndex]->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mEyeRenderTarget[0][texIndex]->flush();

    glClearColor(0, 0, 0.5, 1);
    mEyeRenderTarget[1][texIndex]->bindTarget();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mEyeRenderTarget[1][texIndex]->getFBO());    
    mEyeRenderTarget[1][texIndex]->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mEyeRenderTarget[1][texIndex]->flush();

    return TRUE;
}

BOOL LLHMDImplOculus::bindEyeRenderTarget(int which)
{
    if (!mEyeRenderTarget[0][0])
    {
        if (!initSwapChains())
        {
            return false;
        }
    }

    int texIndex = getFrameIndex() % 3;

    mEyeRenderTarget[which][texIndex]->bindTarget();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mEyeRenderTarget[which][texIndex]->getFBO());

    return true;
}

BOOL LLHMDImplOculus::flushEyeRenderTarget(int which)
{
    if (!mEyeRenderTarget[0][0])
    {
        return FALSE;
    }

    int texIndex = getFrameIndex() % 3;

    mEyeRenderTarget[which][texIndex]->flush();
    return TRUE;
}

BOOL LLHMDImplOculus::copyToEyeRenderTarget(int which_eye, LLRenderTarget& source, int mask)
{
    if (!mEyeRenderTarget[0][0])
    {
        return FALSE;
    }

	//Source contains the dimensions of the Desktop Window.  The w/h defined below are the size of the 
	//Rift display.   Smaller values will result in a scaled down version of the 3D elements in a portion of the viewport.
	//2D UI elements are unaffected by these values. 
    int texIndex = getFrameIndex() % 3;
    S32 w        = getViewportWidth();
    S32 h        = getViewportHeight();
    BOOL do_depth = (mask & GL_DEPTH_BUFFER_BIT) > 0;
    LLGLDepthTest depthTest(do_depth ? GL_TRUE : GL_FALSE, do_depth ? GL_TRUE : GL_FALSE);
	mEyeRenderTarget[which_eye][texIndex]->copyContents(
                                                source,
                                                0, 0, source.getWidth(), source.getHeight(),
                                                0, 0, w,                 h,
                                                mask, GL_NEAREST);
    return TRUE;
}

BOOL LLHMDImplOculus::releaseEyeRenderTarget(int which)
{
    if (!mEyeRenderTarget[0][0])
    {
        return FALSE;

    }

    if (!mNsightDebugMode)
    {
        int texIndex = getFrameIndex() % 3;
        S32 w = getViewportWidth();
        S32 h = getViewportHeight();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mEyeRenderTarget[which][texIndex]->getFBO());
        glBindTexture(GL_TEXTURE_2D, mSwapTexture[which][texIndex]);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
        glBindTexture(GL_TEXTURE_2D, 0);
        ovr_CommitTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[which]);
    }

    return true;
}

BOOL LLHMDImplOculus::releaseAllEyeRenderTargets()
{
    destroySwapChains();
    return true;
}

BOOL LLHMDImplOculus::endFrame()
{
    if (!mEyeRenderTarget[0][0])
    {
        if (!initSwapChains())
        {
            return FALSE;
        }
    }

    S32 viewport_w  = getViewportWidth();
	S32 viewport_h = gViewerWindow->getWindowHeightRaw(); //Controls the Y position of the menu bar.
    S32 window_w    = gViewerWindow->getWindowWidthRaw();
    S32 window_h    = gViewerWindow->getWindowHeightRaw();

    if (!mNsightDebugMode)
    {
        ovrLayerEyeFov eyeLayer;

        eyeLayer.Header.Type  = ovrLayerType_EyeFov;

        // OriginAtBottomLeft because OpenGL and HighQuality because HIGH KWALITY!
        eyeLayer.Header.Flags = ovrLayerFlag_HighQuality | ovrLayerFlag_TextureOriginAtBottomLeft;

        ovrRecti viewport;

        viewport.Pos.x = 0;
        viewport.Pos.y = 0;

        viewport.Size = mOculus->mViewport;

        eyeLayer.Viewport[0] = viewport;
        eyeLayer.Viewport[1] = viewport;

        eyeLayer.ColorTexture[0] = mOculus->mSwapChain[0];
        eyeLayer.ColorTexture[1] = mOculus->mSwapChain[1];
        eyeLayer.RenderPose[0]   = mOculus->mEyeRenderPose[0];
        eyeLayer.RenderPose[1]   = mOculus->mEyeRenderPose[1];
        eyeLayer.Fov[0]          = mOculus->mEyeRenderDesc[0].Fov;
        eyeLayer.Fov[1]          = mOculus->mEyeRenderDesc[1].Fov;

        ovrViewScaleDesc viewScaleDesc;
        viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

        viewScaleDesc.HmdToEyeOffset[0] = mOculus->mEyeRenderDesc[0].HmdToEyeOffset;
        viewScaleDesc.HmdToEyeOffset[1] = mOculus->mEyeRenderDesc[1].HmdToEyeOffset;

        ovrLayerHeader* layers[1] = { &eyeLayer.Header };

        ovrResult result = ovr_SubmitFrame(mOculus->mHMD, getFrameIndex(), &viewScaleDesc, layers, 1);
        incrementFrameIndex();

        if (!OVR_SUCCESS(result))
        {
            if ((result == ovrError_DisplayLost)
             || (result == ovrError_HardwareGone)
             || (result == ovrError_CatastrophicFailure))
            {
                gHMD.isHMDConnected(false);
                return FALSE;
            }
        }
    }

    int texIndex = getFrameIndex() % 3;

    // Copy left eye to left half of default framebuffer
    LLRenderTarget::copyContentsToFramebuffer(
        *(mEyeRenderTarget[0][texIndex]),
        0, 0, viewport_w,      viewport_h,
        0, 0, window_w >> 1,   window_h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Copy right eye to right half of default framebuffer
    LLRenderTarget::copyContentsToFramebuffer(
        *(mEyeRenderTarget[1][texIndex]),
        0, 0, viewport_w, viewport_h,
        window_w >> 1, 0, window_w, window_h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    return TRUE;
}

BOOL LLHMDImplOculus::postSwap()
{
    // Oculus does nada special at this point, this is just for API compliance...
    return FALSE;
}

F32 LLHMDImplOculus::getAspect() const
{
    return gHMD.isHMDMode() ? mAspect : LLViewerCamera::getInstance()->getAspect();
}

LLVector3 LLHMDImplOculus::getHeadPosition() const
{
    return mHeadPos;
}

void LLHMDImplOculus::getEyeProjection(int whichEye, glh::matrix4f& projOut, float zNear, float zFar) const
{
    ovrMatrix4f proj = ovrMatrix4f_Projection(mOculus->mHMDDesc.DefaultEyeFov[whichEye], zNear, zFar, ovrProjection_ClipRangeOpenGL);
    glh::matrix4f p = glh::matrix4f(&proj.M[0][0]);
    projOut = p.transpose();
}

void LLHMDImplOculus::getEyeOffset(int whichEye, LLVector3& offsetOut) const
{
    ovrVector3f offset = mOculus->mEyeRenderDesc[whichEye].HmdToEyeOffset;
    offsetOut.set(offset.x, offset.y, offset.z);
}

void LLHMDImplOculus::resetOrientation()
{
    ovr_RecenterTrackingOrigin(mOculus->mHMD);
}

F32 LLHMDImplOculus::getVerticalFOV() const
{
    return mVerticalFovRadians;
}

S32 LLHMDImplOculus::getViewportWidth() const
{
    return mOculus->mViewport.w;
}

S32 LLHMDImplOculus::getViewportHeight() const
{
    return mOculus->mViewport.h;
}

#endif // LL_HMD_SUPPORTED_OCULUS
