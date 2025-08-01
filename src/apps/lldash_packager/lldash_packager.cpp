#define BUILDING_DLL
#include "lldash_packager.hpp"

#include "lib_pipeline/pipeline.hpp"
#include "lib_media/out/http.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "plugins/Dasher/mpeg_dash.hpp"
#include "lib_media/stream/adaptive_streaming_common.hpp"
#include "lib_media/out/filesystem.hpp"
#include "lib_media/out/http_sink.hpp"
#include "lib_media/common/attributes.hpp"
#include "lib_utils/format.hpp"
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

struct Logger : LogSink
{
  void send(Level level, const char* msg) override
  {
	if(level > maxLevel)
		return;

	if (onError) {
		onError(format("[lldpkg::%s] %s\n", name.c_str(), msg).c_str(), (int)level);
	} else {
		fprintf(stderr, "[lldpkg::%s] %s\n", name.c_str(), msg);
		fflush(stderr);
	}
  }

  Level maxLevel = Level::Info;
  string name;
  std::function<void(const char*, int level)> onError = nullptr;
};

struct lldpkg_handle {
	lldpkg_handle(int num_streams) : streams(num_streams) {}

	struct Stream {
		Stream() : fifo(256) {}
		int64_t initTimeIn180k = fractionToClock(g_SystemClock->now());
		int64_t timeIn180k = -1;
		QueueLockFree<Data> fifo;
	};
	vector<Stream> streams;

	unique_ptr<Pipeline> pipe;

