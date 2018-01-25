/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef QVGEMAINWINDOW_H
#define QVGEMAINWINDOW_H

#include <QPlainTextEdit>
#include <QStatusBar>

#include <base/CMainWindow.h>


class qvgeMainWindow : public CMainWindow
{
    Q_OBJECT

public:
    typedef CMainWindow Super;

	friend class qvgeNodeEditorUIController;

    qvgeMainWindow();

    virtual void init(int argc, char *argv[]);

protected:
    virtual bool сreateDocument(const QByteArray &docType);
	virtual void onNewDocumentCreated(const QByteArray &docType);
    virtual bool openDocument(const QString &fileName, QByteArray &docType);
    virtual bool saveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType);

	virtual QString getAboutText() const;

	virtual void doReadSettings(QSettings& settings);
	virtual void doWriteSettings(QSettings& settings);
	
private:
	qvgeNodeEditorUIController *m_graphEditController = NULL;

    QPlainTextEdit *m_textEditor = NULL;
};

#endif // QVGEMAINWINDOW_H
