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
#define LL_HMD_EXPERIMENTAL 0

#include "llpointer.h"

class LLHMDImpl;
class LLViewerTexture;
class LLVertexBuffer;
class LLMouseHandler;


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
        kFlag_Pre_Initialized           = 1 << 0,
        kFlag_Post_Initialized          = 1 << 1,
        kFlag_FailedInit                = 1 << 2,
        kFlag_HMDConnected              = 1 << 3,
        kFlag_MainIsFullScreen          = 1 << 4,
        kFlag_CursorIntersectsWorld     = 1 << 5,
        kFlag_CursorIntersectsUI        = 1 << 6,
        kFlag_AdvancedMode              = 1 << 7,
        kFlag_ChangingRenderContext     = 1 << 8,
        kFlag_HMDAllowed                = 1 << 9,
        kFlag_MoveFollowsLookDir        = 1 << 10,
        kFlag_HMDSensorConnected        = 1 << 11,
        kFlag_LatencyTesterConnected    = 1 << 12,
        kFlag_HMDMirror                 = 1 << 13,
        kFlag_SavingSettings            = 1 << 14,
    };

    enum eUIPresetType
    {
        kCustom = 0,
        kDefault,
        kUser,
    };

    struct UISurfaceShapeSettings
    {
        U32 mPresetType;
        U32 mPresetTypeIndex;
        F32 mOffsetX;
        F32 mOffsetY;
        F32 mOffsetZ;
        F32 mToroidRadiusWidth;
        F32 mToroidRadiusDepth;
        F32 mToroidCrossSectionRadiusWidth;
        F32 mToroidCrossSectionRadiusHeight;
        F32 mArcHorizontal;
        F32 mArcVertical;
        F32 mUIMagnification;
    };


