#define BUILDING_DLL
#include "bin2dash.hpp"

#include "lib_pipeline/pipeline.hpp"
#include "lib_media/out/http.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "plugins/Dasher/mpeg_dash.hpp"
#include "lib_media/stream/adaptive_streaming_common.hpp"
#include "lib_media/out/http_sink.hpp"
#include "lib_media/common/attributes.hpp"
#include "lib_utils/os.hpp"
#include "lib_utils/log.hpp"
#include "lib_utils/time.hpp" //getUTC()
#include "lib_utils/system_clock.hpp"
#include "lib_utils/queue_lockfree.hpp"
#include <chrono>
#include <cstdio>

using namespace Modules;
using namespace Pipelines;
using namespace std;
using namespace chrono;

struct vrt_handle {
	vrt_handle(int num_streams) : streams(num_streams) {}

	struct Stream {
		Stream() : fifo(256) {}
		int64_t initTimeIn180k = fractionToClock(g_SystemClock->now());
		int64_t timeIn180k = -1;
		QueueLockFree<Data> fifo;
	};
	vector<Stream> streams;

	unique_ptr<Pipeline> pipe;
};

struct ExternalSource : Modules::Module {
	ExternalSource(Modules::KHost* host, QueueLockFree<Data> &fifo) : fifo(fifo) {
		out = addOutput();
		host->activate(true);
	}
	void process() override {
		Data data;
		if(!fifo.read(data)) {
			this_thread::sleep_for(chrono::milliseconds(10));
			return;
		}
		out->post(data);
	}
	Modules::OutputDefault* out;
	QueueLockFree<Data> &fifo;
};

vrt_handle* vrt_create_ext(const char* name, int num_streams, const streamDesc *streams, const char* publish_url, int seg_dur_in_ms, int timeshift_buffer_depth_in_ms) {
	try {
		auto h = make_unique<vrt_handle>(num_streams);

		// Pipeline
		h->pipe = make_unique<Pipeline>(g_Log, false, Threading::OnePerModule);

		// Build parameters
		string mp4Basename;
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

		// Create Dasher
		Modules::DasherConfig dashCfg {};
		dashCfg.mpdName = format("%s.mpd", name);
		dashCfg.live = true;
		dashCfg.segDurationInMs = seg_dur_in_ms;
		dashCfg.timeShiftBufferDepthInMs = timeshift_buffer_depth_in_ms;
		dashCfg.initialOffsetInMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		if (num_streams > 1)
			for (int stream = 0; stream < num_streams; ++stream)
				dashCfg.tileInfo.push_back({ 1, (int)streams[stream].tileNumber, 0, (int)streams[stream].quality, 0 });
		auto dasher = h->pipe->add("MPEG_DASH", &dashCfg);
		
		// Create sink
		if (mp4Basename.empty()) {
			HttpOutputConfig sinkCfg {};
			sinkCfg.url = publish_url;
			sinkCfg.userAgent = "bin2dash";

			auto sink = h->pipe->add("HttpSink", &sinkCfg);
			h->pipe->connect(dasher, sink);
			h->pipe->connect(GetOutputPin(dasher, 1), sink, true);
		}

		for (int stream = 0; stream < num_streams; ++stream) {
			auto source = h->pipe->addModule<ExternalSource>(h->streams[stream].fifo);

			Mp4MuxConfig cfg {};
			cfg.baseName = mp4Basename;
			cfg.segmentDurationInMs = seg_dur_in_ms == 0 ? 1 : seg_dur_in_ms;
			cfg.segmentPolicy = FragmentedSegment;
			cfg.fragmentPolicy = OneFragmentPerFrame;
			cfg.compatFlags = mp4Flags;
			cfg.MP4_4CC = streams[stream].MP4_4CC;
			auto muxer = h->pipe->add("GPACMuxMP4", &cfg);
			h->pipe->connect(source, muxer);
			h->pipe->connect(muxer, GetInputPin(dasher, stream));

			auto data = make_shared<DataBaseRef>(nullptr);
			auto meta = make_shared<MetadataPktVideo>();
			meta->timeScale = Fraction(1000, 1);
			data->setMetadata(meta);
			data->set(PresentationTime{ });
			data->set(DecodingTime{ });
			data->set(CueFlags{});
			h->streams[stream].fifo.write(data);
		}

		h->pipe->start();

		return h.release();
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return nullptr;
	}
}

vrt_handle* vrt_create(const char* name, uint32_t MP4_4CC, const char *publish_url, int seg_dur_in_ms, int timeshift_buffer_depth_in_ms) {
	streamDesc sd = { MP4_4CC, 0, 0 };
	return vrt_create_ext(name, 1, &sd, publish_url, seg_dur_in_ms, timeshift_buffer_depth_in_ms);
}

void vrt_destroy(vrt_handle* h) {
	try {
		delete h;
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
	}
}

bool vrt_push_buffer_ext(vrt_handle* h, int stream_index, const uint8_t * buffer, const size_t bufferSize) {
	try {
		if (!h)
			throw runtime_error("[vrt_push_buffer] handle can't be NULL");
		if (!buffer)
			throw runtime_error("[vrt_push_buffer] buffer can't be NULL");
		if (stream_index < 0 || stream_index >= (int)h->streams.size())
			throw runtime_error("[vrt_push_buffer] invalid stream_index");

		h->streams[stream_index].timeIn180k = fractionToClock(g_SystemClock->now()) - h->streams[stream_index].initTimeIn180k;

		auto data = make_shared<DataRaw>(bufferSize);
		memcpy(data->buffer->data().ptr, buffer, bufferSize);
		data->set(PresentationTime { h->streams[stream_index].timeIn180k });
		data->set(DecodingTime { h->streams[stream_index].timeIn180k });
		CueFlags cueFlags {};
		cueFlags.keyframe = true;
		data->set(cueFlags);
		h->streams[stream_index].fifo.write(data);

		return true;
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return false;
	}
}

bool vrt_push_buffer(vrt_handle* h, const uint8_t * buffer, const size_t bufferSize) {
	return vrt_push_buffer_ext(h, 0, buffer, bufferSize);
}

int64_t vrt_get_media_time_ext(vrt_handle* h, int stream_index, int timescale) {
	try {
		if (!h)
			throw runtime_error("[vrt_get_media_time] handle can't be NULL");
		if (stream_index < 0 || stream_index >= (int)h->streams.size())
			throw runtime_error("[vrt_push_buffer] invalid stream_index");

		return rescale(h->streams[stream_index].timeIn180k, IClock::Rate, timescale);
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		return -1;
	}
}

int64_t vrt_get_media_time(vrt_handle* h, int timescale) {
	return vrt_get_media_time_ext(h, 0, timescale);
}

