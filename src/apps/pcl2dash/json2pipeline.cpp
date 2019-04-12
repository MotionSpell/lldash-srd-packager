#include "lib_pipeline/pipeline.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "lib_media/stream/mpeg_dash.hpp"
#include "lib_utils/os.hpp"
#include "multi_file_reader.hpp"
#include "cwi_pcl_encoder.hpp"
#include "options.hpp"

using namespace Modules;
using namespace Pipelines;

extern const char *g_appName;
extern encoder_params parseParamFile(std::string param_file);

std::unique_ptr<Pipeline> buildPipeline(const IConfig &iconfig) {
	auto config = safe_cast<const Config>(&iconfig);
	auto pipeline = uptr(new Pipeline(false, (Pipeline::Threading)config->threading));

	auto input = pipeline->addModule<MultifileReader>(config->inputPath, config->numFrames);

	encoder_params pclEncoderParams;
	if (!config->param_file.empty())
		pclEncoderParams = parseParamFile(config->param_file);
	auto pclEncoder = pipeline->addModule<CWI_PCLEncoder>(pclEncoderParams);
	pipeline->connect(input, 0, pclEncoder, 0);

	auto const prefix = Stream::AdaptiveStreamingCommon::getCommonPrefixVideo(0, Resolution(0, 0));
	auto const subdir = prefix + "/";
	if (!dirExists(subdir))
		mkdir(subdir);

	auto muxer = pipeline->addModule<Mux::GPACMuxMP4>(subdir + prefix, config->segDurInMs == 0 ? 1 : config->segDurInMs,
		Mux::GPACMuxMP4::FragmentedSegment, Mux::GPACMuxMP4::OneFragmentPerFrame, Mux::GPACMuxMP4::ExactInputDur | Mux::GPACMuxMP4::SegNumStartsAtZero,
		(('c' << 24) | ('w' << 16) | ('i' << 8) | '1'));
	pipeline->connect(pclEncoder, 0, muxer, 0);

	auto dasher = pipeline->addModule<Stream::MPEG_DASH>("", format("%s.mpd", g_appName), Stream::AdaptiveStreamingCommon::Live, config->segDurInMs,
		0, 0, 0, std::vector<std::string>{}, "id", 0, config->delayInSeg);
	pipeline->connect(muxer, 0, dasher, 0);

	return pipeline;
}
