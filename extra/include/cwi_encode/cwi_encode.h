#pragma once

//copied from https://raw.githubusercontent.com/shirsub/cwi-codec-lib/master/apps/cwi_encode/include/pcl/apps/cwi_encode/cwi_encode.h
//TODO: include repo as submodule when it builds (and put Signals at the same level)

#include <sstream>

struct encoder_params
{
	int num_threads = 1;
	bool do_inter_frame = false;
	int gop_size = 1;
	double exp_factor = 0.0;
	int octree_bits = 7;
	int color_bits = 8;
	int jpeg_quality = 85;
	int macroblock_size = 16;
};

#ifdef _MSC_VER
#define CWI_IMPORT __declspec(dllimport)
#else
#define CWI_IMPORT
#endif

class CWI_IMPORT cwi_encode
{
public:
	int cwi_encoder(encoder_params param, void* pc, std::stringstream& comp_frame, uint64_t timestamp);
	int cwi_decoder(encoder_params param, void* pc, std::stringstream& comp_frame);
};

CWI_IMPORT int load_ply_file_XYZRGB(std::string path, void **pc);
CWI_IMPORT void delete_ply_data(void *pc);

