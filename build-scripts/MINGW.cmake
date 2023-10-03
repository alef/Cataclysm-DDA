#[=======================================================================[

MINGW
-----

Toolchain file for MingGW.

Used by CMakePresets.json -> "toolchainFile".

MSYS2: https://www.msys2.org/docs/cmake/

#]=======================================================================]

# We are cross-compiling
set(CMAKE_SYSTEM_NAME Windows)

# Fix GCC 13 bug, refer to CDDA #65542
string(APPEND CMAKE_CXX_FLAGS_INIT " -Wno-dangling-reference")

