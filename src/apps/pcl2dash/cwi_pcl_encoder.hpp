#pragma once

#include "lib_modules/utils/helper.hpp"
#include <cwi_encode/cwi_encode.h>

namespace Modules {
class DataAVPacket;
}

class CWI_PCLEncoder : public Modules::ModuleS {
public:
	CWI_PCLEncoder(const encoder_params &params);
	~CWI_PCLEncoder();
	void process(Modules::Data) override;

private:
	Modules::OutputDataDefault<Modules::DataAVPacket>* output;
	encoder_params params;
};