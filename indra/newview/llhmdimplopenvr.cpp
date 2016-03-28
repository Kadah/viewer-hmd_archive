/** 
* @file llhmdimplopenvr.cpp
* @brief Implementation of llhmd for OpenVR API devices
* @author graham@lindenlab.com
*
* $LicenseInfo:firstyear=2016&license=viewerlgpl$
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

#include "llhmdimplopenvr.h"
#include "llhmd.h"

#if LL_HMD_OPENVR_SUPPORTED && LL_HMD_SUPPORTED

#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llview.h"
#include "llviewerdisplay.h"
#include "pipeline.h"
#include "llrendertarget.h"
#include "llnotificationsutil.h"
#include "lltimer.h"

#include "openvr.h"

#if LL_WINDOWS
#pragma comment(lib,"libopenvr")
#endif

struct LLHMDImplOpenVR::OpenVRData
{
    vr::IVRSystem*          mHMD = nullptr;
    vr::TrackedDevicePose_t mPose;
};


///////////////////////////////////////////////////////////////////////////
// OpenVr -> LL conversion funcs
///////////////////////////////////////////////////////////////////////////

LLMatrix4 ToMatrix4(const vr::HmdMatrix44_t& matIn)
{
    LLMatrix4 matOut;
    matOut.initRows(
            LLVector4(matIn.m[0][0], matIn.m[0][1], matIn.m[0][2], matIn.m[0][3]),
            LLVector4(matIn.m[1][0], matIn.m[1][1], matIn.m[1][2], matIn.m[1][3]),
            LLVector4(matIn.m[2][0], matIn.m[2][1], matIn.m[2][2], matIn.m[2][3]),
            LLVector4(matIn.m[3][0], matIn.m[3][1], matIn.m[3][2], matIn.m[3][3]));
    return matOut;
}

LLMatrix4 ToMatrix4(const vr::HmdMatrix34_t& matIn)
{
    LLMatrix4 matOut;
    matOut.initRows(
            LLVector4(matIn.m[0][0], matIn.m[0][1], matIn.m[0][2], matIn.m[0][3]),
            LLVector4(matIn.m[1][0], matIn.m[1][1], matIn.m[1][2], matIn.m[1][3]),
            LLVector4(matIn.m[2][0], matIn.m[2][1], matIn.m[2][2], matIn.m[2][3]),
            LLVector4(0.0f, 0.0f, 0.0f, 1.0f));
    return matOut;
}

void FromSteamVRProjection(const vr::HmdMatrix44_t& matIn, glh::matrix4f& projOut)
{
    projOut = glh::matrix4f((float*)&matIn.m[0][0]);
    projOut = projOut.inverse();
}

void FromSteamVRTransform(const vr::HmdMatrix34_t& matIn, LLQuaternion& eyeRotationOut, LLVector3& headPositionOut)
{
    LLMatrix4 mat = ToMatrix4(matIn);
    eyeRotationOut.set(mat.getMat3());
    headPositionOut.set(mat.getTranslation());
}

LLHMDImplOpenVR::LLHMDImplOpenVR()
: mOpenVR(NULL)
, mFrameIndex(0)
, mSubmittedFrameIndex(0)
    , mTrackingCaps(0)
, mEyeToScreenDistance(kDefaultEyeToScreenDistance)
, mInterpupillaryDistance(kDefaultInterpupillaryOffset)
, mVerticalFovRadians(kDefaultVerticalFOVRadians)
, mAspect(kDefaultAspect)
{
    mOpenVR = new LLHMDImplOpenVR::OpenVRData;
    mEyeRotation.setEulerAngles(0.0, 0.0f, 0.0f);
    mHeadPosition.set(LLVector4::zero);
    for (int i = 0; i < 2; ++i)
    {
        mEyeRT[i] = nullptr;
        mEyeViewOffset[i].setZero();
        mProjection[i].setIdentity();
    }
}

LLHMDImplOpenVR::~LLHMDImplOpenVR()
{
    shutdown();
    delete mOpenVR;
}

// OpenVR method for probing headset presence without DLL dep.
// Not something to be called per frame as it's kinda 'spensive.
BOOL LLHMDImplOpenVR::HasHeadMountedDisplay()
{    
    return vr::VR_IsHmdPresent();
}

BOOL LLHMDImplOpenVR::init()
{
    vr::EVRInitError error = vr::VRInitError_None;
    mOpenVR->mHMD = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None)
    {
        gHMD.isFailedInit(TRUE);

        delete mOpenVR;
        mOpenVR = nullptr;
    }

    if (!vr::VRCompositor())
    {
        gHMD.isFailedInit(TRUE);
        delete mOpenVR;
        mOpenVR = nullptr;
    }

    calculateViewportSettings();

    gHMD.isHMDConnected(LLHMDImplOpenVR::HasHeadMountedDisplay() && (mOpenVR != NULL));

    return gHMD.isHMDConnected();
}

void LLHMDImplOpenVR::destroy()
{
    gHMD.isHMDConnected(false);
    gHMD.setRenderMode(LLHMD::RenderMode_None);

    destroyRenderTargets();

    delete mOpenVR;
    mOpenVR = NULL;

    vr::VR_Shutdown();    
}

void LLHMDImplOpenVR::shutdown()
{
    if (!gHMD.isInitialized())
    {
        return;
    }

    destroy();    
}

void LLHMDImplOpenVR::createRenderTargets()
{
    S32 w = getViewportWidth();
    S32 h = getViewportHeight();

    mEyeRT[0] = new LLRenderTarget();
    mEyeRT[0]->allocate(w, h, GL_SRGB8_ALPHA8, true, true, LLTexUnit::TT_TEXTURE, TRUE);
    mEyeRT[1] = new LLRenderTarget();
    mEyeRT[1]->allocate(w, h, GL_SRGB8_ALPHA8, true, true, LLTexUnit::TT_TEXTURE, TRUE);
}

void LLHMDImplOpenVR::destroyRenderTargets()
{
    delete mEyeRT[0];
    mEyeRT[0] = NULL;
    delete mEyeRT[1];
    mEyeRT[1] = NULL;
}

S32 LLHMDImplOpenVR::getViewportWidth() const
{
    U32 resX = 0;
    U32 resY = 0;
    mOpenVR->mHMD->GetRecommendedRenderTargetSize(&resX, &resY);
    return (S32)resX;
}

S32 LLHMDImplOpenVR::getViewportHeight() const
{
    U32 resX = 0;
    U32 resY = 0;
    mOpenVR->mHMD->GetRecommendedRenderTargetSize(&resX, &resY);
    return (S32)resY;
}

void LLHMDImplOpenVR::resetFrameIndex()
{
    mFrameIndex = 0;
    mSubmittedFrameIndex = 0;
}

U32 LLHMDImplOpenVR::getFrameIndex()
{
    return mFrameIndex;
}

void LLHMDImplOpenVR::incrementFrameIndex()
{
    ++mFrameIndex;
}

U32 LLHMDImplOpenVR::getSubmittedFrameIndex()
{
    return mSubmittedFrameIndex;
}

void LLHMDImplOpenVR::incrementSubmittedFrameIndex()
{
    ++mSubmittedFrameIndex;
}

void LLHMDImplOpenVR::resetOrientation()
{
    mOpenVR->mHMD->ResetSeatedZeroPose();
    resetFrameIndex();
}

BOOL LLHMDImplOpenVR::calculateViewportSettings()
{
    return true;
}

BOOL LLHMDImplOpenVR::beginFrame()
{
    if (mOpenVR)
    {
        vr::VREvent_t event;
        while (mOpenVR->mHMD->PollNextEvent(&event, sizeof(event)))
        {
            switch (event.eventType)
            {
            case vr::VREvent_TrackedDeviceActivated:
            {
                onDeviceActivated(event.trackedDeviceIndex);
            }
            break;

            case vr::VREvent_TrackedDeviceDeactivated:
            {
                onDeviceDeactivated(event.trackedDeviceIndex);
            }
            break;

            case vr::VREvent_TrackedDeviceUpdated:
            {
                onDeviceUpdated(event.trackedDeviceIndex);
            }
            break;
            }
        }
    }

    if (!mEyeRT[0])
    {
        createRenderTargets();
    }

    LLMatrix4 transform(mEyeRotation, LLVector4(mHeadPosition, 1.0f));

    vr::VRCompositor()->WaitGetPoses(&mOpenVR->mPose, 1, NULL, 0);

    // TODO Make this loop through all devices and find the HMD one...
    vr::ETrackedDeviceClass devClass = mOpenVR->mHMD->GetTrackedDeviceClass(0);

    if (mOpenVR->mPose.bPoseIsValid && (devClass == vr::TrackedDeviceClass_HMD))
    {
        FromSteamVRTransform(mOpenVR->mPose.mDeviceToAbsoluteTracking, mEyeRotation, mHeadPosition);
    }

    // Get projection mats for each eye for current near/far settings
    mProjection[0] = ToMatrix4(mOpenVR->mHMD->GetProjectionMatrix(vr::Eye_Left,  0.25f, 4096.0f, vr::API_OpenGL));
    mProjection[1] = ToMatrix4(mOpenVR->mHMD->GetProjectionMatrix(vr::Eye_Right, 0.25f, 4096.0f, vr::API_OpenGL));
    mProjection[0].invert(); // clipspace to left eyespace
    mProjection[1].invert(); // clipspace to right eyespace

    return TRUE;
}

BOOL LLHMDImplOpenVR::bindEyeRT(int eyeIndex)
{
    if (mEyeRT[eyeIndex])
    {
        mEyeRT[eyeIndex]->bindTarget();
        return TRUE;
    }
    return FALSE;
}

BOOL LLHMDImplOpenVR::releaseEyeRT(int eyeIndex)
{
    if (mEyeRT[eyeIndex])
    {
        // Bind default FBO, unbinding our eye FBO implicitly
        //
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Bind the RTs texture so we can let the compositor
        // copy that subimage data to the Vive
        mEyeRT[eyeIndex]->bindTexture(0, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);

        GLint texId = mEyeRT[eyeIndex]->getTexture();

        vr::Texture_t eyeTexture;

        eyeTexture.handle = (void*)uintptr_t(texId);
        eyeTexture.eType = vr::API_OpenGL;
        eyeTexture.eColorSpace = vr::ColorSpace_Gamma;

        vr::VRCompositor()->Submit(vr::EVREye(eyeIndex), &eyeTexture);
    }

    return TRUE;
}

BOOL LLHMDImplOpenVR::endFrame()
{
    incrementSubmittedFrameIndex();
    return TRUE;
}

BOOL LLHMDImplOpenVR::postSwap()
{
    vr::VRCompositor()->PostPresentHandoff();
    return TRUE;
}

BOOL LLHMDImplOpenVR::releaseAllEyeRT()
{
    destroyRenderTargets();
    return true;
}

void LLHMDImplOpenVR::onDeviceActivated(int trackedDeviceIndex)
{
    (void)trackedDeviceIndex;
}

void LLHMDImplOpenVR::onDeviceDeactivated(int trackedDeviceIndex)
{
    (void)trackedDeviceIndex;
}

void LLHMDImplOpenVR::onDeviceUpdated(int trackedDeviceIndex)
{
    (void)trackedDeviceIndex;
}

void LLHMDImplOpenVR::getEyeProjection(int whichEye, glh::matrix4f& proj, float zNear, float zFar) const
{
    FromSteamVRProjection(mOpenVR->mHMD->GetProjectionMatrix(vr::EVREye(whichEye), zNear, zFar, vr::API_OpenGL), proj);
}

void LLHMDImplOpenVR::getEyeOffset(int whichEye, LLVector3& offsetOut) const
{
    LLQuaternion q;
    LLVector3    t;
    FromSteamVRTransform(mOpenVR->mHMD->GetEyeToHeadTransform(vr::EVREye(whichEye)), q, t);
    offsetOut = t;
}

LLVector3 LLHMDImplOpenVR::getHeadPosition() const
{
    return mHeadPosition;
}

F32 LLHMDImplOpenVR::getRoll() const
{
    float roll;
    float pitch;
    float yaw;
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
    return roll;
}

F32 LLHMDImplOpenVR::getPitch() const
{
    float roll;
    float pitch;
    float yaw;
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
    return pitch;
}

F32 LLHMDImplOpenVR::getYaw() const
{
    float roll;
    float pitch;
    float yaw;
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
    return yaw;

}

void LLHMDImplOpenVR::getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const
{
    mEyeRotation.getEulerAngles(&roll, &pitch, &yaw);
}

#endif // LL_HMD_OPENVR_SUPPORTED && LL_HMD_SUPPORTED
