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


#define OCULUS_12 1
#define OCULUS_08 !OCULUS_12

#include "OVR_CAPI_GL.h"


struct LLHMDImplOculus::OculusData
{
    LLHMDImplOculus::OculusData()
    {
        mCurrentSwapChainIndex[0] = 0;
        mCurrentSwapChainIndex[1] = 0;
        mMirrorTexture = 0;
    }

#if OCULUS_12
    typedef ovrSession          Headset;
    typedef ovrTextureSwapChain SwapChain;
    typedef ovrMirrorTexture    MirrorTexture;
#else
    typedef ovrHmd              Headset;
    typedef ovrSwapTextureSet*  SwapChain;
    typedef ovrTexture*         MirrorTexture;
#endif

    // Oculus-Specific data structures
    ovrResult m_initResult;
    Headset mHMD;
    ovrHmdDesc mHMDDesc;
    ovrGraphicsLuid mLUID;
    ovrSizei mViewport;
    ovrPosef mEyeRenderPose[ovrEye_Count];
    ovrMatrix4f mProjection[ovrEye_Count];
    ovrEyeRenderDesc mEyeRenderDesc[ovrEye_Count];
    SwapChain mSwapChain[ovrEye_Count];
    int mCurrentSwapChainIndex[ovrEye_Count];
    MirrorTexture mMirrorTexture;
    GLuint mMirrorFbo = 0;
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
{
    mOculus = new LLHMDImplOculus::OculusData;
    mEyeRotation.setEulerAngles(0.0f, 10.0f, 30.0f); // "haters gonna hate" sane defaults (better than NaNs)
    mHeadPos.set(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < ovrEye_Count; ++i)
    {
        for (int t = 0; t < 3; ++t)
        {
            mEyeRT[i][t] = nullptr;
        }
    }
}

LLHMDImplOculus::~LLHMDImplOculus()
{
    shutdown();

    delete mOculus;
}


// Oculus method for probing headset presence without DLL dep.
// Not something to be called per frame as it's kinda 'spensive.
bool HasHeadMountedDisplay()
{    
    #if OCULUS_12
        const int cTimeoutMs = 512;
        ovrDetectResult result = ovr_Detect(cTimeoutMs);
        if (result.IsOculusServiceRunning)
        {
            return result.IsOculusHMDConnected;
        }
    #endif

    return false;
}

BOOL LLHMDImplOculus::init()
{
    ovrInitParams params;

    params.Flags = 0;

    //params.Flags |= ovrInit_Debug;

    params.RequestedMinorVersion = OVR_MINOR_VERSION;
    params.LogCallback = NULL;
    params.ConnectionTimeoutMS = 0;

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

    #if OCULUS_12
        mTrackingCaps = ovr_GetTrackingState(mOculus->mHMD, 0.0, ovrTrue).StatusFlags;
    #else
        mTrackingCaps = ovr_GetTrackingState(mOculus->mHMD, 0.0).StatusFlags;
    #endif

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
        if (mEyeRT[i][0])
        {
            // Release Oculus' swap textures from our RTs before we destroy them (twice!).
            for (int t = 0; t < 3; ++t)
            {
                mEyeRT[i][t]->forceTarget(0, 0, 0, GL_RGBA);
                delete mEyeRT[i][t];
                mEyeRT[i][t] = nullptr;
            }

            #if OCULUS_12
                ovr_DestroyTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[i]);
            #else
                ovr_DestroySwapTextureSet(mOculus->mHMD, mOculus->mSwapChain[i]);
                mOculus->mSwapChain[i] = nullptr;
            #endif
        }
    }

    if (mOculus->mMirrorFbo)
    {
        glDeleteFramebuffers(1, &mOculus->mMirrorFbo);        
    }
    mOculus->mMirrorFbo = 0;

    if (mOculus->mMirrorTexture)
    {
        ovr_DestroyMirrorTexture(mOculus->mHMD, mOculus->mMirrorTexture);
    }
    mOculus->mMirrorTexture = 0;
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

