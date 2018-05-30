#pragma once

#include "lib_modules/modules.hpp"
#include <string>

class MultifileReader : public Modules::ModuleS {
public:
	MultifileReader(const std::string &path)
	: path(path) {
		output = addOutput<Modules::OutputDataDefault<Modules::DataRaw>>();
	}

	~MultifileReader() {}

	void process(Modules::Data) override {
		auto p = format("%s", path);
		auto file = fopen(p.c_str(), "rb");
		if (!file)
			throw error(format("Can't open file for reading: %s", p));
		fseek(file, 0, SEEK_END);
		auto const size = ftell(file);
		fseek(file, 0, SEEK_SET);
		auto out = output->getBuffer(size);
		fwrite(out->data(), 1, (size_t)out->size(), file);
		fclose(file);
		//out->setMediaTime();
		output->emit(out);
	}

private:
	std::string path;
	Modules::OutputDefault *output;
};
