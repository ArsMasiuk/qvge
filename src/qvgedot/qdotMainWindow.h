/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QPlainTextEdit>
#include <QStatusBar>

#include <appbase/CMainWindow.h>


class qdotMainWindow : public CMainWindow
{
    Q_OBJECT

public:
    typedef CMainWindow Super;

	qdotMainWindow(QWidget *parent = nullptr);
	virtual ~qdotMainWindow();

    virtual void init(const QStringList& args);

	virtual QSettings& getApplicationSettings() const;

protected:
	virtual void createStartPage();

    virtual bool createDocument(const QByteArray &docType);
	virtual void destroyDocument();
	virtual void onNewDocumentCreated(const QByteArray &docType);
    virtual bool openDocument(const QString &fileName, QByteArray &docType);
    virtual bool saveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType);

	virtual QString getAboutText() const;

	virtual void doReadSettings(QSettings& settings);
	virtual void doWriteSettings(QSettings& settings);
	
private:
	void updateFileAssociations();

    QPlainTextEdit *m_textEditor = nullptr;

	bool m_portable = false;
};

