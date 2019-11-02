#include "CEasyTableWidget.h"

CEasyTableWidget::CEasyTableWidget(QWidget * parent) : QTableWidget(parent) 
{
}

CEasyTableWidget::~CEasyTableWidget() 
{
}

void CEasyTableWidget::reset()
{
	setRowCount(0);
	setColumnCount(0);
}

QTableWidgetItem* CEasyTableWidget::setCellText(int row, int column, const QString& text)
{
	if (row < 0 || column < 0)
		return nullptr;

	if (row >= rowCount() - 1)
		setRowCount(row + 1);

	if (column >= columnCount() - 1)
		setColumnCount(column + 1);

	auto *cellItem = item(row, column);
	if (cellItem == nullptr)
	{
		cellItem = new QTableWidgetItem(text);
		setItem(row, column, cellItem);
		return cellItem;
	}
	else
	{
		cellItem->setText(text);
		return cellItem;
	}
}
