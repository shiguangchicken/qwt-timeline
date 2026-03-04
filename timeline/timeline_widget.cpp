#include "timeline_widget.h"

#include <QBoxLayout>
#include <QColor>
#include <QEvent>
#include <QHash>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScrollBar>
#include <QToolTip>
#include <QTreeView>
#include <QVector>
#include <QWheelEvent>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_item.h>
#include <qwt_plot_opengl_canvas.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <vector>

#include "theme.h"

class TimelineScaleDraw : public QwtScaleDraw {
public:
    virtual double extent(const QFont& f) const override { return QwtScaleDraw::extent(f); }
    virtual void drawTick(QPainter* p, double v, double l) const override { QwtScaleDraw::drawTick(p, v, l); }
    virtual void drawBackbone(QPainter* p) const override { QwtScaleDraw::drawBackbone(p); }
    virtual void drawLabel(QPainter* p, double v) const override { QwtScaleDraw::drawLabel(p, v); }
    void draw(QPainter* painter, const QPalette& palette) const override
    {
        QwtScaleDraw::draw(painter, palette);
        if (!showMouseText_) {
            return;
        }

        uint64_t factor = 1;
        const char* unit = getTimeUnit(static_cast<int64_t>(mouseTime_), factor);
        const QString text = QString::number(mouseTime_ / static_cast<double>(factor)) + " " + unit;

        painter->save();
        painter->setPen(QPen(QColor::fromRgba(timeline_color::CURRENT_TIME), 1));
        const QPointF axisPos = QwtScaleDraw::pos();
        const int baselineY = static_cast<int>(axisPos.y() + painter->fontMetrics().height() - 2);
        painter->drawText(QPointF(mouseX_, baselineY), text);
        painter->restore();
    }

    void setMouseTracker(bool showMouseText, double mouseTime, double mouseX)
    {
        showMouseText_ = showMouseText;
        mouseTime_ = mouseTime;
        mouseX_ = mouseX;
    }

public:
    TimelineScaleDraw() = default;

    QwtText label(double value) const override
    {
        uint64_t factor = 1;
        const char* unit = getTimeUnit(static_cast<int64_t>(value), factor);
        QString labelStr = QString::number(value / static_cast<double>(factor));
        return QwtText(labelStr + " " + unit);
    }

    static const char* getTimeUnit(int64_t time, uint64_t& factor)
    {
        uint64_t value = std::abs(time);
        if (value < 1000) {
            factor = 1;
            return "ns";
        } else if (value < 1000 * 1000) {
            factor = 1000;
            return "us";
        } else if (value < 1000 * 1000 * 1000) {
            factor = 1000 * 1000;
            return "ms";
        } else {
            factor = 1000 * 1000 * 1000;
            return "s";
        }
    }

private:
    bool showMouseText_ = false;
    double mouseTime_ = 0.0;
    double mouseX_ = 0.0;
};

class TimelineWidget::TimelinePlotItem : public QwtPlotItem {
public:
    explicit TimelinePlotItem(TimelineWidget* owner) : owner_(owner) { setZ(20.0); }

