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

#include "llpointer.h"
#include "glh/glh_linear.h"

class LLHMDImpl;
class LLViewerTexture;
class LLVertexBuffer;
class LLMouseHandler;
class LLRenderTarget;

// TODO: move some of the data to this class instead of always requiring an extra method call via PIMPL
class LLHMD
{
public:
    enum eRenderMode
    {
        RenderMode_None = 0,        // Do not render to HMD
        RenderMode_HMD,             // render to HMD  (Direct Mode)
        RenderMode_ScreenStereo,    // render to main window in stereoscopic mode (Debugging or Extended Mode)
        RenderMode_Last = RenderMode_ScreenStereo,
    };

    enum eCameraEye
    {
        CENTER_EYE  = 0,
        LEFT_EYE    = 1,
        RIGHT_EYE   = 2,
    };

    enum eRPY
    {
        ROLL = 0,
        PITCH = 1,
        YAW = 2,
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
        kFlag_MainIsMaximized           = 1 << 10,
        kFlag_HMDMirror                 = 1 << 11,
        kFlag_SavingSettings            = 1 << 12,
        kFlag_UsingDebugHMD             = 1 << 13,
        kFlag_HMDDisplayEnabled         = 1 << 14,
        kFlag_HMDDirectMode             = 1 << 15,
        kFlag_PositionTrackingEnabled   = 1 << 16,
        kFlag_FrameInProgress           = 1 << 17,
        kFlag_SettingsChanged           = 1 << 18,
        kFlag_MouselookYawOnly          = 1 << 19,
        kFlag_AllowTextRoll             = 1 << 20
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

    enum eMouselookControlMode
    {
        kMouselookControl_BEGIN = 0,

        // Move Direction = mouse
        // Look Direction = HMD
        // Fire Direction = mouse (Move Direction)
        kMouselookControl_Independent = kMouselookControl_BEGIN,

        // Move Direction = HMD (extra yaw rotation added after threshold)
        // Look Direction = HMD (Move Direction)
        // Fire Direction = HMD (Move Direction)
        kMouselookControl_Linked,

        kMouselookControl_END,
        kMouselookControl_Default = kMouselookControl_Independent,
    };

public:
    LLHMD();
    ~LLHMD();

    BOOL init();
    void shutdown();

    BOOL isInitialized() const { return ((mFlags & kFlag_Pre_Initialized) != 0) ? TRUE : FALSE; }
    void isInitialized(BOOL b) { if (b) { mFlags |= kFlag_Pre_Initialized; } else { mFlags &= ~kFlag_Pre_Initialized; } }

    BOOL isFailedInit() const { return ((mFlags & kFlag_FailedInit) != 0) ? TRUE : FALSE; }
    void isFailedInit(BOOL b) { if (b) { mFlags |= kFlag_FailedInit; } else { mFlags &= ~kFlag_FailedInit; } }

    BOOL isHMDConnected() const { return ((mFlags & kFlag_HMDConnected) != 0) ? TRUE : FALSE; }
    void isHMDConnected(BOOL b) { if (b) { mFlags |= kFlag_HMDConnected; } else { mFlags &= ~kFlag_HMDConnected; } }

    BOOL isMouselookYawOnly() const { return ((mFlags & kFlag_MouselookYawOnly) != 0) ? TRUE : FALSE; }
    void isMouselookYawOnly(BOOL b) { if (b) { mFlags |= kFlag_MouselookYawOnly; } else { mFlags &= ~kFlag_MouselookYawOnly; } }

    BOOL allowTextRoll() const { return ((mFlags & kFlag_AllowTextRoll) != 0) ? TRUE : FALSE; }
    void allowTextRoll(BOOL b) { if (b) { mFlags |= kFlag_AllowTextRoll; } else { mFlags &= ~kFlag_AllowTextRoll; } }

