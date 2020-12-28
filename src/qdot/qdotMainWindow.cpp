/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "qdotMainWindow.h"
#include "qdotVersion.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#include <appbase/CPlatformServices.h>


qdotMainWindow::qdotMainWindow(QWidget *parent): 
	Super(parent)
{
    QString bitString;
	int bits = CPlatformServices::GetPlatformBits();
	if (bits > 0) bitString = QString("%1bit").arg(bits);

    QApplication::setOrganizationName("qvge");
    QApplication::setApplicationName("Qt Visual GraphViz Assistent");
	QApplication::setApplicationVersion(qdotVersionString);
    QApplication::setApplicationDisplayName(QString("%1 %2 (%3)")
		.arg(QApplication::applicationName(), QApplication::applicationVersion(), bitString));

	//CDocumentFormat xgr = { "XGR binary graph format", "*.xgr", { "xgr" }, true, true };
	//CDocumentFormat gexf = { "GEXF", "*.gexf", {"gexf"}, true, true };
	//CDocumentFormat graphml = { "GraphML", "*.graphml", { "graphml" }, true, true };
    //CDocumentFormat gml = { "GML", "*.gml", { "gml" }, false, true };
    //CDocumentFormat csv = { "CSV text file", "*.csv", { "csv" }, false, true };
	CDocumentFormat dot = { "DOT/GraphViz", "*.dot *.gv", { "dot", "gv" }, true, true };
	CDocumentFormat dotplain = { "Plain DOT/GraphViz", "*.plain *.txt", { "plain", "txt" }, false, true };

    CDocument graph = { tr("GraphViz Document"), tr("Graph in GraphViz format"), "graphviz", true,
                        { dot, dotplain } };
    addDocument(graph);

    //CDocumentFormat txt = { tr("Plain text file"), "*.txt", { "txt" }, true, true };
    //CDocument text = { tr("Text Document"), tr("Simple text document"), "text", true, {txt} };
    //addDocument(text);

	// default files associations
	//updateFileAssociations();
}


qdotMainWindow::~qdotMainWindow()
{
	// causes crash here
	//destroyDocument();
}


void qdotMainWindow::init(const QStringList& args)
{
	// check portable start
	QString localINI = QCoreApplication::applicationDirPath() + "/qdot.ini";
	m_portable = (QFile::exists(localINI));

    Super::init(args);

	if (m_portable)
		statusBar()->showMessage(tr("qdot started (portable edition)."));
	else
		statusBar()->showMessage(tr("qdot started."));
}


QSettings& qdotMainWindow::getApplicationSettings() const
{
	if (m_portable)
	{
		static QString localINI = QCoreApplication::applicationDirPath() + "/qdot.ini";
		static QSettings localSettings(localINI, QSettings::IniFormat);
		return localSettings;
	}

	return CMainWindow::getApplicationSettings();
}


void qdotMainWindow::createStartPage()
{

}

bool qdotMainWindow::createDocument(const QByteArray &docType)
{
    // scene
    if (docType == "graphviz")
    {

        return true;
    }

    // unknown type
    return false;
}


void qdotMainWindow::destroyDocument()
{

}


void qdotMainWindow::onNewDocumentCreated(const QByteArray &docType)
{
	// wizard
	if (docType == "graphviz")
	{
	}
}


bool qdotMainWindow::openDocument(const QString &fileName, QByteArray &docType)
{
	QString format = QFileInfo(fileName).suffix().toLower();

	// graph formats
    if (docType == "graphviz")
	{
		QString lastError;

        if (createDocument(docType))
		{
			// ...
			return true;
		}

		if (lastError.size())
		{
			QMessageBox::critical(NULL, fileName, lastError);
		}

		return false;
	}
	
	// unknown format

    return false;
}


bool qdotMainWindow::saveDocument(const QString &fileName, const QString &/*selectedFilter*/, const QByteArray &docType)
{
    // graph
	if (docType == "graphviz")
	{

	}

    // unknown type
    return false;
}


QString qdotMainWindow::getAboutText() const
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
            "<br>&nbsp; - GraphViz &copy; <i>GraphViz development team</i>"
		);
}


void qdotMainWindow::doReadSettings(QSettings& settings)
{
	Super::doReadSettings(settings);

}


void qdotMainWindow::doWriteSettings(QSettings& settings)
{
	Super::doWriteSettings(settings);

}


// privates

void qdotMainWindow::updateFileAssociations()
{

}
