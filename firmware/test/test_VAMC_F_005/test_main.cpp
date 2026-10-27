#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "command_ids.h"
#include "command_matcher.h"
#include "command_map.h"

using namespace vamc;

void setUp() {}
void tearDown() {}

struct Case {
    const char* transcript;
    CommandId expected;
};

static void assert_matches(const Case* cases, size_t n, MatchKind kind) {
    char msg[160];
    for (size_t i = 0; i < n; ++i) {
        MatchResult r = CommandMatcher::match(cases[i].transcript);
        snprintf(msg, sizeof(msg), "transcript: \"%s\"", cases[i].transcript);
        TEST_ASSERT_EQUAL_MESSAGE(MatchOutcome::MATCHED, r.outcome, msg);
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].expected, r.command, msg);
        TEST_ASSERT_EQUAL_MESSAGE(kind, r.kind, msg);
    }
}

static void assert_outcome(const char* const* transcripts, size_t n, MatchOutcome outcome) {
    char msg[160];
    for (size_t i = 0; i < n; ++i) {
        MatchResult r = CommandMatcher::match(transcripts[i]);
        snprintf(msg, sizeof(msg), "transcript: \"%s\"", transcripts[i] ? transcripts[i] : "(null)");
        TEST_ASSERT_EQUAL_MESSAGE(outcome, r.outcome, msg);
        TEST_ASSERT_EQUAL_MESSAGE(MatchKind::NONE, r.kind, msg);
    }
}

// TC-ASR-02 (transcript level): exact command wording for all 7 commands.
void test_TC_ASR_02_exact_phrases_map_to_all_7_commands() {
    const Case cases[] = {
        {"volume up", CommandId::VOLUME_UP},
        {"volume down", CommandId::VOLUME_DOWN},
        {"pause", CommandId::PAUSE},
        {"resume", CommandId::RESUME},
        {"play", CommandId::PLAY},
        {"next", CommandId::NEXT},
        {"previous", CommandId::PREVIOUS},
    };
    assert_matches(cases, sizeof(cases) / sizeof(cases[0]), MatchKind::PHRASE);
}

// Issue #5 user story: "turn it up" and "volume up" both work.
void test_TC_ASR_02_user_story_turn_it_up() {
    const Case cases[] = {
        {"turn it up", CommandId::VOLUME_UP},
        {"volume up", CommandId::VOLUME_UP},
        {"Turn it up, please.", CommandId::VOLUME_UP},
        {"turn it down", CommandId::VOLUME_DOWN},
    };
    assert_matches(cases, sizeof(cases) / sizeof(cases[0]), MatchKind::PHRASE);
}

// TC-ASR-02: near-synonym phrases for every command.
void test_TC_ASR_02_near_synonyms_for_every_command() {
    const Case cases[] = {
        {"louder", CommandId::VOLUME_UP},
        {"turn the volume up", CommandId::VOLUME_UP},
        {"increase the volume", CommandId::VOLUME_UP},
        {"can you turn it up a little bit", CommandId::VOLUME_UP},
        {"quieter", CommandId::VOLUME_DOWN},
        {"lower the volume", CommandId::VOLUME_DOWN},
        {"turn down the volume please", CommandId::VOLUME_DOWN},
        {"pause the music", CommandId::PAUSE},
        {"pause it", CommandId::PAUSE},
        {"continue", CommandId::RESUME},
        {"unpause", CommandId::RESUME},
        {"resume playback", CommandId::RESUME},
        {"play music", CommandId::PLAY},
        {"start the song", CommandId::PLAY},
        {"next song", CommandId::NEXT},
        {"skip this track", CommandId::NEXT},
        {"go to the next one", CommandId::NEXT},
        {"go back", CommandId::PREVIOUS},
        {"previous track", CommandId::PREVIOUS},
        {"prev", CommandId::PREVIOUS},
    };
    assert_matches(cases, sizeof(cases) / sizeof(cases[0]), MatchKind::PHRASE);
}

