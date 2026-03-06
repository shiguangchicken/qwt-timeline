#include "timeline_view.h"

#include <QGridLayout>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <algorithm>

#include "theme.h"

namespace {

constexpr int kNodeRowHeight = 60;

class TimelineNodeItemDelegate : public QStyledItemDelegate {
public:
    explicit TimelineNodeItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(kNodeRowHeight);
        return size;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        painter->setBrush(Qt::NoBrush);

        if (index.column() != 0)
            return;

        auto view = qobject_cast<QTreeView*>(parent());
        if (!view)
            return;

        painter->save();
        painter->setPen(QPen(QColor::fromRgba(timeline_color::ROW_LINE), 1));

        int y = option.rect.bottom() - 1;

        painter->drawLine(0, y, view->viewport()->width(), y);

        painter->restore();
    }
};

}  // namespace

TimelineView::TimelineView(QWidget* parent)
    : QWidget(parent)
    , node_view_(new QTreeView(this))
    , node_model_(new TimelineNodeModel(this))
    , timeline_widget_(new TimelineWidget(this))
    , vertical_scroll_bar_(new QScrollBar(Qt::Vertical, this))
    , horizontal_scroll_bar_(new QScrollBar(Qt::Horizontal, this))
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(4);
    layout->setVerticalSpacing(4);

    layout->addWidget(node_view_, 0, 0);
    layout->addWidget(timeline_widget_, 0, 1);
    layout->addWidget(vertical_scroll_bar_, 0, 2);
    layout->addWidget(horizontal_scroll_bar_, 1, 0, 1, 2);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setRowStretch(0, 1);

    node_view_->setModel(node_model_);
    node_view_->setUniformRowHeights(true);
    node_view_->setAlternatingRowColors(false);
    node_view_->setItemDelegate(new TimelineNodeItemDelegate(node_view_));
    node_view_->header()->hide();
    node_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    timeline_widget_->init(node_model_, node_view_);
    timeline_widget_->setRowHeight(kNodeRowHeight);

    connect(vertical_scroll_bar_, &QScrollBar::valueChanged, this, &TimelineView::syncTreeFromVerticalScroll);
    connect(node_view_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        syncVerticalScrollFromTree();
        timeline_widget_->draw();
    });
    connect(node_view_->verticalScrollBar(), &QScrollBar::rangeChanged, this,
            [this](int, int) { syncVerticalScrollFromTree(); });

    connect(horizontal_scroll_bar_, &QScrollBar::valueChanged, this, [this](int value) {
        const uint64_t start = static_cast<uint64_t>(value);
        const uint64_t end = start + windowSize_;
        timeline_widget_->setTimeRange(start, end);
    });

    connect(timeline_widget_, &TimelineWidget::timeRangeChanged, this,
            [this](uint64_t start, uint64_t end) { updateHorizontalScrollBarFromRange(start, end); });

    connect(node_view_, &QTreeView::expanded, this, [this](const QModelIndex&) {
        syncVerticalScrollFromTree();
        timeline_widget_->draw();
    });
    connect(node_view_, &QTreeView::collapsed, this, [this](const QModelIndex&) {
        syncVerticalScrollFromTree();
        timeline_widget_->draw();
    });
}

void TimelineView::setRootNode(TimelineNode* rootNode)
{
    rootNode_ = rootNode;
    node_model_->setRootNode(rootNode_);

    if (rootNode_ != nullptr) {
        fullMin_ = std::numeric_limits<uint64_t>::max();
        fullMax_ = 0;
        rootNode_->foreachNode([this](TimelineNode* node) {
            if (node->isVisible()) {
                fullMin_ = std::min(fullMin_, node->minTime());
                fullMax_ = std::max(fullMax_, node->maxTime());
            }
        });
    } else {
        fullMin_ = 0;
        fullMax_ = 0;
    }

    for (int row = 0; row < node_model_->rowCount(); ++row) {
        node_view_->expand(node_model_->index(row, 0));
    }

    configureHorizontalRange();
    syncVerticalScrollFromTree();
    timeline_widget_->draw();
}

TimelineNode* TimelineView::rootNode() const
{
    return rootNode_;
}

void TimelineView::syncVerticalScrollFromTree()
{
    QScrollBar* treeBar = node_view_->verticalScrollBar();
    vertical_scroll_bar_->setRange(treeBar->minimum(), treeBar->maximum());
    vertical_scroll_bar_->setPageStep(treeBar->pageStep());
    vertical_scroll_bar_->setSingleStep(treeBar->singleStep());
    vertical_scroll_bar_->setValue(treeBar->value());
}

void TimelineView::syncTreeFromVerticalScroll(int value)
{
    if (node_view_->verticalScrollBar()->value() != value) {
        node_view_->verticalScrollBar()->setValue(value);
    }
    timeline_widget_->draw();
}

void TimelineView::configureHorizontalRange()
{
    timeline_widget_->setFullRange(fullMin_, std::max(fullMax_, fullMin_ + 1));

    if (fullMax_ <= fullMin_) {
        windowSize_ = 1;
        updateHorizontalScrollBarFromRange(0, windowSize_);
        timeline_widget_->setTimeRange(0, windowSize_);
        return;
    }

    const uint64_t total = fullMax_ - fullMin_;
    windowSize_ = total;

    updateHorizontalScrollBarFromRange(fullMin_, fullMin_ + windowSize_);

    timeline_widget_->setTimeRange(fullMin_, fullMin_ + windowSize_);
}

void TimelineView::updateHorizontalScrollBarFromRange(uint64_t start, uint64_t end)
{
    uint64_t safeEnd = end;
    if (safeEnd <= start) {
        safeEnd = start + 1;
    }

    if (fullMax_ <= fullMin_) {
        windowSize_ = 1;
        horizontal_scroll_bar_->blockSignals(true);
        horizontal_scroll_bar_->setRange(0, 0);
        horizontal_scroll_bar_->setPageStep(1);
        horizontal_scroll_bar_->setSingleStep(1);
        horizontal_scroll_bar_->setValue(0);
        horizontal_scroll_bar_->blockSignals(false);
        return;
    }

    const uint64_t total = fullMax_ - fullMin_;
    windowSize_ = std::clamp<uint64_t>(safeEnd - start, 1, total);
    const uint64_t maxStart = (total > windowSize_) ? (fullMax_ - windowSize_) : fullMin_;
    const uint64_t clampedStart = std::clamp<uint64_t>(start, fullMin_, maxStart);

    horizontal_scroll_bar_->blockSignals(true);
    horizontal_scroll_bar_->setRange(static_cast<int>(fullMin_), static_cast<int>(maxStart));
    horizontal_scroll_bar_->setPageStep(static_cast<int>(windowSize_));
    horizontal_scroll_bar_->setSingleStep(std::max(1, static_cast<int>(windowSize_ / 20)));
    horizontal_scroll_bar_->setValue(static_cast<int>(clampedStart));
    horizontal_scroll_bar_->blockSignals(false);
}
