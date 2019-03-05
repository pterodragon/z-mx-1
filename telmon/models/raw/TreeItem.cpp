#include "TreeItem.h"
#include <QList>
#include <QVariant>
#include <QDebug>


TreeItem::TreeItem(QList<QVariant> *a_data,
                   TreeItem *a_parentItem) :
    m_childItems(new QList<TreeItem*>),
    m_itemData(a_data),
    m_parentItem(a_parentItem)
{
    constructorInvariant();
}


void TreeItem::constructorInvariant() noexcept
{
    //validate that m_itemData is not nullptr
    // so no need to check in every place in the code if m_itemData is nullptr etc...
    if (!m_itemData)
    {
        //print critical msg
        //TODO: print functio name time etc!
        qCritical() << "Constructed tree item without m_itemData! using default values";

        // init with default values
        m_itemData = new QList<QVariant>();
    }
}


TreeItem::~TreeItem()
{
    // if you delete a tree Item, we make sure all
    //children are also deleted
    qDeleteAll(*m_childItems);
    delete m_childItems;
    delete m_itemData;
}


TreeItem *TreeItem::parent() const noexcept
{
    return m_parentItem;
}


TreeItem *TreeItem::child(int a_row) const
{
    // can improve performance by using at
    // read http://doc.qt.io/qt-5/qlist.html#value
    return m_childItems->value(a_row);
}


int TreeItem::childCount() const
{
    return m_childItems->count();
}


int TreeItem::childNumber() const
{
    if (m_parentItem) // no need to check for m_childItems, always init in TreeItem::TreeItem()
    {
        return m_parentItem->m_childItems->indexOf(const_cast<TreeItem*>(this));
    }

    // the root item has no no parent item, so we return 0 to be consistent with
    // the other items
    return 0;
}


int TreeItem::columnCount() const
{
    return m_itemData->count();
}


QVariant TreeItem::data(int column) const
{
    return m_itemData->value(column);
}


// setting data
bool TreeItem::setData(int column, const QVariant &value)
{
    // todo can imperove performance, if we know that m_itemData size is no more than 2
    if (column < 0 || column >= m_itemData->size())
    {
        return false;
    }

    (*m_itemData)[column] = value;
    return true; // data was set successfully
}

// dive into
bool TreeItem::insertChildren(int position, int count, int columns)
{
    qDebug() << "insertChildren: pos, count, columns=" << position << count << columns; //TESITNG

    if (position < 0 || position > m_childItems->size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        // new TreeItem(new QList<QVariant>(std::initializer_list<QVariant> {a_header0, a_header1}))
        //TreeItem *item = new TreeItem(data, this);
        // to do -- adjust for data setting
        TreeItem *item = new TreeItem(new QList<QVariant>(std::initializer_list<QVariant> {row, columns}), this);
        m_childItems->insert(position, item);
    }
    return true;
}

// dive into
bool TreeItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > m_childItems->size())
    {
        return false;
    }

    for (int row = 0; row < count; ++row)
    {
        delete m_childItems->takeAt(position);
    }
    return true;
}


bool TreeItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > m_itemData->size())
    {
        return false;
    }

    for (int column = 0; column < columns; ++column)
    {
        m_itemData->insert(position, QVariant());
    }

    foreach (TreeItem *child, *m_childItems)
    {
        child->insertColumns(position, columns);
    }

    return true;
}

// # # # # # # # #

bool TreeItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > m_itemData->size())
        return false;

    for (int column = 0; column < columns; ++column)
        m_itemData->removeAt(position); // changed from remove to removeAt

    foreach (TreeItem *child, *m_childItems)
        child->removeColumns(position, columns);

    return true;
}


// # # # # # # # # NEW

bool TreeItem::insertChildItem(const int a_position, const char* a_col0Data, const char* a_col1Data) noexcept
{
    if (a_position < 0 || a_position > m_childItems->size())
    {
        qWarning() << "Could not insert child to the following position:" << a_position;
        return false;
    }

    m_childItems->insert(a_position, new TreeItem(new QList<QVariant>(std::initializer_list<QVariant> {a_col0Data, a_col1Data}), this));
    return true;
}










