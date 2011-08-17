/**
 * @file sumLightsV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
 


float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da);
vec3 calcPointLightSpecular(inout vec4 specular, vec3 view, vec3 v, vec3 n, vec3 l, float r, float pw, vec3 lightCol);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 atmosGetDiffuseSunlightColor();
vec3 scaleDownLight(vec3 light);

vec4 sumLightsSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	vec4 col = vec4(0.0, 0.0, 0.0, color.a);
	
	vec3 view = normalize(pos);
	
	/// collect all the specular values from each calcXXXLightSpecular() function
	vec4 specularSum = vec4(0.0);
	
	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	col.rgb += gl_LightSource[1].diffuse.rgb * calcDirectionalLightSpecular(specularColor, view, norm, gl_LightSource[1].position.xyz, gl_LightSource[1].diffuse.rgb, 1.0);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[2].position.xyz, gl_LightSource[2].linearAttenuation, gl_LightSource[2].quadraticAttenuation, gl_LightSource[2].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[3].position.xyz, gl_LightSource[3].linearAttenuation, gl_LightSource[3].quadraticAttenuation, gl_LightSource[3].diffuse.rgb);
	//col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[4].position.xyz, gl_LightSource[4].linearAttenuation, gl_LightSource[4].quadraticAttenuation, gl_LightSource[4].diffuse.rgb);
	col.rgb = scaleDownLight(col.rgb);
						
	// Add windlight lights
	col.rgb += atmosAmbient(baseCol.rgb);
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLightSpecular(specularSum, view, norm, gl_LightSource[0].position.xyz, atmosGetDiffuseSunlightColor()*baseCol.a, 1.0));

	col.rgb = min(col.rgb*color.rgb, 1.0);
	specularColor.rgb = min(specularColor.rgb*specularSum.rgb, 1.0);
	col.rgb += specularColor.rgb;

	return col;	
}
