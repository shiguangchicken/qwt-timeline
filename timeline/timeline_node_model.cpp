#include "timeline_node_model.h"

#include <QString>

TimelineNodeModel::TimelineNodeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

TimelineNodeModel::TimelineNodeModel(TimelineNode* rootNode, QObject* parent)
    : QAbstractItemModel(parent)
    , rootNode_(rootNode)
{
}

QModelIndex TimelineNodeModel::index(int row, int column, const QModelIndex& parentIndex) const
{
    if (column < 0 || column >= columnCount(parentIndex) || row < 0) {
        return QModelIndex();
    }

    TimelineNode* parentNode = nodeFromIndex(parentIndex);
    if (parentNode == nullptr) {
        parentNode = rootNode_;
    }
    if (parentNode == nullptr) {
        return QModelIndex();
    }

    TimelineNode* childNode = parentNode->visibleChildAt(row);
    if (childNode == nullptr) {
        return QModelIndex();
    }

    return createIndex(row, column, childNode);
}

QModelIndex TimelineNodeModel::parent(const QModelIndex& childIndex) const
{
    if (!childIndex.isValid()) {
        return QModelIndex();
    }

    TimelineNode* childNode = nodeFromIndex(childIndex);
    if (childNode == nullptr) {
        return QModelIndex();
    }

    TimelineNode* parentNode = childNode->parent();
    if (parentNode == nullptr || parentNode == rootNode_) {
        return QModelIndex();
    }

    TimelineNode* grandParentNode = parentNode->parent();
    if (grandParentNode == nullptr) {
        return QModelIndex();
    }

    const int row = grandParentNode->visibleChildRow(parentNode);
    if (row < 0) {
        return QModelIndex();
    }

    return createIndex(row, 0, parentNode);
}

int TimelineNodeModel::rowCount(const QModelIndex& parentIndex) const
{
    if (parentIndex.column() > 0) {
        return 0;
    }

    TimelineNode* parentNode = nodeFromIndex(parentIndex);
    if (parentNode == nullptr) {
        parentNode = rootNode_;
    }
    if (parentNode == nullptr) {
        return 0;
    }

    return parentNode->visibleChildCount();
}

int TimelineNodeModel::columnCount(const QModelIndex& parentIndex) const
{
    Q_UNUSED(parentIndex);
    return 1;
}

QVariant TimelineNodeModel::data(const QModelIndex& itemIndex, int role) const
{
    if (!itemIndex.isValid()) {
        return {};
    }

    TimelineNode* node = nodeFromIndex(itemIndex);
    if (node == nullptr) {
        return {};
    }

    if (role == Qt::DisplayRole && itemIndex.column() == 0) {
        return QString::fromStdString(node->name());
    }

    return {};
}

Qt::ItemFlags TimelineNodeModel::flags(const QModelIndex& itemIndex) const
{
    if (!itemIndex.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void TimelineNodeModel::setRootNode(TimelineNode* node)
{
    beginResetModel();
    rootNode_ = node;
    endResetModel();
}

TimelineNode* TimelineNodeModel::rootNode() const
{
    return rootNode_;
}

TimelineNode* TimelineNodeModel::nodeFromIndex(const QModelIndex& itemIndex) const
{
    if (!itemIndex.isValid()) {
        return nullptr;
    }
    return static_cast<TimelineNode*>(itemIndex.internalPointer());
}