    void draw(QPainter* painter,
              const QwtScaleMap& xMap,
              const QwtScaleMap& yMap,
              const QRectF& canvasRect) const override
    {
        Q_UNUSED(canvasRect);
        if (owner_ == nullptr || owner_->model_ == nullptr || owner_->tree_ == nullptr) {
            return;
        }

        struct RowNode {
            TimelineNode* node = nullptr;
            QModelIndex index;
        };

        std::vector<RowNode> visibleRows;
        visibleRows.reserve(128);

        std::function<void(const QModelIndex&, int&)> collect = [&](const QModelIndex& parentIndex, int& rowCounter) {
            const int rows = owner_->model_->rowCount(parentIndex);
            for (int r = 0; r < rows; ++r) {
                const QModelIndex idx = owner_->model_->index(r, 0, parentIndex);
                auto* node = owner_->model_->nodeFromIndex(idx);
                if (node != nullptr) {
                    visibleRows.push_back({node, idx});
                    ++rowCounter;
                    if (owner_->tree_->isExpanded(idx)) {
                        collect(idx, rowCounter);
                    }
                }
            }
        };

        int rowCounter = 0;
        collect(QModelIndex(), rowCounter);

        for (const auto& item : visibleRows) {
            if (item.node->type() == TimelineNode::TIMELINE_COUNTER) {
                double maxValue = 0.0;
                for (const TimelineEvent* event : item.node->events()) {
                    if (const CounterTimelineEvent* counterEvent = dynamic_cast<const CounterTimelineEvent*>(event)) {
                        maxValue = std::max(maxValue, counterEvent->value);
                    }
                }
                if (maxValue <= 0.0) {
                    maxValue = 1.0;
                }
                item.node->setMaxCounterValue(maxValue);
            }
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);

        const QColor rowLineColor = QColor::fromRgba(timeline_color::ROW_LINE);
        painter->setPen(QPen(rowLineColor, 1));
        for (const auto& item : visibleRows) {
            const QRect rowRect = owner_->tree_->visualRect(item.index);
            if (!rowRect.isValid()) {
                continue;
            }
            const double yLine = static_cast<double>(rowRect.bottom() + 1);
            painter->drawLine(QPointF(xMap.transform(static_cast<double>(owner_->visibleStart_)), yLine),
                              QPointF(xMap.transform(static_cast<double>(owner_->visibleEnd_)), yLine));
        }

        QHash<uint32_t, QVector<QRectF>> fillRectsByColor;
        QVector<QRectF> selectedFillRects;
        QVector<QLineF> selectedBoundaryLines;

        static constexpr size_t kMaxDrawnEventsInView = 1000;
        auto collectEventsInView = [&](TimelineNode* node) {
            std::vector<const TimelineEvent*> eventsToDraw;
            if (node == nullptr) {
                return eventsToDraw;
            }

            const auto& events = node->events();
            if (events.empty()) {
                return eventsToDraw;
            }

            auto firstVisibleIt = std::lower_bound(
                events.begin(), events.end(), owner_->visibleStart_,
                [](const TimelineEvent* event, uint64_t visibleStart) { return event->start < visibleStart; });
            size_t startIndex = static_cast<size_t>(std::distance(events.begin(), firstVisibleIt));
            while (startIndex > 0 && events[startIndex - 1]->end >= owner_->visibleStart_) {
                --startIndex;
            }

            auto endVisibleIt = std::upper_bound(
                events.begin() + static_cast<std::ptrdiff_t>(startIndex), events.end(), owner_->visibleEnd_,
                [](uint64_t visibleEnd, const TimelineEvent* event) { return visibleEnd < event->start; });
            const size_t endIndex = static_cast<size_t>(std::distance(events.begin(), endVisibleIt));

            if (startIndex >= endIndex) {
                return eventsToDraw;
            }

            const size_t visibleCount = endIndex - startIndex;
            eventsToDraw.reserve(std::min(visibleCount, kMaxDrawnEventsInView));

            if (visibleCount <= kMaxDrawnEventsInView) {
                eventsToDraw.insert(eventsToDraw.end(), events.begin() + static_cast<std::ptrdiff_t>(startIndex),
                                    events.begin() + static_cast<std::ptrdiff_t>(endIndex));
                return eventsToDraw;
            }

            for (size_t i = 0; i < kMaxDrawnEventsInView; ++i) {
                const size_t sampledOffset = (i * (visibleCount - 1)) / (kMaxDrawnEventsInView - 1);
                eventsToDraw.push_back(events[startIndex + sampledOffset]);
            }
            return eventsToDraw;
        };

        std::vector<std::vector<const TimelineEvent*>> rowEventsToDraw;
        rowEventsToDraw.reserve(visibleRows.size());
        for (const auto& item : visibleRows) {
            rowEventsToDraw.push_back(collectEventsInView(item.node));
        }

        for (size_t rowIdx = 0; rowIdx < visibleRows.size(); ++rowIdx) {
            const auto& item = visibleRows[rowIdx];
            const QRect rowRect = owner_->tree_->visualRect(item.index);
            if (!rowRect.isValid()) {
                continue;
            }

            const int barTop = rowRect.top() + 1;
            const int barBottom = rowRect.bottom();
            if (barBottom <= barTop) {
                continue;
            }

            for (const TimelineEvent* event : rowEventsToDraw[rowIdx]) {
                const uint64_t clippedStart = std::max(event->start, owner_->visibleStart_);
                const uint64_t clippedEnd = std::min(event->end, owner_->visibleEnd_);
                const double left = xMap.transform(static_cast<double>(clippedStart));
                double right = xMap.transform(static_cast<double>(clippedEnd));
                // if the event is too narrow to be visible, we still want to show it with a minimum width
                if (right - left < 1.0) {
                    right = left + 1.0;
                }
                const double rowTop = static_cast<double>(barTop);
                const double rowBottom = static_cast<double>(barBottom);
                double rectTop = rowTop;
                double rectBottom = rowBottom;
                if (item.node->type() == TimelineNode::TIMELINE_COUNTER) {
                    const double nodeHeight = std::max(0.0, rowBottom - rowTop);
                    const double maxValue = item.node->maxCounterValue();
                    if (nodeHeight > 0.0 && maxValue > 0.0) {
                        if (const CounterTimelineEvent* counterEvent =
                                dynamic_cast<const CounterTimelineEvent*>(event)) {
                            const double heightRatio = std::clamp(counterEvent->value / maxValue, 0.0, 1.0);
                            const double height = std::clamp(heightRatio * nodeHeight, 0.0, nodeHeight);
                            rectBottom = rowBottom;
                            rectTop = rowBottom - height;
                        }
                    }
                }

                QRectF rect(QPointF(left, rectTop), QPointF(right, rectBottom));
                rect = rect.normalized();

                const bool isSelected = owner_->hasSelectedEvent_ && owner_->selectedRowIndex_.isValid() &&
                                        owner_->selectedRowIndex_ == item.index && owner_->selectedEvent_ == event;

                if (isSelected) {
                    selectedFillRects.push_back(rect);
                    const double yTop = canvasRect.top();
                    const double yBottom = canvasRect.bottom();
                    selectedBoundaryLines.push_back(QLineF(QPointF(rect.left(), yTop), QPointF(rect.left(), yBottom)));
                    selectedBoundaryLines.push_back(
                        QLineF(QPointF(rect.right(), yTop), QPointF(rect.right(), yBottom)));
                } else {
                    fillRectsByColor[event->color].push_back(rect);
                }
            }
        }

        painter->setPen(Qt::NoPen);
        for (auto it = fillRectsByColor.constBegin(); it != fillRectsByColor.constEnd(); ++it) {
            const QColor fillColor = QColor::fromRgba(it.key());
            painter->setBrush(fillColor);
            const QVector<QRectF>& rects = it.value();
            if (!rects.isEmpty()) {
                painter->drawRects(rects.constData(), rects.size());
            }
        }

        if (!selectedFillRects.isEmpty()) {
            painter->setBrush(QColor::fromRgba(timeline_color::HIGHLIGHT));
            painter->drawRects(selectedFillRects.constData(), selectedFillRects.size());
        }

        // Draw event names
        painter->setPen(Qt::white);
        for (size_t rowIdx = 0; rowIdx < visibleRows.size(); ++rowIdx) {
            const auto& item = visibleRows[rowIdx];
            const QRect rowRect = owner_->tree_->visualRect(item.index);
            if (!rowRect.isValid())
                continue;
            const int barTop = rowRect.top() + 1;
            const int barBottom = rowRect.bottom();
            for (const TimelineEvent* event : rowEventsToDraw[rowIdx]) {
                const uint64_t clippedStart = std::max(event->start, owner_->visibleStart_);
                const uint64_t clippedEnd = std::min(event->end, owner_->visibleEnd_);
                const double left = xMap.transform(static_cast<double>(clippedStart));
                const double right = xMap.transform(static_cast<double>(clippedEnd));
                QRectF rect(QPointF(left, static_cast<double>(barTop)), QPointF(right, static_cast<double>(barBottom)));
                if (rect.width() > 10) {
                    QString name = QString::fromUtf8(event->name.data(), static_cast<int>(event->name.size()));
                    if (name.isEmpty()) {
                        continue;
                    }
                    painter->drawText(
                        rect, Qt::AlignCenter,
                        painter->fontMetrics().elidedText(name, Qt::ElideRight, static_cast<int>(rect.width())));
                }
            }
        }

        if (!selectedBoundaryLines.isEmpty()) {
            painter->setPen(QPen(QColor::fromRgba(timeline_color::HIGHLIGHT), 1));
            painter->drawLines(selectedBoundaryLines.constData(), selectedBoundaryLines.size());
        }

        if (owner_->showMouseTracker_) {
            const double x = xMap.transform(owner_->mouseTrackerTime_);
            const double yTop = canvasRect.top();
            const double yBottom = canvasRect.bottom();

            painter->setPen(QPen(QColor::fromRgba(timeline_color::HIGHLIGHT), 1));
            painter->drawLine(QPointF(x, yTop), QPointF(x, yBottom));
        }

        painter->restore();
    }

private:
    TimelineWidget* owner_ = nullptr;
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent), plot_(new QwtPlot(this)), plotItem_(new TimelinePlotItem(this))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot_);

    plot_->setAutoReplot(false);
    plot_->setCanvasBackground(QColor::fromRgba(timeline_color::DEEP_GRAY));
    auto* glCanvas = new QwtPlotOpenGLCanvas();
    plot_->setCanvas(glCanvas);

    auto* grid = new QwtPlotGrid();
    grid->setMajorPen(QPen(QColor::fromRgba(timeline_color::ROW_LINE), 1, Qt::DotLine));
    grid->enableY(false);
    grid->attach(plot_);

    plot_->setAxisScaleDraw(QwtPlot::xBottom, new TimelineScaleDraw());

    plot_->setMouseTracking(true);
    plot_->canvas()->setMouseTracking(true);
    plot_->installEventFilter(this);
    plot_->canvas()->installEventFilter(this);

    plot_->enableAxis(QwtPlot::yLeft, false);
    plot_->enableAxis(QwtPlot::xBottom, true);
    plot_->setAxisScale(QwtPlot::xBottom, static_cast<double>(visibleStart_), static_cast<double>(visibleEnd_));
    plot_->setAxisScale(QwtPlot::yLeft, 500.0, 0.0);

    plotItem_->attach(plot_);

    mouseTrackerTimer_.setInterval(40);
    mouseTrackerTimer_.setSingleShot(true);
    connect(&mouseTrackerTimer_, &QTimer::timeout, this, &TimelineWidget::flushMouseTracker);
}

