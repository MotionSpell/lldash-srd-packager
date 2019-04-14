#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include "cwipc_realsense2/api.h"
#include <string>

#include <experimental/filesystem>
using namespace std::experimental::filesystem::v1;

#define USE_FAKE_CLOCK

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

		char *errorMessage = nullptr;
		if (path.empty()) {
			source = cwipc_realsense2(&errorMessage);
			if (!source) {
				log(Warning, "cwipc_realsense2() error: \"%s\". Using synthetic capture source.", errorMessage ? errorMessage : "");
				source = cwipc_synthetic();
				if (!source) {
					cwipc_source_free(source);
					throw error(format("cwipc_synthetic() error: returned NULL. Aborting."));
				}
			}
		} else if (!resolvePaths(path).empty()) {
			log(Warning, "Using file based capture with pattern from \"%s\".", path);
		} else
			throw error(format("Error: file argument (\"%s\") couldn't be resolved. Aborting.", path));
	}

	~MultifileReader() {
		cwipc_source_free(source);
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
				if (!frame) {
					log(Warning, "cwipc_read() error.");
				}
				if (initTimeIn180k == -1) {
#ifndef USE_FAKE_CLOCK
					initTimeIn180k = timescaleToClock(cwipc_timestamp(frame), 1000000);
				}
				timeIn180k = timescaleToClock(cwipc_timestamp(frame), 1000000) - initTimeIn180k;
#else
					initTimeIn180k = fractionToClock(g_SystemClock->now());
				}
				timeIn180k = fractionToClock(g_SystemClock->now()) - initTimeIn180k;
#endif
			} else {
				if (initTimeIn180k == -1) initTimeIn180k = fractionToClock(g_SystemClock->now());
				timeIn180k = fractionToClock(g_SystemClock->now()) - initTimeIn180k;
				char *errorMessage = nullptr;
				frame = cwipc_read(paths[numFrame % paths.size()].c_str(), timeIn180k, &errorMessage);
				if (!frame) {
					log(Warning, "cwipc_read() error: \"%s\".", errorMessage ? errorMessage : "");
				}
			}

			auto out = output->getBuffer(sizeof(decltype(&frame)));
			memcpy(out->data(), &frame, sizeof(decltype(&frame)));
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
