#ifndef SPDNET_BASE_PLATFORM_H
#define SPDNET_BASE_PLATFORM_H

#if defined _MSC_VER || defined __MINGW32__
#define SPDNET_PLATFORM_WINDOWS 1
#else
#define SPDNET_PLATFORM_LINUX 1
#endif

#if defined(SPDNET_PLATFORM_WINDOWS)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif


#ifdef __GNUC__
#define SPDNET_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#else
#define SPDNET_PREDICT_TRUE(x) (x)
#endif


#ifdef __GNUC__
#define SPDNET_PREDICT_FALSE(x) (__builtin_expect(x, 0))
#else
#define SPDNET_PREDICT_FALSE(x) (x)
#endif


#endif  // SPDNET_BASE_PLATFORM_H