/** 
 * @file llfloaterhmdconfigdebug.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterhmdconfigdebug.h"
#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llfloaterreg.h"
#include "llhmd.h"

LLFloaterHMDConfigDebug* LLFloaterHMDConfigDebug::sInstance = NULL;

LLFloaterHMDConfigDebug::LLFloaterHMDConfigDebug(const LLSD& key)
    : LLFloater(key)
    , mInterpupillaryOffsetSliderCtrl(NULL)
    , mInterpupillaryOffsetAmountCtrl(NULL)
    , mInterpupillaryOffsetOriginal(64.0f)
    , mEyeToScreenSliderCtrl(NULL)
    , mEyeToScreenAmountCtrl(NULL)
    , mEyeToScreenDistanceOriginal(41.0f)
    , mMotionPredictionCheckBoxCtrl(NULL)
    , mMotionPredictionDeltaSliderCtrl(NULL)
    , mMotionPredictionDeltaAmountCtrl(NULL)
    , mMotionPredictionCheckedOriginal(TRUE)
    , mMotionPredictionDeltaOriginal(30.0f)
    , mUISurfaceOffsetDepthSliderCtrl(NULL)
    , mUISurfaceOffsetDepthAmountCtrl(NULL)
    , mUISurfaceOffsetDepthOriginal(0.0f)
    , mUISurfaceToroidRadiusWidthSliderCtrl(NULL)
    , mUISurfaceToroidRadiusWidthAmountCtrl(NULL)
    , mUISurfaceToroidRadiusWidthOriginal(0.3f)
    , mUISurfaceToroidRadiusDepthSliderCtrl(NULL)
    , mUISurfaceToroidRadiusDepthAmountCtrl(NULL)
    , mUISurfaceToroidRadiusDepthOriginal(0.6f)
    , mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl(NULL)
    , mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl(NULL)
    , mUISurfaceToroidCrossSectionRadiusWidthOriginal(0.5f)
    , mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl(NULL)
    , mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl(NULL)
    , mUISurfaceToroidCrossSectionRadiusHeightOriginal(0.7f)
    , mUISurfaceToroidArcHorizontalSliderCtrl(NULL)
    , mUISurfaceToroidArcHorizontalAmountCtrl(NULL)
    , mUISurfaceToroidArcHorizontalOriginal(1.0f)
    , mUISurfaceToroidArcVerticalSliderCtrl(NULL)
    , mUISurfaceToroidArcVerticalAmountCtrl(NULL)
    , mUISurfaceToroidArcVerticalOriginal(0.6f)
    , mUIMagnificationSliderCtrl(NULL)
    , mUIMagnificationAmountCtrl(NULL)
    , mUIMagnificationOriginal(600.0f)
    , mUISurfaceShapePresetSliderCtrl(NULL)
    , mUISurfaceShapePresetLabelCtrl(NULL)
    , mUISurfaceShapePresetOriginal(0.0f)
    , mDirty(FALSE)
{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfigDebug.Calibrate", boost::bind(&LLFloaterHMDConfigDebug::onClickCalibrate, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.ResetValues", boost::bind(&LLFloaterHMDConfigDebug::onClickResetValues, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.Cancel", boost::bind(&LLFloaterHMDConfigDebug::onClickCancel, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.Save", boost::bind(&LLFloaterHMDConfigDebug::onClickSave, this));

    mCommitCallbackRegistrar.add("HMDConfigDebug.SetInterpupillaryOffset", boost::bind(&LLFloaterHMDConfigDebug::onSetInterpupillaryOffset, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceOffsetDepth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceOffsetDepth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUIMagnification", boost::bind(&LLFloaterHMDConfigDebug::onSetUIMagnification, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUIShapePreset", boost::bind(&LLFloaterHMDConfigDebug::onSetUIShapePreset, this));

    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidRadiusWidth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusWidth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidRadiusDepth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusDepth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidCrossSectionRadiusWidth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusWidth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidCrossSectionRadiusHeight", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusHeight, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidArcHorizontal", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcHorizontal, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidArcVertical", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcVertical, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetEyeToScreenDistance", boost::bind(&LLFloaterHMDConfigDebug::onSetEyeToScreenDistance, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckMotionPrediction", boost::bind(&LLFloaterHMDConfigDebug::onCheckMotionPrediction, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetMotionPredictionDelta", boost::bind(&LLFloaterHMDConfigDebug::onSetMotionPredictionDelta, this));
}

LLFloaterHMDConfigDebug::~LLFloaterHMDConfigDebug()
{
    sInstance = NULL;
}

//static
LLFloaterHMDConfigDebug* LLFloaterHMDConfigDebug::getInstance()
{
    if (!sInstance)
    {
		sInstance = (LLFloaterReg::getTypedInstance<LLFloaterHMDConfigDebug>("floater_hmd_config_debug"));
    }
    return sInstance;
}

BOOL LLFloaterHMDConfigDebug::postBuild()
{
    setVisible(FALSE);

    mInterpupillaryOffsetSliderCtrl = getChild<LLSlider>("hmd_config_debug_interpupillary_offset_slider");
    mInterpupillaryOffsetAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_interpupillary_offset_slider_amount");
    mUIMagnificationSliderCtrl = getChild<LLSlider>("hmd_config_debug_ui_magnification_slider");
    mUIMagnificationAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_ui_magnification_slider_amount");
    mUISurfaceOffsetDepthSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_offset_depth_slider");
    mUISurfaceOffsetDepthAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_offset_depth_slider_amount");
    mUISurfaceShapePresetSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_shape_preset_slider");
    mUISurfaceShapePresetLabelCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_shape_preset_value");
    mUISurfaceToroidRadiusWidthSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_toroid_radius_width_slider");
    mUISurfaceToroidRadiusWidthAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_toroid_radius_width_slider_amount");
    mUISurfaceToroidRadiusDepthSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_toroid_radius_depth_slider");
    mUISurfaceToroidRadiusDepthAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_toroid_radius_depth_slider_amount");
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_toroid_cross_section_radius_width_slider");
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_toroid_cross_section_radius_width_slider_amount");
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_toroid_cross_section_radius_height_slider");
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_toroid_cross_section_radius_height_slider_amount");
    mUISurfaceToroidArcHorizontalSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_toroid_arc_horizontal_slider");
    mUISurfaceToroidArcHorizontalAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_toroid_arc_horizontal_slider_amount");
    mUISurfaceToroidArcVerticalSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_toroid_arc_vertical_slider");
    mUISurfaceToroidArcVerticalAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_toroid_arc_vertical_slider_amount");
    mEyeToScreenSliderCtrl = getChild<LLSlider>("hmd_config_debug_eye_to_screen_distance_slider");
    mEyeToScreenAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_eye_to_screen_distance_slider_amount");
    mMotionPredictionCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_hmd_motion_prediction");
    mMotionPredictionDeltaSliderCtrl = getChild<LLSlider>("hmd_config_debug_motion_prediction_delta_slider");
    mMotionPredictionDeltaAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_motion_prediction_delta_slider_amount");

    return LLFloater::postBuild();
}

void LLFloaterHMDConfigDebug::onOpen(const LLSD& key)
{
    LLFloaterHMDConfigDebug* pPanel = LLFloaterHMDConfigDebug::getInstance();
    gHMD.shouldShowCalibrationUI(TRUE);

    if (pPanel->mInterpupillaryOffsetSliderCtrl)
    {
        pPanel->mInterpupillaryOffsetOriginal = gHMD.getInterpupillaryOffset() * 1000.0f;
        pPanel->mInterpupillaryOffsetSliderCtrl->setValue(pPanel->mInterpupillaryOffsetOriginal);
        pPanel->updateInterpupillaryOffsetLabel();
    }
    if (pPanel->mEyeToScreenSliderCtrl)
    {
        pPanel->mEyeToScreenDistanceOriginal = gHMD.getEyeToScreenDistance() * 1000.0f;
        pPanel->mEyeToScreenSliderCtrl->setValue(pPanel->mEyeToScreenDistanceOriginal);
        pPanel->updateEyeToScreenDistanceLabel();
    }
    if (pPanel->mMotionPredictionDeltaSliderCtrl)
    {
        pPanel->mMotionPredictionDeltaOriginal = gHMD.getMotionPredictionDelta() * 1000.0f;
        pPanel->mMotionPredictionDeltaSliderCtrl->setValue(pPanel->mMotionPredictionDeltaOriginal);
        pPanel->updateMotionPredictionDeltaLabel();
    }
    if (pPanel->mMotionPredictionCheckBoxCtrl)
    {
        pPanel->mMotionPredictionCheckedOriginal = gHMD.useMotionPrediction();
        pPanel->mMotionPredictionCheckBoxCtrl->setValue(pPanel->mMotionPredictionCheckedOriginal);
    }
    if (pPanel->mUISurfaceOffsetDepthSliderCtrl)
    {
        pPanel->mUISurfaceOffsetDepthOriginal = gHMD.getUISurfaceOffsetDepth();
        pPanel->mUISurfaceOffsetDepthSliderCtrl->setValue(pPanel->mUISurfaceOffsetDepthOriginal);
        pPanel->updateUISurfaceOffsetDepthLabel();
    }
    if (pPanel->mUISurfaceToroidRadiusWidthSliderCtrl)
    {
        pPanel->mUISurfaceToroidRadiusWidthOriginal = gHMD.getUISurfaceToroidRadiusWidth();
        pPanel->mUISurfaceToroidRadiusWidthSliderCtrl->setValue(pPanel->mUISurfaceToroidRadiusWidthOriginal);
        pPanel->updateUISurfaceToroidRadiusWidthLabel();
    }
    if (pPanel->mUISurfaceToroidRadiusDepthSliderCtrl)
    {
        pPanel->mUISurfaceToroidRadiusDepthOriginal = gHMD.getUISurfaceToroidRadiusDepth();
        pPanel->mUISurfaceToroidRadiusDepthSliderCtrl->setValue(pPanel->mUISurfaceToroidRadiusDepthOriginal);
        pPanel->updateUISurfaceToroidRadiusDepthLabel();
    }
    if (pPanel->mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl)
    {
        pPanel->mUISurfaceToroidCrossSectionRadiusWidthOriginal = gHMD.getUISurfaceToroidCrossSectionRadiusWidth();
        pPanel->mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(pPanel->mUISurfaceToroidCrossSectionRadiusWidthOriginal);
        pPanel->updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    }
    if (pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl)
    {
        pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal = gHMD.getUISurfaceToroidCrossSectionRadiusHeight();
        pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal);
        pPanel->updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    }
    if (pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl)
    {
        pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal = gHMD.getUISurfaceToroidCrossSectionRadiusHeight();
        pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal);
        pPanel->updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    }
    if (pPanel->mUISurfaceToroidArcHorizontalSliderCtrl)
    {
        pPanel->mUISurfaceToroidArcHorizontalOriginal = gHMD.getUISurfaceArcHorizontal() / F_PI;
        pPanel->mUISurfaceToroidArcHorizontalSliderCtrl->setValue(pPanel->mUISurfaceToroidArcHorizontalOriginal);
        pPanel->updateUISurfaceToroidArcHorizontalLabel();
    }
    if (pPanel->mUISurfaceToroidArcVerticalSliderCtrl)
    {
        pPanel->mUISurfaceToroidArcVerticalOriginal = gHMD.getUISurfaceArcVertical() / F_PI;
        pPanel->mUISurfaceToroidArcVerticalSliderCtrl->setValue(pPanel->mUISurfaceToroidArcVerticalOriginal);
        pPanel->updateUISurfaceToroidArcVerticalLabel();
    }
    if (pPanel->mUIMagnificationSliderCtrl)
    {
        pPanel->mUIMagnificationOriginal = gHMD.getUIMagnification();
        pPanel->mUIMagnificationSliderCtrl->setValue(pPanel->mUIMagnificationOriginal);
        pPanel->updateUIMagnificationLabel();
    }
    if (pPanel->mUISurfaceShapePresetSliderCtrl)
    {
        pPanel->mUISurfaceShapePresetOriginal = (F32)gHMD.getUIShapePresetIndex();
        pPanel->mUISurfaceShapePresetSliderCtrl->setValue(pPanel->mUISurfaceShapePresetOriginal);
        pPanel->updateUIShapePresetLabel();
    }
    mDirty = FALSE;
}

void LLFloaterHMDConfigDebug::onClose(bool app_quitting)
{
	if(app_quitting)
    {
		return;
    }
    if (mDirty)
    {
        onClickCancel();
    }
    gHMD.shouldShowCalibrationUI(FALSE);
}

void LLFloaterHMDConfigDebug::onClickCalibrate()
{
    gHMD.BeginManualCalibration();
}

void LLFloaterHMDConfigDebug::onClickResetValues()
{
    mInterpupillaryOffsetSliderCtrl->setValue(gHMD.getInterpupillaryOffsetDefault() * 1000.0f);
    onSetInterpupillaryOffset();
    mEyeToScreenSliderCtrl->setValue(gHMD.getEyeToScreenDistanceDefault() * 1000.0f);
    onSetEyeToScreenDistance();
    mMotionPredictionDeltaSliderCtrl->setValue(gHMD.getMotionPredictionDeltaDefault() * 1000.0f);
    onSetMotionPredictionDelta();
    mMotionPredictionCheckBoxCtrl->setValue(gHMD.useMotionPredictionDefault());
    onCheckMotionPrediction();
    mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndexDefault());
    onSetUIShapePreset();
}

void LLFloaterHMDConfigDebug::onClickCancel()
{
    // turn off panel and throw away values
    mInterpupillaryOffsetSliderCtrl->setValue(mInterpupillaryOffsetOriginal);
    onSetInterpupillaryOffset();
    mUIMagnificationSliderCtrl->setValue(mUIMagnificationOriginal);
    onSetUIMagnification();
    mUISurfaceOffsetDepthSliderCtrl->setValue(mUISurfaceOffsetDepthOriginal);
    onSetUISurfaceOffsetDepth();
    mUISurfaceToroidRadiusWidthSliderCtrl->setValue(mUISurfaceToroidRadiusWidthOriginal);
    onSetUISurfaceToroidRadiusWidth();
    mUISurfaceToroidRadiusDepthSliderCtrl->setValue(mUISurfaceToroidRadiusDepthOriginal);
    onSetUISurfaceToroidRadiusDepth();
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(mUISurfaceToroidCrossSectionRadiusWidthOriginal);
    onSetUISurfaceToroidCrossSectionRadiusWidth();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(mUISurfaceToroidCrossSectionRadiusHeightOriginal);
    onSetUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidArcHorizontalSliderCtrl->setValue(mUISurfaceToroidArcHorizontalOriginal);
    onSetUISurfaceToroidArcHorizontal();
    mUISurfaceToroidArcVerticalSliderCtrl->setValue(mUISurfaceToroidArcVerticalOriginal);
    onSetUISurfaceToroidArcVertical();
    mUISurfaceShapePresetSliderCtrl->setValue(mUISurfaceShapePresetOriginal);
    onSetUIShapePreset();
    mEyeToScreenSliderCtrl->setValue(mEyeToScreenDistanceOriginal);
    onSetEyeToScreenDistance();
    mMotionPredictionDeltaSliderCtrl->setValue(mMotionPredictionDeltaOriginal);
    onSetMotionPredictionDelta();
    mMotionPredictionCheckBoxCtrl->setValue(mMotionPredictionCheckedOriginal);
    onCheckMotionPrediction();
    mDirty = FALSE;

	closeFloater(false);
}

void LLFloaterHMDConfigDebug::onClickSave()
{
    gHMD.saveSettings();
    mDirty = FALSE;
	closeFloater(false);
}

void LLFloaterHMDConfigDebug::onSetInterpupillaryOffset()
{
    F32 f = mInterpupillaryOffsetSliderCtrl->getValueF32();
    gHMD.setInterpupillaryOffset(f / 1000.0f);
    updateInterpupillaryOffsetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateInterpupillaryOffsetLabel()
{
    std::ostringstream s;
    s << (gHMD.getInterpupillaryOffset() * 1000.0f) << " mm";
    mInterpupillaryOffsetAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUIMagnification()
{
    F32 f = mUIMagnificationSliderCtrl->getValueF32();
    gHMD.setUIMagnification(f);
    updateUIMagnificationLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUIMagnificationLabel()
{
    std::ostringstream s;
    s << gHMD.getUIMagnification();
    mUIMagnificationAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceOffsetDepth()
{
    F32 f = mUISurfaceOffsetDepthSliderCtrl->getValueF32();
    gHMD.setUISurfaceOffsetDepth(f);
    updateUISurfaceOffsetDepthLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceOffsetDepthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceOffsetDepth();
    mUISurfaceOffsetDepthAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUIShapePreset()
{
    F32 f = mUISurfaceShapePresetSliderCtrl->getValueF32();
    gHMD.setUIShapePresetIndex((S32)f);
    updateUIShapePresetLabel();

    mUISurfaceOffsetDepthSliderCtrl->setValue(gHMD.getUISurfaceOffsetDepth());
    updateUISurfaceOffsetDepthLabel();
    mUIMagnificationSliderCtrl->setValue(gHMD.getUIMagnification());
    updateUIMagnificationLabel();
    mUISurfaceToroidRadiusWidthSliderCtrl->setValue(gHMD.getUISurfaceToroidRadiusWidth());
    onSetUISurfaceToroidRadiusWidth();
    mUISurfaceToroidRadiusDepthSliderCtrl->setValue(gHMD.getUISurfaceToroidRadiusDepth());
    onSetUISurfaceToroidRadiusDepth();
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusWidth());
    onSetUISurfaceToroidCrossSectionRadiusWidth();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusHeight());
    onSetUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidArcHorizontalSliderCtrl->setValue(gHMD.getUISurfaceArcHorizontal() / F_PI);
    onSetUISurfaceToroidArcHorizontal();
    mUISurfaceToroidArcVerticalSliderCtrl->setValue(gHMD.getUISurfaceArcVertical() / F_PI);
    onSetUISurfaceToroidArcVertical();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUIShapePresetLabel()
{
    mUISurfaceShapePresetLabelCtrl->setValue(gHMD.getUIShapeName());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusWidth()
{
    F32 f = mUISurfaceToroidRadiusWidthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidRadiusWidth(f);
    updateUISurfaceToroidRadiusWidthLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidRadiusWidthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidRadiusWidth();
    mUISurfaceToroidRadiusWidthAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusDepth()
{
    F32 f = mUISurfaceToroidRadiusDepthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidRadiusDepth(f);
    updateUISurfaceToroidRadiusDepthLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidRadiusDepthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidRadiusDepth();
    mUISurfaceToroidRadiusDepthAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusWidth()
{
    F32 f = mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidCrossSectionRadiusWidth(f);
    updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidCrossSectionRadiusWidthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidCrossSectionRadiusWidth();
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusHeight()
{
    F32 f = mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidCrossSectionRadiusHeight(f);
    updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidCrossSectionRadiusHeightLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcHorizontal()
{
    F32 f = mUISurfaceToroidArcHorizontalSliderCtrl->getValueF32();
    gHMD.setUISurfaceArcHorizontal(f * F_PI);
    updateUISurfaceToroidArcHorizontalLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidArcHorizontalLabel()
{
    std::ostringstream s;
    s << (gHMD.getUISurfaceArcHorizontal() / F_PI) << " PI";
    mUISurfaceToroidArcHorizontalAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcVertical()
{
    F32 f = mUISurfaceToroidArcVerticalSliderCtrl->getValueF32();
    gHMD.setUISurfaceArcVertical(f * F_PI);
    updateUISurfaceToroidArcVerticalLabel();
    updateUIShapePresetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidArcVerticalLabel()
{
    std::ostringstream s;
    s << (gHMD.getUISurfaceArcVertical() / F_PI) << " PI";
    mUISurfaceToroidArcVerticalAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onSetEyeToScreenDistance()
{
    F32 f = mEyeToScreenSliderCtrl->getValueF32();
    gHMD.setEyeToScreenDistance(f / 1000.0f);
    updateEyeToScreenDistanceLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateEyeToScreenDistanceLabel()
{
    std::ostringstream s;
    s << (gHMD.getEyeToScreenDistance() * 1000.0f) << " mm";
    mEyeToScreenAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::onCheckMotionPrediction()
{
    BOOL checked = mMotionPredictionCheckBoxCtrl->get();
    gHMD.useMotionPrediction(checked);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onSetMotionPredictionDelta()
{
    F32 f = mMotionPredictionDeltaSliderCtrl->getValueF32();
    gHMD.setMotionPredictionDelta(f / 1000.0f);
    updateMotionPredictionDeltaLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateMotionPredictionDeltaLabel()
{
    std::ostringstream s;
    s << (gHMD.getMotionPredictionDelta() * 1000.0f) << " ms";
    mMotionPredictionDeltaAmountCtrl->setValue(s.str());
}

void LLFloaterHMDConfigDebug::updateDirty()
{
    LLVector2 cur[] = 
    {
        LLVector2(mInterpupillaryOffsetSliderCtrl->getValueF32(), mInterpupillaryOffsetOriginal),
        LLVector2(mUISurfaceOffsetDepthSliderCtrl->getValueF32(), mUISurfaceOffsetDepthOriginal),
        LLVector2(mUIMagnificationSliderCtrl->getValueF32(), mUIMagnificationOriginal),
        LLVector2(mUISurfaceShapePresetSliderCtrl->getValueF32(), mUISurfaceShapePresetOriginal),
        LLVector2(mUISurfaceToroidRadiusWidthSliderCtrl->getValueF32(), mUISurfaceToroidRadiusWidthOriginal),
        LLVector2(mUISurfaceToroidRadiusDepthSliderCtrl->getValueF32(), mUISurfaceToroidRadiusDepthOriginal),
        LLVector2(mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getValueF32(), mUISurfaceToroidCrossSectionRadiusWidthOriginal),
        LLVector2(mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getValueF32(), mUISurfaceToroidCrossSectionRadiusHeightOriginal),
        LLVector2(mUISurfaceToroidArcHorizontalSliderCtrl->getValueF32(), mUISurfaceToroidArcHorizontalOriginal),
        LLVector2(mUISurfaceToroidArcVerticalSliderCtrl->getValueF32(), mUISurfaceToroidArcVerticalOriginal),
        LLVector2(mEyeToScreenSliderCtrl->getValueF32(), mEyeToScreenDistanceOriginal),
        LLVector2(mMotionPredictionDeltaSliderCtrl->getValueF32(), mMotionPredictionDeltaOriginal),
    };
    U32 numVals = sizeof(cur) / sizeof(LLVector2);

    mDirty = FALSE;
    for (U32 i = 0; i < numVals; ++i)
    {
        mDirty = mDirty || (cur[i][0] != cur[i][1] ? TRUE : FALSE);
    }
    mDirty = mDirty || (mMotionPredictionCheckBoxCtrl->get() != mMotionPredictionCheckedOriginal);
}
