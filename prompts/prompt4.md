Change TimelineScaleDraw to displayed at the TOP
instead of the bottom.

Requirements:

1. TimelineScaleDraw is drawn at the top of the plot.
2. The QTreeView content area must be padded from the top
   by the height of TimelineScaleDraw.
3. the plot do not show axis on the bottom

Implementation details:

- Use a QVBoxLayout as the main layout.
- TimelineScaleWidget (or scale container) at index 0.
- QTreeView below it.
- The scale height should be fixed (e.g. 40px) or sizeHint based.
- TreeView should take remaining space (stretch factor 1).

If TimelineScaleDraw is painted manually inside the same widget:

- Override sizeHint() to return proper height.
- Add top margin to treeview via:
     treeView->setViewportMargins(0, scaleHeight, 0, 0);

OR use layout spacing instead of viewportMargins if separate widget.