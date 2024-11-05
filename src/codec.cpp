#include "codec.hpp"
#include "spec.hpp"
#include "controller.hpp"

using namespace uhda;

UhdaStatus UhdaCodec::init() {
	uint32_t num_func_groups_resp;
	auto status = get_parameter(0, param::NODE_COUNT, num_func_groups_resp);
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	uint8_t num_func_groups = num_func_groups_resp & 0xFF;
	uint8_t func_groups_start_nid = num_func_groups_resp >> 16 & 0xFF;

	for (uint8_t func_group_i = func_groups_start_nid;
		func_group_i < func_groups_start_nid + num_func_groups;
		 ++func_group_i) {
		uint32_t func_group_type_resp;
		status = get_parameter(func_group_i, param::FUNC_GROUP_TYPE, func_group_type_resp);
		if (status != UHDA_STATUS_SUCCESS) {
			return status;
		}

		if ((func_group_type_resp & 0xFF) != func_group_type::AUDIO) {
			continue;
		}

		status = set_power_state(func_group_i, 0);
		if (status != UHDA_STATUS_SUCCESS) {
			return status;
		}

		uint32_t num_widgets_resp;
		status = get_parameter(func_group_i, param::NODE_COUNT, num_widgets_resp);
		if (status != UHDA_STATUS_SUCCESS) {
			return status;
		}

		uint8_t num_widgets = num_widgets_resp & 0xFF;
		uint8_t widgets_start_nid = num_widgets_resp >> 16 & 0xFF;

		for (uint8_t widget_i = widgets_start_nid; widget_i < widgets_start_nid + num_widgets; ++widget_i) {
			uint32_t audio_caps;
			uint32_t in_amp_caps;
			uint32_t out_amp_caps;
			uint32_t pin_caps;
			uint32_t conn_list_len_resp;
			uint32_t default_config;

			status = get_parameter(widget_i, param::AUDIO_CAPS, audio_caps);
			if (status != UHDA_STATUS_SUCCESS) {
				return status;
			}

			status = get_parameter(widget_i, param::IN_AMP_CAPS, in_amp_caps);
			if (status != UHDA_STATUS_SUCCESS) {
				return status;
			}

			status = get_parameter(widget_i, param::OUT_AMP_CAPS, out_amp_caps);
			if (status != UHDA_STATUS_SUCCESS) {
				return status;
			}

			status = get_parameter(widget_i, param::PIN_CAPS, pin_caps);
			if (status != UHDA_STATUS_SUCCESS) {
				return status;
			}

			status = get_parameter(widget_i, param::CONN_LIST_LEN, conn_list_len_resp);
			if (status != UHDA_STATUS_SUCCESS) {
				return status;
			}

			status = get_config_default(widget_i, default_config);
			if (status != UHDA_STATUS_SUCCESS) {
				return status;
			}

			uint8_t type = audio_caps >> 20 & 0b1111;

			if (conn_list_len_resp & 1 << 7) {
				uhda_kernel_log("error: long-form connection lists are not supported");
				return UHDA_STATUS_UNSUPPORTED;
			}

			vector<uint8_t> connections;
			uint8_t conn_list_len = conn_list_len_resp & 0x7F;
			for (uint8_t i = 0; i < conn_list_len; i += 4) {
				uint32_t resp;
				status = get_connection_list(widget_i, i, resp);
				if (status != UHDA_STATUS_SUCCESS) {
					return status;
				}

				uint8_t count = conn_list_len - i;
				if (count > 4) {
					count = 4;
				}

				for (uint8_t j = 0; j < count; ++j) {
					uint8_t nid = resp >> (j * 8) & 0xFF;
					if (!connections.push(nid)) {
						return UHDA_STATUS_NO_MEMORY;
					}
				}
			}

			UhdaWidget widget {
				.codec = this,
				.connections {move(connections)},
				.in_amp_caps = in_amp_caps,
				.out_amp_caps = out_amp_caps,
				.pin_caps = pin_caps,
				.default_config = default_config,
				.nid = widget_i,
				.type = type,
				.default_dev = static_cast<uint8_t>(default_config >> 20 & 0xF)
			};
			if (widgets.size() <= widget_i) {
				if (!widgets.resize(widget_i + 1)) {
					return UHDA_STATUS_NO_MEMORY;
				}
			}
			widgets[widget_i] = move(widget);

			if (type == widget_type::AUDIO_OUT) {
				if (!dac_nids.push(widget_i)) {
					return UHDA_STATUS_NO_MEMORY;
				}
			}
			else if (type == widget_type::PIN_COMPLEX) {
				if (!output_nids.push(widget_i)) {
					return UHDA_STATUS_NO_MEMORY;
				}
			}
		}
	}

	status = find_output_paths();
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	for (auto pin_i : output_nids) {
		auto& pin = widgets[pin_i];
		// check if output capable
		if (!(pin.pin_caps & 1 << 4)) {
			continue;
		}
		uint8_t connectivity = pin.default_config >> 30;
		// no physical connection
		if (connectivity == 1) {
			continue;
		}

		uint8_t sequence = pin.default_config & 0xF;
		uint8_t assoc = pin.default_config >> 4 & 0xF;

		if (pin.default_dev == default_dev::LINE_OUT &&
			(connectivity == 0b10 || connectivity == 0b11)) {
			pin.default_dev = default_dev::SPEAKER;
		}

		if (!assoc) {
			continue;
		}

		auto new_output_ptr = uhda_kernel_malloc(sizeof(UhdaOutput));
		if (!new_output_ptr) {
			return UHDA_STATUS_NO_MEMORY;
		}
		auto* new_output = construct<UhdaOutput>(new_output_ptr, UhdaOutput {
			.widget = &pin,
			.sequence = sequence
		});

		if (assoc == 0b1111) {
			// low-priority independent output

			auto group_ptr = uhda_kernel_malloc(sizeof(UhdaOutputGroup));
			if (!group_ptr) {
				new_output->~UhdaOutput();
				uhda_kernel_free(new_output, sizeof(UhdaOutput));
				return UHDA_STATUS_NO_MEMORY;
			}

			auto* group = construct<UhdaOutputGroup>(group_ptr, assoc);
			if (!group->outputs.push(new_output)) {
				new_output->~UhdaOutput();
				uhda_kernel_free(new_output, sizeof(UhdaOutput));
				group->~UhdaOutputGroup();
				uhda_kernel_free(group_ptr, sizeof(UhdaOutputGroup));
				return UHDA_STATUS_NO_MEMORY;
			}

			if (!output_groups.push(group)) {
				group->~UhdaOutputGroup();
				uhda_kernel_free(group_ptr, sizeof(UhdaOutputGroup));
				return UHDA_STATUS_NO_MEMORY;
			}
			continue;
		}

		UhdaOutputGroup* group = nullptr;

		for (auto output_group : output_groups) {
			if (output_group->assoc == assoc) {
				group = output_group;
				break;
			}
		}

		if (group) {
			if (group->outputs.back()->sequence <= sequence) {
				if (!group->outputs.push(move(new_output))) {
					return UHDA_STATUS_NO_MEMORY;
				}
			}
			else {
				for (auto& output : group->outputs) {
					if (output->sequence > sequence) {
						if (!group->outputs.insert(&output, move(new_output))) {
							return UHDA_STATUS_NO_MEMORY;
						}
						break;
					}
				}
			}
		}
		else {
			auto group_ptr = uhda_kernel_malloc(sizeof(UhdaOutputGroup));
			if (!group_ptr) {
				new_output->~UhdaOutput();
				uhda_kernel_free(new_output, sizeof(UhdaOutput));
				return UHDA_STATUS_NO_MEMORY;
			}

			auto* new_group = construct<UhdaOutputGroup>(group_ptr, assoc);
			if (!new_group->outputs.push(new_output)) {
				new_output->~UhdaOutput();
				uhda_kernel_free(new_output, sizeof(UhdaOutput));
				new_group->~UhdaOutputGroup();
				uhda_kernel_free(group_ptr, sizeof(UhdaOutputGroup));
				return UHDA_STATUS_NO_MEMORY;
			}

			if (output_groups.is_empty() || output_groups.back()->assoc <= assoc) {
				if (!output_groups.push(new_group)) {
					new_group->~UhdaOutputGroup();
					uhda_kernel_free(group_ptr, sizeof(UhdaOutputGroup));
					return UHDA_STATUS_NO_MEMORY;
				}
			}
			else {
				for (auto& output_group : output_groups) {
					if (output_group->assoc > assoc) {
						if (!output_groups.insert(&output_group, new_group)) {
							new_group->~UhdaOutputGroup();
							uhda_kernel_free(group_ptr, sizeof(UhdaOutputGroup));
							return UHDA_STATUS_NO_MEMORY;
						}
						break;
					}
				}
			}
		}
	}

	return UHDA_STATUS_SUCCESS;
}

