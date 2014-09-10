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
    , mUISurfaceOffsetDepthSliderCtrl(NULL)
    , mUISurfaceOffsetDepthAmountCtrl(NULL)
    , mUISurfaceOffsetDepthOriginal(0.0f)
    , mUISurfaceOffsetVerticalSliderCtrl(NULL)
    , mUISurfaceOffsetVerticalAmountCtrl(NULL)
    , mUISurfaceOffsetVerticalOriginal(0.0f)
    , mUISurfaceOffsetHorizontalSliderCtrl(NULL)
    , mUISurfaceOffsetHorizontalAmountCtrl(NULL)
    , mUISurfaceOffsetHorizontalOriginal(0.0f)
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
    , mLowPersistenceCheckBoxCtrl(NULL)
    , mLowPersistenceCheckedOriginal(TRUE)
    , mPLOCheckBoxCtrl(NULL)
    , mPLOCheckedOriginal(TRUE)
    , mMotionPredictionCheckBoxCtrl(NULL)
    , mMotionPredictionCheckedOriginal(TRUE)
    , mTimewarpCheckBoxCtrl(NULL)
    , mTimewarpCheckedOriginal(TRUE)
    , mTimewarpIntervalSliderCtrl(NULL)
    , mTimewarpIntervalAmountCtrl(NULL)
    , mTimewarpIntervalOriginal(0.01f)
    , mUseSRGBDistortionCheckBoxCtrl(NULL)
    , mUseSRGBDistortionCheckedOriginal(TRUE)
    , mMouselookYawOnlyCheckBoxCtrl(NULL)
    , mMouselookYawOnlyCheckedOriginal(TRUE)
    , mDirty(FALSE)
{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfigDebug.AddPreset", boost::bind(&LLFloaterHMDConfigDebug::onClickAddPreset, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.RemovePreset", boost::bind(&LLFloaterHMDConfigDebug::onClickRemovePreset, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.ResetValues", boost::bind(&LLFloaterHMDConfigDebug::onClickResetValues, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.Cancel", boost::bind(&LLFloaterHMDConfigDebug::onClickCancel, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.Save", boost::bind(&LLFloaterHMDConfigDebug::onClickSave, this));

    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceOffsetDepth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceOffsetDepth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceOffsetVertical", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceOffsetVertical, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceOffsetHorizontal", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceOffsetHorizontal, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUIMagnification", boost::bind(&LLFloaterHMDConfigDebug::onSetUIMagnification, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUIShapePreset", boost::bind(&LLFloaterHMDConfigDebug::onSetUIShapePreset, this));

    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidRadiusWidth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusWidth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidRadiusDepth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidRadiusDepth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidCrossSectionRadiusWidth", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusWidth, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidCrossSectionRadiusHeight", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidCrossSectionRadiusHeight, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidArcHorizontal", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcHorizontal, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetUISurfaceToroidArcVertical", boost::bind(&LLFloaterHMDConfigDebug::onSetUISurfaceToroidArcVertical, this));

    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckLowPersistence", boost::bind(&LLFloaterHMDConfigDebug::onCheckLowPersistence, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckPixelLuminanceOverdrive", boost::bind(&LLFloaterHMDConfigDebug::onCheckPixelLuminanceOverdrive, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckMotionPrediction", boost::bind(&LLFloaterHMDConfigDebug::onCheckMotionPrediction, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckTimewarp", boost::bind(&LLFloaterHMDConfigDebug::onCheckTimewarp, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.SetTimewarpInterval", boost::bind(&LLFloaterHMDConfigDebug::onSetTimewarpInterval, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckUseSRGBDistortion", boost::bind(&LLFloaterHMDConfigDebug::onCheckUseSRGBDistortion, this));
    mCommitCallbackRegistrar.add("HMDConfigDebug.CheckMouselookYawOnly", boost::bind(&LLFloaterHMDConfigDebug::onCheckMouselookYawOnly, this));
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

    mUIMagnificationSliderCtrl = getChild<LLSlider>("hmd_config_debug_ui_magnification_slider");
    mUIMagnificationAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_ui_magnification_slider_amount");
    mUISurfaceOffsetDepthSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_offset_depth_slider");
    mUISurfaceOffsetDepthAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_offset_depth_slider_amount");
    mUISurfaceOffsetVerticalSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_offset_vertical_slider");
    mUISurfaceOffsetVerticalAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_offset_vertical_slider_amount");
    mUISurfaceOffsetHorizontalSliderCtrl = getChild<LLSlider>("hmd_config_debug_uisurface_offset_horizontal_slider");
    mUISurfaceOffsetHorizontalAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_uisurface_offset_horizontal_slider_amount");
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
    mLowPersistenceCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_low_persistence");
    mPLOCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_pixel_luminance_overdrive");
    mMotionPredictionCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_motion_prediction");
    mTimewarpCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_timewarp");
    mTimewarpIntervalSliderCtrl = getChild<LLSlider>("hmd_config_debug_timewarp_interval_slider");
    mTimewarpIntervalAmountCtrl = getChild<LLUICtrl>("hmd_config_debug_timewarp_interval_slider_amount");
    mUseSRGBDistortionCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_use_srgb_distortion");
    mMouselookYawOnlyCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_config_debug_mouselook_yaw_only");

    return LLFloater::postBuild();
}

void LLFloaterHMDConfigDebug::onOpen(const LLSD& key)
{
    LLFloaterHMDConfigDebug* pPanel = LLFloaterHMDConfigDebug::getInstance();
    if (pPanel->mUISurfaceOffsetDepthSliderCtrl)
    {
        pPanel->mUISurfaceOffsetDepthOriginal = gHMD.getUISurfaceOffsetDepth();
        pPanel->mUISurfaceOffsetDepthSliderCtrl->setValue(pPanel->mUISurfaceOffsetDepthOriginal);
        pPanel->updateUISurfaceOffsetDepthLabel();
    }
    if (pPanel->mUISurfaceOffsetVerticalSliderCtrl)
    {
        pPanel->mUISurfaceOffsetVerticalOriginal = gHMD.getUISurfaceOffsetVertical();
        pPanel->mUISurfaceOffsetVerticalSliderCtrl->setValue(pPanel->mUISurfaceOffsetVerticalOriginal);
        pPanel->updateUISurfaceOffsetVerticalLabel();
    }
    if (pPanel->mUISurfaceOffsetHorizontalSliderCtrl)
    {
        pPanel->mUISurfaceOffsetHorizontalOriginal = gHMD.getUISurfaceOffsetHorizontal();
        pPanel->mUISurfaceOffsetHorizontalSliderCtrl->setValue(pPanel->mUISurfaceOffsetHorizontalOriginal);
        pPanel->updateUISurfaceOffsetHorizontalLabel();
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
    if (pPanel->mLowPersistenceCheckBoxCtrl)
    {
        pPanel->mLowPersistenceCheckedOriginal = gHMD.useLowPersistence();
        pPanel->mLowPersistenceCheckBoxCtrl->setValue(pPanel->mLowPersistenceCheckedOriginal);
    }
    if (pPanel->mPLOCheckBoxCtrl)
    {
        pPanel->mPLOCheckedOriginal = gHMD.usePixelLuminanceOverdrive();
        pPanel->mPLOCheckBoxCtrl->setValue(pPanel->mPLOCheckedOriginal);
    }
    if (pPanel->mMotionPredictionCheckBoxCtrl)
    {
        pPanel->mMotionPredictionCheckedOriginal = gHMD.useMotionPrediction();
        pPanel->mMotionPredictionCheckBoxCtrl->setValue(pPanel->mMotionPredictionCheckedOriginal);
    }
    if (pPanel->mTimewarpCheckBoxCtrl)
    {
        pPanel->mTimewarpCheckedOriginal = gHMD.isTimewarpEnabled();
        pPanel->mTimewarpCheckBoxCtrl->setValue(pPanel->mTimewarpCheckedOriginal);
    }
    if (pPanel->mTimewarpIntervalSliderCtrl)
    {
        pPanel->mTimewarpIntervalOriginal = gHMD.getTimewarpIntervalSeconds() * 1000.0f;
        pPanel->mTimewarpIntervalSliderCtrl->setValue(pPanel->mTimewarpCheckedOriginal);
        pPanel->updateTimewarpIntervalLabel();
    }
    if (pPanel->mUseSRGBDistortionCheckBoxCtrl)
    {
        pPanel->mUseSRGBDistortionCheckedOriginal = gHMD.useSRGBDistortion();
        pPanel->mUseSRGBDistortionCheckBoxCtrl->setValue(pPanel->mUseSRGBDistortionCheckedOriginal);
    }
    if (pPanel->mMouselookYawOnlyCheckBoxCtrl)
    {
        pPanel->mMouselookYawOnlyCheckedOriginal = gHMD.isMouselookYawOnly();
        pPanel->mMouselookYawOnlyCheckBoxCtrl->setValue(pPanel->mMouselookYawOnlyCheckedOriginal);
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
    mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndexDefault());
    onSetUIShapePreset();
    mLowPersistenceCheckBoxCtrl->setValue(gHMD.useLowPersistenceDefault());
    onCheckLowPersistence();
    mPLOCheckBoxCtrl->setValue(gHMD.usePixelLuminanceOverdriveDefault());
    onCheckPixelLuminanceOverdrive();
    mMotionPredictionCheckBoxCtrl->setValue(gHMD.useMotionPredictionDefault());
    onCheckMotionPrediction();
    mTimewarpCheckBoxCtrl->setValue(gHMD.isTimewarpEnabledDefault());
    onCheckTimewarp();
    mTimewarpIntervalSliderCtrl->setValue(gHMD.getTimewarpIntervalSecondsDefault());
    void onSetTimewarpInterval();
    mUseSRGBDistortionCheckBoxCtrl->setValue(gHMD.useSRGBDistortionDefault());
    onCheckUseSRGBDistortion();
    mMouselookYawOnlyCheckBoxCtrl->setValue(gHMD.isMouselookYawOnlyDefault());
    onCheckMouselookYawOnly();
}

void LLFloaterHMDConfigDebug::onClickCancel()
{
    // turn off panel and throw away values
    mUIMagnificationSliderCtrl->setValue(mUIMagnificationOriginal);
    onSetUIMagnification();
    mUISurfaceOffsetDepthSliderCtrl->setValue(mUISurfaceOffsetDepthOriginal);
    onSetUISurfaceOffsetDepth();
    mUISurfaceOffsetVerticalSliderCtrl->setValue(mUISurfaceOffsetVerticalOriginal);
    onSetUISurfaceOffsetVertical();
    mUISurfaceOffsetHorizontalSliderCtrl->setValue(mUISurfaceOffsetHorizontalOriginal);
    onSetUISurfaceOffsetHorizontal();
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
    mLowPersistenceCheckBoxCtrl->setValue(mLowPersistenceCheckedOriginal);
    onCheckLowPersistence();
    mPLOCheckBoxCtrl->setValue(mPLOCheckedOriginal);
    onCheckPixelLuminanceOverdrive();
    mMotionPredictionCheckBoxCtrl->setValue(mMotionPredictionCheckedOriginal);
    onCheckMotionPrediction();
    mTimewarpCheckBoxCtrl->setValue(mTimewarpCheckedOriginal);
    onCheckTimewarp();
    mTimewarpIntervalSliderCtrl->setValue(mTimewarpIntervalOriginal);
    onSetTimewarpInterval();
    mUseSRGBDistortionCheckBoxCtrl->setValue(mUseSRGBDistortionCheckedOriginal);
    onCheckUseSRGBDistortion();
    mMouselookYawOnlyCheckBoxCtrl->setValue(mMouselookYawOnlyCheckedOriginal);
    onCheckMouselookYawOnly();
    mDirty = FALSE;

	closeFloater(false);
}

void LLFloaterHMDConfigDebug::onClickSave()
{
    gHMD.saveSettings();
    mDirty = FALSE;
	closeFloater(false);
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

void LLFloaterHMDConfigDebug::onSetUISurfaceOffsetVertical()
{
    F32 f = llround(mUISurfaceOffsetVerticalSliderCtrl->getValueF32(), mUISurfaceOffsetVerticalSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceOffsetVertical(f);
    updateUISurfaceOffsetVerticalLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceOffsetVerticalLabel()
{
    mUISurfaceOffsetVerticalAmountCtrl->setValue(llformat("%.2f", gHMD.getUISurfaceOffsetVertical()));
}

void LLFloaterHMDConfigDebug::onSetUISurfaceOffsetHorizontal()
{
    F32 f = llround(mUISurfaceOffsetHorizontalSliderCtrl->getValueF32(), mUISurfaceOffsetHorizontalSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceOffsetHorizontal(f);
    updateUISurfaceOffsetHorizontalLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateUISurfaceOffsetHorizontalLabel()
{
    mUISurfaceOffsetHorizontalAmountCtrl->setValue(llformat("%.2f", gHMD.getUISurfaceOffsetHorizontal()));
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
    mUISurfaceOffsetVerticalSliderCtrl->setValue(llround(gHMD.getUISurfaceOffsetVertical(), mUISurfaceOffsetVerticalSliderCtrl->getIncrement()));
    updateUISurfaceOffsetVerticalLabel();
    mUISurfaceOffsetHorizontalSliderCtrl->setValue(llround(gHMD.getUISurfaceOffsetHorizontal(), mUISurfaceOffsetHorizontalSliderCtrl->getIncrement()));
    updateUISurfaceOffsetHorizontalLabel();
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
    mUISurfaceToroidRadiusWidthAmountCtrl->setValue(llformat("%.03f", gHMD.getUISurfaceToroidRadiusWidth()));
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
    mUISurfaceToroidRadiusDepthAmountCtrl->setValue(llformat("%.03f", gHMD.getUISurfaceToroidRadiusDepth()));
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
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl->setValue(llformat("%.03f", gHMD.getUISurfaceToroidCrossSectionRadiusWidth()));
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
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl->setValue(llformat("%.03f", gHMD.getUISurfaceToroidCrossSectionRadiusHeight()));
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
	args["[VAL]"] = llformat("%.03f", gHMD.getUISurfaceArcHorizontal() / F_PI);
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
	args["[VAL]"] = llformat("%.03f", gHMD.getUISurfaceArcVertical() / F_PI);
	mUISurfaceToroidArcVerticalAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsRadians", args));
}

void LLFloaterHMDConfigDebug::onCheckLowPersistence()
{
    BOOL checked = mLowPersistenceCheckBoxCtrl->get();
    gHMD.useLowPersistence(checked);
    gHMD.renderSettingsChanged(TRUE);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onCheckPixelLuminanceOverdrive()
{
    BOOL checked = mPLOCheckBoxCtrl->get();
    gHMD.usePixelLuminanceOverdrive(checked);
    gHMD.renderSettingsChanged(TRUE);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onCheckMotionPrediction()
{
    BOOL checked = mMotionPredictionCheckBoxCtrl->get();
    gHMD.useMotionPrediction(checked);
    gHMD.renderSettingsChanged(TRUE);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onCheckTimewarp()
{
    BOOL checked = mTimewarpCheckBoxCtrl->get();
    gHMD.isTimewarpEnabled(checked);
    gHMD.renderSettingsChanged(TRUE);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onSetTimewarpInterval()
{
    F32 f = llround(mTimewarpIntervalSliderCtrl->getValueF32(), mTimewarpIntervalSliderCtrl->getIncrement());
    gHMD.setTimewarpIntervalSeconds(f / 1000.0f);
    updateTimewarpIntervalLabel();
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateTimewarpIntervalLabel()
{
    LLStringUtil::format_map_t args;
    args["[VAL]"] = llformat("%.03f", gHMD.getTimewarpIntervalSeconds() * 1000.0f);
    mTimewarpIntervalAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsMilliseconds", args));
}

void LLFloaterHMDConfigDebug::onCheckUseSRGBDistortion()
{
    BOOL checked = mUseSRGBDistortionCheckBoxCtrl->get();
    gHMD.useSRGBDistortion(checked);
    gHMD.renderSettingsChanged(TRUE);
    updateDirty();
}

void LLFloaterHMDConfigDebug::onCheckMouselookYawOnly()
{
    BOOL checked = mMouselookYawOnlyCheckBoxCtrl->get();
    gHMD.isMouselookYawOnly(checked);
    updateDirty();
}

void LLFloaterHMDConfigDebug::updateDirty()
{
    LLVector2 cur[] = 
    {
        LLVector2(mUISurfaceOffsetDepthSliderCtrl->getValueF32(), mUISurfaceOffsetDepthOriginal),
        LLVector2(mUISurfaceOffsetVerticalSliderCtrl->getValueF32(), mUISurfaceOffsetVerticalOriginal),
        LLVector2(mUISurfaceOffsetHorizontalSliderCtrl->getValueF32(), mUISurfaceOffsetHorizontalOriginal),
        LLVector2(mUIMagnificationSliderCtrl->getValueF32(), mUIMagnificationOriginal),
        LLVector2(mUISurfaceShapePresetSliderCtrl->getValueF32(), mUISurfaceShapePresetOriginal),
        LLVector2(mUISurfaceToroidRadiusWidthSliderCtrl->getValueF32(), mUISurfaceToroidRadiusWidthOriginal),
        LLVector2(mUISurfaceToroidRadiusDepthSliderCtrl->getValueF32(), mUISurfaceToroidRadiusDepthOriginal),
        LLVector2(mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getValueF32(), mUISurfaceToroidCrossSectionRadiusWidthOriginal),
        LLVector2(mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getValueF32(), mUISurfaceToroidCrossSectionRadiusHeightOriginal),
        LLVector2(mUISurfaceToroidArcHorizontalSliderCtrl->getValueF32(), mUISurfaceToroidArcHorizontalOriginal),
        LLVector2(mUISurfaceToroidArcVerticalSliderCtrl->getValueF32(), mUISurfaceToroidArcVerticalOriginal),
        LLVector2(mTimewarpIntervalSliderCtrl->getValueF32(), mTimewarpIntervalOriginal),
    };
    U32 numVals = sizeof(cur) / sizeof(LLVector2);

    mDirty = FALSE;
    for (U32 i = 0; i < numVals; ++i)
    {
        mDirty = mDirty || !is_approx_equal(cur[i][0], cur[i][1]);
    }
    mDirty = mDirty || (mLowPersistenceCheckBoxCtrl->get() != mLowPersistenceCheckedOriginal);
    mDirty = mDirty || (mPLOCheckBoxCtrl->get() != mPLOCheckedOriginal);
    mDirty = mDirty || (mMotionPredictionCheckBoxCtrl->get() != mMotionPredictionCheckedOriginal);
    mDirty = mDirty || (mTimewarpCheckBoxCtrl->get() != mTimewarpCheckedOriginal);
    mDirty = mDirty || (mUseSRGBDistortionCheckBoxCtrl->get() != mUseSRGBDistortionCheckedOriginal);
    mDirty = mDirty || (mMouselookYawOnlyCheckBoxCtrl->get() != mMouselookYawOnlyCheckedOriginal);
}