void TimelineWidget::init(TimelineNodeModel* model, QTreeView* tree)
{
    model_ = model;
    tree_ = tree;

    if (model_ != nullptr) {
        connect(model_, &QAbstractItemModel::modelReset, this, &TimelineWidget::draw);
        connect(model_, &QAbstractItemModel::layoutChanged, this, &TimelineWidget::draw);
        connect(model_, &QAbstractItemModel::rowsInserted, this, &TimelineWidget::draw);
        connect(model_, &QAbstractItemModel::rowsRemoved, this, &TimelineWidget::draw);
    }

    if (tree_ != nullptr) {
        connect(tree_, &QTreeView::expanded, this, [this](const QModelIndex&) { draw(); });
        connect(tree_, &QTreeView::collapsed, this, [this](const QModelIndex&) { draw(); });
        if (tree_->verticalScrollBar() != nullptr) {
            connect(tree_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) { draw(); });
        }
    }

    draw();
}

void TimelineWidget::setRowHeight(int rowHeight)
{
    rowHeight_ = std::max(1, rowHeight);
    draw();
}

void TimelineWidget::setFullRange(uint64_t start, uint64_t end)
{
    if (end <= start) {
        end = start + 1;
    }
    fullStart_ = start;
    fullEnd_ = end;
    setTimeRange(visibleStart_, visibleEnd_);
}

