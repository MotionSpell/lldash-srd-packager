#define BUILDING_DLL
#include "bin2dash.hpp"

#include "lib_pipeline/pipeline.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "lib_media/out/http.hpp"
#include "lib_media/stream/mpeg_dash.hpp"
#include "lib_utils/os.hpp"
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
	shared_ptr<DataAVPacket> inputData;
	IPipelinedModule *inputModule = nullptr;
	int64_t initTimeIn180k = fractionToClock(g_SystemClock->now()), timeIn180k = -1;
};

vrt_handle* vrt_create(const char* name, uint32_t MP4_4CC, const char* publish_url, int seg_dur_in_ms, int timeshift_buffer_depth_in_ms) {
	try {
		auto h = make_unique<vrt_handle>();

		// Input data
		h->inputData = make_shared<DataAVPacket>(0);
		auto codecCtx = shptr(avcodec_alloc_context3(nullptr));
		codecCtx->time_base = { 1, IClock::Rate };
		h->inputData->setMetadata(make_shared<MetadataPktLibavVideo>(codecCtx));

		// Pipeline
		h->pipe = make_unique<Pipeline>(false, Pipeline::Mono);
		std::string mp4Basename;
		if (!publish_url || strlen(publish_url) <= 0) {
			auto const prefix = Stream::AdaptiveStreamingCommon::getCommonPrefixVideo(0, Resolution(0, 0));
			auto const subdir = prefix + "/";
			if (!dirExists(subdir))
				mkdir(subdir);

			mp4Basename = subdir + prefix;
		}

		auto muxer = h->inputModule = h->pipe->addModule<Mux::GPACMuxMP4>(mp4Basename, seg_dur_in_ms == 0 ? 1 : seg_dur_in_ms,
			Mux::GPACMuxMP4::FragmentedSegment, Mux::GPACMuxMP4::OneFragmentPerFrame, Mux::GPACMuxMP4::ExactInputDur | Mux::GPACMuxMP4::SegNumStartsAtZero, MP4_4CC);
		auto dasher = h->pipe->addModule<Stream::MPEG_DASH>("", format("%s.mpd", name), Stream::AdaptiveStreamingCommon::Live, seg_dur_in_ms, timeshift_buffer_depth_in_ms);
		h->pipe->connect(muxer, 0, dasher, 0);

		if (mp4Basename.empty()) {
			auto sink = h->pipe->addModule<Out::HTTP>(publish_url);
			h->pipe->connect(dasher, 0, sink, 0);
			h->pipe->connect(dasher, 1, sink, 0, true);
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
		auto data = safe_cast<DataAVPacket>(h->inputData);
		auto pkt = data->getPacket();
		data->resize(bufferSize);
		memcpy(data->data(), buffer, bufferSize);
		h->timeIn180k = fractionToClock(g_SystemClock->now()) - h->initTimeIn180k;
		data->setMediaTime(h->timeIn180k);
		pkt->dts = h->timeIn180k;
		pkt->pts = h->timeIn180k;
		pkt->flags |= AV_PKT_FLAG_KEY;

		h->inputModule->getInput(0)->push(data);
		h->inputModule->getInput(0)->process();
		return true;
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return false;
	}
}

int64_t vrt_get_media_time(vrt_handle* h, int timescale) {
	return convertToTimescale(h->timeIn180k, IClock::Rate, timescale);
}
