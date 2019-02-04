#pragma once

#include <QWidget>

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

private:
	Ui::CStartPage ui;
	CMainWindow *m_parent;
};
