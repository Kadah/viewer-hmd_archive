/** 
 * @file llhudrender.cpp
 * @brief LLHUDRender class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llhudrender.h"

#include "llrender.h"
#include "llgl.h"
#include "llviewercamera.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llfontgl.h"
#include "llglheaders.h"
#include "llviewerwindow.h"
#include "llui.h"
#include "llhmd.h"
#include "llviewerdisplay.h"


void hud_render_utf8text(const std::string &str, const LLVector3 &pos_agent,
					 const LLFontGL &font,
					 const U8 style,
					 const LLFontGL::ShadowType shadow,
					 const F32 x_offset, const F32 y_offset,
					 const LLColor4& color,
					 const BOOL orthographic,
                     BOOL keepLevel)
{
	LLWString wstr(utf8str_to_wstring(str));
	hud_render_text(wstr, pos_agent, font, style, shadow, x_offset, y_offset, color, orthographic, keepLevel);
}

void hud_render_text(const LLWString &wstr, const LLVector3 &pos_agent,
					const LLFontGL &font,
					const U8 style,
					const LLFontGL::ShadowType shadow,
					const F32 x_offset, const F32 y_offset,
					const LLColor4& color,
					const BOOL orthographic,
                    BOOL keepLevel)
{
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	// Do cheap plane culling
	LLVector3 dir_vec = pos_agent - camera->getOrigin();
	dir_vec /= dir_vec.magVec();

	if (wstr.empty() || (!orthographic && dir_vec * camera->getAtAxis() <= 0.f))
	{
		return;
	}

	LLVector3 right_axis;
	LLVector3 up_axis;
	if (orthographic)
	{
		right_axis.setVec(0.f, -1.f / (F32)(gHMD.isHMDMode() ? gHMD.getHMDUIHeight() : gViewerWindow->getWorldViewHeightScaled()), 0.f);
		up_axis.setVec(0.f, 0.f, 1.f / (F32)(gHMD.isHMDMode() ? gHMD.getHMDUIHeight() : gViewerWindow->getWorldViewHeightScaled()));
	}
	else
	{
		camera->getPixelVectors(pos_agent, up_axis, right_axis, keepLevel);
	}

	LLVector3 render_pos = pos_agent + (floorf(x_offset) * right_axis) + (floorf(y_offset) * up_axis);

	//get the render_pos in screen space
	
	F64 winX, winY, winZ;
	S32	viewport[4];
    gViewerWindow->getWorldViewportRaw(viewport);

	F64 mdlv[16];
	F64 proj[16];

	for (U32 i = 0; i < 16; i++)
	{
		mdlv[i] = (F64) gGLModelView[i];
		proj[i] = (F64) gGLProjection[i];
	}

	gluProject(render_pos.mV[0], render_pos.mV[1], render_pos.mV[2],
				mdlv, proj, (GLint*) viewport,
				&winX, &winY, &winZ);
		
	//fonts all render orthographically, set up projection
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	LLUI::pushMatrix();

    // setup ortho camera
    gl_state_for_2d(viewport[2], viewport[3]);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	
	winX -= viewport[0];
	winY -= viewport[1];
	LLUI::loadIdentity();
	gGL.loadIdentity();

    LLVector4 uit((F32) winX*1.0f/LLFontGL::sScaleX, (F32) winY*1.0f/(LLFontGL::sScaleY), -(((F32) winZ*2.f)-1.f), 1.0f);
    if (!orthographic && keepLevel && gHMD.isHMDMode())
    {
        LLMatrix4 mat_neg_roll(0.0f, 0.0f, gHMD.getHMDRoll());
        LLMatrix4 mat_roll(0.0f, 0.0f, -gHMD.getHMDRoll());
        uit = uit * mat_roll;
        gGL.multMatrix((GLfloat*)mat_neg_roll.mMatrix);
    }
    LLUI::translate(uit[VX], uit[VY], uit[VZ]);
	font.render(wstr, 0, 0, 1, color, LLFontGL::LEFT, LLFontGL::BASELINE, style, shadow, wstr.length(), 1000);

	LLUI::popMatrix();
	gGL.popMatrix();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
}
