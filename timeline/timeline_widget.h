#pragma once

#include <QPersistentModelIndex>
#include <QPoint>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "timeline_node_model.h"

class QTreeView;
class QwtPlot;
class QMouseEvent;
class QWheelEvent;
class QEvent;
class TimelineNode;
struct TimelineEvent;

class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget* parent = nullptr);

    void init(TimelineNodeModel* model, QTreeView* tree);
    void setRowHeight(int rowHeight);
    void setFullRange(uint64_t start, uint64_t end);
    void setTimeRange(uint64_t start, uint64_t end);
    void draw();

    uint64_t timeRangeStart() const;
    uint64_t timeRangeEnd() const;

signals:
    void timeRangeChanged(uint64_t start, uint64_t end);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool handleWheelEvent(QWheelEvent* wheelEvent);
    void scheduleMouseTrackerUpdate(const QPoint& pos);
    void flushMouseTracker();
    void applyZoomAround(double anchorTime, double zoomFactor);
    void updateSelectionFromPosition(const QPoint& pos);
    std::vector<const TimelineEvent*> collectEventsInView(TimelineNode* node) const;

    class TimelinePlotItem;

    QwtPlot* plot_ = nullptr;
    TimelinePlotItem* plotItem_ = nullptr;
    TimelineNodeModel* model_ = nullptr;
    QTreeView* tree_ = nullptr;
    uint64_t visibleStart_ = 0;
    uint64_t visibleEnd_ = 1000;
    uint64_t fullStart_ = 0;
    uint64_t fullEnd_ = 1000;
    int rowHeight_ = 60;

    QTimer mouseTrackerTimer_;
    QPoint pendingMousePos_;
    bool hasPendingMousePos_ = false;
    bool showMouseTracker_ = false;
    double mouseTrackerTime_ = 0.0;

    QPersistentModelIndex selectedRowIndex_;
    const TimelineEvent* selectedEvent_ = nullptr;
    bool hasSelectedEvent_ = false;
};
