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
    };

public:
    LLHMD();
    ~LLHMD();

    BOOL init();
    void shutdown();
    void onIdle();
    void adjustLookAt(LLVector3& origin, LLVector3& up, LLVector3& poi);

    // True if the HMD has been successfully initialized
    BOOL isInitialized() const;
    BOOL failedInit() const;

    // experimental: attempt to render 2D UI
    BOOL shouldRender2DUI() const;
    void shouldRender2DUI(BOOL b);

    // True if the HMD is initialized and currently in a render mode != RenderMode_None
    BOOL shouldRender() const;

    // get/set current HMD rendering mode
    U32 getRenderMode() const;
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

    void setFOV(F32 fov);

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

private:
    LLHMDImpl* mImpl;
};

extern LLHMD gHMD;

#endif // LL_LLHMD_H

