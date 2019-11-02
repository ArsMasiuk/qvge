#include "CMainWindow.h"
#include "CPlatformServices.h"
#include "CStartPage.h"

#include <QFileDialog>
#include <QDockWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QInputDialog>
#include <QTimer>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QApplication>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QCloseEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QToolButton>


CMainWindow::CMainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_isChanged(false)
{ 
	qint64 pid = qApp->applicationPid();
	m_stringPID = QString::number(pid);

	QApplication::setOrganizationName("home");
	QApplication::setApplicationName("application");

	setAcceptDrops(true);

	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(onQuit()));
}


CMainWindow::~CMainWindow()
{
}


void CMainWindow::onQuit()
{
	removeInstance();
}


void CMainWindow::exit()
{
	// close all instances
	QVariantMap pidFileMap = getActiveInstances();
	for (auto it = pidFileMap.constBegin(); it != pidFileMap.constEnd(); ++it)
	{
		bool isCurrent = (m_stringPID == it.key());
		if (!isCurrent)
		{
			QVariantMap dataMap = it.value().value<QVariantMap>();
			CPlatformServices::CloseWindow(dataMap["hwnd"].toUInt());
		}
	}

	// close me as well
	close();
}


void CMainWindow::addDocument(const CDocument& doc)
{
    if (doc.canCreate)
    {
        m_docTypeCreate << doc.type;
    }

    m_docTypes[doc.type] = doc;
}


QDockWidget* CMainWindow::createDockWindow(const QString& name, const QString& title, Qt::DockWidgetArea area, QWidget* widget)
{
	QDockWidget *docker = new QDockWidget(title);
	docker->setObjectName(name);

	addDockWidget(area, docker);
	if (area == Qt::NoDockWidgetArea)
		docker->setFloating(true);

	docker->setWidget(widget);

	return docker;
}


// protected

void CMainWindow::init(const QStringList& args)
{
    createMainMenu();

    createFileToolbar();

	createWindowsMenu();

    createHelpMenu();

    readSettings();

	createStartPage();

    processParams(args);
}


void CMainWindow::closeEvent(QCloseEvent *event)
{
	if (saveOnExit()) 
	{
		writeSettings();
		event->accept();
	}
	else 
	{
		event->ignore();
	}
}


void CMainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	event->acceptProposedAction();
}


void CMainWindow::dragMoveEvent(QDragMoveEvent* event)
{
	event->acceptProposedAction();
}


void CMainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}


void CMainWindow::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();

	// check for our needed mime type, here a file or a list of files
	if (mimeData->hasUrls())
	{
		QList<QUrl> urlList = mimeData->urls();

		// extract the local paths of the files
		for (int i = 0; i < urlList.size() && i < 32; ++i)
		{
			QString filePath = urlList.at(i).toLocalFile();
			if (!filePath.isEmpty())
				doOpenDocument(filePath);
		}
	}
}


void CMainWindow::processParams(const QStringList& args)
{
    if (args.count() >= 3)
    {
        QString command(args.at(1));

        if (command == "create")
        {
            createNewDocument(args.at(2).toLocal8Bit());
            return;
        }

        if (command == "open")
        {
            doOpenDocument(args.at(2));
            return;
        }
    }

	// assotiated file?
	if (args.count() == 2)
	{
		doOpenDocument(args.at(1));
		return;
	}

	onCurrentFileChanged();
}


