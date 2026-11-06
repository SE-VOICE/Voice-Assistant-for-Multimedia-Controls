#include "command_matcher.h"
#include <ctype.h>
#include <string.h>

namespace vamc {

namespace {

using namespace matcher_limits;

// A phrase is a sequence of content words (after filler removal) that names
// one command. Matching is whole-word and in order.
struct Phrase {
    CommandId command;
    uint8_t length;
    const char* words[3];
};

const Phrase kPhrases[] = {
    // VOLUME_UP
    {CommandId::VOLUME_UP, 2, {"volume", "up"}},
    {CommandId::VOLUME_UP, 2, {"vol", "up"}},
    {CommandId::VOLUME_UP, 2, {"turn", "up"}},
    {CommandId::VOLUME_UP, 3, {"turn", "volume", "up"}},
    {CommandId::VOLUME_UP, 3, {"turn", "up", "volume"}},
    {CommandId::VOLUME_UP, 2, {"increase", "volume"}},
    {CommandId::VOLUME_UP, 2, {"raise", "volume"}},
    {CommandId::VOLUME_UP, 1, {"louder"}},
    // VOLUME_DOWN
    {CommandId::VOLUME_DOWN, 2, {"volume", "down"}},
    {CommandId::VOLUME_DOWN, 2, {"vol", "down"}},
    {CommandId::VOLUME_DOWN, 2, {"turn", "down"}},
    {CommandId::VOLUME_DOWN, 3, {"turn", "volume", "down"}},
    {CommandId::VOLUME_DOWN, 3, {"turn", "down", "volume"}},
    {CommandId::VOLUME_DOWN, 2, {"decrease", "volume"}},
    {CommandId::VOLUME_DOWN, 2, {"lower", "volume"}},
    {CommandId::VOLUME_DOWN, 2, {"reduce", "volume"}},
    {CommandId::VOLUME_DOWN, 1, {"quieter"}},
    {CommandId::VOLUME_DOWN, 1, {"softer"}},
    // PAUSE
    {CommandId::PAUSE, 1, {"pause"}},
    // RESUME
    {CommandId::RESUME, 1, {"resume"}},
    {CommandId::RESUME, 1, {"continue"}},
    {CommandId::RESUME, 1, {"unpause"}},
    // PLAY
    {CommandId::PLAY, 1, {"play"}},
    {CommandId::PLAY, 1, {"start"}},
    // NEXT
    {CommandId::NEXT, 1, {"next"}},
    {CommandId::NEXT, 1, {"skip"}},
    // PREVIOUS
    {CommandId::PREVIOUS, 1, {"previous"}},
    {CommandId::PREVIOUS, 1, {"prev"}},
    {CommandId::PREVIOUS, 1, {"back"}},
};
const size_t kPhraseCount = sizeof(kPhrases) / sizeof(kPhrases[0]);

// Words that carry no command meaning and are dropped before phrase matching:
// politeness, pronouns, and media nouns ("next song" == "next").
const char* const kFillers[] = {
    "a", "an", "the", "please", "can", "could", "would", "will", "you",
    "hey", "now", "it", "its", "this", "that", "my", "me", "just", "go",
    "to", "song", "track", "music", "playback", "one", "and", "then",
    "little", "bit", "some", "ok", "okay", "thanks", "thank",
};
const size_t kFillerCount = sizeof(kFillers) / sizeof(kFillers[0]);

bool is_filler(const char* word) {
    for (size_t i = 0; i < kFillerCount; ++i) {
        if (strcmp(word, kFillers[i]) == 0) return true;
    }
    return false;
}

// Returns the canonical vocabulary string equal to `word`, or nullptr.
const char* find_vocabulary(const char* word) {
    for (size_t p = 0; p < kPhraseCount; ++p) {
        for (uint8_t w = 0; w < kPhrases[p].length; ++w) {
            if (strcmp(word, kPhrases[p].words[w]) == 0) return kPhrases[p].words[w];
        }
    }
    for (size_t i = 0; i < kFillerCount; ++i) {
        if (strcmp(word, kFillers[i]) == 0) return kFillers[i];
    }
    return nullptr;
}

void consider_candidate(const char* word, const char* candidate, uint8_t limit,
                        uint8_t& best_dist, const char*& best, bool& tie) {
    uint8_t d = CommandMatcher::edit_distance(word, candidate);
    if (d > limit) return;
    if (d < best_dist) {
        best_dist = d;
        best = candidate;
        tie = false;
    } else if (d == best_dist && best != nullptr && strcmp(best, candidate) != 0) {
        tie = true;
    }
}

// Near match: the single closest vocabulary word within the threshold.
// Returns nullptr if the word is too short, nothing is close enough, or two
// different words are equally close.
const char* correct_word(const char* word) {
    size_t len = strlen(word);
    if (len < kFuzzyMinWordLen) return nullptr;
    uint8_t limit = (len >= kFuzzyLongWordLen) ? kFuzzyMaxDistLong : kFuzzyMaxDistShort;

    uint8_t best_dist = 255;
    const char* best = nullptr;
    bool tie = false;
    for (size_t p = 0; p < kPhraseCount; ++p) {
        for (uint8_t w = 0; w < kPhrases[p].length; ++w) {
            consider_candidate(word, kPhrases[p].words[w], limit, best_dist, best, tie);
        }
    }
    for (size_t i = 0; i < kFillerCount; ++i) {
        consider_candidate(word, kFillers[i], limit, best_dist, best, tie);
    }
    return tie ? nullptr : best;
}

MatchResult make_result(MatchOutcome outcome, MatchKind kind, CommandId command) {
    MatchResult r;
    r.outcome = outcome;
    r.kind = kind;
    r.command = command;
    return r;
}

MatchResult unrecognized() {
    return make_result(MatchOutcome::UNRECOGNIZED, MatchKind::NONE, CommandId::PAUSE);
}

} // namespace

uint8_t CommandMatcher::edit_distance(const char* a, const char* b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    if (la >= kMaxTokenChars || lb >= kMaxTokenChars) return 255;

    uint8_t prev[kMaxTokenChars];
    uint8_t curr[kMaxTokenChars];
    for (size_t j = 0; j <= lb; ++j) prev[j] = static_cast<uint8_t>(j);
    for (size_t i = 1; i <= la; ++i) {
        curr[0] = static_cast<uint8_t>(i);
        for (size_t j = 1; j <= lb; ++j) {
            uint8_t sub = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
            uint8_t del = prev[j] + 1;
            uint8_t ins = curr[j - 1] + 1;
            uint8_t m = sub < del ? sub : del;
            curr[j] = m < ins ? m : ins;
        }
        memcpy(prev, curr, lb + 1);
    }
    return prev[lb];
}

MatchResult CommandMatcher::match(const char* transcript) {
    if (transcript == nullptr) return unrecognized();

    // 1. Normalise: ASCII letters/digits lowercased into words; apostrophes
    //    are dropped ("it's" -> "its"); everything else separates words.
    char raw[kMaxTokens][kMaxTokenChars];
    size_t token_count = 0;
    size_t token_len = 0;
    size_t i = 0;
    for (;; ++i) {
        if (i > kMaxTranscriptChars) return unrecognized();
        char c = transcript[i];
        unsigned char uc = static_cast<unsigned char>(c);
        bool word_char = (c != '\0') && (uc < 0x80) && isalnum(uc);
        if (word_char) {
            if (token_len == 0 && token_count == kMaxTokens) return unrecognized();
            if (token_len + 1 >= kMaxTokenChars) return unrecognized();
            raw[token_count][token_len++] = static_cast<char>(tolower(uc));
            continue;
        }
        if (c == '\'' && token_len > 0) continue;
        if (token_len > 0) {
            raw[token_count][token_len] = '\0';
            ++token_count;
            token_len = 0;
        }
        if (c == '\0') break;
    }

    // 2. Resolve each word to vocabulary, correcting near misses; drop fillers.
    const char* content[kMaxTokens];
    size_t content_count = 0;
    bool corrected = false;
    for (size_t t = 0; t < token_count; ++t) {
        const char* word = find_vocabulary(raw[t]);
        if (word == nullptr) {
            word = correct_word(raw[t]);
            if (word == nullptr) return unrecognized();  // unknown word
            corrected = true;
        }
        if (!is_filler(word)) content[content_count++] = word;
    }
    if (content_count == 0) return unrecognized();

    // 3. Cover the content words with phrases, longest phrase first. Every
    //    content word must belong to a phrase; leftovers mean "not a command".
    bool have_command = false;
    bool ambiguous = false;
    CommandId command = CommandId::PAUSE;
    size_t pos = 0;
    while (pos < content_count) {
        const Phrase* best = nullptr;
        for (size_t p = 0; p < kPhraseCount; ++p) {
            const Phrase& ph = kPhrases[p];
            if (pos + ph.length > content_count) continue;
            if (best != nullptr && ph.length <= best->length) continue;
            bool equal = true;
            for (uint8_t w = 0; w < ph.length && equal; ++w) {
                equal = strcmp(content[pos + w], ph.words[w]) == 0;
            }
            if (equal) best = &ph;
        }
        if (best == nullptr) return unrecognized();
        if (have_command && best->command != command) ambiguous = true;
        command = best->command;
        have_command = true;
        pos += best->length;
    }

    if (ambiguous) return make_result(MatchOutcome::AMBIGUOUS, MatchKind::NONE, command);
    return make_result(MatchOutcome::MATCHED,
                       corrected ? MatchKind::FUZZY : MatchKind::PHRASE, command);
}

} // namespace vamc
