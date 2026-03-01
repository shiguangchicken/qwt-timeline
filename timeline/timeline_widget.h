#pragma once

#include "timeline_node_model.h"

#include <QWidget>

class QTreeView;
class QwtPlot;

class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget* parent = nullptr);

    void init(TimelineNodeModel* model, QTreeView* tree);
    void setTimeRange(uint64_t start, uint64_t end);
    void draw();

    uint64_t timeRangeStart() const;
    uint64_t timeRangeEnd() const;

private:
    class TimelinePlotItem;

    QwtPlot* plot_ = nullptr;
    TimelinePlotItem* plotItem_ = nullptr;
    TimelineNodeModel* model_ = nullptr;
    QTreeView* tree_ = nullptr;
    uint64_t visibleStart_ = 0;
    uint64_t visibleEnd_ = 1000;
    int rowHeight_ = 24;
};
