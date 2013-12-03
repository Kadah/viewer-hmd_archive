/** 
 * @file llfloaterhmdconfig.h
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

#ifndef LL_LLFLOATERHMDCONFIG_H
#define LL_LLFLOATERHMDCONFIG_H

#include "linden_common.h"

#include "llfloater.h"

class LLSlider;

class LLFloaterHMDConfig : public LLFloater
{
    friend class LLFloaterReg;

public:
    LLFloaterHMDConfig(const LLSD& key);
	~LLFloaterHMDConfig();

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);

    static LLFloaterHMDConfig* getInstance();

    void onClickCancel();
    void onClickSave();
    void onClickResetValues();

protected:
    void onSetInterpupillaryOffset();
    void updateInterpupillaryOffsetLabel();
    void onSetUISurfaceOffsetDepth();
    void updateUISurfaceOffsetDepthLabel();
    void onSetUIMagnification();
    void updateUIMagnificationLabel();
    void onSetUIShapePreset();
    void updateUIShapePresetLabel();

    void updateDirty();

protected:
    LLSlider* mInterpupillaryOffsetSliderCtrl;
    LLUICtrl* mInterpupillaryOffsetAmountCtrl;
    F32 mInterpupillaryOffsetOriginal;
    LLSlider* mUISurfaceOffsetDepthSliderCtrl;
    LLUICtrl* mUISurfaceOffsetDepthAmountCtrl;
    F32 mUISurfaceOffsetDepthOriginal;
    LLSlider* mUIMagnificationSliderCtrl;
    LLUICtrl* mUIMagnificationAmountCtrl;
    F32 mUIMagnificationOriginal;
    LLSlider* mUISurfaceShapePresetSliderCtrl;
    LLUICtrl* mUISurfaceShapePresetLabelCtrl;
    F32 mUISurfaceShapePresetOriginal;
    BOOL mDirty;

    static LLFloaterHMDConfig* sInstance;
};

#endif // LL_LLFLOATERHMDCONFIG_H
