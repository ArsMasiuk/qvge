#pragma once

#include <QToolButton>
#include <QIcon>
#include <QVariant>
#include <QAction>


namespace QSint
{


class QMenuButton : public QToolButton
{
    Q_OBJECT

public:
    QMenuButton(QWidget *parent = Q_NULLPTR);

    QAction* addAction(const QString &text, const QVariant &data = QVariant());
    QAction* addAction(const QIcon &icon, const QString &text, const QVariant &data = QVariant());

	void setDefaultAction(QAction* act);

public Q_SLOTS:
    QAction* selectAction(const QVariant &data);
    QAction* selectActionByIndex(int index);

Q_SIGNALS:
    void activated(QVariant data);

protected Q_SLOTS:
	virtual void onAction(QAction* act);

protected:
    virtual void actionEvent(QActionEvent *event);

	QMenu *m_localMenu;
};


}
