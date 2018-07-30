#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include <cwi_encode/cwi_encode.h>
#include <string>

class MultifileReader : public Modules::ActiveModule {
public:
	MultifileReader(const std::string &path)
	: path(path) {
		output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
	}

	~MultifileReader() {}

	bool work() override {
		Tools::Profiler profiler("Processing PCC frame");

		//TODO: void getPointCloud(long *netTimestamp, long *captureTimestamp, void *frame);

		void *res = nullptr;
		load_ply_file_XYZRGB(path, &res);
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
};
