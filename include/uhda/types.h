#pragma once

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

#include <stdint.h>
#include <stddef.h>

typedef enum UhdaStatus {
	UHDA_STATUS_SUCCESS,
	UHDA_STATUS_UNSUPPORTED,
	UHDA_STATUS_NO_MEMORY,
	UHDA_STATUS_TIMEOUT
} UhdaStatus;

typedef uintptr_t UhdaIrqState;

typedef bool (*UhdaIrqHandlerFn)(void* arg);

typedef struct UhdaController UhdaController;
typedef struct UhdaCodec UhdaCodec;

typedef struct UhdaPath UhdaPath;
typedef struct UhdaStream UhdaStream;
typedef struct UhdaOutput UhdaOutput;
typedef struct UhdaOutputGroup UhdaOutputGroup;

typedef enum UhdaOutputType {
	UHDA_OUTPUT_TYPE_LINE_OUT,
	UHDA_OUTPUT_TYPE_SPEAKER,
	UHDA_OUTPUT_TYPE_HEADPHONE,
	UHDA_OUTPUT_TYPE_CD,
	UHDA_OUTPUT_TYPE_SPDIF_OUT,
	UHDA_OUTPUT_TYPE_OTHER_DIGITAL_OUT,
	UHDA_OUTPUT_TYPE_UNKNOWN
} UhdaOutputType;

typedef struct UhdaOutputInfo {
	UhdaOutputType type;
} UhdaOutputInfo;

typedef void (*UhdaBufferFillFn)(void* arg, void* buffer, uint32_t space);

typedef enum UhdaFormat {
	UHDA_FORMAT_PCM8,
	UHDA_FORMAT_PCM16,
	UHDA_FORMAT_PCM20,
	UHDA_FORMAT_PCM24,
	UHDA_FORMAT_PCM32
} UhdaFormat;

typedef struct UhdaStreamParams {
	uint32_t sample_rate;
	uint32_t channels;
	UhdaFormat fmt;
} UhdaStreamParams;
