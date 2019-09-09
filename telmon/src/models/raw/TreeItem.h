#ifndef TREEITEM_H
#define TREEITEM_H

// based on http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html


template<class T>
class QList;

class QVariant;
class QString;

class TreeItem
{
public:
    /**
     * @brief Notice:TreeItem get ownerships of a_data and he will release it!
     * @param a_data
     * @param a_parentItem
     */
    explicit TreeItem(QList<QVariant> *a_data, TreeItem *a_parentItem);
    virtual ~TreeItem();

    /**
     * @brief get specific child
     * @param row
     * @return TreeItem
     */
    TreeItem *child(int row) const;

    /**
     * @brief get total number of children
     * @return
     */
    int childCount() const;

    int childNumber() const;


    /**
     * @brief Get information about the number of columns associated with the item
     * @return
     */
    int columnCount() const;


    /**
     * @brief Obtain information for specific column
     * @param column
     * @return
     */
    QVariant data(int column) const;


    bool setData(int column, const QVariant &value);


    bool insertChildren(int position, int count, int columns);


    bool removeChildren(int position, int count);


    bool removeColumns(int position, int columns);


    bool insertColumns(int position, int columns);


    /**
     * @brief get row parent item
     * @return
     */
    TreeItem *parent() const noexcept;

    // new
    bool insertChildItem(const int position, const QString& a_col0Data, const QString& a_col1Data) noexcept;


private:
    QList<TreeItem*> *m_childItems;
    QList<QVariant> * m_itemData;
    TreeItem *m_parentItem;

    virtual void constructorInvariant() noexcept;
};

#endif // TREEITEM_H
