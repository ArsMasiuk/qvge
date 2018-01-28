/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "qvgeMainWindow.h"
#include "qvgeNodeEditorUIController.h"
#include "qvgeVersion.h"

#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>

#include <base/CPlatformServices.h>


qvgeMainWindow::qvgeMainWindow()
{
	QString bitString;
	int bits = CPlatformServices::GetPlatformBits();
	if (bits > 0) bitString = QString(" %1bit").arg(bits);

    QApplication::setApplicationName("Qt Visual Graph Editor");
    QApplication::setApplicationVersion(qvgeVersion.toString() + tr(" (Beta)") + bitString);

	CDocumentFormat gexf = { "GEXF", "*.gexf", {"gexf"}, false, true };
	CDocumentFormat graphml = { "GraphML", "*.graphml", {"graphml"}, false, true };
//    CDocumentFormat gr = { "Old plain GR", "*.gr", false, true };
	CDocumentFormat xgr = { "XML Graph", "*.xgr", {"xgr"}, true, true };
    CDocumentFormat gml = { "GML", "*.gml", { "gml" }, false, true };
    CDocumentFormat dot = { "DOT", "*.dot *.gv", { "dot","gv" }, true, true };
    CDocument graph = { tr("Graph Document"), tr("Directed or undirected graph"), "graph", true,
                        {gexf, graphml, gml, dot, xgr} };
    addDocument(graph);

    CDocumentFormat txt = { tr("Plain Text"), "*.txt", { "txt" }, true, true };
    CDocument text = { tr("Text Document"), tr("Simple text document"), "text", true, {txt} };
    addDocument(text);
}


void qvgeMainWindow::init(int argc, char *argv[])
{
    Super::init(argc, argv);

    statusBar()->showMessage(tr("qvge started."));
}


bool qvgeMainWindow::createDocument(const QByteArray &docType)
{
    // scene
    if (docType == "graph")
    {
		m_graphEditController = new qvgeNodeEditorUIController(this);

        // restore settings for this instance
        readSettings();

        return true;
    }

    // text
    if (docType == "text")
    {
        m_textEditor = new QPlainTextEdit(this);
        setCentralWidget(m_textEditor);

        connect(m_textEditor, &QPlainTextEdit::textChanged, this, &CMainWindow::onDocumentChanged);

        // restore settings for this instance
        readSettings();

        return true;
    }

    // unknown type
    return false;
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
    if (format == "graphml" || format == "gexf" || format == "xgr" || format == "gml" || format == "dot")
	{
		docType = "graph";

        if (createDocument(docType) && m_graphEditController->loadFromFile(fileName, format))
		{
			return true;
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

        return m_graphEditController->saveToFile(fileName, extType);
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
			"<p>&copy; 2016-2018 Ars L. Masiuk");
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
