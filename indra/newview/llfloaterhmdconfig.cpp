/** 
 * @file llfloaterhmdconfig.cpp
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

#if 0

#include "llfloaterhmdconfig.h"
#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llfloaterreg.h"
#include "llhmd.h"
#include "lltrans.h"


LLFloaterHMDConfig* LLFloaterHMDConfig::sInstance = NULL;

LLFloaterHMDConfig::LLFloaterHMDConfig(const LLSD& key)
    : LLFloater(key)
    , mInterpupillaryOffsetSliderCtrl(NULL)
    , mInterpupillaryOffsetAmountCtrl(NULL)
    , mInterpupillaryOffsetOriginal(0.064f)
    , mUISurfaceOffsetDepthSliderCtrl(NULL)
    , mUISurfaceOffsetDepthAmountCtrl(NULL)
    , mUISurfaceOffsetDepthOriginal(0.0f)
    , mUIMagnificationSliderCtrl(NULL)
    , mUIMagnificationAmountCtrl(NULL)
    , mUIMagnificationOriginal(600.0f)
    , mUISurfaceShapePresetSliderCtrl(NULL)
    , mUISurfaceShapePresetLabelCtrl(NULL)
    , mUISurfaceShapePresetOriginal(0.0f)
    , mDirty(FALSE)
{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfig.ResetValues", boost::bind(&LLFloaterHMDConfig::onClickResetValues, this));
    mCommitCallbackRegistrar.add("HMDConfig.RemovePreset", boost::bind(&LLFloaterHMDConfig::onClickRemovePreset, this));
    mCommitCallbackRegistrar.add("HMDConfig.Cancel", boost::bind(&LLFloaterHMDConfig::onClickCancel, this));
    mCommitCallbackRegistrar.add("HMDConfig.Save", boost::bind(&LLFloaterHMDConfig::onClickSave, this));

    mCommitCallbackRegistrar.add("HMDConfig.SetInterpupillaryOffset", boost::bind(&LLFloaterHMDConfig::onSetInterpupillaryOffset, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUISurfaceOffsetDepth", boost::bind(&LLFloaterHMDConfig::onSetUISurfaceOffsetDepth, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUIMagnification", boost::bind(&LLFloaterHMDConfig::onSetUIMagnification, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetUIShapePreset", boost::bind(&LLFloaterHMDConfig::onSetUIShapePreset, this));
}

LLFloaterHMDConfig::~LLFloaterHMDConfig()
{
    sInstance = NULL;
}

//static
LLFloaterHMDConfig* LLFloaterHMDConfig::getInstance()
{
    if (!sInstance)
    {
		sInstance = (LLFloaterReg::getTypedInstance<LLFloaterHMDConfig>("floater_hmd_config"));
    }
    return sInstance;
}

BOOL LLFloaterHMDConfig::postBuild()
{
    setVisible(FALSE);

    mInterpupillaryOffsetSliderCtrl = getChild<LLSlider>("interpupillary_offset_slider");
    mInterpupillaryOffsetAmountCtrl = getChild<LLUICtrl>("interpupillary_offset_slider_amount");
    mUISurfaceOffsetDepthSliderCtrl = getChild<LLSlider>("uisurface_offset_depth_slider");
    mUISurfaceOffsetDepthAmountCtrl = getChild<LLUICtrl>("uisurface_offset_depth_slider_amount");
    mUIMagnificationSliderCtrl = getChild<LLSlider>("ui_magnification_slider");
    mUIMagnificationAmountCtrl = getChild<LLUICtrl>("ui_magnification_slider_amount");
    mUISurfaceShapePresetSliderCtrl = getChild<LLSlider>("uisurface_shape_preset_slider");
    mUISurfaceShapePresetLabelCtrl = getChild<LLUICtrl>("uisurface_shape_preset_value");

    return LLFloater::postBuild();
}

void LLFloaterHMDConfig::onOpen(const LLSD& key)
{
    LLFloaterHMDConfig* pPanel = LLFloaterHMDConfig::getInstance();
    if (pPanel->mInterpupillaryOffsetSliderCtrl)
    {
        pPanel->mInterpupillaryOffsetOriginal = gHMD.getInterpupillaryOffset() * 1000.0f;
        pPanel->mInterpupillaryOffsetSliderCtrl->setValue(pPanel->mInterpupillaryOffsetOriginal);
        pPanel->updateInterpupillaryOffsetLabel();
    }
    if (pPanel->mUISurfaceOffsetDepthSliderCtrl)
    {
        pPanel->mUISurfaceOffsetDepthOriginal = gHMD.getUISurfaceOffsetDepth();
        pPanel->mUISurfaceOffsetDepthSliderCtrl->setValue(pPanel->mUISurfaceOffsetDepthOriginal);
        pPanel->updateUISurfaceOffsetDepthLabel();
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

void LLFloaterHMDConfig::onClose(bool app_quitting)
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

void LLFloaterHMDConfig::onClickResetValues()
{
    mInterpupillaryOffsetSliderCtrl->setValue(gHMD.getInterpupillaryOffsetDefault() * 1000.0f);
    onSetInterpupillaryOffset();
    mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndexDefault());
    onSetUIShapePreset();
}

void LLFloaterHMDConfig::onClickRemovePreset()
{
    if (!gHMD.removePreset((S32)mUISurfaceShapePresetSliderCtrl->getValue()))
    {
        return;
    }
    mUISurfaceShapePresetSliderCtrl->setValue((F32)gHMD.getUIShapePresetIndex());
    mUISurfaceShapePresetSliderCtrl->setMaxValue((F32)gHMD.getNumUIShapePresets());
    onSetUIShapePreset();
}

void LLFloaterHMDConfig::onClickCancel()
{
    // turn off panel and throw away values
    mInterpupillaryOffsetSliderCtrl->setValue(mInterpupillaryOffsetOriginal);
    onSetInterpupillaryOffset();
    mUISurfaceOffsetDepthSliderCtrl->setValue(mUISurfaceOffsetDepthOriginal);
    onSetUISurfaceOffsetDepth();
    mUIMagnificationSliderCtrl->setValue(mUIMagnificationOriginal);
    onSetUIMagnification();
    mUISurfaceShapePresetSliderCtrl->setValue(mUISurfaceShapePresetOriginal);
    onSetUIShapePreset();
    mDirty = FALSE;

	closeFloater(false);
}

void LLFloaterHMDConfig::onClickSave()
{
    gHMD.saveSettings();
    mDirty = FALSE;
	closeFloater(false);
}

void LLFloaterHMDConfig::onSetInterpupillaryOffset()
{
    F32 f = llround(mInterpupillaryOffsetSliderCtrl->getValueF32(), mInterpupillaryOffsetSliderCtrl->getIncrement());
    F32 newVal = llround(f / 1000.0f, mInterpupillaryOffsetSliderCtrl->getIncrement() / 1000.0f);
    gHMD.setInterpupillaryOffset(newVal);
    updateInterpupillaryOffsetLabel();
    updateDirty();
}

void LLFloaterHMDConfig::updateInterpupillaryOffsetLabel()
{
	LLStringUtil::format_map_t args;
	args["[VAL]"] = llformat("%.1f", gHMD.getInterpupillaryOffset() * 1000.0f);
	mInterpupillaryOffsetAmountCtrl->setValue(LLTrans::getString("HMDConfigUnitsMillimeters", args));
}

void LLFloaterHMDConfig::onSetUIMagnification()
{
    F32 f = llround(mUIMagnificationSliderCtrl->getValueF32(), mUIMagnificationSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUIMagnification(f);
    updateUIMagnificationLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfig::updateUIMagnificationLabel()
{
    mUIMagnificationAmountCtrl->setValue(llformat("%.0f", gHMD.getUIMagnification()));
}

void LLFloaterHMDConfig::onSetUISurfaceOffsetDepth()
{
    F32 f = llround(mUISurfaceOffsetDepthSliderCtrl->getValueF32(), mUISurfaceOffsetDepthSliderCtrl->getIncrement());
    U32 oldType = gHMD.getUIShapePresetType();
    gHMD.setUISurfaceOffsetDepth(f);
    updateUISurfaceOffsetDepthLabel();
    updateUIShapePresetLabel(oldType != gHMD.getUIShapePresetType());
    updateDirty();
}

void LLFloaterHMDConfig::updateUISurfaceOffsetDepthLabel()
{
    mUISurfaceOffsetDepthAmountCtrl->setValue(llformat("%.2f", gHMD.getUISurfaceOffsetDepth()));
}

void LLFloaterHMDConfig::onSetUIShapePreset()
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
    updateDirty();
}

void LLFloaterHMDConfig::updateUIShapePresetLabel(BOOL typeChanged)
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
        LLButton* buttonRemove = getChild<LLButton>("hmd_config_hmd_remove_preset");
        switch (gHMD.getUIShapePresetType())
        {
        case LLHMD::kUser:
            if (buttonRemove) buttonRemove->setEnabled(TRUE);
            break;
        case LLHMD::kCustom:
        case LLHMD::kDefault:
        default:
            if (buttonRemove) buttonRemove->setEnabled(FALSE);
            break;
        }
    }
}

void LLFloaterHMDConfig::updateDirty()
{
    LLVector2 cur[] = 
    {
        LLVector2(mInterpupillaryOffsetSliderCtrl->getValueF32(), mInterpupillaryOffsetOriginal),
        LLVector2(mUISurfaceOffsetDepthSliderCtrl->getValueF32(), mUISurfaceOffsetDepthOriginal),
        LLVector2(mUIMagnificationSliderCtrl->getValueF32(), mUIMagnificationOriginal),
        LLVector2(mUISurfaceShapePresetSliderCtrl->getValueF32(), mUISurfaceShapePresetOriginal),
    };
    U32 numVals = sizeof(cur) / sizeof(LLVector2);

    mDirty = FALSE;
    for (U32 i = 0; i < numVals; ++i)
    {
        mDirty = mDirty || !is_approx_equal(cur[i][0], cur[i][1]);
    }
}

#endif