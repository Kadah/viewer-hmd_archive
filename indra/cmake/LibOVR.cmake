# -*- cmake -*-
include(Prebuilt)
use_prebuilt_binary(libovr)

if (WINDOWS)
  set(LIBOVR_LIBRARIES 
    debug libovrd.lib
    optimized libovr.lib
    )
elseif (DARWIN)
  set(LIBOVR_LIBRARIES 
    debug libovr.a
    optimized libovr.a
    )
endif (WINDOWS)
set(LIBOVR_INCLUDE_DIRS
  "${LIBS_PREBUILT_DIR}/include"
  "${LIBS_PREBUILT_DIR}/Src"
  )
