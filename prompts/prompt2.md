
# generate more nodes
1. generate more nodes Core0 - Core10  , Stream0 - Stream20 in main_window.cpp
2. generate more events

# TimelineWidget
1. treeview on the left item height should equal to the plot height on the right
2. treeview on the left do not show the header
3. use ctrl + mouse wheel to zoom in and zoom out timeline, only scale for x time axis\
4. draw a vertical line follow the mouse position, overide the mouseMoveEvent, use qtimer to update at least 40ms when mouse moved to reduce the cpu and gpu workload