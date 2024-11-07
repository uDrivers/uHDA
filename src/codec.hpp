#pragma once

#include "uhda/types.h"
#include "widget.hpp"
#include "vector.hpp"

struct UhdaController;

struct UhdaCodec;

struct UhdaPath {
	UhdaCodec* codec;
	uhda::vector<UhdaWidget*> widgets;
	uint8_t gain;
};

struct UhdaOutput {
	UhdaWidget* widget;
	uint8_t sequence;
};

struct UhdaOutputGroup {
	explicit UhdaOutputGroup(uint8_t assoc) : assoc {assoc} {}

	~UhdaOutputGroup() {
		for (auto output : outputs) {
			output->~UhdaOutput();
			uhda_kernel_free(output, sizeof(UhdaOutput));
		}
	}

	uhda::vector<UhdaOutput*> outputs;
	uint8_t assoc;
};

struct UhdaCodec {
	UhdaCodec(UhdaController* controller, uint8_t cid) : controller {controller}, cid {cid} {}

	~UhdaCodec() {
		for (auto group : output_groups) {
			group->~UhdaOutputGroup();
			uhda_kernel_free(group, sizeof(UhdaOutputGroup));
		}
	}

	UhdaStatus init();
	UhdaStatus find_output_paths();

	UhdaStatus get_parameter(uint8_t nid, uint8_t param, uint32_t& res) const;
	UhdaStatus get_connection_list(uint8_t nid, uint8_t offset_index, uint32_t& res) const;
	UhdaStatus get_pin_sense(uint8_t nid, uint32_t& res) const;
	UhdaStatus get_config_default(uint8_t nid, uint32_t& res) const;

	[[nodiscard]] UhdaStatus set_selected_connection(uint8_t nid, uint8_t index) const;
	[[nodiscard]] UhdaStatus set_amp_gain_mute(uint8_t nid, uint16_t data) const;
	[[nodiscard]] UhdaStatus set_converter_format(uint8_t nid, uint16_t format) const;
	[[nodiscard]] UhdaStatus set_converter_control(uint8_t nid, uint8_t stream, uint8_t channel) const;
	[[nodiscard]] UhdaStatus set_pin_control(uint8_t nid, uint8_t data) const;
	[[nodiscard]] UhdaStatus set_pin_sense(uint8_t nid, uint8_t data) const;
	[[nodiscard]] UhdaStatus set_eapd_enable(uint8_t nid, uint8_t data) const;
	[[nodiscard]] UhdaStatus set_converter_channel_count(uint8_t nid, uint8_t count) const;
	[[nodiscard]] UhdaStatus set_power_state(uint8_t nid, uint8_t data) const;

	UhdaController* controller;
	uhda::vector<UhdaWidget> widgets;
	uhda::vector<uint8_t> dac_nids;
	uhda::vector<uint8_t> output_nids;
	uhda::vector<UhdaPath> output_paths;
	uhda::vector<UhdaOutputGroup*> output_groups;
	uint8_t cid;
};
