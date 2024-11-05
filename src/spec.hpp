#pragma once
#include "reg.hpp"

namespace uhda {
	namespace regs {
		static constexpr BitRegister<uint16_t> GCAP {0};
		static constexpr BasicRegister<uint8_t> VMIN {0x2};
		static constexpr BasicRegister<uint8_t> VMAJ {0x3};
		static constexpr BitRegister<uint16_t> OUTPAY {0x4};
		static constexpr BitRegister<uint16_t> INPAY {0x6};
		static constexpr BitRegister<uint32_t> GCTL {0x8};
		static constexpr BitRegister<uint16_t> WAKEEN {0xC};
		static constexpr BitRegister<uint16_t> STATESTS {0xE};
		static constexpr BitRegister<uint16_t> GSTS {0x10};
		static constexpr BitRegister<uint16_t> OUTSTRMPAY {0x18};
		static constexpr BitRegister<uint16_t> INSTRMPAY {0x1A};
		static constexpr BitRegister<uint32_t> INTCTL {0x20};
		static constexpr BitRegister<uint32_t> INTSTS {0x24};
		static constexpr BasicRegister<uint32_t> WALCLK {0x30};
		static constexpr BasicRegister<uint32_t> SSYNC {0x38};
		static constexpr BasicRegister<uint32_t> CORBLBASE {0x40};
		static constexpr BasicRegister<uint32_t> CORBUBASE {0x44};
		static constexpr BitRegister<uint16_t> CORBWP {0x48};
		static constexpr BitRegister<uint16_t> CORBRP {0x4A};
		static constexpr BitRegister<uint8_t> CORBCTL {0x4C};
		static constexpr BitRegister<uint8_t> CORBSTS {0x4D};
		static constexpr BitRegister<uint8_t> CORBSIZE {0x4E};
		static constexpr BasicRegister<uint32_t> RIRBLBASE {0x50};
		static constexpr BasicRegister<uint32_t> RIRBUBASE {0x54};
		static constexpr BitRegister<uint16_t> RIRBWP {0x58};
		static constexpr BasicRegister<uint16_t> RINTCNT {0x5A};
		static constexpr BitRegister<uint8_t> RIRBCTL {0x5C};
		static constexpr BitRegister<uint8_t> RIRBSTS {0x5D};
		static constexpr BitRegister<uint8_t> RIRBSIZE {0x5E};
		static constexpr BitRegister<uint32_t> ICOI {0x60};
		static constexpr BitRegister<uint32_t> ICII {0x64};
		static constexpr BitRegister<uint16_t> ICIS {0x68};
		static constexpr BitRegister<uint32_t> DPLBASE {0x70};
		static constexpr BasicRegister<uint32_t> DPUBASE {0x74};

		namespace stream {
			static constexpr BitRegister<uint8_t> CTL0 {0};
			static constexpr BitRegister<uint8_t> CTL1 {0x1};
			static constexpr BitRegister<uint8_t> CTL2 {0x2};
			static constexpr BitRegister<uint8_t> STS {0x3};
			static constexpr BasicRegister<uint32_t> LPIB {0x4};
			static constexpr BasicRegister<uint32_t> CBL {0x8};
			static constexpr BasicRegister<uint16_t> LVI {0xC};
			static constexpr BasicRegister<uint16_t> FIFOS {0x10};
			static constexpr BitRegister<uint16_t> FMT {0x12};
			static constexpr BasicRegister<uint32_t> BDPL {0x18};
			static constexpr BasicRegister<uint32_t> BDPU {0x1C};
		}
	}

	namespace gcap {
		static constexpr BitField<uint16_t, bool> OK64 {0, 1};
		static constexpr BitField<uint16_t, uint8_t> NSDO {1, 2};
		static constexpr BitField<uint16_t, uint8_t> BSS {3, 5};
		static constexpr BitField<uint16_t, uint8_t> ISS {8, 4};
		static constexpr BitField<uint16_t, uint8_t> OSS {12, 4};
	}

	namespace gctl {
		static constexpr BitField<uint32_t, bool> CRST {0, 1};
		static constexpr BitField<uint32_t, bool> FCNTRL {1, 1};
		static constexpr BitField<uint32_t, bool> UNSOL {8, 1};
	}

	namespace intctl {
		static constexpr BitField<uint32_t, uint32_t> SIE {0, 30};
		static constexpr BitField<uint32_t, bool> CIE {30, 1};
		static constexpr BitField<uint32_t, bool> GIE {31, 1};
	}

	namespace intsts {
		static constexpr BitField<uint32_t, uint32_t> SIS {0, 30};
		static constexpr BitField<uint32_t, bool> CIS {30, 1};
		static constexpr BitField<uint32_t, bool> GIS {31, 1};
	}