public:
    LLHMD();
    ~LLHMD();

    BOOL init();
    void shutdown();
    void onIdle();

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
    BOOL cursorIntersectsWorld() const { return ((mFlags & kFlag_CursorIntersectsWorld) != 0) ? TRUE : FALSE; }
    void cursorIntersectsWorld(BOOL b) { if (b) { mFlags |= kFlag_CursorIntersectsWorld; } else { mFlags &= ~kFlag_CursorIntersectsWorld; } }
    BOOL cursorIntersectsUI() const { return ((mFlags & kFlag_CursorIntersectsUI) != 0) ? TRUE : FALSE; }
    void cursorIntersectsUI(BOOL b) { if (b) { mFlags |= kFlag_CursorIntersectsUI; } else { mFlags &= ~kFlag_CursorIntersectsUI; } }
    BOOL isAdvancedMode() const { return ((mFlags & kFlag_AdvancedMode) != 0) ? TRUE : FALSE; }
    void isAdvancedMode(BOOL b) { if (b) { mFlags |= kFlag_AdvancedMode; } else { mFlags &= ~kFlag_AdvancedMode; } }
    BOOL isChangingRenderContext() const { return ((mFlags & kFlag_ChangingRenderContext) != 0) ? TRUE : FALSE; }
    void isChangingRenderContext(BOOL b) { if (b) { mFlags |= kFlag_ChangingRenderContext; } else { mFlags &= ~kFlag_ChangingRenderContext; } }
    BOOL isHMDAllowed() const { return ((mFlags & kFlag_HMDAllowed) != 0) ? TRUE : FALSE; }
    void isHMDAllowed(BOOL b) { if (b) { mFlags |= kFlag_HMDAllowed; } else { mFlags &= ~kFlag_HMDAllowed; } }
    BOOL moveFollowsLookDir() const { return ((mFlags & kFlag_MoveFollowsLookDir) != 0) ? TRUE : FALSE; }
    void moveFollowsLookDir(BOOL b) { if (b) { mFlags |= kFlag_MoveFollowsLookDir; } else { mFlags &= ~kFlag_MoveFollowsLookDir; } }
    BOOL isHMDSensorConnected() const { return ((mFlags & kFlag_HMDSensorConnected) != 0) ? TRUE : FALSE; }
    void isHMDSensorConnected(BOOL b) { if (b) { mFlags |= kFlag_HMDSensorConnected; } else { mFlags &= ~kFlag_HMDSensorConnected; } }
    BOOL isLatencyTesterConnected() const { return ((mFlags & kFlag_LatencyTesterConnected) != 0) ? TRUE : FALSE; }
    void isLatencyTesterConnected(BOOL b) { if (b) { mFlags |= kFlag_LatencyTesterConnected; } else { mFlags &= ~kFlag_LatencyTesterConnected; } }
    BOOL isHMDMirror() const { return ((mFlags & kFlag_HMDMirror) != 0) ? TRUE : FALSE; }
    void isHMDMirror(BOOL b) { if (b) { mFlags |= kFlag_HMDMirror; } else { mFlags &= ~kFlag_HMDMirror; } }
    BOOL isSavingSettings() const { return ((mFlags & kFlag_SavingSettings) != 0) ? TRUE : FALSE; }
    void isSavingSettings(BOOL b) { if (b) { mFlags |= kFlag_SavingSettings; } else { mFlags &= ~kFlag_SavingSettings; } }

    // True if the HMD is initialized and currently in a render mode != RenderMode_None
    BOOL isHMDMode() const { return mRenderMode != RenderMode_None; }

    // get/set current HMD rendering mode
    U32 getRenderMode() const { return mRenderMode; }
    void setRenderMode(U32 mode, bool setFocusWindow = true);
    BOOL setRenderWindowMain();
    BOOL setRenderWindowHMD();
    void setFocusWindowMain();
    void setFocusWindowHMD();
    void onAppFocusGained();
    void onAppFocusLost();
    void renderUnusedMainWindow();
    void renderUnusedHMDWindow();
    U32 suspendHMDMode();
    void resumeHMDMode(U32 prevRenderMode);

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
    F32 getAspect();
    F32 getUIAspect() const { return mPresetUIAspect; }
    F32 getEyeDepth() const { return mEyeDepth; }
    F32 getUIEyeDepth() const { return mUIEyeDepth; }
    F32 getUIMagnification() { return mUIShape.mUIMagnification; }
    void setUIMagnification(F32 f);

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
    F32 getHMDRoll() const;
    F32 getHMDPitch() const;
    F32 getHMDYaw() const;

    // head correction (difference in rotation between head and body)
    LLQuaternion getHeadRotationCorrection() const;
    void addHeadRotationCorrection(LLQuaternion quat);
    void resetHeadRotationCorrection();
    void resetOrientation();

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

    LLCoordScreen getMainWindowPos() const { return mMainWindowPos; }
    void setMainWindowPos(LLCoordScreen pos) { mMainWindowPos = pos; }
    S32 getMainWindowWidth() const { return mMainWindowSize.mX; }
    void setMainWindowWidth(S32 w) { mMainWindowSize.mX = w; }
    S32 getMainWindowHeight() const { return mMainWindowSize.mY; }
    void setMainWindowHeight(S32 h) { mMainWindowSize.mY = h; }
    LLCoordScreen getMainWindowSize() const { return mMainWindowSize; }
    S32 getMainClientWidth() const { return mMainClientSize.mX; }
    void setMainClientWidth(S32 w) { mMainClientSize.mX = w; }
    S32 getMainClientHeight() const { return mMainClientSize.mY; }
    void setMainClientHeight(S32 h) { mMainClientSize.mY = h; }
    LLCoordWindow getMainClientSize() const { return mMainClientSize; }
    LLCoordWindow getHMDClientSize() const { return LLCoordWindow(getHMDWidth(), getHMDHeight()); }

    std::string getUIShapeName() const;
    F32 getUISurfaceArcHorizontal() const { return mUIShape.mArcHorizontal; }
    void setUISurfaceArcHorizontal(F32 f) { setUISurfaceParam(&mUIShape.mArcHorizontal, f); }
    F32 getUISurfaceArcVertical() const { return mUIShape.mArcVertical; }
    void setUISurfaceArcVertical(F32 f) { setUISurfaceParam(&mUIShape.mArcVertical, f); }
    F32 getUISurfaceToroidRadiusWidth() const { return mUIShape.mToroidRadiusWidth; }
    void setUISurfaceToroidRadiusWidth(F32 f) { setUISurfaceParam(&mUIShape.mToroidRadiusWidth, f); }
    F32 getUISurfaceToroidRadiusDepth() const { return mUIShape.mToroidRadiusDepth; }
    void setUISurfaceToroidRadiusDepth(F32 f) { setUISurfaceParam(&mUIShape.mToroidRadiusDepth, f); }
    F32 getUISurfaceToroidCrossSectionRadiusWidth() const { return mUIShape.mToroidCrossSectionRadiusWidth; }
    void setUISurfaceToroidCrossSectionRadiusWidth(F32 f) { setUISurfaceParam(&mUIShape.mToroidCrossSectionRadiusWidth, f); }
    F32 getUISurfaceToroidCrossSectionRadiusHeight() const { return mUIShape.mToroidCrossSectionRadiusHeight; }
    void setUISurfaceToroidCrossSectionRadiusHeight(F32 f) { setUISurfaceParam(&mUIShape.mToroidCrossSectionRadiusHeight, f); }
    F32 getUISurfaceOffsetWidth() const { return mUIShape.mOffsetX; }
    void setUISurfaceOffsetWidth(F32 f) { setUISurfaceParam(&mUIShape.mOffsetX, f); }
    F32 getUISurfaceOffsetHeight() const { return mUIShape.mOffsetY; }
    void setUISurfaceOffsetHeight(F32 f) { setUISurfaceParam(&mUIShape.mOffsetY, f); }
    F32 getUISurfaceOffsetDepth() const { return mUIShape.mOffsetZ; }
    void setUISurfaceOffsetDepth(F32 f) { setUISurfaceParam(&mUIShape.mOffsetZ, f); }
    U32 getUIShapePresetType() const { return mUIShape.mPresetType; }
    U32 getUIShapePresetTypeIndex() const { return mUIShape.mPresetTypeIndex; }
    S32 getUIShapePresetIndex() const { return mUIShapePreset; }
    void setUIShapePresetIndex(S32 idx);
    S32 getUIShapePresetIndexDefault() const { return 1; }
    LLHMD::UISurfaceShapeSettings getUIShapePreset(S32 idx);
    S32 getNumUIShapePresets() const { return (S32)mUIPresetValues.size(); }
    BOOL addPreset();
    BOOL removePreset(S32 idx);

    const char* getLatencyTesterResults();

    LLViewerTexture* getCursorImage(U32 cursorType) { return (cursorType < mCursorTextures.size()) ? mCursorTextures[cursorType].get() : NULL; }
    const LLVector2& getCursorHotspotOffset(U32 cursorType) { return (cursorType < mCursorHotSpotOffsets.size()) ? mCursorHotSpotOffsets[cursorType] : LLVector2::zero; }

    LLVertexBuffer* createUISurface();
    void getUISurfaceCoordinates(F32 ha, F32 va, LLVector4& pos, LLVector2* uv = NULL);
    void calculateMouseWorld(S32 mouse_x, S32 mouse_y, LLVector3& world);
    void updateHMDMouseInfo();
    const LLVector3& getMouseWorld() const { return mMouseWorld; }
    void updateMouseRaycast(const LLVector4a& mwe) { mMouseWorldEnd = mwe; }
    const LLVector4a& getMouseWorldEnd() const { return mMouseWorldEnd; }
    void setMouseWorldRaycastIntersection(const LLVector3& intersection)
    {
        LLVector4a ni;
        ni.load3(intersection.mV);
        setMouseWorldRaycastIntersection(ni);
    }
    void setMouseWorldRaycastIntersection(const LLVector4a& intersection) { mMouseWorldRaycastIntersection = intersection; }
    void setMouseWorldRaycastIntersection(const LLVector4a& intersection, const LLVector4a& normal, const LLVector4a& tangent)
    {
        mMouseWorldRaycastIntersection = intersection;
        mMouseWorldRaycastNormal = normal;
        mMouseWorldRaycastTangent = tangent;
    }
    const LLVector4a& getMouseWorldRaycastIntersection() const { return mMouseWorldRaycastIntersection; }
    const LLVector4a& getMouseWorldRaycastNormal() const { return mMouseWorldRaycastNormal; }
    const LLVector4a& getMouseWorldRaycastTangent() const { return mMouseWorldRaycastTangent; }

    // returns TRUE if we're in HMD Mode, mh is valid and mh has a valid mouse intersect override (in either UI or global coordinate space)
    BOOL handleMouseIntersectOverride(LLMouseHandler* mh);

    F32 getWorldCursorSizeMult() const { return mMouseWorldSizeMult; }

    void saveSettings();

    static void onChangeHMDAdvancedMode();
    static void onChangeInterpupillaryDistance();
    static void onChangeEyeToScreenDistance();
    static void onChangeEyeDepth();
    static void onChangeUISurfaceSavedParams();
    static void onChangeUISurfaceShape();
    static void onChangeUIMagnification();
    static void onChangeUIShapePreset();
    static void onChangeWorldCursorSizeMult();
    static void onChangePresetValues();
    static void onChangeMoveFollowsLookDir();

