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

#define VRT_4CC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

extern "C" {
// Opaque handle.
struct vrt_handle;

struct streamDesc {
    uint32_t MP4_4CC; // codec identifier. Build with VRT_4CC(). For example VRT_4CC('c','w','i','1') for "cwi1".
    uint32_t tileNumber;
    uint32_t quality;
};

// Creates a new packager/streamer and starts the streaming session.
// @streams: owned by caller
// The returned pipeline must be freed using vrt_destroy().
VRT_EXPORT vrt_handle* vrt_create(const char* name, int num_streams, const streamDesc *streams, const char *publish_url = "", int seg_dur_in_ms = 10000, int timeshift_buffer_depth_in_ms = 30000);

// Destroys a pipeline. This frees all the resources.
VRT_EXPORT void vrt_destroy(vrt_handle* h);

// Pushes a buffer to @stream_index. The caller owns it ; the buffer will be copied internally.
VRT_EXPORT bool vrt_push_buffer(vrt_handle* h, int stream_index, const uint8_t * buffer, const size_t bufferSize);

// Gets the current media time in @timescale unit for @stream_index. Returns -1 on error.
VRT_EXPORT int64_t vrt_get_media_time(vrt_handle* h, int stream_index, int timescale);
}
