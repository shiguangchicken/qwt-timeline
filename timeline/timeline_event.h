#pragma once

#include <cstdint>
#include <string_view>

#include "theme.h"

struct TimelineEvent {
    virtual ~TimelineEvent() = default;
    std::string_view name;
    uint64_t start = 0;
    uint64_t end = 0;
    uint32_t color = timeline_color::DEFAULT;

    TimelineEvent *pre = nullptr;
    TimelineEvent *next = nullptr;

    uint64_t duration() const { return end - start; }
};

struct CounterTimelineEvent : public TimelineEvent {
    double value = 0.0;
};
