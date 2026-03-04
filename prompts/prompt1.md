Purpose

Implement a reusable Timeline Widgets Library using Qt + Qwt (OpenGL backend), and provide a simple test demo that generates dummy timeline data and displays it.

1️⃣ Overall Requirements

Create a reusable timeline widgets library

Create a simple test demo

Generate dummy timeline data

Modify main_window.cpp

Display two independent TimelineView widgets in the main window

Use:

Qt (Widgets)

Qwt with OpenGL backend enabled

Use modern C++ (C++17 or later)

2️⃣ Timeline Library Structure

Organize into a small reusable module:

timeline/
    theme.h
    timeline_event.h
    timeline_node.h / timeline_node.cpp
    timeline_node_model.h / timeline_node_model.cpp
    timeline_widget.h / timeline_widget.cpp
    timeline_view.h / timeline_view.cpp
3️⃣ Theme Definition
theme.h

Define timeline color theme values:

namespace timeline_color {
    constexpr uint32_t DEFAULT = 0xff9e9e9e,
    constexpr uint32_t HIGHLIGHT = 0xffFBDEBB
    constexpr uint32_t GRAY = 0xff645A45
    constexpr uint32_t DEEP_GRAY = 0xffE0E0E0
    constexpr uint32_t ROW_LINE = 0xffC5BEB0
    constexpr uint32_t EVENT_EDGE_LINE = 0xffE59B03
    constexpr uint32_t MOUSE_LINE = 0xffDCD8CF
}


4️⃣ Timeline Data Structures
TimelineEvent
struct TimelineEvent {
    std::string_view name;
    uint64_t start;
    uint64_t end;
    uint32_t color = timeline_color::DEFAULT;

    TimelineEvent* pre = nullptr;
    TimelineEvent* next = nullptr;

    uint64_t duration() const { return end - start; }
};
TimelineNode

Hierarchical timeline structure.

class TimelineNode {
public:
    enum NodeType { TIMELINE, TIMELINE_AREA };

    TimelineNode();
    virtual ~TimelineNode();
    TimelineNode(const std::string& name, NodeType type = TIMELINE);

    TimelineNode* parent();
    std::vector<TimelineNode*> children();
    TimelineNode* childAt(int index);

    void addChild(TimelineNode* node);

    void setName(std::string&& name);
    const std::string& name() const;

    const std::vector<TimelineEvent*>& events() const;
    void addEvent(TimelineEvent* event);
    void addEvents(const std::vector<TimelineEvent*>& events);

    void foreachNode(const std::function<void(TimelineNode*)>& func);

    bool isExpand() const;
    void setExpand(bool expand);

    bool isVisible() const;
    void setVisible(bool v);

    void sortEvents();

    uint64_t minTime() const;
    uint64_t maxTime() const;
};

Implementation details:

Maintain parent pointer

Maintain vector of children

Maintain vector of TimelineEvent*

minTime() and maxTime() should traverse children recursively

sortEvents() sorts by start time

5️⃣ TimelineNodeModel

Create TimelineNodeModel derived from QAbstractItemModel to support:

Tree hierarchy

Expand / collapse

Node visibility

Required overrides:

rowCount

columnCount

index

parent

data

flags

Display node name in column 0.

6️⃣ TimelineWidget

Responsible for drawing timeline using Qwt with OpenGL backend.

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget* parent = nullptr);

    void init(TimelineNodeModel* model, QTreeView* tree);
    void draw();
};
Requirements:

Internally use QwtPlot

Enable OpenGL rendering:

QwtPlotCanvas with OpenGL

X axis = time

Y axis = row index aligned with tree view visible rows

Draw each TimelineEvent as:

Filled rectangle

Positioned at [start, end]

Height fixed per row

Use color from theme

Support horizontal scrolling

Support vertical synchronization with QTreeView

7️⃣ TimelineView (Composite Widget)

Contains:

QTreeView* node_view_;
TimelineNodeModel* node_model_;
TimelineWidget* timeline_widget_;
QScrollBar* vertical_scroll_bar_;
QScrollBar* horizontal_scroll_bar_;

Layout:

-----------------------------------------
| QTreeView | TimelineWidget           |
-----------------------------------------
| horizontal_scroll_bar_               |
-----------------------------------------

Requirements:

Vertical scroll bar shared between tree and timeline

Horizontal scroll bar controls time range

Synchronize scrolling

When tree expands/collapses → timeline updates

8️⃣ Test Demo (Modify main_window.cpp)

Update main_window.cpp:

Create dummy timeline data

Build a small tree:

Example structure:

CPU
  Core0
  Core1
GPU
  Stream0
  Stream1

For each node:

Generate 10–20 random TimelineEvents

Random start/end

Random color

Create TWO TimelineView instances:

TimelineView* timeline1;
TimelineView* timeline2;

Add both into main window layout vertically.

9️⃣ Dummy Data Generation

Use:

std::mt19937

Random durations

Sequential time ranges

Example idea:

Each event start = previous end + random gap

Duration random between 10–200

🔟 Additional Notes

Keep code clean and modular

Use smart pointers where appropriate

Avoid memory leaks

Use Q_OBJECT where necessary

Separate header/source files

Comment important logic

Ensure build works with Qwt OpenGL enabled


Build:

cmake -S /home/meng/Documents/projects/qwt-timeline -B /home/meng/Documents/projects/qwt-timeline/build/Desktop_Qt_6_9_3-Debug -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_GENERATOR:STRING=Ninja -DCMAKE_COLOR_DIAGNOSTICS:BOOL=ON -DCMAKE_PREFIX_PATH:PATH=/home/meng/Qt/6.9.3/gcc_64 

cmake --build ./build/Desktop_Qt_6_9_3-Debug/

Run:
./build/Desktop_Qt_6_9_3-Debug/qwt-timeline


✅ Expected Result

Reusable timeline widget library

Two timeline views shown in main window

Scrollable

Expandable tree

OpenGL accelerated rendering via Qwt

Dummy data displayed correctly


# create timline cmake
create a CMakeLists.txt in timeline directory
use add_subdirectory() on the main CMakeLists.txt


# create qwt submodule

1. add qwt submodule in directory timeline/qwt,  qwt git url: git@github.com:opencor/qwt.git
2. create a cmake build command to build qwt submodule, qwt should configure by qmake, qmake path: /home/meng/Qt/6.9.3/gcc_64/bin/qmake


# use timeline widget use opengl
1. qwt submodule confiure and build with QwtOpenGL
2. use QwtPlotOpenGLCanvas in TimelineWidget to use hardware accelerate
