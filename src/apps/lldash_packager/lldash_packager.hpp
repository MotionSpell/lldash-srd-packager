/**
 * @file lldash_packager.hpp
 * @brief Public interface for the **LLDash Packager** library.
 *
 * This header exposes a C‑compatible API that allows an application to:
 *   - create / destroy a packager instance,
 *   - start packaging of a DASH/MP4 stream from a URL.
 *
 * The library is compiled as a shared object/DLL; the macros below expose
 * proper symbol visibility on Windows and Unix‑like platforms.
 */

#pragma once

#include <cstdint>
#include <cstddef> // size_t

/* --------------------------------------------------------------------------- *
 *  Platform specific export macro
 * --------------------------------------------------------------------------- */
#ifdef _WIN32
#   ifdef BUILDING_DLL
#       define LLDPKG_EXPORT __declspec(dllexport)
#   else
#       define LLDPKG_EXPORT __declspec(dllimport)
#   endif
#else
#   define LLDPKG_EXPORT __attribute__((visibility("default")))
#endif

/* --------------------------------------------------------------------------- *
 *  Convenience macro for building a four‑character code (FourCC) value.
 *  Example: VRT_4CC('c','w','i','1') → 0x63776931
 * --------------------------------------------------------------------------- */
#define VRT_4CC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

/*! @brief Current API version of the LLDashPackager.
 *
 * The value is encoded as a 32‑bit date: YYYYMMDD.
 * Clients should compare this against the constant passed to
 * {@link lldpkg_create} to verify ABI compatibility at runtime.
 */
const uint64_t LLDASH_PACKAGER_API_VERSION = 0x20250724;

extern "C" {

/*! @brief Opaque handle representing a live‑streaming pipeline. */
struct lldpkg_handle;

/*! @brief Description of a single media stream inside the packager.
 *
 * All members are **inherited** from the MPEG‑DASH SRD (Segmented
 * Representation Descriptor) specification.
 */
struct StreamDesc {
    /** FourCC codec identifier (e.g. VRT_4CC('c','w','i','1')). */
    uint32_t MP4_4CC;

    /* MPEG‑DASH SRD parameters ------------------------------------------ */
    uint32_t objectX;
    uint32_t objectY;
    uint32_t objectWidth;
    uint32_t objectHeight;
    uint32_t totalWidth;
    uint32_t totalHeight;
};

/*! @enum LLDashPackagerMessageLevel
 *  @brief Log level used by the callback function.
 */
enum LLDashPackagerMessageLevel {
    VRTMessageError   = 0, /**< Fatal error – pipeline will abort.      */
    VRTMessageWarning = 1, /**< Non‑fatal problem, but may indicate
                              a configuration issue.              */
    VRTMessageInfo    = 2, /**< Informational message.                 */
    VRTMessageDebug   = 3  /**< Verbose debugging output.              */
};

/*! @brief Callback type for user supplied log messages.
 *
 * The library will invoke this function on a background thread
 * whenever it has something to report. The callback must be fast and
 * non‑blocking; the library guarantees that it will not be called
 * concurrently with itself, but may be called from any thread.
 *
 * @param msg   NULK‑terminated message string.
 * @param level Severity of the message – one of {@link LLDashPackagerMessageLevel}.
 */
typedef void (*LLDashPackagerMessageCallback)(const char *msg,
                                              int level);

/* --------------------------------------------------------------------------- *
 *  API functions
 * --------------------------------------------------------------------------- */

/*! @brief Create a new packager/streamer instance.
 *
 * The function allocates and configures an internal pipeline that
 * accepts raw media buffers, segments them according to the
 * specified parameters, and optionally publishes the resulting
 * fragments to a remote server via HTTP(S).
 *
 * @param name                Optional human‑readable identifier for the
 *                            instance.  May be `nullptr` or empty.
 * @param onError             Callback invoked by the library for log
 *                            messages. Pass `nullptr` for no logging.
 * @param level               Logging verbosity (see
 *                            {@link LLDashPackagerMessageLevel}).
 * @param num_streams         Number of media streams to be managed.
 * @param streams             Array of @c StreamDesc structures – must
 *                            outlive the handle.  The library **copies**
 *                            the descriptors internally, so you can free
 *                            your buffer afterwards.
 * @param publish_url         Optional URL where fragments will be POSTed.
 *                            Leave empty if you only want local storage.
 * @param seg_dur_in_ms       Target segment duration in milliseconds.
 * @param timeshift_buffer_depth_in_ms  Depth of the time‑shift buffer
 *                            (used for low‑latency playback).
 * @param api_version         Optional API version to validate against.
 *
 * @return Pointer to a newly allocated {@link lldpkg_handle}.  Must be
 *         destroyed with {@link lldpkg_destroy} when no longer needed.
 */
LLDPKG_EXPORT lldpkg_handle* lldpkg_create(
    const char* name,
    LLDashPackagerMessageCallback onError,
    int level,
    int num_streams,
    const StreamDesc* streams,
    const char *publish_url = "",
    int seg_dur_in_ms = 10000,
    int timeshift_buffer_depth_in_ms = 30000,
    uint64_t api_version = LLDASH_PACKAGER_API_VERSION);

/*! @brief Destroy a previously created pipeline.
 *
 * Frees all internal resources.  The caller can optionally flush any
 * buffered data before destruction.
 *
 * @param h     Handle returned by {@link lldpkg_create}.
 * @param flush If `true`, the library will push remaining fragments to
 *              the destination (if any) before freeing memory.
 */
LLDPKG_EXPORT void lldpkg_destroy(lldpkg_handle* h, bool flush = false);

/*! @brief Push a raw media buffer into the pipeline.
 *
 * The function copies the contents of `buffer` internally.  The caller
 * retains ownership and may immediately reuse or free the original
 * memory after this call returns.
 *
 * @param h            Handle returned by {@link lldpkg_create}.
 * @param stream_index Zero‑based index into the array passed to
 *                     {@link lldpkg_create}.  Must be < `num_streams`.
 * @param buffer       Pointer to raw media data (e.g. a frame or packet).
 * @param bufferSize   Size of the buffer in bytes.
 *
 * @return `true` if the push succeeded; `false` if the pipeline is in an
 *         error state and will not accept further input.
 */
LLDPKG_EXPORT bool lldpkg_push_buffer(
    lldpkg_handle* h,
    int stream_index,
    const uint8_t * buffer,
    const size_t bufferSize);

/*! @brief Retrieve the current media time for a stream.
 *
 * The returned value is expressed in the supplied `timescale` unit
 * (e.g. 1000 for milliseconds).  If an error occurs, -1 is returned.
 *
 * @param h           Handle returned by {@link lldpkg_create}.
 * @param stream_index Zero‑based index of the stream.
 * @param timescale   Desired units per second.
 *
 * @return Current media time in `timescale` units or -1 on error.
 */
LLDPKG_EXPORT int64_t lldpkg_get_media_time(
    lldpkg_handle* h,
    int stream_index,
    int timescale);

/*! @brief Return the library version string.
 *
 * The format is typically “MAJOR.MINOR.PATCH” and may include a
 * build‑date suffix.  This function is safe to call before
 * {@link lldpkg_create}.
 *
 * @return NUL‑terminated C string containing the version.
 */
LLDPKG_EXPORT const char *lldpkg_get_version();

} // extern "C"