// Transcripts arrive with ASR casing and punctuation; these must not matter.
void test_case_punctuation_and_whitespace_are_ignored() {
    const Case cases[] = {
        {"NEXT!", CommandId::NEXT},
        {"  Pause.  ", CommandId::PAUSE},
        {"Volume-Up", CommandId::VOLUME_UP},
        {"volume_up", CommandId::VOLUME_UP},
        {"it's quieter", CommandId::VOLUME_DOWN},
        {"\tplay\n", CommandId::PLAY},
    };
    assert_matches(cases, sizeof(cases) / sizeof(cases[0]), MatchKind::PHRASE);
}

// Near matches: a slightly misrecognised word is corrected by edit distance.
void test_near_match_corrects_misrecognised_words() {
    const Case cases[] = {
        {"volum up", CommandId::VOLUME_UP},
        {"turn the volumn down", CommandId::VOLUME_DOWN},
        {"paused", CommandId::PAUSE},
        {"resum", CommandId::RESUME},
        {"previos song", CommandId::PREVIOUS},
        {"pervious", CommandId::PREVIOUS},
        {"skips", CommandId::NEXT},
        {"pleas pause", CommandId::PAUSE},
    };
    assert_matches(cases, sizeof(cases) / sizeof(cases[0]), MatchKind::FUZZY);
}

// Short real words are never "corrected" into commands (false-trigger guard),
// and ties between two equally close words are not guessed.
void test_near_match_does_not_guess() {
    const char* const transcripts[] = {
        "plan",    // 4 letters: not corrected to "play"
        "nest",    // 4 letters: not corrected to "next"
        "lowder",  // equally close to "louder" and "lower"
        "volume",  // a known word that is not a whole command
        "turn",
        "up",
    };
    assert_outcome(transcripts, sizeof(transcripts) / sizeof(transcripts[0]),
                   MatchOutcome::UNRECOGNIZED);
}

// Every phrase the command-map module accepts must mean the same command here,
// so the two modules never disagree about a transcript.
void test_consistent_with_command_mapper_synonyms() {
    const char* const phrases[] = {
        "volume up", "volume_up", "vol up", "increase volume", "louder",
        "volume down", "volume_down", "vol down", "decrease volume", "quieter", "softer",
        "pause", "pause playback", "pause music",
        "resume", "resume playback", "continue",
        "play", "play music", "start playback", "start music",
        "next", "next track", "next song", "skip",
        "previous", "prev", "previous track", "previous song", "back",
    };
    char msg[160];
    for (size_t i = 0; i < sizeof(phrases) / sizeof(phrases[0]); ++i) {
        CommandId expected;
        snprintf(msg, sizeof(msg), "phrase: \"%s\"", phrases[i]);
        TEST_ASSERT_TRUE_MESSAGE(CommandMapper::from_string(phrases[i], expected), msg);
        MatchResult r = CommandMatcher::match(phrases[i]);
        TEST_ASSERT_EQUAL_MESSAGE(MatchOutcome::MATCHED, r.outcome, msg);
        TEST_ASSERT_EQUAL_MESSAGE(expected, r.command, msg);
    }
}

void test_unsupported_or_empty_transcripts_are_unrecognized() {
    const char* const transcripts[] = {
        nullptr,
        "",
        "   ",
        "...",
        "please",                   // filler only
        "what's the weather",
        "pause the dishwasher",     // command word plus unrelated content
        "volume up 5",
        "stop",                     // not one of the 7 commands
        "mute",
        "ponga",                    // gibberish
        "caf\xc3\xa9 next",         // non-ASCII bytes split words: "caf" is unknown
    };
    assert_outcome(transcripts, sizeof(transcripts) / sizeof(transcripts[0]),
                   MatchOutcome::UNRECOGNIZED);
}

