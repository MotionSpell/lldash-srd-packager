#include "../bin2dash/bin2dash.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <experimental/filesystem>

using namespace std::experimental::filesystem::v1;

const char *g_appName = "bin2dash_app";

namespace {
std::vector<std::string> resolvePaths(std::string path) {
	std::vector<std::string> res;
	if (!path.empty()) {
		for (const auto & entry : directory_iterator(path)) {
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
	std::string inputPath;
	int segDurInMs = 10000;
	int sleepAfterFrameInMs = 0;
	std::string publishUrl;
};

static void usage() {
	fprintf(stderr, "Usage: %s [options, see below] [file_pattern]\n", g_appName);
	Config cfg;
	fprintf(stderr, "\t-d\tdurationInMs: 0: segmentTimeline, otherwise SegmentNumber[default=%d]\n", cfg.segDurInMs);
	fprintf(stderr, "\t-s\tsleepAfterFrameInMs: sleep time in ms after each frame, used for regulation[default=%d]\n", cfg.sleepAfterFrameInMs);
	fprintf(stderr, "\t-u\tpublishURL: if empty files are written and the node-gpac-http server should be used, otherwise use the Evanescent SFU.[default=\"%s\"]\n", cfg.publishUrl.c_str());
}

Config args(int argc, char const* argv[]) {
	Config opts;

	int argIdx = 0;
	while (++argIdx < argc) {
		if (!strcmp("-d", argv[argIdx])) {
			if (++argIdx >= argc) {
				usage();
				throw std::runtime_error("Missing argument after \"-d\". Aborting.");
			}
			opts.segDurInMs = atoi(argv[argIdx]);
			fprintf(stderr, "Detected segment duration of %dms\n", opts.segDurInMs);
		} else if (!strcmp("-s", argv[argIdx])) {
			if (++argIdx >= argc) {
				usage();
				throw std::runtime_error("Missing argument after \"-s\". Aborting.");
			}
			opts.sleepAfterFrameInMs = atoi(argv[argIdx]);
			fprintf(stderr, "Detected sleep after frame of %dms\n", opts.sleepAfterFrameInMs);
		} else if (!strcmp("-u", argv[argIdx])) {
			if (++argIdx >= argc) {
				usage();
				throw std::runtime_error("Missing argument after \"-u\". Aborting.");
			}
			opts.publishUrl = argv[argIdx];
			fprintf(stderr, "Detected publish URL \"%s\"\n", opts.publishUrl.c_str());
		} else {
			opts.inputPath = argv[argIdx];
			fprintf(stderr, "Detected input path \"%s\"\n", opts.inputPath.c_str());
		}
	}

	if (opts.inputPath.empty()) {
		usage();
		throw std::runtime_error("No input path. Aborting.");
	}

	return opts;
}

int main(int argc, char const* argv[]) {
	try {
		auto config = args(argc, argv);
		auto handle = vrt_create("vrtogether", VRT_4CC('c','w','i','1'), config.publishUrl.c_str(), config.segDurInMs);

		auto paths = resolvePaths(config.inputPath);
		if (paths.empty())
			throw std::runtime_error(std::string("No file found for path \"") + config.inputPath + "\")");

		int64_t i = 0;
		while (1) {
			auto buf = loadFile(paths[i % paths.size()]);
			vrt_push_buffer(handle, buf.data(), buf.size());

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
