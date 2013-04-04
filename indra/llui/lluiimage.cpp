/** 
 * @file lluiimage.cpp
 * @brief UI implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

// Utilities functions the user interface needs

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

// Project includes
#include "lluiimage.h"
#include "llui.h"

LLUIImage::LLUIImage(const std::string& name, LLPointer<LLTexture> image)
:	mName(name),
	mImage(image),
	mScaleRegion(0.f, 1.f, 1.f, 0.f),
	mClipRegion(0.f, 1.f, 1.f, 0.f),
	mUniformScaling(TRUE),
	mNoClip(TRUE),
	mImageLoaded(NULL)
{
}

LLUIImage::~LLUIImage()
{
	delete mImageLoaded;
}

void LLUIImage::setClipRegion(const LLRectf& region) 
{ 
	mClipRegion = region; 
	mNoClip = mClipRegion.mLeft == 0.f
				&& mClipRegion.mRight == 1.f
				&& mClipRegion.mBottom == 0.f
				&& mClipRegion.mTop == 1.f;
}

void LLUIImage::setScaleRegion(const LLRectf& region) 
{ 
	mScaleRegion = region; 
	mUniformScaling = mScaleRegion.mLeft == 0.f
					&& mScaleRegion.mRight == 1.f
					&& mScaleRegion.mBottom == 0.f
					&& mScaleRegion.mTop == 1.f;
}

//TODO: move drawing implementation inside class
void LLUIImage::draw(S32 x, S32 y, const LLColor4& color) const
{
	gl_draw_scaled_image(x, y, getWidth(), getHeight(), mImage, color, mClipRegion);
}

void LLUIImage::draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
	if (mUniformScaling)
	{
		gl_draw_scaled_image(x, y, width, height, mImage, color, mClipRegion);
	}
	else
	{
		gl_draw_scaled_image_with_border(
			x, y, 
			width, height, 
			mImage, 
			color,
			FALSE,
			mClipRegion,
			mScaleRegion);
	}
}

void LLUIImage::drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
	gl_draw_scaled_image_with_border(
		x, y, 
		width, height, 
		mImage, 
		color, 
		TRUE,
		mClipRegion,
		mScaleRegion);
}

void LLUIImage::drawBorder(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, S32 border_width) const
{
	LLRect border_rect;
	border_rect.setOriginAndSize(x, y, width, height);
	border_rect.stretch(border_width, border_width);
	drawSolid(border_rect, color);
}

void LLUIImage::draw3D(const LLVector3& origin_agent, const LLVector3& x_axis, const LLVector3& y_axis, 
						const LLRect& rect, const LLColor4& color)
{
	F32 border_scale = 1.f;
	F32 border_height = (1.f - mScaleRegion.getHeight()) * getHeight();
	F32 border_width = (1.f - mScaleRegion.getWidth()) * getWidth();
	if (rect.getHeight() < border_height || rect.getWidth() < border_width)
	{
		 if(border_height - rect.getHeight() > border_width - rect.getWidth())
		 {
			 border_scale = (F32)rect.getHeight() / border_height;
		 }
		 else
		 {
			border_scale = (F32)rect.getWidth() / border_width;
		 }
	}

	LLUI::pushMatrix();
	{ 
		LLVector3 rect_origin = origin_agent + (rect.mLeft * x_axis) + (rect.mBottom * y_axis); 
		LLUI::translate(rect_origin.mV[VX],
						rect_origin.mV[VY], 
						rect_origin.mV[VZ]);
		gGL.getTexUnit(0)->bind(getImage());
		gGL.color4fv(color.mV);

		LLRectf center_uv_rect(mClipRegion.mLeft + mScaleRegion.mLeft * mClipRegion.getWidth(),
							mClipRegion.mBottom + mScaleRegion.mTop * mClipRegion.getHeight(),
							mClipRegion.mLeft + mScaleRegion.mRight * mClipRegion.getWidth(),
							mClipRegion.mBottom + mScaleRegion.mBottom * mClipRegion.getHeight());
		gl_segmented_rect_3d_tex(mClipRegion,
								center_uv_rect,
								LLRectf(border_width * border_scale * 0.5f / (F32)rect.getWidth(),
										(rect.getHeight() - (border_height * border_scale * 0.5f)) / (F32)rect.getHeight(),
										(rect.getWidth() - (border_width * border_scale * 0.5f)) / (F32)rect.getWidth(),
										(border_height * border_scale * 0.5f) / (F32)rect.getHeight()),
								rect.getWidth() * x_axis, 
								rect.getHeight() * y_axis);
		
	} LLUI::popMatrix();
}


S32 LLUIImage::getWidth() const
{ 
	// return clipped dimensions of actual image area
	return llround((F32)mImage->getWidth(0) * mClipRegion.getWidth()); 
}

S32 LLUIImage::getHeight() const
{ 
	// return clipped dimensions of actual image area
	return llround((F32)mImage->getHeight(0) * mClipRegion.getHeight()); 
}

S32 LLUIImage::getTextureWidth() const
{
	return mImage->getWidth(0);
}

S32 LLUIImage::getTextureHeight() const
{
	return mImage->getHeight(0);
}

boost::signals2::connection LLUIImage::addLoadedCallback( const image_loaded_signal_t::slot_type& cb ) 
{
	if (!mImageLoaded) 
	{
		mImageLoaded = new image_loaded_signal_t();
	}
	return mImageLoaded->connect(cb);
}


void LLUIImage::onImageLoaded()
{
	if (mImageLoaded)
	{
		(*mImageLoaded)();
	}
}


namespace LLInitParam
{
	void ParamValue<LLUIImage*>::updateValueFromBlock()
	{
		// The keyword "none" is specifically requesting a null image
		// do not default to current value. Used to overwrite template images. 
		if (name() == "none")
		{
			updateValue(NULL);
			return;
		}

		LLUIImage* imagep =  LLUI::getUIImage(name());
		if (imagep)
		{
			updateValue(imagep);
		}
	}
	
	void ParamValue<LLUIImage*>::updateBlockFromValue(bool make_block_authoritative)
	{
		if (getValue() == NULL)
		{
			name.set("none", make_block_authoritative);
		}
		else
		{
			name.set(getValue()->getName(), make_block_authoritative);
		}
	}

	
	bool ParamCompare<LLUIImage*, false>::equals(
		LLUIImage* const &a,
		LLUIImage* const &b)
	{
		// force all LLUIImages for XML UI export to be "non-default"
		if (!a && !b)
			return false;
		else
			return (a == b);
	}
}

