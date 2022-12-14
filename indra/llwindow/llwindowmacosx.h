/** 
 * @file llwindowmacosx.h
 * @brief Mac implementation of LLWindow class
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

#ifndef LL_LLWINDOWMACOSX_H
#define LL_LLWINDOWMACOSX_H

#include "llwindow.h"
#include "llwindowcallbacks.h"
#include "llwindowmacosx-objc.h"

#include "lltimer.h"

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>

// AssertMacros.h does bad things.
#include "fix_macros.h"
#undef verify
#undef require


class LLWindowMacOSX : public LLWindow
{
public:
	/*virtual*/ void show();
	/*virtual*/ void hide();
	/*virtual*/ void close();
	/*virtual*/ BOOL getVisible();
	/*virtual*/ BOOL getMinimized();
	/*virtual*/ BOOL getMaximized();
	/*virtual*/ BOOL maximize();
	/*virtual*/ void minimize();
	/*virtual*/ void restore();
	/*virtual*/ BOOL getFullscreen();
	/*virtual*/ BOOL getPosition(LLCoordScreen *position);
	/*virtual*/ BOOL getSize(LLCoordScreen *size);
	/*virtual*/ BOOL getSize(LLCoordWindow *size);
	/*virtual*/ BOOL setPosition(LLCoordScreen position);
	/*virtual*/ BOOL setSizeImpl(LLCoordScreen size, BOOL adjustPosition);
	/*virtual*/ BOOL setSizeImpl(LLCoordWindow size, BOOL adjustPosition);
	/*virtual*/ BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp = NULL);
	/*virtual*/ BOOL setCursorPosition(LLCoordWindow position);
	/*virtual*/ BOOL getCursorPosition(LLCoordWindow *position);
	/*virtual*/ void showCursor();
	/*virtual*/ void hideCursor();
	/*virtual*/ void showCursorFromMouseMove();
	/*virtual*/ void hideCursorUntilMouseMove();
	/*virtual*/ BOOL isCursorHidden();
	/*virtual*/ void updateCursor();
	/*virtual*/ ECursorType getCursor() const;
	/*virtual*/ void captureMouse();
	/*virtual*/ void releaseMouse();
	/*virtual*/ void setMouseClipping( BOOL b );
	/*virtual*/ BOOL isClipboardTextAvailable();
	/*virtual*/ BOOL pasteTextFromClipboard(LLWString &dst);
	/*virtual*/ BOOL copyTextToClipboard(const LLWString & src);
	/*virtual*/ void flashIcon(F32 seconds);
	/*virtual*/ F32 getGamma();
	/*virtual*/ BOOL setGamma(const F32 gamma); // Set the gamma
	/*virtual*/ U32 getFSAASamples();
	/*virtual*/ void setFSAASamples(const U32 fsaa_samples);
	/*virtual*/ BOOL restoreGamma();			// Restore original gamma table (before updating gamma)
	/*virtual*/ ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void gatherInput();
	/*virtual*/ void delayInputProcessing() {};
	/*virtual*/ void swapBuffers();
	
	// handy coordinate space conversion routines
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to);
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to);
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordGL *to);
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordWindow *to);
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordGL *to);
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordScreen *to);

	/*virtual*/ LLWindowResolution* getSupportedResolutions(S32 &num_resolutions);
	/*virtual*/ F32	getNativeAspectRatio();
	/*virtual*/ F32 getPixelAspectRatio();
	/*virtual*/ void setNativeAspectRatio(F32 ratio) { mOverrideAspectRatio = ratio; }

	/*virtual*/ void beforeDialog();
	/*virtual*/ void afterDialog();

	/*virtual*/ BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b);

	/*virtual*/ void *getPlatformWindow(S32 idx = -1);
	/*virtual*/ void bringToFront() {};
	
	/*virtual*/ void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b);
	/*virtual*/ void interruptLanguageTextInput();
	/*virtual*/ void spawnWebBrowser(const std::string& escaped_url, bool async);

	static std::vector<std::string> getDynamicFallbackFontList();

	// Provide native key event data
	/*virtual*/ LLSD getNativeKeyData();
	
	void* getWindow() { return mWindow[mCurRCIdx]; }
	LLWindowCallbacks* getCallbacks() { return mCallbacks; }
	LLPreeditor* getPreeditor() { return mPreeditor; }
	
	void updateMouseDeltas(float* deltas);
	void getMouseDeltas(float* delta);
	
	void handleDragNDrop(std::string url, LLWindowCallbacks::DragNDropAction action);
    
    bool allowsLanguageInput() { return mLanguageTextInputAllowed; }

    // HMD support
    /*virtual*/ BOOL initHMDWindow(S32 left, S32 top, S32 width, S32 height, BOOL forceMirror, BOOL& isMirror);
    /*virtual*/ BOOL destroyHMDWindow();
    /*virtual*/ BOOL setRenderWindow(S32 idx, BOOL fullscreen);
    /*virtual*/ BOOL setFocusWindow(S32 idx);
    /*virtual*/ void setHMDMode(BOOL mode, U32 min_width = 0, U32 min_height = 0);
    /*virtual*/ S32 getDisplayCount();
    /*virtual*/ void enableVSync(BOOL b);
    
    // Mac Overrides to get values for these instead of what the getSize() and getPosition methods return.
    // adding to get real info from the Mac since this class is very inconsistent about returning 
    // whole window (i.e. "Frame") vs. view size and position.  The following get functions in this class return
    // data that is not usable in the corresponding set functions:
    //
    // getPosition(LLCoordScreen* position) - returns position of the upper-left corner of the VIEW, not the windowframe
    //
    // However, the following methods expect data from the Frame, not the view:
    // setPosition(LLCoordScreen position) - sets the position assuming you passed in the upper left of the FRAME
    //
    // Needless to say, this makes getting and setting the window position problematical on the Mac.
    // I would fix these, but I'm afraid of what else is now relying upon the current behavior.  *sigh*
    // Instead, I'll just add methods that get the "correct" data so that the setPosition method can be called
    // with the correct data on the Mac.
    
    // returns the upper-left screen coordinates for the window frame (including the title bar and any borders)
    /*virtual*/ BOOL getFramePos(LLCoordScreen* pos);

    void adjustPosForHMDScaling(LLCoordGL& pt);
    void enterFullScreen();
    void exitFullScreen(LLCoordScreen pos, LLCoordWindow size);
    void scaleBackSurface(BOOL scale);

