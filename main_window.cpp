#include "main_window.h"
#include "./ui_mainwindow.h"

#include "timeline/theme.h"
#include "timeline/timeline_event.h"
#include "timeline/timeline_node.h"
#include "timeline/timeline_view.h"

#include <QVBoxLayout>

#include <array>
#include <functional>
#include <memory>
#include <random>
#include <string>

namespace {

void addRandomEvents(TimelineNode* node, std::mt19937& rng)
{
    if (node == nullptr) {
        return;
    }

    static constexpr std::array<std::string_view, 8> kEventNames = {
        "Decode", "Render", "Dispatch", "Wait", "Compute", "IO", "Kernel", "Sync"
    };
    static constexpr std::array<uint32_t, 6> kColors = {
        timeline_color::DEFAULT,
        timeline_color::GRAY,
        timeline_color::EVENT_EDGE_LINE,
        0xff86d8b2,
        0xffb5a3ff,
        0xff4d76ff,
    };

    std::uniform_int_distribution<int> countDist(35, 65);
    std::uniform_int_distribution<int> gapDist(2, 40);
    std::uniform_int_distribution<int> durationDist(8, 260);
    std::uniform_int_distribution<int> nameDist(0, static_cast<int>(kEventNames.size() - 1));
    std::uniform_int_distribution<int> colorDist(0, static_cast<int>(kColors.size() - 1));

    const int eventCount = countDist(rng);
    uint64_t cursor = 0;
    for (int i = 0; i < eventCount; ++i) {
        cursor += static_cast<uint64_t>(gapDist(rng));
        const uint64_t duration = static_cast<uint64_t>(durationDist(rng));
        auto* event = new TimelineEvent();
        event->name = kEventNames[nameDist(rng)];
        event->start = cursor;
        event->end = cursor + duration;
        event->color = kColors[colorDist(rng)];
        node->addEvent(event);
        cursor = event->end;
    }
}

std::unique_ptr<TimelineNode> createDemoTree(uint32_t seedShift)
{
    auto root = std::make_unique<TimelineNode>("Root", TimelineNode::TIMELINE_AREA);

    auto* cpu = new TimelineNode("CPU");
    for (int i = 0; i <= 10; ++i) {
        cpu->addChild(new TimelineNode("Core" + std::to_string(i)));
    }

    auto* gpu = new TimelineNode("GPU");
    for (int i = 0; i <= 20; ++i) {
        gpu->addChild(new TimelineNode("Stream" + std::to_string(i)));
    }

    root->addChild(cpu);
    root->addChild(gpu);

    std::mt19937 rng(1337u + seedShift);
    root->foreachNode([&](TimelineNode* node) {
        if (node != root.get() && node != cpu && node != gpu) {
            addRandomEvents(node, rng);
        }
    });

    root->sortEvents();
    return root;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto* centralLayout = new QVBoxLayout(ui->centralwidget);
    centralLayout->setContentsMargins(8, 8, 8, 8);
    centralLayout->setSpacing(8);

    timeline1_ = new TimelineView(ui->centralwidget);
    timeline2_ = new TimelineView(ui->centralwidget);

    centralLayout->addWidget(timeline1_);
    centralLayout->addWidget(timeline2_);

    root1_ = createDemoTree(0);
    root2_ = createDemoTree(2026);

    timeline1_->setRootNode(root1_.get());
    timeline2_->setRootNode(root2_.get());
}

MainWindow::~MainWindow()
{
    delete ui;
}
