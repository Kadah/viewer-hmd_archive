/** 
 * @file llfloaterhmdconfigdebug.h
 * @brief A floater showing the head mounted display (HMD) config UI
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

#ifndef LL_LLFLOATERHMDCONFIGDEBUG_H
#define LL_LLFLOATERHMDCONFIGDEBUG_H

#include "linden_common.h"

#include "llfloater.h"

class LLSlider;
class LLCheckBoxCtrl;

class LLFloaterHMDConfigDebug : public LLFloater
{
    friend class LLFloaterReg;

public:
    LLFloaterHMDConfigDebug(const LLSD& key);
	~LLFloaterHMDConfigDebug();

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);

    static LLFloaterHMDConfigDebug* getInstance();

    void onClickAddPreset();
    void onClickRemovePreset();
    void onClickCancel();
    void onClickSave();
    void onClickResetValues();

protected:
    void onSetUISurfaceOffsetDepth();
    void updateUISurfaceOffsetDepthLabel();
    void onSetUISurfaceOffsetVertical();
    void updateUISurfaceOffsetVerticalLabel();
    void onSetUISurfaceOffsetHorizontal();
    void updateUISurfaceOffsetHorizontalLabel();
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
    void updateUIShapePresetLabel(BOOL typeChanged);

    void onCheckLowPersistence();
    void onCheckPixelLuminanceOverdrive();
    void onCheckMotionPrediction();
    void onCheckTimewarp();
    void onSetTimewarpInterval();
    void updateTimewarpIntervalLabel();
    void onCheckDynamicResolutionScaling();

    void updateDirty();

protected:
    LLSlider* mUISurfaceOffsetDepthSliderCtrl;
    LLUICtrl* mUISurfaceOffsetDepthAmountCtrl;
    F32 mUISurfaceOffsetDepthOriginal;
    LLSlider* mUISurfaceOffsetVerticalSliderCtrl;
    LLUICtrl* mUISurfaceOffsetVerticalAmountCtrl;
    F32 mUISurfaceOffsetVerticalOriginal;
    LLSlider* mUISurfaceOffsetHorizontalSliderCtrl;
    LLUICtrl* mUISurfaceOffsetHorizontalAmountCtrl;
    F32 mUISurfaceOffsetHorizontalOriginal;
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
    LLCheckBoxCtrl* mLowPersistenceCheckBoxCtrl;
    BOOL mLowPersistenceCheckedOriginal;
    LLCheckBoxCtrl* mPLOCheckBoxCtrl;
    BOOL mPLOCheckedOriginal;
    LLCheckBoxCtrl* mMotionPredictionCheckBoxCtrl;
    BOOL mMotionPredictionCheckedOriginal;
    LLCheckBoxCtrl* mTimewarpCheckBoxCtrl;
    BOOL mTimewarpCheckedOriginal;
    LLSlider* mTimewarpIntervalSliderCtrl;
    LLUICtrl* mTimewarpIntervalAmountCtrl;
    F32 mTimewarpIntervalOriginal;
    LLCheckBoxCtrl* mDynamicResolutionScalingCheckBoxCtrl;
    BOOL mDynamicResolutionScalingCheckedOriginal;

    BOOL mDirty;

    static LLFloaterHMDConfigDebug* sInstance;
};

#endif // LL_LLFLOATERHMDCONFIGDEBUG_H
