/** 
 * @file barreldistortF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2D screenMap;

VARYING vec2 vary_texcoord0;

uniform vec2 ScreenCenter;
uniform vec2 ScaleIn;
uniform vec2 ScaleOut;
uniform vec2 LensCenter;
uniform vec4 HmdWarpParam;

// Scales input texture coordinates for distortion.
vec2 HmdWarp(vec2 in01)
{
	vec2 theta = (in01 - LensCenter) * ScaleIn; // Scales to [-1, 1]

	float rSq = theta.x * theta.x + theta.y * theta.y;

	vec2 rvector =	theta * (HmdWarpParam.x + HmdWarpParam.y * rSq +
					HmdWarpParam.z * rSq * rSq +
					HmdWarpParam.w * rSq * rSq * rSq);

	return LensCenter + ScaleOut * rvector;
}

void main() 
{
	vec2 tc = HmdWarp(vary_texcoord0);

	vec2 clamped_tc = clamp(tc, ScreenCenter-vec2(0.5,0.5),
			          ScreenCenter+vec2(0.5,0.5)) - tc;
	
	if (dot(clamped_tc, clamped_tc) > 0.0)
	{
		frag_color = vec4(1,1,1,1);
	}
	else
	{
		frag_color = texture2D(screenMap, tc.xy);
	}
}
