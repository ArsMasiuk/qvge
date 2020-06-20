#pragma once

#include <QList>
#include <QMenu>
#include <QBoxLayout>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QAction>
#include <QLineEdit>


namespace QSint
{


/**
    \brief Double spin editor with additional drop down menu.
    \since 0.4

    \image html DoubleSpinComboBox.png An example of DoubleSpinComboBox
*/
class DoubleSpinComboBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    /** Constructor.
      */
    explicit DoubleSpinComboBox(QWidget *parent = 0);

	void setValueList(const QList<double> &values);

Q_SIGNALS:

protected Q_SLOTS:
    void onAction(QAction* action);

protected:
	virtual void resizeEvent(QResizeEvent* event);

	QToolButton *m_button;
};


} // namespace
