/** 
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llviewerdisplay.h"

#include "llgl.h"
#include "llrender.h"
#include "llglheaders.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "lldynamictexture.h"
#include "lldrawpoolalpha.h"
#include "llfeaturemanager.h"
//#include "llfirstuse.h"
#include "llhudmanager.h"
#include "llimagebmp.h"
#include "llmemory.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstartup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "lltooldraganddrop.h"
#include "lltoolpie.h"
#include "lltracker.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvograss.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llviewershadermgr.h"
#include "llfasttimer.h"
#include "llfloatertools.h"
#include "llviewertexturelist.h"
#include "llfocusmgr.h"
#include "llcubemap.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "lldrawpoolbump.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llpostprocess.h"
#include "llhmd.h"
#include "llrootview.h"

extern LLPointer<LLViewerTexture> gStartTexture;
extern bool gShiftFrame;

LLPointer<LLViewerTexture> gDisconnectedImagep = NULL;

// used to toggle renderer back on after teleport
const F32 TELEPORT_RENDER_DELAY = 20.f; // Max time a teleport is allowed to take before we raise the curtain
const F32 TELEPORT_ARRIVAL_DELAY = 2.f; // Time to preload the world before raising the curtain after we've actually already arrived.
const F32 TELEPORT_LOCAL_DELAY = 1.0f;  // Delay to prevent teleports after starting an in-sim teleport.
BOOL		 gTeleportDisplay = FALSE;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
const F32		RESTORE_GL_TIME = 5.f;	// Wait this long while reloading textures before we raise the curtain

BOOL gForceRenderLandFence = FALSE;
BOOL gDisplaySwapBuffers = FALSE;
BOOL gDepthDirty = FALSE;
BOOL gResizeScreenTexture = FALSE;
BOOL gWindowResized = FALSE;
BOOL gSnapshot = FALSE;
BOOL gShaderProfileFrame = FALSE;

U32 gRecentFrameCount = 0; // number of 'recent' frames
LLFrameTimer gRecentFPSTime;
LLFrameTimer gRecentMemoryTime;

// Rendering stuff
void pre_show_depth_buffer();
void post_show_depth_buffer();
void render_ui(F32 zoom_factor = 1.f, int subfield = 0);
void render_hud_attachments();
void render_ui_3d(BOOL hmdUIMode = FALSE);
void render_ui_2d();
void render_disconnected_background();
void drawBox(const LLVector3& c, const LLVector3& r);

void display_startup()
{
	if (   !gViewerWindow
		|| !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible() 
		|| gViewerWindow->getWindow()->getMinimized() )
	{
		return; 
	}

	gPipeline.updateGL();

	// Update images?
	//gImageList.updateImages(0.01f);
	
	// Written as branch to appease GCC which doesn't like different
	// pointer types across ternary ops
	//
	if (!LLViewerFetchedTexture::sWhiteImagep.isNull())
	{
		LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
	}

	LLGLSDefault gls_default;

	// Required for HTML update in login screen
	static S32 frame_count = 0;

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	if (frame_count++ > 1) // make sure we have rendered a frame first
	{
		LLViewerDynamicTexture::updateAllInstances();
	}

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	LLGLSUIDefault gls_ui;
	gPipeline.disableLights();

	if (gViewerWindow)
    {
	    gViewerWindow->setup2DRender();
    }
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

	gGL.color4f(1,1,1,1);
	if (gViewerWindow)
    {
	    gViewerWindow->draw();
    }
	gGL.flush();

	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	if (gViewerWindow && gViewerWindow->getWindow())
    {
	    gViewerWindow->getWindow()->swapBuffers();
    }

	glClear(GL_DEPTH_BUFFER_BIT);
}

static LLFastTimer::DeclareTimer FTM_UPDATE_CAMERA("Update Camera");

void display_update_camera()
{
	LLFastTimer t(FTM_UPDATE_CAMERA);
	// TODO: cut draw distance down if customizing avatar?
	// TODO: cut draw distance on per-parcel basis?

	// Cut draw distance in half when customizing avatar,
	// but on the viewer only.
	F32 final_far = gAgentCamera.mDrawDistance;
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode())
	{
		final_far *= 0.5f;
	}
	LLViewerCamera::getInstance()->setFar(final_far);
	gViewerWindow->setup3DRender();
	
	// update all the sky/atmospheric/water settings
	LLWLParamManager::getInstance()->update(LLViewerCamera::getInstance());
	LLWaterParamManager::getInstance()->update(LLViewerCamera::getInstance());

	// Update land visibility too
	LLWorld::getInstance()->setLandFarClip(final_far);
}

// Write some stats to llinfos
void display_stats()
{
	F32 fps_log_freq = gSavedSettings.getF32("FPSLogFrequency");
	if (fps_log_freq > 0.f && gRecentFPSTime.getElapsedTimeF32() >= fps_log_freq)
	{
		F32 fps = gRecentFrameCount / fps_log_freq;
		llinfos << llformat("FPS: %.02f", fps) << llendl;
		gRecentFrameCount = 0;
		gRecentFPSTime.reset();
	}
	F32 mem_log_freq = gSavedSettings.getF32("MemoryLogFrequency");
	if (mem_log_freq > 0.f && gRecentMemoryTime.getElapsedTimeF32() >= mem_log_freq)
	{
		gMemoryAllocated = LLMemory::getCurrentRSS();
		U32 memory = (U32)(gMemoryAllocated / (1024*1024));
		llinfos << llformat("MEMORY: %d MB", memory) << llendl;
		LLMemory::logMemoryInfo(TRUE) ;
		gRecentMemoryTime.reset();
	}
}

static LLFastTimer::DeclareTimer FTM_PICK("Picking");
static LLFastTimer::DeclareTimer FTM_RENDER("Render", true);
static LLFastTimer::DeclareTimer FTM_UPDATE_SKY("Update Sky");
static LLFastTimer::DeclareTimer FTM_UPDATE_TEXTURES("Update Textures");
static LLFastTimer::DeclareTimer FTM_IMAGE_UPDATE("Update Images");
static LLFastTimer::DeclareTimer FTM_IMAGE_UPDATE_CLASS("Class");
static LLFastTimer::DeclareTimer FTM_IMAGE_UPDATE_BUMP("Image Update Bump");
static LLFastTimer::DeclareTimer FTM_IMAGE_UPDATE_LIST("List");
static LLFastTimer::DeclareTimer FTM_IMAGE_UPDATE_DELETE("Delete");
static LLFastTimer::DeclareTimer FTM_RESIZE_WINDOW("Resize Window");
static LLFastTimer::DeclareTimer FTM_HUD_UPDATE("HUD Update");
static LLFastTimer::DeclareTimer FTM_DISPLAY_UPDATE_GEOM("Update Geom");
static LLFastTimer::DeclareTimer FTM_TEXTURE_UNBIND("Texture Unbind");
static LLFastTimer::DeclareTimer FTM_TELEPORT_DISPLAY("Teleport Display");


// Paint the display!
void display(BOOL rebuild, F32 zoom_factor, int subfield, BOOL for_snapshot)
{
	LLFastTimer t(FTM_RENDER);

	if (gWindowResized)
	{ 
        //skip render on frames where window has been resized...unless we're taking the final snapshot, that is.
		LLFastTimer t(FTM_RESIZE_WINDOW);
		gGL.flush();
		glClear(GL_COLOR_BUFFER_BIT);
		gViewerWindow->getWindow()->swapBuffers();
		LLPipeline::refreshCachedSettings();
		gPipeline.resizeScreenTexture();
		gResizeScreenTexture = FALSE;
		gWindowResized = FALSE;
        if (!LLAppViewer::instance()->isSavingFinalSnapshot())
        {
		    return;
        }
	}

	if (LLPipeline::sRenderDeferred)
	{ //hack to make sky show up in deferred snapshots
		for_snapshot = FALSE;
	}

	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_pause();
	}

	gSnapshot = for_snapshot;

	LLGLSDefault gls_default;
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);
	
	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	
	stop_glerror();

	gPipeline.disableLights();
	
	//reset vertex buffers if needed
	gPipeline.doResetVertexBuffers();

	stop_glerror();

	// Don't draw if the window is hidden or minimized.
	// In fact, must explicitly check the minimized state before drawing.
	// Attempting to draw into a minimized window causes a GL error. JC
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible() 
		|| gViewerWindow->getWindow()->getMinimized() )
	{
		// Clean up memory the pools may have allocated
		if (rebuild)
		{
			stop_glerror();
			gPipeline.rebuildPools();
			stop_glerror();
		}

		stop_glerror();
		gViewerWindow->returnEmptyPicks();
		stop_glerror();
		return; 
	}

	gViewerWindow->checkSettings();
	
	{
		LLFastTimer ftm(FTM_PICK);
		LLAppViewer::instance()->pingMainloopTimeout("Display:Pick");
		gViewerWindow->performPick();
	}
	
	LLAppViewer::instance()->pingMainloopTimeout("Display:CheckStates");
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	
	//////////////////////////////////////////////////////////
	//
	// Logic for forcing window updates if we're in drone mode.
	//

	// *TODO: Investigate running display() during gHeadlessClient.  See if this early exit is needed DK 2011-02-18
	if (gHeadlessClient) 
	{
#if LL_WINDOWS
		static F32 last_update_time = 0.f;
		if ((gFrameTimeSeconds - last_update_time) > 1.f)
		{
			InvalidateRect((HWND)gViewerWindow->getPlatformWindow(), NULL, FALSE);
			last_update_time = gFrameTimeSeconds;
		}
#elif LL_DARWIN
		// MBW -- Do something clever here.
#endif
		// Not actually rendering, don't bother.
		return;
	}


	//
	// Bail out if we're in the startup state and don't want to try to
	// render the world.
	//
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Startup");
		display_startup();
		return;
	}


	if (gShaderProfileFrame)
	{
		LLGLSLShader::initProfile();
	}

	//LLGLState::verify(FALSE);

	/////////////////////////////////////////////////
	//
	// Update GL Texture statistics (used for discard logic?)
	//

	LLAppViewer::instance()->pingMainloopTimeout("Display:TextureStats");
	stop_glerror();

	LLImageGL::updateStats(gFrameTimeSeconds);
	
	LLVOAvatar::sRenderName = gSavedSettings.getS32("AvatarNameTagMode");
	LLVOAvatar::sRenderGroupTitles = (gSavedSettings.getBOOL("NameTagShowGroupTitles") && gSavedSettings.getS32("AvatarNameTagMode"));
	
	gPipeline.mBackfaceCull = TRUE;
	gFrameCount++;
	gRecentFrameCount++;
	if (gFocusMgr.getAppHasFocus())
	{
		gForegroundFrameCount++;
	}

	//////////////////////////////////////////////////////////
	//
	// Display start screen if we're teleporting, and skip render
	//

	if (gTeleportDisplay)
	{
		LLFastTimer t(FTM_TELEPORT_DISPLAY);
		LLAppViewer::instance()->pingMainloopTimeout("Display:Teleport");
		const F32 TELEPORT_ARRIVAL_DELAY = 2.f; // Time to preload the world before raising the curtain after we've actually already arrived.

		S32 attach_count = 0;
		if (isAgentAvatarValid())
		{
			attach_count = gAgentAvatarp->getAttachmentCount();
		}
		F32 teleport_save_time = TELEPORT_EXPIRY + TELEPORT_EXPIRY_PER_ATTACHMENT * attach_count;
		F32 teleport_elapsed = gTeleportDisplayTimer.getElapsedTimeF32();
		F32 teleport_percent = teleport_elapsed * (100.f / teleport_save_time);
		if( (gAgent.getTeleportState() != LLAgent::TELEPORT_START) && (teleport_percent > 100.f) )
		{
			// Give up.  Don't keep the UI locked forever.
			gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
			gAgent.setTeleportMessage(std::string());
		}

		const std::string& message = gAgent.getTeleportMessage();
		switch( gAgent.getTeleportState() )
		{
		case LLAgent::TELEPORT_PENDING:
			gTeleportDisplayTimer.reset();
			gViewerWindow->setShowProgress(TRUE);
			gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
			gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["pending"]);
			gViewerWindow->setProgressString(LLAgent::sTeleportProgressMessages["pending"]);
			break;

		case LLAgent::TELEPORT_START:
			// Transition to REQUESTED.  Viewer has sent some kind
			// of TeleportRequest to the source simulator
			gTeleportDisplayTimer.reset();
			gViewerWindow->setShowProgress(TRUE);
			gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
			gAgent.setTeleportState( LLAgent::TELEPORT_REQUESTED );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["requesting"]);
			gViewerWindow->setProgressString(LLAgent::sTeleportProgressMessages["requesting"]);
			break;

		case LLAgent::TELEPORT_REQUESTED:
			// Waiting for source simulator to respond
			gViewerWindow->setProgressPercent( llmin(teleport_percent, 37.5f) );
			gViewerWindow->setProgressString(message);
			break;

		case LLAgent::TELEPORT_MOVING:
			// Viewer has received destination location from source simulator
			gViewerWindow->setProgressPercent( llmin(teleport_percent, 75.f) );
			gViewerWindow->setProgressString(message);
			break;

		case LLAgent::TELEPORT_START_ARRIVAL:
			// Transition to ARRIVING.  Viewer has received avatar update, etc., from destination simulator
			gTeleportArrivalTimer.reset();
				gViewerWindow->setProgressCancelButtonVisible(FALSE, LLTrans::getString("Cancel"));
			gViewerWindow->setProgressPercent(75.f);
			gAgent.setTeleportState( LLAgent::TELEPORT_ARRIVING );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["arriving"]);
			gTextureList.mForceResetTextureStats = TRUE;
			gAgentCamera.resetView(TRUE, TRUE);
			
			break;

		case LLAgent::TELEPORT_ARRIVING:
			// Make the user wait while content "pre-caches"
			{
				F32 arrival_fraction = (gTeleportArrivalTimer.getElapsedTimeF32() / TELEPORT_ARRIVAL_DELAY);
				if( arrival_fraction > 1.f )
				{
					arrival_fraction = 1.f;
					//LLFirstUse::useTeleport();
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
				gViewerWindow->setProgressCancelButtonVisible(FALSE, LLTrans::getString("Cancel"));
				gViewerWindow->setProgressPercent(  arrival_fraction * 25.f + 75.f);
				gViewerWindow->setProgressString(message);
			}
			break;

		case LLAgent::TELEPORT_LOCAL:
			// Short delay when teleporting in the same sim (progress screen active but not shown - did not
			// fall-through from TELEPORT_START)
			{
				if( gTeleportDisplayTimer.getElapsedTimeF32() > TELEPORT_LOCAL_DELAY )
				{
					//LLFirstUse::useTeleport();
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
			}
			break;

		case LLAgent::TELEPORT_NONE:
			// No teleport in progress
			gViewerWindow->setShowProgress(FALSE);
			gTeleportDisplay = FALSE;
			break;
		}
	}
    else if(LLAppViewer::instance()->logoutRequestSent())
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Logout");
		F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
		if (percent_done > 100.f)
		{
			percent_done = 100.f;
		}

		if( LLApp::isExiting() )
		{
			percent_done = 100.f;
		}
		
		gViewerWindow->setProgressPercent( percent_done );
	}
	else
	if (gRestoreGL)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:RestoreGL");
		F32 percent_done = gRestoreGLTimer.getElapsedTimeF32() * 100.f / RESTORE_GL_TIME;
		if( percent_done > 100.f )
		{
			gViewerWindow->setShowProgress(FALSE);
			gRestoreGL = FALSE;
		}
		else
		{

			if( LLApp::isExiting() )
			{
				percent_done = 100.f;
			}
			
			gViewerWindow->setProgressPercent( percent_done );
		}
	}

	//////////////////////////
	//
	// Prepare for the next frame
	//

	/////////////////////////////
	//
	// Update the camera
	//
	//

	LLAppViewer::instance()->pingMainloopTimeout("Display:Camera");
	LLViewerCamera::getInstance()->setZoomParameters(zoom_factor, subfield);
	LLViewerCamera::getInstance()->setNear(MIN_NEAR_PLANE);

	//////////////////////////
	//
	// clear the next buffer
	// (must follow dynamic texture writing since that uses the frame buffer)
	//

	if (gDisconnected)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Disconnected");
		render_ui();
	}
	
	//////////////////////////
	//
	// Set rendering options
	//
	//
	LLAppViewer::instance()->pingMainloopTimeout("Display:RenderSetup");
	stop_glerror();

	///////////////////////////////////////
	//
	// Slam lighting parameters back to our defaults.
	// Note that these are not the same as GL defaults...

	stop_glerror();
	gGL.setAmbientLightColor(LLColor4::white);
	stop_glerror();
			
	/////////////////////////////////////
	//
	// Render
	//
	// Actually push all of our triangles to the screen.
	//

	// do render-to-texture stuff here
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:DynamicTextures");
		LLFastTimer t(FTM_UPDATE_TEXTURES);
		if (LLViewerDynamicTexture::updateAllInstances())
		{
			gGL.setColorMask(true, true);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	}

    gPipeline.resetFrameStats();	// Reset per-frame statistics.

    for (U32 i = (U32)LLViewerCamera::CENTER_EYE; i <= (U32)LLViewerCamera::RIGHT_EYE; ++i)
	{
        LLViewerCamera::sCurrentEye = i;
        U32 render_mode = gHMD.getRenderMode();
        switch(i)
        {
        case LLViewerCamera::CENTER_EYE:
            if (!gDisconnected && !gSnapshot && gHMD.isPostDetectionInitialized() && gHMD.isHMDConnected())
            {
                if (render_mode == LLHMD::RenderMode_HMD)
                {
                    if (((S32)LLFrameTimer::getFrameCount() % 30) == 0)
                    {
                        gHMD.renderUnusedMainWindow();
                    }
                }
                else
                {
                    if (((S32)LLFrameTimer::getFrameCount() % 30) == 0)
                    {
                        gHMD.renderUnusedHMDWindow();
                    }
                    gHMD.setRenderWindowMain();
                }
            }
            if (render_mode != LLHMD::RenderMode_None)
            {
                continue;
            }
            break;
        case LLViewerCamera::LEFT_EYE:
        case LLViewerCamera::RIGHT_EYE:
            {
				if ((!gHMD.isAdvancedMode() && !gHMD.isPostDetectionInitialized()) || render_mode == LLHMD::RenderMode_None)
                {
                    continue;
                }
                gHMD.setCurrentEye(i);
                if (gHMD.isPostDetectionInitialized() &&
                    gHMD.isHMDConnected() &&
                    i == LLViewerCamera::LEFT_EYE &&
                    render_mode == LLHMD::RenderMode_HMD)
                {
                    if (!gHMD.setRenderWindowHMD())
                    {
                        i = LLViewerCamera::RIGHT_EYE;
                        gHMD.setRenderMode(LLHMD::RenderMode_None);
                        LL_WARNS("HMD") << "Could not set Render Window to HMD.  Aborting and setting RenderMode to normal." << LL_ENDL;
                        continue;
                    }
                }
            }
            break;
        }
		gViewerWindow->setup3DViewport();

		if (!gDisconnected)
		{
			LLAppViewer::instance()->pingMainloopTimeout("Display:Update");
			if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
			{ //don't draw hud objects in this frame
				gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
			}

			if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
			{ //don't draw hud particles in this frame
				gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
			}

			//upkeep gl name pools
			LLGLNamePool::upkeepPools();
		
			stop_glerror();
			display_update_camera();
			stop_glerror();
				
			// *TODO: merge these two methods
			{
				LLFastTimer t(FTM_HUD_UPDATE);
				LLHUDManager::getInstance()->updateEffects();
				LLHUDObject::updateAll();
				stop_glerror();
			}

			{
				LLFastTimer t(FTM_DISPLAY_UPDATE_GEOM);
				const F32 max_geom_update_time = 0.005f*10.f*gFrameIntervalSeconds; // 50 ms/second update time
				gPipeline.createObjects(max_geom_update_time);
				gPipeline.processPartitionQ();
				gPipeline.updateGeom(max_geom_update_time);
				stop_glerror();
			}

			gPipeline.updateGL();
		
			stop_glerror();

			S32 water_clip = 0;
			if ((LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT) > 1) &&
				 (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_WATER) || 
				  gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_VOIDWATER)))
			{
				if (LLViewerCamera::getInstance()->cameraUnderWater())
				{
					water_clip = -1;
				}
				else
				{
					water_clip = 1;
				}
			}
		
			LLAppViewer::instance()->pingMainloopTimeout("Display:Cull");
		
			//Increment drawable frame counter
			LLDrawable::incrementVisible();

			LLSpatialGroup::sNoDelete = TRUE;
			LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();

			S32 occlusion = LLPipeline::sUseOcclusion;
			if (gDepthDirty)
			{ //depth buffer is invalid, don't overwrite occlusion state
				LLPipeline::sUseOcclusion = llmin(occlusion, 1);
			}
			gDepthDirty = FALSE;

			LLGLState::checkStates();
			LLGLState::checkTextureChannels();
			LLGLState::checkClientArrays();

			static LLCullResult result;
			LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
			LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater() ? TRUE : FALSE;
			gPipeline.updateCull(*LLViewerCamera::getInstance(), result, water_clip);
			stop_glerror();

			LLGLState::checkStates();
			LLGLState::checkTextureChannels();
			LLGLState::checkClientArrays();

			BOOL to_texture = gPipeline.canUseVertexShaders() &&
							LLPipeline::sRenderGlow;

			LLAppViewer::instance()->pingMainloopTimeout("Display:Swap");
		
			{ 
				if (gResizeScreenTexture)
				{
					gResizeScreenTexture = FALSE;
					gPipeline.resizeScreenTexture();
				}

				gGL.setColorMask(true, true);
				glClearColor(0,0,0,0);

				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();

				if (!for_snapshot)
				{
					if (gFrameCount > 1)
					{ //for some reason, ATI 4800 series will error out if you 
					  //try to generate a shadow before the first frame is through
						gPipeline.generateSunShadow(*LLViewerCamera::getInstance());
					}

					LLVertexBuffer::unbind();

					LLGLState::checkStates();
					LLGLState::checkTextureChannels();
					LLGLState::checkClientArrays();

                    glh::matrix4f proj = glh_get_current_projection();
                    glh::matrix4f mod = glh_get_current_modelview();
					glViewport(0,0,512,512);
					LLVOAvatar::updateFreezeCounter() ;

					if(!LLPipeline::sMemAllocationThrottled)
					{		
						LLVOAvatar::updateImpostors();
					}

					glh_set_current_projection(proj);
					glh_set_current_modelview(mod);
					gGL.matrixMode(LLRender::MM_PROJECTION);
					gGL.loadMatrix(proj.m);
					gGL.matrixMode(LLRender::MM_MODELVIEW);
					gGL.loadMatrix(mod.m);
					gViewerWindow->setup3DViewport();

					LLGLState::checkStates();
					LLGLState::checkTextureChannels();
					LLGLState::checkClientArrays();

				}
				glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			}

			LLGLState::checkStates();
			LLGLState::checkClientArrays();

			//if (!for_snapshot)
			{
				LLAppViewer::instance()->pingMainloopTimeout("Display:Imagery");
				gPipeline.generateWaterReflection(*LLViewerCamera::getInstance());
				gPipeline.generateHighlight(*LLViewerCamera::getInstance());
				gPipeline.renderPhysicsDisplay();
			}

			LLGLState::checkStates();
			LLGLState::checkClientArrays();

			//////////////////////////////////////
			//
			// Update images, using the image stats generated during object update/culling
			//
			// Can put objects onto the retextured list.
			//
			// Doing this here gives hardware occlusion queries extra time to complete
			LLAppViewer::instance()->pingMainloopTimeout("Display:UpdateImages");
		
			{
				LLFastTimer t(FTM_IMAGE_UPDATE);
			
				{
					LLFastTimer t(FTM_IMAGE_UPDATE_CLASS);
					LLViewerTexture::updateClass(LLViewerCamera::getInstance()->getVelocityStat()->getMean(),
												LLViewerCamera::getInstance()->getAngularVelocityStat()->getMean());
				}

			
				{
					LLFastTimer t(FTM_IMAGE_UPDATE_BUMP);
					gBumpImageList.updateImages();  // must be called before gTextureList version so that it's textures are thrown out first.
				}

				{
					LLFastTimer t(FTM_IMAGE_UPDATE_LIST);
					F32 max_image_decode_time = 0.050f*gFrameIntervalSeconds; // 50 ms/second decode time
					max_image_decode_time = llclamp(max_image_decode_time, 0.002f, 0.005f ); // min 2ms/frame, max 5ms/frame)
					gTextureList.updateImages(max_image_decode_time);
				}

				/*{
					LLFastTimer t(FTM_IMAGE_UPDATE_DELETE);
					//remove dead textures from GL
					LLImageGL::deleteDeadTextures();
					stop_glerror();
				}*/
			}

			LLGLState::checkStates();
			LLGLState::checkClientArrays();

			///////////////////////////////////
			//
			// StateSort
			//
			// Responsible for taking visible objects, and adding them to the appropriate draw orders.
			// In the case of alpha objects, z-sorts them first.
			// Also creates special lists for outlines and selected face rendering.
			//
			LLAppViewer::instance()->pingMainloopTimeout("Display:StateSort");
			{
				LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
				gPipeline.stateSort(*LLViewerCamera::getInstance(), result);
				stop_glerror();
				
				if (rebuild)
				{
					//////////////////////////////////////
					//
					// rebuildPools
					//
					//
					gPipeline.rebuildPools();
					stop_glerror();
				}
			}

			LLGLState::checkStates();
			LLGLState::checkClientArrays();

			LLPipeline::sUseOcclusion = occlusion;

			{
				LLAppViewer::instance()->pingMainloopTimeout("Display:Sky");
				LLFastTimer t(FTM_UPDATE_SKY);	
				gSky.updateSky();
			}

			if(gUseWireframe)
			{
				glClearColor(0.5f, 0.5f, 0.5f, 0.f);
				glClear(GL_COLOR_BUFFER_BIT);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}

			LLAppViewer::instance()->pingMainloopTimeout("Display:RenderStart");
		
			//// render frontmost floater opaque for occlusion culling purposes
			//LLFloater* frontmost_floaterp = gFloaterView->getFrontmost();
			//// assumes frontmost floater with focus is opaque
			//if (frontmost_floaterp && gFocusMgr.childHasKeyboardFocus(frontmost_floaterp))
			//{
			//	gGL.matrixMode(LLRender::MM_MODELVIEW);
			//	gGL.pushMatrix();
			//	{
			//		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			//		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
			//		gGL.loadIdentity();

			//		LLRect floater_rect = frontmost_floaterp->calcScreenRect();
			//		// deflate by one pixel so rounding errors don't occlude outside of floater extents
			//		floater_rect.stretch(-1);
			//		LLRectf floater_3d_rect((F32)floater_rect.mLeft / (F32)gViewerWindow->getWindowWidthScaled(), 
			//								(F32)floater_rect.mTop / (F32)gViewerWindow->getWindowHeightScaled(),
			//								(F32)floater_rect.mRight / (F32)gViewerWindow->getWindowWidthScaled(),
			//								(F32)floater_rect.mBottom / (F32)gViewerWindow->getWindowHeightScaled());
			//		floater_3d_rect.translate(-0.5f, -0.5f);
			//		gGL.translatef(0.f, 0.f, -LLViewerCamera::getInstance()->getNear());
			//		gGL.scalef(LLViewerCamera::getInstance()->getNear() * LLViewerCamera::getInstance()->getAspect() / sinf(LLViewerCamera::getInstance()->getView()), LLViewerCamera::getInstance()->getNear() / sinf(LLViewerCamera::getInstance()->getView()), 1.f);
			//		gGL.color4fv(LLColor4::white.mV);
			//		gGL.begin(LLVertexBuffer::QUADS);
			//		{
			//			gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mBottom, 0.f);
			//			gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mTop, 0.f);
			//			gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mTop, 0.f);
			//			gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mBottom, 0.f);
			//		}
			//		gGL.end();
			//		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			//	}
			//	gGL.popMatrix();
			//}

			LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater() ? TRUE : FALSE;

			LLGLState::checkStates();
			LLGLState::checkClientArrays();

			LLGLState::checkStates();
			LLGLState::checkClientArrays();

			stop_glerror();

			if (to_texture)
			{
				gGL.setColorMask(true, true);
					
				if (LLPipeline::sRenderDeferred)
				{
					gPipeline.mDeferredScreen.bindTarget();
                    glClearColor(1,0,1,1);
					gPipeline.mDeferredScreen.clear();
				}
				else
				{
					gPipeline.mScreen.bindTarget();
					if (LLPipeline::sUnderWaterRender && !gPipeline.canUseWindLightShaders())
					{
						const LLColor4 &col = LLDrawPoolWater::sWaterFogColor;
						glClearColor(col.mV[0], col.mV[1], col.mV[2], 0.f);
					}
					gPipeline.mScreen.clear();
				}
			
				gGL.setColorMask(true, false);
			}
		
			LLAppViewer::instance()->pingMainloopTimeout("Display:RenderGeom");
			if (!(LLAppViewer::instance()->logoutRequestSent() && LLAppViewer::instance()->hasSavedFinalSnapshot())
					&& !gRestoreGL)
			{
				LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

				if (gSavedSettings.getBOOL("RenderDepthPrePass") && LLGLSLShader::sNoFixedFunction)
				{
					gGL.setColorMask(false, false);
				
					U32 types[] = { 
						LLRenderPass::PASS_SIMPLE, 
						LLRenderPass::PASS_FULLBRIGHT, 
						LLRenderPass::PASS_SHINY 
					};

					U32 num_types = LL_ARRAY_SIZE(types);
					gOcclusionProgram.bind();
					for (U32 i = 0; i < num_types; i++)
					{
						gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE);
					}

					gOcclusionProgram.unbind();
				}

				gGL.setColorMask(true, false);
				if (LLPipeline::sRenderDeferred)
				{
					gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance());
				}
				else
				{
					gPipeline.renderGeom(*LLViewerCamera::getInstance(), TRUE);
				}
			
				gGL.setColorMask(true, true);

				//store this frame's modelview matrix for use
				//when rendering next frame's occlusion queries
				for (U32 i = 0; i < 16; i++)
				{
					gGLLastModelView[i] = gGLModelView[i];
					gGLLastProjection[i] = gGLProjection[i];
				}
				stop_glerror();
			}

			{
				LLFastTimer t(FTM_TEXTURE_UNBIND);
				for (U32 i = 0; i < gGLManager.mNumTextureImageUnits; i++)
				{ //dummy cleanup of any currently bound textures
					if (gGL.getTexUnit(i)->getCurrType() != LLTexUnit::TT_NONE)
					{
						gGL.getTexUnit(i)->unbind(gGL.getTexUnit(i)->getCurrType());
						gGL.getTexUnit(i)->disable();
					}
				}
			}

			LLAppViewer::instance()->pingMainloopTimeout("Display:RenderFlush");		

			if (to_texture)
			{
				if (LLPipeline::sRenderDeferred)
				{
					gPipeline.mDeferredScreen.flush();
					if(LLRenderTarget::sUseFBO)
					{
						LLRenderTarget::copyContentsToFramebuffer(gPipeline.mDeferredScreen, 0, 0, gPipeline.mDeferredScreen.getWidth(), 
																  gPipeline.mDeferredScreen.getHeight(), 0, 0, 
																  gPipeline.mDeferredScreen.getWidth(), 
																  gPipeline.mDeferredScreen.getHeight(), 
																  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
					}
				}
				else
				{
					gPipeline.mScreen.flush();
					if(LLRenderTarget::sUseFBO)
					{				
						LLRenderTarget::copyContentsToFramebuffer(gPipeline.mScreen, 0, 0, gPipeline.mScreen.getWidth(), 
																  gPipeline.mScreen.getHeight(), 0, 0, 
																  gPipeline.mScreen.getWidth(), 
																  gPipeline.mScreen.getHeight(), 
																  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
					}
				}
			}

			if (LLPipeline::sRenderDeferred)
			{
				gPipeline.renderDeferredLighting();
			}

			LLPipeline::sUnderWaterRender = FALSE;

		    LLAppViewer::instance()->pingMainloopTimeout("Display:RenderUI");
		    if (!for_snapshot)
		    {
			    LLFastTimer t(FTM_RENDER_UI);
                render_ui(1.0f, 0);
		    }
		}
	}

    // this prevents forced shutdown while in HMD mode showing only a black screen
    LLViewerCamera::sCurrentEye = LLViewerCamera::CENTER_EYE;

    if (!gDisconnected)
    {
	    LLSpatialGroup::sNoDelete = FALSE;
	    gPipeline.clearReferences();
	    gPipeline.rebuildGroups();
    }

	LLAppViewer::instance()->pingMainloopTimeout("Display:FrameStats");
	
	stop_glerror();

	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_resume();
		LLPipeline::sRenderFrameTest = FALSE;
	}

	display_stats();
				
	LLAppViewer::instance()->pingMainloopTimeout("Display:Done");

	gShiftFrame = false;

	if (gShaderProfileFrame)
	{
		gShaderProfileFrame = FALSE;
		LLGLSLShader::finishProfile();
	}
}