#if LL_HMD_EXPERIMENTAL 
    const LLVector2& getCamFrustumLocs() const { return mCamFrustumUILocs; }
#endif

private:
    void calculateUIEyeDepth();
    void setUISurfaceParam(F32* p, F32 f);
    void calculateMouseWorld2(F32 nx, F32 ny, LLVector3& world);

private:
    LLHMDImpl* mImpl;
    U32 mFlags;
    U32 mRenderMode;
    F32 mInterpupillaryDistance;
    LLHMD::UISurfaceShapeSettings mUIShape;
    F32 mEyeDepth;
    F32 mUIEyeDepth;
    S32 mUIShapePreset;
    U32 mNextUserPresetIndex;
    F32 mBaseModelView[16];
    F32 mBaseModelViewInv[16];
    F32 mBaseProjection[16];
    F32 mUIModelView[16];
    F32 mUIModelViewInv[16];
    F32 mBaseLookAt[16];
    LLCoordScreen mMainWindowPos;
    LLCoordScreen mMainWindowSize;
    LLCoordWindow mMainClientSize;
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
    F32 mPresetUIAspect;
    std::vector<UISurfaceShapeSettings> mUIPresetValues;
    std::vector<LLPointer<LLViewerTexture> > mCursorTextures;
    std::vector<LLVector2> mCursorHotSpotOffsets;
    LLPointer<LLViewerTexture> mCalibrateBackgroundTexture;
    LLPointer<LLViewerTexture> mCalibrateForegroundTexture;
