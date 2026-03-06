#include "timeline_node.h"

TimelineNode::TimelineNode() = default;

TimelineNode::TimelineNode(const std::string& name, NodeType type) : name_(name), type_(type) {}

TimelineNode::~TimelineNode()
{
    for (TimelineNode* child : children_) {
        delete child;
    }
}

TimelineNode* TimelineNode::parent()
{
    return parent_;
}

const TimelineNode* TimelineNode::parent() const
{
    return parent_;
}

std::vector<TimelineNode*> TimelineNode::children()
{
    return children_;
}

std::vector<TimelineNode*> TimelineNode::children() const
{
    return children_;
}

TimelineNode* TimelineNode::childAt(int index)
{
    if (index < 0 || index >= static_cast<int>(children_.size())) {
        return nullptr;
    }
    return children_[index];
}

const TimelineNode* TimelineNode::childAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(children_.size())) {
        return nullptr;
    }
    return children_[index];
}

int TimelineNode::childCount() const
{
    return static_cast<int>(children_.size());
}

int TimelineNode::visibleChildCount() const
{
    int count = 0;
    for (const TimelineNode* child : children_) {
        if (child->isVisible()) {
            ++count;
        }
    }
    return count;
}

TimelineNode* TimelineNode::visibleChildAt(int index)
{
    if (index < 0) {
        return nullptr;
    }

    int current = 0;
    for (TimelineNode* child : children_) {
        if (!child->isVisible()) {
            continue;
        }
        if (current == index) {
            return child;
        }
        ++current;
    }
    return nullptr;
}

const TimelineNode* TimelineNode::visibleChildAt(int index) const
{
    if (index < 0) {
        return nullptr;
    }

    int current = 0;
    for (const TimelineNode* child : children_) {
        if (!child->isVisible()) {
            continue;
        }
        if (current == index) {
            return child;
        }
        ++current;
    }
    return nullptr;
}

int TimelineNode::visibleChildRow(const TimelineNode* node) const
{
    if (node == nullptr) {
        return -1;
    }

    int row = 0;
    for (const TimelineNode* child : children_) {
        if (!child->isVisible()) {
            continue;
        }
        if (child == node) {
            return row;
        }
        ++row;
    }
    return -1;
}

int TimelineNode::rowInParent() const
{
    if (parent_ == nullptr) {
        return 0;
    }
    return parent_->visibleChildRow(this);
}

void TimelineNode::addChild(TimelineNode* node)
{
    if (node == nullptr) {
        return;
    }
    node->parent_ = this;
    children_.push_back(node);
}

void TimelineNode::setName(std::string&& name)
{
    name_ = std::move(name);
}

const std::string& TimelineNode::name() const
{
    return name_;
}

const std::vector<BaseEvent*>& TimelineNode::events() const
{
    return events_;
}

void TimelineNode::addEvent(BaseEvent* event)
{
    if (event == nullptr) {
        return;
    }
    events_.push_back(event);
}

void TimelineNode::addEvents(const std::vector<BaseEvent*>& events)
{
    for (BaseEvent* event : events) {
        addEvent(event);
    }
}

void TimelineNode::foreachNode(const std::function<void(TimelineNode*)>& func)
{
    func(this);
    for (TimelineNode* child : children_) {
        child->foreachNode(func);
    }
}

bool TimelineNode::isExpand() const
{
    return expand_;
}

void TimelineNode::setExpand(bool expand)
{
    expand_ = expand;
}

bool TimelineNode::isVisible() const
{
    return visible_;
}

void TimelineNode::setVisible(bool v)
{
    visible_ = v;
}

void TimelineNode::sortEvents()
{
    std::sort(events_.begin(), events_.end(),
              [](const BaseEvent* lhs, const BaseEvent* rhs) { return lhs->start < rhs->start; });

    // set max value after sorted
    double maxValue = 0.0;
    uint64_t minTime = std::numeric_limits<uint64_t>::max();
    uint64_t maxTime = 0;
    for (const BaseEvent* event : events_) {
        const CounterTimelineEvent* counterEvent = dynamic_cast<const CounterTimelineEvent*>(event);
        if (type_ == TIMELINE_COUNTER && counterEvent) {
            maxValue = std::max(maxValue, counterEvent->value);
        }
        minTime = std::min(minTime, event->start);
        maxTime = std::max(maxTime, event->start);
    }
    if (maxValue <= 0.0) {
        maxValue = 1.0;
    }
    setMaxCounterValue(maxValue);

    minTime_ = minTime;
    maxTime_ = maxTime;
}

uint64_t TimelineNode::minTime() const
{
    return minTime_;
}

uint64_t TimelineNode::maxTime() const
{
    return maxTime_;
}

void TimelineNode::setMaxCounterValue(double maxValue)
{
    maxCounterValue_ = maxValue;
}

double TimelineNode::maxCounterValue() const
{
    return maxCounterValue_;
}