void render_hud_attachments()
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
		
	glh::matrix4f current_proj = glh_get_current_projection();
	glh::matrix4f current_mod = glh_get_current_modelview();

	// clamp target zoom level to reasonable values
	gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, 0.1f, 1.f);
	// smoothly interpolate current zoom level
	gAgentCamera.mHUDCurZoom = lerp(gAgentCamera.mHUDCurZoom, gAgentCamera.mHUDTargetZoom, LLCriticalDamp::getInterpolant(0.03f));

	if (LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices())
	{
		LLPipeline::sRenderingHUDs = TRUE;
		LLCamera hud_cam = *LLViewerCamera::getInstance();
		hud_cam.setOrigin(-1.f,0,0);
		hud_cam.setAxes(LLVector3(1,0,0), LLVector3(0,1,0), LLVector3(0,0,1));
		LLViewerCamera::updateFrustumPlanes(hud_cam, TRUE);

		bool render_particles = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) && gSavedSettings.getBOOL("RenderHUDParticles");
		
		//only render hud objects
		gPipeline.pushRenderTypeMask();
		
		// turn off everything
		gPipeline.andRenderTypeMask(LLPipeline::END_RENDER_TYPES);
		// turn on HUD
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		// turn on HUD particles
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);

		// if particles are off, turn off hud-particles as well
		if (!render_particles)
		{
			// turn back off HUD particles
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
		}

		bool has_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}

		S32 use_occlusion = LLPipeline::sUseOcclusion;
		LLPipeline::sUseOcclusion = 0;
				
		//cull, sort, and render hud objects
		static LLCullResult result;
		LLSpatialGroup::sNoDelete = TRUE;

		LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
		gPipeline.updateCull(hud_cam, result);

		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_MATERIAL);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_SHINY);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISIBLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISI_SHINY);
		
		gPipeline.stateSort(hud_cam, result);

		gPipeline.renderGeom(hud_cam);

		LLSpatialGroup::sNoDelete = FALSE;
		//gPipeline.clearReferences();

		render_hud_elements();

		//restore type mask
		gPipeline.popRenderTypeMask();

		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		LLPipeline::sUseOcclusion = use_occlusion;
		LLPipeline::sRenderingHUDs = FALSE;
	}
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();

	glh_set_current_projection(current_proj);
	glh_set_current_modelview(current_mod);
}

