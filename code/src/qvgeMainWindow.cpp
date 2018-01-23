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
#include <QPixmapCache>

#include <qvge/CFileSerializerGEXF.h>
#include <qvge/CFileSerializerGraphML.h>
#include <qvge/CFileSerializerXGR.h>

#include <base/CPlatformServices.h>


qvgeMainWindow::qvgeMainWindow()
{
	QString bitString;
	int bits = CPlatformServices::GetPlatformBits();
	if (bits > 0) bitString = QString(" %1bit").arg(bits);

    QApplication::setApplicationName("Qt Visual Graph Editor");
    QApplication::setApplicationVersion(qvgeVersion.toString() + tr(" (Beta)") + bitString);

    CDocumentFormat gexf = { "GEXF", "*.gexf", false, true };
    CDocumentFormat graphml = { "GraphML", "*.graphml", true, true };
//    CDocumentFormat gr = { "Old plain GR", "*.gr", false, true };
    CDocumentFormat xgr = { "XML Graph", "*.xgr", true, true };
    CDocument graph = { tr("Graph Document"), tr("Directed or undirected graph"), "graph", true, {gexf, graphml, xgr} };
    addDocument(graph);

    CDocumentFormat txt = { tr("Plain Text"), "*.txt", true, true };
    CDocument text = { tr("Text Document"), tr("Simple text document"), "text", true, {txt} };
    addDocument(text);
}


void qvgeMainWindow::init(int argc, char *argv[])
{
    Super::init(argc, argv);

    statusBar()->showMessage(tr("qvge started."));
}


bool qvgeMainWindow::onCreateNewDocument(const QByteArray &docType)
{
    // scene
    if (docType == "graph")
    {
        m_editorScene = new CNodeEditorScene(this);
        m_editorView = new CEditorView(m_editorScene, this);
        setCentralWidget(m_editorView);

        /*auto uiController = */new qvgeNodeEditorUIController(this, m_editorScene, m_editorView);

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


bool qvgeMainWindow::onOpenDocument(const QString &fileName, QByteArray &docType)
{
	// graphs formats
    if (fileName.toLower().endsWith(".graphml"))
    {
        docType = "graph";

        if (onCreateNewDocument(docType))
        {
			if (CFileSerializerGraphML().load(fileName, *m_editorScene))
			{
				m_editorScene->addUndoState();
				return true;
			}
        }

        return false;
    }


    if (fileName.toLower().endsWith(".gexf"))
    {
        docType = "graph";

        if (onCreateNewDocument(docType))
        {
			if (CFileSerializerGEXF().load(fileName, *m_editorScene))
			{
				m_editorScene->addUndoState();
				return true;
			}
        }
        
		return false;
    }


    if (fileName.toLower().endsWith(".xgr"))
    {
        docType = "graph";

        if (onCreateNewDocument(docType))
        {
            if (CFileSerializerXGR().load(fileName, *m_editorScene))
            {
                m_editorScene->addUndoState();
                return true;
            }
        }

        return false;
    }

	// fallback: load as text
	//if (fileName.toLower().endsWith(".txt"))
	{
		docType = "text";

		if (onCreateNewDocument(docType))
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


bool qvgeMainWindow::onSaveDocument(const QString &fileName, const QString &/*selectedFilter*/, const QByteArray &docType)
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

		if (extType == "xgr")
		{
			if (CFileSerializerXGR().save(fileName, *m_editorScene))
			{
				return true;
			}

			return false;
		}
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

	if (m_editorView)
	{
		bool isAA = m_editorView->renderHints().testFlag(QPainter::Antialiasing);
		isAA = settings.value("antialiasing", isAA).toBool();
		m_editorView->setRenderHint(QPainter::Antialiasing, isAA);

		int cacheRam = QPixmapCache::cacheLimit();
		cacheRam = settings.value("cacheRam", cacheRam).toInt();
		QPixmapCache::setCacheLimit(cacheRam);
	}
}


void qvgeMainWindow::doWriteSettings(QSettings& settings)
{
	Super::doWriteSettings(settings);

	if (m_editorView)
	{
		bool isAA = m_editorView->renderHints().testFlag(QPainter::Antialiasing);
		settings.setValue("antialiasing", isAA);

		int cacheRam = QPixmapCache::cacheLimit();
		settings.setValue("cacheRam", cacheRam);
	}
}
