#pragma once

#include "timeline_node.h"

#include <QAbstractItemModel>

class TimelineNodeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit TimelineNodeModel(QObject* parent = nullptr);
    explicit TimelineNodeModel(TimelineNode* rootNode, QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setRootNode(TimelineNode* node);
    TimelineNode* rootNode() const;
    TimelineNode* nodeFromIndex(const QModelIndex& index) const;

private:
    TimelineNode* rootNode_ = nullptr;
};