void CMainWindow::createMainMenu()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));

    m_newDocument = m_fileMenu->addAction(QIcon(":/Icons/New"), tr("&New"));
    m_newDocument->setStatusTip(tr("Create new document"));

    fillNewFileMenu();

    m_openDocument = m_fileMenu->addAction(QIcon(":/Icons/Open"), tr("&Open..."), this, SLOT(on_actionOpen_triggered()));
    m_openDocument->setStatusTip(tr("Open a document"));
    m_openDocument->setShortcut(QKeySequence::Open);

    m_saveDocument = m_fileMenu->addAction(QIcon(":/Icons/Save"), tr("&Save"), this, SLOT(on_actionSave_triggered()));
    m_saveDocument->setStatusTip(tr("Save current document"));
    m_saveDocument->setShortcut(QKeySequence::Save);

    m_saveAsDocument = m_fileMenu->addAction(tr("Save &As..."), this, SLOT(on_actionSaveAs_triggered()));
    m_saveAsDocument->setStatusTip(tr("Save current document under another name"));
    m_saveAsDocument->setShortcut(QKeySequence::SaveAs);

	m_exportDocument = m_fileMenu->addAction(QIcon(":/Icons/Export"), tr("&Export..."));
	m_exportDocument->setStatusTip(tr("Export current document into another format"));
	m_exportDocument->setVisible(false); 

    m_fileMenu->addSeparator();

	m_recentFilesMenu = m_fileMenu->addMenu(tr("Recent Files"));
	connect(m_recentFilesMenu, SIGNAL(aboutToShow()), this, SLOT(fillRecentFilesMenu()));
	connect(m_recentFilesMenu, SIGNAL(triggered(QAction*)), this, SLOT(onRecentFilesMenuAction(QAction*)));

	m_fileMenu->addSeparator();

	QAction *closeDocument = m_fileMenu->addAction(tr("Close"), this, SLOT(close()));
	closeDocument->setStatusTip(tr("Close current document"));
	closeDocument->setShortcut(QKeySequence::Close);

    QAction *exitApp = m_fileMenu->addAction(tr("E&xit"), this, SLOT(exit()));
    exitApp->setStatusTip(tr("Leave the application closing all windows"));
    exitApp->setShortcut(QKeySequence::Quit);
}


void CMainWindow::fillNewFileMenu()
{
    if (m_docTypeCreate.isEmpty())
        return;

    if (m_docTypeCreate.count() == 1)
    {
        const CDocument& doc = m_docTypes[*m_docTypeCreate.begin()];
		m_newDocument->setText(tr("New"));
        m_newDocument->setToolTip(tr("New") + " " + doc.name);
        m_newDocument->setStatusTip(doc.description);
		m_newDocument->setShortcut(QKeySequence::New);

        connect(m_newDocument, SIGNAL(triggered()), this, SLOT(createNewDocument()));
        m_newDocument->setEnabled(true);
    }
    else
    {
        QMenu *newActionsMenu = new QMenu();
        m_newDocument->setMenu(newActionsMenu);

        for (const QByteArray& docType : m_docTypeCreate)
        {
            const CDocument& doc = m_docTypes[docType];
            QAction *newAction = newActionsMenu->addAction(doc.name);
            newAction->setData(docType);
            newAction->setStatusTip(doc.description);
        }

		newActionsMenu->setDefaultAction(newActionsMenu->actions().first());
		connect(m_newDocument, SIGNAL(triggered()), newActionsMenu->defaultAction(), SIGNAL(triggered()));

        connect(newActionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(createNewDocument(QAction*)));
        m_newDocument->setEnabled(true);
    }
}


void CMainWindow::createFileToolbar()
{
    QToolBar *fileToolbar = addToolBar(tr("File"));
    fileToolbar->setObjectName("fileToolbar");
    fileToolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    fileToolbar->addAction(m_newDocument);
    fileToolbar->addAction(m_openDocument);
    fileToolbar->addAction(m_saveDocument);
}


void CMainWindow::createStartPage()
{
	CStartPage *startPage = new CStartPage(this);
	setCentralWidget(startPage);
}


void CMainWindow::updateTitle()
{
    setWindowTitle(QString("%1 - %2")
                   .arg(m_mainTitleText, QApplication::applicationDisplayName())
    );
}


void CMainWindow::updateActions()
{
    m_saveDocument->setEnabled(m_currentDocType.size());
    m_saveAsDocument->setEnabled(m_currentDocType.size());
}


void CMainWindow::onDocumentChanged()
{
    // already changed - do nothing
    if (m_isChanged)
        return;

    m_isChanged = true;

	onCurrentFileChanged();
}


