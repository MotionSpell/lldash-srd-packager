#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include <cwi_encode/cwi_encode.h>
#include <string>

//#define DISABLE_PLY_LOADER

class MultifileReader : public Modules::ActiveModule {
public:
	MultifileReader(const std::string &path)
	: path(path) {
		output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
	}

	~MultifileReader() {}

	bool work() override {
		//TODO: void getPointCloud(long *netTimestamp, long *captureTimestamp, void *frame);

		Tools::Profiler profiler("Processing PCC frame");
#ifdef DISABLE_PLY_LOADER
		auto p = format("%s", path);
		auto file = fopen(p.c_str(), "rb");
		if (!file) {
			log(Warning, "Can't open file \"%s\" for reading. Exiting.", p);
			return false;
		}
		fseek(file, 0, SEEK_END);
		auto const size = ftell(file);
		fseek(file, 0, SEEK_SET);

		auto out = output->getBuffer(size);
		fread(out->data(), 1, (size_t)out->size(), file);
		fclose(file);
#else
		void *res = nullptr;
		load_ply_file_XYZRGB(path, &res);
		auto out = output->getBuffer(sizeof(res));
		memcpy(out->data(), &res, sizeof(res));
#endif
		out->setMediaTime(fractionToClock(g_SystemClock->now()));
		g_SystemClock->sleep(Fraction(1, 100)); //FIXME
		output->emit(out);

		return true;
	}

private:
	std::string path;
	Modules::OutputDefault *output;
};
