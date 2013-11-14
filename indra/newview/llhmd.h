/** 
* @file   llhmd.h
* @brief  Header file for llhmd
* @author voidpointer@lindenlab.com
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

#if LL_WINDOWS || LL_DARWIN
    #define LL_HMD_SUPPORTED 1
#else
    // We do not support the Oculus Rift on other platforms at the moment
    #define LL_HMD_SUPPORTED 0
#endif

class LLHMDImpl;
class LLViewerTexture;
class LLVertexBuffer;


// TODO: move some of the data to this class instead of always requiring an extra method call via PIMPL
class LLHMD
{
public:
    enum eRenderMode
    {
        RenderMode_None = 0,        // Do not render to HMD
        RenderMode_HMD,             // render to HMD
        RenderMode_ScreenStereo,    // render to screen in stereoscopic mode (with distortion).  Useful for debugging.
        RenderMode_Last = RenderMode_ScreenStereo,
    };

    enum eFlags
    {
        kFlag_None                      = 0,
        kFlag_Initialized               = 1 << 0,
        kFlag_Pre_Initialized           = 1 << 1,
        kFlag_Post_Initialized          = 1 << 2,
        kFlag_FailedInit                = 1 << 3,
        kFlag_HMDConnected              = 1 << 4,
        kFlag_MainIsFullScreen          = 1 << 5,
        kFlag_IsCalibrated              = 1 << 6,
        kFlag_ShowDepthVisual           = 1 << 7,
        kFlag_ShowCalibrationUI         = 1 << 8,
        kFlag_CursorIntersectsWorld     = 1 << 9,
        kFlag_CursorIntersectsUI        = 1 << 10,
        kFlag_FullWidthUIMode           = 1 << 11,
        kFlag_ChangingRenderContext     = 1 << 12,
    };

public:
    LLHMD();
    ~LLHMD();

    BOOL init();
    void shutdown();
    void onIdle();

    BOOL isInitialized() const { return ((mFlags & kFlag_Initialized) != 0) ? TRUE : FALSE; }
    void isInitialized(BOOL b) { if (b) { mFlags |= kFlag_Initialized; } else { mFlags &= ~kFlag_Initialized; } }
    BOOL isPreDetectionInitialized() const { return ((mFlags & kFlag_Pre_Initialized) != 0) ? TRUE : FALSE; }
    void isPreDetectionInitialized(BOOL b) { if (b) { mFlags |= kFlag_Pre_Initialized; } else { mFlags &= ~kFlag_Pre_Initialized; } }
    BOOL isPostDetectionInitialized() const { return ((mFlags & kFlag_Post_Initialized) != 0) ? TRUE : FALSE; }
    void isPostDetectionInitialized(BOOL b) { if (b) { mFlags |= kFlag_Post_Initialized; } else { mFlags &= ~kFlag_Post_Initialized; } }
    BOOL failedInit() const { return ((mFlags & kFlag_FailedInit) != 0) ? TRUE : FALSE; }
    void failedInit(BOOL b) { if (b) { mFlags |= kFlag_FailedInit; } else { mFlags &= ~kFlag_FailedInit; } }
    BOOL isHMDConnected() const { return ((mFlags & kFlag_HMDConnected) != 0) ? TRUE : FALSE; }
    void isHMDConnected(BOOL b) { if (b) { mFlags |= kFlag_HMDConnected; } else { mFlags &= ~kFlag_HMDConnected; } }
    BOOL isMainFullScreen() const { return ((mFlags & kFlag_MainIsFullScreen) != 0) ? TRUE : FALSE; }
    void isMainFullScreen(BOOL b) { if (b) { mFlags |= kFlag_MainIsFullScreen; } else { mFlags &= ~kFlag_MainIsFullScreen; } }
    BOOL isCalibrated() const { return ((mFlags & kFlag_IsCalibrated) != 0) ? TRUE : FALSE; }
    void isCalibrated(BOOL b) { if (b) { mFlags |= kFlag_IsCalibrated; } else { mFlags &= ~kFlag_IsCalibrated; } }
    BOOL shouldShowDepthVisual() const { return ((mFlags & kFlag_ShowDepthVisual) != 0) ? TRUE : FALSE; }
    void shouldShowDepthVisual(BOOL b) { if (b) { mFlags |= kFlag_ShowDepthVisual; } else { mFlags &= ~kFlag_ShowDepthVisual; } }
    BOOL shouldShowCalibrationUI() const { return ((mFlags & kFlag_ShowCalibrationUI) != 0) ? TRUE : FALSE; }
    void shouldShowCalibrationUI(BOOL b) { if (b) { mFlags |= kFlag_ShowCalibrationUI; } else { mFlags &= ~kFlag_ShowCalibrationUI; } }
    BOOL cursorIntersectsWorld() const { return ((mFlags & kFlag_CursorIntersectsWorld) != 0) ? TRUE : FALSE; }
    void cursorIntersectsWorld(BOOL b) { if (b) { mFlags |= kFlag_CursorIntersectsWorld; } else { mFlags &= ~kFlag_CursorIntersectsWorld; } }
    BOOL cursorIntersectsUI() const { return ((mFlags & kFlag_CursorIntersectsUI) != 0) ? TRUE : FALSE; }
    void cursorIntersectsUI(BOOL b) { if (b) { mFlags |= kFlag_CursorIntersectsUI; } else { mFlags &= ~kFlag_CursorIntersectsUI; } }
    BOOL isFullWidthUIMode() const { return ((mFlags & kFlag_FullWidthUIMode) != 0) ? TRUE : FALSE; }
    void isFullWidthUIMode(BOOL b) { if (b) { mFlags |= kFlag_FullWidthUIMode; } else { mFlags &= ~kFlag_FullWidthUIMode; } }
    BOOL isChangingRenderContext() const { return ((mFlags & kFlag_ChangingRenderContext) != 0) ? TRUE : FALSE; }
    void isChangingRenderContext(BOOL b) { if (b) { mFlags |= kFlag_ChangingRenderContext; } else { mFlags &= ~kFlag_ChangingRenderContext; } }

    BOOL isManuallyCalibrating() const;
    void BeginManualCalibration();
    const std::string& getCalibrationText() const;

    // True if the HMD is initialized and currently in a render mode != RenderMode_None
    BOOL isHMDMode() const { return mRenderMode != RenderMode_None; }

    // get/set current HMD rendering mode
    U32 getRenderMode() const { return mRenderMode; }
    void setRenderMode(U32 mode, bool setFocusWindow = true);
    void setRenderWindowMain();
    void setRenderWindowHMD();
    void setFocusWindowMain();
    void setFocusWindowHMD();
    void onAppFocusGained();
    void onAppFocusLost();

    // 0 = center, 1 = left, 2 = right.  Input clamped to [0,2]
    U32 getCurrentEye() const;
    void setCurrentEye(U32 eye);

    // size and Lower-Left corner for the viewer of the current eye
    void getViewportInfo(S32& x, S32& y, S32& w, S32& h);

    S32 getHMDWidth() const;
    S32 getHMDEyeWidth() const;
    S32 getHMDHeight() const;
    S32 getHMDUIWidth() const;
    S32 getHMDUIHeight() const;

    F32 getPhysicalScreenWidth() const;
    F32 getPhysicalScreenHeight() const; 
    F32 getInterpupillaryOffset() const;
    F32 getInterpupillaryOffsetDefault() const;
    void setInterpupillaryOffset(F32 f);

    F32 getLensSeparationDistance() const;

    F32 getEyeToScreenDistance() const;
    void setEyeToScreenDistance(F32 f);
    F32 getEyeToScreenDistanceDefault() const;
    F32 getVerticalFOV() const;

    F32 getEyeDepth() const { return mEyeDepth; }
    F32 getUIEyeDepth();
    F32 getUIMagnification() { return mUIMagnification; }
    void setUIMagnification(F32 f) { mUIMagnification = f; calculateUIEyeDepth(); }

    // coefficients for the distortion function.
    LLVector4 getDistortionConstants() const;

    // CenterOffsets are the offsets of lens distortion center from the center of one-eye screen half. [-1, 1] Range.
    F32 getXCenterOffset() const;
    F32 getYCenterOffset() const;

    // Scale is a factor of how much larger will the input image be, with a factor of 1.0f being no scaling.
    // An inverse of this value is applied to sampled UV coordinates (1/Scale).
    F32 getDistortionScale() const;

    BOOL useMotionPrediction() const;
    BOOL useMotionPredictionDefault() const;
    void useMotionPrediction(BOOL b);
    F32 getMotionPredictionDelta() const;
    F32 getMotionPredictionDeltaDefault() const;
    void setMotionPredictionDelta(F32 f);

    // Get the current HMD orientation
    LLQuaternion getHMDOrient() const;
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const;

    // head correction (difference in rotation between head and body)
    LLQuaternion getHeadRotationCorrection() const;
    void addHeadRotationCorrection(LLQuaternion quat);

    void setBaseModelView(F32* m);
    F32* getBaseModelView() { return mBaseModelView; }
    F32* getBaseModelViewInv() { return mBaseModelViewInv; }
    void setBaseProjection(F32* m) { for (int i = 0; i < 16; ++i) { mBaseProjection[i] = m[i]; } }
    F32* getBaseProjection() { return mBaseProjection; }
    void setUIModelView(F32* m);
    F32* getUIModelView() { return mUIModelView; }
    F32* getUIModelViewInv() { return mUIModelViewInv; }
    void setBaseLookAt(F32* m) { for (int i = 0; i < 16; ++i) { mBaseLookAt[i] = m[i]; } }
    F32* getBaseLookAt() { return mBaseLookAt; }
    
    //// array of parameters for controlling additional Red and Blue scaling in order to reduce chromatic aberration
    //// caused by the Rift lenses.  Additional per-channel scaling is applied after distortion:
    //// Index [0] - Red channel constant coefficient.
    //// Index [1] - Red channel r^2 coefficient.
    //// Index [2] - Blue channel constant coefficient.
    //// Index [3] - Blue channel r^2 coefficient.
    //LLVector4 getChromaticAberrationConstants() const;

    F32 getOrthoPixelOffset() const;

    S32 getMainWindowWidth() const { return mMainWindowWidth; }
    void setMainWindowWidth(S32 w) { mMainWindowWidth = w; }
    S32 getMainWindowHeight() const { return mMainWindowHeight; }
    void setMainWindowHeight(S32 h) { mMainWindowHeight = h; }
    void getMainWindowSize(S32& w, S32& h) { w = mMainWindowWidth; h = mMainWindowHeight; }
    void setMainWindowSize(S32 w, S32 h) { mMainWindowWidth = w; mMainWindowHeight = h; }

    const LLVector2& getUISurfaceArc() const { return mUICurvedSurfaceArc; }
    F32 getUISurfaceArcHorizontal() const { return mUICurvedSurfaceArc[VX]; }
    void setUISurfaceArcHorizontal(F32 f) { mUICurvedSurfaceArc[VX] = f; onChangeUISurfaceShape(); }
    F32 getUISurfaceArcVertical() const { return mUICurvedSurfaceArc[VY]; }
    void setUISurfaceArcVertical(F32 f) { mUICurvedSurfaceArc[VY] = f; onChangeUISurfaceShape(); }
    const LLVector4& getUISurfaceRadius() const { return mUICurvedSurfaceRadius; }
    F32 getUISurfaceToroidRadiusWidth() const { return mUICurvedSurfaceRadius[0]; }
    void setUISurfaceToroidRadiusWidth(F32 f) { mUICurvedSurfaceRadius[0] =  f; onChangeUISurfaceShape(); }
    F32 getUISurfaceToroidRadiusDepth() const { return mUICurvedSurfaceRadius[1]; }
    void setUISurfaceToroidRadiusDepth(F32 f) { mUICurvedSurfaceRadius[1] =  f; onChangeUISurfaceShape(); }
    F32 getUISurfaceToroidCrossSectionRadiusWidth() const { return mUICurvedSurfaceRadius[3]; }
    void setUISurfaceToroidCrossSectionRadiusWidth(F32 f) { mUICurvedSurfaceRadius[3] =  f; onChangeUISurfaceShape(); }
    F32 getUISurfaceToroidCrossSectionRadiusHeight() const { return mUICurvedSurfaceRadius[2]; }
    void setUISurfaceToroidCrossSectionRadiusHeight(F32 f) { mUICurvedSurfaceRadius[2] =  f; onChangeUISurfaceShape(); }
    const LLVector3& getUISurfaceOffsets() const { return mUICurvedSurfaceOffsets; }
    void setUISurfaceOffsetWidth(F32 f) { mUICurvedSurfaceOffsets[VX] = f; onChangeUISurfaceShape(); }
    void setUISurfaceOffsetHeight(F32 f) { mUICurvedSurfaceOffsets[VY] = f; onChangeUISurfaceShape(); }
    void setUISurfaceOffsetDepth(F32 f) { mUICurvedSurfaceOffsets[VZ] = f; onChangeUISurfaceShape(); }

    LLViewerTexture* getCursorImage(U32 cursorType);
    LLViewerTexture* getCalibrateBackground();
    LLViewerTexture* getCalibrateForeground();

    LLVertexBuffer* createUISurface();
    void getUISurfaceCoordinates(F32 ha, F32 va, LLVector4& pos, LLVector2* uv = NULL);
    void updateHMDMouseInfo(S32 ui_x, S32 ui_y);
    const LLVector3& getMouseWorld() const { return mMouseWorld; }
    void setMouseWorldEnd(const LLVector4a& mwe) { mMouseWorldEnd = mwe; }
    const LLVector4a& getMouseWorldEnd() const { return mMouseWorldEnd; }
    void setMouseWorldIntersection(const LLVector4a& intersection, const LLVector4a& normal, const LLVector4a& tangent)
    {
        mMouseWorldRaycastIntersection = intersection;
        mMouseWorldRaycastNormal = normal;
        mMouseWorldRaycastTangent = tangent;
    }
    const LLVector4a& getMouseWorldRaycastIntersection() const { return mMouseWorldRaycastIntersection; }
    const LLVector4a& getMouseWorldRaycastNormal() const { return mMouseWorldRaycastNormal; }
    const LLVector4a& getMouseWorldRaycastTangent() const { return mMouseWorldRaycastTangent; }

    F32 getWorldCursorSizeMult() const { return mMouseWorldSizeMult; }

    void saveSettings();

    static void onChangeUISurfaceSavedParams();
    static void onChangeUISurfaceShape();
    static void onChangeEyeDepth();
    static void onChangeUIMagnification();
    static void onChangeWorldCursorSizeMult();

private:
    void calculateUIEyeDepth();

private:
    LLHMDImpl* mImpl;
    U32 mFlags;
    U32 mRenderMode;
    F32 mBaseModelView[16];
    F32 mBaseModelViewInv[16];
    F32 mBaseProjection[16];
    F32 mUIModelView[16];
    F32 mUIModelViewInv[16];
    F32 mBaseLookAt[16];
    S32 mMainWindowWidth;
    S32 mMainWindowHeight;
    LLVector2 mUICurvedSurfaceArc;
    LLVector4 mUICurvedSurfaceRadius;
    LLVector3 mUICurvedSurfaceOffsets;
    F32 mEyeDepth;
    F32 mUIMagnification;
    F32 mUIEyeDepth;
    // in-world coordinates of mouse pointer on the UI surface
    LLVector3 mMouseWorld;
    // in-world coordinates of raycast from viewpoint into world, assuming no collisions.
    // Used for rendering in-world cursor over sky, etc.
    LLVector4a mMouseWorldEnd;
    // gDebugRaycastIntersection - i.e. where ray from eye to the world (through (mMouseWorld) meets landscape (or an object)
    LLVector4a mMouseWorldRaycastIntersection;
    LLVector4a mMouseWorldRaycastNormal;
    LLVector4a mMouseWorldRaycastTangent;
    F32 mMouseWorldSizeMult;
};

extern LLHMD gHMD;

#endif // LL_LLHMD_H