#if LL_HMD_EXPERIMENTAL 
    LLVector2 mCamFrustumUILocs;
#endif
};

extern LLHMD gHMD;



// dummmy class to satisfy API requirements on platforms which we don't support HMD on
class LLHMDImpl
{
public:
    static const S32 kDefaultHResolution = 1280;
    static const S32 kDefaultVResolution = 800;
    static const F32 kDefaultHScreenSize;
    static const F32 kDefaultVScreenSize;
    static const F32 kDefaultInterpupillaryOffset;
    static const F32 kDefaultLenSeparationDistance;
    static const F32 kDefaultEyeToScreenDistance;
    static const F32 kDefaultDistortionConstant0;
    static const F32 kDefaultDistortionConstant1;
    static const F32 kDefaultDistortionConstant2;
    static const F32 kDefaultDistortionConstant3;
    static const F32 kDefaultXCenterOffset;
    static const F32 kDefaultYCenterOffset;
    static const F32 kDefaultDistortionScale;
    static const F32 kDefaultOrthoPixelOffset;
    static const F32 kDefaultVerticalFOVRadians;
    static const F32 kDefaultAspect;
    static const F32 kDefaultAspectMult;


public:
    LLHMDImpl() {}
    virtual ~LLHMDImpl() {}

    virtual BOOL preInit() { return FALSE; }
    virtual BOOL postDetectionInit() { return FALSE; }
    virtual void shutdown() {}
    virtual void onIdle() {}
    virtual U32 getCurrentEye() const { return 0; }
    virtual void setCurrentEye(U32 eye) {}
    virtual void getViewportInfo(S32& x, S32& y, S32& w, S32& h) { x = y = w = h = 0; }

