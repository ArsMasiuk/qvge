#include "qfontbutton.h"


namespace QSint
{


QString QFontButton::fontToText(const QFont& font)
{
	QString result(font.family());

	if (font.pointSizeF() > 0)
		result += QString(",%1pt").arg(font.pointSizeF());
	else
		result += QString(",%1px").arg(font.pixelSize());

	if (font.bold())		result += ",Bold";
	if (font.italic())		result += ",Italic";
	if (font.underline())	result += ",Underline";

	return result;
}


QFontButton::QFontButton(QWidget *parent) : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setPopupMode(QToolButton::MenuButtonPopup);

    setText("Aa");
	setCurrentFont(QFont());

    connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));

    m_fontDialog = new QFontDialog(this);
    m_fontDialog->setOption(QFontDialog::DontUseNativeDialog, false);
    m_fontDialog->setSizeGripEnabled(false);

    QMenu *menu = new QMenu(this);
    setMenu(menu);

    QWidgetAction *dialogAction = new QWidgetAction(this);
    dialogAction->setDefaultWidget(m_fontDialog);
    menu->addAction(dialogAction);

    connect(menu, SIGNAL(aboutToShow()), this, SLOT(onDialogShown()));
    connect(m_fontDialog, SIGNAL(accepted()), menu, SLOT(hide()));
    connect(m_fontDialog, SIGNAL(rejected()), menu, SLOT(hide()));
    connect(m_fontDialog, SIGNAL(fontSelected(const QFont&)), this, SLOT(onDialogFontSelected(const QFont&)));
}


void QFontButton::setCurrentFont(const QFont &font)
{
	m_font = font;

	setToolTip(fontToText(font));

	if (m_font.pixelSize() > 20 || m_font.pointSize() > 20)
	{
		QFont f(font);
		f.setPointSize(20);
		setFont(f);
	}
	else
		setFont(m_font);
}


void QFontButton::onClicked()
{
    Q_EMIT activated(m_font);
}


void QFontButton::onDialogShown()
{
    m_fontDialog->setCurrentFont(m_font);

    m_fontDialog->setVisible(true);
}


void QFontButton::onDialogFontSelected(const QFont &font)
{
    setCurrentFont(font);

    onClicked();
}


}
