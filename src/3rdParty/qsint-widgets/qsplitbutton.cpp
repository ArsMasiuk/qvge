#include "qsplitbutton.h"

#include <QMenu>
#include <QActionEvent>


namespace QSint
{


QSplitButton::QSplitButton(QWidget *parent) : QToolButton(parent)
{
    connect(this, SIGNAL(triggered(QAction*)), this, SLOT(onAction(QAction*)));

	setMenu(m_localMenu = new QMenu());
}


QAction* QSplitButton::addAction(const QString &text, const QVariant &v)
{
    QAction* act = new QAction(text, parent());
	if (v.isNull())
		act->setData(text);
	else
		act->setData(v);

	m_localMenu->addAction(act);

	if (m_localMenu->actions().count() == 1)
		setDefaultAction(act);

    return act;
}


QAction* QSplitButton::addAction(const QIcon &icon, const QString &text, const QVariant &v)
{
	QAction* act = addAction(text, v);
	act->setIcon(icon);
    return act;
}


QAction* QSplitButton::selectAction(const QVariant &data)
{
    for (auto act: m_localMenu->actions())
    {
        if (act->data() == data && act->isEnabled() && act->isVisible())
        {
            setDefaultAction(act);
            return act;
        }
    }

    // not found
    return NULL;
}


QAction *QSplitButton::selectActionByIndex(int index)
{
    if (index >= 0 && index < m_localMenu->actions().count())
    {
        auto act = m_localMenu->actions()[index];

        if (act->isEnabled() && act->isVisible())
        {
            setDefaultAction(act);
            return act;
        }
    }

    // not found
    return NULL;
}


void QSplitButton::actionEvent(QActionEvent *event)
{
    QToolButton::actionEvent(event);

    // set default action
    if (defaultAction() == NULL)
    {
        for (auto act: m_localMenu->actions())
        {
            if (act->isEnabled() && act->isVisible())
            {
                setDefaultAction(act);
                return;
            }
        }
    }
}


void QSplitButton::onAction(QAction* act)
{
    setDefaultAction(act);

	if (act)
		Q_EMIT activated(act->data());
}


void QSplitButton::setDefaultAction(QAction* act)
{
	if (act != defaultAction())
	{
		// prevent status tip
		QString oldStatusTip = statusTip();

		QToolButton::setDefaultAction(act);
		m_localMenu->setDefaultAction(act);

		if (statusTip().isEmpty())
			setStatusTip(oldStatusTip);
	}
}


}
