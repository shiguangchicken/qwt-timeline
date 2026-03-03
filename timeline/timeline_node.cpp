#include "timeline_node.h"

TimelineNode::TimelineNode() = default;

TimelineNode::TimelineNode(const std::string& name, NodeType type) : name_(name), type_(type) {}

TimelineNode::~TimelineNode()
{
    for (TimelineNode* child : children_) {
        delete child;
    }
    for (TimelineEvent* event : events_) {
        delete event;
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

const std::vector<TimelineEvent*>& TimelineNode::events() const
{
    return events_;
}

void TimelineNode::addEvent(TimelineEvent* event)
{
    if (event == nullptr) {
        return;
    }
    if (!events_.empty()) {
        TimelineEvent* prev = events_.back();
        prev->next = event;
        event->pre = prev;
    }
    events_.push_back(event);
}

void TimelineNode::addEvents(const std::vector<TimelineEvent*>& events)
{
    for (TimelineEvent* event : events) {
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
              [](const TimelineEvent* lhs, const TimelineEvent* rhs) { return lhs->start < rhs->start; });

    for (std::size_t i = 0; i < events_.size(); ++i) {
        TimelineEvent* current = events_[i];
        current->pre = (i == 0) ? nullptr : events_[i - 1];
        current->next = (i + 1 < events_.size()) ? events_[i + 1] : nullptr;
    }

    for (TimelineNode* child : children_) {
        child->sortEvents();
    }
}

uint64_t TimelineNode::minTime() const
{
    uint64_t minValue = std::numeric_limits<uint64_t>::max();
    bool hasValue = false;

    for (const TimelineEvent* event : events_) {
        minValue = std::min(minValue, event->start);
        hasValue = true;
    }

    for (const TimelineNode* child : children_) {
        const uint64_t childMin = child->minTime();
        if (childMin != std::numeric_limits<uint64_t>::max()) {
            minValue = std::min(minValue, childMin);
            hasValue = true;
        }
    }

    return hasValue ? minValue : 0;
}

uint64_t TimelineNode::maxTime() const
{
    uint64_t maxValue = 0;

    for (const TimelineEvent* event : events_) {
        maxValue = std::max(maxValue, event->end);
    }

    for (const TimelineNode* child : children_) {
        maxValue = std::max(maxValue, child->maxTime());
    }

    return maxValue;
}

void TimelineNode::setMaxCounterValue(double maxValue)
{
    maxCounterValue_ = maxValue;
}

double TimelineNode::maxCounterValue() const
{
    return maxCounterValue_;
}
