#pragma once

#include "lib_modules/modules.hpp"
#include "lib_utils/profiler.hpp"
#include "lib_utils/system_clock.hpp"
#include <cwi_encode/cwi_encode.h>
#include <string>

#ifdef _MSC_VER

#include <windows.h>
#include <filesystem>
using namespace std::experimental::filesystem::v1;

namespace {
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
char* GetThisPath(char* dest, size_t destSize) {
	if (!dest) return NULL;
	if (MAX_PATH > destSize) return NULL;
	DWORD length = GetModuleFileNameA(NULL, dest, (DWORD)destSize);
	PathRemoveFileSpecA(dest);
	return dest;
}

#else /*_MSC_VER*/

#include <experimental/filesystem>
using namespace std::experimental::filesystem::v1;

namespace {

#endif /*_MSC_VER*/

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

#if _MSC_VER
		char dest[MAX_PATH];
		GetThisPath(dest, MAX_PATH);
		auto const DLLName = "\\multiFrame.dll";
		strncat(dest, DLLName, MAX_PATH);
		hInstLibrary = LoadLibraryA(dest);
		if (hInstLibrary) {
			getPointCloud = (GetPointCloudFunction)GetProcAddress(hInstLibrary, "getPointCloud");
			log(Warning, "Using capture from %s", dest);
		} else if (!resolvePaths(path).empty()) {
			log(Warning, "Using file based capture with pattern from %s", path);
		} else
			throw error(format("ERROR: no DLL '%s' found and no file argument (\'%s\'). Check usage.", dest, path));
#else
		if (!resolvePaths(path).empty()) {
                        log(Warning, "Using file based capture with pattern from %s", path);
                } else
                        throw error(format("ERROR: no file argument (\'%s\'). Check usage.", path));
#endif /*_MSC_VER*/
	}

	~MultifileReader() {
#ifdef _MSC_VER
		if (hInstLibrary) {
			FreeLibrary(hInstLibrary);
		}
#endif
	}

	bool work() override {
		std::vector<std::string> paths = resolvePaths(path);

		auto const cond = [this]() {
			return ((numFrame++ < numFrameMax) || numFrameMax == -1);
		};

		if (cond()) {
			Tools::Profiler profiler("Processing PCC frame");

			void *res = nullptr;
			uint64_t t = 4;
#ifdef _MSC_VER
			if (hInstLibrary) {
				res = &t;
				getPointCloud(&t, &res);
				log(Debug, "got a pointcloud captured at = %s", t);
			} else
#endif
			{
				load_ply_file_XYZRGB(paths[numFrame % paths.size()], &res);
			}

			auto out = output->getBuffer(sizeof(decltype(t)) + sizeof(res));
			memcpy(out->data(), &t, sizeof(decltype(t)));
			memcpy(out->data() + sizeof(decltype(t)), &res, sizeof(res));
			out->setMediaTime(fractionToClock(g_SystemClock->now()) - initTimeIn180k);
			g_SystemClock->sleep(Fraction(1, 100)); //FIXME
			output->emit(out);

			return true;
		} else {
			return false;
		}
	}

private:
	std::string path;
	Modules::OutputDefault *output;
#if _MSC_VER
	HINSTANCE hInstLibrary = { };
#endif
	//void getPointCloud(long *netTimestamp, long *captureTimestamp, void *frame);
	typedef void(*GetPointCloudFunction)(uint64_t *, void **);
	GetPointCloudFunction getPointCloud = nullptr;
	int numFrame = 0, numFrameMax;
	uint64_t initTimeIn180k = fractionToClock(g_SystemClock->now());
};
