#include "cwi_pcl_encoder.hpp"

using namespace Modules;

CWI_PCLEncoder::CWI_PCLEncoder(const Params &params)
: params(params) {
	output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
}

CWI_PCLEncoder::~CWI_PCLEncoder() {
}

void CWI_PCLEncoder::process(Data data) {
	auto out = output->getBuffer(0);
	//process(out);
	//out->setMediaTime();

//capture frame rate: int fps
//timestamp: long netTimestamp, long captureTimestamp
//Geometry resolution (Number of points): long int ptCount
//Point size: float ptSize
	//out->setMetadata();

	output->emit(out);
}