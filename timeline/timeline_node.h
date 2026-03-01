#pragma once

#include "timeline_event.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

class TimelineNode {
public:
    enum NodeType {
        TIMELINE,
        TIMELINE_AREA,
    };

    TimelineNode();
    virtual ~TimelineNode();
    TimelineNode(const std::string& name, NodeType type = TIMELINE);

    TimelineNode* parent();
    const TimelineNode* parent() const;
    std::vector<TimelineNode*> children();
    std::vector<TimelineNode*> children() const;
    TimelineNode* childAt(int index);
    const TimelineNode* childAt(int index) const;
    int childCount() const;
    int visibleChildCount() const;
    TimelineNode* visibleChildAt(int index);
    const TimelineNode* visibleChildAt(int index) const;
    int visibleChildRow(const TimelineNode* node) const;
    int rowInParent() const;

    void addChild(TimelineNode* node);

    void setName(std::string&& name);
    const std::string& name() const;

    const std::vector<TimelineEvent*>& events() const;
    void addEvent(TimelineEvent* event);
    void addEvents(const std::vector<TimelineEvent*>& events);

    void foreachNode(const std::function<void(TimelineNode*)>& func);

    bool isExpand() const;
    void setExpand(bool expand);

    bool isVisible() const;
    void setVisible(bool v);

    void sortEvents();

    uint64_t minTime() const;
    uint64_t maxTime() const;

private:
    TimelineNode* parent_ = nullptr;
    std::vector<TimelineNode*> children_;
    std::vector<TimelineEvent*> events_;
    std::string name_;
    NodeType type_ = TIMELINE;
    bool expand_ = true;
    bool visible_ = true;
};
