#include "command_map.h"
#include <string.h>
#include <ctype.h>

namespace vamc {

uint16_t CommandMapper::to_hid_usage(CommandId id) {
    switch (id) {
        case CommandId::VOLUME_UP:
            return hid_usage::VOLUME_INCREMENT;
        case CommandId::VOLUME_DOWN:
            return hid_usage::VOLUME_DECREMENT;
        case CommandId::PAUSE:
            return hid_usage::PAUSE;
        case CommandId::RESUME:
            return hid_usage::PLAY; // or PLAY_PAUSE
        case CommandId::PLAY:
            return hid_usage::PLAY;
        case CommandId::NEXT:
            return hid_usage::SCAN_NEXT_TRACK;
        case CommandId::PREVIOUS:
            return hid_usage::SCAN_PREV_TRACK;
        default:
            return hid_usage::NONE;
    }
}

bool CommandMapper::is_supported(CommandId id) {
    switch (id) {
        case CommandId::VOLUME_UP:
        case CommandId::VOLUME_DOWN:
        case CommandId::PAUSE:
        case CommandId::RESUME:
        case CommandId::PLAY:
        case CommandId::NEXT:
        case CommandId::PREVIOUS:
            return true;
        default:
            return false;
    }
}

const char* CommandMapper::to_string(CommandId id) {
    switch (id) {
        case CommandId::VOLUME_UP:   return "VOLUME_UP";
        case CommandId::VOLUME_DOWN: return "VOLUME_DOWN";
        case CommandId::PAUSE:       return "PAUSE";
        case CommandId::RESUME:      return "RESUME";
        case CommandId::PLAY:        return "PLAY";
        case CommandId::NEXT:        return "NEXT";
        case CommandId::PREVIOUS:    return "PREVIOUS";
        default:                     return "UNKNOWN";
    }
}

static bool str_equals_ignore_case(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

bool CommandMapper::from_string(const char* str, CommandId& out_id) {
    if (!str) return false;

    // Skip leading whitespace
    while (*str && isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    if (*str == '\0') return false;

    // Check canonical names and common natural language synonyms
    if (str_equals_ignore_case(str, "volume up") ||
        str_equals_ignore_case(str, "volume_up") ||
        str_equals_ignore_case(str, "vol up") ||
        str_equals_ignore_case(str, "increase volume") ||
        str_equals_ignore_case(str, "louder")) {
        out_id = CommandId::VOLUME_UP;
        return true;
    }
    if (str_equals_ignore_case(str, "volume down") ||
        str_equals_ignore_case(str, "volume_down") ||
        str_equals_ignore_case(str, "vol down") ||
        str_equals_ignore_case(str, "decrease volume") ||
        str_equals_ignore_case(str, "quieter") ||
        str_equals_ignore_case(str, "softer")) {
        out_id = CommandId::VOLUME_DOWN;
        return true;
    }
    if (str_equals_ignore_case(str, "pause") ||
        str_equals_ignore_case(str, "pause playback") ||
        str_equals_ignore_case(str, "pause music")) {
        out_id = CommandId::PAUSE;
        return true;
    }
    if (str_equals_ignore_case(str, "resume") ||
        str_equals_ignore_case(str, "resume playback") ||
        str_equals_ignore_case(str, "continue")) {
        out_id = CommandId::RESUME;
        return true;
    }
    if (str_equals_ignore_case(str, "play") ||
        str_equals_ignore_case(str, "play music") ||
        str_equals_ignore_case(str, "start playback") ||
        str_equals_ignore_case(str, "start music")) {
        out_id = CommandId::PLAY;
        return true;
    }
    if (str_equals_ignore_case(str, "next") ||
        str_equals_ignore_case(str, "next track") ||
        str_equals_ignore_case(str, "next song") ||
        str_equals_ignore_case(str, "skip")) {
        out_id = CommandId::NEXT;
        return true;
    }
    if (str_equals_ignore_case(str, "previous") ||
        str_equals_ignore_case(str, "prev") ||
        str_equals_ignore_case(str, "previous track") ||
        str_equals_ignore_case(str, "previous song") ||
        str_equals_ignore_case(str, "back")) {
        out_id = CommandId::PREVIOUS;
        return true;
    }

    return false;
}

} // namespace vamc
