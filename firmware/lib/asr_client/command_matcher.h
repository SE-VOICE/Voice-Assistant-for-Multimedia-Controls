#pragma once

#include "command_ids.h"
#include <stdint.h>
#include <stddef.h>

namespace vamc {

// VAMC-F-005: map a cloud ASR transcript to one of the 7 supported commands.
//
// The matcher is pure text logic: no heap, no I/O, no hardware. It decides
// what the transcript means; it does not send anything or drive feedback.
// Reporting a non-match (error LED/buzzer, no command sent) is VAMC-F-006.

enum class MatchOutcome : uint8_t {
    MATCHED,       // exactly one command, `command` is valid
    UNRECOGNIZED,  // no supported command could be read from the transcript
    AMBIGUOUS      // the transcript names two or more different commands
};

enum class MatchKind : uint8_t {
    NONE,    // outcome is not MATCHED
    PHRASE,  // every word was known vocabulary (exact phrase or synonym)
    FUZZY    // at least one word was corrected by edit distance (near match)
};

struct MatchResult {
    MatchOutcome outcome;
    MatchKind kind;
    CommandId command;  // meaningful only when outcome == MATCHED
};

namespace matcher_limits {
    // Transcripts longer than this are rejected as UNRECOGNIZED rather than
    // truncated, so a partial read can never produce a command.
    constexpr size_t kMaxTranscriptChars = 128;
    constexpr size_t kMaxTokens = 16;
    constexpr size_t kMaxTokenChars = 24;  // including terminator

    // Near-match thresholds (pending team review; see PR description).
    // Words shorter than kFuzzyMinWordLen are never corrected, because short
    // real words sit too close to command words ("plan"/"play", "nest"/"next")
    // and a correction there would be a false trigger.
    constexpr size_t kFuzzyMinWordLen = 5;
    constexpr size_t kFuzzyLongWordLen = 7;
    constexpr uint8_t kFuzzyMaxDistShort = 1;  // words of length 5..6
    constexpr uint8_t kFuzzyMaxDistLong = 2;   // words of length >= 7
} // namespace matcher_limits

class CommandMatcher {
public:
    // Matches a transcript such as "Turn it up, please." Null, empty, overlong
    // or filler-only transcripts are UNRECOGNIZED.
    static MatchResult match(const char* transcript);

    // Exposed for tests: bounded Levenshtein distance between two lowercase
    // words. Returns 255 when either word exceeds kMaxTokenChars - 1.
    static uint8_t edit_distance(const char* a, const char* b);
};

} // namespace vamc
