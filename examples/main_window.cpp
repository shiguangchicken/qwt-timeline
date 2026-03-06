#include "main_window.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "theme.h"
#include "timeline_event.h"
#include "timeline_node.h"
#include "timeline_view.h"
#include "ui_mainwindow.h"

namespace {

std::atomic<uint64_t> totoal_event_count = 0;

void addRandomEvents(TimelineNode* node, std::mt19937& rng)
{
    if (node == nullptr) {
        return;
    }

    static constexpr std::array<std::string_view, 8> kEventNames = {"Decode",  "Render", "Dispatch", "Wait",
                                                                    "Compute", "IO",     "Kernel",   "Sync"};
    static constexpr std::array<uint32_t, 6> kColors = {
        timeline_color::DEFAULT,
        timeline_color::GRAY,
        timeline_color::EVENT_EDGE_LINE,
        0xffFFD54F,
        0xffFF9800,
        0xff9CCC65,
    };

    std::uniform_int_distribution<int> countDist(200000, 210000);
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
        totoal_event_count++;
    }
}

void addCounterEvents(TimelineNode* node, std::mt19937& rng)
{
    if (node == nullptr) {
        return;
    }

    static constexpr std::array<uint32_t, 4> kCounterColors = {
        timeline_color::DEFAULT,
        0xff86d8b2,
        0xffb5a3ff,
        0xff4d76ff,
    };

    std::uniform_int_distribution<int> gapDist(5, 45);
    std::uniform_int_distribution<int> durationDist(10, 120);
    std::uniform_real_distribution<double> valueDist(15.0, 220.0);
    std::uniform_int_distribution<int> colorDist(0, static_cast<int>(kCounterColors.size() - 1));

    const int eventCount = 200000;
    uint64_t cursor = 0;
    for (int i = 0; i < eventCount; ++i) {
        cursor += static_cast<uint64_t>(gapDist(rng));
        const uint64_t duration = static_cast<uint64_t>(durationDist(rng));
        auto* event = new CounterTimelineEvent();
        event->start = cursor;
        event->end = cursor + duration;
        event->color = kCounterColors[colorDist(rng)];
        event->value = valueDist(rng);
        node->addEvent(event);
        cursor = event->end;
        totoal_event_count++;
    }
}

void addRelation(TimelineNode* node1, TimelineNode* node2)
{
    if (node1 == nullptr || node2 == nullptr) {
        return;
    }

    const auto& ev1 = node1->events();
    const auto& ev2 = node2->events();
    const size_t n = std::min(ev1.size(), ev2.size());
    for (size_t i = 0; i < n; ++i) {
        auto* t1 = dynamic_cast<TimelineEvent*>(ev1[i]);
        auto* t2 = dynamic_cast<TimelineEvent*>(ev2[i]);
        if (t1 && t2) {
            t1->setRelationNext(t2);
            t2->setRelationPre(t1);
        }
    }
}

std::unique_ptr<TimelineNode> createDemoTree(uint32_t seedShift)
{
    auto root = std::make_unique<TimelineNode>("Root", TimelineNode::TIMELINE);

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

    auto* memory = new TimelineNode("Memory");
    for (int i = 0; i < 5; ++i) {
        memory->addChild(new TimelineNode("Channel" + std::to_string(i), TimelineNode::TIMELINE_COUNTER));
    }
    root->addChild(memory);

    const uint32_t baseSeed = 1337u + seedShift;

    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<size_t>(memory->childCount()));
    for (int i = 0; i < memory->childCount(); ++i) {
        TimelineNode* memoryNode = memory->childAt(i);
        const uint32_t nodeSeed = baseSeed + 10000u + static_cast<uint32_t>(i);
        futures.push_back(std::async(std::launch::async, [memoryNode, nodeSeed]() {
            std::mt19937 localRng(nodeSeed);
            addCounterEvents(memoryNode, localRng);
            memoryNode->sortEvents();
        }));
    }

    std::vector<TimelineNode*> randomEventNodes;
    randomEventNodes.reserve(static_cast<size_t>(cpu->childCount() + gpu->childCount()));
    root->foreachNode([&](TimelineNode* node) {
        if (node == root.get() || node == cpu || node == gpu || node == memory) {
            return;
        }
        if (node->type() == TimelineNode::TIMELINE_COUNTER) {
            return;
        }
        randomEventNodes.push_back(node);
    });

    futures.reserve(futures.size() + randomEventNodes.size());
    for (size_t i = 0; i < randomEventNodes.size(); ++i) {
        TimelineNode* node = randomEventNodes[i];
        const uint32_t nodeSeed = baseSeed + 20000u + static_cast<uint32_t>(i);
        futures.push_back(std::async(std::launch::async, [node, nodeSeed]() {
            std::mt19937 localRng(nodeSeed);
            addRandomEvents(node, localRng);
            node->sortEvents();
        }));
    }

    for (auto& future : futures) {
        future.get();
    }

    // Link adjacent GPU stream events: event[i]->next -> corresponding event[i] of next stream
    for (int i = 0; i + 1 < gpu->childCount(); ++i) {
        addRelation(gpu->childAt(i), gpu->childAt(i + 1));
    }

    return root;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto* centralLayout = new QVBoxLayout(ui->centralwidget);
    centralLayout->setContentsMargins(8, 8, 8, 8);
    centralLayout->setSpacing(8);

    timeline1_ = new TimelineView(ui->centralwidget);
    timeline2_ = new TimelineView(ui->centralwidget);

    centralLayout->addWidget(timeline1_);
    centralLayout->addWidget(timeline2_);

    QElapsedTimer demoTreeBuildTimer;
    demoTreeBuildTimer.start();
    root1_ = createDemoTree(0);
    root2_ = createDemoTree(2026);
    qDebug() << "Total event count:" << totoal_event_count;
    qDebug() << "createDemoTree total cost time (ms):" << demoTreeBuildTimer.elapsed();

    timeline1_->setRootNode(root1_.get());
    timeline2_->setRootNode(root2_.get());
}

MainWindow::~MainWindow()
{
    root1_->foreachNode([](TimelineNode* node) {
        for (BaseEvent* event : node->events()) {
            delete event;
        }
    });
    root2_->foreachNode([](TimelineNode* node) {
        for (BaseEvent* event : node->events()) {
            delete event;
        }
    });
    delete ui;
}
