#pragma once

#include <QWidget>

namespace Ui {
	class CQuickHelpUI;
}


class CQuickHelpUI : public QWidget
{
	Q_OBJECT

public:
	CQuickHelpUI(QWidget *parent = 0);
	~CQuickHelpUI();

private:
	Ui::CQuickHelpUI *ui;
};

