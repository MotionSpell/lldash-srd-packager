#pragma once

#include "lib_modules/utils/helper.hpp"
#include "cwipc_codec/api.h" //MSVC: Warning 4275

namespace Modules {
class DataAVPacket;
}

//struct cwipc_encoder_params;
//class cwipc_encoder;

class CWI_PCLEncoder : public Modules::ModuleS {
public:
	CWI_PCLEncoder(cwipc_encoder_params params);
	~CWI_PCLEncoder();
	void process(Modules::Data) override;

private:
	Modules::OutputDataDefault<Modules::DataAVPacket>* output;
	cwipc_encoder *encoder;
	cwipc_encoder_params params;
};