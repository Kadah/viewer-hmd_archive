/**
 * @file llwindowmacosx.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llwindowmacosx.h"

#include "llkeyboardmacosx.h"
#include "llwindowcallbacks.h"
#include "llpreeditor.h"

#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "indra_constants.h"

#include <OpenGL/OpenGL.h>
#include <CoreServices/CoreServices.h>

extern BOOL gDebugWindowProc;

// culled from winuser.h
//const S32	WHEEL_DELTA = 120;     /* Value for rolling one detent */
// On the Mac, the scroll wheel reports a delta of 1 for each detent.
// There's also acceleration for faster scrolling, based on a slider in the system preferences.
const S32	WHEEL_DELTA = 1;     /* Value for rolling one detent */
const S32	BITS_PER_PIXEL = 32;
const S32	MAX_NUM_RESOLUTIONS = 32;


//
// LLWindowMacOSX
//

BOOL LLWindowMacOSX::sUseMultGL = FALSE;

// Cross-platform bits:

BOOL check_for_card(const char* RENDERER, const char* bad_card)
{
	if (!strnicmp(RENDERER, bad_card, strlen(bad_card)))
	{
		std::string buffer = llformat(
			"Your video card appears to be a %s, which Second Life does not support.\n"
			"\n"
			"Second Life requires a video card with 32 Mb of memory or more, as well as\n"
			"multitexture support.  We explicitly support nVidia GeForce 2 or better, \n"
			"and ATI Radeon 8500 or better.\n"
			"\n"
			"If you own a supported card and continue to receive this message, try \n"
			"updating to the latest video card drivers. Otherwise look in the\n"
			"secondlife.com support section or e-mail technical support\n"
			"\n"
			"You can try to run Second Life, but it will probably crash or run\n"
			"very slowly.  Try anyway?",
			bad_card);
		S32 button = OSMessageBox(buffer.c_str(), "Unsupported video card", OSMB_YESNO);
		if (OSBTN_YES == button)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	return FALSE;
}

// Switch to determine whether we capture all displays, or just the main one.
// We may want to base this on the setting of _DEBUG...

#define CAPTURE_ALL_DISPLAYS 0
//static double getDictDouble (CFDictionaryRef refDict, CFStringRef key);
static long getDictLong (CFDictionaryRef refDict, CFStringRef key);

// MBW -- HACK ALERT
// On the Mac, to put up an OS dialog in full screen mode, we must first switch OUT of full screen mode.
// The proper way to do this is to bracket the dialog with calls to beforeDialog() and afterDialog(), but these
// require a pointer to the LLWindowMacOSX object.  Stash it here and maintain in the constructor and destructor.
// This assumes that there will be only one object of this class at any time.  Hopefully this is true.
static LLWindowMacOSX *gWindowImplementation = NULL;

LLWindowMacOSX::LLWindowMacOSX(LLWindowCallbacks* callbacks,
							   const std::string& title, const std::string& name, S32 x, S32 y, S32 width,
							   S32 height, U32 flags,
							   BOOL fullscreen, BOOL clearBg,
							   BOOL disable_vsync, BOOL use_gl,
							   BOOL ignore_pixel_depth,
							   U32 fsaa_samples)
	: LLWindow(NULL, fullscreen, flags)
{
	// *HACK: During window construction we get lots of OS events for window
	// reshape, activate, etc. that the viewer isn't ready to handle.
	// Route them to a dummy callback structure until the end of constructor.
	LLWindowCallbacks null_callbacks;
	mCallbacks = &null_callbacks;

	// Voodoo for calling cocoa from carbon (see llwindowmacosx-objc.mm).
	setupCocoa();

	// Initialize the keyboard
	gKeyboard = new LLKeyboardMacOSX();
	gKeyboard->setCallbacks(callbacks);

	// Ignore use_gl for now, only used for drones on PC
	mWindow[0] = mWindow[1] = NULL;
    mGLView[0] = mGLView[1] = NULL;
    mCurRCIdx = 0;
    mHMDMode = FALSE;
    mHMDSize[0] = mHMDSize[1] = 0;
    mHMDScale[0] = mHMDScale[1] = 1.0f;
	mContext = NULL;
	mPixelFormat = NULL;
	mDisplay = CGMainDisplayID();
	mSimulatedRightClick = FALSE;
	mLastModifiers = 0;
	mHandsOffEvents = FALSE;
	mCursorDecoupled = FALSE;
	mCursorLastEventDeltaX = 0;
	mCursorLastEventDeltaY = 0;
	mCursorIgnoreNextDelta = FALSE;
	mNeedsResize = FALSE;
	mOverrideAspectRatio = 0.f;
	mMaximized = FALSE;
	mMinimized = FALSE;
	mLanguageTextInputAllowed = FALSE;
	mPreeditor = NULL;
	mFSAASamples = fsaa_samples;
	mForceRebuild = FALSE;

	// Get the original aspect ratio of the main device.
	mOriginalAspectRatio = (double)CGDisplayPixelsWide(mDisplay) / (double)CGDisplayPixelsHigh(mDisplay);

	// Stash the window title
	mWindowTitle = title;
	//mWindowTitle[0] = title.length();

	mDragOverrideCursor = -1;

	// Set up global event handlers (the fullscreen case needs this)
	//InstallStandardEventHandler(GetApplicationEventTarget());

	// Stash an object pointer for OSMessageBox()
	gWindowImplementation = this;
	// Create the GL context and set it up for windowed or fullscreen, as appropriate.
	if(createContext(x, y, width, height, 32, fullscreen, disable_vsync))
	{
		if (mWindow[0] != NULL)
		{
			makeWindowOrderFront(mWindow[0]);
		}

		if (!gGLManager.initGL())
		{
			setupFailure(
				"Second Life is unable to run because your video card drivers\n"
				"are out of date or unsupported. Please make sure you have\n"
				"the latest video card drivers installed.\n"
				"If you continue to receive this message, contact customer service.",
				"Error",
				OSMB_OK);
			return;
		}

		//start with arrow cursor
		initCursors();
		setCursor( UI_CURSOR_ARROW );
		
		allowLanguageTextInput(NULL, FALSE);
	}

	mCallbacks = callbacks;
	stop_glerror();
	
	
}

void LLWindowMacOSX::adjustPosForHMDScaling(LLCoordGL& pt)
{
    if (mHMDMode)
    {
        if (mHMDScale[0] != 0.0f)
        {
            pt.mX = llround((F32)pt.mX / mHMDScale[0]);
        }
        if (mHMDScale[1] != 0.0f)
        {
            pt.mY = llround((F32)pt.mY / mHMDScale[1]);
        }
    }
}

// These functions are used as wrappers for our internal event handling callbacks.
// It's a good idea to wrap these to avoid reworking more code than we need to within LLWindow.

bool callKeyUp(unsigned short key, unsigned int mask)
{
	return gKeyboard->handleKeyUp(key, mask);
}

bool callKeyDown(unsigned short key, unsigned int mask)
{
	return gKeyboard->handleKeyDown(key, mask);
}

void callResetKeys()
{
	gKeyboard->resetKeys();
}

bool callUnicodeCallback(wchar_t character, unsigned int mask)
{
	return gWindowImplementation->getCallbacks()->handleUnicodeChar(character, mask);
}

void callFocus()
{
	if (gWindowImplementation)
	{
		gWindowImplementation->getCallbacks()->handleFocus(gWindowImplementation);
	}
}

void callFocusLost()
{
	gWindowImplementation->getCallbacks()->handleFocusLost(gWindowImplementation);
}

void callRightMouseDown(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }
    
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleRightMouseDown(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
}

void callRightMouseUp(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }
    
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleRightMouseUp(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
}

void callLeftMouseDown(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }
    
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleMouseDown(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
}

void callLeftMouseUp(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }
    
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleMouseUp(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
	
}

void callDoubleClick(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }
    
	LLCoordGL	outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleDoubleClick(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
}

void callResize(unsigned int width, unsigned int height)
{
	if (gWindowImplementation != NULL)
	{
		gWindowImplementation->getCallbacks()->handleResize(gWindowImplementation, width, height);
	}
}

void callMouseMoved(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	float deltas[2];
	gWindowImplementation->getMouseDeltas(deltas);
	outCoords.mX += deltas[0];
	outCoords.mY += deltas[1];
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleMouseMove(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
	//gWindowImplementation->getCallbacks()->handleScrollWheel(gWindowImplementation, 0);
}

void callScrollMoved(float delta)
{
	gWindowImplementation->getCallbacks()->handleScrollWheel(gWindowImplementation, delta);
}

void callMouseExit()
{
	gWindowImplementation->getCallbacks()->handleMouseLeave(gWindowImplementation);
}

void callWindowFocus()
{
   if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
	{
		gWindowImplementation->getCallbacks()->handleFocus (gWindowImplementation);
	}
	else
	{
		LL_WARNS("COCOA") << "Window Implementation or callbacks not yet initialized." << LL_ENDL;
	}


}

void callWindowUnfocus()
{
	gWindowImplementation->getCallbacks()->handleFocusLost(gWindowImplementation);
}

void callDeltaUpdate(float *delta, MASK mask)
{
	gWindowImplementation->updateMouseDeltas(delta);
}

void callMiddleMouseDown(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	float deltas[2];
	gWindowImplementation->getMouseDeltas(deltas);
	outCoords.mX += deltas[0];
	outCoords.mY += deltas[1];
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleMiddleMouseDown(gWindowImplementation, outCoords, mask);
}

void callMiddleMouseUp(float *pos, MASK mask)
{
	LLCoordGL outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	float deltas[2];
	gWindowImplementation->getMouseDeltas(deltas);
	outCoords.mX += deltas[0];
	outCoords.mY += deltas[1];
    gWindowImplementation->adjustPosForHMDScaling(outCoords);
	gWindowImplementation->getCallbacks()->handleMiddleMouseUp(gWindowImplementation, outCoords, mask);
}

void callModifier(MASK mask)
{
	gKeyboard->handleModifier(mask);
}

void callHandleDragEntered(std::string url)
{
	gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_START_TRACKING);
}

void callHandleDragExited(std::string url)
{
	gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_STOP_TRACKING);
}