	namespace corbwp {
		static constexpr BitField<uint16_t, uint8_t> WP {0, 8};
	}

	namespace corbrp {
		static constexpr BitField<uint16_t, uint8_t> RP {0, 8};
		static constexpr BitField<uint16_t, bool> RST {15, 1};
	}

	namespace corbctl {
		static constexpr BitField<uint8_t, bool> MEIE {0, 1};
		static constexpr BitField<uint8_t, bool> RUN {1, 1};
	}

	namespace corbsize {
		static constexpr BitField<uint8_t, uint8_t> SIZE {0, 2};
		static constexpr BitField<uint8_t, uint8_t> SZCAP {4, 4};
	}

	namespace rirbwp {
		static constexpr BitField<uint16_t, uint8_t> WP {0, 8};
		static constexpr BitField<uint16_t, bool> RST {15, 1};
	}

	namespace rirbctl {
		static constexpr BitField<uint8_t, bool> INTCTL {0, 1};
		static constexpr BitField<uint8_t, bool> DMAEN {1, 1};
		static constexpr BitField<uint8_t, bool> OIC {2, 1};
	}

	namespace rirbsts {
		static constexpr BitField<uint8_t, bool> INTFL {0, 1};
		static constexpr BitField<uint8_t, bool> bois {2, 1};
	}

	namespace rirbsize {
		static constexpr BitField<uint8_t, uint8_t> SIZE {0, 2};
		static constexpr BitField<uint8_t, uint8_t> SZCAP {4, 4};
	}

	namespace dplbase {
		static constexpr BitField<uint32_t, bool> DPBE {0, 1};
		static constexpr BitField<uint32_t, uint32_t> BASE {7, 25};
	}

	namespace sdctl0 {
		static constexpr BitField<uint8_t, bool> RST {0, 1};
		static constexpr BitField<uint8_t, bool> RUN {1, 1};
		static constexpr BitField<uint8_t, bool> IOCE {2, 1};
		static constexpr BitField<uint8_t, bool> FEIE {3, 1};
		static constexpr BitField<uint8_t, bool> DEIE {4, 1};
	}

	namespace sdctl2 {
		static constexpr BitField<uint8_t, uint8_t> STRIPE {0, 2};
		static constexpr BitField<uint8_t, bool> TP {2, 1};
		static constexpr BitField<uint8_t, bool> DIR {3, 1};
		static constexpr BitField<uint8_t, uint8_t> STRM {4, 4};
	}

	namespace sdlvi {
		static constexpr BitField<uint16_t, uint8_t> LVI {0, 8};
	}

	namespace sdsts {
		static constexpr BitField<uint8_t, bool> BCIS {2, 1};
		static constexpr BitField<uint8_t, bool> FIFOE {3, 1};
		static constexpr BitField<uint8_t, bool> DESE {4, 1};
		static constexpr BitField<uint8_t, bool> FIFORDY {5, 1};
	}

	namespace sdfmt {
		static constexpr BitField<uint16_t, uint8_t> CHAN {0, 4};
		static constexpr BitField<uint16_t, uint8_t> BITS {4, 3};
		static constexpr BitField<uint16_t, uint8_t> DIV {8, 3};
		static constexpr BitField<uint16_t, uint8_t> MULT {11, 3};
		static constexpr BitField<uint16_t, uint8_t> BASE {14, 1};

		static constexpr uint8_t BASE_48KHZ = 0;
		static constexpr uint8_t BASE_441KHZ = 1;

		static constexpr uint8_t MULT_1 = 0b000;
		static constexpr uint8_t MULT_2 = 0b001;
		static constexpr uint8_t MULT_3 = 0b010;
		static constexpr uint8_t MULT_4 = 0b011;

		static constexpr uint8_t DIV_1 = 0b000;
		static constexpr uint8_t DIV_2 = 0b001;
		static constexpr uint8_t DIV_3 = 0b010;
		static constexpr uint8_t DIV_4 = 0b011;
		static constexpr uint8_t DIV_5 = 0b100;
		static constexpr uint8_t DIV_6 = 0b101;
		static constexpr uint8_t DIV_7 = 0b110;
		static constexpr uint8_t DIV_8 = 0b111;

		static constexpr uint8_t BITS_8 = 0b000;
		static constexpr uint8_t BITS_16 = 0b001;
		static constexpr uint8_t BITS_20 = 0b010;
		static constexpr uint8_t BITS_24 = 0b011;
		static constexpr uint8_t BITS_32 = 0b100;
	}

