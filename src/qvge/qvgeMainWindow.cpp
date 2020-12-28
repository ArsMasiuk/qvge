﻿/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "qvgeMainWindow.h"
#include "qvgeVersion.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#include <appbase/CPlatformServices.h>
#include <commonui/CNodeEditorUIController.h>


qvgeMainWindow::qvgeMainWindow(QWidget *parent): 
	Super(parent)
{
    QString bitString;
	int bits = CPlatformServices::GetPlatformBits();
	if (bits > 0) bitString = QString("%1bit").arg(bits);

    QApplication::setOrganizationName("qvge");
    QApplication::setApplicationName("Qt Visual Graph Editor");
	QApplication::setApplicationVersion(qvgeVersionString);
    QApplication::setApplicationDisplayName(QString("%1 %2 (%3)")
		.arg(QApplication::applicationName(), QApplication::applicationVersion(), bitString));

	CDocumentFormat xgr = { "XGR binary graph format", "*.xgr", { "xgr" }, true, true };
	CDocumentFormat gexf = { "GEXF", "*.gexf", {"gexf"}, true, true };
	CDocumentFormat graphml = { "GraphML", "*.graphml", { "graphml" }, true, true };
    CDocumentFormat gml = { "GML", "*.gml", { "gml" }, false, true };
    CDocumentFormat csv = { "CSV text file", "*.csv", { "csv" }, false, true };
	CDocumentFormat dot = { "DOT/GraphViz", "*.dot *.gv", { "dot", "gv" }, true, true };
	CDocumentFormat dotplain = { "Plain DOT/GraphViz", "*.plain *.txt", { "plain", "txt" }, false, true };

    CDocument graph = { tr("Graph Document"), tr("Directed or undirected graph"), "graph", true,
                        { xgr, gexf, graphml, gml, csv, dot, dotplain } };
    addDocument(graph);

    //CDocumentFormat txt = { tr("Plain text file"), "*.txt", { "txt" }, true, true };
    //CDocument text = { tr("Text Document"), tr("Simple text document"), "text", true, {txt} };
    //addDocument(text);

	// default files associations
	updateFileAssociations();
}


qvgeMainWindow::~qvgeMainWindow()
{
	// causes crash here
	//destroyDocument();
}


void qvgeMainWindow::init(const QStringList& args)
{
	// check portable start
	QString localINI = QCoreApplication::applicationDirPath() + "/qvge.ini";
	m_portable = (QFile::exists(localINI));

    Super::init(args);

	if (m_portable)
		statusBar()->showMessage(tr("qvge started (portable edition)."));
	else
		statusBar()->showMessage(tr("qvge started."));
}


QSettings& qvgeMainWindow::getApplicationSettings() const
{
	if (m_portable)
	{
		static QString localINI = QCoreApplication::applicationDirPath() + "/qvge.ini";
		static QSettings localSettings(localINI, QSettings::IniFormat);
		return localSettings;
	}

	return CMainWindow::getApplicationSettings();
}


bool qvgeMainWindow::createDocument(const QByteArray &docType)
{
    // scene
    if (docType == "graph")
    {
		if (m_graphEditController == nullptr)
		{
			m_graphEditController = new CNodeEditorUIController(this);
			m_graphEditController->doReadSettings(getApplicationSettings());
		}

        return true;
    }

    // text
    if (docType == "text")
    {
		if (m_textEditor == nullptr)
		{
			m_textEditor = new QPlainTextEdit(this);
			setCentralWidget(m_textEditor);

			connect(m_textEditor, &QPlainTextEdit::textChanged, this, &CMainWindow::onDocumentChanged);
		}

        return true;
    }

    // unknown type
    return false;
}


void qvgeMainWindow::destroyDocument()
{
	if (m_graphEditController)
	{
		m_graphEditController->disconnect();
		delete m_graphEditController;
		m_graphEditController = nullptr;
	}

	if (m_textEditor)
	{
		m_textEditor->disconnect();
		delete m_textEditor;
		m_textEditor = nullptr;
	}
}


