#include "cwi_pcl_encoder.hpp"
#include "lib_media/common/libav.hpp"
#include "lib_utils/profiler.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

using namespace Modules;

//#define FIXME_USE_FAKE_PCC

CWI_PCLEncoder::CWI_PCLEncoder(const encoder_params &params)
: params(params) {
	addInput(new Input<DataBase>(this));
	output = addOutput<OutputDataDefault<DataAVPacket>>();
	auto codecCtx = shptr(avcodec_alloc_context3(nullptr));
	//TODO: capture frame rate: int fps
	codecCtx->time_base = { IClock::Rate, 1 };
	/*FrameRate: ok
	Geometry resolution: TODO: have a new quantizer type
	Quantization parameter: TODO: have a new quality/bitrate type*/
	//codecCtx->extradata_size = ;
	//codecCtx->extradata = av_malloc(codecCtx->extradata_size);
	output->setMetadata(make_shared<MetadataPktLibavVideo>(codecCtx));
}

CWI_PCLEncoder::~CWI_PCLEncoder() {
}

void CWI_PCLEncoder::process(Data data) {
	Tools::Profiler p("Processing PCC frame");
	auto out = output->getBuffer(0);
	AVPacket *pkt = out->getPacket();

	{
#ifndef FIXME_USE_FAKE_PCC
		Tools::Profiler p("  Encoding time only");
		std::stringstream comp_frame;
		cwi_encode encoder;
		encoder.cwi_encoder(params, *((void**)data->data()), comp_frame);
		auto const resData = comp_frame.str();
		auto const resDataSize = resData.size();
		if (av_grow_packet(pkt, (int)resDataSize))
			throw error(format("impossible to resize sample to size %s", resDataSize));
		memcpy(pkt->data, resData.c_str(), resDataSize);
		auto ptr = *(void**)data->data();
		delete_ply_data(*(void**)data->data());
#else /*FIXME_USE_FAKE_PCC*/
		cwi_test(nullptr);
		av_grow_packet(pkt, 100);
#endif /*FIXME_USE_FAKE_PCC*/
	}

	pkt->dts = pkt->pts = data->getMediaTime();
	pkt->flags |= AV_PKT_FLAG_KEY;
	out->setMediaTime(data->getMediaTime());

	//timestamp: long netTimestamp, long captureTimestamp
	//Geometry resolution (Number of points): long int ptCount
	//Point size: float ptSize

	output->emit(out);
}