void TimelineWidget::setTimeRange(uint64_t start, uint64_t end)
{
    if (end <= start) {
        end = start + 1;
    }

    if (fullEnd_ <= fullStart_) {
        fullEnd_ = fullStart_ + 1;
    }

    const uint64_t fullSpan = fullEnd_ - fullStart_;
    uint64_t span = end - start;
    span = std::max<uint64_t>(1, std::min<uint64_t>(span, fullSpan));

    start = std::max(start, fullStart_);
    if (start + span > fullEnd_) {
        start = fullEnd_ - span;
    }
    end = start + span;

    if (visibleStart_ == start && visibleEnd_ == end) {
        return;
    }

    visibleStart_ = start;
    visibleEnd_ = end;
    draw();
    emit timeRangeChanged(visibleStart_, visibleEnd_);
}

void TimelineWidget::draw()
{
    int visibleCount = 0;
    if (model_ != nullptr && tree_ != nullptr) {
        std::function<void(const QModelIndex&)> countVisible = [&](const QModelIndex& parentIndex) {
            const int rows = model_->rowCount(parentIndex);
            for (int i = 0; i < rows; ++i) {
                const QModelIndex idx = model_->index(i, 0, parentIndex);
                ++visibleCount;
                if (tree_->isExpanded(idx)) {
                    countVisible(idx);
                }
            }
        };
        countVisible(QModelIndex());
    }

    const int plotHeight = std::max(visibleCount, 1) * rowHeight_;
    plot_->setAxisScale(QwtPlot::xBottom, static_cast<double>(visibleStart_), static_cast<double>(visibleEnd_));
    plot_->setAxisScale(QwtPlot::yLeft, static_cast<double>(plotHeight), 0.0);

    if (auto* scaleDraw = dynamic_cast<TimelineScaleDraw*>(plot_->axisScaleDraw(QwtPlot::xBottom))) {
        const double mouseX = plot_->transform(QwtPlot::xBottom, mouseTrackerTime_);
        scaleDraw->setMouseTracker(showMouseTracker_, mouseTrackerTime_, mouseX);
    }

    plot_->axisWidget(QwtPlot::xBottom)->update();
    plot_->replot();
}