void callHandleDragUpdated(std::string url)
{
	gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_TRACK);
}

void callHandleDragDropped(std::string url)
{
	gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_DROPPED);
}

void callQuitHandler()
{
	if (gWindowImplementation)
	{
		if(gWindowImplementation->getCallbacks()->handleCloseRequest(gWindowImplementation))
		{
			gWindowImplementation->getCallbacks()->handleQuit(gWindowImplementation);
		}
	}
}

void getPreeditSelectionRange(int *position, int *length)
{
	if (gWindowImplementation->getPreeditor())
	{
		gWindowImplementation->getPreeditor()->getSelectionRange(position, length);
	}
}

void getPreeditMarkedRange(int *position, int *length)
{
	if (gWindowImplementation->getPreeditor())
	{
		gWindowImplementation->getPreeditor()->getPreeditRange(position, length);
	}
}

void setPreeditMarkedRange(int position, int length)
{
	if (gWindowImplementation->getPreeditor())
	{
		gWindowImplementation->getPreeditor()->markAsPreedit(position, length);
	}
}

bool handleUnicodeCharacter(wchar_t c)
{
    bool success = false;
	if (gWindowImplementation->getPreeditor())
	{
        success = gWindowImplementation->getPreeditor()->handleUnicodeCharHere(c);
	}
    
    return success;
}

void resetPreedit()
{
	if (gWindowImplementation->getPreeditor())
	{
		gWindowImplementation->getPreeditor()->resetPreedit();
	}
}

// For reasons of convenience, handle IME updates here.
// This largely mirrors the old implementation, only sans the carbon parameters.
void setMarkedText(unsigned short *unitext, unsigned int *selectedRange, unsigned int *replacementRange, long text_len, attributedStringInfo segments)
{
	if (gWindowImplementation->getPreeditor())
	{
		LLPreeditor *preeditor = gWindowImplementation->getPreeditor();
		preeditor->resetPreedit();
		// This should be a viable replacement for the kEventParamTextInputSendReplaceRange parameter.
		if (replacementRange[0] < replacementRange[1])
		{
			const LLWString& text = preeditor->getPreeditString();
			const S32 location = wstring_wstring_length_from_utf16_length(text, 0, replacementRange[0]);
			const S32 length = wstring_wstring_length_from_utf16_length(text, location, replacementRange[1]);
			preeditor->markAsPreedit(location, length);
		}
		
		LLWString fix_str = utf16str_to_wstring(llutf16string(unitext, text_len));
		
		S32 caret_position = fix_str.length();
		
		preeditor->updatePreedit(fix_str, segments.seg_lengths, segments.seg_standouts, caret_position);
	}
}

void getPreeditLocation(float *location, unsigned int length)
{
	if (gWindowImplementation->getPreeditor())
	{
		LLPreeditor *preeditor = gWindowImplementation->getPreeditor();
		LLCoordGL coord;
		LLCoordScreen screen;
		LLRect rect;
		
		preeditor->getPreeditLocation(length, &coord, &rect, NULL);
		
		float c[4] = {coord.mX, coord.mY, 0, 0};
		
		convertRectToScreen(gWindowImplementation->getWindow(), c);
		
		location[0] = c[0];
		location[1] = c[1];
	}
}

void LLWindowMacOSX::updateMouseDeltas(float* deltas)
{
	if (mCursorDecoupled)
	{
		mCursorLastEventDeltaX = llround(deltas[0]);
		mCursorLastEventDeltaY = llround(-deltas[1]);
		
		if (mCursorIgnoreNextDelta)
		{
			mCursorLastEventDeltaX = 0;
			mCursorLastEventDeltaY = 0;
			mCursorIgnoreNextDelta = FALSE;
		}
	} else {
		mCursorLastEventDeltaX = 0;
		mCursorLastEventDeltaY = 0;
	}
}

void LLWindowMacOSX::getMouseDeltas(float* delta)
{
	delta[0] = mCursorLastEventDeltaX;
	delta[1] = mCursorLastEventDeltaY;
}

BOOL LLWindowMacOSX::createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync)
{
	BOOL			glNeedsInit = FALSE;

	mFullscreen = fullscreen;
	
	if (mWindow[0] == NULL)
	{
		mWindow[0] = getMainAppWindow();
	}

	if(mContext == NULL)
	{
		// Our OpenGL view is already defined within SecondLife.xib.
		// Get the view instead.
		mGLView[0] = createOpenGLView(mWindow[0], mFSAASamples, !disable_vsync);
		mContext = getCGLContextObj(mGLView[0]);
		
		// Since we just created the context, it needs to be set up.
		glNeedsInit = TRUE;
		
		gGLManager.mVRAM = getVramSize(mGLView[0]);
	}
	
	// This sets up our view to recieve text from our non-inline text input window.
	setupInputWindow(mWindow[0], mGLView[0]);
	
	// Hook up the context to a drawable

	if(mContext != NULL)
	{
		U32 err = CGLSetCurrentContext(mContext);
		if (err != kCGLNoError)
		{
			setupFailure("Can't activate GL rendering context", "Error", OSMB_OK);
			return FALSE;
		}
	}

    enableVSync(!disable_vsync);

	//enable multi-threaded OpenGL
	if (sUseMultGL)
	{
		CGLError cgl_err;
		CGLContextObj ctx = CGLGetCurrentContext();

		cgl_err =  CGLEnable( ctx, kCGLCEMPEngine);

		if (cgl_err != kCGLNoError )
		{
			LL_DEBUGS("GLInit") << "Multi-threaded OpenGL not available." << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("GLInit") << "Multi-threaded OpenGL enabled." << LL_ENDL;
		}
	}
	makeFirstResponder(mWindow[0], mGLView[0]);
    
	return TRUE;
}

// We only support OS X 10.7's fullscreen app mode which is literally a full screen window that fills a virtual desktop.
// This makes this method obsolete.
BOOL LLWindowMacOSX::switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp)
{
	return FALSE;
}

void LLWindowMacOSX::destroyContext()
{
	if (!mContext)
	{
		// We don't have a context
		return;
	}
	// Unhook the GL context from any drawable it may have
	if(mContext != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: unhooking drawable " << LL_ENDL;
		CGLSetCurrentContext(NULL);
	}

	// Clean up remaining GL state before blowing away window
	gGLManager.shutdownGL();

	// Clean up the pixel format
	if(mPixelFormat != NULL)
	{
		CGLDestroyPixelFormat(mPixelFormat);
		mPixelFormat = NULL;
	}

	// Clean up the GL context
	if(mContext != NULL)
	{
		CGLDestroyContext(mContext);
	}
	
	// Destroy our LLOpenGLView
	if (mGLView[mCurRCIdx] != NULL)
	{
		removeGLView(mGLView[mCurRCIdx]);
		mGLView[mCurRCIdx] = NULL;
	}
	
	// Close the window
	if(mWindow[mCurRCIdx] != NULL)
	{
        NSWindowRef dead_window = mWindow[mCurRCIdx];
        mWindow[mCurRCIdx] = NULL;
		closeWindow(dead_window);
	}

}

