#include "lib_appcommon/options.hpp"
#include "lib_utils/format.hpp"
#include "lib_utils/tools.hpp"
#include "options.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

const char *g_appName = "pcl2dash";

extern int safeMain(int argc, char const* argv[]);

static void usage() {
	std::cerr << "Usage: " << g_appName << " [PCL_files_pattern]" << std::endl;
	std::cerr << "Examples:" << std::endl;
	std::cerr << "\t" << g_appName << std::endl;
	std::cerr << "\t" << g_appName << " folder/file%04d.ply" << std::endl;
}

std::unique_ptr<const IConfig> processArgs(int argc, char const* argv[]) {
	if (argc < 1 || argc > 2) {
		usage();
		throw std::runtime_error(format("[%s] Expected 0 or 1 argument, got %s", g_appName, argc - 1));
	}
	auto opts = uptr(new Config);
	if (argc >= 2)
		opts->inputPath = argv[1];
	return std::move(opts);
}

int main(int argc, char const* argv[]) {
	try {
		return safeMain(argc, argv);
	} catch (std::exception const& e) {
		std::cerr << "[" << g_appName << "] " << "Error: " << e.what() << std::endl;
		return 1;
	}
}
