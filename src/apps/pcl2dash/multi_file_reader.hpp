#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include <cwi_encode/cwi_encode.h>
#include <string>
#include <windows.h>

namespace {
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
char* GetThisPath(char* dest, size_t destSize) {
	if (!dest) return NULL;
	if (MAX_PATH > destSize) return NULL;
	DWORD length = GetModuleFileNameA(NULL, dest, destSize);
	PathRemoveFileSpecA(dest);
	return dest;
}
}

class MultifileReader : public Modules::ActiveModule {
public:
	MultifileReader(const std::string &path, const int numFrameMax)
	: path(path), numFrameMax(numFrameMax) {
		output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();

		char dest[MAX_PATH];
		GetThisPath(dest, MAX_PATH);
		auto const DLLName = "\\multiFrame.dll";
		strncat(dest, DLLName, MAX_PATH);
		hInstLibrary = LoadLibraryA(dest);
		if (hInstLibrary) {
			getPointCloud = (GetPointCloudFunction)GetProcAddress(hInstLibrary, "getPointCloud");
			log(Warning, "Using capture from %s", dest);
		} else if (!path.empty()) {
			log(Warning, "Using file based capture with pattern from %s", path);
		} else
			throw error(format("ERROR: no DLL '%s' found and no file argument. Check usage.", dest));
	}

	~MultifileReader() {
		if (hInstLibrary) {
			FreeLibrary(hInstLibrary);
		}
	}

	bool work() override {
		if (numFrame++ < numFrameMax) {
			Tools::Profiler profiler("Processing PCC frame");

			void *res = nullptr;
			if (hInstLibrary) {
				long t = 4;
				res = &t;
				getPointCloud(&t, &res);
				log(Debug, "got a pointcloud captured at = %s", t);
			} else {
				load_ply_file_XYZRGB(path, &res);
			}

			auto out = output->getBuffer(sizeof(res));
			memcpy(out->data(), &res, sizeof(res));
			out->setMediaTime(fractionToClock(g_SystemClock->now()));
			g_SystemClock->sleep(Fraction(1, 100)); //FIXME
			output->emit(out);

			return true;
		} else {
			return false;
		}
	}

private:
	std::string path;
	Modules::OutputDefault *output;
	HINSTANCE hInstLibrary = { 0 };
	//void getPointCloud(long *netTimestamp, long *captureTimestamp, void *frame);
	typedef void(*GetPointCloudFunction)(long *, void **);
	GetPointCloudFunction getPointCloud = nullptr;
	int numFrame = 0, numFrameMax;
};
