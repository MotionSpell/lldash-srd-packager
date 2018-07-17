#include "lib_pipeline/pipeline.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "lib_media/stream/mpeg_dash.hpp"
#include "multi_file_reader.hpp"
#include "cwi_pcl_encoder.hpp"
#include "options.hpp"

using namespace Modules;
using namespace Pipelines;

extern const char *g_appName;

std::unique_ptr<Pipeline> buildPipeline(const IConfig &iconfig) {
	auto pipeline = uptr(new Pipeline);
	auto config = safe_cast<const Config>(&iconfig);
	auto const segDurationInMs = 1000;

	auto input = pipeline->addModule<MultifileReader>(config->inputPath);
	encoder_params pclEncoderParams;
	auto pclEncoder = pipeline->addModule<CWI_PCLEncoder>(pclEncoderParams);
	pipeline->connect(input, 0, pclEncoder, 0);

	auto muxer = pipeline->addModule<Mux::GPACMuxMP4>("pcl", segDurationInMs, Mux::GPACMuxMP4::FragmentedSegment, Mux::GPACMuxMP4::OneFragmentPerFrame, Mux::GPACMuxMP4::Browsers | Mux::GPACMuxMP4::ExactInputDur | Mux::GPACMuxMP4::NoEditLists);
	pipeline->connect(pclEncoder, 0, muxer, 0);

	auto dasher = pipeline->addModule<Stream::MPEG_DASH>("", format("%s.mpd", g_appName), Stream::AdaptiveStreamingCommon::Live, segDurationInMs);
	pipeline->connect(muxer, 0, dasher, 0);

	return pipeline;
}
