#ifndef CSPLITBUTTON_H
#define CSPLITBUTTON_H

#include <QWidget>
#include <QToolButton>
#include <QMenu>


namespace QSint
{


/**
 * @brief The SplitButton class represents Microsoft Office-like "Split Button" control which consists
 * of a clickable button itself and a drop-down list of some common actions connected with the button.
 *
 * Clicking on the button fires its default action (set via setDefaultAction() method).
 * Depending of the operation modes, default button action can be set automatically after choosing an option
 * from the drop-down list (see setLastActionDefault()) or can be toggled to the another one action
 * after the click on the button (see setActionsToggle()).
 *
 * \since 0.4
 */
class SplitButton : public QWidget
{
    Q_OBJECT

public:
    explicit SplitButton(QWidget *parent = nullptr);

    /// Set button's icon size to \a size.
    void setIconSize(int size);
    /// Set action \a action as default (it will be invoked after clicking on the button).
    void setDefaultAction(QAction *action);
    /// If \a on, the recently chosen action from the list will be set as default.
    void setLastActionDefault(bool on);
    /// If \a on, clicking on the button will automatically advance to the next action in the list.
    void setActionsToggle(bool on);

signals:

private slots:
    void onMenuShow();
    void onMenuTriggered(QAction *action);
    void onButtonClicked();

private:
    QToolButton *m_button;
    QToolButton *m_split;
    QMenu *m_menu;
    bool m_setLast = false;
    bool m_toggle = false;
};


}


#endif // CSPLITBUTTON_H