UhdaStatus UhdaCodec::find_output_paths() {
	struct StackEntry {
		UhdaWidget& widget;
		uint8_t con_index;
		uint8_t con_range_index;
		uint8_t con_range_end;
	};

	vector<StackEntry> stack;

	for (auto pin_i : output_nids) {
		auto& pin = widgets[pin_i];
		// check if output capable
		if (!(pin.pin_caps & 1 << 4)) {
			continue;
		}
		uint8_t connectivity = pin.default_config >> 30;
		// no physical connection
		if (connectivity == 1) {
			continue;
		}

		if (!stack.push({
			.widget = pin,
			.con_index = 0,
			.con_range_index = 0xFF,
			.con_range_end = 0
		})) {
			return UHDA_STATUS_NO_MEMORY;
		}

		while (true) {
			if (stack.is_empty()) {
				break;
			}

			auto* cur_entry = &stack.back();
			auto& cur_widget = cur_entry->widget;
			if (cur_entry->con_index == cur_widget.connections.size()) {
				stack.pop();
				continue;
			}

			if (cur_entry->con_range_index > cur_entry->con_range_end) {
				cur_entry->con_range_index = cur_widget.connections[cur_entry->con_index++];
				if (cur_entry->con_range_index & 1 << 7) {
					uhda_kernel_log(
						"warning: first connection list entry can't be a range, treating as an individual entry");
					cur_entry->con_range_index &= 0x7F;
				}

				if (cur_entry->con_index < cur_widget.connections.size() &&
					cur_widget.connections[cur_entry->con_index] & 1 << 7) {
					cur_entry->con_range_end = cur_widget.connections[cur_entry->con_index++] & 0x7F;
				}
				else {
					cur_entry->con_range_end = cur_entry->con_range_index;
				}
			}

			uint8_t nid = cur_entry->con_range_index++;
			if (nid >= widgets.size()) {
				uhda_kernel_log("warning: invalid nid in connection list");
				continue;
			}

			auto& assoc_widget = widgets[nid];
			if (assoc_widget.type == widget_type::AUDIO_OUT) {
				UhdaPath path {
					.codec = this,
					.widgets {},
					.gain = 0
				};
				for (auto& entry : stack) {
					if (!path.widgets.push(&entry.widget)) {
						return UHDA_STATUS_NO_MEMORY;
					}
				}
				if (!path.widgets.push(&assoc_widget)) {
					return UHDA_STATUS_NO_MEMORY;
				}

				if (!output_paths.push(move(path))) {
					return UHDA_STATUS_NO_MEMORY;
				}
			}
			else {
				bool circular_path = false;
				for (auto& entry : stack) {
					if (&entry.widget == &assoc_widget) {
						circular_path = true;
						break;
					}
				}

				if (circular_path || stack.size() >= 20) {
					continue;
				}
				if (!stack.push({
					.widget = assoc_widget,
					.con_index = 0,
					.con_range_index = 0xFF,
					.con_range_end = 0
				})) {
					return UHDA_STATUS_NO_MEMORY;
				}
			}
		}
	}

	return UHDA_STATUS_SUCCESS;
}