uint64_t TimelineWidget::timeRangeStart() const
{
    return visibleStart_;
}

uint64_t TimelineWidget::timeRangeEnd() const
{
    return visibleEnd_;
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    scheduleMouseTrackerUpdate(event->position().toPoint());
    QWidget::mouseMoveEvent(event);
}

bool TimelineWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != plot_->canvas() && watched != plot_) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        scheduleMouseTrackerUpdate(mouseEvent->position().toPoint());
        return false;
    }

    if (event->type() == QEvent::Leave) {
        showMouseTracker_ = false;
        hasPendingMousePos_ = false;
        mouseTrackerTimer_.stop();
        draw();
        return false;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            updateSelectionFromPosition(mouseEvent->position().toPoint());
            mouseEvent->accept();
            return true;
        }
        return false;
    }

    if (event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        return handleWheelEvent(wheelEvent);
    }

    return QWidget::eventFilter(watched, event);
}

bool TimelineWidget::handleWheelEvent(QWheelEvent* wheelEvent)
{
    if (wheelEvent == nullptr) {
        return false;
    }

    int delta = wheelEvent->angleDelta().y();
    if (delta == 0) {
        delta = wheelEvent->angleDelta().x();
    }
    if (delta == 0) {
        return false;
    }

    if ((wheelEvent->modifiers() & Qt::ControlModifier) != 0) {
        const double anchor = plot_->invTransform(QwtPlot::xBottom, wheelEvent->position().x());
        const double zoomFactor = (delta > 0) ? 0.85 : (1.0 / 0.85);
        applyZoomAround(anchor, zoomFactor);
        wheelEvent->accept();
        return true;
    }

    if ((wheelEvent->modifiers() & Qt::ShiftModifier) == 0) {
        if (tree_ == nullptr || tree_->verticalScrollBar() == nullptr) {
            return false;
        }

        QScrollBar* bar = tree_->verticalScrollBar();
        const int wheelLines = 3;
        const int steps = std::max(1, std::abs(delta) / 120) * wheelLines;
        const int direction = (delta > 0) ? -1 : 1;
        const int offset = direction * steps * std::max(1, bar->singleStep());

        bar->setValue(bar->value() + offset);
        draw();
        wheelEvent->accept();
        return true;
    }

    if (visibleEnd_ <= visibleStart_ || fullEnd_ <= fullStart_) {
        return false;
    }

    const uint64_t span = visibleEnd_ - visibleStart_;
    const uint64_t panStep = std::max<uint64_t>(1, span / 10);
    const int deltaNotches = std::max(1, std::abs(delta) / 120);
    const int direction = (delta > 0) ? -1 : 1;
    const int64_t offset =
        static_cast<int64_t>(direction) * static_cast<int64_t>(deltaNotches) * static_cast<int64_t>(panStep);

    const int64_t minStart = static_cast<int64_t>(fullStart_);
    const int64_t maxStart = static_cast<int64_t>(fullEnd_ - span);
    const int64_t candidate = static_cast<int64_t>(visibleStart_) + offset;
    const int64_t clampedStart = std::clamp(candidate, minStart, maxStart);
    const uint64_t newStart = static_cast<uint64_t>(std::max<int64_t>(0, clampedStart));

    setTimeRange(newStart, newStart + span);
    wheelEvent->accept();
    return true;
}