void CMainWindow::onCurrentFileChanged()
{
	m_mainTitleText = m_currentFileName;

	if (m_currentFileName.isEmpty())
		m_mainTitleText = tr("New File");

	if (m_isChanged)
		m_mainTitleText = "* " + m_mainTitleText;

	updateTitle();
	updateActions();
	updateInstance();
	updateRecentFiles();
}


// actions

void CMainWindow::createNewDocument()
{
    createNewDocument(*m_docTypeCreate.begin());
}


void CMainWindow::createNewDocument(QAction *act)
{
    QByteArray docType = act->data().toByteArray();
    createNewDocument(docType);
}


void CMainWindow::createNewDocument(const QByteArray &docType)
{
    // document presents - run new instance
    if (m_currentDocType.size())
    {
        QStringList args;
        args << "create" << docType;
        QProcess::startDetached(QCoreApplication::applicationFilePath(), args);

        return;
    }

    // no document - create in place
    if (createDocument(docType))
    {
		// restore settings for this instance
		readSettings();

        m_currentDocType = docType;

		onNewDocumentCreated(docType);

		m_isChanged = false;

		onCurrentFileChanged();

        return;
    }

    // failed
    qDebug() << "Cannot create document.";
}


bool CMainWindow::createDocument(const QByteArray &docType)
{
    qDebug() << docType;

    return true;
}


void CMainWindow::selectAndOpenDocument()
{
	on_actionOpen_triggered();
}


bool CMainWindow::openDocument(const QString &fileName)
{
	return doOpenDocument(fileName);
}


void CMainWindow::on_actionOpen_triggered()
{
    QString title = tr("Open File");
    QString filter = tr("Any File (*.*)");
    onOpenDocumentDialog(title, filter);

	QString loadName = m_currentFileName;
	if (loadName.isEmpty())
		loadName = m_lastPath;
    if (loadName.isEmpty())
        loadName = QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(NULL, title, loadName, filter, &m_lastOpenFilter);
    if (fileName.isEmpty())
        return;

    doOpenDocument(fileName);
}


bool CMainWindow::getDocFormatFromName(const QString &normalizedName, const CDocument **doc, const CDocumentFormat **fmt, QString *suffix)
{
	const QString ext = QFileInfo(normalizedName).suffix().toLower();

	for (const auto &docType : m_docTypes)
	{
		for (int i = 0; i < docType.formats.size(); ++i)
		{
			const auto &format = docType.formats.at(i);
			if (format.suffixes.contains(ext))
			{
				if (doc) *doc = &docType;
				if (fmt) *fmt = &format;
				if (suffix) *suffix = ext;
				return true;
			}
		}
	}

	// invalid format
	return false;
}


bool CMainWindow::doOpenDocument(const QString &fileName)
{
	QString normalizedName = QDir::toNativeSeparators(QFileInfo(fileName).canonicalFilePath());

    // file does not exist
    if (!QFile::exists(normalizedName))
    {
		statusBar()->showMessage(tr("Failed to open: %1").arg(fileName));

        QMessageBox::critical(NULL, 
			fileName,
			tr("Document file does not exist or not accessible.")
		);

        return false;
    }

	// check if the document already opened in another instance
	if (activateInstance(normalizedName))
		return true;

    // document presents - run new instance
    if (m_currentDocType.size())
    {
		// store current settings to be read by the new instance
		writeSettings();

        QStringList args;
        args << "open" << normalizedName;
        QProcess::startDetached(QCoreApplication::applicationFilePath(), args);

        return true;
    }

	// guess document type (TO REVISE!)
	const CDocument *fdoc; 
	const CDocumentFormat *fformat;
	QString fsuffix;
	bool found = getDocFormatFromName(normalizedName, &fdoc, &fformat, &fsuffix);

	QByteArray fileDocType;
	if (found) {
		fileDocType = fdoc->type;
	}

    // open in place (fileDocType can be changed by this fn!)
    if (openDocument(normalizedName, fileDocType))
    {
		// restore settings for this instance
		readSettings();

        m_currentFileName = normalizedName;
		m_currentDocType = fileDocType;
        m_isChanged = false;
		m_lastPath = QFileInfo(m_currentFileName).absolutePath();

        statusBar()->showMessage(tr("Opened successfully: %1").arg(fileName));

		onCurrentFileChanged();

		return true;
	}

	// failure
	statusBar()->showMessage(tr("Failed to open: %1").arg(fileName));

	QMessageBox::critical(NULL,
		fileName,
		tr("Document cannot be opened. Check access rights and path."));

	return false;
}


