# -*- cmake -*-

if (WINDOWS)
  set(LIBOVR_LIBRARIES 
    debug libovrd.lib
    optimized libovr.lib
    )
endif (WINDOWS)
set(LIBOVR_INCLUDE_DIRS
  "${LIBS_PREBUILT_DIR}/include"
  "${LIBS_PREBUILT_DIR}/Src"
  )
