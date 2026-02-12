#pragma once

// For static library builds, we don't need __declspec
#ifdef SCREEN_STREAM_SDK_STATIC
#define SCREEN_STREAM_SDK_EXPORT
#elif defined(SCREEN_STREAM_SDK_EXPORTS)
#define SCREEN_STREAM_SDK_EXPORT __declspec(dllexport)
#else
#define SCREEN_STREAM_SDK_EXPORT __declspec(dllimport)
#endif