void CMainWindow::onOpenDocumentDialog(QString &title, QString &filter)
{
    filter = "";

    QString allFilters;

    for (const CDocument& doc : m_docTypes)
    {
        for (const auto& format : doc.formats)
        {
            if (format.canRead)
            {
                filter += format.name + " (" + format.filters + ") ;;";
                allFilters += format.filters + " ";
            }
        }
    }

    if (allFilters.size())
    {
        allFilters.chop(1);
        allFilters = tr("Any supported format (%1)").arg(allFilters);
        filter += allFilters;

        if (m_lastOpenFilter.isEmpty())
            m_lastOpenFilter = allFilters;
    }
}


void CMainWindow::on_actionSave_triggered()
{
	save();
}


void CMainWindow::on_actionSaveAs_triggered()
{
	saveAs();
}


bool CMainWindow::save()
{
	if (m_currentFileName.isEmpty())
		return saveAs();

	return doSaveDocument(m_currentFileName, "", m_currentDocType);
}


static QString cutLastSuffix(const QString& fileName)
{
	int idx = fileName.lastIndexOf(".");
	if (idx < 0)
		return fileName;
	else
		return fileName.left(idx);
}


bool CMainWindow::saveAs()
{
    if (m_currentDocType.isEmpty())
        return true;

    QString filter;
	QMap<QString, QString> filterToSuffix;
	const CDocument& doc = m_docTypes[m_currentDocType];
    for (const CDocumentFormat& format : doc.formats)
    {
        if (format.canSave)
        {
			QString formatFilter = format.name + " (" + format.filters + ")";
            filter += formatFilter + ";;";

			filterToSuffix[formatFilter] = format.suffixes.first();
        }
    }

    if (filter.size())
        filter.chop(2);

    QString title = tr("Save File");
    onSaveDocumentDialog(title, filter);

	QString saveName = cutLastSuffix(m_currentFileName);
	if (saveName.isEmpty())
		saveName = m_lastPath;
    if (saveName.isEmpty())
        saveName = QDir::homePath();

    QString selectedFilter = m_lastSaveFilter;

    QString fileName = QFileDialog::getSaveFileName(NULL, title, saveName, filter, &selectedFilter);
    if (fileName.isEmpty())
        return false;

	// workaround: auto append suffix (QTBUG!)
	QString selectedSuffix = QFileInfo(fileName).suffix().toLower();
	if (selectedSuffix.isEmpty() && filterToSuffix.contains(selectedFilter))
		fileName += "." + filterToSuffix[selectedFilter];

    QString normalizedName = QDir::toNativeSeparators(fileName);

    return doSaveDocument(normalizedName, selectedFilter, m_currentDocType);
}


bool CMainWindow::doSaveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType)
{
    if (saveDocument(fileName, selectedFilter, docType))
    {
        m_currentFileName = fileName;
        m_isChanged = false;
        m_lastSaveFilter = selectedFilter;
		m_lastPath = QFileInfo(m_currentFileName).absolutePath();

        statusBar()->showMessage(tr("Document saved successfully."));

		onCurrentFileChanged();

		return true;
    }
    else
    {
        QMessageBox::critical(NULL, tr("Save Error"), tr("Document cannot be saved. Check access rights and path."));
    }

	return false;
}


