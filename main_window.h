#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include <memory>

class TimelineNode;
class TimelineView;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    TimelineView* timeline1_ = nullptr;
    TimelineView* timeline2_ = nullptr;
    std::unique_ptr<TimelineNode> root1_;
    std::unique_ptr<TimelineNode> root2_;
};
#endif // MAIN_WINDOW_H