    BOOL cursorIntersectsWorld() const { return ((mFlags & kFlag_CursorIntersectsWorld) != 0) ? TRUE : FALSE; }
    void cursorIntersectsWorld(BOOL b) { if (b) { mFlags |= kFlag_CursorIntersectsWorld; } else { mFlags &= ~kFlag_CursorIntersectsWorld; } }
    BOOL cursorIntersectsUI() const { return ((mFlags & kFlag_CursorIntersectsUI) != 0) ? TRUE : FALSE; }
    void cursorIntersectsUI(BOOL b) { if (b) { mFlags |= kFlag_CursorIntersectsUI; } else { mFlags &= ~kFlag_CursorIntersectsUI; } }

    BOOL renderSettingsChanged() const { return ((mFlags & kFlag_SettingsChanged) != 0) ? TRUE : FALSE; }
    void renderSettingsChanged(BOOL b) { if (b) { mFlags |= kFlag_SettingsChanged; } else { mFlags &= ~kFlag_SettingsChanged; } }


    // True if render mode != RenderMode_None
    BOOL isHMDMode() const { return mRenderMode != RenderMode_None; }

    // get/set current HMD rendering mode
    U32 getRenderMode() const { return mRenderMode; }
    void setRenderMode(U32 mode, bool setFocusWindow = true);

    void onAppFocusGained();
    void onAppFocusLost();

    U32 suspendHMDMode();
    void resumeHMDMode(U32 prevRenderMode);

    // size and Lower-Left corner for the viewer of the current eye
    void getViewportInfo(S32& x, S32& y, S32& w, S32& h);
    void getViewportInfo(S32 vp[4]);

    F32 getPixelDensity() const;
    void setPixelDensity(F32 pixelDensity);

    S32 getHMDWidth() const;
    S32 getHMDHeight() const;
    S32 getHMDEyeWidth() const;

    S32 getHMDUIWidth() const;
    S32 getHMDUIHeight() const;

    S32 getHMDViewportWidth() const;
    S32 getHMDViewportHeight() const;

    F32 getInterpupillaryOffset() const;
    F32 getInterpupillaryOffsetDefault() const;

    F32 getEyeToScreenDistance() const;
    void setEyeToScreenDistance(F32 f);

    F32 getVerticalFOV() const;
    F32 getAspect();

    F32 getUIAspect() const { return (F32)getHMDUIWidth() / (F32)getHMDUIHeight(); }
    F32 getUIEyeDepth() const { return mUIEyeDepth; }
    F32 getUIMagnification() { return mUIShape.mUIMagnification; }

    void setUIMagnification(F32 f);
    void calculateUIEyeDepth();

    // Get the current HMD orientation
    void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const;
    LLQuaternion getHMDRotation() const;
    void getHMDLastRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const;
    void getHMDDeltaRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const;

    F32 getHMDRoll() const;
    F32 getHMDLastRoll() const;
    F32 getHMDDeltaRoll() const;
    F32 getHMDPitch() const;
    F32 getHMDLastPitch() const;
    F32 getHMDDeltaPitch() const;
    F32 getHMDYaw() const;
    F32 getHMDLastYaw() const;
    F32 getHMDDeltaYaw() const;

    void resetOrientation();

    void setBaseModelView(F32* m);
    F32* getBaseModelView() { return mBaseModelView; }
    F32* getBaseModelViewInv() { return mBaseModelViewInv; }
    void setBaseProjection(F32* m) { for (int i = 0; i < 16; ++i) { mBaseProjection[i] = m[i]; } }
    F32* getBaseProjection() { return mBaseProjection; }
    void setUIModelView(F32* m);
    F32* getUIModelView() { return mUIModelView; }
    F32* getUIModelViewInv() { return mUIModelViewInv; }
    
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
    F32 getUISurfaceOffsetHorizontal() const { return mUIShape.mOffsetX; }
    void setUISurfaceOffsetHorizontal(F32 f) { setUISurfaceParam(&mUIShape.mOffsetX, f); }
    F32 getUISurfaceOffsetVertical() const { return mUIShape.mOffsetY; }
    void setUISurfaceOffsetVertical(F32 f) { setUISurfaceParam(&mUIShape.mOffsetY, f); }
    F32 getUISurfaceOffsetDepth() const { return mUIShape.mOffsetZ; }
    void setUISurfaceOffsetDepth(F32 f) { setUISurfaceParam(&mUIShape.mOffsetZ, f); }
    U32 getUIShapePresetType() const { return mUIShape.mPresetType; }
    U32 getUIShapePresetTypeIndex() const { return mUIShape.mPresetTypeIndex; }
    S32 getUIShapePresetIndex() const { return mUIShapePreset; }
    void setUIShapePresetIndex(S32 idx);
    LLHMD::UISurfaceShapeSettings getUIShapePreset(S32 idx);
    S32 getNumUIShapePresets() const { return (S32)mUIPresetValues.size(); }
    BOOL addPreset();
    BOOL updatePreset();
    BOOL removePreset(S32 idx);
    void saveSettings();

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