LLWindowMacOSX::~LLWindowMacOSX()
{
	destroyContext();

	if(mSupportedResolutions != NULL)
	{
		delete []mSupportedResolutions;
	}

	gWindowImplementation = NULL;

}


void LLWindowMacOSX::show()
{
}

void LLWindowMacOSX::hide()
{
	setMouseClipping(FALSE);
}

//virtual
void LLWindowMacOSX::minimize()
{
	setMouseClipping(FALSE);
	showCursor();
}

//virtual
void LLWindowMacOSX::restore()
{
	show();
}


// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowMacOSX::close()
{
	// Is window is already closed?
	//	if (!mWindow)
	//	{
	//		return;
	//	}

	// Make sure cursor is visible and we haven't mangled the clipping state.
	setMouseClipping(FALSE);
	showCursor();

	destroyContext();
}

BOOL LLWindowMacOSX::isValid()
{
	return (mFullscreen || (mWindow[mCurRCIdx] != NULL));
}

BOOL LLWindowMacOSX::getVisible()
{
	return (mFullscreen || (mWindow[mCurRCIdx] != NULL));
}

BOOL LLWindowMacOSX::getMinimized()
{
	return mMinimized;
}

BOOL LLWindowMacOSX::getMaximized()
{
	return mMaximized;
}

BOOL LLWindowMacOSX::maximize()
{
	if (mWindow[mCurRCIdx] && !mMaximized)
	{
        // *TODO: Implement maximize code or simplify this method
	}

	return mMaximized;
}

BOOL LLWindowMacOSX::getFullscreen()
{
	return mFullscreen;
}

void LLWindowMacOSX::gatherInput()
{
	updateCursor();
}

BOOL LLWindowMacOSX::getPosition(LLCoordScreen *position)
{
	S32 err = -1;

	if(mFullscreen && !mHMDMode)
	{
		position->mX = 0;
		position->mY = 0;
		err = noErr;
	}
	else if (mWindow[mCurRCIdx])
	{
        float rect[4];
		getContentViewBounds(mWindow[mCurRCIdx], rect);

		position->mX = rect[0];
		position->mY = rect[1];
		err = noErr;
	}
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}

	return (err == noErr);
}

