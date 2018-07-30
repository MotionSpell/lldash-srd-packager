#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include <cwi_encode/cwi_encode.h>
#include <string>
#include <windows.h>

class MultifileReader : public Modules::ActiveModule {
public:
	MultifileReader(const std::string &path)
	: path(path) {
		output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
		
		auto const DLLName = "multiFrame.dll";
		hInstLibrary = LoadLibraryA(DLLName);
		if (hInstLibrary) {
			getPointCloud = (GetPointCloudFunction)GetProcAddress(hInstLibrary, "getPointCloud");
			log(Warning, "Using capture from %s", DLLName);
		} else if (!path.empty()) {
			log(Warning, "Using file based capture with pattern from %s", path);
		} else
			throw error(format("ERROR: no DLL '%s' found and no file argument. Check usage.", DLLName));
	}

	~MultifileReader() {
		if (hInstLibrary) {
			FreeLibrary(hInstLibrary);
		}
	}

	bool work() override {
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
	}

private:
	std::string path;
	Modules::OutputDefault *output;
	HINSTANCE hInstLibrary = { 0 };
	//void getPointCloud(long *netTimestamp, long *captureTimestamp, void *frame);
	typedef void(*GetPointCloudFunction)(long *, void **);
	GetPointCloudFunction getPointCloud = nullptr;
};