LLRect get_whole_screen_region()
{
    LLRect whole_screen = gViewerWindow->getWorldViewRectScaled();
	
	// apply camera zoom transform (for high res screenshots)
	F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
	S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
	if (zoom_factor > 1.f)
	{
        S32 wsw = whole_screen.getWidth();
        S32 wsh = whole_screen.getHeight();
		S32 num_horizontal_tiles = llceil(zoom_factor);
		S32 tile_width = llround((F32)wsw / zoom_factor);
		S32 tile_height = llround((F32)wsh / zoom_factor);
		int tile_y = sub_region / num_horizontal_tiles;
		int tile_x = sub_region - (tile_y * num_horizontal_tiles);
			
        whole_screen.setLeftTopAndSize(tile_x * tile_width, wsh - (tile_y * tile_height), tile_width, tile_height);
	}
	return whole_screen;
}

bool get_hud_matrices(const LLRect& screen_region, glh::matrix4f &proj, glh::matrix4f &model)
{
	if (isAgentAvatarValid() && gAgentAvatarp->hasHUDAttachment())
	{
		F32 zoom_level = gAgentCamera.mHUDCurZoom;
		LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();
		F32 aspect_ratio = LLViewerCamera::getInstance()->getUIAspect();
		F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		proj = gl_ortho(-0.5f * aspect_ratio, 0.5f * aspect_ratio, -0.5f, 0.5f, 0.f, hud_depth);
		proj.element(2,2) = -0.01f; // wtf??
		
		glh::matrix4f mat;
        F32 wvsw = (F32)gViewerWindow->getWorldViewWidthScaled();
        F32 wvsh = (F32)gViewerWindow->getWorldViewHeightScaled();
		F32 scale_x = wvsw / (F32)screen_region.getWidth();
	    F32 scale_y = wvsh / (F32)screen_region.getHeight();
		mat.set_scale(glh::vec3f(scale_x, scale_y, 1.f));
		mat.set_translate(
			glh::vec3f(clamp_rescale((F32)(screen_region.getCenterX() - screen_region.mLeft), 0.f, wvsw, 0.5f * scale_x * aspect_ratio, -0.5f * scale_x * aspect_ratio),
					   clamp_rescale((F32)(screen_region.getCenterY() - screen_region.mBottom), 0.f, wvsh, 0.5f * scale_y, -0.5f * scale_y),
					   0.f));
		proj *= mat;
        //if (gHMD.isHMDMode())
        //{
        //    mat.make_identity();
        //    mat.set_translate(glh::vec3f(gHMD.getInterpupillaryOffset(), 0.0f, 0.0f));
        //    proj = mat * proj;
        //}

		glh::matrix4f tmp_model((GLfloat*) OGL_TO_CFR_ROTATION);
		mat.set_scale(glh::vec3f(zoom_level, zoom_level, zoom_level));
		mat.set_translate(glh::vec3f(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f));
		tmp_model *= mat;
        //if (gHMD.isHMDMode())
        //{
        //    F32 viewOffset = gHMD.getInterpupillaryOffset();
        //    mat.make_identity();
        //    mat.set_translate(glh::vec3f(LLViewerCamera::sCurrentEye == LLViewerCamera::LEFT_EYE ? viewOffset : -viewOffset, 0.0f, 0.0f));
        //    tmp_model = mat * tmp_model;
        //}
		model = tmp_model;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool get_hud_matrices(glh::matrix4f &proj, glh::matrix4f &model)
{
	LLRect whole_screen = get_whole_screen_region();
	return get_hud_matrices(whole_screen, proj, model);
}

BOOL setup_hud_matrices()
{
	LLRect whole_screen = get_whole_screen_region();
	return setup_hud_matrices(whole_screen);
}

BOOL setup_hud_matrices(const LLRect& screen_region)
{
	glh::matrix4f proj, model;
	bool result = get_hud_matrices(screen_region, proj, model);
	if (!result) return result;
	
	// set up transform to keep HUD objects in front of camera
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.loadMatrix(proj.m);
	glh_set_current_projection(proj);
	
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadMatrix(model.m);
	glh_set_current_modelview(model);
	return TRUE;
}

static LLFastTimer::DeclareTimer FTM_SWAP("Swap");

void render_hmd_mouse_cursor_3d()
{
    if (gHMD.isHMDMode() && !gHMD.cursorIntersectsUI())
    {
        LLViewerCamera* camera = LLViewerCamera::getInstance();
        LLVector3 origin = camera->getOrigin();
        LLVector3 pt;
        if (gHMD.cursorIntersectsWorld())
        {
            pt.set(gHMD.getMouseWorldRaycastIntersection().getF32ptr());
        }
        else
        {
            pt.set(gHMD.getMouseWorldEnd().getF32ptr());
        }
        LLVector3 delta = pt - origin;
        F32 dist = delta.magVec();
        F32 tanA = tanf(DEG_TO_RAD * gHMD.getWorldCursorSizeMult());
        F32 scalingFactor = dist * tanA;

        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();
        gGL.loadMatrix(gGLModelView);

        LLVertexBuffer::unbind();
        LLGLSUIDefault s1;
        LLGLDisable fog(GL_FOG);
        gPipeline.disableLights();

        LLViewerTexture* pCursorTexture = gHMD.getCursorImage((U32)gViewerWindow->getWindow()->getCursor());
        if (pCursorTexture)
        {
            if (LLGLSLShader::sNoFixedFunction)
            {
                gOneTextureNoColorProgram.bind();
            }
            gGL.setColorMask(true, false);
            gGL.color4f(1.0f,1.0f,1.0f,1.0f);
            gGL.getTexUnit(0)->bind(pCursorTexture);

            LLVector3 l	= camera->getLeftAxis() * scalingFactor;
            LLVector3 u	= camera->getUpAxis()   * scalingFactor;
            // mouse-pointer upper-left is at the intersection point,
            // thus the rect is offset right and down from the center
            LLVector3 bottomLeft	= pt - u;
            LLVector3 bottomRight	= pt - l - u;
            LLVector3 topLeft		= pt;
            LLVector3 topRight		= pt - l;
            // only go to 0.98 of the texture width so that annoying lines on right side are not drawn.
            // Even though the textures are blank on the right side, we still render a thin white line for no
            // apparent reason.  Only going to 0.98 of the texture width seems to solve this problem and since
            // no mouse cursor textures go all the way to the right side anyway, there's no loss of quality.
            gGL.begin( LLRender::TRIANGLE_STRIP );
                gGL.texCoord2f( 0.0f,  0.0f ); gGL.vertex3fv( bottomLeft.mV );
                gGL.texCoord2f( 0.98f, 0.0f	); gGL.vertex3fv( bottomRight.mV );
                gGL.texCoord2f( 0.0f,  1.0f ); gGL.vertex3fv( topLeft.mV );
            gGL.end();
            gGL.begin( LLRender::TRIANGLE_STRIP );
                gGL.texCoord2f( 0.98f, 0.0f	); gGL.vertex3fv( bottomRight.mV );
                gGL.texCoord2f( 0.98f, 1.0f ); gGL.vertex3fv( topRight.mV );
                gGL.texCoord2f( 0.0f,  1.0f ); gGL.vertex3fv( topLeft.mV );
            gGL.end();
            gGL.flush();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gOneTextureNoColorProgram.unbind();
            }
        }
        else
        {
            LLVector3 translate(gHMD.getMouseWorldRaycastIntersection().getF32ptr());
            gGL.translatef(translate.mV[0], translate.mV[1], translate.mV[2]);

            LLVector4a debug_binormal;
            debug_binormal.setCross3(gHMD.getMouseWorldRaycastNormal(), gHMD.getMouseWorldRaycastTangent());
            debug_binormal.mul(gHMD.getMouseWorldRaycastTangent().getF32ptr()[3]);
            LLVector3 normal(gHMD.getMouseWorldRaycastNormal().getF32ptr());
            LLVector3 binormal(debug_binormal.getF32ptr());

            LLCoordFrame orient;
            orient.lookDir(normal, binormal);
            LLMatrix4 rotation;
            orient.getRotMatrixToParent(rotation);
            gGL.multMatrix((float*)rotation.mMatrix);

            if (LLGLSLShader::sNoFixedFunction)
            {
		        gDebugProgram.bind();
            }
	        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

            gGL.diffuseColor4f(1,0,0,0.5f);
            drawBox(LLVector3::zero, LLVector3(0.1f * scalingFactor, 0.022f * scalingFactor, 0.022f * scalingFactor));
            gGL.diffuseColor4f(0,1,0,0.5f);
            drawBox(LLVector3::zero, LLVector3(0.021f * scalingFactor, 0.1f * scalingFactor, 0.021f * scalingFactor));
            gGL.diffuseColor4f(0,0,1,0.5f);
            drawBox(LLVector3::zero, LLVector3(0.02f * scalingFactor, 0.02f * scalingFactor, 0.1f * scalingFactor));

            gGL.flush();
            if (LLGLSLShader::sNoFixedFunction)
            {
		        gDebugProgram.unbind();
            }
        }
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }
}

void render_hmd_mouse_cursor_2d()
{
    if (gHMD.isHMDMode())
    {
        if (gHMD.cursorIntersectsUI())
        {
            gGL.pushMatrix();
            gGL.pushUIMatrix();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.bind();
            }
            S32 mx = gViewerWindow->getCurrentMouseX();
            S32 my = gViewerWindow->getCurrentMouseY();
            LLViewerTexture* pCursorTexture = gHMD.getCursorImage((U32)gViewerWindow->getWindow()->getCursor());
            if (pCursorTexture)
            {
                gl_draw_scaled_image(mx, my - 32, 32, 32, pCursorTexture);
            }
            else
            {
                gl_line_2d(mx - 10, my, mx + 10, my, LLColor4(1.0f, 0.0f, 0.0f));
                gl_line_2d(mx, my - 10, mx, my + 10, LLColor4(0.0f, 1.0f, 0.0f));
            }
            gGL.popUIMatrix();
            gGL.popMatrix();
            gGL.flush();
            if (LLGLSLShader::sNoFixedFunction)
            {
                gUIProgram.unbind();
            }
        }
    }
}


