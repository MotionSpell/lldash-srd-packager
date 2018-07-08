#pragma once

#include "lib_modules/utils/helper.hpp"

namespace Modules {
class DataAVPacket;
}

//FIXME: should be elsewhere but it would need to be declared in cwi_encode.h
struct Params {
	int num_threads = 1;
	bool do_inter_frame = 0;
	int gop_size = 1;
	double exp_factor = 0.2;
	int octree_bits = 11;
	int color_bits = 8;
	int jpeg_quality = 0;
	int macroblock_size = 16;
};

class CWI_PCLEncoder : public Modules::ModuleS {
public:
	CWI_PCLEncoder(const Params &params);
	~CWI_PCLEncoder();
	void process(Modules::Data) override;

private:
	Modules::OutputDataDefault<Modules::DataAVPacket>* output;
	Params params;
};