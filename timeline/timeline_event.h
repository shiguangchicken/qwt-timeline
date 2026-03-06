#pragma once

#include <cstdint>
#include <string_view>

#include "theme.h"

enum class EventType {
    Timeline,
    Counter,
};

struct BaseEvent {
    virtual ~BaseEvent() = default;
    std::string_view name;
    uint64_t start = 0;
    uint64_t end = 0;
    uint32_t color = timeline_color::DEFAULT;

    uint64_t duration() const { return end - start; }
    virtual EventType type() const = 0;
};

struct TimelineEvent : public BaseEvent {
    TimelineEvent* pre = nullptr;
    TimelineEvent* next = nullptr;

    void setRelationPre(TimelineEvent* preEvent)
    {
        TimelineEvent* temp = this;
        while (temp->pre != nullptr) {
            temp = temp->pre;
        }
        if (preEvent != nullptr) {
            temp->pre = preEvent;
        }
    }
    void setRelationNext(TimelineEvent* nextEvent)
    {
        TimelineEvent* temp = this;
        while (temp->next != nullptr) {
            temp = temp->next;
        }
        if (nextEvent != nullptr) {
            temp->next = nextEvent;
        }
    }
    virtual EventType type() const override { return EventType::Timeline; }
};

struct CounterTimelineEvent : public TimelineEvent {
    double value = 0.0;
    virtual EventType type() const override { return EventType::Counter; }
};