void render_ui(F32 zoom_factor, int subfield)
{
	LLGLState::checkStates();
	
	glh::matrix4f saved_view = glh_get_current_modelview();

	if (!gSnapshot)
	{
		gGL.pushMatrix();
		gGL.loadMatrix(gGLLastModelView);
		glh_set_current_modelview(glh_copy_matrix(gGLLastModelView));
	}
	
    BOOL to_texture = gPipeline.canUseVertexShaders() && LLPipeline::sRenderGlow;
	if (to_texture)
	{
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();
        gGL.loadIdentity();

		gPipeline.renderBloom(gSnapshot, zoom_factor, subfield);
        if (LLViewerCamera::sCurrentEye != LLViewerCamera::CENTER_EYE)
        {
            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.popMatrix();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.popMatrix();

            render_hud_elements();  // in-world text, labels, nametags
            if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
            {
                //LLFastTimer t(FTM_RENDER_UI);
                render_ui_3d(FALSE);
				LLGLState::checkStates();
            }

            render_hmd_mouse_cursor_3d();

            if (gPipeline.mUIScreen.isComplete())
            {
                if (gPipeline.mHMDUISurface.isNull())
                {
                    gPipeline.mHMDUISurface = gHMD.createUISurface();
                }
                gGL.matrixMode(LLRender::MM_MODELVIEW);
                gGL.pushMatrix();
                LLMatrix4 m1(gHMD.getUIModelViewInv());
                LLMatrix4 m2(gHMD.getBaseModelView());
                LLMatrix4 mt(0.0f, 0.0f, 0.0f, LLVector4(LLViewerCamera::sCurrentEye == LLViewerCamera::LEFT_EYE ? gHMD.getInterpupillaryOffset() : -gHMD.getInterpupillaryOffset(), 0.0f, gHMD.getUIEyeDepth(), 1.0f));
                m2 *= mt;
                m1 *= m2;
                gGL.loadMatrix((GLfloat*)m1.mMatrix);
                gOneTextureNoColorProgram.bind();
                gGL.setColorMask(true, true);
                gGL.getTexUnit(0)->bind(&gPipeline.mUIScreen);
                LLVertexBuffer* buff = gPipeline.mHMDUISurface;
                {
                    LLGLDisable cull(GL_CULL_FACE);
                    LLGLEnable blend(GL_BLEND);
                    buff->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0);
                    buff->drawRange(LLRender::TRIANGLES, 0, buff->getNumVerts()-1, buff->getNumIndices(), 0);
                }
                gOneTextureNoColorProgram.unbind();
                gGL.matrixMode(LLRender::MM_MODELVIEW);
                gGL.popMatrix();
            }
            else if (LLViewerCamera::sCurrentEye == LLViewerCamera::RIGHT_EYE)
            {
                if (!gPipeline.mUIScreen.allocate(gHMD.getHMDUIWidth(), gHMD.getHMDUIHeight(), GL_RGBA, FALSE, FALSE, LLTexUnit::TT_TEXTURE, TRUE))
                {
                    llwarns << "could not allocate UI buffer for HMD render mode" << LL_ENDL;
                    return;
                }
            }

            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.pushMatrix();
            gGL.loadIdentity();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.pushMatrix();
            gGL.loadIdentity();

            // handle HMD distortion and copying mScreen to framebuffer
            gPipeline.postRender(&gPipeline.mLeftEye, &gPipeline.mRightEye);
        }
        else
        {
            gPipeline.postRender();
        }
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
        LLVertexBuffer::unbind();
        LLGLState::checkStates();
        LLGLState::checkTextureChannels();
	}
    if (LLViewerCamera::sCurrentEye == LLViewerCamera::CENTER_EYE)
    {
        render_hud_elements();  // in-world text, labels, nametags
    }
    if (LLViewerCamera::sCurrentEye != LLViewerCamera::LEFT_EYE)
    {
        if (LLViewerCamera::sCurrentEye == LLViewerCamera::RIGHT_EYE)
        {
            gPipeline.mUIScreen.bindTarget();
            gGL.setColorMask(true, true);
            glClearColor(0.0f,0.0f,0.0f,0.0f);
            gPipeline.mUIScreen.clear();
            gGL.color4f(1,1,1,1);
            LLUI::setDestIsRenderTarget(TRUE);
            // this is necessary even though it theoretically already is using that blend type due to
            // setting the DestIsRenderTarget flag
            gGL.setSceneBlendType(LLRender::BT_ALPHA);
        }

        render_hud_attachments();   // huds worn by avatar
	    LLGLSDefault gls_default;
	    LLGLSUIDefault gls_ui;
	    gPipeline.disableLights();
        gGL.setColorMask(true, gHMD.isHMDMode());

        if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	    {
		    LLFastTimer t(FTM_RENDER_UI);
		    if (!gDisconnected)
		    {
                render_ui_3d(TRUE); // edit outline, move arrows, selection highlighting, debug axes, etc.
			    LLGLState::checkStates();
		    }
		    else if (LLViewerCamera::sCurrentEye == LLViewerCamera::CENTER_EYE)
		    {
			    render_disconnected_background();
		    }
            render_ui_2d(); // Side/bottom buttons, 2D UI windows, etc.
		    LLGLState::checkStates();
	    }
	    gGL.flush();
        gViewerWindow->setup2DRender();
        gViewerWindow->updateDebugText();
        gViewerWindow->drawDebugText(); // debugging text

        render_hmd_mouse_cursor_2d();

        if (LLViewerCamera::sCurrentEye == LLViewerCamera::RIGHT_EYE)
        {
            gPipeline.mUIScreen.flush();
            if (LLRenderTarget::sUseFBO)
            {
                //copy depth buffer from mScreen to framebuffer
                LLRenderTarget::copyContentsToFramebuffer(gPipeline.mScreen, 0, 0, gPipeline.mScreen.getWidth(), gPipeline.mScreen.getHeight(), 
                    0, 0, gPipeline.mScreen.getWidth(), gPipeline.mScreen.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            }
            LLUI::setDestIsRenderTarget(FALSE);
        }
    }

    // copy 
	LLVertexBuffer::unbind();
	if (!gSnapshot)
	{
		glh_set_current_modelview(saved_view);
		gGL.popMatrix();
	}
	if (gDisplaySwapBuffers && LLViewerCamera::sCurrentEye != LLViewerCamera::LEFT_EYE)
	{
		LLFastTimer t(FTM_SWAP);
		gViewerWindow->getWindow()->swapBuffers();
	}
	gDisplaySwapBuffers = TRUE;
}

