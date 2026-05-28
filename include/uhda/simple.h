#pragma once

#include "types.h"
#include "uhda.h"

/*
 * This file provides a simpler double-buffered stream API implemented on top of the more advanced API.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UhdaSimpleStream UhdaSimpleStream;

/*
 * Stream parameters
 *
 * `ring_buffer_size` is the size of an internal ring buffer that is used to queue output.
 *
 * Note: the ring buffer size is rounded to be a multiple of 0x1000.
 */
typedef struct UhdaSimpleStreamParams {
	uint32_t sample_rate;
	uint32_t channels;
	UhdaFormat fmt;
	uint32_t ring_buffer_size;
} UhdaSimpleStreamParams;

UhdaStatus uhda_simple_stream_new(UhdaStream* base, UhdaSimpleStream** res);
void uhda_simple_stream_destroy(UhdaSimpleStream* stream);

/*
 * Sets up a path for playback.
 */
UhdaStatus uhda_simple_path_setup(UhdaPath* path, UhdaSimpleStream* stream);

/*
 * Sets up a stream for playback.
 */
UhdaStatus uhda_simple_stream_setup(
	UhdaSimpleStream* stream,
	const UhdaSimpleStreamParams* params);

/*
 * Begins/stops playback on the stream.
 */
UhdaStatus uhda_simple_stream_play(UhdaSimpleStream* stream, bool play);

/*
 * Queues data to the stream and returns the actual amount of data written in `size`.
 *
 * Note: this function is asynchronous, it doesn't block if the ring buffer space is exhausted.
 */
UhdaStatus uhda_simple_stream_queue_data(UhdaSimpleStream* stream, const void* data, uint32_t* size);

/*
 * Clears all currently queued data from the stream.
 */
UhdaStatus uhda_simple_stream_clear_queue(UhdaSimpleStream* stream);

/*
 * Gets the amount of remaining queued data within a stream.
 */
UhdaStatus uhda_simple_stream_get_remaining(const UhdaSimpleStream* stream, uint32_t* remaining);

/*
 * Gets the size of the stream's ring buffer chosen in uhda_simple_stream_setup.
 */
uint32_t uhda_simple_stream_get_buffer_size(const UhdaSimpleStream* stream);

/*
 * Gets the underlying base stream passed to uhda_simple_stream_new.
 */
UhdaStream* uhda_simple_stream_get_base(const UhdaSimpleStream* stream);

#ifdef __cplusplus
}
#endif
