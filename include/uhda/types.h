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
	UHDA_STATUS_MISALIGNED_MEMORY,
	UHDA_STATUS_TIMEOUT,
} UhdaStatus;

typedef struct UhdaScatterChunk {
	uintptr_t phys;
	void* virt;
} UhdaScatterChunk;

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

typedef void (*UhdaPeriodFn)(UhdaStream* stream, void* arg);

typedef enum UhdaFormat {
	UHDA_FORMAT_PCM8,
	UHDA_FORMAT_PCM16,
	UHDA_FORMAT_PCM20,
	UHDA_FORMAT_PCM24,
	UHDA_FORMAT_PCM32
} UhdaFormat;

typedef struct UhdaPathInfo {
	const uint32_t* supported_sample_rates;
	uint32_t supported_sample_rate_count;
	const UhdaFormat* supported_formats;
	uint32_t supported_formats_count;
} UhdaPathInfo;

/*
 * Stream parameters
 *
 * `period_count` is the count of periods in one buffer.
 * `period_size` is the size of one period.
 * `period_callback_distance` is the amount of periods between calls to the period callback.
 * `period_callback` is a callback called when a period worth of data is consumed/produced.
 *
 * Total buffer size is equal to `period_count * period_size`.
 *
 * Note: the period callback is run in an interrupt context and is mandatory.
 * Note: it is advised for `period_count` to be evenly divisible by `period_callback_distance`,
 * if not then the time between the last and first callbacks of the buffer is different from the rest.
 */

typedef struct UhdaStreamParams {
	uint32_t sample_rate;
	uint32_t channels;
	UhdaFormat fmt;
	uint32_t period_count;
	uint32_t period_size;
	uint32_t period_callback_distance;
	UhdaPeriodFn period_callback;
	void* period_callback_arg;
} UhdaStreamParams;

#define UHDA_MIN_PERIODS 2
#define UHDA_MAX_PERIODS 256

#define UHDA_MIN_PERIOD_SIZE 2
#define UHDA_PERIOD_SIZE_ALIGNMENT 2
#define UHDA_MAX_PERIOD_SIZE 0xFFFFFFFF

#define UHDA_MIN_PERIOD_CALLBACK_DISTANCE 1

typedef enum UhdaStreamStatus {
	UHDA_STREAM_STATUS_UNINITIALIZED,
	UHDA_STREAM_STATUS_RUNNING,
	UHDA_STREAM_STATUS_PAUSED
} UhdaStreamStatus;
