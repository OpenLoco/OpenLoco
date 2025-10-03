set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_SYSTEM_PROCESSOR "i686")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

# Support both Debian/Ubuntu (/usr/lib/i386-linux-gnu) and Arch Linux (/usr/lib32) paths
set(CMAKE_FIND_ROOT_PATH "/usr/lib/i386-linux-gnu" "/usr/lib32")

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)

# Auto-detect PNG and ZLIB library paths for different distributions
if(EXISTS "/usr/lib32/libpng16.so" AND EXISTS "/usr/lib32/libz.so")
    # Arch Linux paths
    set(PNG_LIBRARY "/usr/lib32/libpng16.so" CACHE FILEPATH "PNG library")
    set(ZLIB_LIBRARY "/usr/lib32/libz.so" CACHE FILEPATH "ZLIB library")
elseif(EXISTS "/usr/lib/i386-linux-gnu/libpng16.so" AND EXISTS "/usr/lib/i386-linux-gnu/libz.so")
    # Debian/Ubuntu paths
    set(PNG_LIBRARY "/usr/lib/i386-linux-gnu/libpng16.so" CACHE FILEPATH "PNG library")
    set(ZLIB_LIBRARY "/usr/lib/i386-linux-gnu/libz.so" CACHE FILEPATH "ZLIB library")
endif()
