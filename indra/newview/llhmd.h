/** 
* @file   llhmd.h
* @brief  Header file for llhmd
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
#ifndef LL_LLHMD_H
#define LL_LLHMD_H

class LLHMDImpl;

// TODO: move some of the data to this class instead of always requiring an extra method call via PIMPL
class LLHMD
{
public:
    enum eRenderMode
    {
        RenderMode_None = 0,            // Do not render to HMD
        RenderMode_HMD,                 // render to HMD
        RenderMode_ScreenStereo,        // render to screen in stereoscopic mode (no distortion).  Useful for debugging.
        RenderMode_ScreenStereoDistort, // render to screen in stereoscopic mode (with distortion).  Useful for debugging.
        RenderMode_Last = RenderMode_ScreenStereoDistort,
    };

    enum
    {
        kHMDHeight = 800,
        kHMDWidth = 1280,
        kHMDEyeWidth = 640,
        kHMDUIWidth = kHMDWidth,
        kHMDUIHeight = kHMDHeight,
    };

    enum eFlags
    {
        kFlag_None                      = 0,
        kFlag_Initialized               = 1 << 0,
        kFlag_FailedInit                = 1 << 1,
        kFlag_MainIsFullScreen          = 1 << 2,
        kFlag_Render2DUI                = 1 << 3,
        kFlag_IsCalibrated              = 1 << 4,
    };

public:
    LLHMD();
    ~LLHMD();

    BOOL init();
    void shutdown();
    void onIdle();

    BOOL isInitialized() const { return ((mFlags & kFlag_Initialized) != 0) ? TRUE : FALSE; }
    void isInitialized(BOOL b) { if (b) { mFlags |= kFlag_Initialized; } else { mFlags &= ~kFlag_Initialized; } }
    BOOL failedInit() const { return ((mFlags & kFlag_FailedInit) != 0) ? TRUE : FALSE; }
    void failedInit(BOOL b) { if (b) { mFlags |= kFlag_FailedInit; } else { mFlags &= ~kFlag_FailedInit; } }
    BOOL isMainFullScreen() const { return ((mFlags & kFlag_MainIsFullScreen) != 0) ? TRUE : FALSE; }
    void isMainFullScreen(BOOL b) { if (b) { mFlags |= kFlag_MainIsFullScreen; } else { mFlags &= ~kFlag_MainIsFullScreen; } }
    BOOL shouldRender2DUI() const { return ((mFlags & kFlag_Render2DUI) != 0) ? TRUE : FALSE; }
    void shouldRender2DUI(BOOL b) { if (b) { mFlags |= kFlag_Render2DUI; } else { mFlags &= ~kFlag_Render2DUI; } }
    BOOL isCalibrated() const { return ((mFlags & kFlag_IsCalibrated) != 0) ? TRUE : FALSE; }
    void isCalibrated(BOOL b) { if (b) { mFlags |= kFlag_IsCalibrated; } else { mFlags &= ~kFlag_IsCalibrated; } }

    // True if the HMD is initialized and currently in a render mode != RenderMode_None
    BOOL shouldRender() const { return mRenderMode != RenderMode_None; }

    // get/set current HMD rendering mode
    U32 getRenderMode() const { return mRenderMode; }
    void setRenderMode(U32 mode);
    void setRenderWindowMain();
    void setRenderWindowHMD();
    void setFocusWindowMain();
    void setFocusWindowHMD();

    // 0 = center, 1 = left, 2 = right.  Input clamped to [0,2]
    U32 getCurrentEye() const;
    void setCurrentEye(U32 eye);

    // size and Lower-Left corner for the viewer of the current eye
    void getViewportInfo(S32& x, S32& y, S32& w, S32& h);

    F32 getHScreenSize() const;
    F32 getVScreenSize() const;
    F32 getInterpupillaryOffset() const;
    F32 getLensSeparationDistance() const;
    F32 getEyeToScreenDistance() const;

    // coefficients for the distortion function.
    LLVector4 getDistortionConstants() const;

    // CenterOffsets are the offsets of lens distortion center from the center of one-eye screen half. [-1, 1] Range.
    F32 getXCenterOffset() const;
    F32 getYCenterOffset() const;

    // Scale is a factor of how much larger will the input image be, with a factor of 1.0f being no scaling.
    // An inverse of this value is applied to sampled UV coordinates (1/Scale).
    F32 getDistortionScale() const;

    // Get the current HMD orientation
    LLQuaternion getHMDOrient() const;
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const;
    const LLVector3& getRawHMDRollPitchYaw() const;

    F32 getVerticalFOV() const;

    void setBaseModelView(F32* m) { for (int i = 0; i < 16; ++i) { mBaseModelView[i] = m[i]; } }
    F32* getBaseModelView() { return mBaseModelView; }
    void setBaseProjection(F32* m) { for (int i = 0; i < 16; ++i) { mBaseProjection[i] = m[i]; } }
    F32* getBaseProjection() { return mBaseProjection; }
    
    //// array of parameters for controlling additional Red and Blue scaling in order to reduce chromatic aberration
    //// caused by the Rift lenses.  Additional per-channel scaling is applied after distortion:
    //// Index [0] - Red channel constant coefficient.
    //// Index [1] - Red channel r^2 coefficient.
    //// Index [2] - Blue channel constant coefficient.
    //// Index [3] - Blue channel r^2 coefficient.
    //LLVector4 getChromaticAberrationConstants() const;

    //// Translation to be applied to view matrix.
    //LLMatrix4 getViewAdjustMatrix() const;
    //// Projection matrix used with the current eye.
    //LLMatrix4 getProjectionMatrix() const;
    //// Orthographic projection used with this eye.
    //LLMatrix4 getOrthoProjectionMatrix() const;

    F32 getOrthoPixelOffset() const;

    S32 getMainWindowWidth() const { return mMainWindowWidth; }
    void setMainWindowWidth(S32 w) { mMainWindowWidth = w; }
    S32 getMainWindowHeight() const { return mMainWindowHeight; }
    void setMainWindowHeight(S32 h) { mMainWindowHeight = h; }
    void getMainWindowSize(S32& w, S32& h) { w = mMainWindowWidth; h = mMainWindowHeight; }
    void setMainWindowSize(S32 w, S32 h) { mMainWindowWidth = w; mMainWindowHeight = h; }

    S32 getOptWindowRaw() const { return mOptWindowRaw; }
    S32 getOptWindowScaled() const { return mOptWindowScaled; }
    S32 getOptWorldViewRaw() const { return mOptWorldViewRaw; }
    S32 getOptWorldViewScaled() const { return mOptWorldViewScaled; }
    F32 getUISurfaceFudge() const { return mUISurface_Fudge; }
    void getUISurfaceX(F32& start, F32& end) const { start = mUISurface_B[0]; end = mUISurface_B[1]; }
    void getUISurfaceY(F32& start, F32& end) const { start = mUISurface_A[0]; end = mUISurface_A[1]; }
    F32 getUISurfaceRadius() const { return mUISurface_R; }

    static void onChangeInterpupillaryOffsetModifer();
    static void onChangeLensSeparationDistanceModifier();
    static void onChangeEyeToScreenDistanceModifier();
    static void onChangeXCenterOffsetModifier();
    static void onChangeShouldChangeFOV();
    static void onChangeWindowRaw();
    static void onChangeWindowScaled();
    static void onChangeWorldViewRaw();
    static void onChangeWorldViewScaled();
    static void onChangeVerticalFOVModifier();
    static void onChangeUISurfaceShape();

private:
    LLHMDImpl* mImpl;
    F32 mInterpupillaryMod;
    F32 mLensSepMod;
    F32 mEyeToScreenMod;
    F32 mXCenterOffsetMod;
    F32 mVerticalFOVMod;
    U32 mFlags;
    U32 mRenderMode;
    F32 mBaseModelView[16];
    F32 mBaseProjection[16];
    S32 mOptWindowRaw;
    S32 mOptWindowScaled;
    S32 mOptWorldViewRaw;
    S32 mOptWorldViewScaled;
    S32 mMainWindowWidth;
    S32 mMainWindowHeight;
    F32 mUISurface_Fudge;
    F32 mUISurface_R;
    LLVector3 mUISurface_A;
    LLVector3 mUISurface_B;
 };

extern LLHMD gHMD;

#endif // LL_LLHMD_H

