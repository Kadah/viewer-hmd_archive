/** 
 * @file llviewerdisplay.h
 * @brief LLViewerDisplay class header file
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

#ifndef LL_LLVIEWERDISPLAY_H
#define LL_LLVIEWERDISPLAY_H

class LLPostProcess;
class LLCullResult;

struct ui_render_options
{
    ui_render_options()
        : zoom_factor(1.0f)
        , subfield(0)
        , do_hud_elements(FALSE)
        , do_hud_attach(FALSE)
        , render_3d_ui(FALSE)
        , for_hmd(FALSE)
        , hmd_eye(-1)
        , hmd_pre_post(FALSE)
    {
    }

    F32 zoom_factor;
    int subfield;
    BOOL do_hud_attach;
    BOOL do_hud_elements;
    BOOL render_3d_ui;
    BOOL for_hmd;
    int  hmd_eye;
    BOOL hmd_pre_post;
};

class LLViewerDisplay
{
public:
    static void display_startup();
    static void update_camera();
    static void display(BOOL rebuild = TRUE, F32 zoom_factor = 1.f, int subfield = 0, BOOL for_snapshot = FALSE);
    static void render_ui(ui_render_options& options);    
    static void swap(BOOL doSwap, BOOL newSwap);

    // Utility
    static void push_state_gl_identity();
    static void push_state_gl();
    static void pop_state_gl();

    static void display_cleanup();

private:
    static void render_ui_3d(BOOL showAxes = FALSE);
    static void render_ui_2d(BOOL forHMD = FALSE);

    // Rendering stuff
    static void display_stats();
    static void update();
    static S32 cull(LLCullResult& cullResult);
    static void display_swap();
    static void display_imagery();
    static void update_images();
    static void state_sort(BOOL rebuild, LLCullResult& cullResult);
    static void render_start(BOOL to_texture);
    static void render_geom();
    static void render_flush(BOOL to_texture);
    static void render_hud_attachments();
    static BOOL setup_hud_matrices();
    static void renderCoordinateAxes();
    static void draw_axes();
    static void render_disconnected_background();
    static void render_frame(BOOL rebuild, BOOL forHMD = FALSE, int whichEye = -1);

public:
    static BOOL gDisplaySwapBuffers;
    static BOOL gDepthDirty;
    static BOOL gTeleportDisplay;
    static LLFrameTimer	gTeleportDisplayTimer;
    static BOOL gForceRenderLandFence;
    static BOOL gResizeScreenTexture;
    static BOOL gWindowResized;
    static BOOL gShaderProfileFrame;

private:
    static LLFrameTimer gTeleportArrivalTimer;
    static BOOL gSnapshot;
    static U32 gRecentFrameCount; // number of 'recent' frames
    static LLFrameTimer gRecentFPSTime;
    static LLFrameTimer gRecentMemoryTime;
};

#endif // LL_LLVIEWERDISPLAY_H
