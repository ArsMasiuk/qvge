#include "spincombobox.h"


namespace QSint
{


SpinComboBox::SpinComboBox(QWidget *parent) :
	QSpinBox(parent)
{
	m_button = new QToolButton(this);
	m_button->setFixedWidth(16);
	m_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_button->setPopupMode(QToolButton::InstantPopup);
	m_button->setVisible(false);

    connect(m_button, SIGNAL(triggered(QAction*)), this, SLOT(onAction(QAction*)));

	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	setLayout(mainLayout);

	mainLayout->addStretch();
	mainLayout->addWidget(m_button);
}


void SpinComboBox::setValueList(const QList<int> &values)
{
	m_button->setVisible(!values.isEmpty());
	if (values.isEmpty())
		return;

	QMenu *m = new QMenu(this);
	for (int i : values)
	{
        QAction *a = m->addAction(QString::number(i));
		a->setData(i);
	}

	m_button->setMenu(m);
}


// protected members

void SpinComboBox::resizeEvent(QResizeEvent *event)
{
	QSpinBox::resizeEvent(event);

    int arrowSize = m_button->width();

    layout()->setContentsMargins(1, 1, arrowSize - 1, 1);
}


void SpinComboBox::onAction(QAction* act)
{
	int v = act->data().toInt();

	setValue(v);
}


} // namespace