// returns the upper-left screen coordinates for the window frame (including the title bar and any borders)
BOOL LLWindowMacOSX::getFramePos(LLCoordScreen* pos)
{
    if (pos)
    {
        if (mFullscreen && !mHMDMode)
        {
            pos->mX = pos->mY = 0;
            return TRUE;
        }
        else if (mWindow[mCurRCIdx])
        {
            float rect[4];
            getWindowSize(mWindow[mCurRCIdx], rect);
            pos->mX = llround(rect[0]);
            pos->mY = llround(rect[1]);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL LLWindowMacOSX::getSize(LLCoordScreen *size)
{
	S32 err = -1;

    if (mFullscreen && !mHMDMode)
    {
        size->mX = mFullscreenWidth;
        size->mY = mFullscreenHeight;
        err = noErr;
    }
    else if (mWindow[mCurRCIdx])
    {
        float rect[4];
		getContentViewBounds(mWindow[mCurRCIdx], rect);
        S32 sz[2] = { llround(rect[2]), llround(rect[3]) };
        if (mHMDMode)
        {
            size->mX = llmin(sz[0], mHMDSize[0]);
            size->mY = llmin(sz[1], mHMDSize[1]);
        }
        else
        {
            size->mX = sz[0];
            size->mY = sz[1];
        }
        err = noErr;
    }
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}

	return (err == noErr);
}

BOOL LLWindowMacOSX::getSize(LLCoordWindow *size)
{
	S32 err = -1;

    if (mFullscreen && !mHMDMode)
    {
        size->mX = mFullscreenWidth;
        size->mY = mFullscreenHeight;
        err = noErr;
    }
    else if (mWindow[mCurRCIdx])
    {
        float rect[4];
		getContentViewBounds(mWindow[mCurRCIdx], rect);
        S32 sz[2] = { llround(rect[2]), llround(rect[3]) };
        if (mHMDMode)
        {
            size->mX = llmin(sz[0], mHMDSize[0]);
            size->mY = llmin(sz[1], mHMDSize[1]);
        }
        else
        {
            size->mX = sz[0];
            size->mY = sz[1];
        }
        err = noErr;
    }
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}
	
	return (err == noErr);
}

BOOL LLWindowMacOSX::setPosition(const LLCoordScreen position)
{
	if (mWindow[mCurRCIdx])
	{
        float pos[2] = {position.mX, position.mY};
		setWindowPos(mWindow[mCurRCIdx], pos);
        return TRUE;
	}

	return FALSE;
}

void LLWindowMacOSX::adjustWindowToFitScreen(LLCoordWindow& size)
{
    if (!mWindow[mCurRCIdx] || mFullscreen)
    {
        return;
    }

    float winBounds[4], screenBounds[4], initialPos[2];
    getWindowSize(mWindow[mCurRCIdx], winBounds);
    initialPos[0] = winBounds[0];
    initialPos[1] = winBounds[1];
    winBounds[2] = (F32)size.mX;
    winBounds[3] = (F32)size.mY;
    int screen_id = getScreenFromPoint(winBounds);
    if (screen_id >= 0)
    {
        getScreenSize(screen_id, screenBounds);

        // check to see if window is too big for current screen
        winBounds[2] = llmin(winBounds[2], screenBounds[2]);
        winBounds[3] = llmin(winBounds[3], screenBounds[3]);
        if (winBounds[0] < screenBounds[0])
        {
            winBounds[0] = screenBounds[0];
        }
        else if ((winBounds[0] + winBounds[2]) > (screenBounds[0] + screenBounds[2]))
        {
            winBounds[0] = (screenBounds[0] + screenBounds[2]) - winBounds[2];
            if (winBounds[0] < screenBounds[0])
            {
                winBounds[0] = screenBounds[0];
                winBounds[2] = screenBounds[2];
            }
        }

        // now ensure that window position (with adjusted size) fits on the screen
        if (winBounds[1] < screenBounds[1])
        {
            winBounds[1] = screenBounds[1];
        }
        else if ((winBounds[1] + winBounds[3]) > (screenBounds[1] + screenBounds[3]))
        {
            winBounds[1] = (screenBounds[1] + screenBounds[3]) - winBounds[3];
            if (winBounds[1] < screenBounds[1])
            {
                winBounds[1] = screenBounds[1];
                winBounds[3] = screenBounds[3];
            }
        }
        if (!is_approx_equal(initialPos[0], winBounds[0]) || !is_approx_equal(initialPos[1], winBounds[1]))
        {
            setWindowPos(mWindow[mCurRCIdx], winBounds);
        }
        size.mX = llround(winBounds[2]);
        size.mY = llround(winBounds[3]);
    }
}

BOOL LLWindowMacOSX::setSizeImpl(const LLCoordScreen size, BOOL adjustPosition)
{
	if (mWindow[mCurRCIdx])
	{
        LLCoordWindow to;
        convertCoords(size, &to);
        if (adjustPosition && !mFullscreen)
        {
            adjustWindowToFitScreen(to);
        }
		setWindowSize(mWindow[mCurRCIdx], to.mX, to.mY);
        return TRUE;
	}

	return FALSE;
}

BOOL LLWindowMacOSX::setSizeImpl(const LLCoordWindow size, BOOL adjustPosition)
{
	if (mWindow[mCurRCIdx])
	{
        LLCoordWindow actualSize(size);
        actualSize.mY += (mFullscreen ? 0 : 22);
        if (adjustPosition && !mFullscreen)
        {
            adjustWindowToFitScreen(actualSize);
        }
        setWindowSize(mWindow[mCurRCIdx], actualSize.mX, actualSize.mY);
        return TRUE;
	}
    
	return FALSE;
}

void LLWindowMacOSX::swapBuffers()
{
	CGLFlushDrawable(mContext);
}

F32 LLWindowMacOSX::getGamma()
{
	F32 result = 2.2;	// Default to something sane

	CGGammaValue redMin;
	CGGammaValue redMax;
	CGGammaValue redGamma;
	CGGammaValue greenMin;
	CGGammaValue greenMax;
	CGGammaValue greenGamma;
	CGGammaValue blueMin;
	CGGammaValue blueMax;
	CGGammaValue blueGamma;

	if(CGGetDisplayTransferByFormula(
		mDisplay,
		&redMin,
		&redMax,
		&redGamma,
		&greenMin,
		&greenMax,
		&greenGamma,
		&blueMin,
		&blueMax,
		&blueGamma) == noErr)
	{
		// So many choices...
		// Let's just return the green channel gamma for now.
		result = greenGamma;
	}

	return result;
}

U32 LLWindowMacOSX::getFSAASamples()
{
	return mFSAASamples;
}

void LLWindowMacOSX::setFSAASamples(const U32 samples)
{
	mFSAASamples = samples;
	mForceRebuild = TRUE;
}

BOOL LLWindowMacOSX::restoreGamma()
{
	CGDisplayRestoreColorSyncSettings();
	return true;
}

BOOL LLWindowMacOSX::setGamma(const F32 gamma)
{
	CGGammaValue redMin;
	CGGammaValue redMax;
	CGGammaValue redGamma;
	CGGammaValue greenMin;
	CGGammaValue greenMax;
	CGGammaValue greenGamma;
	CGGammaValue blueMin;
	CGGammaValue blueMax;
	CGGammaValue blueGamma;

	// MBW -- XXX -- Should we allow this in windowed mode?

	if(CGGetDisplayTransferByFormula(
		mDisplay,
		&redMin,
		&redMax,
		&redGamma,
		&greenMin,
		&greenMax,
		&greenGamma,
		&blueMin,
		&blueMax,
		&blueGamma) != noErr)
	{
		return false;
	}

	if(CGSetDisplayTransferByFormula(
		mDisplay,
		redMin,
		redMax,
		gamma,
		greenMin,
		greenMax,
		gamma,
		blueMin,
		blueMax,
		gamma) != noErr)
	{
		return false;
	}


	return true;
}

BOOL LLWindowMacOSX::isCursorHidden()
{
	return mCursorHidden;
}



// Constrains the mouse to the window.
void LLWindowMacOSX::setMouseClipping( BOOL b )
{
	// Just stash the requested state.  We'll simulate this when the cursor is hidden by decoupling.
	mIsMouseClipping = b;

	if(b)
	{
		//		llinfos << "setMouseClipping(TRUE)" << llendl;
	}
	else
	{
		//		llinfos << "setMouseClipping(FALSE)" << llendl;
	}

	adjustCursorDecouple();
}

BOOL LLWindowMacOSX::setCursorPosition(const LLCoordWindow position)
{
    S32 oldIdx = mCurRCIdx;
    LLCoordWindow pos2 = position;
    if (mHMDMode)
    {
        if (mCurRCIdx == 1)
        {
            mCurRCIdx = 0;
        }
        if (mHMDScale[0] != 0.0f)
        {
            pos2.mX = (S32)((F32)pos2.mX * mHMDScale[0]);
        }
        if (mHMDScale[1] != 0.0f)
        {
            pos2.mY = (S32)((F32)pos2.mY * mHMDScale[1]);
        }
    }
    LLCoordScreen screen_pos;
    BOOL result = convertCoords(pos2, &screen_pos);
    mCurRCIdx = oldIdx;
	if (!result)
	{
		return FALSE;
	}
    result = FALSE;

	CGPoint newPosition;

	//	llinfos << "setCursorPosition(" << screen_pos.mX << ", " << screen_pos.mY << ")" << llendl;

	newPosition.x = screen_pos.mX;
	newPosition.y = screen_pos.mY;

	CGSetLocalEventsSuppressionInterval(0.0);
	if(CGWarpMouseCursorPosition(newPosition) == noErr)
	{
		result = TRUE;
	}

	// Under certain circumstances, this will trigger us to decouple the cursor.
	adjustCursorDecouple(true);

	// trigger mouse move callback
	LLCoordGL gl_pos;
	convertCoords(position, &gl_pos);
	mCallbacks->handleMouseMove(this, gl_pos, (MASK)0);

	return result;
}

BOOL LLWindowMacOSX::getCursorPosition(LLCoordWindow *position)
{
	if (mWindow[0] == NULL)
    {
		return FALSE;
    }
	
	float cursor_point[2];
	getCursorPos(mWindow[0], cursor_point);
    
	if(mCursorDecoupled)
	{
		//		CGMouseDelta x, y;

		// If the cursor's decoupled, we need to read the latest movement delta as well.
		//		CGGetLastMouseDelta( &x, &y );
		//		cursor_point.h += x;
		//		cursor_point.v += y;

		// CGGetLastMouseDelta may behave strangely when the cursor's first captured.
		// Stash in the event handler instead.
		cursor_point[0] += mCursorLastEventDeltaX;
		cursor_point[1] += mCursorLastEventDeltaY;
	}

    if (mHMDMode)
    {
        keepMouseWithinBounds(cursor_point, 0, mHMDSize[0], mHMDSize[1]);
    }

	position->mX = cursor_point[0];
	position->mY = cursor_point[1];

	return TRUE;
}

void LLWindowMacOSX::keepMouseWithinBounds(float* cp, S32 winIdx, S32 w, S32 h)
{
    F32 actualBounds[2] = { (F32)w, (F32)h };
    if (mHMDMode)
    {
        for (S32 i = 0; i < 2; ++i)
        {
            if (mHMDScale[i] != 0.0f)
            {
                actualBounds[i] *= mHMDScale[i];
            }
        }
    }
    BOOL outOfBounds = cp[0] < 0.0f || cp[0] > actualBounds[0] || cp[1] < 0.0f || cp[1] > actualBounds[1];
    if (outOfBounds)
    {
        float scrPt[2];
        for (S32 i = 0; i < 2; ++i)
        {
            cp[i] = scrPt[i] = llmax(0.0f, llmin(actualBounds[i], cp[i]));
        }
        convertWindowToScreen(mWindow[winIdx], scrPt);
        CGPoint newPosition;
        newPosition.x = scrPt[0];
        newPosition.y = scrPt[1];
        CGSetLocalEventsSuppressionInterval(0.0);
        CGWarpMouseCursorPosition(newPosition);
    }
    if (mHMDMode)
    {
        // adjust position for HMD scaling
        for (S32 i = 0; i < 2; ++i)
        {
            if (mHMDScale[i] != 0.0f)
            {
                cp[i] /= mHMDScale[i];
            }
        }
    }
}

void LLWindowMacOSX::adjustCursorDecouple(bool warpingMouse)
{
	if(mIsMouseClipping && mCursorHidden)
	{
		if(warpingMouse)
		{
			// The cursor should be decoupled.  Make sure it is.
			if(!mCursorDecoupled)
			{
				//			llinfos << "adjustCursorDecouple: decoupling cursor" << llendl;
				CGAssociateMouseAndMouseCursorPosition(false);
				mCursorDecoupled = true;
				mCursorIgnoreNextDelta = TRUE;
			}
		}
	}
	else
	{
		// The cursor should not be decoupled.  Make sure it isn't.
		if(mCursorDecoupled)
		{
			//			llinfos << "adjustCursorDecouple: recoupling cursor" << llendl;
			CGAssociateMouseAndMouseCursorPosition(true);
			mCursorDecoupled = false;
		}
	}
}

F32 LLWindowMacOSX::getNativeAspectRatio()
{
	if (mFullscreen)
	{
		return (F32)mFullscreenWidth / (F32)mFullscreenHeight;
	}
	else
	{
		// The constructor for this class grabs the aspect ratio of the monitor before doing any resolution
		// switching, and stashes it in mOriginalAspectRatio.  Here, we just return it.

		if (mOverrideAspectRatio > 0.f)
		{
			return mOverrideAspectRatio;
		}

		return mOriginalAspectRatio;
	}
}

F32 LLWindowMacOSX::getPixelAspectRatio()
{
	//OS X always enforces a 1:1 pixel aspect ratio, regardless of video mode
	return 1.f;
}

//static SInt32 oldWindowLevel;

// MBW -- XXX -- There's got to be a better way than this.  Find it, please...

// Since we're no longer supporting the "typical" fullscreen mode with CGL or NSOpenGL anymore, these are unnecessary. -Geenz
void LLWindowMacOSX::beforeDialog()
{
}

void LLWindowMacOSX::afterDialog()
{
}


void LLWindowMacOSX::flashIcon(F32 seconds)
{
	// For consistency with OS X conventions, the number of seconds given is ignored and
	// left up to the OS (which will actually bounce it for one second).
	requestUserAttention();
}

BOOL LLWindowMacOSX::isClipboardTextAvailable()
{
	return pasteBoardAvailable();
}

BOOL LLWindowMacOSX::pasteTextFromClipboard(LLWString &dst)
{	
	llutf16string str(copyFromPBoard());
	dst = utf16str_to_wstring(str);
	if (dst != L"")
	{
		return true;
	} else {
		return false;
	}
}

BOOL LLWindowMacOSX::copyTextToClipboard(const LLWString &s)
{
	BOOL result = false;
	llutf16string utf16str = wstring_to_utf16str(s);
	
	result = copyToPBoard(utf16str.data(), utf16str.length());

	return result;
}


// protected
BOOL LLWindowMacOSX::resetDisplayResolution()
{
	// This is only called from elsewhere in this class, and it's not used by the Mac implementation.
	return true;
}


LLWindow::LLWindowResolution* LLWindowMacOSX::getSupportedResolutions(S32 &num_resolutions)
{
	if (!mSupportedResolutions)
	{
		CFArrayRef modes = CGDisplayAvailableModes(mDisplay);

		if(modes != NULL)
		{
			CFIndex index, cnt;

			mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
			mNumSupportedResolutions = 0;

			//  Examine each mode
			cnt = CFArrayGetCount( modes );

			for ( index = 0; (index < cnt) && (mNumSupportedResolutions < MAX_NUM_RESOLUTIONS); index++ )
			{
				//  Pull the mode dictionary out of the CFArray
				CFDictionaryRef mode = (CFDictionaryRef)CFArrayGetValueAtIndex( modes, index );
				long width = getDictLong(mode, kCGDisplayWidth);
				long height = getDictLong(mode, kCGDisplayHeight);
				long bits = getDictLong(mode, kCGDisplayBitsPerPixel);

				if(bits == BITS_PER_PIXEL && width >= 800 && height >= 600)
				{
					BOOL resolution_exists = FALSE;
					for(S32 i = 0; i < mNumSupportedResolutions; i++)
					{
						if (mSupportedResolutions[i].mWidth == width &&
							mSupportedResolutions[i].mHeight == height)
						{
							resolution_exists = TRUE;
						}
					}
					if (!resolution_exists)
					{
						mSupportedResolutions[mNumSupportedResolutions].mWidth = width;
						mSupportedResolutions[mNumSupportedResolutions].mHeight = height;
						mNumSupportedResolutions++;
					}
				}
			}
		}
	}

	num_resolutions = mNumSupportedResolutions;
	return mSupportedResolutions;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordGL from, LLCoordWindow *to)
{
    if (to)
    {
	    to->mX = from.mX;
	    to->mY = from.mY;
        if (mHMDMode && mCurRCIdx == 0)
        {
            to->mY = llmin(to->mY, mHMDSize[1]);
        }
	    return TRUE;
    }
    return FALSE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
    if (to)
    {
	    to->mX = from.mX;
        to->mY = from.mY;
	    return TRUE;
    }
    return FALSE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordScreen from, LLCoordWindow* to)
{
	if (mWindow[mCurRCIdx] && to)
	{
		float mouse_point[2];

		mouse_point[0] = from.mX;
		mouse_point[1] = from.mY;
		
		convertScreenToWindow(mWindow[mCurRCIdx], mouse_point);

		to->mX = mouse_point[0];
		to->mY = mouse_point[1];

		return TRUE;
	}
	return FALSE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordScreen *to)
{
	if (mWindow[mCurRCIdx] && to)
	{
		float mouse_point[2];

		mouse_point[0] = from.mX;
		mouse_point[1] = from.mY;

		convertWindowToScreen(mWindow[mCurRCIdx], mouse_point);

		to->mX = mouse_point[0];
		to->mY = mouse_point[1];

		return TRUE;
	}
	return FALSE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordScreen from, LLCoordGL *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}

BOOL LLWindowMacOSX::convertCoords(LLCoordGL from, LLCoordScreen *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}


void LLWindowMacOSX::setupFailure(const std::string& text, const std::string& caption, U32 type)
{
	destroyContext();

	OSMessageBox(text, caption, type);
}

			// Note on event recording - QUIT is a known special case and we are choosing NOT to record it for the record and playback feature 
			// it is handled at a very low-level
const char* cursorIDToName(int id)
{
	switch (id)
	{
		case UI_CURSOR_ARROW:							return "UI_CURSOR_ARROW";
		case UI_CURSOR_WAIT:							return "UI_CURSOR_WAIT";
		case UI_CURSOR_HAND:							return "UI_CURSOR_HAND";
		case UI_CURSOR_IBEAM:							return "UI_CURSOR_IBEAM";
		case UI_CURSOR_CROSS:							return "UI_CURSOR_CROSS";
		case UI_CURSOR_SIZENWSE:						return "UI_CURSOR_SIZENWSE";
		case UI_CURSOR_SIZENESW:						return "UI_CURSOR_SIZENESW";
		case UI_CURSOR_SIZEWE:							return "UI_CURSOR_SIZEWE";
		case UI_CURSOR_SIZENS:							return "UI_CURSOR_SIZENS";
		case UI_CURSOR_NO:								return "UI_CURSOR_NO";
		case UI_CURSOR_WORKING:							return "UI_CURSOR_WORKING";
		case UI_CURSOR_TOOLGRAB:						return "UI_CURSOR_TOOLGRAB";
		case UI_CURSOR_TOOLLAND:						return "UI_CURSOR_TOOLLAND";
		case UI_CURSOR_TOOLFOCUS:						return "UI_CURSOR_TOOLFOCUS";
		case UI_CURSOR_TOOLCREATE:						return "UI_CURSOR_TOOLCREATE";
		case UI_CURSOR_ARROWDRAG:						return "UI_CURSOR_ARROWDRAG";
		case UI_CURSOR_ARROWCOPY:						return "UI_CURSOR_ARROWCOPY";
		case UI_CURSOR_ARROWDRAGMULTI:					return "UI_CURSOR_ARROWDRAGMULTI";
		case UI_CURSOR_ARROWCOPYMULTI:					return "UI_CURSOR_ARROWCOPYMULTI";
		case UI_CURSOR_NOLOCKED:						return "UI_CURSOR_NOLOCKED";
		case UI_CURSOR_ARROWLOCKED:						return "UI_CURSOR_ARROWLOCKED";
		case UI_CURSOR_GRABLOCKED:						return "UI_CURSOR_GRABLOCKED";
		case UI_CURSOR_TOOLTRANSLATE:					return "UI_CURSOR_TOOLTRANSLATE";
		case UI_CURSOR_TOOLROTATE:						return "UI_CURSOR_TOOLROTATE";
		case UI_CURSOR_TOOLSCALE:						return "UI_CURSOR_TOOLSCALE";
		case UI_CURSOR_TOOLCAMERA:						return "UI_CURSOR_TOOLCAMERA";
		case UI_CURSOR_TOOLPAN:							return "UI_CURSOR_TOOLPAN";
		case UI_CURSOR_TOOLZOOMIN:						return "UI_CURSOR_TOOLZOOMIN";
		case UI_CURSOR_TOOLPICKOBJECT3:					return "UI_CURSOR_TOOLPICKOBJECT3";
		case UI_CURSOR_TOOLPLAY:						return "UI_CURSOR_TOOLPLAY";
		case UI_CURSOR_TOOLPAUSE:						return "UI_CURSOR_TOOLPAUSE";
		case UI_CURSOR_TOOLMEDIAOPEN:					return "UI_CURSOR_TOOLMEDIAOPEN";
		case UI_CURSOR_PIPETTE:							return "UI_CURSOR_PIPETTE";
		case UI_CURSOR_TOOLSIT:							return "UI_CURSOR_TOOLSIT";
		case UI_CURSOR_TOOLBUY:							return "UI_CURSOR_TOOLBUY";
		case UI_CURSOR_TOOLOPEN:						return "UI_CURSOR_TOOLOPEN";
		case UI_CURSOR_TOOLPATHFINDING:					return "UI_CURSOR_PATHFINDING";
		case UI_CURSOR_TOOLPATHFINDING_PATH_START:		return "UI_CURSOR_PATHFINDING_START";
		case UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD:	return "UI_CURSOR_PATHFINDING_START_ADD";
		case UI_CURSOR_TOOLPATHFINDING_PATH_END:		return "UI_CURSOR_PATHFINDING_END";
		case UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD:	return "UI_CURSOR_PATHFINDING_END_ADD";
		case UI_CURSOR_TOOLNO:							return "UI_CURSOR_NO";
	}

	llerrs << "cursorIDToName: unknown cursor id" << id << llendl;

	return "UI_CURSOR_ARROW";
}

static CursorRef gCursors[UI_CURSOR_COUNT];


static void initPixmapCursor(int cursorid, int hotspotX, int hotspotY)
{
	// cursors are in <Application Bundle>/Contents/Resources/cursors_mac/UI_CURSOR_FOO.tif
	std::string fullpath = gDirUtilp->getAppRODataDir();
	fullpath += gDirUtilp->getDirDelimiter();
	fullpath += "cursors_mac";
	fullpath += gDirUtilp->getDirDelimiter();
	fullpath += cursorIDToName(cursorid);
	fullpath += ".tif";

	gCursors[cursorid] = createImageCursor(fullpath.c_str(), hotspotX, hotspotY);
}

void LLWindowMacOSX::updateCursor()
{
	S32 result = 0;

	if (mDragOverrideCursor != -1)
	{
		// A drag is in progress...remember the requested cursor and we'll
		// restore it when it is done
		mCurrentCursor = mNextCursor;
		return;
	}
		
	if (mNextCursor == UI_CURSOR_ARROW
		&& mBusyCount > 0)
	{
		mNextCursor = UI_CURSOR_WORKING;
	}
	
	if(mCurrentCursor == mNextCursor)
		return;

	// RN: replace multi-drag cursors with single versions
	if (mNextCursor == UI_CURSOR_ARROWDRAGMULTI)
	{
		mNextCursor = UI_CURSOR_ARROWDRAG;
	}
	else if (mNextCursor == UI_CURSOR_ARROWCOPYMULTI)
	{
		mNextCursor = UI_CURSOR_ARROWCOPY;
	}

	switch(mNextCursor)
	{
	default:
	case UI_CURSOR_ARROW:
		setArrowCursor();
		if(mCursorHidden || mHMDMode)
		{
			// Since InitCursor resets the hide level, correct for it here.
			hideNSCursor();
		}
		break;

		// MBW -- XXX -- Some of the standard Windows cursors have no standard Mac equivalents.
		//    Find out what they look like and replicate them.

		// These are essentially correct
	case UI_CURSOR_WAIT:		/* Apple purposely doesn't allow us to set the beachball cursor manually.  Let NSApp figure out when to do this. */	break;
	case UI_CURSOR_IBEAM:		setIBeamCursor();	break;
	case UI_CURSOR_CROSS:		setCrossCursor();	break;
	case UI_CURSOR_HAND:		setPointingHandCursor();	break;
		//		case UI_CURSOR_NO:			SetThemeCursor(kThemeNotAllowedCursor);	break;
	case UI_CURSOR_ARROWCOPY:   setCopyCursor();	break;

		// Double-check these
	case UI_CURSOR_NO:
	case UI_CURSOR_SIZEWE:
	case UI_CURSOR_SIZENS:
	case UI_CURSOR_SIZENWSE:
	case UI_CURSOR_SIZENESW:
	case UI_CURSOR_WORKING:
	case UI_CURSOR_TOOLGRAB:
	case UI_CURSOR_TOOLLAND:
	case UI_CURSOR_TOOLFOCUS:
	case UI_CURSOR_TOOLCREATE:
	case UI_CURSOR_ARROWDRAG:
	case UI_CURSOR_NOLOCKED:
	case UI_CURSOR_ARROWLOCKED:
	case UI_CURSOR_GRABLOCKED:
	case UI_CURSOR_TOOLTRANSLATE:
	case UI_CURSOR_TOOLROTATE:
	case UI_CURSOR_TOOLSCALE:
	case UI_CURSOR_TOOLCAMERA:
	case UI_CURSOR_TOOLPAN:
	case UI_CURSOR_TOOLZOOMIN:
	case UI_CURSOR_TOOLPICKOBJECT3:
	case UI_CURSOR_TOOLPLAY:
	case UI_CURSOR_TOOLPAUSE:
	case UI_CURSOR_TOOLMEDIAOPEN:
	case UI_CURSOR_TOOLSIT:
	case UI_CURSOR_TOOLBUY:
	case UI_CURSOR_TOOLOPEN:
	case UI_CURSOR_TOOLPATHFINDING:
	case UI_CURSOR_TOOLPATHFINDING_PATH_START:
	case UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD:
	case UI_CURSOR_TOOLPATHFINDING_PATH_END:
	case UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD:
	case UI_CURSOR_TOOLNO:
		result = setImageCursor(gCursors[mNextCursor]);
		break;

	}

	if(result != noErr)
	{
		setArrowCursor();
		if(mCursorHidden || mHMDMode)
		{
			hideNSCursor();
		}
	}

	mCurrentCursor = mNextCursor;
}

ECursorType LLWindowMacOSX::getCursor() const
{
    if (mDragOverrideCursor >= 0)
    {
        return (ECursorType)mDragOverrideCursor;
    }
    else
    {
	    return mCurrentCursor;
    }
}

void LLWindowMacOSX::initCursors()
{
	initPixmapCursor(UI_CURSOR_NO, 8, 8);
	initPixmapCursor(UI_CURSOR_WORKING, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLGRAB, 2, 14);
	initPixmapCursor(UI_CURSOR_TOOLLAND, 13, 8);
	initPixmapCursor(UI_CURSOR_TOOLFOCUS, 7, 6);
	initPixmapCursor(UI_CURSOR_TOOLCREATE, 7, 7);
	initPixmapCursor(UI_CURSOR_ARROWDRAG, 1, 1);
	initPixmapCursor(UI_CURSOR_ARROWCOPY, 1, 1);
	initPixmapCursor(UI_CURSOR_NOLOCKED, 8, 8);
	initPixmapCursor(UI_CURSOR_ARROWLOCKED, 1, 1);
	initPixmapCursor(UI_CURSOR_GRABLOCKED, 2, 14);
	initPixmapCursor(UI_CURSOR_TOOLTRANSLATE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLROTATE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLSCALE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLCAMERA, 7, 6);
	initPixmapCursor(UI_CURSOR_TOOLPAN, 7, 6);
	initPixmapCursor(UI_CURSOR_TOOLZOOMIN, 7, 6);
	initPixmapCursor(UI_CURSOR_TOOLPICKOBJECT3, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLPLAY, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLPAUSE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLMEDIAOPEN, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLSIT, 20, 15);
	initPixmapCursor(UI_CURSOR_TOOLBUY, 20, 15);
	initPixmapCursor(UI_CURSOR_TOOLOPEN, 20, 15);
	initPixmapCursor(UI_CURSOR_TOOLPATHFINDING, 16, 16);
	initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_START, 16, 16);
	initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD, 16, 16);
	initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_END, 16, 16);
	initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD, 16, 16);
	initPixmapCursor(UI_CURSOR_TOOLNO, 8, 8);

	initPixmapCursor(UI_CURSOR_SIZENWSE, 10, 10);
	initPixmapCursor(UI_CURSOR_SIZENESW, 10, 10);
	initPixmapCursor(UI_CURSOR_SIZEWE, 10, 10);
	initPixmapCursor(UI_CURSOR_SIZENS, 10, 10);

}

