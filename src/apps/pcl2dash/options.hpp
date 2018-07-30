#pragma once

#include "lib_appcommon/options.hpp"
#include "lib_pipeline/pipeline.hpp"
#include <string>

struct Config : IConfig {
	std::string inputPath;
	int numFrames = std::numeric_limits<int>::max();
	int threading = 2;
};