    S32 getMouselookControlMode() const { return mMouselookControlMode; }
    void setMouselookControlMode(S32 newMode) { mMouselookControlMode = llclamp(newMode, (S32)kMouselookControl_BEGIN, (S32)(kMouselookControl_END - 1)); }

    // returns TRUE if we're in HMD Mode, mh is valid and mh has a valid mouse intersect override (in either UI or global coordinate space)
    BOOL handleMouseIntersectOverride(LLMouseHandler* mh);

    F32 getWorldCursorSizeMult() const { return mMouseWorldSizeMult; }

    void setupStereoValues();
    void setupStereoCullFrustum();
    void setupEye(int which);

    void getEyeProjection(int whichEye, glh::matrix4f& projOut) const { projOut  = mEyeProjection[whichEye]; }
    void getEyeOffset(int whichEye, LLVector3& offsetOut)       const { offsetOut= mEyeOffset[whichEye];     }

    void getCameraOffset(LLVector3& offsetOut) const { offsetOut = mCameraOffset; }
    void getProjection(glh::matrix4f& projOut) const { projOut   = mProjection;   }

    void setup2DRender();
    void render3DUI();
    void prerender2DUI();
    void postRender2DUI();

    // defaults
    S32 getUIShapePresetIndexDefault() const { return 1; }

    BOOL isMouselookYawOnlyDefault() const { return TRUE; }

    BOOL beginFrame();
    BOOL bindEyeRT(int which);
    BOOL releaseEyeRT(int which);
    BOOL endFrame();
    BOOL releaseAllEyeRT();

    void setup3DViewport(S32 x_offset, S32 y_offset, BOOL forEye);

    LLVector3 getHeadPosition() const;
    const LLQuaternion& getAgentRotation() const { return mAgentRot; }

    void reshapeUI(BOOL useUIViewPort);

    static void onChangeInterpupillaryDistance();
    static void onChangeUISurfaceSavedParams();
    static void onChangeUISurfaceShape();
    static void onChangeUIMagnification();
    static void onChangeUIShapePreset();
    static void onChangeWorldCursorSizeMult();
    static void onChangePresetValues();
    static void onChangeMouselookSettings();
    static void onChangeMouselookControlMode();
    static void onChangeRenderSettings();
    static void onChangeAllowTextRoll();

private:
    void setUISurfaceParam(F32* p, F32 f);
    void renderCursor2D();
    void renderCursor3D();

private:
    LLHMDImpl* mImpl;
    U32 mFlags;
    U32 mRenderMode;
    F32 mInterpupillaryDistance;
    LLHMD::UISurfaceShapeSettings mUIShape;
    F32 mUIEyeDepth;
    S32 mUIShapePreset;
    U32 mNextUserPresetIndex;
    F32 mBaseModelView[16];
    F32 mBaseModelViewInv[16];
    F32 mBaseProjection[16];
    F32 mUIModelView[16];
    F32 mUIModelViewInv[16];
    LLCoordScreen mMainWindowPos;
    LLCoordScreen mMainWindowSize;
    LLCoordWindow mMainClientSize;
    F32 mMainWindowFOV;
    F32 mMainWindowAspect;
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
    std::vector<UISurfaceShapeSettings> mUIPresetValues;
    std::vector<LLPointer<LLViewerTexture> > mCursorTextures;
    std::vector<LLVector2> mCursorHotSpotOffsets;
    S32 mMouselookControlMode;
    F32 mMouselookRotThreshold;
    F32 mMouselookRotMax;
    F32 mMouselookTurnSpeedMax;
    LLVector3 mLastRollPitchYaw;
    F32 mStereoCameraFOV;
    LLVector3 mStereoCameraPosition;
    LLVector3 mCameraOffset;
    LLVector3 mEyeOffset[2];
    glh::matrix4f mEyeProjection[2];
    glh::matrix4f mProjection;

