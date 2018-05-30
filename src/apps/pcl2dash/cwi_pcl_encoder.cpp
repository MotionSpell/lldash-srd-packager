#include "cwi_pcl_encoder.hpp"

using namespace Modules;

CWI_PCLEncoder::CWI_PCLEncoder() {
	output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
}

CWI_PCLEncoder::~CWI_PCLEncoder() {
}

void CWI_PCLEncoder::process(Data data) {
	auto out = output->getBuffer(0);
	//process(out);
	//out->setMediaTime();
	//out->setMetadata();
	output->emit(out);
}