BOOL LLHMDImplOculus::initSwapChain(int eyeIndex)
{

#if OCULUS_12
    ovrTextureSwapChainDesc swapChainDesc;
    swapChainDesc.Type      = ovrTexture_2D;
    swapChainDesc.ArraySize = 3;
    swapChainDesc.Width     = mOculus->mViewport.w;
    swapChainDesc.Height    = mOculus->mViewport.h;
    swapChainDesc.MipLevels = 1;
    swapChainDesc.Format    = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    swapChainDesc.SampleCount = 1;
    swapChainDesc.StaticImage = ovrFalse;

    ovrResult result = ovr_CreateTextureSwapChainGL(mOculus->mHMD, &swapChainDesc, &mOculus->mSwapChain[eyeIndex]);
#else
    ovrResult result = ovr_CreateSwapTextureSetGL(mOculus->mHMD, GL_SRGB8_ALPHA8, mOculus->mViewport.w, mOculus->mViewport.h, &mOculus->mSwapChain[eyeIndex]);
#endif

    if (!OVR_SUCCESS(result))
    {
        return FALSE;
    }

    int length = 0;

#if OCULUS_12
    result = ovr_GetTextureSwapChainLength(mOculus->mHMD, mOculus->mSwapChain[eyeIndex], &length);
    if (!OVR_SUCCESS(result))
    {
        return FALSE;
    }
#else
    length = mOculus->mSwapChain[eyeIndex]->TextureCount;
#endif

    llassert(length <= 3);

    for (int texIndex = 0; texIndex < length; ++texIndex)
    {
        GLuint swapChainTextureId;

    #if OCULUS_12
        result = ovr_GetTextureSwapChainBufferGL(mOculus->mHMD, mOculus->mSwapChain[eyeIndex], texIndex, &swapChainTextureId);
        if (!OVR_SUCCESS(result))
        {
            return FALSE;
        }
    #else
        ovrGLTexture* tex = (ovrGLTexture*)&mOculus->mSwapChain[eyeIndex]->Textures[texIndex];
        swapChainTextureId = tex->OGL.TexId;
    #endif

        mEyeRT[eyeIndex][texIndex] = new LLRenderTarget();
        mEyeRT[eyeIndex][texIndex]->forceTarget(mOculus->mViewport.w, mOculus->mViewport.h, swapChainTextureId, GL_SRGB8_ALPHA8, LLTexUnit::TT_TEXTURE);
    }

    if (!mOculus->mMirrorTexture)
    {

// TODO THESE SHOULD BE USING THE DIMS OF THE MAIN WINDOW (WHICH WILL GET THIS MIRROR FBO BOUNCED TO IT)

    #if OCULUS_12
        ovrMirrorTextureDesc mirrorTextureDesc;

        memset(&mirrorTextureDesc, 0, sizeof(mirrorTextureDesc));
        mirrorTextureDesc.Width  = gViewerWindow->getWindowWidthRaw();
        mirrorTextureDesc.Height = gViewerWindow->getWindowHeightRaw();
        mirrorTextureDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

        // Create mirror texture and an FBO used to copy mirror texture to back buffer
        result = ovr_CreateMirrorTextureGL(mOculus->mHMD, &mirrorTextureDesc, &mOculus->mMirrorTexture);
    #else
        result = ovr_CreateMirrorTextureGL(mOculus->mHMD, GL_SRGB8_ALPHA8, mOculus->mViewport.w * 2, mOculus->mViewport.h, reinterpret_cast<ovrTexture**>(&mOculus->mMirrorTexture));
    #endif

        if (!OVR_SUCCESS(result))
        {
            return FALSE;
        }

        glGenFramebuffers(1, &mOculus->mMirrorFbo);

        // Configure the mirror read buffer
        GLuint mirrorTextureId;

    #if OCULUS_12
        result = ovr_GetMirrorTextureBufferGL(mOculus->mHMD, mOculus->mMirrorTexture, &mirrorTextureId);
        if (!OVR_SUCCESS(result))
        {
            return FALSE;
        }    
    #else
        mirrorTextureId = reinterpret_cast<ovrGLTexture*>(mOculus->mMirrorTexture)->OGL.TexId;
    #endif

        glBindTexture(GL_TEXTURE_2D, mirrorTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, mOculus->mMirrorFbo);

        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
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

    #if OCULUS_12
        mOculus->mEyeRenderDesc[ovrEye_Left ].HmdToEyeOffset,
        mOculus->mEyeRenderDesc[ovrEye_Right].HmdToEyeOffset
    #else
        mOculus->mEyeRenderDesc[ovrEye_Left ].HmdToEyeViewOffset,
        mOculus->mEyeRenderDesc[ovrEye_Right].HmdToEyeViewOffset
    #endif

    };

    #if OCULUS_12
        const double predictedDisplayTime = ovr_GetPredictedDisplayTime(mOculus->mHMD, getFrameIndex());
        const ovrTrackingState  tracking_state  = ovr_GetTrackingState(mOculus->mHMD, predictedDisplayTime, ovrTrue);
    #else
        ovrFrameTiming          frame_timing    = ovr_GetFrameTiming(mOculus->mHMD, 0);
        const ovrTrackingState  tracking_state  = ovr_GetTrackingState(mOculus->mHMD, frame_timing.DisplayMidpointSeconds);
    #endif

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

    return true;
}

BOOL LLHMDImplOculus::releaseEyeRT(int which)
{
    if (!mEyeRT[0][0])
    {
        return false;
    }

    int texIndex = mOculus->mCurrentSwapChainIndex[which];

    mEyeRT[which][texIndex]->flush();

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

    #if OCULUS_12
        viewScaleDesc.HmdToEyeOffset[0] = mOculus->mEyeRenderDesc[0].HmdToEyeOffset;
        viewScaleDesc.HmdToEyeOffset[1] = mOculus->mEyeRenderDesc[1].HmdToEyeOffset;
    #else
        viewScaleDesc.HmdToEyeViewOffset[0] = mOculus->mEyeRenderDesc[0].HmdToEyeViewOffset;
        viewScaleDesc.HmdToEyeViewOffset[1] = mOculus->mEyeRenderDesc[1].HmdToEyeViewOffset;
    #endif

    ovrLayerHeader* layers[1] = { &eyeLayer.Header };

    #if OCULUS_12
        ovr_CommitTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[0]);
        ovr_CommitTextureSwapChain(mOculus->mHMD, mOculus->mSwapChain[1]);
    #endif

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
    if (mOculus->mMirrorFbo)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mOculus->mMirrorFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, viewportSizeY, viewportSizeX, 0, 0, 0, gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
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
    #if OCULUS_12
        ovr_RecenterTrackingOrigin(mOculus->mHMD);
    #else
        ovr_RecenterPose(mOculus->mHMD);
    #endif

    resetFrameIndex();
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