void LLWindowMacOSX::captureMouse()
{
	// By registering a global CarbonEvent handler for mouse move events, we ensure that
	// mouse events are always processed.  Thus, capture and release are unnecessary.
}

void LLWindowMacOSX::releaseMouse()
{
	// By registering a global CarbonEvent handler for mouse move events, we ensure that
	// mouse events are always processed.  Thus, capture and release are unnecessary.
}

void LLWindowMacOSX::hideCursor()
{
	if(!mCursorHidden)
	{
		//		llinfos << "hideCursor: hiding" << llendl;
		mCursorHidden = TRUE;
		mHideCursorPermanent = TRUE;
        if (!mHMDMode)
        {
		    hideNSCursor();
        }
	}
	else
	{
		//		llinfos << "hideCursor: already hidden" << llendl;
	}

	adjustCursorDecouple();
}

void LLWindowMacOSX::showCursor()
{
	if(mCursorHidden)
	{
		//		llinfos << "showCursor: showing" << llendl;
		mCursorHidden = FALSE;
		mHideCursorPermanent = FALSE;
        if (!mHMDMode)
        {
		    showNSCursor();
        }
	}
	else
	{
		//		llinfos << "showCursor: already visible" << llendl;
	}

	adjustCursorDecouple();
}

void LLWindowMacOSX::showCursorFromMouseMove()
{
	if (!mHideCursorPermanent)
	{
		showCursor();
	}
}

