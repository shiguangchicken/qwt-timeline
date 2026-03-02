
# generate more nodes
1. generate more nodes Core0 - Core10  , Stream0 - Stream20 in main_window.cpp
2. generate more events

# TimelineWidget
1. treeview on the left item height should equal to the plot height on the right
2. treeview on the left do not show the header
3. use ctrl + mouse wheel to zoom in and zoom out timeline, only scale for x time axis\
4. draw a vertical line follow the mouse position, overide the mouseMoveEvent, use qtimer to update at least 40ms when mouse moved to reduce the cpu and gpu workload

# TimelineWidget
1. treeview node height change to 60, use QStyledItemDelegate to show the view item node border
2. event timeline node height align with the left tree view node height. the same node height
3. use QPainter drawRects to speed up the draw


# TimelineWidget
1. TimelineWidget : the tree view node height is 60, right. But event node height is small, i want it 60 height, too. please fix this bug

# TimelineWidget
1. the wheel event also effect on the TimelineWidget
2. the mouse left click the event, it can be hightlight color, and draw vertical line on both event's left and right
3. show total timeline range when the ui first show 


# TimelineWidget
1. only the wheel event control vertical scrollbar, the same action as the treeview vertical scroll bar. 
2. shift + wheel event ctrol the horizental scroll bar.
3. ctrl + wheel event still keep zoom in , zoom out.


# TimelineWidget
1. no plot grid in y axis
2. x aixs add unit like this
    const char* TimelineWidget::getTimeUint(int64_t time, uint64_t& factor)
{
    uint64_t value = abs(time);
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

3. mouse tracker show a text when hover on a event, text: event name, event start time, event end time

4. event rect show the event name on center, and cut the string if the name is oversize the rect

# main_window.cpp
1. createDemoTree do not add events on the parent node like CPU and GPU