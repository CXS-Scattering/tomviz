/* This source file is part of the Tomviz project, https://tomviz.org/.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#include "AbstractDataModel.h"

#include <QTreeWidgetItem>

AbstractDataModel::AbstractDataModel(QObject* parent_)
  : QAbstractItemModel(parent_)
{}

AbstractDataModel::~AbstractDataModel()
{
  qDeleteAll(m_rootItem->takeChildren());
  delete m_rootItem;
}

int AbstractDataModel::rowCount(const QModelIndex& p) const
{
  return getItem(p)->childCount();
}

int AbstractDataModel::columnCount(const QModelIndex& p) const
{
  return getItem(p)->columnCount();
}

QModelIndex AbstractDataModel::index(int row, int column,
                                     const QModelIndex& p) const
{
  if (!hasIndex(row, column, p)) {
    return QModelIndex();
  }

  QTreeWidgetItem* childItem = getItem(p)->child(row);

  return (childItem ? createIndex(row, column, childItem) : QModelIndex());
}

QModelIndex AbstractDataModel::parent(const QModelIndex& index_) const
{
  if (!index_.isValid()) {
    return QModelIndex();
  }

  QTreeWidgetItem* childItem = getItem(index_);
  QTreeWidgetItem* parentItem = childItem->parent();

  if (parentItem == m_rootItem) {
    return QModelIndex();
  }

  if (parentItem == nullptr) {
    return QModelIndex();
  }

  QTreeWidgetItem* grandParentItem = parentItem->parent();
  const int row = grandParentItem->indexOfChild(parentItem);

  return QAbstractItemModel::createIndex(row, 0, parentItem);
}

QVariant AbstractDataModel::data(const QModelIndex& index_, int role) const
{
  if (!isIndexValidUpperBound(index_)) {
    return QVariant();
  }

  QModelIndex parentIndex = index_.parent();
  QTreeWidgetItem* parent_ = getItem(parentIndex);

  QTreeWidgetItem* item = parent_->child(index_.row());
  if (role == Qt::DisplayRole) {
    return item->data(index_.column(), role);
  } else {
    return QVariant();
  }
}

bool AbstractDataModel::setData(const QModelIndex& index_,
                                const QVariant& value, int role)
{
  if (!isIndexValidUpperBound(index_)) {
    return false;
  }

  QTreeWidgetItem* item = getItem(index_);
  if (role == Qt::DisplayRole) {
    item->setData(index_.column(), Qt::DisplayRole, value);
    QAbstractItemModel::dataChanged(index_, index_);
    return true;
  }

  return false;
}

QVariant AbstractDataModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return m_rootItem->data(section, role);
  }

  return QVariant();
}

Qt::ItemFlags AbstractDataModel::flags(const QModelIndex& index_) const
{
  if (index_.isValid() && index_.column() == 0) {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  }

  return QAbstractItemModel::flags(index_);
}

bool AbstractDataModel::isIndexValidUpperBound(const QModelIndex& index_) const
{
  if (!index_.isValid()) {
    return false;
  }

  QModelIndex const& parent_ = index_.parent();
  if (index_.row() >= rowCount(parent_) ||
      index_.column() >= columnCount(parent_)) {
    return false;
  }

  return true;
}

QTreeWidgetItem* AbstractDataModel::getItem(const QModelIndex& index) const
{
  if (index.isValid()) {
    QTreeWidgetItem* item =
      static_cast<QTreeWidgetItem*>(index.internalPointer());
    if (item) {
      return item;
    }
  }

  return m_rootItem;
}

const QModelIndex AbstractDataModel::getDefaultIndex()
{
  return index(0, 0, QModelIndex());
}

bool AbstractDataModel::removeRows(int row, int count,
                                   const QModelIndex& parent)
{
  QTreeWidgetItem* parentItem = getItem(parent);
  if (row < 0 || row + count > parentItem->childCount()) {
    return false;
  }

  QAbstractItemModel::beginRemoveRows(parent, row, row + count - 1);

  for (int i = 0; i < count; i++) {
    // Shortens the list on every iteration
    delete parentItem->takeChild(row);
  }

  QAbstractItemModel::endRemoveRows();
  return true;
}