void renderCoordinateAxes()
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.begin(LLRender::LINES);
		gGL.color3f(1.0f, 0.0f, 0.0f);   // i direction = X-Axis = red
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(2.0f, 0.0f, 0.0f);
		gGL.vertex3f(3.0f, 0.0f, 0.0f);
		gGL.vertex3f(5.0f, 0.0f, 0.0f);
		gGL.vertex3f(6.0f, 0.0f, 0.0f);
		gGL.vertex3f(8.0f, 0.0f, 0.0f);
		// Make an X
		gGL.vertex3f(11.0f, 1.0f, 1.0f);
		gGL.vertex3f(11.0f, -1.0f, -1.0f);
		gGL.vertex3f(11.0f, 1.0f, -1.0f);
		gGL.vertex3f(11.0f, -1.0f, 1.0f);

		gGL.color3f(0.0f, 1.0f, 0.0f);   // j direction = Y-Axis = green
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 2.0f, 0.0f);
		gGL.vertex3f(0.0f, 3.0f, 0.0f);
		gGL.vertex3f(0.0f, 5.0f, 0.0f);
		gGL.vertex3f(0.0f, 6.0f, 0.0f);
		gGL.vertex3f(0.0f, 8.0f, 0.0f);
		// Make a Y
		gGL.vertex3f(1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(-1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, -1.0f);

		gGL.color3f(0.0f, 0.0f, 1.0f);   // Z-Axis = blue
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 2.0f);
		gGL.vertex3f(0.0f, 0.0f, 3.0f);
		gGL.vertex3f(0.0f, 0.0f, 5.0f);
		gGL.vertex3f(0.0f, 0.0f, 6.0f);
		gGL.vertex3f(0.0f, 0.0f, 8.0f);
		// Make a Z
		gGL.vertex3f(-1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(1.0f, -1.0f, 11.0f);
	gGL.end();
}


