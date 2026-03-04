---
timeline_widget change 
---

1. draw a text to display current time. time format is the same as TimelineScaleDraw::getTimeUnit. The text position x is the mouse x positon. The text y postion on the bottom area of the TimelineScaleDraw. draw text in the function TimelineScaleDraw::draw() in file timeline_widget.cpp.
2. build the project after change, build command in .vscode/tasks.json


1. the events in the node is sorted, use binary search to find the start and end event in current view. 
2. timeline_widget.cpp:185, do not draw all the events if the events count in current view more than 1000. select at most 1000 events to draw. 
3. build the project after change, build command in .vscode/tasks.json



Implement a custom QwtScaleDraw subclass named TimelineScaleDraw.

Requirements:

1. Override:
   virtual QwtText label(double value) const override;

2. The input value is in nanoseconds.

3. Automatically choose unit based on current scale range.

4. Units:
   ns / us / ms / s

5. Formatting rules:
   - Remove trailing zeros
   - Keep max 3 decimal digits
   - Example outputs:
        7s
        +5ms
        +1.5ms
        999us

6. Add helper:
   QString formatTime(double ns, Unit unit);

7. Update unit when scaleDiv changes.

Ensure smooth behavior during zoom and pan.