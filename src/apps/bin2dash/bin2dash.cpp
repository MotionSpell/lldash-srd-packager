#define BUILDING_DLL
#include "bin2dash.hpp"

#include "lib_pipeline/pipeline.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "lib_media/stream/mpeg_dash.hpp"
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

vrt_handle* vrt_create(const char* name, int seg_dur_in_ms, int timeshift_buffer_depth_in_ms) {
	try {
		auto h = make_unique<vrt_handle>();

		// Input data
		h->inputData = make_shared<DataAVPacket>(0);
		auto codecCtx = shptr(avcodec_alloc_context3(nullptr));
		codecCtx->time_base = { 1, IClock::Rate };
		h->inputData->setMetadata(make_shared<MetadataPktLibavVideo>(codecCtx));

		// Pipeline
		h->pipe = make_unique<Pipeline>();
		auto muxer = h->inputModule = h->pipe->addModule<Mux::GPACMuxMP4>(name, seg_dur_in_ms == 0 ? 1 : seg_dur_in_ms,
			Mux::GPACMuxMP4::FragmentedSegment, Mux::GPACMuxMP4::OneFragmentPerFrame, Mux::GPACMuxMP4::ExactInputDur);
		auto dasher = h->pipe->addModule<Stream::MPEG_DASH>("", format("%s.mpd", name), Stream::AdaptiveStreamingCommon::Live, seg_dur_in_ms, timeshift_buffer_depth_in_ms);
		h->pipe->connect(muxer, 0, dasher, 0);

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
		h->timeIn180k = fractionToClock(g_SystemClock->now()) - h->initTimeIn180k;
		data->setMediaTime(h->timeIn180k);
		//data->setMediaTime();
		//pkt->dts = ;
		//pkt->dts = ;
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