void draw_axes() 
{
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	// A vertical white line at origin
	LLVector3 v = gAgent.getPositionAgent();
	gGL.begin(LLRender::LINES);
		gGL.color3f(1.0f, 1.0f, 1.0f); 
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 40.0f);
	gGL.end();
	// Some coordinate axes
	gGL.pushMatrix();
		gGL.translatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
		renderCoordinateAxes();
	gGL.popMatrix();
}

void render_ui_3d(BOOL hmdUIMode)
{
	LLGLSPipeline gls_pipeline;

	//////////////////////////////////////
	//
	// Render 3D UI elements
	// NOTE: zbuffer is cleared before we get here by LLDrawPoolHUD,
	//		 so 3d elements requiring Z buffer are moved to LLDrawPoolHUD
	//

	/////////////////////////////////////////////////////////////
	//
	// Render 2.5D elements (2D elements in the world)
	// Stuff without z writes
	//

	// Debugging stuff goes before the UI.

	stop_glerror();
	
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

    if (!gHMD.isHMDMode() || !hmdUIMode)
    {
	    // Coordinate axes
	    if (gSavedSettings.getBOOL("ShowAxes"))
	    {
		    draw_axes();
	    }
    }

    // render HUD selections/highlights
    gViewerWindow->renderSelections(gHMD.isHMDMode() ? hmdUIMode : TRUE);

    stop_glerror();

    if (LLGLSLShader::sNoFixedFunction)
    {
        gUIProgram.unbind();
    }
}