	namespace verb {
		static constexpr BitField<uint32_t, uint32_t> PAYLOAD {0, 20};
		static constexpr BitField<uint32_t, uint8_t> NODE_ID {20, 8};
		static constexpr BitField<uint32_t, uint8_t> CODEC_ADDRESS {28, 4};
	}

	namespace pcm_format = sdfmt;

	struct PcmFormat {
		BitValue<uint16_t> value;

		constexpr uint32_t set_sample_rate(uint32_t rate) {
			uint8_t base = 0;
			uint8_t mult = 0;
			uint8_t div = 0;
			uint32_t real_rate = 0;

			if (rate <= 5513) {
				real_rate = 5513;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_8;
			}
			else if (rate <= 6000) {
				real_rate = 6000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_8;
			}
			else if (rate <= 6300) {
				real_rate = 6300;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_7;
			}
			else if (rate <= 6857) {
				real_rate = 6857;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_7;
			}
			else if (rate <= 7350) {
				real_rate = 7350;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_6;
			}
			else if (rate <= 8000) {
				real_rate = 8000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_6;
			}
			else if (rate <= 8820) {
				real_rate = 8820;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_5;
			}
			else if (rate <= 9600) {
				real_rate = 9600;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_5;
			}
			else if (rate <= 11025) {
				real_rate = 11025;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_4;
			}
			else if (rate <= 12000) {
				real_rate = 12000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_4;
			}
			else if (rate <= 12600) {
				real_rate = 12600;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_7;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 13714) {
				real_rate = 13714;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_7;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 14700) {
				real_rate = 14700;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_3;
			}
			else if (rate <= 16000) {
				real_rate = 16000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_3;
			}
			else if (rate <= 16538) {
				real_rate = 16538;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_8;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 17640) {
				real_rate = 17640;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_5;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 18000) {
				real_rate = 18000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_8;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 18900) {
				real_rate = 18900;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_7;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 19200) {
				real_rate = 19200;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_5;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 20571) {
				real_rate = 20571;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_7;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 22050) {
				real_rate = 22050;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_2;
			}
			else if (rate <= 24000) {
				real_rate = 24000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_2;
			}
			else if (rate <= 25200) {
				real_rate = 25200;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_7;
				mult = pcm_format::MULT_4;
			}
			else if (rate <= 26460) {
				real_rate = 26460;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_5;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 27429) {
				real_rate = 27429;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_7;
				mult = pcm_format::MULT_4;
			}
			else if (rate <= 28800) {
				real_rate = 28800;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_5;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 29400) {
				real_rate = 29400;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_3;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 32000) {
				real_rate = 32000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_3;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 33075) {
				real_rate = 33075;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_4;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 35280) {
				real_rate = 35280;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_5;
				mult = pcm_format::MULT_4;
			}
			else if (rate <= 36000) {
				real_rate = 36000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_4;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 38400) {
				real_rate = 38400;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_5;
				mult = pcm_format::MULT_4;
			}
			else if (rate <= 44100) {
				real_rate = 44100;
				base = pcm_format::BASE_441KHZ;
			}
			else if (rate <= 48000) {
				real_rate = 48000;
				base = pcm_format::BASE_48KHZ;
			}
			else if (rate <= 58800) {
				real_rate = 58800;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_3;
				mult = pcm_format::MULT_4;
			}
			else if (rate <= 64000) {
				real_rate = 64000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_3;
				mult = pcm_format::MULT_4;
			}
			else if (rate <= 66150) {
				real_rate = 66150;
				base = pcm_format::BASE_441KHZ;
				div = pcm_format::DIV_2;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 72000) {
				real_rate = 72000;
				base = pcm_format::BASE_48KHZ;
				div = pcm_format::DIV_2;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 88200) {
				real_rate = 88200;
				base = pcm_format::BASE_441KHZ;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 96000) {
				real_rate = 96000;
				base = pcm_format::BASE_48KHZ;
				mult = pcm_format::MULT_2;
			}
			else if (rate <= 132300) {
				real_rate = 132300;
				base = pcm_format::BASE_441KHZ;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 144000) {
				real_rate = 144000;
				base = pcm_format::BASE_48KHZ;
				mult = pcm_format::MULT_3;
			}
			else if (rate <= 176400) {
				real_rate = 176400;
				base = pcm_format::BASE_441KHZ;
				mult = pcm_format::MULT_4;
			}
			else {
				real_rate = 192000;
				base = pcm_format::BASE_48KHZ;
				mult = pcm_format::MULT_4;
			}

			value &= ~pcm_format::DIV;
			value &= ~pcm_format::MULT;
			value &= ~pcm_format::BASE;
			value |= pcm_format::DIV(div);
			value |= pcm_format::MULT(mult);
			value |= pcm_format::BASE(base);
			return real_rate;
		}

