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
#include "lltrans.h"


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

    mCommitCallbackRegistrar.add("HMDConfigDebug.AddPreset", boost::bind(&LLFloaterHMDConfigDebug::onClickAddPreset, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.RemovePreset", boost::bind(&LLFloaterHMDConfigDebug::onClickRemovePreset, this));
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
        pPanel->mUISurfaceShapePresetSliderCtrl->setMaxValue((F32)llmax(0, gHMD.getNumUIShapePresets() - 1));
        pPanel->mUISurfaceShapePresetSliderCtrl->setValue(pPanel->mUISurfaceShapePresetOriginal);
        pPanel->updateUIShapePresetLabel(TRUE);
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
}

void LLFloaterHMDConfigDebug::onClickAddPreset()
{
    if (!gHMD.addPreset())
    {
        return;
    }
    mUISurfaceShapePresetSliderCtrl->setMaxValue((F32)gHMD.getNumUIShapePresets());
    mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndex());
    onSetUIShapePreset();
}

void LLFloaterHMDConfigDebug::onClickRemovePreset()
{
    if (!gHMD.removePreset((S32)mUISurfaceShapePresetSliderCtrl->getValue()))
    {
        return;
    }
    mUISurfaceShapePresetSliderCtrl->setMaxValue((F32)gHMD.getNumUIShapePresets());
    mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndex());
    onSetUIShapePreset();
}

