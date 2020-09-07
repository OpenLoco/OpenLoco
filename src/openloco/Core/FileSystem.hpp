/// @file
/// This file enables access to std::filesystem as `fs` namespace
/// or injects third-party drop-in replacement on build configs
/// where std::filesystem is not available.

#pragma once

// Find out if std::filesystem is available:
#if defined(_MSC_VER) // Visual Studio supports <filesystem>
#define HAVE_STD_FILESYSTEM 1
#elif defined(__APPLE__) // XCode has the header, but reports error when included.
#define HAVE_STD_FILESYSTEM 0
#elif defined(__has_include) // For GCC/Clang check if the header exists.
#if __has_include(<filesystem>)
#define HAVE_STD_FILESYSTEM 1
#else
#define HAVE_STD_FILESYSTEM 0
#endif
#else // By default assume not supported.
#define HAVE_STD_FILESYSTEM 0
#endif

#if HAVE_STD_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "../thirdparty/filesystem.hpp"
namespace fs = ghc::filesystem;
#endif

#undef HAVE_STD_FILESYSTEM // Not needed any more, don't make it public.
