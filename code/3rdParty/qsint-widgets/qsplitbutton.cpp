#include "qsplitbutton.h"

#include <QMenu>
#include <QActionEvent>


namespace QSint
{


QSplitButton::QSplitButton(QWidget *parent) : QToolButton(parent)
{
    connect(this, SIGNAL(triggered(QAction*)), this, SLOT(onAction(QAction*)));
}


QAction* QSplitButton::addAction(const QString &text, const QVariant &v)
{
    QAction* act = new QAction(text, parent());
    act->setData(v);
    QToolButton::addAction(act);
    return act;
}


QAction* QSplitButton::addAction(const QIcon &icon, const QString &text, const QVariant &v)
{
    QAction* act = new QAction(icon, text, parent());
    act->setData(v);
    QToolButton::addAction(act);
    return act;
}


QAction* QSplitButton::selectAction(const QVariant &data)
{
    for (auto act: actions())
    {
        if (act->data() == data && act->isEnabled() && act->isVisible())
        {
            setDefaultAction(act);

            if (menu())
                menu()->setDefaultAction(act);

            return act;
        }
    }

    // not found
    return NULL;
}


QAction *QSplitButton::selectActionByIndex(int index)
{
    if (index >= 0 && index < actions().count())
    {
        auto act = actions()[index];

        if (act->isEnabled() && act->isVisible())
        {
            setDefaultAction(act);

            if (menu())
                menu()->setDefaultAction(act);

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
        for (auto act: actions())
        {
            if (act->isEnabled() && act->isVisible())
            {
                setDefaultAction(act);

                if (menu())
                    menu()->setDefaultAction(act);

                return;
            }
        }
    }
}


void QSplitButton::onAction(QAction* act)
{
    setDefaultAction(act);

    if (menu())
        menu()->setDefaultAction(act);

    Q_EMIT activated(act->data());
}


}
