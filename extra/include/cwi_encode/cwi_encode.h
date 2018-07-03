//copied from https://raw.githubusercontent.com/shirsub/cwi-codec-lib/master/apps/cwi_encode/include/pcl/apps/cwi_encode/cwi_encode.h
//TODO: include repo as submodule when it builds (and put Signals at the same level)

#include <sstream>

class __declspec(dllexport) cwi_encode
{
public:
	int cwi_encoder(CWI_PCLEncoder::Params param, void* pc, std::stringstream& comp_frame);
	int cwi_decoder(CWI_PCLEncoder::Params param, void* pc, std::stringstream& comp_frame);
};