protected:
	LLWindowMacOSX(LLWindowCallbacks* callbacks,
		const std::string& title, const std::string& name, int x, int y, int width, int height, U32 flags,
		BOOL fullscreen, BOOL clearBg, BOOL disable_vsync, BOOL use_gl,
		BOOL ignore_pixel_depth,
		U32 fsaa_samples);
	~LLWindowMacOSX();

	void	initCursors();
	BOOL	isValid();
	void	moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);


	// Changes display resolution. Returns true if successful
	BOOL	setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

	// Go back to last fullscreen display resolution.
	BOOL	setFullscreenResolution();

	// Restore the display resolution to its value before we ran the app.
	BOOL	resetDisplayResolution();

	BOOL	shouldPostQuit() { return mPostQuit; }
    
    //Satisfy MAINT-3135 and MAINT-3288 with a flag.
    /*virtual */ void setOldResize(bool oldresize) {setResizeMode(oldresize, mGLView); }

private:
    void restoreGLContext();

protected:
	//
	// Platform specific methods
	//

	// create or re-create the GL context/window.  Called from the constructor and switchContext().
	BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync);
	void destroyContext();
	void setupFailure(const std::string& text, const std::string& caption, U32 type);
    void keepMouseWithinBounds(float* cp, S32 winIdx, S32 w, S32 h);
	void adjustCursorDecouple(bool warpingMouse = false);
	static MASK modifiersToMask(S16 modifiers);

    void adjustWindowToFitScreen(LLCoordWindow& size);
    void adjustHMDScale();

#if LL_OS_DRAGDROP_ENABLED
	
	//static OSErr dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow, void * handlerRefCon, DragRef theDrag);
	//static OSErr dragReceiveHandler(WindowRef theWindow, void * handlerRefCon,	DragRef theDrag);
	
	
#endif // LL_OS_DRAGDROP_ENABLED
	
	//
	// Platform specific variables
	//
	
	// Use generic pointers here.  This lets us do some funky Obj-C interop using Obj-C objects without having to worry about any compilation problems that may arise.
	NSWindowRef			mWindow[2];
	GLViewRef			mGLView[2];
	CGLContextObj		mContext;
	CGLPixelFormatObj	mPixelFormat;
	CGDirectDisplayID	mDisplay;
	
	LLRect		mOldMouseClip;  // Screen rect to which the mouse cursor was globally constrained before we changed it in clipMouse()
	std::string mWindowTitle;
	double		mOriginalAspectRatio;
	BOOL		mSimulatedRightClick;
	U32			mLastModifiers;
	BOOL		mHandsOffEvents;	// When true, temporarially disable CarbonEvent processing.
	// Used to allow event processing when putting up dialogs in fullscreen mode.
	BOOL		mCursorDecoupled;
	S32			mCursorLastEventDeltaX;
	S32			mCursorLastEventDeltaY;
	BOOL		mCursorIgnoreNextDelta;
	BOOL		mNeedsResize;		// Constructor figured out the window is too big, it needs a resize.
	LLCoordScreen   mNeedsResizeSize;
	F32			mOverrideAspectRatio;
	BOOL		mMaximized;
	BOOL		mMinimized;
	U32			mFSAASamples;
	BOOL		mForceRebuild;
    S32	        mDragOverrideCursor;
    BOOL        mHMDMode;
    S32         mHMDSize[2];
    F32         mHMDScale[2];

	// Input method management through Text Service Manager.
	BOOL		mLanguageTextInputAllowed;
	LLPreeditor*	mPreeditor;
	
	static BOOL	sUseMultGL;

	friend class LLWindowManager;
	
};


class LLSplashScreenMacOSX : public LLSplashScreen
{
public:
	LLSplashScreenMacOSX();
	virtual ~LLSplashScreenMacOSX();

	/*virtual*/ void showImpl();
	/*virtual*/ void updateImpl(const std::string& mesg);
	/*virtual*/ void hideImpl();

private:
	WindowRef   mWindow;
};

S32 OSMessageBoxMacOSX(const std::string& text, const std::string& caption, U32 type);

void load_url_external(const char* url);

#endif //LL_LLWINDOWMACOSX_H