		constexpr uint32_t set_channels(uint32_t channels) {
			if (channels == 0) {
				channels = 1;
			}
			else if (channels > 16) {
				channels = 16;
			}

			value &= ~pcm_format::CHAN;
			value |= pcm_format::CHAN(channels - 1);
			return channels;
		}

		constexpr uint32_t set_bits_per_sample(uint32_t bits) {
			uint8_t bits_value = 0;
			if (bits <= 8) {
				bits = 8;
				bits_value = pcm_format::BITS_8;
			}
			else if (bits <= 16) {
				bits = 16;
				bits_value = pcm_format::BITS_16;
			}
			else if (bits <= 20) {
				bits = 20;
				bits_value = pcm_format::BITS_20;
			}
			else if (bits <= 24) {
				bits = 24;
				bits_value = pcm_format::BITS_24;
			}
			else {
				bits = 32;
				bits_value = pcm_format::BITS_32;
			}

			value &= ~pcm_format::BITS;
			value |= pcm_format::BITS(bits_value);
			return bits;
		}
	};

	struct BufferDescriptor {
		uint64_t address;
		uint32_t length;
		uint32_t ioc;
	};

	struct VerbDescriptor {
		BitValue<uint32_t> value;

		constexpr void set_payload(uint32_t payload) {
			value |= verb::PAYLOAD(payload);
		}

		constexpr void set_nid(uint8_t nid) {
			value |= verb::NODE_ID(nid);
		}

		constexpr void set_cid(uint8_t cid) {
			value |= verb::CODEC_ADDRESS(cid);
		}
	};

	struct ResponseDescriptor {
		uint32_t resp;
		uint32_t resp_ex;

		[[nodiscard]] constexpr uint8_t get_codec() const {
			return resp_ex & 0b1111;
		}

		[[nodiscard]] constexpr bool is_unsol() const {
			return resp_ex >> 4 & 1;
		}
	};

	namespace cmd {
		enum : uint16_t {
			SET_CONVERTER_FORMAT = 0x2,
			SET_AMP_GAIN_MUTE = 0x3,
			GET_CONVERTER_FORMAT = 0xA,
			GET_AMP_GAIN_MUTE = 0xB,
			GET_PARAM = 0xF00,
			GET_CONN_SELECT = 0xF01,
			GET_CONN_LIST = 0xF02,
			GET_CONVERTER_CONTROL = 0xF06,
			GET_PIN_CONTROL = 0xF07,
			GET_EAPD_ENABLE = 0xF0C,
			GET_VOLUME_KNOB = 0xF0F,
			GET_CONFIG_DEFAULT = 0xF1C,
			GET_CONVERTER_CHANNEL_COUNT = 0xF2D,
			SET_CONN_SELECT = 0x701,
			SET_POWER_STATE = 0X705,
			SET_CONVERTER_CONTROL = 0x706,
			SET_PIN_CONTROL = 0x707,
			SET_EAPD_ENABLE = 0x70C,
			SET_VOLUME_KNOB = 0x70F,
			SET_CONVERTER_CHANNEL_COUNT = 0x72D
		};
	}

	namespace param {
		enum : uint8_t {
			NODE_COUNT = 0x4,
			FUNC_GROUP_TYPE = 0x5,
			AUDIO_CAPS = 0x9,
			PIN_CAPS = 0xC,
			IN_AMP_CAPS = 0xD,
			CONN_LIST_LEN = 0xE,
			OUT_AMP_CAPS = 0x12
		};
	}

	namespace func_group_type {
		enum : uint8_t {
			AUDIO = 0x1
		};
	}

	namespace widget_type {
		enum : uint8_t {
			AUDIO_OUT = 0x0,
			AUDIO_IN = 0x1,
			AUDIO_MIXER = 0x2,
			AUDIO_SELECTOR = 0x3,
			PIN_COMPLEX = 0x4,
			POWER_WIDGET = 0x5,
			VOLUME_KNOB_WIDGET = 0x6,
			BEEP_GENERATOR_WIDGET = 0x7
		};
	}

	namespace default_dev {
		enum : uint8_t {
			LINE_OUT = 0,
			SPEAKER = 1,
			HP_OUT = 2,
			CD = 3,
			SPDIF_OUT = 4,
			DIGITAL_OTHER_OUT = 5,
			MODEM_LINE_SIDE = 6,
			MODEM_HANDSET_SIDE = 7,
			LINE_IN = 8,
			AUX = 9,
			MIN_IN = 10,
			TELEPHONY = 11,
			SPDIF_IN = 12,
			DIGITAL_OTHER_IN = 13,
			RESERVED = 14,
			OTHER = 15
		};
	}
}
