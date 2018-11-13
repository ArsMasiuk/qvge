#ifndef QFONTBUTTON_H
#define QFONTBUTTON_H

#include <QToolButton>
#include <QWidgetAction>
#include <QFontDialog>
#include <QMenu>


namespace QSint
{


class QFontButton : public QToolButton
{
    Q_OBJECT

public:
    QFontButton(QWidget *parent = Q_NULLPTR);

	const QFont& currentFont() const { return m_font; }

	static QString fontToText(const QFont& font);

public Q_SLOTS:
	void setCurrentFont(const QFont &font);

Q_SIGNALS:
    void activated(const QFont &font);

private Q_SLOTS:
    void onClicked();
    void onDialogShown();
    void onDialogFontSelected(const QFont&);

private:
    QFontDialog *m_fontDialog;
	QFont m_font;
};


}

#endif // QFONTBUTTON_H
