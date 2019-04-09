#pragma once

#include <cstdint>
#include <cstddef> // size_t

#ifdef _WIN32
#ifdef BUILDING_DLL
#define VRT_EXPORT __declspec(dllexport)
#else
#define VRT_EXPORT __declspec(dllimport)
#endif
#else
#define VRT_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
// Opaque handle.
struct vrt_handle;

// Creates a new packager/streamer and starts the streaming session.
// The returned pipeline must be freed using vrt_destroy().
VRT_EXPORT vrt_handle* vrt_create(const char* name, int seg_dur_in_ms = 10000);

// Destroys a pipeline. This frees all the resources.
VRT_EXPORT void vrt_destroy(vrt_handle* h);

// Pushes a buffer. The caller owns it ; the buffer  as it will be copied internally.
VRT_EXPORT bool vrt_push_buffer(vrt_handle* h, const uint8_t * buffer, const size_t bufferSize);
}
