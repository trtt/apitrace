#include "freezetreewidget.h"

#include <QScrollBar>
#include <QHeaderView>
#include <QDebug>

FreezeTreeWidget::FreezeTreeWidget(QWidget* parent) : QTreeView(parent)
{
    frozenTreeView = new QTreeView(this);

    setup();

    connect(header(), &QHeaderView::sectionResized, this,
            &FreezeTreeWidget::updateSectionWidth);

    connect(frozenTreeView->verticalScrollBar(), &QAbstractSlider::valueChanged,
            verticalScrollBar(), &QAbstractSlider::setValue);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged,
            frozenTreeView->verticalScrollBar(), &QAbstractSlider::setValue);

    connect(frozenTreeView, &QTreeView::collapsed, this, &QTreeView::collapse);
    connect(frozenTreeView, &QTreeView::expanded, this, &QTreeView::expand);

    connect(frozenTreeView->header(),  &QHeaderView::sectionClicked,
            [this]() {
                frozenTreeView->header()->setSortIndicatorShown(true);
                header()->setSortIndicatorShown(false);
            });
    connect(header(),  &QHeaderView::sectionClicked,
            [this]() {
                frozenTreeView->header()->setSortIndicatorShown(false);
                header()->setSortIndicatorShown(true);
            });
}

FreezeTreeWidget::~FreezeTreeWidget()
{
    delete frozenTreeView;
}

void FreezeTreeWidget::setColumnsFrozen(unsigned numColumns) {
    if (numColumns > numColumnsFrozen) {
        for (int col = numColumnsFrozen; col < numColumns; ++col) {
            frozenTreeView->setColumnHidden(col, false);
        }
    }
    for (int col = numColumns; col < model()->columnCount(); ++col) {
        frozenTreeView->setColumnHidden(col, true);
    }
    numColumnsFrozen = numColumns;
    updateFrozenTableGeometry();
}

void FreezeTreeWidget::setModel(QAbstractItemModel* model) {
    frozenTreeView->setModel(model);
    connect(model, &QAbstractItemModel::modelReset, [=]() {
                setColumnsFrozen(numColumnsFrozen);
            });
    QTreeView::setModel(model);
    frozenTreeView->setSelectionModel(selectionModel());
}

void FreezeTreeWidget::setSelectionModel(QItemSelectionModel* selectionModel)
{
    frozenTreeView->setSelectionModel(selectionModel);
    QTreeView::setSelectionModel(selectionModel);
}

void FreezeTreeWidget::setup()
{
    frozenTreeView->setUniformRowHeights(true);
    frozenTreeView->setFocusPolicy(Qt::NoFocus);
    frozenTreeView->header()->setSectionResizeMode(QHeaderView::Fixed);
    frozenTreeView->setSortingEnabled(true);

    viewport()->stackUnder(frozenTreeView);

    frozenTreeView->setStyleSheet("QTreeView { border: none;"
                                  "selection-background-color: #999}");
    for (int i = 0; i < 4; ++i) {
        frozenTreeView->setColumnWidth(i, columnWidth(i) );
    }

    frozenTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozenTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozenTreeView->show();

    frozenTreeView->setDragEnabled(false);

    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
    frozenTreeView->setVerticalScrollMode(ScrollPerPixel);
}

void FreezeTreeWidget::updateSectionWidth(int logicalIndex, int /* oldSize */, int newSize)
{
    if (logicalIndex < numColumnsFrozen){
        frozenTreeView->setColumnWidth(logicalIndex, newSize);
        updateFrozenTableGeometry();
    }
}

void FreezeTreeWidget::resizeEvent(QResizeEvent * event)
{
    QTreeView::resizeEvent(event);
    updateFrozenTableGeometry();
}

QModelIndex FreezeTreeWidget::moveCursor(CursorAction cursorAction,
                                         Qt::KeyboardModifiers modifiers)
{
    QModelIndex current = QTreeView::moveCursor(cursorAction, modifiers);

    //if (cursorAction == MoveLeft && current.column() > 0
            //&& visualRect(current).topLeft().x() < frozenTreeView->columnWidth(0) ){
        //const int newValue = horizontalScrollBar()->value() + visualRect(current).topLeft().x()
            //- frozenTreeView->columnWidth(0);
        //horizontalScrollBar()->setValue(newValue);
    //}
    return current;
}

void FreezeTreeWidget::scrollTo (const QModelIndex & index, ScrollHint hint){
    frozenTreeView->scrollTo(index, hint);
    QTreeView::scrollTo(index, hint);
}

void FreezeTreeWidget::updateFrozenTableGeometry()
{
    int size = 0;
    for (int i = 0; i < numColumnsFrozen; ++i) {
        size += columnWidth(i);
    }
    frozenTreeView->setGeometry(frameWidth(), frameWidth(), size,
                                viewport()->height()+header()->height());
}
