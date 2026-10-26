#include "diagnostics_ota.h"
#include "command_map.h"
#include <stdio.h>

namespace vamc {

CircularLogBuffer::CircularLogBuffer(size_t capacity)
    : capacity_((capacity == 0) ? 1 : capacity),
      count_(0),
      head_(0),
      buffer_(new LogEntry[capacity_]) {}

CircularLogBuffer::~CircularLogBuffer() {
    delete[] buffer_;
}

void CircularLogBuffer::append(const LogEntry& entry) {
    buffer_[head_] = entry;
    head_ = (head_ + 1) % capacity_;
    if (count_ < capacity_) {
        ++count_;
    }
}

void CircularLogBuffer::clear() {
    count_ = 0;
    head_ = 0;
}

bool CircularLogBuffer::get_entry(size_t index, LogEntry& out_entry) const {
    if (index >= count_) {
        return false;
    }

    // Determine position of the oldest entry in the circular buffer
    size_t oldest_index = (count_ == capacity_) ? head_ : 0;
    size_t physical_index = (oldest_index + index) % capacity_;

    out_entry = buffer_[physical_index];
    return true;
}

size_t CircularLogBuffer::format_entry(const LogEntry& entry, char* out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0) return 0;

    const char* cmd_name = CommandMapper::to_string(entry.command);
    int written = snprintf(out_buf, buf_size, "[TS:%u ms] CMD:%s ERR:0x%04X",
                           static_cast<unsigned>(entry.timestamp),
                           cmd_name,
                           static_cast<unsigned>(entry.error_code));

    if (written < 0) return 0;
    return (static_cast<size_t>(written) >= buf_size) ? (buf_size - 1) : static_cast<size_t>(written);
}

void CircularLogBuffer::dump(void (*print_fn)(const char* line)) const {
    if (!print_fn) return;

    char line_buf[128];
    for (size_t i = 0; i < count_; ++i) {
        LogEntry entry;
        if (get_entry(i, entry)) {
            format_entry(entry, line_buf, sizeof(line_buf));
            print_fn(line_buf);
        }
    }
}

} // namespace vamc
