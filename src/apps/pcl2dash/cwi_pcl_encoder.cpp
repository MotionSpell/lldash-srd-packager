#include "cwi_pcl_encoder.hpp"
#include "lib_media/common/libav.hpp"
#include "lib_utils/profiler.hpp"
#include <cassert>

extern "C" {
#include <libavcodec/avcodec.h>
}

/*the current encoder tries to do some inter-frame encoding but is instable - this hack creates an encoder for each frame*/
static auto const CWI_INTER_HACK = 1;

using namespace Modules;

CWI_PCLEncoder::CWI_PCLEncoder(cwipc_encoder_params params)
: params(params) {
	addInput(new Input<DataBase>(this));
	output = addOutput<OutputDataDefault<DataAVPacket>>();
	auto codecCtx = shptr(avcodec_alloc_context3(nullptr));
	//TODO: capture frame rate: int fps
	codecCtx->time_base = { 1, IClock::Rate };
	/*FrameRate: ok
	Geometry resolution: TODO: have a new quantizer type
	Quantization parameter: TODO: have a new quality/bitrate type*/
	//codecCtx->extradata_size = ;
	//codecCtx->extradata = av_malloc(codecCtx->extradata_size);
	output->setMetadata(make_shared<MetadataPktLibavVideo>(codecCtx));

	encoder = cwipc_new_encoder(CWIPC_ENCODER_PARAM_VERSION, &params);
}

CWI_PCLEncoder::~CWI_PCLEncoder() {
	/*TODO: flush()*/
	cwipc_encoder_free(encoder);
}

void CWI_PCLEncoder::process(Data data) {
	Tools::Profiler p("Processing PCC frame");
	auto out = output->getBuffer(0);
	AVPacket *pkt = out->getPacket();

	{
		Tools::Profiler p("  Encoding time only");

		if(CWI_INTER_HACK)
		{
			assert(!cwipc_encoder_available(encoder, false)); /*we are already flushed*/
			cwipc_encoder_free(encoder);
			encoder = cwipc_new_encoder(CWIPC_ENCODER_PARAM_VERSION, &params);
		}

		auto pc = *((cwipc**)(data->data()));
		cwipc_encoder_feed(encoder, pc);
		cwipc_free(pc);

		assert(cwipc_encoder_available(encoder, false));
		auto const resDataSize = cwipc_encoder_get_encoded_size(encoder);
		if (av_grow_packet(pkt, (int)resDataSize))
			throw error(format("impossible to resize sample to size %s", resDataSize));
		if (!cwipc_encoder_copy_data(encoder, (void*)pkt->data, resDataSize))
			throw error(format("impossible to copy the encoded data (size %s)", resDataSize));
		pkt->dts = pkt->pts = data->getMediaTime();
		pkt->flags |= AV_PKT_FLAG_KEY;
		out->setMediaTime(data->getMediaTime());
		//timestamp: long netTimestamp, long captureTimestamp
		//Geometry resolution (Number of points): long int ptCount
		//Point size: float ptSize
		output->emit(out);

		assert(!cwipc_encoder_available(encoder, false));
	}
}
