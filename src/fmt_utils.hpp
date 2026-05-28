#pragma once

#include "spec.hpp"

namespace uhda {
	inline PcmFormat pcm_format_from_params(uint32_t sample_rate, uint32_t channels, UhdaFormat fmt) {
		PcmFormat pcm_fmt {};
		pcm_fmt.set_sample_rate(sample_rate);
		pcm_fmt.set_channels(channels);

		uint8_t bits = 0;
		switch (fmt) {
			case UHDA_FORMAT_PCM8:
				bits = 8;
				break;
			case UHDA_FORMAT_PCM16:
				bits = 16;
				break;
			case UHDA_FORMAT_PCM20:
				bits = 20;
				break;
			case UHDA_FORMAT_PCM24:
				bits = 24;
				break;
			case UHDA_FORMAT_PCM32:
				bits = 32;
				break;
		}

		pcm_fmt.set_bits_per_sample(bits);

		return pcm_fmt;
	}
}