void LLWindowMacOSX::hideCursorUntilMouseMove()
{
	if (!mHideCursorPermanent)
	{
		hideCursor();
		mHideCursorPermanent = FALSE;
	}
}



//
// LLSplashScreenMacOSX
//
LLSplashScreenMacOSX::LLSplashScreenMacOSX()
{
	mWindow = NULL;
}

LLSplashScreenMacOSX::~LLSplashScreenMacOSX()
{
}

void LLSplashScreenMacOSX::showImpl()
{
	// This code _could_ be used to display a spash screen...
}

void LLSplashScreenMacOSX::updateImpl(const std::string& mesg)
{
	if(mWindow != NULL)
	{
		CFStringRef string = NULL;

		string = CFStringCreateWithCString(NULL, mesg.c_str(), kCFStringEncodingUTF8);
	}
}


void LLSplashScreenMacOSX::hideImpl()
{
	if(mWindow != NULL)
	{
		mWindow = NULL;
	}
}

S32 OSMessageBoxMacOSX(const std::string& text, const std::string& caption, U32 type)
{
	return showAlert(text, caption, type);
}

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void LLWindowMacOSX::spawnWebBrowser(const std::string& escaped_url, bool async)
{
	// I'm fairly certain that this is all legitimate under Apple's currently supported APIs.
	
	bool found = false;
	S32 i;
	for (i = 0; i < gURLProtocolWhitelistCount; i++)
	{
		if (escaped_url.find(gURLProtocolWhitelist[i]) != std::string::npos)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		llwarns << "spawn_web_browser called for url with protocol not on whitelist: " << escaped_url << llendl;
		return;
	}

	S32 result = 0;
	CFURLRef urlRef = NULL;

	llinfos << "Opening URL " << escaped_url << llendl;

	CFStringRef	stringRef = CFStringCreateWithCString(NULL, escaped_url.c_str(), kCFStringEncodingUTF8);
	if (stringRef)
	{
		// This will succeed if the string is a full URL, including the http://
		// Note that URLs specified this way need to be properly percent-escaped.
		urlRef = CFURLCreateWithString(NULL, stringRef, NULL);

		// Don't use CRURLCreateWithFileSystemPath -- only want valid URLs

		CFRelease(stringRef);
	}

	if (urlRef)
	{
		result = LSOpenCFURLRef(urlRef, NULL);

		if (result != noErr)
		{
			llinfos << "Error " << result << " on open." << llendl;
		}

		CFRelease(urlRef);
	}
	else
	{
		llinfos << "Error: couldn't create URL." << llendl;
	}
}