// Two different commands in one transcript must not pick either one.
// (Reporting this to the user is VAMC-F-006.)
void test_conflicting_commands_are_ambiguous() {
    const char* const transcripts[] = {
        "pause next",
        "play and then skip",
        "volume up volume down",
        "play the next song",
    };
    assert_outcome(transcripts, sizeof(transcripts) / sizeof(transcripts[0]),
                   MatchOutcome::AMBIGUOUS);
}

void test_repeated_same_command_is_still_one_match() {
    const Case cases[] = {
        {"louder louder", CommandId::VOLUME_UP},
        {"next next", CommandId::NEXT},
    };
    assert_matches(cases, sizeof(cases) / sizeof(cases[0]), MatchKind::PHRASE);
}

void test_input_limits_reject_rather_than_truncate() {
    char at_limit[matcher_limits::kMaxTranscriptChars + 1];
    memset(at_limit, ' ', sizeof(at_limit));
    memcpy(at_limit, "next", 4);
    at_limit[matcher_limits::kMaxTranscriptChars] = '\0';
    TEST_ASSERT_EQUAL(MatchOutcome::MATCHED, CommandMatcher::match(at_limit).outcome);

    char over_limit[matcher_limits::kMaxTranscriptChars + 2];
    memset(over_limit, ' ', sizeof(over_limit));
    memcpy(over_limit, "next", 4);
    over_limit[matcher_limits::kMaxTranscriptChars + 1] = '\0';
    TEST_ASSERT_EQUAL(MatchOutcome::UNRECOGNIZED, CommandMatcher::match(over_limit).outcome);

    // 16 words is accepted; the 17th word is rejected.
    TEST_ASSERT_EQUAL(MatchOutcome::MATCHED, CommandMatcher::match(
        "please please please please please please please please "
        "please please please please please please please next").outcome);
    TEST_ASSERT_EQUAL(MatchOutcome::UNRECOGNIZED, CommandMatcher::match(
        "please please please please please please please please "
        "please please please please please please please please next").outcome);

    // A single word longer than the token buffer is rejected.
    TEST_ASSERT_EQUAL(MatchOutcome::UNRECOGNIZED,
                      CommandMatcher::match("nextnextnextnextnextnextnext").outcome);
}

void test_edit_distance() {
    TEST_ASSERT_EQUAL_UINT8(0, CommandMatcher::edit_distance("pause", "pause"));
    TEST_ASSERT_EQUAL_UINT8(1, CommandMatcher::edit_distance("volum", "volume"));
    TEST_ASSERT_EQUAL_UINT8(1, CommandMatcher::edit_distance("previos", "previous"));
    TEST_ASSERT_EQUAL_UINT8(2, CommandMatcher::edit_distance("pervious", "previous"));
    TEST_ASSERT_EQUAL_UINT8(4, CommandMatcher::edit_distance("play", "pause"));
    TEST_ASSERT_EQUAL_UINT8(4, CommandMatcher::edit_distance("", "next"));
    TEST_ASSERT_EQUAL_UINT8(255, CommandMatcher::edit_distance("aaaaaaaaaaaaaaaaaaaaaaaaa", "a"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_ASR_02_exact_phrases_map_to_all_7_commands);
    RUN_TEST(test_TC_ASR_02_user_story_turn_it_up);
    RUN_TEST(test_TC_ASR_02_near_synonyms_for_every_command);
    RUN_TEST(test_case_punctuation_and_whitespace_are_ignored);
    RUN_TEST(test_near_match_corrects_misrecognised_words);
    RUN_TEST(test_near_match_does_not_guess);
    RUN_TEST(test_consistent_with_command_mapper_synonyms);
    RUN_TEST(test_unsupported_or_empty_transcripts_are_unrecognized);
    RUN_TEST(test_conflicting_commands_are_ambiguous);
    RUN_TEST(test_repeated_same_command_is_still_one_match);
    RUN_TEST(test_input_limits_reject_rather_than_truncate);
    RUN_TEST(test_edit_distance);
    return UNITY_END();
}
