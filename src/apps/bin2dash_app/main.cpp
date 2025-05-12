#include "../bin2dash/bin2dash.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

const char *g_appName = "bin2dash_app";

namespace {
std::vector<std::string> resolvePaths(std::string path) {
	std::vector<std::string> res;
	if (!path.empty()) {
		for (const auto & entry : std::filesystem::directory_iterator(path)) {
			res.push_back(entry.path().string());
		}
	}
	return res;
}

std::vector<uint8_t> loadFile(std::string filename) {
	auto file = fopen(filename.c_str(), "rb");
	if (!file)
		throw std::runtime_error(std::string("Can't open file for reading: \"") + filename + "\"");
	fseek(file, 0, SEEK_END);
	auto const size = ftell(file);
	fseek(file, 0, SEEK_SET);
	std::vector<uint8_t> buf(size);
	int read = (int)fread(buf.data(), 1, size, file);
	fclose(file);
	if (read != size)
		throw std::runtime_error(std::string("Can't read enough data for file \"") + filename + "\"");
	return buf;
}
}

struct Config {
	bool help = false;
	std::string inputPath;
	int segDurInMs = 2000;
	int sleepAfterFrameInMs = 200;
	std::string publishUrl = ".";
};

static void usage() {
	fprintf(stderr, "Usage: %s [options, see below] [file_pattern]\n", g_appName);
	Config cfg;
	fprintf(stderr, "\t-d\tdurationInMs: 0=segmentTimeline, otherwise SegmentNumber (default: %d)\n", cfg.segDurInMs);
	fprintf(stderr, "\t-s\tsleepAfterFrameInMs: sleep time in ms after each frame, used for regulation (default: %d)\n", cfg.sleepAfterFrameInMs);
	fprintf(stderr, "\t-u\tpublishURL: if empty files are written and the node-gpac-http server should be used, otherwise use the Evanescent SFU. (default=\"%s\")\n", cfg.publishUrl.c_str());
}

Config parseCommandLine(int argc, char const* argv[]) {
	Config opts;

	int argIdx = 0;

	auto pop = [&]() {
		if (argIdx >= argc)
			throw std::runtime_error("Incomplete command line");
		return std::string(argv[argIdx++]);
	};

	while (argIdx < argc) {
		auto const word = pop();

		if (word == "-h" || word == "--help"){
			opts.help = true;
			usage();
			return opts;
		}

		if (word == "-d")
			opts.segDurInMs = atoi(pop().c_str());
		else if (word == "-s")
			opts.sleepAfterFrameInMs = atoi(pop().c_str());
		else if (word == "-u")
			opts.publishUrl = pop();
		else
			opts.inputPath = word;
	}

	if (opts.inputPath.empty()) {
		throw std::runtime_error("No input path. Aborting.");
	}

	return opts;
}

int main(int argc, char const* argv[]) {
	try {
		auto config = parseCommandLine(argc, argv);
		if(config.help)
			return 0;

		auto const numQuality = 2;
		auto const numTiles = 3;
		auto const numStreams = numQuality * numTiles;
		printf("%d stream(s)\n", numStreams);
		StreamDesc desc[numStreams] = {};
		for (int q=0; q<numQuality; ++q) {
			for (int t=0; t<numTiles; ++t) {
				auto &d = desc[q*numTiles+t];
				d.MP4_4CC = VRT_4CC('c','w','i','1');
				d.objectX = q;
				d.objectY = t;
				d.objectWidth = 1;
				d.objectHeight = 1;
				d.totalWidth = numTiles;
				d.totalHeight = numTiles;

				auto fourCC = (char*)&d.MP4_4CC;
				printf("\t stream %d: 4CC=%c%c%c%c, SRD { %u, %u, %u, %u, %u, %u }\n", q*numTiles+t, fourCC[0], fourCC[1], fourCC[2], fourCC[3],
					d.objectX, d.objectY, d.objectWidth, d.objectHeight, d.totalWidth, d.totalHeight);
			}
		}

		// add trailing separator if not present
		auto publishUrl = config.publishUrl;
		if (isalnum(config.publishUrl.back()))
			publishUrl += "/";

		auto handle = vrt_create_ext("vrtogether", numStreams, desc, publishUrl.c_str(), config.segDurInMs);

		auto paths = resolvePaths(config.inputPath);
		if (paths.empty())
			throw std::runtime_error(std::string("No file found for path \"") + config.inputPath + "\")");

		int64_t i = 0;
		while (1) {
			auto buf = loadFile(paths[i % paths.size()]);
			for (int j=0; j<numStreams; ++j)
				vrt_push_buffer_ext(handle, j, buf.data(), buf.size());

			if (config.sleepAfterFrameInMs)
				std::this_thread::sleep_for(std::chrono::milliseconds(config.sleepAfterFrameInMs));

			i++;
		}

		vrt_destroy(handle);
	} catch (std::exception const& e) {
		fprintf(stderr, "[%s] Error: %s\n", g_appName, e.what());
		return 1;
	}
}