    virtual S32 getHMDWidth() const { return kDefaultHResolution; }
    virtual S32 getHMDEyeWidth() const { return (kDefaultHResolution / 2); }
    virtual S32 getHMDHeight() const { return kDefaultVResolution; }
    virtual S32 getHMDUIWidth() const { return kDefaultHResolution; }
    virtual S32 getHMDUIHeight() const { return kDefaultVResolution; }
    virtual F32 getPhysicalScreenWidth() const { return kDefaultHScreenSize; }
    virtual F32 getPhysicalScreenHeight() const { return kDefaultVScreenSize; }
    virtual F32 getInterpupillaryOffset() const { return kDefaultInterpupillaryOffset; }
    virtual F32 getInterpupillaryOffsetDefault() const { return kDefaultInterpupillaryOffset; }
    virtual void setInterpupillaryOffset(F32 f) {}
    virtual F32 getLensSeparationDistance() const { return kDefaultLenSeparationDistance; }
    virtual F32 getEyeToScreenDistance() const { return kDefaultEyeToScreenDistance; }
    virtual F32 getEyeToScreenDistanceDefault() const { return kDefaultEyeToScreenDistance; }
    virtual void setEyeToScreenDistance(F32 f) {}
    virtual F32 getVerticalFOV() { return kDefaultVerticalFOVRadians; }
    virtual F32 getAspect() { return kDefaultAspect; }
    virtual F32 getAspectMultiplier() { return kDefaultAspectMult; }
    virtual void setAspectMultiplier(F32 f) {}

    virtual LLVector4 getDistortionConstants() const { return LLVector4::zero; }

    virtual F32 getXCenterOffset() const { return 0.0f; }
    virtual F32 getYCenterOffset() const { return 0.0f; }
    virtual F32 getDistortionScale() const { return kDefaultDistortionScale; }

    virtual BOOL useMotionPrediction() { return FALSE; }
    virtual BOOL useMotionPredictionDefault() const { return FALSE; }
    virtual void useMotionPrediction(BOOL b) {}
    virtual F32 getMotionPredictionDelta() { return 0.0f; }
    virtual F32 getMotionPredictionDeltaDefault() const { return 0.03f; }
    virtual void setMotionPredictionDelta(F32 f) {}

    virtual LLQuaternion getHMDOrient() const { return LLQuaternion::DEFAULT; }

    virtual F32 getRoll() const { return 0.0f; }
    virtual F32 getPitch() const { return 0.0f; }
    virtual F32 getYaw() const { return 0.0f; }
    virtual void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = pitch = yaw = 0.0f; }

    virtual LLQuaternion getHeadRotationCorrection() const { return LLQuaternion::DEFAULT; }
    virtual void addHeadRotationCorrection(LLQuaternion quat) {}
    virtual void resetHeadRotationCorrection() {}

    virtual F32 getOrthoPixelOffset() const { return kDefaultOrthoPixelOffset; }

    virtual void resetOrientation() {}

    virtual const char* getLatencyTesterResults() { return ""; }
};

#endif // LL_LLHMD_H