void TimelineWidget::scheduleMouseTrackerUpdate(const QPoint& pos)
{
    pendingMousePos_ = pos;
    hasPendingMousePos_ = true;
    if (!mouseTrackerTimer_.isActive()) {
        mouseTrackerTimer_.start();
    }
}

void TimelineWidget::flushMouseTracker()
{
    if (!hasPendingMousePos_) {
        return;
    }

    const double time = plot_->invTransform(QwtPlot::xBottom, pendingMousePos_.x());
    mouseTrackerTime_ = std::clamp(time, static_cast<double>(visibleStart_), static_cast<double>(visibleEnd_));
    hasPendingMousePos_ = false;
    showMouseTracker_ = true;

    // Tooltip logic
    if (model_ != nullptr && tree_ != nullptr) {
        bool found = false;
        std::function<void(const QModelIndex&)> visitVisible = [&](const QModelIndex& parentIndex) {
            if (found)
                return;
            const int rows = model_->rowCount(parentIndex);
            for (int i = 0; i < rows; ++i) {
                if (found)
                    return;
                const QModelIndex idx = model_->index(i, 0, parentIndex);
                TimelineNode* node = model_->nodeFromIndex(idx);
                if (node == nullptr)
                    continue;
                const QRect rowRect = tree_->visualRect(idx);
                if (!rowRect.isValid())
                    continue;
                const int barTop = rowRect.top() + 1;
                const int barBottom = rowRect.bottom();
                if (pendingMousePos_.y() >= barTop && pendingMousePos_.y() <= barBottom) {
                    for (const TimelineEvent* event : node->events()) {
                        if (event->end < visibleStart_ || event->start > visibleEnd_)
                            continue;
                        const uint64_t clippedStart = std::max(event->start, visibleStart_);
                        const uint64_t clippedEnd = std::min(event->end, visibleEnd_);
                        const double left = plot_->transform(QwtPlot::xBottom, static_cast<double>(clippedStart));
                        const double right = plot_->transform(QwtPlot::xBottom, static_cast<double>(clippedEnd));
                        if (pendingMousePos_.x() >= left && pendingMousePos_.x() <= right) {
                            QString tip =
                                QString("Name: %1\nStart: %2 ns\nEnd: %3 ns")
                                    .arg(QString::fromUtf8(event->name.data(), static_cast<int>(event->name.size())))
                                    .arg(event->start)
                                    .arg(event->end);
                            QToolTip::showText(plot_->canvas()->mapToGlobal(pendingMousePos_), tip, plot_->canvas());
                            found = true;
                            break;
                        }
                    }
                }
                if (!found && tree_->isExpanded(idx))
                    visitVisible(idx);
            }
        };
        visitVisible(QModelIndex());
        if (!found)
            QToolTip::hideText();
    }

    draw();
}

