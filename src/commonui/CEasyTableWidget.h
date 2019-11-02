#pragma once

#include <QTableWidget>

class CEasyTableWidget : public QTableWidget 
{

public:
	CEasyTableWidget(QWidget * parent = Q_NULLPTR);
	~CEasyTableWidget();

	void reset();

	QTableWidgetItem* setCellText(int row, int column, const QString& text);
	QTableWidgetItem* setCellText(int row, int column, const QStringRef& text) { return setCellText(row, column, text.toString()); }
};
