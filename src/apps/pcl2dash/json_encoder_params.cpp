#include "rapidjson/error/en.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include "lib_utils/log.hpp"
#include "lib_utils/tools.hpp"
#include "lib_utils/format.hpp"
#include "cwipc_codec/api.h" //MSVC: Warning 4275
#include "options.hpp"
#include <iostream>

using namespace rapidjson;

cwipc_encoder_params parseParamFile(std::string param_file) {
	FILE *f = fopen(param_file.c_str(), "rb");
	if (!f)
		throw std::runtime_error(format("Error: could not open JSON file \"%s\".", param_file));
	fseek(f, 0, SEEK_END);
	auto const fsize = ftell(f);
	if (!fsize)
		throw std::runtime_error(format("Error: JSON file \"%s\" ha an invalid size 0.", param_file));
	fseek(f, 0, SEEK_SET);

	auto readBuffer = std::make_unique<char[]>(fsize);
	FileReadStream is(f, readBuffer.get(), fsize);

	Document d;
	d.ParseStream(is);
	if (d.HasParseError()) {
		std::cerr << "Schema file '" << param_file << "' is not a valid JSON" << std::endl;
		std::cerr << "Error(offset " << static_cast<unsigned>(d.GetErrorOffset()) << "): " << GetParseError_En(d.GetParseError()) << "%s" << std::endl;
		fclose(f);
		throw std::runtime_error(format("Invalid JSON file: %s", param_file));
	}
	fclose(f);

	CWIPC_ENCODER_PARAMS;
	try {
		for (auto const &dd : d.GetObject()) {
			if (dd.name.GetString() == std::string("num_threads")) {
				pclEncoderParams.num_threads = d["num_threads"].GetInt();
			} else if (dd.name.GetString() == std::string("do_inter_frame")) {
				pclEncoderParams.do_inter_frame = d["do_inter_frame"].GetBool();
			} else if (dd.name.GetString() == std::string("gop_size")) {
				pclEncoderParams.gop_size = d["gop_size"].GetInt();
			} else if (dd.name.GetString() == std::string("exp_factor")) {
				pclEncoderParams.exp_factor = d["exp_factor"].GetDouble();
			} else if (dd.name.GetString() == std::string("octree_bits")) {
				pclEncoderParams.octree_bits = d["octree_bits"].GetInt();
			} else if (dd.name.GetString() == std::string("color_bits")) {
				pclEncoderParams.color_bits = d["color_bits"].GetInt();
			} else if (dd.name.GetString() == std::string("jpeg_quality")) {
				pclEncoderParams.jpeg_quality = d["jpeg_quality"].GetInt();
			} else if (dd.name.GetString() == std::string("macroblock_size")) {
				pclEncoderParams.macroblock_size = d["macroblock_size"].GetInt();
			} else {
				std::cerr << "Unknown JSON: " << dd.name.GetString() << std::endl;
			}
		}
	} catch (std::runtime_error const& e) {
		std::cerr << "Catched error: " << e.what() << std::endl;
		exit(1);
	}

	return pclEncoderParams;
}