void render_ui_2d()
{
	LLGLSUIDefault gls_ui;

	/////////////////////////////////////////////////////////////
	//
	// Render 2D UI elements that overlay the world (no z compare)

	//  Disable wireframe mode below here, as this is HUD/menus
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//  Menu overlays, HUD, etc

    if (LLViewerCamera::sCurrentEye == LLViewerCamera::CENTER_EYE)
    {
        gViewerWindow->setup2DRender();
    }
    else if (LLViewerCamera::sCurrentEye == LLViewerCamera::RIGHT_EYE)
    {
        gViewerWindow->setup2DRender(0, 0, gHMD.getHMDUIWidth(), gHMD.getHMDUIHeight());
    }

	F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
	S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();

	if (zoom_factor > 1.f)
	{
		//decompose subregion number to x and y values
		int pos_y = sub_region / llceil(zoom_factor);
		int pos_x = sub_region - (pos_y*llceil(zoom_factor));
		// offset for this tile
		LLFontGL::sCurOrigin.mX -= llround((F32)gViewerWindow->getWindowWidthScaled() * (F32)pos_x / zoom_factor);
		LLFontGL::sCurOrigin.mY -= llround((F32)gViewerWindow->getWindowHeightScaled() * (F32)pos_y / zoom_factor);
	}

	stop_glerror();
	//gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

	// render outline for HUD
	if (isAgentAvatarValid() && gAgentCamera.mHUDCurZoom < 0.98f)
	{
		gGL.pushMatrix();
        S32 half_width  = gViewerWindow->getWorldViewWidthScaled() / 2;
		S32 half_height = gViewerWindow->getWorldViewWidthScaled() / 2;
		gGL.scalef(LLUI::getScaleFactor().mV[0], LLUI::getScaleFactor().mV[1], 1.f);
		gGL.translatef((F32)half_width, (F32)half_height, 0.f);
		F32 zoom = gAgentCamera.mHUDCurZoom;
		gGL.scalef(zoom,zoom,1.f);
		gGL.color4fv(LLColor4::white.mV);
		gl_rect_2d(-half_width, half_height, half_width, -half_height, FALSE);
		gGL.popMatrix();
		stop_glerror();
	}

    gViewerWindow->draw();

	// reset current origin for font rendering, in case of tiling render
	LLFontGL::sCurOrigin.set(0, 0);
}

