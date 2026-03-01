#include "timeline_view.h"

#include <QGridLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QTreeView>

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
    node_view_->header()->setStretchLastSection(true);
    node_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    timeline_widget_->init(node_model_, node_view_);

    connect(vertical_scroll_bar_, &QScrollBar::valueChanged, this, &TimelineView::syncTreeFromVerticalScroll);
    connect(node_view_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        syncVerticalScrollFromTree();
        timeline_widget_->draw();
    });
    connect(node_view_->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int) {
        syncVerticalScrollFromTree();
    });

    connect(horizontal_scroll_bar_, &QScrollBar::valueChanged, this, [this](int value) {
        const uint64_t start = static_cast<uint64_t>(value);
        const uint64_t end = start + windowSize_;
        timeline_widget_->setTimeRange(start, end);
    });

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
        rootNode_->sortEvents();
        fullMin_ = rootNode_->minTime();
        fullMax_ = rootNode_->maxTime();
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
    if (fullMax_ <= fullMin_) {
        horizontal_scroll_bar_->setRange(0, 0);
        timeline_widget_->setTimeRange(0, windowSize_);
        return;
    }

    const uint64_t total = fullMax_ - fullMin_;
    const uint64_t desiredWindow = std::max<uint64_t>(windowSize_, 1);
    windowSize_ = std::min<uint64_t>(desiredWindow, total);

    const uint64_t maxStart = (total > windowSize_) ? (fullMax_ - windowSize_) : fullMin_;
    horizontal_scroll_bar_->setRange(static_cast<int>(fullMin_), static_cast<int>(maxStart));
    horizontal_scroll_bar_->setPageStep(static_cast<int>(windowSize_));
    horizontal_scroll_bar_->setSingleStep(std::max(1, static_cast<int>(windowSize_ / 20)));
    horizontal_scroll_bar_->setValue(static_cast<int>(fullMin_));

    timeline_widget_->setTimeRange(fullMin_, fullMin_ + windowSize_);
}
