#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include "cwipc_realsense2/api.h"
#include <string>

#include <experimental/filesystem>
using namespace std::experimental::filesystem::v1;

namespace {

std::vector<std::string> resolvePaths(std::string path) {
	std::vector<std::string> res;
	for (const auto & entry : directory_iterator(path)) {
		res.push_back(entry.path().string());
	}
	return res;
}
}

class MultifileReader : public Modules::ActiveModule {
public:
	MultifileReader(const std::string &path, const int numFrameMax)
	: path(path), numFrameMax(numFrameMax) {
		output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();

		auto source = cwipc_realsense2(nullptr);
		if (!source) {
			source = cwipc_synthetic();
			log(Warning, "Using synthetic capture source");
		} else if (!resolvePaths(path).empty()) {
			log(Warning, "Using file based capture with pattern from %s", path);
		} else
			throw error(format("ERROR: no realsense2 found and no file argument (\'%s\'). Check usage.", path));
	}

	~MultifileReader() {
	}

	bool work() override {
		std::vector<std::string> paths = resolvePaths(path);

		auto const cond = [this]() {
			return ((numFrame++ < numFrameMax) || numFrameMax == -1);
		};

		if (cond()) {
			Tools::Profiler profiler("Processing PCC frame");

			cwipc *frame = nullptr;
			int64_t timeIn180k;
			if (source) {
				frame = cwipc_source_get(source);
				if (initTimeIn180k == -1) initTimeIn180k = timescaleToClock(cwipc_timestamp(frame), 1000000);
				timeIn180k = timescaleToClock(cwipc_timestamp(frame), 1000000) - initTimeIn180k;
			} else {
				if (initTimeIn180k == -1) initTimeIn180k = fractionToClock(g_SystemClock->now());
				timeIn180k = fractionToClock(g_SystemClock->now()) - initTimeIn180k;
				frame = cwipc_read(paths[numFrame % paths.size()].c_str(), timeIn180k, nullptr);
			}

			auto out = output->getBuffer(sizeof(decltype(frame)));
			memcpy(out->data(), &frame, sizeof(decltype(frame)));
			out->setMediaTime(timeIn180k);
			g_SystemClock->sleep(Fraction(1, 100));
			output->emit(out);

			return true;
		} else {
			return false;
		}
	}

private:
	std::string path;
	Modules::OutputDefault *output;
	int numFrame = 0, numFrameMax;
	int64_t initTimeIn180k = -1;
	cwipc_source *source = nullptr;
};
