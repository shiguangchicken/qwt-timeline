#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "timeline_event.h"

class TimelineNode {
public:
    enum NodeType {
        TIMELINE,
        TIMELINE_AREA,
        TIMELINE_COUNTER,
    };

    TimelineNode();
    virtual ~TimelineNode();
    TimelineNode(const std::string &name, NodeType type = TIMELINE);

    NodeType type() const { return type_; }

    TimelineNode *parent();

    const TimelineNode *parent() const;

    std::vector<TimelineNode *> children();

    std::vector<TimelineNode *> children() const;

    TimelineNode *childAt(int index);

    const TimelineNode *childAt(int index) const;

    int childCount() const;

    int visibleChildCount() const;

    TimelineNode *visibleChildAt(int index);

    const TimelineNode *visibleChildAt(int index) const;

    int visibleChildRow(const TimelineNode *node) const;

    int rowInParent() const;

    void addChild(TimelineNode *node);

    void setName(std::string &&name);

    const std::string &name() const;

    const std::vector<BaseEvent *> &events() const;

    void addEvent(BaseEvent *event);

    void addEvents(const std::vector<BaseEvent *> &events);

    void foreachNode(const std::function<void(TimelineNode *)> &func);

    bool isExpand() const;

    void setExpand(bool expand);

    bool isVisible() const;

    void setVisible(bool v);

    void sortEvents();

    void setMaxCounterValue(double maxValue);
    double maxCounterValue() const;

    uint64_t minTime() const;
    uint64_t maxTime() const;

private:
    TimelineNode *parent_ = nullptr;
    std::vector<TimelineNode *> children_;
    std::vector<BaseEvent *> events_;
    std::string name_;
    NodeType type_ = TIMELINE;
    bool expand_ = true;
    bool visible_ = true;
    double maxCounterValue_ = 0.0;
};
