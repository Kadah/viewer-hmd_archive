#ifndef LL_VIEWER_EVENT_RECORDER
#define LL_VIEWER_EVENT_RECORDER


#include "linden_common.h" 

#include "lldir.h" 
#include "llsd.h"  
#include "llfile.h"
#include "llvfile.h"
#include "lldate.h"
#include "llsdserialize.h"
#include "llkeyboard.h"
#include "llstring.h"

#include <sstream>

#include "llsingleton.h" // includes llerror which we need here so we can skip the include here

class LLViewerEventRecorder : public LLSingleton<LLViewerEventRecorder>
{

 public:

  LLViewerEventRecorder(); // TODO Protect constructor better if we can (not happy in private section) - could add a factory... - we are singleton
  ~LLViewerEventRecorder();


  void updateMouseEventInfo(S32 local_x,S32 local_y, S32 global_x, S32 global_y,  std::string mName);
  void setMouseLocalCoords(S32 x,S32 y);
  void setMouseGlobalCoords(S32 x,S32 y);

  void logMouseEvent(std::string button_state, std::string button_name );
  void logKeyEvent(KEY key, MASK mask);
  void logKeyUnicodeEvent(llwchar uni_char);

  void logVisibilityChange(std::string xui, std::string name, BOOL visibility, std::string event_subtype);

  void clear_xui();
  std::string get_xui();
  void update_xui(std::string xui);

  bool getLoggingStatus();
  void setEventLoggingOn();
  void setEventLoggingOff();

  void playbackRecording();

  bool displayViewerEventRecorderMenuItems();


 protected:
  // On if we wish to log events at the moment - toggle via Develop/Recorder submenu
  bool logEvents;

  std::string mLogFilename;
  llofstream  mLog; 


 private:

  // Mouse event info 
  S32 global_x;
  S32 global_y;
  S32 local_x;
  S32 local_y;

  // XUI path of UI element
  std::string xui;

  // Actually write the event out to llsd log file
  void recordEvent(LLSD event);

  void clear(S32 r); 

  static const S32 UNDEFINED=-1;
};
#endif
