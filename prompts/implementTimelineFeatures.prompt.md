---
name: implementTimelineFeatures
description: Add enhancements to a timeline widget and demo tree logic.
argument-hint: Details about widget behaviors and demo tree requirements
---
Given a codebase with a custom timeline widget and a demo tree generator, implement the following improvements:

1. Disable or remove the plot grid on the Y axis.
2. Add logic to choose human-readable time units for the X axis labels using a helper like `getTimeUint`. unit shows on the x axis, and the time value is divided by the factor to show in the correct unit.

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

3. When rendering event rectangles, draw the event name centered and trim it to fit the rectangle.
4. Show a textual tracker or tooltip displaying event name, start time, and end time when hovering over an event. the tooltip should update as the mouse moves over different events, and it should disappear when the mouse is not hovering over any event. the tooltip should be positioned near the mouse cursor.
5. Ensure mouse wheel, scrolling, and tooltip behaviors are updated accordingly.
6. In the demo tree creation function, avoid adding events to parent nodes such as CPU/GPU.

Apply changes by editing appropriate `.cpp` and `.h` files, and rebuild to verify the modifications.