void qvgeMainWindow::onNewDocumentCreated(const QByteArray &docType)
{
	// wizard
	if (docType == "graph")
	{
        m_graphEditController->onNewDocumentCreated();
	}
}


bool qvgeMainWindow::openDocument(const QString &fileName, QByteArray &docType)
{
	QString format = QFileInfo(fileName).suffix().toLower();

	// graph formats
    if (docType == "graph")
	{
		QString lastError;

        if (createDocument(docType))
		{
			if (m_graphEditController->loadFromFile(format, fileName, &lastError))
			{
				m_graphEditController->onDocumentLoaded(fileName);
				return true;
			}

			// terminate incomplete document
			//destroyDocument();
		}

		if (lastError.size())
		{
			QMessageBox::critical(NULL, fileName, lastError);
		}

		return false;
	}

	// fallback: load as text
	//if (fileName.toLower().endsWith(".txt"))
	{
		docType = "text";

        if (createDocument(docType))
		{
			QFile f(fileName);
			if (f.open(QFile::ReadOnly))
			{
				QTextStream ts(&f);
				m_textEditor->setPlainText(ts.readAll());
				f.close();
				return true;
			}

			// terminate incomplete document
			//destroyDocument();
		}
	}

    return false;
}


bool qvgeMainWindow::saveDocument(const QString &fileName, const QString &/*selectedFilter*/, const QByteArray &docType)
{
    // text
    if (docType == "text")
    {
        QFile f(fileName);
        if (f.open(QFile::WriteOnly))
        {
            QTextStream ts(&f);
            ts << m_textEditor->toPlainText();
            f.close();
            return true;
        }
        return false;
    }

    // graph
	if (docType == "graph")
	{
		QString extType = QFileInfo(fileName).suffix().toLower();

		QString lastError;	// TODO

        return m_graphEditController->saveToFile(extType, fileName, &lastError);
	}

    // unknown type
    return false;
}


QString qvgeMainWindow::getAboutText() const
{
	return Super::getAboutText()
		+ QString(
			"<p>This is a free software."
			"<br>It comes without warranty of any kind. Use it on your own risk."
			"<p>&copy; 2016-2020 Ars L. Masiuk"
			"<hr>"
			"<p><i>Credits:</i>"
			"<br>&nbsp; - Qt framework &copy; <i>The Qt Company Ltd</i>"
			"<br>&nbsp; - Qt property browser framework &copy; <i>The Qt Company Ltd</i>"
			"<br>&nbsp; - QSint widgets library &copy; <i>Sintegrial Technologies</i>"
            "<br>&nbsp; - QProcessInfo &copy; <i>Baldur Karlsson</i>"
			"<br>&nbsp; - menu & toolbar graphics &copy; <i>Inkscape project</i>"
#ifdef USE_OGDF
            "<br>&nbsp; - OGDF &copy; <i>OGDF development team</i>"
#endif
		);
}


void qvgeMainWindow::doReadSettings(QSettings& settings)
{
	Super::doReadSettings(settings);

	if (m_graphEditController)
	{
		m_graphEditController->doReadSettings(settings);
	}
}


void qvgeMainWindow::doWriteSettings(QSettings& settings)
{
	Super::doWriteSettings(settings);

	if (m_graphEditController)
	{
		m_graphEditController->doWriteSettings(settings);
	}
}


// privates

void qvgeMainWindow::updateFileAssociations()
{
#if defined Q_OS_WIN32

	CPlatformWin32::registerFileType("qvge.xgr", "QVGE native graph document", ".xgr", 0);

#elif defined Q_OS_LINUX

	// assuming application-xgr has been already added
    //QSettings mimeapps("/usr/share/applications/defaults.list", QSettings::NativeFormat);
    QSettings mimeapps(QDir::homePath() + "/.config/mimeapps.list", QSettings::IniFormat);
    mimeapps.beginGroup("Default Applications");
	mimeapps.setValue("application/xgr", "qvge.desktop");
	mimeapps.endGroup();
    mimeapps.sync();
    qDebug() << mimeapps.status();

#endif
}
