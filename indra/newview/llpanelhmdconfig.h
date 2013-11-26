/** 
 * @file llpanelhmdconfig.h
 * @brief A panel showing the head mounted display (HMD) config UI
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLPANELHMDCONFIG_H
#define LL_LLPANELHMDCONFIG_H

#include "linden_common.h"

#include "llpanel.h"

class LLSlider;
class LLCheckBoxCtrl;

class LLPanelHMDConfig : public LLPanel
{
 public:
    LLPanelHMDConfig();
	~LLPanelHMDConfig();
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();

    static void toggleVisibility();
    static LLPanelHMDConfig* getInstance();

    void onClickToggleWorldView();
    void onClickCalibrate();
    void onClickCancel();
    void onClickSave();
    void onClickResetValues();

    void onSetInterpupillaryOffset();
    void updateInterpupillaryOffsetLabel();
    void onSetEyeToScreenDistance();
    void updateEyeToScreenDistanceLabel();
    void onCheckMotionPrediction();
    void onSetMotionPredictionDelta();
    void updateMotionPredictionDeltaLabel();
    void onSetUISurfaceOffsetDepth();
    void updateUISurfaceOffsetDepthLabel();
    void onSetUISurfaceToroidRadiusWidth();
    void updateUISurfaceToroidRadiusWidthLabel();
    void onSetUISurfaceToroidRadiusDepth();
    void updateUISurfaceToroidRadiusDepthLabel();
    void onSetUISurfaceToroidCrossSectionRadiusWidth();
    void updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    void onSetUISurfaceToroidCrossSectionRadiusHeight();
    void updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    void onSetUISurfaceToroidArcHorizontal();
    void updateUISurfaceToroidArcHorizontalLabel();
    void onSetUISurfaceToroidArcVertical();
    void updateUISurfaceToroidArcVerticalLabel();
    void onSetUIMagnification();
    void updateUIMagnificationLabel();
    void onSetUIShapePreset();
    void updateUIShapePresetLabel();

 private:
    static LLPanelHMDConfig*  sInstance;

    LLButton* mToggleViewButton;
    LLSlider* mInterpupillaryOffsetSliderCtrl;
    LLUICtrl* mInterpupillaryOffsetAmountCtrl;
    LLSlider* mEyeToScreenSliderCtrl;
    LLUICtrl* mEyeToScreenAmountCtrl;
    LLCheckBoxCtrl* mMotionPredictionCheckBoxCtrl;
    LLSlider* mMotionPredictionDeltaSliderCtrl;
    LLUICtrl* mMotionPredictionDeltaAmountCtrl;
    F32 mInterpupillaryOffsetOriginal;
    F32 mEyeToScreenDistanceOriginal;
    BOOL mMotionPredictionCheckedOriginal;
    F32 mMotionPredictionDeltaOriginal;
    LLSlider* mUISurfaceOffsetDepthSliderCtrl;
    LLUICtrl* mUISurfaceOffsetDepthAmountCtrl;
    F32 mUISurfaceOffsetDepthOriginal;
    LLSlider* mUISurfaceToroidRadiusWidthSliderCtrl;
    LLUICtrl* mUISurfaceToroidRadiusWidthAmountCtrl;
    F32 mUISurfaceToroidRadiusWidthOriginal;
    LLSlider* mUISurfaceToroidRadiusDepthSliderCtrl;
    LLUICtrl* mUISurfaceToroidRadiusDepthAmountCtrl;
    F32 mUISurfaceToroidRadiusDepthOriginal;
    LLSlider* mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl;
    LLUICtrl* mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl;
    F32 mUISurfaceToroidCrossSectionRadiusWidthOriginal;
    LLSlider* mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl;
    LLUICtrl* mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl;
    F32 mUISurfaceToroidCrossSectionRadiusHeightOriginal;
    LLSlider* mUISurfaceToroidArcHorizontalSliderCtrl;
    LLUICtrl* mUISurfaceToroidArcHorizontalAmountCtrl;
    F32 mUISurfaceToroidArcHorizontalOriginal;
    LLSlider* mUISurfaceToroidArcVerticalSliderCtrl;
    LLUICtrl* mUISurfaceToroidArcVerticalAmountCtrl;
    F32 mUISurfaceToroidArcVerticalOriginal;
    LLSlider* mUIMagnificationSliderCtrl;
    LLUICtrl* mUIMagnificationAmountCtrl;
    F32 mUIMagnificationOriginal;
    LLSlider* mUISurfaceShapePresetSliderCtrl;
    LLUICtrl* mUISurfaceShapePresetLabelCtrl;
    F32 mUISurfaceShapePresetOriginal;
};

#endif // LL_LLPANELHMDCONFIG_H
