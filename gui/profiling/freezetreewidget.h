#pragma once

#include <QTreeView>

class FreezeTreeWidget : public QTreeView {
    Q_OBJECT

public:
    FreezeTreeWidget(QWidget* parent);
    ~FreezeTreeWidget();
    void setModel(QAbstractItemModel* model);
    void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible) Q_DECL_OVERRIDE;
    void setSelectionModel(QItemSelectionModel* selectionModel);
    void setColumnsFrozen(unsigned numColumns);

protected:
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;

private:
    unsigned numColumnsFrozen = 0;
    QTreeView *frozenTreeView;
    void setup();
    void updateFrozenTableGeometry();

private slots:
    void updateSectionWidth(int logicalIndex, int oldSize, int newSize);
};
