#pragma once
#include "vector.hpp"

struct UhdaCodec;

struct UhdaWidget {
	UhdaCodec* codec;
	uhda::vector<uint8_t> connections;
	uint32_t in_amp_caps;
	uint32_t out_amp_caps;
	uint32_t pin_caps;
	uint32_t default_config;
	uint8_t nid;
	uint8_t type;
	uint8_t default_dev;
};
