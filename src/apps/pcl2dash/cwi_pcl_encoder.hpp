#pragma once

#include "lib_modules/modules.hpp"

class CWI_PCLEncoder : public Modules::ModuleS {
public:
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

	CWI_PCLEncoder(const Params &params);
	~CWI_PCLEncoder();
	void process(Modules::Data) override;

private:
	Modules::OutputDefault *output;
	Params params;
};