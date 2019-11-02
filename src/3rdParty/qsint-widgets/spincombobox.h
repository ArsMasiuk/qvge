#pragma once

#include <QList>
#include <QMenu>
#include <QBoxLayout>
#include <QSpinBox>
#include <QToolButton>
#include <QAction>
#include <QLineEdit>


namespace QSint
{


/**
    \brief Spin editor with additional drop down menu.
    \since 0.4

    \image html SpinComboBox.png An example of SpinComboBox
*/
class SpinComboBox : public QSpinBox
{
    Q_OBJECT

public:
    /** Constructor.
      */
    explicit SpinComboBox(QWidget *parent = 0);

	void setValueList(const QList<int> &values);

Q_SIGNALS:

protected Q_SLOTS:
    void onAction(QAction* action);

protected:
	virtual void resizeEvent(QResizeEvent* event);

	QToolButton *m_button;
};


} // namespace
