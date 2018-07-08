//copied from https://raw.githubusercontent.com/shirsub/cwi-codec-lib/master/apps/cwi_encode/include/pcl/apps/cwi_encode/cwi_encode.h
//TODO: include repo as submodule when it builds (and put Signals at the same level)

#include <sstream>

__declspec(dllimport) int __cdecl cwi_encoder(void *param, void* pc, void *comp_frame);
__declspec(dllimport) int __cdecl cwi_decoder(void *param, void* pc, void *comp_frame);
__declspec(dllimport) int __cdecl cwi_test(void *p);