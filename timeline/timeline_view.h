#pragma once

#include "timeline_node_model.h"
#include "timeline_widget.h"

#include <QWidget>

class QTreeView;
class QScrollBar;

class TimelineView : public QWidget {
    Q_OBJECT

public:
    explicit TimelineView(QWidget* parent = nullptr);

    void setRootNode(TimelineNode* rootNode);
    TimelineNode* rootNode() const;

private:
    void syncVerticalScrollFromTree();
    void syncTreeFromVerticalScroll(int value);
    void configureHorizontalRange();

private:
    QTreeView* node_view_ = nullptr;
    TimelineNodeModel* node_model_ = nullptr;
    TimelineWidget* timeline_widget_ = nullptr;
    QScrollBar* vertical_scroll_bar_ = nullptr;
    QScrollBar* horizontal_scroll_bar_ = nullptr;

    TimelineNode* rootNode_ = nullptr;
    uint64_t fullMin_ = 0;
    uint64_t fullMax_ = 0;
    uint64_t windowSize_ = 1000;
};
