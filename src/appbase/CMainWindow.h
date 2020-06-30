#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QActionGroup>
#include <QToolBar>
#include <QSet>
#include <QByteArray>
#include <QString>
#include <QSettings>


struct CDocumentFormat
{
    QString name;		// textual description i.e "Images"
    QString filters;	// filters in form like: "*.png *.xpm *.jpg" 
	QStringList suffixes;	// supported suffixes like: png xpm jpg (first one assumed to be default)
    bool canSave, canRead;
};


struct CDocument
{
    QString name;
    QString description;
    QByteArray type;
    bool canCreate;
    QList<CDocumentFormat> formats;
};


class CMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CMainWindow(QWidget *parent = nullptr);
    virtual ~CMainWindow();

    virtual void init(const QStringList& args);

	QDockWidget* createDockWindow(const QString& name, const QString& title, Qt::DockWidgetArea area, QWidget* widget = nullptr);

	virtual QSettings& getApplicationSettings() const;
	virtual void readSettings();
	virtual void writeSettings();

    void addDocument(const CDocument& doc);
	QList<CDocument> getRegisteredDocumentTypes() const { return m_docTypes.values(); }
	QStringList getRecentFilesList() const;
	void cleanRecentFilesList();
	bool removeRecentDocument(const QString& name);

	QMenu* getFileMenu() { return m_fileMenu; }
	QMenu* getHelpMenu() { return m_helpMenu; }
	QAction* getFileExportAction() { return m_exportDocument; }
	QAction* getWindowMenuAction() { return m_windowsMenuAction; }
	QString getCurrentFileName() const { return m_currentFileName; }

public Q_SLOTS:
	virtual void onDocumentChanged();
	virtual void onAboutApplication();

	virtual void createNewDocument(const QByteArray &docType);

	void selectAndOpenDocument();
	bool openDocument(const QString &fileName);

protected:
	void closeEvent(QCloseEvent *event);

	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dragLeaveEvent(QDragLeaveEvent* event);
	void dropEvent(QDropEvent* event);

    virtual void processParams(const QStringList& args);

    virtual void createMainMenu();
	virtual void createWindowsMenu();
	virtual void createHelpMenu();

	virtual void createStartPage();
	
	virtual void fillNewFileMenu();
    virtual void createFileToolbar();
    virtual void updateActions();
	virtual void updateRecentFiles();
	virtual void onCurrentFileChanged();

	virtual void updateTitle();
	virtual QString getAboutText() const;

    virtual bool createDocument(const QByteArray &docType);
	virtual void onNewDocumentCreated(const QByteArray &docType) {}

    virtual void onOpenDocumentDialog(QString &title, QString &filter);
    virtual bool doOpenDocument(const QString &fileName);
    virtual bool openDocument(const QString &fileName, QByteArray &docType) { return false; }
	virtual bool getDocFormatFromName(const QString &normalizedName, const CDocument **doc, const CDocumentFormat **format, QString *suffix);

    virtual void onSaveDocumentDialog(QString &title, QString &filter) {}
    virtual bool doSaveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType);
    virtual bool saveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType) { return true; }

	virtual bool saveOnExit();
	virtual bool save();
	virtual bool saveAs();

    virtual bool activateInstance(const QString &fileName);
	virtual void updateInstance();
	virtual void removeInstance();
	virtual QVariantMap getActiveInstances();

	virtual void doReadSettings(QSettings& settings);
	virtual void doWriteSettings(QSettings& settings);

protected Q_SLOTS:
    void createNewDocument();
    void createNewDocument(QAction*);

    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();

	void fillRecentFilesMenu();
	void onRecentFilesMenuAction(QAction*);

	void fillWindowsMenu();
	void onWindowsMenuAction(QAction*);

private Q_SLOTS:
	void exit();
	void onQuit();

protected:
    QMenu *m_fileMenu;
    QMenu *m_newMenu;
	QMenu *m_recentFilesMenu;
	QMenu *m_helpMenu;

    QAction *m_newDocument;
    QAction *m_openDocument;
    QAction *m_saveDocument;
    QAction *m_saveAsDocument;
	QAction *m_exportDocument;
	
	QMenu *m_windowsMenu;
	QAction *m_windowsMenuAction;

    QString m_currentFileName;
	QString m_lastPath;
    QByteArray m_currentDocType;
    bool m_isChanged;
	QString m_mainTitleText;
	
	QString m_stringPID;

    QString m_lastOpenFilter, m_lastSaveFilter;

    QMap<QByteArray, CDocument> m_docTypes;
    QList<QByteArray> m_docTypeCreate;
};

#endif // MAINWINDOW_H