bool CMainWindow::saveOnExit()
{
	if (!m_isChanged)
		return true;

	const QMessageBox::StandardButton ret = QMessageBox::warning(
		this, 
		m_currentFileName, 
		tr("The document has been modified.\n"
				"Do you want to save your changes?"),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

	switch (ret) 
	{
	case QMessageBox::Save:
		return save();
	case QMessageBox::Cancel:
		return false;
	default:
		break;
	}

	return true;
}


// recent files management

QStringList CMainWindow::getRecentFilesList() const
{
	QSettings &settings = getApplicationSettings();

	return settings.value("recentFiles").toStringList();
}


void CMainWindow::updateRecentFiles()
{
	if (m_currentFileName.isEmpty())
		return;

	QSettings &settings = getApplicationSettings();

	QStringList recentFiles = settings.value("recentFiles").toStringList();
	int index = recentFiles.indexOf(m_currentFileName);
	if (index == 0)
		return;

	if (index > 0) {
		recentFiles.move(index, 0);
	}
	else {
		recentFiles.prepend(m_currentFileName);
		
		// to parameterize
		if (recentFiles.size() > 20)
			recentFiles.removeLast();
	}

	settings.setValue("recentFiles", recentFiles);
}


void CMainWindow::fillRecentFilesMenu()
{
	m_recentFilesMenu->clear();
	 
	QSettings &settings = getApplicationSettings();

	QStringList recentFiles = settings.value("recentFiles").toStringList();

	for (int i = 0; i < recentFiles.size(); ++i)
	{
		QAction *recentAction = m_recentFilesMenu->addAction(recentFiles.at(i));
		recentAction->setData(i);
	}
}


void CMainWindow::onRecentFilesMenuAction(QAction *recentAction)
{
	QString filePath = recentAction->text();

	if (doOpenDocument(filePath))
		return;

	// failed - remove
	QSettings &settings = getApplicationSettings();

	QStringList recentFiles = settings.value("recentFiles").toStringList();
	recentFiles.removeAt(recentAction->data().toInt());

	settings.setValue("recentFiles", recentFiles);
}


// instance management

QVariantMap CMainWindow::getActiveInstances()
{
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

	QVariantMap pidFileMap = settings.value("instances").value<QVariantMap>();

	CPlatformServices::PIDs livingPids = CPlatformServices::GetRunningPIDs();
	bool mapUpdated = false;

	// check if alive
	QList<QString> spids = pidFileMap.keys();
	for (auto spid : spids)
	{
		if (!livingPids.contains(spid.toUInt()))
		{
			pidFileMap.remove(spid);
			mapUpdated = true;
		}
	}

	// write cleaned map back
	if (mapUpdated)
	{
		settings.setValue("instances", pidFileMap);
	}

	return pidFileMap;
}


void CMainWindow::updateInstance()
{
	QSettings &settings = getApplicationSettings();

	QVariantMap pidFileMap = settings.value("instances").value<QVariantMap>();
	QVariantMap dataMap = pidFileMap[m_stringPID].value<QVariantMap>();
	dataMap["title"] = m_mainTitleText;
	dataMap["file"] = m_currentFileName;
	dataMap["hwnd"] = (uint)effectiveWinId();
	dataMap["spid"] = m_stringPID;
	pidFileMap[m_stringPID] = dataMap; 
	settings.setValue("instances", pidFileMap);
}


void CMainWindow::removeInstance()
{
	QSettings &settings = getApplicationSettings();

	QVariantMap pidFileMap = settings.value("instances").value<QVariantMap>();
	pidFileMap.remove(m_stringPID);
	settings.setValue("instances", pidFileMap);
}


bool CMainWindow::activateInstance(const QString &fileName)
{
	QString normalizedName = fileName; // QDir::toNativeSeparators(QFileInfo(fileName).canonicalFilePath());
#ifdef WIN32
	normalizedName = normalizedName.toLower();
#endif

	// current instance?
	if (normalizedName == m_currentFileName)
	{
		raise();
		activateWindow();
		return true;
	}

	// else check running instances
	QVariantMap pidFileMap = getActiveInstances();

	for (auto it = pidFileMap.constBegin(); it != pidFileMap.constEnd(); ++it)
	{
		QVariantMap dataMap = it.value().value<QVariantMap>();

		QString fileName = dataMap["file"].toString();
#ifdef WIN32
		fileName = fileName.toLower();
#endif

		if (normalizedName == fileName)
		{
			// found: switch to instance
			CPlatformServices::SetActiveWindow(dataMap["hwnd"].toUInt());
			return true;
		}
	}

	return false;
}


void CMainWindow::createWindowsMenu()
{
	m_windowsMenu = new QMenu(tr("&Window"));
	m_windowsMenuAction = menuBar()->addMenu(m_windowsMenu);
	
	connect(m_windowsMenu, SIGNAL(aboutToShow()), this, SLOT(fillWindowsMenu()));
	connect(m_windowsMenu, SIGNAL(triggered(QAction*)), this, SLOT(onWindowsMenuAction(QAction*)));
}


void CMainWindow::fillWindowsMenu()
{
	QVariantMap pidFileMap = getActiveInstances();

	m_windowsMenu->clear();

	char hotKey = '1';

	for (auto it = pidFileMap.constBegin(); it != pidFileMap.constEnd(); ++it)
	{
		QVariantMap dataMap = it.value().value<QVariantMap>();
		QString fileTitle = dataMap["title"].toString();

		QString text = QString("&%1 %2").arg(hotKey++).arg(fileTitle);
		QAction *windowAction = m_windowsMenu->addAction(text);
		windowAction->setCheckable(true);
		windowAction->setChecked(m_stringPID == it.key());
		windowAction->setData(dataMap);
	}
}


void CMainWindow::onWindowsMenuAction(QAction *windowAction)
{
	QVariantMap dataMap = windowAction->data().value<QVariantMap>();

	QString spid = dataMap["spid"].toString();
	if (spid.isEmpty() || spid == m_stringPID)
		return;

	CPlatformServices::SetActiveWindow(dataMap["hwnd"].toUInt());
}


// help

void CMainWindow::createHelpMenu()
{
	m_helpMenu = new QMenu(tr("&Help"));
	menuBar()->addMenu(m_helpMenu);

	m_helpMenu->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));
	m_helpMenu->addAction(tr("&About..."), this, SLOT(onAboutApplication()));
}