LLSD LLWindowMacOSX::getNativeKeyData()
{
	LLSD result = LLSD::emptyMap();
#if 0
	if(mRawKeyEvent)
	{
		char char_code = 0;
		UInt32 key_code = 0;
		UInt32 modifiers = 0;
		UInt32 keyboard_type = 0;

		GetEventParameter (mRawKeyEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &char_code);
		GetEventParameter (mRawKeyEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &key_code);
		GetEventParameter (mRawKeyEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
		GetEventParameter (mRawKeyEvent, kEventParamKeyboardType, typeUInt32, NULL, sizeof(UInt32), NULL, &keyboard_type);

		result["char_code"] = (S32)char_code;
		result["key_code"] = (S32)key_code;
		result["modifiers"] = (S32)modifiers;
		result["keyboard_type"] = (S32)keyboard_type;

#if 0
		// This causes trouble for control characters -- apparently character codes less than 32 (escape, control-A, etc)
		// cause llsd serialization to create XML that the llsd deserializer won't parse!
		std::string unicode;
		S32 err = noErr;
		EventParamType actualType = typeUTF8Text;
		UInt32 actualSize = 0;
		char *buffer = NULL;

		err = GetEventParameter (mRawKeyEvent, kEventParamKeyUnicodes, typeUTF8Text, &actualType, 0, &actualSize, NULL);
		if(err == noErr)
		{
			// allocate a buffer and get the actual data.
			buffer = new char[actualSize];
			err = GetEventParameter (mRawKeyEvent, kEventParamKeyUnicodes, typeUTF8Text, &actualType, actualSize, &actualSize, buffer);
			if(err == noErr)
			{
				unicode.assign(buffer, actualSize);
			}
			delete[] buffer;
		}

		result["unicode"] = unicode;
#endif

	}
#endif

	lldebugs << "native key data is: " << result << llendl;

	return result;
}


BOOL LLWindowMacOSX::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	// Is this even used anywhere?  Do we really need an OS color picker?
	BOOL	retval = FALSE;
	//S32		error = 0;
	return (retval);
}


void *LLWindowMacOSX::getPlatformWindow()
{
	// NOTE: this will be NULL in fullscreen mode.  Plan accordingly.
	return (void*)(mWindow[mCurRCIdx]);
}

// get a double value from a dictionary
/*
static double getDictDouble (CFDictionaryRef refDict, CFStringRef key)
{
	double double_value;
	CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
	if (!number_value) // if can't get a number for the dictionary
		return -1;  // fail
	if (!CFNumberGetValue(number_value, kCFNumberDoubleType, &double_value)) // or if cant convert it
		return -1; // fail
	return double_value; // otherwise return the long value
}*/

// get a long value from a dictionary
static long getDictLong (CFDictionaryRef refDict, CFStringRef key)
{
	long int_value;
	CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
	if (!number_value) // if can't get a number for the dictionary
		return -1;  // fail
	if (!CFNumberGetValue(number_value, kCFNumberLongType, &int_value)) // or if cant convert it
		return -1; // fail
	return int_value; // otherwise return the long value
}

void LLWindowMacOSX::allowLanguageTextInput(LLPreeditor *preeditor, BOOL b)
{
    allowDirectMarkedTextInput(b, mGLView[mCurRCIdx]);
	
	if (preeditor != mPreeditor && !b)
	{
		// This condition may occur by a call to
		// setEnabled(BOOL) against LLTextEditor or LLLineEditor
		// when the control is not focused.
		// We need to silently ignore the case so that
		// the language input status of the focused control
		// is not disturbed.
		return;
	}
    
	// Take care of old and new preeditors.
	if (preeditor != mPreeditor || !b)
	{
		// We need to interrupt before updating mPreeditor,
		// so that the fix string from input method goes to
		// the old preeditor.
		if (mLanguageTextInputAllowed)
		{
			interruptLanguageTextInput();
		}
		mPreeditor = (b ? preeditor : NULL);
	}
	
	if (b == mLanguageTextInputAllowed)
	{
		return;
	}
	mLanguageTextInputAllowed = b;
}

void LLWindowMacOSX::interruptLanguageTextInput()
{
	commitCurrentPreedit(mGLView[mCurRCIdx]);
}

//static
std::vector<std::string> LLWindowMacOSX::getDynamicFallbackFontList()
{
	// Fonts previously in getFontListSans() have moved to fonts.xml.
	return std::vector<std::string>();
}

// static
MASK LLWindowMacOSX::modifiersToMask(S16 modifiers)
{
	MASK mask = 0;
	if(modifiers & MAC_SHIFT_KEY) { mask |= MASK_SHIFT; }
	if(modifiers & (MAC_CMD_KEY | MAC_CTRL_KEY)) { mask |= MASK_CONTROL; }
	if(modifiers & MAC_ALT_KEY) { mask |= MASK_ALT; }
	return mask;
}

// HMD Support
/*virtual*/
BOOL LLWindowMacOSX::initHMDWindow(S32 left, S32 top, S32 width, S32 height, BOOL& isMirror)
{
    LL_INFOS("Window") << "initHMDWindow" << LL_ENDL;
    destroyHMDWindow();

    mHMDSize[0] = width;
    mHMDSize[1] = height;

    S32 screen_count = getDisplayCount();
    for (S32 screen_id = 0; screen_id < screen_count; screen_id++)
    {
        if (getDisplayId(screen_id) == (long)left)
        {
            mHMDScreenId = screen_id;
            break;
        }
    }
    if (mHMDScreenId < 0)
    {
        // Not found -> exit with error
        LL_INFOS("Window") << "Failed to create HMD window - could not find display id " << left << LL_ENDL;
        return FALSE;
    }

    isMirror = CGDisplayIsInMirrorSet((CGDirectDisplayID)left);
    if (isMirror)
    {
        // don't create a window in this case since we just want to use the "advanced" HMD mode in this case
        return TRUE;
    }

    LL_INFOS("Window") << "Creating the HMD window on screen " << mHMDScreenId << LL_ENDL;
    mWindow[1] = createFullScreenWindow(mHMDScreenId, FALSE);
    if (mWindow[1] != NULL)
    {
        LL_INFOS("Window") << "Creating the HMD GL view" << LL_ENDL;
        mGLView[1] = createFullScreenView(mWindow[1]);
    }
    if (mWindow[1] == NULL || mGLView[1] == NULL)
    {
        LL_INFOS("Window") << "Error creating HMD window" << LL_ENDL;
        destroyHMDWindow();
        return FALSE;
    }

    if (mWindow[0] && mGLView[0])
    {
        // the above just stole the focus from the main window, so unless we want the initial 
        // focus status to be screwed up, we need to set it back to the main window here.
        makeFirstResponder(mWindow[0], mGLView[0]);
        makeWindowOrderFront(mWindow[0]);
    }

    return TRUE;
}

