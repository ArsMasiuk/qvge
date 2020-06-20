#include "splitbutton.h"

#include <QVBoxLayout>


namespace QSint
{


SplitButton::SplitButton(QWidget *parent) : QWidget(parent)
{
    m_button = new QToolButton(this);
    m_button->setObjectName("Button");
    m_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    m_button->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_split = new QToolButton(this);
    m_split->setObjectName("Split");
    m_split->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_split->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_split->setPopupMode(QToolButton::InstantPopup);

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setContentsMargins(0,0,0,0);
    vbl->setSpacing(0);
    setLayout(vbl);
    vbl->addWidget(m_button);
    vbl->addWidget(m_split);

    m_menu = new QMenu(this);
    m_split->setMenu(m_menu);

    connect(m_menu, &QMenu::aboutToShow, this, &SplitButton::onMenuShow);
    connect(m_menu, &QMenu::triggered, this, &SplitButton::onMenuTriggered);
    connect(m_button, &QToolButton::clicked, this, &SplitButton::onButtonClicked);

    setIconSize(32);
    m_split->setMinimumHeight(24);

    // style
    setStyleSheet(
        "QToolButton#Button{"
        "border: 1px solid #ccc; border-top-left-radius: 5px; border-top-right-radius: 5px; "
        "background: #ddd;"
        "}"

        "QToolButton#Split{"
        "border: 1px solid #ccc; border-top-color: #ddd; border-bottom-right-radius: 5px; border-bottom-left-radius: 5px;"
        "background: #ddd;"
        "}"

        "QToolButton#Button:hover, QToolButton#Split:hover"
        "{"
        "background: #def; border-color: #09e;"
        "}"

        "QToolButton#Button:clicked, QToolButton#Split:clicked"
        "{"
        "background: #09e;"
        "}"
    );
}


void SplitButton::onMenuShow()
{
     m_menu->clear();
     m_menu->addActions(actions());
     m_menu->setDefaultAction(m_button->defaultAction());
}


void SplitButton::onMenuTriggered(QAction *action)
{
    if (m_setLast)
    {
        setDefaultAction(action);
    }
}


void SplitButton::onButtonClicked()
{
    const auto& acts = actions();

    if (m_toggle && acts.count())
    {
        int index = acts.indexOf(m_button->defaultAction());
        if (index < 0 || index == acts.count()-1)
            setDefaultAction(acts.first());
        else
            setDefaultAction(acts.at(index+1));
    }
}


void SplitButton::setDefaultAction(QAction *action)
{
    // Qt bug workaround: setDefaultAction is adding action to actions
    while (m_button->actions().count())
        m_button->removeAction(m_button->actions().first());

    m_button->setDefaultAction(action);

    m_split->setDefaultAction(action);
}


void SplitButton::setIconSize(int size)
{
    m_button->setIconSize(QSize(size, size));
}


void SplitButton::setLastActionDefault(bool on)
{
    m_setLast = on;
}


void SplitButton::setActionsToggle(bool on)
{
    m_toggle = on;
}


}
