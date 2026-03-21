# toolchain file for cross-compiling from Linux to Windows

# set the target OS and architecture
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# specify the cross compiler
# Prefer the explicit posix-threading variant (Debian/Ubuntu ships both win32 and posix variants).
# On openSUSE/Fedora the plain name already uses posix threading, so fall back to that.
find_program(MINGW_C   NAMES x86_64-w64-mingw32-gcc-posix x86_64-w64-mingw32-gcc REQUIRED)
find_program(MINGW_CXX NAMES x86_64-w64-mingw32-g++-posix x86_64-w64-mingw32-g++ REQUIRED)
set(CMAKE_C_COMPILER   ${MINGW_C})
set(CMAKE_CXX_COMPILER ${MINGW_CXX})
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# where is the target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
