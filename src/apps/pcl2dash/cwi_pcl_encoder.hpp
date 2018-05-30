#pragma once

#include "lib_modules/modules.hpp"

class CWI_PCLEncoder : public Modules::ModuleS {
public:
	CWI_PCLEncoder();
	~CWI_PCLEncoder();
	void process(Modules::Data) override;

private:
	Modules::OutputDefault *output;
};