	bool error = false;
	Logger logger;
	std::function<bool(const char*)> errorCbk;
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

static bool startsWith(string s, string prefix) {
  return s.substr(0, prefix.size()) == prefix;
}

lldpkg_handle* lldpkg_create(const char* name, LLDashPackagerMessageCallback onError, int level, int num_streams, const StreamDesc* streams, const char* publish_url, int seg_dur_in_ms, int timeshift_buffer_depth_in_ms, uint64_t api_version) {
	try {
		if (api_version != LLDASH_PACKAGER_API_VERSION)
			throw std::runtime_error(format("Inconsistent API version between compilation (%s) and runtime (%s). Aborting.", LLDASH_PACKAGER_API_VERSION, api_version).c_str());

		auto h = make_unique<lldpkg_handle>(num_streams);
		h->logger.name = name;
		h->logger.maxLevel = (Level)level;
		h->logger.setLevel((Level)level);
		h->logger.onError = onError;
		h->errorCbk = [onError](const char *msg) { 
			if (onError) onError(msg, Level::Error);
			return true;
		};
		setGlobalLogger(h->logger);

		// Pipeline
		h->pipe = make_unique<Pipeline>(&h->logger, false, Threading::OnePerModule);

		// Error management
		auto hErr = h.get();
		h->pipe->registerErrorCallback([&](const char *str) {
			h->logger.log(Info, format("Error flag set because \"%s\"", str).c_str());
			hErr->error = true;
			h->errorCbk(str);
			return false;
		});

		// Build parameters
		auto mp4Flags = ExactInputDur | SegNumStartsAtZero;
		if(startsWith(publish_url, "http")) {
			mp4Flags = mp4Flags | FlushFragMemory;
		} else {
			auto const prefix = Stream::AdaptiveStreamingCommon::getCommonPrefixVideo(0, Resolution(0, 0));
			auto const subdir = prefix + "/";
			if (!dirExists(subdir))
				mkdir(subdir);
		}

		// Create Dasher
		Modules::DasherConfig dashCfg {};
		dashCfg.mpdName = format("%s.mpd", name);
		dashCfg.live = true;
		dashCfg.forceRealDurations = true;
		dashCfg.presignalNextSegment = true;
		dashCfg.segDurationInMs = seg_dur_in_ms;
		dashCfg.timeShiftBufferDepthInMs = timeshift_buffer_depth_in_ms;
		dashCfg.initialOffsetInMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		if (num_streams > 1)
			for (int stream = 0; stream < num_streams; ++stream)
				dashCfg.tileInfo.push_back({ 0, (int)streams[stream].objectX,
					(int)streams[stream].objectY, (int)streams[stream].objectWidth,
					(int)streams[stream].objectHeight, (int)streams[stream].totalWidth,
					(int)streams[stream].totalHeight });
		auto dasher = h->pipe->add("MPEG_DASH", &dashCfg);
		
		// Create sink
		IFilter* sink = nullptr;
		if(startsWith(publish_url, "http")) {
			HttpOutputConfig sinkCfg {};
			sinkCfg.url = publish_url;
			sinkCfg.userAgent = "bin2dash";
			sinkCfg.maxConnectFailCount = 0;
			sink = h->pipe->add("HttpSink", &sinkCfg);
			h->logger.log(Info, format("Pushing to HTTP at \"%s\"", publish_url).c_str());
		} else {
			FileSystemSinkConfig sinkCfg {};
			sinkCfg.directory = publish_url;
			sink = h->pipe->add("FileSystemSink", &sinkCfg);
			h->logger.log(Info, format("Pushing to filesystem at \"%s\"", publish_url).c_str());
		}
		h->pipe->connect(dasher, sink);
		h->pipe->connect(GetOutputPin(dasher, 1), sink, true);

		for (int stream = 0; stream < num_streams; ++stream) {
			auto source = h->pipe->addModule<ExternalSource>(h->streams[stream].fifo);

			Mp4MuxConfig cfg {};
			cfg.segmentDurationInMs = seg_dur_in_ms == 0 ? 1 : seg_dur_in_ms;
			cfg.segmentPolicy = FragmentedSegment;
			cfg.fragmentPolicy = OneFragmentPerFrame;
			cfg.compatFlags = mp4Flags;
			cfg.MP4_4CC = streams[stream].MP4_4CC;
			auto muxer = h->pipe->add("GPACMuxMP4", &cfg);
			h->pipe->connect(source, muxer);
			h->pipe->connect(muxer, GetInputPin(dasher, stream));

			auto data = make_shared<DataRaw>(0);
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
		if (onError) {
			char errbuf[128];
			snprintf(errbuf, sizeof(errbuf), "[%s] exception caught: %s\n", __func__, err.what());
			onError(errbuf, Level::Error);
		}
		return nullptr;
	}
}


void lldpkg_destroy(lldpkg_handle* h, bool flush) {
	try {
		if (flush) {
			h->pipe->exitSync();
			h->pipe->waitForEndOfStream();
		}

		delete h;
	} catch (exception const& err) {
		fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
		fflush(stderr);
		if (h && h->errorCbk) {
			char errbuf[128];
			snprintf(errbuf, sizeof(errbuf), "[%s] exception caught: %s\n", __func__, err.what());
			h->errorCbk(errbuf);
		}
	}
}

bool lldpkg_push_buffer(lldpkg_handle* h, int stream_index, const uint8_t * buffer, const size_t bufferSize) {
	try {
		if (!h)
			throw runtime_error("[lldpkg_push_buffer] handle can't be NULL");
		if (!buffer)
			throw runtime_error("[lldpkg_push_buffer] buffer can't be NULL");
		if (stream_index < 0 || stream_index >= (int)h->streams.size())
			throw runtime_error("[lldpkg_push_buffer] invalid stream_index");
		if (h->error)
			throw runtime_error("[lldpkg_push_buffer] error state detected");
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
		h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
		return false;
	}
}


int64_t lldpkg_get_media_time(lldpkg_handle* h, int stream_index, int timescale) {
	try {
		if (!h)
			throw runtime_error("[lldpkg_get_media_time] handle can't be NULL");
		if (stream_index < 0 || stream_index >= (int)h->streams.size())
			throw runtime_error("[lldpkg_push_buffer] invalid stream_index");

		return rescale(h->streams[stream_index].timeIn180k, IClock::Rate, timescale);
	} catch (exception const& err) {
		h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
		return -1;
	}
}


const char *lldpkg_get_version() {
#ifdef LLDASH_VERSION
#define LLDASH_VERSION_STRINGIFY2(x) LLDASH_VERSION_STRINGIFY(x)
#define LLDASH_VERSION_STRINGIFY(x) #x
  return LLDASH_VERSION_STRINGIFY2(LLDASH_VERSION);
#else
	return "unknown";
#endif
}