void CMainWindow::onAboutApplication()
{
	QMessageBox::about(this,
		tr("About %1").arg(qApp->applicationDisplayName()),
		getAboutText());
}


QString CMainWindow::getAboutText() const
{
	return QString("<b>%1</b><br>Version %2")
		.arg(qApp->applicationName())
		.arg(qApp->applicationVersion());
}


// settings

QSettings& CMainWindow::getApplicationSettings() const
{
	static QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
	return settings;
}


void CMainWindow::readSettings()
{
	QSettings& settings = getApplicationSettings();

	settings.sync();

	doReadSettings(settings);
}


void CMainWindow::doReadSettings(QSettings& settings)
{
	showNormal();


    // window geometry
	const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
	if (geometry.isEmpty()) 
	{
		const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
		resize(availableGeometry.width()-200, availableGeometry.height()-100);
		move(100, 50);
	}
	else {
		restoreGeometry(geometry);
	}


    // toolbars & dock widgets
    QApplication::processEvents();
    const QByteArray state = settings.value("windowState").toByteArray();
    if (!state.isEmpty())
        restoreState(state);


    // window state
    if (settings.value("maximized", true).toBool())
    {
//#ifdef Q_OS_WIN32
//		showMaximized();
//#else
		showNormal();
        QTimer::singleShot(0, this, SLOT(showMaximized()));
//#endif
    }
	else
		showNormal();


	// path
	m_lastPath = settings.value("lastPath").toString();
}


void CMainWindow::writeSettings()
{
	QSettings& settings = getApplicationSettings();

	doWriteSettings(settings);

	settings.sync();
}


void CMainWindow::doWriteSettings(QSettings& settings)
{
	settings.setValue("windowState", saveState());
	settings.setValue("geometry", saveGeometry());
	settings.setValue("maximized", isMaximized());

	settings.setValue("lastPath", m_lastPath);
}
