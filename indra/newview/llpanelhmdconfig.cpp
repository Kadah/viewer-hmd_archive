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
}

LLPanelHMDConfig::~LLPanelHMDConfig()
{
    sInstance = NULL;
}

//static
LLPanelHMDConfig* LLPanelHMDConfig::getInstance()
{
    if (!sInstance) 
        sInstance = new LLPanelHMDConfig();

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
            pPanel->onSetInterpupillaryOffset();
        }
        if (pPanel->mEyeToScreenSliderCtrl)
        {
            pPanel->mEyeToScreenDistanceOriginal = gHMD.getEyeToScreenDistance() * 1000.0f;
            pPanel->mEyeToScreenSliderCtrl->setValue(pPanel->mEyeToScreenDistanceOriginal);
            pPanel->onSetEyeToScreenDistance();
        }
        if (pPanel->mMotionPredictionDeltaSliderCtrl)
        {
            pPanel->mMotionPredictionDeltaOriginal = gHMD.getMotionPredictionDelta() * 1000.0f;
            pPanel->mMotionPredictionDeltaSliderCtrl->setValue(pPanel->mMotionPredictionDeltaOriginal);
            pPanel->onSetMotionPredictionDelta();
        }
        if (pPanel->mMotionPredictionCheckBoxCtrl)
        {
            pPanel->mMotionPredictionCheckedOriginal = gHMD.useMotionPrediction();
            pPanel->mMotionPredictionCheckBoxCtrl->setValue(pPanel->mMotionPredictionCheckedOriginal);
            pPanel->onCheckMotionPrediction();
        }

        if (pPanel->mUISurfaceOffsetDepthSliderCtrl)
        {
            pPanel->mUISurfaceOffsetDepthOriginal = gHMD.getUISurfaceOffsets()[VZ];
            pPanel->mUISurfaceOffsetDepthSliderCtrl->setValue(pPanel->mUISurfaceOffsetDepthOriginal);
            pPanel->onSetUISurfaceOffsetDepth();
        }
        if (pPanel->mUISurfaceToroidRadiusWidthSliderCtrl)
        {
            pPanel->mUISurfaceToroidRadiusWidthOriginal = gHMD.getUISurfaceToroidRadiusWidth();
            pPanel->mUISurfaceToroidRadiusWidthSliderCtrl->setValue(pPanel->mUISurfaceToroidRadiusWidthOriginal);
            pPanel->onSetUISurfaceToroidRadiusWidth();
        }
        if (pPanel->mUISurfaceToroidRadiusDepthSliderCtrl)
        {
            pPanel->mUISurfaceToroidRadiusDepthOriginal = gHMD.getUISurfaceToroidRadiusDepth();
            pPanel->mUISurfaceToroidRadiusDepthSliderCtrl->setValue(pPanel->mUISurfaceToroidRadiusDepthOriginal);
            pPanel->onSetUISurfaceToroidRadiusDepth();
        }
        if (pPanel->mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl)
        {
            pPanel->mUISurfaceToroidCrossSectionRadiusWidthOriginal = gHMD.getUISurfaceToroidCrossSectionRadiusWidth();
            pPanel->mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(pPanel->mUISurfaceToroidCrossSectionRadiusWidthOriginal);
            pPanel->onSetUISurfaceToroidCrossSectionRadiusWidth();
        }
        if (pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl)
        {
            pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal = gHMD.getUISurfaceToroidCrossSectionRadiusHeight();
            pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal);
            pPanel->onSetUISurfaceToroidCrossSectionRadiusHeight();
        }
        if (pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl)
        {
            pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal = gHMD.getUISurfaceToroidCrossSectionRadiusHeight();
            pPanel->mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(pPanel->mUISurfaceToroidCrossSectionRadiusHeightOriginal);
            pPanel->onSetUISurfaceToroidCrossSectionRadiusHeight();
        }
        if (pPanel->mUISurfaceToroidArcHorizontalSliderCtrl)
        {
            pPanel->mUISurfaceToroidArcHorizontalOriginal = gHMD.getUISurfaceArcHorizontal() / F_PI;
            pPanel->mUISurfaceToroidArcHorizontalSliderCtrl->setValue(pPanel->mUISurfaceToroidArcHorizontalOriginal);
            pPanel->onSetUISurfaceToroidArcHorizontal();
        }
        if (pPanel->mUISurfaceToroidArcVerticalSliderCtrl)
        {
            pPanel->mUISurfaceToroidArcVerticalOriginal = gHMD.getUISurfaceArcVertical() / F_PI;
            pPanel->mUISurfaceToroidArcVerticalSliderCtrl->setValue(pPanel->mUISurfaceToroidArcVerticalOriginal);
            pPanel->onSetUISurfaceToroidArcVertical();
        }
        if (pPanel->mUIMagnificationSliderCtrl)
        {
            pPanel->mUIMagnificationOriginal = gHMD.getUIMagnification();
            pPanel->mUIMagnificationSliderCtrl->setValue(pPanel->mUIMagnificationOriginal);
            pPanel->onSetUIMagnification();
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
            LLUI::getRootView()->getChildView("menu_stack")->setVisible(!gHMD.shouldRender() || gHMD.isCalibrated());
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

    gHMD.onChangeUISurfaceSavedParams();
    mUISurfaceOffsetDepthSliderCtrl->setValue(gHMD.getUISurfaceOffsets()[VZ]);
    onSetUISurfaceOffsetDepth();
    mUISurfaceToroidRadiusWidthSliderCtrl->setValue(gHMD.getUISurfaceToroidRadiusWidth());
    onSetUISurfaceToroidRadiusWidth();
    mUISurfaceToroidRadiusDepthSliderCtrl->setValue(gHMD.getUISurfaceToroidRadiusDepth());
    onSetUISurfaceToroidRadiusDepth();
    mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusWidth());
    onSetUISurfaceToroidCrossSectionRadiusWidth();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusHeight());
    onSetUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->setValue(gHMD.getUISurfaceToroidCrossSectionRadiusHeight());
    onSetUISurfaceToroidCrossSectionRadiusHeight();
    mUISurfaceToroidArcHorizontalSliderCtrl->setValue(gHMD.getUISurfaceArcHorizontal() / F_PI);
    onSetUISurfaceToroidArcHorizontal();
    mUISurfaceToroidArcVerticalSliderCtrl->setValue(gHMD.getUISurfaceArcVertical() / F_PI);
    onSetUISurfaceToroidArcVertical();
    gHMD.onChangeUIMagnification();
    mUIMagnificationSliderCtrl->setValue(gHMD.getUIMagnification());
    onSetUIMagnification();
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
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onClickSave()
{
    // turn off panel - all values are saved already
    gHMD.saveSettings();
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onSetInterpupillaryOffset()
{
    F32 f = mInterpupillaryOffsetSliderCtrl->getValueF32();
    gHMD.setInterpupillaryOffset(f / 1000.0f);
    std::ostringstream s;
    s << f << " mm";
    mInterpupillaryOffsetAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetEyeToScreenDistance()
{
    F32 f = mEyeToScreenSliderCtrl->getValueF32();
    gHMD.setEyeToScreenDistance(f / 1000.0f);
    std::ostringstream s;
    s << f << " mm";
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
    std::ostringstream s;
    s << f << " ms";
    mMotionPredictionDeltaAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceOffsetDepth()
{
    F32 f = mUISurfaceOffsetDepthSliderCtrl->getValueF32();
    gHMD.setUISurfaceOffsetDepth(f);
    std::ostringstream s;
    s << f;
    mUISurfaceOffsetDepthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidRadiusWidth()
{
    F32 f = mUISurfaceToroidRadiusWidthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidRadiusWidth(f);
    std::ostringstream s;
    s << f;
    mUISurfaceToroidRadiusWidthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidRadiusDepth()
{
    F32 f = mUISurfaceToroidRadiusDepthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidRadiusDepth(f);
    std::ostringstream s;
    s << f;
    mUISurfaceToroidRadiusDepthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidCrossSectionRadiusWidth()
{
    F32 f = mUISurfaceToroidCrossSectionRadiusWidthSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidCrossSectionRadiusWidth(f);
    std::ostringstream s;
    s << f;
    mUISurfaceToroidCrossSectionRadiusWidthAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidCrossSectionRadiusHeight()
{
    F32 f = mUISurfaceToroidCrossSectionRadiusHeightSliderCtrl->getValueF32();
    gHMD.setUISurfaceToroidCrossSectionRadiusHeight(f);
    std::ostringstream s;
    s << f;
    mUISurfaceToroidCrossSectionRadiusHeightAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidArcHorizontal()
{
    F32 f = mUISurfaceToroidArcHorizontalSliderCtrl->getValueF32();
    gHMD.setUISurfaceArcHorizontal(f * F_PI);
    std::ostringstream s;
    s << f << " PI";
    mUISurfaceToroidArcHorizontalAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUISurfaceToroidArcVertical()
{
    F32 f = mUISurfaceToroidArcVerticalSliderCtrl->getValueF32();
    gHMD.setUISurfaceArcVertical(f * F_PI);
    std::ostringstream s;
    s << f << " PI";
    mUISurfaceToroidArcVerticalAmountCtrl->setValue(s.str());
}

void LLPanelHMDConfig::onSetUIMagnification()
{
    F32 f = mUIMagnificationSliderCtrl->getValueF32();
    gHMD.setUIMagnification(f);
    std::ostringstream s;
    s << f;
    mUIMagnificationAmountCtrl->setValue(s.str());
}
