#pragma once

#include "lib_appcommon/options.hpp"
#include "lib_pipeline/pipeline.hpp"
#include <string>

#define CWIPC_ENCODER_PARAMS cwipc_encoder_params pclEncoderParams { 1, false, 1, 0.0, 7, 8, 85, 16 }

struct Config : IConfig {
	std::string inputPath;
	int numFrames = std::numeric_limits<int>::max();
	int threading = 2;
	int segDurInMs = 1;
	int delayInSeg = 0;
	std::string param_file;
	std::string publish_url;
};
