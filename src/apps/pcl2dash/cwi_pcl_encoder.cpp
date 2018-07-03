#include "cwi_pcl_encoder.hpp"
#include <cwi_encode/cwi_encode.h>
#include "lib_utils/profiler.hpp"

using namespace Modules;

#define FIXME_USE_FAKE_PCC

CWI_PCLEncoder::CWI_PCLEncoder(const Params &params)
: params(params) {
	addInput(new Input<DataBase>(this));
	output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
}

CWI_PCLEncoder::~CWI_PCLEncoder() {
}

void CWI_PCLEncoder::process(Data data) {
	Tools::Profiler p("Processing PCC frame");
	auto out = output->getBuffer(0);

	{
#ifndef FIXME_USE_FAKE_PCC
		Tools::Profiler p("  Encoding time only");
		cwi_encode cwi_encode_object;
		std::stringstream comp_frame;
		cwi_encode_object.cwi_encoder(params, (void*)data->data(), comp_frame);
		auto const resData = comp_frame.str();
		auto const resDataSize = resData.size();
		out->resize(resDataSize);
		memcpy(out->data(), resData.c_str(), resDataSize);
#else /*FIXME_USE_FAKE_PCC*/
		out->resize(100);
#endif /*FIXME_USE_FAKE_PCC*/
	}

	out->setMediaTime(data->getMediaTime());

	//capture frame rate: int fps
	//timestamp: long netTimestamp, long captureTimestamp
	//Geometry resolution (Number of points): long int ptCount
	//Point size: float ptSize
	//out->setMetadata();

	output->emit(out);
}