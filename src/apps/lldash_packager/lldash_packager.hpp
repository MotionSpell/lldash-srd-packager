#pragma once

#include <cstdint>
#include <cstddef> // size_t

#ifdef _WIN32
#ifdef BUILDING_DLL
#define LLDPKG_EXPORT __declspec(dllexport)
#else
#define LLDPKG_EXPORT __declspec(dllimport)
#endif
#else
#define LLDPKG_EXPORT __attribute__((visibility("default")))
#endif

#define VRT_4CC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

const uint64_t LLDASH_PACKAGER_API_VERSION = 0x20250620B;

extern "C" {
// Opaque handle.
struct lldpkg_handle;

struct StreamDesc {
    uint32_t MP4_4CC; // codec identifier. Build with VRT_4CC(). For example VRT_4CC('c','w','i','1') for "cwi1".

    /*MPEG-DASH SRD parameters*/
    uint32_t objectX;
    uint32_t objectY;
    uint32_t objectWidth;
    uint32_t objectHeight;
    uint32_t totalWidth;
    uint32_t totalHeight;
};

enum LLDashPackagerMessageLevel { VRTMessageError=0, VRTMessageWarning, VRTMessageInfo, VRTMessageDebug };
typedef void (*LLDashPackagerMessageCallback)(const char *msg, int level);

// Creates a new packager/streamer and starts the streaming session.
// @streams: owned by caller
// The returned pipeline must be freed using vrt_destroy().
LLDPKG_EXPORT lldpkg_handle* lldpkg_create(const char* name, LLDashPackagerMessageCallback onError, int num_streams, const StreamDesc* streams, const char *publish_url = "", int seg_dur_in_ms = 10000, int timeshift_buffer_depth_in_ms = 30000, uint64_t api_version = LLDASH_PACKAGER_API_VERSION);


// Destroys a pipeline. This frees all the resources.
LLDPKG_EXPORT void lldpkg_destroy(lldpkg_handle* h);

// Pushes a buffer to @stream_index. The caller owns it ; the buffer will be copied internally.
// Returns false when the error flag of the pipeline is set.
LLDPKG_EXPORT bool lldpkg_push_buffer_ext(lldpkg_handle* h, int stream_index, const uint8_t * buffer, const size_t bufferSize);


// Gets the current media time in @timescale unit for @stream_index. Returns -1 on error.
LLDPKG_EXPORT int64_t lldpkg_get_media_time_ext(lldpkg_handle* h, int stream_index, int timescale);



// Gets the current parent version. Used to ensure build consistency.
LLDPKG_EXPORT const char *lldpkg_get_version();
}
