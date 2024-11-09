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

typedef enum UhdaIrqHint {
	UHDA_IRQ_HINT_ANY,
	UHDA_IRQ_HINT_INTX
} UhdaIrqHint;

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

typedef enum UhdaColor {
	UHDA_COLOR_UNKNOWN = 0,
	UHDA_COLOR_BLACK = 1,
	UHDA_COLOR_GREY = 2,
	UHDA_COLOR_BLUE = 3,
	UHDA_COLOR_GREEN = 4,
	UHDA_COLOR_RED = 5,
	UHDA_COLOR_ORANGE = 6,
	UHDA_COLOR_YELLOW = 7,
	UHDA_COLOR_PURPLE = 8,
	UHDA_COLOR_PINK = 9,
	UHDA_COLOR_WHITE = 14,
	UHDA_COLOR_OTHER = 15
} UhdaColor;

typedef enum UhdaLocation {
	UHDA_LOCATION_NA = 0,
	UHDA_LOCATION_REAR = 1,
	UHDA_LOCATION_FRONT = 2,
	UHDA_LOCATION_LEFT = 3,
	UHDA_LOCATION_RIGHT = 4,
	UHDA_LOCATION_TOP = 5,
	UHDA_LOCATION_BOTTOM = 6,
	UHDA_LOCATION_SPECIAL = 7,
	UHDA_LOCATION_REAR_PANEL,
	UHDA_LOCATION_DRIVE_BAY,
	UHDA_LOCATION_RISER,
	UHDA_LOCATION_DISPLAY,
	UHDA_LOCATION_ATAPI,
	UHDA_LOCATION_INSIDE_LID,
	UHDA_LOCATION_OUTSIDE_LID,
	UHDA_LOCATION_UNKNOWN
} UhdaLocation;

typedef struct UhdaOutputInfo {
	UhdaOutputType type;
	UhdaColor color;
	UhdaLocation location;
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
