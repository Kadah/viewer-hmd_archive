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

#include "OVR_CAPI_GL.h"

struct LLHMDImplOculus::OculusData
{
    LLHMDImplOculus::OculusData()
    {
        mCurrentSwapChainIndex[0] = 0;
        mCurrentSwapChainIndex[1] = 0;
        mMirrorTexture            = 0;
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
    int                 mCurrentSwapChainIndex[ovrEye_Count];
};

LLHMDImplOculus::LLHMDImplOculus()
: mOculus(NULL)
, mFrameIndex(0)
, mSubmittedFrameIndex(0)
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
            mEyeRT[i][t] = nullptr;
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
    gHMD.setRenderMode(LLHMD::RenderMode_None);

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
            if (mEyeRT[i][t])
            {
                delete mEyeRT[i][t];
                mEyeRT[i][t] = nullptr;
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
    glEnable(GL_FRAMEBUFFER_SRGB);

    ovrTextureSwapChainDesc swapChainDesc;
    swapChainDesc.Type          = ovrTexture_2D;
    swapChainDesc.ArraySize     = 1;
    swapChainDesc.Width         = mOculus->mViewport.w;
    swapChainDesc.Height        = mOculus->mViewport.h;
    swapChainDesc.MipLevels     = 1;
    swapChainDesc.Format        = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
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

        glBindTexture(GL_TEXTURE_2D, swapChainTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
        glBindTexture(GL_TEXTURE_2D, 0);

        mEyeRT[eyeIndex][texIndex] = new LLRenderTarget();
        mEyeRT[eyeIndex][texIndex]->allocate(mOculus->mViewport.w, mOculus->mViewport.h, GL_SRGB8_ALPHA8, true, false, LLTexUnit::TT_RECT_TEXTURE, true, 1);

        mSwapTexture[eyeIndex][texIndex] = swapChainTextureId;
    }

    if (!mOculus->mMirrorTexture)
    {
        ovrMirrorTextureDesc mirrorTextureDesc;

        memset(&mirrorTextureDesc, 0, sizeof(mirrorTextureDesc));
        mirrorTextureDesc.Width  = gViewerWindow->getWindowWidthRaw();
        mirrorTextureDesc.Height = gViewerWindow->getWindowHeightRaw();
        mirrorTextureDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

        // Create mirror texture and an FBO used to copy mirror texture to back buffer
        result = ovr_CreateMirrorTextureGL(mOculus->mHMD, &mirrorTextureDesc, &mOculus->mMirrorTexture);
        if (!OVR_SUCCESS(result))
        {
            return FALSE;
        }

        // Configure the mirror read buffer
        GLuint mirrorTextureId;

        result = ovr_GetMirrorTextureBufferGL(mOculus->mHMD, mOculus->mMirrorTexture, &mirrorTextureId);
        if (!OVR_SUCCESS(result))
        {
            return FALSE;
        }    

        glBindTexture(GL_TEXTURE_2D, mirrorTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
        glBindTexture(GL_TEXTURE_2D, 0);

        mMirrorRT = new LLRenderTarget();
        mMirrorRT->allocate(mirrorTextureDesc.Width, mirrorTextureDesc.Height, GL_SRGB8_ALPHA8, false, false, LLTexUnit::TT_RECT_TEXTURE, true, 1);
        mMirrorRT->setAttachment(0, mirrorTextureId);
    }    

    return true;
}

void LLHMDImplOculus::resetFrameIndex()
{
    mFrameIndex = 0;
    mSubmittedFrameIndex = 0;
}

U32 LLHMDImplOculus::getFrameIndex()
{
    return mFrameIndex;
}

void LLHMDImplOculus::incrementFrameIndex()
{
    ++mFrameIndex;
}

U32 LLHMDImplOculus::getSubmittedFrameIndex()
{
    return mSubmittedFrameIndex;
}

void LLHMDImplOculus::incrementSubmittedFrameIndex()
{
    ++mSubmittedFrameIndex;
}

BOOL LLHMDImplOculus::beginFrame()
{
    ovrVector3f eyeOffsets[ovrEye_Count] =
    {
        mOculus->mEyeRenderDesc[ovrEye_Left ].HmdToEyeOffset,
        mOculus->mEyeRenderDesc[ovrEye_Right].HmdToEyeOffset
    };

    const double predictedDisplayTime      = ovr_GetPredictedDisplayTime(mOculus->mHMD, getFrameIndex());
    const ovrTrackingState  tracking_state = ovr_GetTrackingState(mOculus->mHMD, predictedDisplayTime, ovrTrue);

    ovr_CalcEyePoses(tracking_state.HeadPose.ThePose, eyeOffsets, mOculus->mEyeRenderPose);

    incrementFrameIndex();

    mTrackingCaps = tracking_state.StatusFlags;

    mEyeRotation.set(
        mOculus->mEyeRenderPose[0].Orientation.x,
        mOculus->mEyeRenderPose[0].Orientation.y,
        mOculus->mEyeRenderPose[0].Orientation.z,
        mOculus->mEyeRenderPose[0].Orientation.w);

    mHeadPos.set(tracking_state.HeadPose.ThePose.Position.x, tracking_state.HeadPose.ThePose.Position.y, tracking_state.HeadPose.ThePose.Position.z);

    return TRUE;
}

BOOL LLHMDImplOculus::bindEyeRT(int which)
{
    if (!mEyeRT[0][0])
    {
        if (!initSwapChains())
        {
            return false;
        }
    }

    int texIndex = mOculus->mCurrentSwapChainIndex[which];

    mEyeRT[which][texIndex]->bindTarget();
    mEyeRT[which][texIndex]->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

BOOL LLHMDImplOculus::releaseEyeRT(int which)
{
    if (!mEyeRT[0][0])
    {
        return false;
    }

    int texIndex = mOculus->mCurrentSwapChainIndex[which];

    S32 w = getViewportWidth();
    S32 h = getViewportHeight();

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindTexture(GL_TEXTURE_2D, mSwapTexture[which][texIndex]);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);

#if 0
    S32 srcX0 = 0;
    S32 srcY0 = 0;
    S32 srcX1 = getViewportWidth();
    S32 srcY1 = getViewportHeight();

    S32 dstX0 = (which * 512);
    S32 dstY0 = 0;
    S32 dstX1 = dstX0 + 512;
    S32 dstY1 = 512;

    LLRenderTarget::copyContentsToFramebuffer(
        *(mEyeRT[which][texIndex]),
        srcX0, srcY0, srcX1, srcY1,
        dstX0, dstY0, dstX1, dstY1,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
#endif

    return true;
}

BOOL LLHMDImplOculus::releaseAllEyeRT()
{
    destroySwapChains();
    return true;
}

BOOL LLHMDImplOculus::endFrame()
{
    if (!mEyeRT[0][0])
    {
        if (!initSwapChains())
        {
            return FALSE;
        }
    }

    ovrLayerEyeFov eyeLayer;

    eyeLayer.Header.Type  = ovrLayerType_EyeFov;
    eyeLayer.Header.Flags = ovrLayerFlag_HighQuality | ovrLayerFlag_TextureOriginAtBottomLeft; // Because OpenGL and HighQuality because HIGH KWALITY!

    int viewportSizeX = getViewportWidth();
    int viewportSizeY = getViewportHeight();

    (void)viewportSizeX, (void)viewportSizeY;

    ovrRecti viewport;

    viewport.Pos.x = 0;
    viewport.Pos.y = 0;

    viewport.Size = mOculus->mViewport;

    eyeLayer.Viewport[0] = viewport;
    eyeLayer.Viewport[1] = viewport;

    eyeLayer.ColorTexture[0] = mOculus->mSwapChain[0];
    eyeLayer.ColorTexture[1] = mOculus->mSwapChain[1];

    eyeLayer.RenderPose[0] = mOculus->mEyeRenderPose[0];
    eyeLayer.RenderPose[1] = mOculus->mEyeRenderPose[1];
    eyeLayer.Fov[0] = mOculus->mEyeRenderDesc[0].Fov;
    eyeLayer.Fov[1] = mOculus->mEyeRenderDesc[1].Fov;

    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

    viewScaleDesc.HmdToEyeOffset[0] = mOculus->mEyeRenderDesc[0].HmdToEyeOffset;
    viewScaleDesc.HmdToEyeOffset[1] = mOculus->mEyeRenderDesc[1].HmdToEyeOffset;

    ovrLayerHeader* layers[1] = { &eyeLayer.Header };

    ovr_CommitTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[0]);
    ovr_CommitTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[1]);

    ovrResult result = ovr_SubmitFrame(mOculus->mHMD, getSubmittedFrameIndex(), &viewScaleDesc, layers, 1);
    incrementSubmittedFrameIndex();

    if (!OVR_SUCCESS(result))
    {
        // too late, you're already blind!
        if (result == ovrError_DisplayLost)
        {
            gHMD.isHMDConnected(false);
            return FALSE;
        }
    }

    // Blit mirror texture to back buffer
    if (mMirrorRT)
    {
        S32 w = gViewerWindow->getWindowWidthRaw();
        S32 h = gViewerWindow->getWindowHeightRaw();
        LLRenderTarget::copyContentsToFramebuffer(*mMirrorRT, 0, h, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // Round-robin our three ring circus.
    mOculus->mCurrentSwapChainIndex[0] = mOculus->mCurrentSwapChainIndex[0] < 2 ? (mOculus->mCurrentSwapChainIndex[0] + 1) : 0;
    mOculus->mCurrentSwapChainIndex[1] = mOculus->mCurrentSwapChainIndex[1] < 2 ? (mOculus->mCurrentSwapChainIndex[1] + 1) : 0;
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

void LLHMDImplOculus::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const
{
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
}

void LLHMDImplOculus::getEyeProjection(int whichEye, glh::matrix4f& projOut, float zNear, float zFar) const
{
    ovrMatrix4f proj = ovrMatrix4f_Projection(mOculus->mHMDDesc.DefaultEyeFov[whichEye], zNear, zFar, ovrProjection_ClipRangeOpenGL);
    projOut = glh::matrix4f(&proj.M[0][0]);
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

LLVector3 LLHMDImplOculus::getHeadPosition() const
{
    return mHeadPos;
}

F32 LLHMDImplOculus::getRoll() const
{
    float roll;
    float pitch;
    float yaw;
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
    return roll;
}

F32 LLHMDImplOculus::getPitch() const
{
    float roll;
    float pitch;
    float yaw;
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
    return pitch;
}

F32 LLHMDImplOculus::getYaw() const
{
    float roll;
    float pitch;
    float yaw;
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
    return yaw;

}

#endif // LL_HMD_SUPPORTED