void TimelineWidget::applyZoomAround(double anchorTime, double zoomFactor)
{
    if (zoomFactor <= 0.0) {
        return;
    }

    const double fullStart = static_cast<double>(fullStart_);
    const double fullEnd = static_cast<double>(fullEnd_);
    const double fullSpan = std::max(1.0, fullEnd - fullStart);

    const double rangeStart = static_cast<double>(visibleStart_);
    const double rangeEnd = static_cast<double>(visibleEnd_);
    const double currentSpan = std::max(1.0, rangeEnd - rangeStart);

    const double minSpan = std::min(10.0, fullSpan);
    const double targetSpan = std::clamp(currentSpan * zoomFactor, minSpan, fullSpan);

    const double clampedAnchor = std::clamp(anchorTime, rangeStart, rangeEnd);
    const double ratio = (currentSpan > 0.0) ? ((clampedAnchor - rangeStart) / currentSpan) : 0.5;

    double newStart = clampedAnchor - ratio * targetSpan;
    double newEnd = newStart + targetSpan;

    if (newStart < fullStart) {
        newStart = fullStart;
        newEnd = newStart + targetSpan;
    }
    if (newEnd > fullEnd) {
        newEnd = fullEnd;
        newStart = newEnd - targetSpan;
    }

    setTimeRange(static_cast<uint64_t>(std::max(0.0, newStart)), static_cast<uint64_t>(std::max(1.0, newEnd)));
}

void TimelineWidget::updateSelectionFromPosition(const QPoint& pos)
{
    if (model_ == nullptr || tree_ == nullptr) {
        return;
    }

    bool found = false;
    QPersistentModelIndex selectedIndex;
    const TimelineEvent* selectedEvent = nullptr;

    std::function<void(const QModelIndex&)> visitVisible = [&](const QModelIndex& parentIndex) {
        if (found) {
            return;
        }

        const int rows = model_->rowCount(parentIndex);
        for (int i = 0; i < rows; ++i) {
            if (found) {
                return;
            }

            const QModelIndex idx = model_->index(i, 0, parentIndex);
            TimelineNode* node = model_->nodeFromIndex(idx);
            if (node == nullptr) {
                continue;
            }

            const QRect rowRect = tree_->visualRect(idx);
            if (!rowRect.isValid()) {
                continue;
            }

            const int barTop = rowRect.top() + 1;
            const int barBottom = rowRect.bottom();
            if (pos.y() < barTop || pos.y() > barBottom) {
                if (tree_->isExpanded(idx)) {
                    visitVisible(idx);
                }
                continue;
            }

            for (const TimelineEvent* event : node->events()) {
                if (event->end < visibleStart_ || event->start > visibleEnd_) {
                    continue;
                }

                const uint64_t clippedStart = std::max(event->start, visibleStart_);
                const uint64_t clippedEnd = std::min(event->end, visibleEnd_);
                const double left = plot_->transform(QwtPlot::xBottom, static_cast<double>(clippedStart));
                const double right = plot_->transform(QwtPlot::xBottom, static_cast<double>(clippedEnd));

                QRectF eventRect(QPointF(left, static_cast<double>(barTop)),
                                 QPointF(right, static_cast<double>(barBottom)));
                eventRect = eventRect.normalized();
                if (eventRect.width() < 2.0) {
                    eventRect.setWidth(2.0);
                }

                if (eventRect.contains(QPointF(pos))) {
                    found = true;
                    selectedIndex = idx;
                    selectedEvent = event;
                    return;
                }
            }

            if (tree_->isExpanded(idx)) {
                visitVisible(idx);
            }
        }
    };

    visitVisible(QModelIndex());

    if (found) {
        selectedRowIndex_ = selectedIndex;
        selectedEvent_ = selectedEvent;
        hasSelectedEvent_ = true;
    } else {
        selectedRowIndex_ = QPersistentModelIndex();
        selectedEvent_ = nullptr;
        hasSelectedEvent_ = false;
    }

    draw();
}
