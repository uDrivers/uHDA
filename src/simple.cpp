#include "uhda/simple.h"
#include "uhda/kernel_api.h"
#include "lock_guard.hpp"

static constexpr uint32_t ALLOWED_SOFTWARE_AHEAD = 0x1000 * 4;

#define UHDA_MIN(a, b) ((a) < (b) ? (a) : (b))

#define memcpy __builtin_memcpy
#define memset __builtin_memset

using namespace uhda;

struct RingBuffer {
	char* ptr;
	uint32_t read_pos;
	uint32_t write_pos;
	uint32_t capacity;
	uint32_t size;

	UhdaStatus init(uint32_t new_capacity) {
		ptr = static_cast<char*>(uhda_kernel_malloc(new_capacity));
		if (!ptr) {
			return UHDA_STATUS_NO_MEMORY;
		}

		read_pos = 0;
		write_pos = 0;
		capacity = new_capacity;
		size = 0;
		return UHDA_STATUS_SUCCESS;
	}

	void destroy() {
		if (ptr) {
			uhda_kernel_free(ptr, capacity);
			ptr = nullptr;
		}
	}

	void read(void* data, uint32_t to_read) {
		auto* data_ptr = static_cast<char*>(data);

		uint32_t i = 0;
		while (i < to_read) {
			uint32_t chunk = UHDA_MIN(to_read - i, capacity - read_pos);
			memcpy(data_ptr + i, ptr + read_pos, chunk);

			read_pos += chunk;
			if (read_pos == capacity) {
				read_pos = 0;
			}

			i += chunk;
		}

		size -= to_read;
	}

	void write(const void* data, uint32_t to_write) {
		auto* data_ptr = static_cast<const char*>(data);

		uint32_t i = 0;
		while (i < to_write) {
			uint32_t chunk = UHDA_MIN(to_write - i, capacity - write_pos);
			memcpy(ptr + write_pos, data_ptr + i, chunk);

			write_pos += chunk;
			if (write_pos == capacity) {
				write_pos = 0;
			}

			i += chunk;
		}

		size += to_write;
	}

	void clear() {
		read_pos = 0;
		write_pos = 0;
		size = 0;
	}
};

struct UhdaSimpleStream {
	UhdaStream* base;
	UhdaStreamParams params;
	RingBuffer buffer;
	void* lock;
	uint32_t prev_irq_pos;
	uint32_t current_fill_pos;
	const UhdaScatterChunk* chunks;

	[[nodiscard]] uint32_t get_software_ahead(uint32_t pos) const {
		uint32_t software_ahead;
		if (current_fill_pos >= pos) {
			software_ahead = current_fill_pos - pos;
		}
		else {
			uint32_t buffer_size = params.period_count * params.period_size;
			software_ahead = buffer_size - pos + current_fill_pos;
		}

		return software_ahead;
	}
};

UhdaStatus uhda_simple_stream_new(UhdaStream* base, UhdaSimpleStream** res) {
	auto* stream = static_cast<UhdaSimpleStream*>(uhda_kernel_malloc(sizeof(UhdaSimpleStream)));
	if (!stream) {
		return UHDA_STATUS_NO_MEMORY;
	}
	*stream = {};
	stream->base = base;

	auto status = uhda_kernel_create_spinlock(&stream->lock);
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_free(stream, sizeof(UhdaSimpleStream));
		return status;
	}

	*res = stream;
	return UHDA_STATUS_SUCCESS;
}

void uhda_simple_stream_destroy(UhdaSimpleStream* stream) {
	stream->buffer.destroy();
	uhda_kernel_free_spinlock(stream->lock);
	uhda_kernel_free(stream, sizeof(UhdaSimpleStream));
}

UhdaStatus uhda_simple_path_setup(UhdaPath* path, UhdaSimpleStream* stream) {
	return uhda_path_setup(path, &stream->params, stream->base);
}

