#pragma once
#include "vector.hpp"

struct UhdaCodec;

struct UhdaWidget {
	UhdaCodec* codec;
	uhda::vector<uint8_t> connections;
	uhda::vector<uint32_t> supported_sample_rates;
	uhda::vector<UhdaFormat> supported_formats;
	uint32_t in_amp_caps;
	uint32_t out_amp_caps;
	uint32_t pin_caps;
	uint32_t default_config;
	uint32_t supported_rates;
	uint8_t nid;
	uint8_t type;
	uint8_t default_dev;
	bool trigger : 1;
	bool presence_detect : 1;
};
