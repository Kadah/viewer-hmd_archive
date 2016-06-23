# -*- cmake -*-

if (OPENVR)
include(Prebuilt)
use_prebuilt_binary(openvr)

if (WINDOWS)
  set(OPENVR_LIBRARIES 
    debug libopenvr.lib
    optimized libopenvr.lib
    )
  set(OPENVR_EXE_LINKER_FLAGS 
   "/DELAYLOAD:openvr_api.dll"
    )
elseif (DARWIN)
  set(OPENVR_LIBRARIES 
    debug libopenvr.dylib
    optimized libopenvr.dylib
    )
elseif (LINUX)
  set(OPENVR_LIBRARIES 
    debug libopenvr.so
    optimized libopenvr.so
    )
endif (WINDOWS)
set(OPENVR_INCLUDE_DIRS
  "${LIBS_PREBUILT_DIR}/include"
  )
set(OPENVR_DEFINES
  "-DLL_HMD_OPENVR_SUPPORTED=1"
  )
endif (OPENVR)
