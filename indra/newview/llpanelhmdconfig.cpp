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
#include "llpanelhmdconfig.h"
#include "llfloaterreg.h"

static LLRegisterPanelClassWrapper<LLPanelHMDConfig> t_panel_hmd_config("panel_hmd_config");
LLPanelHMDConfig* LLPanelHMDConfig::sInstance = NULL;

LLPanelHMDConfig::LLPanelHMDConfig()
{
    sInstance = this;

    mCommitCallbackRegistrar.add("HMDConfig.Close", boost::bind(&LLPanelHMDConfig::onClickClose, this));
    mCommitCallbackRegistrar.add("HMDConfig.SetIPP", boost::bind(&LLPanelHMDConfig::onSetIPP, this));
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
    LLUI::getRootView()->getChildView("menu_stack")->setVisible( ! visible );

    LLPanelHMDConfig::getInstance()->setVisible( ! visible );
}

BOOL LLPanelHMDConfig::postBuild()
{
    setVisible(FALSE);

    mIPPSliderCtrl       = getChild<LLSliderCtrl>("ipp_slider");

	return LLPanel::postBuild();
}

void LLPanelHMDConfig::draw()
{
    LLPanel::draw();
}

void LLPanelHMDConfig::onClickClose()
{
    LLPanelHMDConfig::getInstance()->toggleVisibility();
}

void LLPanelHMDConfig::onSetIPP()
{
    llinfos << "Setting IPP to " << mIPPSliderCtrl->getValueF32() << llendl;
}