void LLFloaterHMDConfigDebug::onClickResetValues()
{
    mInterpupillaryOffsetSliderCtrl->setValue(llround(gHMD.getInterpupillaryOffsetDefault() * 1000.0f, mInterpupillaryOffsetSliderCtrl->getIncrement()));
    onSetInterpupillaryOffset();
    mEyeToScreenSliderCtrl->setValue(llround(gHMD.getEyeToScreenDistanceDefault() * 1000.0f, mEyeToScreenSliderCtrl->getIncrement()));
    onSetEyeToScreenDistance();
    mMotionPredictionDeltaSliderCtrl->setValue(llround(gHMD.getMotionPredictionDeltaDefault() * 1000.0f, mMotionPredictionDeltaSliderCtrl->getIncrement()));
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
    F32 f = llround(mInterpupillaryOffsetSliderCtrl->getValueF32(), mInterpupillaryOffsetSliderCtrl->getIncrement());
    F32 newVal = llround(f / 1000.0f, mInterpupillaryOffsetSliderCtrl->getIncrement() / 1000.0f);
    gHMD.setInterpupillaryOffset(newVal);
    updateInterpupillaryOffsetLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateInterpupillaryOffsetLabel()
{
	LLStringUtil::format_map_t args;
	args["[VAL]"] = llformat("%.1f", gHMD.getInterpupillaryOffset() * 1000.0f);
	mInterpupillaryOffsetAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsMillimeters", args));
}

void LLFloaterHMDConfigDebug::onSetUIMagnification()
{
    F32 f = llround(mUIMagnificationSliderCtrl->getValueF32(), mUIMagnificationSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUIMagnification(f);
    updateUIMagnificationLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUIMagnificationLabel()
{
    mUIMagnificationAmountCtrl->setValue(llformat("%.0f", gHMD.getUIMagnification()));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceOffsetDepth()
{
    F32 f = llround(mUISurfaceOffsetDepthSliderCtrl->getValueF32(), mUISurfaceOffsetDepthSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceOffsetDepth(f);
    updateUISurfaceOffsetDepthLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceOffsetDepthLabel()
{
    mUISurfaceOffsetDepthAmountCtrl->setValue(llformat("%.2f", gHMD.getUISurfaceOffsetDepth()));
}

void LLFloaterHMDConfigDebug::onSetUIShapePreset()
{
    S32 f = llround(mUISurfaceShapePresetSliderCtrl->getValueF32());
    U32 oldType = gHMD.getUIShapePresetType();
    if (f > llround(mUISurfaceShapePresetSliderCtrl->getMaxValue()))
    {
        mUISurfaceShapePresetSliderCtrl->setMaxValue((F32)f);
    }
    gHMD.setUIShapePresetIndex(f);
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());

    mUISurfaceOffsetDepthSliderCtrl->setValue(llround(gHMD.getUISurfaceOffsetDepth(), mUISurfaceOffsetDepthSliderCtrl->getIncrement()));
    updateUISurfaceOffsetDepthLabel();
    mUIMagnificationSliderCtrl->setValue(llround(gHMD.getUIMagnification(), mUIMagnificationSliderCtrl->getIncrement()));
    updateUIMagnificationLabel();
    mUISurfaceToroidRadiusWidthSliderCtrl->setValue(llround(gHMD.getUISurfaceToroidRadiusWidth(), mUISurfaceToroidRadiusWidthSliderCtrl->getIncrement()));
    updateUISurfaceToroidRadiusWidthLabel();
    mUISurfaceToroidRadiusDepthSliderCtrl->setValue(llround(gHMD.getUISurfaceToroidRadiusDepth(), mUISurfaceToroidRadiusDepthSliderCtrl->getIncrement()));
    updateUISurfaceToroidRadiusDepthLabel();
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(llround(gHMD.getUISurfaceToroidCrossSectionRadiusWidth(), mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getIncrement()));
    updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(llround(gHMD.getUISurfaceToroidCrossSectionRadiusHeight(), mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getIncrement()));
    updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    mUISurfaceToroidArcHorizontalSliderCtrl->setValue(llround(gHMD.getUISurfaceArcHorizontal() / F_PI, mUISurfaceToroidArcHorizontalSliderCtrl->getIncrement()));
    updateUISurfaceToroidArcHorizontalLabel();
    mUISurfaceToroidArcVerticalSliderCtrl->setValue(llround(gHMD.getUISurfaceArcVertical() / F_PI, mUISurfaceToroidArcVerticalSliderCtrl->getIncrement()));
    updateUISurfaceToroidArcVerticalLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUIShapePresetLabel(BOOL typeChanged)
{
    mUISurfaceShapePresetLabelCtrl->setValue(gHMD.getUIShapeName());
    // This method is called from a number of places since the preset index can be changed as a side effect of a number
    // of other values being modified.  So, to keep the slider in sync with the actual value, we update the slider value
    // here to match the real value.
    if (gHMD.getUIShapePresetIndex() != llround(mUISurfaceShapePresetSliderCtrl->getValueF32()))
    {
        mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndex(), TRUE);
    }
    if (typeChanged)
    {
        LLButton* buttonAdd = getChild<LLButton>("hmd_config_debug_hmd_add_preset");
        LLButton* buttonRemove = getChild<LLButton>("hmd_config_debug_hmd_remove_preset");
        switch (gHMD.getUIShapePresetType())
        {
        case LLHMD::kCustom:
            if (buttonAdd) buttonAdd->setEnabled(TRUE);
            if (buttonRemove) buttonRemove->setEnabled(FALSE);
            break;
        case LLHMD::kDefault:
            if (buttonAdd) buttonAdd->setEnabled(FALSE);
            if (buttonRemove) buttonRemove->setEnabled(FALSE);
            break;
        case LLHMD::kUser:
            if (buttonAdd) buttonAdd->setEnabled(FALSE);
            if (buttonRemove) buttonRemove->setEnabled(TRUE);
            break;
        }
    }
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusWidth()
{
    F32 f = llround(mUISurfaceToroidRadiusWidthSliderCtrl->getValueF32(), mUISurfaceToroidRadiusWidthSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceToroidRadiusWidth(f);
    updateUISurfaceToroidRadiusWidthLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidRadiusWidthLabel()
{
    mUISurfaceToroidRadiusWidthAmountCtrl->setValue(llformat("%.1f", gHMD.getUISurfaceToroidRadiusWidth()));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusDepth()
{
    F32 f = llround(mUISurfaceToroidRadiusDepthSliderCtrl->getValueF32(), mUISurfaceToroidRadiusDepthSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceToroidRadiusDepth(f);
    updateUISurfaceToroidRadiusDepthLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidRadiusDepthLabel()
{
    mUISurfaceToroidRadiusDepthAmountCtrl->setValue(llformat("%.1f", gHMD.getUISurfaceToroidRadiusDepth()));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusWidth()
{
    F32 f = llround(mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getValueF32(), mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceToroidCrossSectionRadiusWidth(f);
    updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidCrossSectionRadiusWidthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidCrossSectionRadiusWidth();
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl->setValue(llformat("%.1f", gHMD.getUISurfaceToroidCrossSectionRadiusWidth()));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusHeight()
{
    F32 f = llround(mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getValueF32(), mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceToroidCrossSectionRadiusHeight(f);
    updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidCrossSectionRadiusHeightLabel()
{
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl->setValue(llformat("%.1f", gHMD.getUISurfaceToroidCrossSectionRadiusHeight()));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcHorizontal()
{
    F32 f = llround(mUISurfaceToroidArcHorizontalSliderCtrl->getValueF32(), mUISurfaceToroidArcHorizontalSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceArcHorizontal(f * F_PI);
    updateUISurfaceToroidArcHorizontalLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidArcHorizontalLabel()
{
	LLStringUtil::format_map_t args;
	args["[VAL]"] = llformat("%.1f", gHMD.getUISurfaceArcHorizontal() / F_PI);
	mUISurfaceToroidArcHorizontalAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsRadians", args));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcVertical()
{
    F32 f = llround(mUISurfaceToroidArcVerticalSliderCtrl->getValueF32(), mUISurfaceToroidArcVerticalSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceArcVertical(f * F_PI);
    updateUISurfaceToroidArcVerticalLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceToroidArcVerticalLabel()
{
	LLStringUtil::format_map_t args;
	args["[VAL]"] = llformat("%.1f", gHMD.getUISurfaceArcVertical() / F_PI);
	mUISurfaceToroidArcVerticalAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsRadians", args));
}

void LLFloaterHMDConfigDebug::onSetEyeToScreenDistance()
{
    F32 f = llround(mEyeToScreenSliderCtrl->getValueF32(), mEyeToScreenSliderCtrl->getIncrement());
    gHMD.setEyeToScreenDistance(llround(f / 1000.0f, mEyeToScreenSliderCtrl->getIncrement() / 1000.0f));
    updateEyeToScreenDistanceLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateEyeToScreenDistanceLabel()
{
	LLStringUtil::format_map_t args;
	args["[VAL]"] = llformat("%.1f", gHMD.getEyeToScreenDistance() * 1000.0f);
	mEyeToScreenAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsMillimeters", args));
}

void LLFloaterHMDConfigDebug::onCheckMotionPrediction()
{
    BOOL checked = mMotionPredictionCheckBoxCtrl->get();
    gHMD.useMotionPrediction(checked);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onSetMotionPredictionDelta()
{
    F32 f = llround(mMotionPredictionDeltaSliderCtrl->getValueF32(), mMotionPredictionDeltaSliderCtrl->getIncrement());
    gHMD.setMotionPredictionDelta(llround(f / 1000.0f, mMotionPredictionDeltaSliderCtrl->getIncrement() / 1000.0f));
    updateMotionPredictionDeltaLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateMotionPredictionDeltaLabel()
{
	LLStringUtil::format_map_t args;
	args["[VAL]"] = llformat("%.0f", gHMD.getMotionPredictionDelta() * 1000.0f);
	mMotionPredictionDeltaAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsMilliseconds", args));
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
        mDirty = mDirty || !is_approx_equal(cur[i][0], cur[i][1]);
    }
    mDirty = mDirty || (mMotionPredictionCheckBoxCtrl->get() != mMotionPredictionCheckedOriginal);
}
