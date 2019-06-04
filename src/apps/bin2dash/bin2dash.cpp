#define BUILDING_DLL
#include "bin2dash.hpp"

#include "lib_pipeline/pipeline.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "lib_media/stream/mpeg_dash.hpp"
#include "lib_media/stream/adaptive_streaming_common.hpp"
#include "lib_media/common/attributes.hpp"
#include "../common/http_poster.hpp"
#include "lib_utils/os.hpp"
#include "lib_utils/log.hpp"
#include "lib_utils/system_clock.hpp"
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
}

using namespace Modules;
using namespace Pipelines;
using namespace std;

struct vrt_handle {
	unique_ptr<Pipeline> pipe;
	shared_ptr<DataRaw> inputData;
	IFilter *inputModule = nullptr;
	int64_t initTimeIn180k = fractionToClock(g_SystemClock->now()), timeIn180k = -1;
};

vrt_handle* vrt_create(const char* name, uint32_t MP4_4CC, const char* publish_url, int seg_dur_in_ms, int timeshift_buffer_depth_in_ms) {
	try {
		auto h = make_unique<vrt_handle>();

		// Input data
		h->inputData = make_shared<DataRaw>(0);
		h->inputData->setMetadata(make_shared<MetadataPktVideo>());

		// Pipeline
		h->pipe = make_unique<Pipeline>(g_Log, false, Threading::Mono);
		std::string mp4Basename;
		auto mp4Flags = ExactInputDur | SegNumStartsAtZero;
		if (!publish_url || strlen(publish_url) <= 0) {
			auto const prefix = Stream::AdaptiveStreamingCommon::getCommonPrefixVideo(0, Resolution(0, 0));
			auto const subdir = prefix + "/";
			if (!dirExists(subdir))
				mkdir(subdir);

			mp4Basename = subdir + prefix;
		} else {
			mp4Flags = mp4Flags | FlushFragMemory;
		}

		Mp4MuxConfig cfg {};
		cfg.baseName = mp4Basename;
		cfg.segmentDurationInMs = seg_dur_in_ms == 0 ? 1 : seg_dur_in_ms;
		cfg.segmentPolicy = FragmentedSegment;
		cfg.fragmentPolicy = OneFragmentPerFrame;
		cfg.compatFlags = mp4Flags;
		cfg.MP4_4CC = MP4_4CC;
		auto muxer = h->inputModule = h->pipe->add("GPACMuxMP4", &cfg);
		Modules::DasherConfig dashCfg {};
		dashCfg.mpdName = format("%s.mpd", name);
		dashCfg.live = true;
		dashCfg.segDurationInMs = seg_dur_in_ms;
		dashCfg.timeShiftBufferDepthInMs = timeshift_buffer_depth_in_ms;
		auto dasher = h->pipe->add("MPEG_DASH", &dashCfg);
		h->pipe->connect(muxer, dasher);

		if (mp4Basename.empty()) {
			auto sink = h->pipe->addModule<HttpPoster>(publish_url, timeshift_buffer_depth_in_ms);
			h->pipe->connect(dasher, sink);
			h->pipe->connect(GetOutputPin(dasher, 1), sink, true);
		}

		h->pipe->start();

		return h.release();
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return nullptr;
	}
}

void vrt_destroy(vrt_handle* h) {
	try {
		delete h;
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
	}
}

bool vrt_push_buffer(vrt_handle* h, const uint8_t * buffer, const size_t bufferSize) {
	try {
		if (!h)
			throw runtime_error("[vrt_push_buffer] handle can't be NULL");
		if (!buffer)
			throw runtime_error("[vrt_push_buffer] buffer can't be NULL");

		auto data = safe_cast<DataRaw>(h->inputData);
		data->buffer->resize(bufferSize);
		memcpy(data->buffer->data().ptr, buffer, bufferSize);
		h->timeIn180k = fractionToClock(g_SystemClock->now()) - h->initTimeIn180k;
		data->set(PresentationTime { h->timeIn180k });
		data->set(DecodingTime { h->timeIn180k });
		CueFlags cueFlags {};
		cueFlags.keyframe = true;
		data->set(cueFlags);

//	h->inputModule->getInput(0)->push(data);
//	h->inputModule->process();
		return true;
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return false;
	}
}

int64_t vrt_get_media_time(vrt_handle* h, int timescale) {
	try {
		if (!h)
			throw runtime_error("[vrt_get_media_time] handle can't be NULL");

		return rescale(h->timeIn180k, IClock::Rate, timescale);
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return -1;
	}
}
