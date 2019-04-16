#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <experimental/filesystem>

using namespace std::experimental::filesystem::v1;

std::vector<std::string> resolvePaths(std::string path) {
	std::vector<std::string> res;
	if (!path.empty()) {
		for (const auto & entry : directory_iterator(path)) {
			res.push_back(entry.path().string());
		}
	}
	return res;
}

void fillVector(std::string fn, std::vector<uint8_t> &buf) {
	auto file = fopen(fn.c_str(), "rb");
	if (!file)
		throw std::runtime_error(std::string("Can't open file for reading: \"") + fn + "\"");
	fseek(file, 0, SEEK_END);
	auto const size = ftell(file);
	fseek(file, 0, SEEK_SET);
	buf.resize(size);
	int read = (int)fread(buf.data(), 1, size, file);
	if (read != size)
		throw std::runtime_error(std::string("Can't read enoug data for file \"") + fn + "\" (read=" + +"bytes)");
	fclose(file);
}
