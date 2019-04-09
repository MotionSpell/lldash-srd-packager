#include "file_reader.hpp"
#include "../bin2dash/bin2dash.hpp"
#include "lib_appcommon/options.hpp"
#include "lib_utils/format.hpp"
#include "lib_utils/tools.hpp"
#include "lib_utils/system_clock.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

const char *g_appName = "bin2dash_app";

struct Config {
	std::string inputPath;
	int segDurInMs = 10000;
	int sleepAfterFrameInMs = 0;
};

std::unique_ptr<const Config> args(int argc, char const* argv[]) {
	CmdLineOptions opt;
	auto opts = uptr(new Config);

	opt.add("d", "durationInMs"       , &opts->segDurInMs         , format("0: segmentTimeline, otherwise SegmentNumber [default=%s]", opts->segDurInMs));
	opt.add("s", "sleepAfterFrameInMs", &opts->sleepAfterFrameInMs, format("Sleep time in ms after each frame, used for regulation [default=%s]", opts->sleepAfterFrameInMs));

	auto files = opt.parse(argc, argv);
	if (files.size() != 1) {
		fprintf(stderr, "Usage: %s [options, see below] [file_pattern]\n", g_appName);
		opt.printHelp(std::cerr);
		throw std::runtime_error("Invalid command line");
	}
	opts->inputPath = files[0];

	return std::move(opts);
}

int main(int argc, char const* argv[]) {
	try {
		auto config = args(argc, argv);
		auto handle = vrt_create("vrtogether", VRT_4CC('c','w','i','1'), config->segDurInMs);

		auto paths = resolvePaths(config->inputPath);
		if (paths.empty())
			throw std::runtime_error(std::string("No file found for path \"") + config->inputPath + "\")");

		std::vector<uint8_t> buf;
		int64_t i = 0;
		while (1) {
			fillVector(paths[i % paths.size()], buf);

			vrt_push_buffer(handle, buf.data(), buf.size());

			if (config->sleepAfterFrameInMs)
				g_SystemClock->sleep(Fraction(config->sleepAfterFrameInMs, 1000));

			i++;
		}

		vrt_destroy(handle);
	} catch (std::exception const& e) {
		std::cerr << "[" << g_appName << "] " << "Error: " << e.what() << std::endl;
		return 1;
	}
}