/*virtual*/
BOOL LLWindowMacOSX::destroyHMDWindow()
{
    LL_INFOS("Window") << "destroyHMDWindow" << LL_ENDL;

    // Destroy the LLOpenGLView
    if (mGLView[1] != NULL)
    {
        removeGLView(mGLView[1]);
        mGLView[1] = NULL;
    }
	
    // Close the window
    if(mWindow[1] != NULL)
    {
        NSWindowRef dead_window = mWindow[1];
        mWindow[1] = NULL;
        closeWindow(dead_window);
    }

    mHMDScreenId = -1;

    return TRUE;
}

/*virtual*/
BOOL LLWindowMacOSX::setRenderWindow(S32 idx, BOOL fullscreen)
{
    if (idx < 0 || idx > 1 || !mGLView[idx])
    {
        // Incorrect parameter or no view -> error
        return FALSE;
    }
    //LL_DEBUGS("Window") << "setRenderWindow : start" << LL_ENDL;

    if (mCurRCIdx == idx && fullscreen == mFullscreen)
    {
        // Already set to the correct window, nothing to do
        return TRUE;
    }

    mCurRCIdx = idx;
    mFullscreen = fullscreen;

    // Set the view on the current context
    setCGLCurrentContext(mGLView[mCurRCIdx]);
    makeFirstResponder(mWindow[mCurRCIdx], mGLView[mCurRCIdx]);
    makeWindowOrderFront(mWindow[mCurRCIdx]);
    
    //LL_DEBUGS("Window") << "setRenderWindow : successful" << LL_ENDL;
    return TRUE;
}

/*virtual*/
BOOL LLWindowMacOSX::setFocusWindow(S32 idx)
{
    if (idx < 0 || idx > 1 || !mWindow[idx] || !mGLView[idx])
    {
        // Incorrect parameter or no view -> error
        return FALSE;
    }
    adjustHMDScale();
    return TRUE;
}

void LLWindowMacOSX::setHMDMode(BOOL mode, U32 min_width, U32 min_height)
{
    BOOL oldHMDMode = mHMDMode;
    mHMDMode = mode;
    if (mHMDMode && !oldHMDMode && !mCursorHidden)
    {
        hideNSCursor();
    }
    else if (!mCursorHidden && oldHMDMode && !mHMDMode)
    {
        showNSCursor();
    }
    setMinSize(min_width, min_height, false);
}

void LLWindowMacOSX::adjustHMDScale()
{
    mHMDScale[0] = mHMDScale[1] = 1.0f;
    if (mHMDMode && mWindow[0])
    {
        float client[4];
        float hmd[2] = { (float)mHMDSize[0], (float)mHMDSize[1] };
        getContentViewBounds(mWindow[0], client);
        if (hmd[0] > 0.0f && client[2] > 0.0f && client[2] < hmd[0])
        {
            mHMDScale[0] = client[2] / hmd[0];
        }
        if (hmd[1] > 0.0f && client[3] > 0.0f && client[3] < hmd[1])
        {
            mHMDScale[1] = client[3] / hmd[1];
        }
    }
}

void LLWindowMacOSX::enterFullScreen()
{
    if (mCurRCIdx < 0 || mCurRCIdx > 1 || !mWindow[mCurRCIdx])
    {
        return;
    }
    float winBounds[4];
    getWindowSize(mWindow[mCurRCIdx], winBounds);
    int screen_id = getScreenFromPoint(winBounds);
    ::enterFullScreen(screen_id, mWindow[mCurRCIdx]);
}

void LLWindowMacOSX::exitFullScreen(LLCoordScreen pos, LLCoordWindow size)
{
    if (mCurRCIdx < 0 || mCurRCIdx > 1 || !mWindow[mCurRCIdx])
    {
        return;
    }
    float winBounds[4];
    getWindowSize(mWindow[mCurRCIdx], winBounds);
    int screen_id = getScreenFromPoint(winBounds);
    leaveFullScreen(screen_id, mWindow[mCurRCIdx], pos.mX, pos.mY, size.mX, size.mY + 22);
}

/*virtual*/
S32 LLWindowMacOSX::getDisplayCount()
{
    return getDisplayCountObjC();
}

/*virtual*/
void LLWindowMacOSX::enableVSync(BOOL b)
{
	// Enable/Disable vertical sync for swap
    if (mContext)
    {
	    GLint frames_per_swap = (b ? 1 : 0);
	    CGLSetParameter(mContext, kCGLCPSwapInterval, &frames_per_swap);
    }
}

#if LL_OS_DRAGDROP_ENABLED
/*
S16 LLWindowMacOSX::dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow,
						  void * handlerRefCon, DragRef drag)
{
	S16 result = 0;
	LLWindowMacOSX *self = (LLWindowMacOSX*)handlerRefCon;

	lldebugs << "drag tracking handler, message = " << message << llendl;

	switch(message)
	{
		case kDragTrackingInWindow:
			result = self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_TRACK);
		break;

		case kDragTrackingEnterHandler:
			result = self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_START_TRACKING);
		break;

		case kDragTrackingLeaveHandler:
			result = self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_STOP_TRACKING);
		break;

		default:
		break;
	}

	return result;
}
OSErr LLWindowMacOSX::dragReceiveHandler(WindowRef theWindow, void * handlerRefCon,
										 DragRef drag)
{
	LLWindowMacOSX *self = (LLWindowMacOSX*)handlerRefCon;
	return self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_DROPPED);

}
*/
void LLWindowMacOSX::handleDragNDrop(std::string url, LLWindowCallbacks::DragNDropAction action)
{
	MASK mask = LLWindowMacOSX::modifiersToMask(getModifiers());

	float mouse_point[2];
	// This will return the mouse point in window coords
	getCursorPos(mWindow[mCurRCIdx], mouse_point);
	
    if (mHMDMode)
    {
        keepMouseWithinBounds(mouse_point, 0, mHMDSize[0], mHMDSize[1]);
    }

	LLCoordWindow window_coords(mouse_point[0], mouse_point[1]);
	LLCoordGL gl_pos;
	convertCoords(window_coords, &gl_pos);

	if(!url.empty())
	{
		LLWindowCallbacks::DragNDropResult res =
		mCallbacks->handleDragNDrop(this, gl_pos, mask, action, url);
		
		switch (res) {
			case LLWindowCallbacks::DND_NONE:		// No drop allowed
				if (action == LLWindowCallbacks::DNDA_TRACK)
				{
					mDragOverrideCursor = 0;
				}
				else {
					mDragOverrideCursor = -1;
				}
				break;
			case LLWindowCallbacks::DND_MOVE:		// Drop accepted would result in a "move" operation
				mDragOverrideCursor = UI_CURSOR_NO;
				break;
			case LLWindowCallbacks::DND_COPY:		// Drop accepted would result in a "copy" operation
				mDragOverrideCursor = UI_CURSOR_ARROWCOPY;
				break;
			default:
				mDragOverrideCursor = -1;
				break;
		}
		// This overrides the cursor being set by setCursor.
		// This is a bit of a hack workaround because lots of areas
		// within the viewer just blindly set the cursor.
		if (mDragOverrideCursor == -1)
		{
			// Restore the cursor
			ECursorType temp_cursor = mCurrentCursor;
			// get around the "setting the same cursor" code in setCursor()
			mCurrentCursor = UI_CURSOR_COUNT;
			setCursor(temp_cursor);
		}
		else {
			// Override the cursor
			switch (mDragOverrideCursor) {
				case 0:
					setArrowCursor();
                    if(mCursorHidden || mHMDMode)
                    {
                        hideNSCursor();
                    }
					break;
				case UI_CURSOR_NO:
					setNotAllowedCursor();
				case UI_CURSOR_ARROWCOPY:
					setCopyCursor();
				default:
					break;
			};
		}
	}
}

#endif // LL_OS_DRAGDROP_ENABLED