UhdaStatus UhdaCodec::get_parameter(uint8_t nid, uint8_t param, uint32_t& res) const {
	auto index = controller->submit_verb(cid, nid, cmd::GET_PARAM, param);
	ResponseDescriptor resp {};
	auto status = controller->wait_for_verb(index, resp);
	res = resp.resp;
	return status;
}

UhdaStatus UhdaCodec::get_connection_list(uint8_t nid, uint8_t offset_index, uint32_t& res) const {
	auto index = controller->submit_verb(cid, nid, cmd::GET_CONN_LIST, offset_index);
	ResponseDescriptor resp {};
	auto status = controller->wait_for_verb(index, resp);
	res = resp.resp;
	return status;
}

UhdaStatus UhdaCodec::get_config_default(uint8_t nid, uint32_t& res) const {
	auto index = controller->submit_verb(cid, nid, cmd::GET_CONFIG_DEFAULT, 0);
	ResponseDescriptor resp {};
	auto status = controller->wait_for_verb(index, resp);
	res = resp.resp;
	return status;
}

UhdaStatus UhdaCodec::set_selected_connection(uint8_t nid, uint8_t index) const {
	auto submit_index = controller->submit_verb(cid, nid, cmd::SET_CONN_SELECT, index);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_amp_gain_mute(uint8_t nid, uint16_t data) const {
	auto index = controller->submit_verb_long(cid, nid, cmd::SET_AMP_GAIN_MUTE, data);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_converter_format(uint8_t nid, uint16_t format) const {
	auto index = controller->submit_verb_long(cid, nid, cmd::SET_CONVERTER_FORMAT, format);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_converter_control(uint8_t nid, uint8_t stream, uint8_t channel) const {
	auto index = controller->submit_verb(cid, nid, cmd::SET_CONVERTER_CONTROL, channel | stream << 4);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_pin_control(uint8_t nid, uint8_t data) const {
	auto index = controller->submit_verb(cid, nid, cmd::SET_PIN_CONTROL, data);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_eapd_enable(uint8_t nid, uint8_t data) const {
	auto index = controller->submit_verb(cid, nid, cmd::SET_EAPD_ENABLE, data);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_converter_channel_count(uint8_t nid, uint8_t count) const {
	auto index = controller->submit_verb(cid, nid, cmd::SET_CONVERTER_CHANNEL_COUNT, count);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}

UhdaStatus UhdaCodec::set_power_state(uint8_t nid, uint8_t data) const {
	auto index = controller->submit_verb(cid, nid, cmd::SET_POWER_STATE, data);
	ResponseDescriptor resp {};
	return controller->wait_for_verb(index, resp);
}
