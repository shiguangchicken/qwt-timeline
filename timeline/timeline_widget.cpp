#include "timeline_widget.h"

#include "theme.h"

#include <QBoxLayout>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QScrollBar>
#include <QTreeView>
#include <qwt_plot.h>
#include <qwt_plot_opengl_canvas.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_item.h>
#include <qwt_scale_map.h>

#include <algorithm>
#include <functional>
#include <vector>

class TimelineWidget::TimelinePlotItem : public QwtPlotItem {
public:
    explicit TimelinePlotItem(TimelineWidget* owner)
        : owner_(owner)
    {
        setZ(20.0);
    }

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
            int row = 0;
        };

        std::vector<RowNode> visibleRows;
        visibleRows.reserve(128);

        std::function<void(const QModelIndex&, int&)> collect = [&](const QModelIndex& parentIndex, int& rowCounter) {
            const int rows = owner_->model_->rowCount(parentIndex);
            for (int r = 0; r < rows; ++r) {
                const QModelIndex idx = owner_->model_->index(r, 0, parentIndex);
                auto* node = owner_->model_->nodeFromIndex(idx);
                if (node != nullptr) {
                    visibleRows.push_back({ node, rowCounter++ });
                    if (owner_->tree_->isExpanded(idx)) {
                        collect(idx, rowCounter);
                    }
                }
            }
        };

        int rowCounter = 0;
        collect(QModelIndex(), rowCounter);

        const int offsetRows = owner_->tree_->verticalScrollBar() ? owner_->tree_->verticalScrollBar()->value() : 0;
        const int offsetPixels = offsetRows * owner_->rowHeight_;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);

        const QColor rowLineColor = QColor::fromRgba(timeline_color::ROW_LINE);
        painter->setPen(QPen(rowLineColor, 1));
        for (const auto& item : visibleRows) {
            const int yTop = item.row * owner_->rowHeight_ - offsetPixels;
            const double yLine = yMap.transform(yTop + owner_->rowHeight_);
            painter->drawLine(QPointF(xMap.transform(static_cast<double>(owner_->visibleStart_)), yLine),
                              QPointF(xMap.transform(static_cast<double>(owner_->visibleEnd_)), yLine));
        }

        for (const auto& item : visibleRows) {
            const int yTop = item.row * owner_->rowHeight_ - offsetPixels;
            const int barTop = yTop + 3;
            const int barBottom = yTop + owner_->rowHeight_ - 3;
            if (barBottom <= barTop) {
                continue;
            }

            for (const TimelineEvent* event : item.node->events()) {
                if (event->end < owner_->visibleStart_ || event->start > owner_->visibleEnd_) {
                    continue;
                }

                const uint64_t clippedStart = std::max(event->start, owner_->visibleStart_);
                const uint64_t clippedEnd = std::min(event->end, owner_->visibleEnd_);
                const double left = xMap.transform(static_cast<double>(clippedStart));
                const double right = xMap.transform(static_cast<double>(clippedEnd));
                const double top = yMap.transform(static_cast<double>(barTop));
                const double bottom = yMap.transform(static_cast<double>(barBottom));

                QRectF rect(QPointF(left, top), QPointF(right, bottom));
                rect = rect.normalized();

                const QColor fillColor = QColor::fromRgba(event->color);
                painter->fillRect(rect, fillColor);
                painter->setPen(QPen(QColor::fromRgba(timeline_color::EVENT_EDGE_LINE), 1));
                painter->drawRect(rect);
            }
        }

        painter->restore();
    }

private:
    TimelineWidget* owner_ = nullptr;
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
    , plot_(new QwtPlot(this))
    , plotItem_(new TimelinePlotItem(this))
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
    grid->attach(plot_);

    plot_->enableAxis(QwtPlot::yLeft, false);
    plot_->enableAxis(QwtPlot::xBottom, true);
    plot_->setAxisScale(QwtPlot::xBottom, static_cast<double>(visibleStart_), static_cast<double>(visibleEnd_));
    plot_->setAxisScale(QwtPlot::yLeft, 500.0, 0.0);

    plotItem_->attach(plot_);
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

void TimelineWidget::setTimeRange(uint64_t start, uint64_t end)
{
    if (end <= start) {
        end = start + 1;
    }
    visibleStart_ = start;
    visibleEnd_ = end;
    draw();
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