    LLVector3 mStereoCullCameraDeltaForwards;
    F32 mStereoCullCameraFOV;
    F32 mStereoCullCameraAspect;
    LLQuaternion mAgentRot;
};

extern LLHMD gHMD;

// dummy class to satisfy API requirements on platforms which we don't support HMD on
// It will get them to compile, but is otherwise pretty damn useless.
class LLHMDImpl
{
public:
    static const S32 kDefaultHResolution;
    static const S32 kDefaultVResolution;
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


public:
    LLHMDImpl() {}
    virtual ~LLHMDImpl() {}

    virtual BOOL init() { return FALSE; }
    virtual void shutdown() {}

    virtual S32 getViewportWidth() const { return 0; }
    virtual S32 getViewportHeight() const { return 0; }
    virtual F32 getPixelDensity() const { return 1.0f; }
    virtual void setPixelDensity(F32 pixelDensity) { (void)pixelDensity; }

    virtual S32 getHMDWidth()    const { return kDefaultHResolution;       }
    virtual S32 getHMDEyeWidth() const { return (kDefaultHResolution / 2); }
    virtual S32 getHMDHeight()   const { return kDefaultVResolution;       }

    virtual S32 getHMDUIWidth()  const { return kDefaultHResolution; }
    virtual S32 getHMDUIHeight() const { return kDefaultVResolution; }

    virtual F32 getEyeToScreenDistance() const { return kDefaultEyeToScreenDistance; }
    virtual void setEyeToScreenDistance(F32 f) {}

    virtual F32 getVerticalFOV()          const { return kDefaultVerticalFOVRadians;   }
    virtual F32 getAspect()               const { return kDefaultAspect;               }
    virtual F32 getInterpupillaryOffset() const { return kDefaultInterpupillaryOffset; }

    virtual F32 getRoll()  const { return 0.0f; }
    virtual F32 getPitch() const { return 0.0f; }
    virtual F32 getYaw()   const { return 0.0f; }

    virtual void getHMDRollPitchYaw(F32& roll, F32& pitch, F32& yaw) const { roll = pitch = yaw = 0.0f; }

    virtual LLQuaternion getHMDRotation() const { return LLQuaternion(); };
    
    virtual void getEyeProjection(int whichEye, glh::matrix4f& proj) const { (void)proj, (void)whichEye; }
    virtual void getEyeOffset(int whichEye, LLVector3& offsetOut)    const { (void)offsetOut, (void)whichEye; }
    virtual void getCameraOffset(LLVector3& offsetOut)               const { (void)offsetOut; }
    virtual void getProjection(glh::matrix4f& projectionOut)         const { (void)projectionOut; }

    virtual void resetOrientation() {}

    virtual BOOL beginFrame()               { return FALSE; }
    virtual BOOL bindEyeRT(int whichEye)    { return FALSE; }
    virtual BOOL releaseEyeRT(int whichEye) { return FALSE; }
    virtual BOOL endFrame()                 { return FALSE; }
    virtual BOOL releaseAllEyeRT()          { return FALSE; }
    virtual U32  getFrameIndex()            { return 0;     }
    virtual U32  getSubmittedFrameIndex()   { return 0;     }

    virtual void resetFrameIndex()              {}
    virtual void incrementFrameIndex()          {}    
    virtual void incrementSubmittedFrameIndex() {}

    virtual LLVector3 getHeadPosition() const { return LLVector3::zero; }
    
    virtual BOOL calculateViewportSettings() { return FALSE; }
};

#endif // LL_LLHMD_H

