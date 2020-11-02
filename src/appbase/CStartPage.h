#pragma once

#include <QWidget>
#include <QMap>

#include "ui_CStartPage.h"

class CMainWindow;


class CStartPage : public QWidget 
{
	Q_OBJECT

public:
	CStartPage(CMainWindow *parent);
	~CStartPage();

protected:
	void createActions();
	void createRecentDocs();

protected Q_SLOTS:
	void onCreateDocument();
	void onRecentDocument();
	void onRemoveDocument();

private Q_SLOTS:
	void on_CleanRecentButton_clicked();

private:
	Ui::CStartPage ui;
	CMainWindow *m_parent = nullptr;

	QMap<int, QWidget*> m_buttons;
};