static void uhda_copy_bytes_from_ring(UhdaSimpleStream* stream, uint32_t size) {
	uint32_t buffer_size = stream->params.period_count * stream->params.period_size;

	while (size) {
		uint32_t period = stream->current_fill_pos / 0x1000;
		uint32_t period_offset = stream->current_fill_pos % 0x1000;
		auto* period_ptr = static_cast<char*>(stream->chunks[period].virt);
		period_ptr += period_offset;

		uint32_t to_copy_period = UHDA_MIN(size, 0x1000 - period_offset);

		uint32_t copy_progress = 0;

		if (stream->buffer.size) {
			uint32_t ring_to_copy = UHDA_MIN(to_copy_period, stream->buffer.size);
			stream->buffer.read(period_ptr, ring_to_copy);
			copy_progress += ring_to_copy;
		}

		if (copy_progress != to_copy_period) {
			memset(period_ptr + copy_progress, 0, to_copy_period - copy_progress);
		}

		size -= to_copy_period;
		stream->current_fill_pos += to_copy_period;

		if (stream->current_fill_pos == buffer_size) {
			stream->current_fill_pos = 0;
		}
	}
}

static void uhda_simple_period_callback(UhdaStream*, void* arg) {
	auto* stream = static_cast<UhdaSimpleStream*>(arg);

	LockGuard guard {stream->lock};

	uint32_t buffer_size = stream->params.period_count * stream->params.period_size;

	uint32_t pos = uhda_stream_get_position(stream->base);

	uint32_t bytes_after_last_irq;
	if (pos >= stream->prev_irq_pos) {
		bytes_after_last_irq = pos - stream->prev_irq_pos;
	}
	else {
		bytes_after_last_irq = buffer_size - stream->prev_irq_pos + pos;
	}

	uhda_copy_bytes_from_ring(stream, bytes_after_last_irq);

	stream->prev_irq_pos = pos;
}

UhdaStatus uhda_simple_stream_setup(UhdaSimpleStream* stream, const UhdaSimpleStreamParams* params) {
	auto status = stream->buffer.init(params->ring_buffer_size);
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	stream->params = {
		.sample_rate = params->sample_rate,
		.channels = params->channels,
		.fmt = params->fmt,
		.period_count = 256,
		.period_size = 0x1000,
		.period_callback_distance = 1,
		.period_callback = uhda_simple_period_callback,
		.period_callback_arg = stream
	};

	status = uhda_stream_setup(stream->base, &stream->params);
	if (status == UHDA_STATUS_SUCCESS) {
		uhda_stream_get_periods(stream->base, &stream->chunks);
	}
	else {
		stream->buffer.destroy();
	}

	return status;
}

UhdaStatus uhda_simple_stream_play(UhdaSimpleStream* stream, bool play) {
	auto status = uhda_stream_get_status(stream->base);

	if (status == UHDA_STREAM_STATUS_PAUSED) {
		auto pos = uhda_stream_get_position(stream->base);
		auto software_ahead = stream->get_software_ahead(pos);

		if (software_ahead < ALLOWED_SOFTWARE_AHEAD) {
			uint32_t allowed_copy = ALLOWED_SOFTWARE_AHEAD - software_ahead;
			uhda_copy_bytes_from_ring(stream, allowed_copy);
		}
	}

	return uhda_stream_play(stream->base, play);
}

UhdaStatus uhda_simple_stream_queue_data(UhdaSimpleStream* stream, const void* data, uint32_t* size) {
	LockGuard guard {stream->lock};

	uint32_t to_copy = UHDA_MIN(*size, stream->buffer.capacity - stream->buffer.size);
	stream->buffer.write(data, to_copy);
	*size = to_copy;

	return UHDA_STATUS_SUCCESS;
}

UhdaStatus uhda_simple_stream_clear_queue(UhdaSimpleStream* stream) {
	LockGuard guard {stream->lock};
	stream->buffer.clear();
	return UHDA_STATUS_SUCCESS;
}

UhdaStatus uhda_simple_stream_get_remaining(const UhdaSimpleStream* stream, uint32_t* remaining) {
	LockGuard guard {stream->lock};
	*remaining = stream->buffer.size;
	return UHDA_STATUS_SUCCESS;
}

uint32_t uhda_simple_stream_get_buffer_size(const UhdaSimpleStream* stream) {
	return stream->buffer.capacity;
}

UhdaStream* uhda_simple_stream_get_base(const UhdaSimpleStream* stream) {
	return stream->base;
}