void render_disconnected_background()
{
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

	gGL.color4f(1,1,1,1);
	if (!gDisconnectedImagep && gDisconnected)
	{
		llinfos << "Loading last bitmap..." << llendl;

		std::string temp_str;
		temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + SCREEN_LAST_FILENAME;

		LLPointer<LLImageBMP> image_bmp = new LLImageBMP;
		if( !image_bmp->load(temp_str) )
		{
			//llinfos << "Bitmap load failed" << llendl;
			return;
		}
		
		LLPointer<LLImageRaw> raw = new LLImageRaw;
		if (!image_bmp->decode(raw, 0.0f))
		{
			llinfos << "Bitmap decode failed" << llendl;
			gDisconnectedImagep = NULL;
			return;
		}

		U8 *rawp = raw->getData();
		S32 npixels = (S32)image_bmp->getWidth()*(S32)image_bmp->getHeight();
		for (S32 i = 0; i < npixels; i++)
		{
			S32 sum = 0;
			sum = *rawp + *(rawp+1) + *(rawp+2);
			sum /= 3;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
		}

		
		raw->expandToPowerOfTwo();
		gDisconnectedImagep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE );
		gStartTexture = gDisconnectedImagep;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}

	// Make sure the progress view always fills the entire window.
	S32 width = gViewerWindow->getWindowWidthScaled();
	S32 height = gViewerWindow->getWindowHeightScaled();

	if (gDisconnectedImagep)
	{
		LLGLSUIDefault gls_ui;
		gViewerWindow->setup2DRender();
		gGL.pushMatrix();
		{
			// scale ui to reflect UIScaleFactor
			// this can't be done in setup2DRender because it requires a
			// pushMatrix/popMatrix pair
			const LLVector2& display_scale = gViewerWindow->getDisplayScale();
			gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

			gGL.getTexUnit(0)->bind(gDisconnectedImagep);
			gGL.color4f(1.f, 1.f, 1.f, 1.f);
			gl_rect_2d_simple_tex(width, height);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
		gGL.popMatrix();
	}
	gGL.flush();

	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.unbind();
	}

}

void display_cleanup()
{
	gDisconnectedImagep = NULL;
}
