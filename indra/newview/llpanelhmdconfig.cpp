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
    : mInterpupillaryOffsetSliderCtrl(NULL)
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
{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfig.SetInterpupillaryOffset", boost::bind(&LLPanelHMDConfig::onSetInterpupillaryOffset, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetEyeToScreenDistance", boost::bind(&LLPanelHMDConfig::onSetEyeToScreenDistance, this));
    mCommitCallbackRegistrar.add("HMDConfig.CheckMotionPrediction", boost::bind(&LLPanelHMDConfig::onCheckMotionPrediction, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetMotionPredictionDelta", boost::bind(&LLPanelHMDConfig::onSetMotionPredictionDelta, this));
    mCommitCallbackRegistrar.add("HMDConfig.Calibrate", boost::bind(&LLPanelHMDConfig::onClickCalibrate, this));
    mCommitCallbackRegistrar.add("HMDConfig.ResetValues", boost::bind(&LLPanelHMDConfig::onClickResetValues, this));
    mCommitCallbackRegistrar.add("HMDConfig.Cancel", boost::bind(&LLPanelHMDConfig::onClickCancel, this));
    mCommitCallbackRegistrar.add("HMDConfig.Save", boost::bind(&LLPanelHMDConfig::onClickSave, this));
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

    // turn off main view (other views e.g. tool tips, snapshot are still available)
    LLUI::getRootView()->getChildView("menu_stack")->setVisible( visible );

    gHMD.shouldShowDepthUI( !visible );
    pPanel->setVisible( !visible );

    if (pPanel->getVisible())
    {
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
    }
}

BOOL LLPanelHMDConfig::postBuild()
{
    setVisible(FALSE);

    mInterpupillaryOffsetSliderCtrl = getChild<LLSlider>("interpupillary_offset_slider");
    mInterpupillaryOffsetAmountCtrl = getChild<LLUICtrl>("interpupillary_offset_slider_amount");
    mEyeToScreenSliderCtrl = getChild<LLSlider>("eye_to_screen_distance_slider");
    mEyeToScreenAmountCtrl = getChild<LLUICtrl>("eye_to_screen_distance_slider_amount");
    mMotionPredictionCheckBoxCtrl = getChild<LLCheckBoxCtrl>("hmd_motion_prediction");
    mMotionPredictionDeltaSliderCtrl = getChild<LLSlider>("motion_prediction_delta_slider");
    mMotionPredictionDeltaAmountCtrl = getChild<LLUICtrl>("motion_prediction_delta_slider_amount");

	return LLPanel::postBuild();
}

void LLPanelHMDConfig::draw()
{
    // maybe override not needed here - TODO: remove if not.
    LLPanel::draw();
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
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onClickSave()
{
    // turn off panel - all values are saved already
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
