#pragma once
#ifndef BUILD_BUILD_CONFIG_H_
#define BUILD_BUILD_CONFIG_H_

// Compiler detection.
#if defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#else
#error Please add support for your compiler in build/build_config.h
#endif

#if defined(COMPILER_MSVC)
typedef __int64 int64;
#elif defined(COMPILER_GCC)
#include <inttypes.h>
typedef int64_t int64;
#endif

#endif // BUILD_BUILD_CONFIG_H_
