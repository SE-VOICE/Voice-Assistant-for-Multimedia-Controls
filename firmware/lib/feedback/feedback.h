#pragma once
#include <stdint.h>
namespace vamc {
enum class FeedbackState : uint8_t {
    LISTENING, RECOGNIZED, SUCCESS, ERROR, LOW_BATTERY, MUTED
};
class Feedback {
public:
    virtual ~Feedback() = default;
    virtual void show(FeedbackState state) = 0;
};
} // namespace vamc
