/** 
 * @file llpanelhmdconfig.cpp
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

#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llpanelhmdconfig.h"
#include "llfloaterreg.h"
#include "llhmd.h"

static LLRegisterPanelClassWrapper<LLPanelHMDConfig> t_panel_hmd_config("panel_hmd_config");
LLPanelHMDConfig* LLPanelHMDConfig::sInstance = NULL;

LLPanelHMDConfig::LLPanelHMDConfig()
    : mToggleViewButton(NULL)
    , mInterpupillaryOffsetSliderCtrl(NULL)
    , mInterpupillaryOffsetAmountCtrl(NULL)
    , mEyeToScreenSliderCtrl(NULL)
    , mEyeToScreenAmountCtrl(NULL)
    , mMotionPredictionCheckBoxCtrl(NULL)
    , mMotionPredictionDeltaSliderCtrl(NULL)
    , mMotionPredictionDeltaAmountCtrl(NULL)
    , mInterpupillaryOffsetOriginal(64.0f)
    , mEyeToScreenDistanceOriginal(41.0f)
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

{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfig.ToggleWorldView", boost::bind(&LLPanelHMDConfig::onClickToggleWorldView, this));
    mCommitCallbackRegistrar.add("HMDConfig.Calibrate", boost::bind(&LLPanelHMDConfig::onClickCalibrate, this));
    mCommitCallbackRegistrar.add("HMDConfig.ResetValues", boost::bind(&LLPanelHMDConfig::onClickResetValues, this));
    mCommitCallbackRegistrar.add("HMDConfig.Cancel", boost::bind(&LLPanelHMDConfig::onClickCancel, this));
    mCommitCallbackRegistrar.add("HMDConfig.Save", boost::bind(&LLPanelHMDConfig::onClickSave, this));

    mCommitCallbackRegistrar.add("HMDConfig.SetInterpupillaryOffset", boost::bind(&LLPanelHMDConfig::onSetInterpupillaryOffset, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetEyeToScreenDistance", boost::bind(&LLPanelHMDConfig::onSetEyeToScreenDistance, this));
    mCommitCallbackRegistrar.add("HMDConfig.CheckMotionPrediction", boost::bind(&LLPanelHMDConfig::onCheckMotionPrediction, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetMotionPredictionDelta", boost::bind(&LLPanelHMDConfig::onSetMotionPredictionDelta, this));

    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceOffsetDepth", boost::bind(&LLPanelHMDConfig::onSetUISurfaceOffsetDepth, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceToroidRadiusWidth", boost::bind(&LLPanelHMDConfig::onSetUISurfaceToroidRadiusWidth, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceToroidRadiusDepth", boost::bind(&LLPanelHMDConfig::onSetUISurfaceToroidRadiusDepth, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceToroidCrossSectionRadiusWidth", boost::bind(&LLPanelHMDConfig::onSetUISurfaceToroidCrossSectionRadiusWidth, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceToroidCrossSectionRadiusHeight", boost::bind(&LLPanelHMDConfig::onSetUISurfaceToroidCrossSectionRadiusHeight, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceToroidArcHorizontal", boost::bind(&LLPanelHMDConfig::onSetUISurfaceToroidArcHorizontal, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceToroidArcVertical", boost::bind(&LLPanelHMDConfig::onSetUISurfaceToroidArcVertical, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUIMagnification", boost::bind(&LLPanelHMDConfig::onSetUIMagnification, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUIShapePreset", boost::bind(&LLPanelHMDConfig::onSetUIShapePreset, this));
}

LLPanelHMDConfig::~LLPanelHMDConfig()
{
    sInstance = NULL;
}

//static
LLPanelHMDConfig* LLPanelHMDConfig::getInstance()
{
    if (!sInstance)
    {
        sInstance = new LLPanelHMDConfig();
    }
    return sInstance;
}

// static
void LLPanelHMDConfig::toggleVisibility()
{
    LLPanelHMDConfig* pPanel = LLPanelHMDConfig::getInstance();
    bool visible = pPanel->getVisible();

    //// turn off main view (other views e.g. tool tips, snapshot are still available)
    //LLUI::getRootView()->getChildView("menu_stack")->setVisible( visible );

    gHMD.shouldShowCalibrationUI( !visible );
    pPanel->setVisible( !visible );

    if (pPanel->getVisible())
    {
        if (pPanel->mToggleViewButton)
        {
            gHMD.shouldShowDepthVisual(TRUE);
            pPanel->onClickToggleWorldView();
        }
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
    }
    else
    {
        LLUI::getRootView()->getChildView("menu_stack")->setVisible(TRUE);
    }
}

BOOL LLPanelHMDConfig::postBuild()
{
    setVisible(FALSE);

    mToggleViewButton = getChild<LLButton>("hmd_toggle_world_view_button");
    mInterpupillaryOffsetSliderCtrl = getChild<LLSlider>("interpupillary_offset_slider");
    mInterpupillaryOffsetAmountCtrl = getChild<LLUICtrl>("interpupillary_offset_slider_amount");
    mEyeToScreenSliderCtrl = getChild<LLSlider>("eye_to_screen_distance_slider");
    mEyeToScreenAmountCtrl = getChild<LLUICtrl>("eye_to_screen_distance_slider_amount");
    mMotionPredictionCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_motion_prediction");
    mMotionPredictionDeltaSliderCtrl = getChild<LLSlider>("motion_prediction_delta_slider");
    mMotionPredictionDeltaAmountCtrl = getChild<LLUICtrl>("motion_prediction_delta_slider_amount");

    mUISurfaceOffsetDepthSliderCtrl = getChild<LLSlider>("uisurface_offset_depth_slider");
    mUISurfaceOffsetDepthAmountCtrl = getChild<LLUICtrl>("uisurface_offset_depth_slider_amount");
    mUISurfaceToroidRadiusWidthSliderCtrl = getChild<LLSlider>("uisurface_toroid_radius_width_slider");
    mUISurfaceToroidRadiusWidthAmountCtrl = getChild<LLUICtrl>("uisurface_toroid_radius_width_slider_amount");
    mUISurfaceToroidRadiusDepthSliderCtrl = getChild<LLSlider>("uisurface_toroid_radius_depth_slider");
    mUISurfaceToroidRadiusDepthAmountCtrl = getChild<LLUICtrl>("uisurface_toroid_radius_depth_slider_amount");
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl = getChild<LLSlider>("uisurface_toroid_cross_section_radius_width_slider");
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl = getChild<LLUICtrl>("uisurface_toroid_cross_section_radius_width_slider_amount");
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl = getChild<LLSlider>("uisurface_toroid_cross_section_radius_height_slider");
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl = getChild<LLUICtrl>("uisurface_toroid_cross_section_radius_height_slider_amount");
    mUISurfaceToroidArcHorizontalSliderCtrl = getChild<LLSlider>("uisurface_toroid_arc_horizontal_slider");
    mUISurfaceToroidArcHorizontalAmountCtrl = getChild<LLUICtrl>("uisurface_toroid_arc_horizontal_slider_amount");
    mUISurfaceToroidArcVerticalSliderCtrl = getChild<LLSlider>("uisurface_toroid_arc_vertical_slider");
    mUISurfaceToroidArcVerticalAmountCtrl = getChild<LLUICtrl>("uisurface_toroid_arc_vertical_slider_amount");
    mUIMagnificationSliderCtrl = getChild<LLSlider>("ui_magnification_slider");
    mUIMagnificationAmountCtrl = getChild<LLUICtrl>("ui_magnification_slider_amount");
    mUISurfaceShapePresetSliderCtrl = getChild<LLSlider>("uisurface_shape_preset_slider");
    mUISurfaceShapePresetLabelCtrl = getChild<LLUICtrl>("uisurface_shape_preset_value");

	return LLPanel::postBuild();
}

void LLPanelHMDConfig::draw()
{
    // maybe override not needed here - TODO: remove if not.
    LLPanel::draw();
}

void LLPanelHMDConfig::onClickToggleWorldView()
{
    LLPanelHMDConfig* pPanel = LLPanelHMDConfig::getInstance();
    if (pPanel && pPanel->getVisible())
    {
        gHMD.shouldShowDepthVisual(!gHMD.shouldShowDepthVisual());
        std::ostringstream s;
        if (gHMD.shouldShowDepthVisual())
        {
            s << "View World"; 
            LLUI::getRootView()->getChildView("menu_stack")->setVisible(FALSE);
        }
        else
        {
            s << "View EyeTest";
            LLUI::getRootView()->getChildView("menu_stack")->setVisible(!gHMD.isHMDMode() || gHMD.isCalibrated());
        }
        pPanel->mToggleViewButton->setLabel(s.str());
    }
    else
    {
        gHMD.shouldShowDepthVisual(FALSE);
        LLUI::getRootView()->getChildView("menu_stack")->setVisible(TRUE);
    }
}

void LLPanelHMDConfig::onClickCalibrate()
{
    gHMD.BeginManualCalibration();
}

void LLPanelHMDConfig::onClickResetValues()
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

void LLPanelHMDConfig::onClickCancel()
{
    // turn off panel and throw away values
    mInterpupillaryOffsetSliderCtrl->setValue(mInterpupillaryOffsetOriginal);
    onSetInterpupillaryOffset();
    mEyeToScreenSliderCtrl->setValue(mEyeToScreenDistanceOriginal);
    onSetEyeToScreenDistance();
    mMotionPredictionDeltaSliderCtrl->setValue(mMotionPredictionDeltaOriginal);
    onSetMotionPredictionDelta();
    mMotionPredictionCheckBoxCtrl->setValue(mMotionPredictionCheckedOriginal);
    onCheckMotionPrediction();
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
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(mUISurfaceToroidCrossSectionRadiusHeightOriginal);
    onSetUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidArcHorizontalSliderCtrl->setValue(mUISurfaceToroidArcHorizontalOriginal);
    onSetUISurfaceToroidArcHorizontal();
    mUISurfaceToroidArcVerticalSliderCtrl->setValue(mUISurfaceToroidArcVerticalOriginal);
    onSetUISurfaceToroidArcVertical();
    mUIMagnificationSliderCtrl->setValue(mUIMagnificationOriginal);
    onSetUIMagnification();
    mUISurfaceShapePresetSliderCtrl->setValue(mUISurfaceShapePresetOriginal);
    onSetUIShapePreset();

    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onClickSave()
{
    gHMD.saveSettings();
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onSetInterpupillaryOffset()
{
    F32 f = mInterpupillaryOffsetSliderCtrl->getValueF32();
    gHMD.setInterpupillaryOffset(f / 1000.0f);
    updateInterpupillaryOffsetLabel();
}

void LLPanelHMDConfig::updateInterpupillaryOffsetLabel()
{
    std::ostringstream s;
    s << (gHMD.getInterpupillaryOffset() * 1000.0f) << " mm";
    mInterpupillaryOffsetAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetEyeToScreenDistance()
{
    F32 f = mEyeToScreenSliderCtrl->getValueF32();
    gHMD.setEyeToScreenDistance(f / 1000.0f);
    updateEyeToScreenDistanceLabel();
}

void LLPanelHMDConfig::updateEyeToScreenDistanceLabel()
{
    std::ostringstream s;
    s << (gHMD.getEyeToScreenDistance() * 1000.0f) << " mm";
    mEyeToScreenAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onCheckMotionPrediction()
{
    BOOL checked = mMotionPredictionCheckBoxCtrl->get();
    gHMD.useMotionPrediction(checked);
}

void LLPanelHMDConfig::onSetMotionPredictionDelta()
{
    F32 f = mMotionPredictionDeltaSliderCtrl->getValueF32();
    gHMD.setMotionPredictionDelta(f / 1000.0f);
    updateMotionPredictionDeltaLabel();
}

void LLPanelHMDConfig::updateMotionPredictionDeltaLabel()
{
    std::ostringstream s;
    s << (gHMD.getMotionPredictionDelta() * 1000.0f) << " ms";
    mMotionPredictionDeltaAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceOffsetDepth()
{
    F32 f = mUISurfaceOffsetDepthSliderCtrl->getValueF32();
    gHMD.setUISurfaceOffsetDepth(f);
    updateUISurfaceOffsetDepthLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceOffsetDepthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceOffsetDepth();
    mUISurfaceOffsetDepthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidRadiusWidth()
{
    F32 f = mUISurfaceToroidRadiusWidthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidRadiusWidth(f);
    updateUISurfaceToroidRadiusWidthLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceToroidRadiusWidthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidRadiusWidth();
    mUISurfaceToroidRadiusWidthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidRadiusDepth()
{
    F32 f = mUISurfaceToroidRadiusDepthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidRadiusDepth(f);
    updateUISurfaceToroidRadiusDepthLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceToroidRadiusDepthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidRadiusDepth();
    mUISurfaceToroidRadiusDepthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidCrossSectionRadiusWidth()
{
    F32 f = mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidCrossSectionRadiusWidth(f);
    updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceToroidCrossSectionRadiusWidthLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidCrossSectionRadiusWidth();
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidCrossSectionRadiusHeight()
{
    F32 f = mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidCrossSectionRadiusHeight(f);
    updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceToroidCrossSectionRadiusHeightLabel()
{
    std::ostringstream s;
    s << gHMD.getUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidArcHorizontal()
{
    F32 f = mUISurfaceToroidArcHorizontalSliderCtrl->getValueF32();
    gHMD.setUISurfaceArcHorizontal(f * F_PI);
    updateUISurfaceToroidArcHorizontalLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceToroidArcHorizontalLabel()
{
    std::ostringstream s;
    s << (gHMD.getUISurfaceArcHorizontal() / F_PI) << " PI";
    mUISurfaceToroidArcHorizontalAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidArcVertical()
{
    F32 f = mUISurfaceToroidArcVerticalSliderCtrl->getValueF32();
    gHMD.setUISurfaceArcVertical(f * F_PI);
    updateUISurfaceToroidArcVerticalLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUISurfaceToroidArcVerticalLabel()
{
    std::ostringstream s;
    s << (gHMD.getUISurfaceArcVertical() / F_PI) << " PI";
    mUISurfaceToroidArcVerticalAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUIMagnification()
{
    F32 f = mUIMagnificationSliderCtrl->getValueF32();
    gHMD.setUIMagnification(f);
    updateUIMagnificationLabel();
    updateUIShapePresetLabel();
}

void LLPanelHMDConfig::updateUIMagnificationLabel()
{
    std::ostringstream s;
    s << gHMD.getUIMagnification();
    mUIMagnificationAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUIShapePreset()
{
    F32 f = mUISurfaceShapePresetSliderCtrl->getValueF32();
    gHMD.setUIShapePresetIndex((S32)f);
    updateUIShapePresetLabel();

    mUISurfaceOffsetDepthSliderCtrl->setValue(gHMD.getUISurfaceOffsetDepth());
    updateUISurfaceOffsetDepthLabel();
    mUISurfaceToroidRadiusWidthSliderCtrl->setValue(gHMD.getUISurfaceToroidRadiusWidth());
    updateUISurfaceToroidRadiusWidthLabel();
    mUISurfaceToroidRadiusDepthSliderCtrl->setValue(gHMD.getUISurfaceToroidRadiusDepth());
    updateUISurfaceToroidRadiusDepthLabel();
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusWidth());
    updateUISurfaceToroidCrossSectionRadiusWidthLabel();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusHeight());
    updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusHeight());
    updateUISurfaceToroidCrossSectionRadiusHeightLabel();
    mUISurfaceToroidArcHorizontalSliderCtrl->setValue(gHMD.getUISurfaceArcHorizontal() / F_PI);
    updateUISurfaceToroidArcHorizontalLabel();
    mUISurfaceToroidArcVerticalSliderCtrl->setValue(gHMD.getUISurfaceArcVertical() / F_PI);
    updateUISurfaceToroidArcVerticalLabel();
    mUIMagnificationSliderCtrl->setValue(gHMD.getUIMagnification());
    updateUIMagnificationLabel();
}

void LLPanelHMDConfig::updateUIShapePresetLabel()
{
    mUISurfaceShapePresetLabelCtrl->setValue(gHMD.getUIShapeName());
}
