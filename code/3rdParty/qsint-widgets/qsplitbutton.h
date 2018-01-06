#pragma once

#include <QToolButton>
#include <QIcon>
#include <QVariant>
#include <QAction>


namespace QSint
{


class QSplitButton : public QToolButton
{
    Q_OBJECT

public:
    QSplitButton(QWidget *parent = Q_NULLPTR);

    QAction* addAction(const QString &text, const QVariant &data = QVariant());
    QAction* addAction(const QIcon &icon, const QString &text, const QVariant &data = QVariant());

public Q_SLOTS:
    QAction* selectAction(const QVariant &data);
    QAction* selectActionByIndex(int index);

Q_SIGNALS:
    void activated(QVariant data);

protected:
    virtual void actionEvent(QActionEvent *event);

private Q_SLOTS:
    void onAction(QAction* act);
};


}
