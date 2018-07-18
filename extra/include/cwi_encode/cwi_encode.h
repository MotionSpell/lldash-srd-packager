#pragma once

//copied from https://raw.githubusercontent.com/shirsub/cwi-codec-lib/master/apps/cwi_encode/include/pcl/apps/cwi_encode/cwi_encode.h
//TODO: include repo as submodule when it builds (and put Signals at the same level)

#include <sstream>

struct encoder_params
{
	int num_threads = 1;
	bool do_inter_frame = 0;
	int gop_size = 1;
	double exp_factor = 0.2;
	int octree_bits = 11;
	int color_bits = 8;
	int jpeg_quality = 0;
	int macroblock_size = 16;
};

class __declspec(dllimport) cwi_encode
{
public:
	int cwi_encoder(encoder_params param, void* pc, std::stringstream& comp_frame);
	int cwi_decoder(encoder_params param, void* pc, std::stringstream& comp_frame);
};

__declspec(dllimport) int load_ply_file_XYZRGB(std::string path, void **pc);
__declspec(dllimport) void delete_ply_data(void *pc);
