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

static LLRegisterPanelClassWrapper<LLPanelHMDConfig> t_panel_hmd_config("panel_hmd_config");
LLPanelHMDConfig* LLPanelHMDConfig::sInstance = NULL;

LLPanelHMDConfig::LLPanelHMDConfig()
{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfig.Calibrate", boost::bind(&LLPanelHMDConfig::onClickCalibrate, this));
    mCommitCallbackRegistrar.add("HMDConfig.Cancel", boost::bind(&LLPanelHMDConfig::onClickCancel, this));
    mCommitCallbackRegistrar.add("HMDConfig.Save", boost::bind(&LLPanelHMDConfig::onClickSave, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetEyeToScreenDistance", boost::bind(&LLPanelHMDConfig::onSetEyeToScreenDistance, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetInterpupillaryOffset", boost::bind(&LLPanelHMDConfig::onSetInterpupillaryOffset, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetLensSeparationDistance", boost::bind(&LLPanelHMDConfig::onSetLensSeparationDistance, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetMotionPrediction", boost::bind(&LLPanelHMDConfig::onSetMotionPrediction, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetVerticalFOV", boost::bind(&LLPanelHMDConfig::onSetVerticalFOV, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetXCenterOffset", boost::bind(&LLPanelHMDConfig::onSetXCenterOffset, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetYCenterOffset", boost::bind(&LLPanelHMDConfig::onSetYCenterOffset, this));
}

LLPanelHMDConfig::~LLPanelHMDConfig()
{
    delete sInstance;
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
    bool visible = LLPanelHMDConfig::getInstance()->getVisible();

    // turn off main view (other views e.g. tool tips, snapshot are still available)
    LLUI::getRootView()->getChildView("menu_stack")->setVisible( visible );

    LLPanelHMDConfig::getInstance()->setVisible( ! visible );
}

BOOL LLPanelHMDConfig::postBuild()
{
    setVisible(FALSE);

    mInterpupillaryOffsetSliderCtrl = getChild<LLSlider>("interpupillary_offset_slider");
    mLensSeparationDistanceSliderCtrl = getChild<LLSlider>("lens_separation_distance_slider");
    mEyeToScreenSliderCtrl = getChild<LLSlider>("eye_to_screen_distance_slider");
    mVerticalFOVSliderCtrl = getChild<LLSlider>("vertical_fov_slider");
    mXCenterOffsetSliderCtrl = getChild<LLSlider>("x_center_offset_slider");
    mYCenterOffsetSliderCtrl = getChild<LLSlider>("y_center_offset_slider");

    mMotionPredictionCheckBoxCtrl = getChild<LLCheckBox>("hmd_motion_prediction");

	return LLPanel::postBuild();
}

void LLPanelHMDConfig::draw()
{
    LLPanel::draw();
}

void LLPanelHMDConfig::onClickCalibrate()
{

}

void LLPanelHMDConfig::onClickCancel()
{
    // add code to restore current settings if appropriate

    // turn off panel
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onClickSave()
{
    // add code to save current settings if appropriate

    // turn off panel
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onSetEyeToScreenDistance()
{
    llinfos << "Eye To Screen Distance changed to  " << mEyeToScreenSliderCtrl->getValueF32() << llendl;
}

void LLPanelHMDConfig::onSetInterpupillaryOffset()
{
    llinfos << "Interpupillary Offset changed to  " << mInterpupillaryOffsetSliderCtrl->getValueF32() << llendl;
}

void LLPanelHMDConfig::onSetLensSeparationDistance()
{
    llinfos << "Lens Separation Distance changed to  " << mLensSeparationDistanceSliderCtrl->getValueF32() << llendl;
}

void LLPanelHMDConfig::onSetVerticalFOV()
{
    llinfos << "Vertical FOV changed to  " << mVerticalFOVSliderCtrl->getValueF32() << llendl;
}

void LLPanelHMDConfig::onSetXCenterOffset()
{
    llinfos << "X Center Offset changed to  " << mXCenterOffsetSliderCtrl->getValueF32() << llendl;
}

void LLPanelHMDConfig::onSetYCenterOffset()
{
    llinfos << "Y Center Offset changed to  " << mYCenterOffsetSliderCtrl->getValueF32() << llendl;
}

void LLPanelHMDConfig::onSetMotionPrediction()
{
    llinfos << "Motion Prediction changed to  " << mMotionPredictionCheckBoxCtrl->get() << llendl;
}
