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
    QString name;
    QString filters;
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
    explicit CMainWindow(QWidget *parent = 0);
    virtual ~CMainWindow();

    virtual void init(int argc, char *argv[]);

    void addDocument(const CDocument& doc);

	QAction* getFileExportAction() { return m_exportDocument; }
	QMenu* getFileMenu() { return m_fileMenu; }
	QAction* getWindowMenuAction() { return m_windowsMenuAction; }
	QString getCurrentFileName() const { return m_currentFileName; }

public Q_SLOTS:
	virtual void onDocumentChanged();
	virtual void onAboutApplication();

protected:
	void closeEvent(QCloseEvent *event);

	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dragLeaveEvent(QDragLeaveEvent* event);
	void dropEvent(QDropEvent* event);

    virtual void processParams(int argc, char *argv[]);

    virtual void createMainMenu();
	virtual void createWindowsMenu();
	virtual void createHelpMenu();
	
	virtual void fillNewFileMenu();
    virtual void createFileToolbar();
    virtual void updateActions();
	virtual void updateRecentFiles();
	virtual void onCurrentFileChanged();

	virtual void updateTitle();
	virtual QString getAboutText() const;

    virtual void doCreateNewDocument(const QByteArray &docType);
    virtual bool onCreateNewDocument(const QByteArray &docType);

    virtual void onOpenDocumentDialog(QString &title, QString &filter);
    virtual bool doOpenDocument(const QString &fileName);
    virtual bool onOpenDocument(const QString &fileName, QByteArray &docType) { return false; }

    virtual void onSaveDocumentDialog(QString &title, QString &filter) {}
    virtual bool doSaveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType);
    virtual bool onSaveDocument(const QString &fileName, const QString &selectedFilter, const QByteArray &docType) { return true; }

	virtual bool saveOnExit();
	virtual bool save();
	virtual bool saveAs();

    virtual bool activateInstance(const QString &fileName);
	virtual void updateInstance();
	virtual void removeInstance();
	virtual QVariantMap getActiveInstances();

	virtual void readSettings();
	virtual void doReadSettings(QSettings& settings);
	virtual void writeSettings();
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
	void onQuit();

protected:
    QMenu *m_fileMenu;
    QMenu *m_newMenu;
	QMenu *m_recentFilesMenu;
    QAction *m_newDocument;
    QAction *m_openDocument;
    QAction *m_saveDocument;
    QAction *m_saveAsDocument;
	QAction *m_exportDocument;

	QMenu *m_windowsMenu;
	QAction *m_windowsMenuAction;

    QString m_currentFileName;
    QByteArray m_currentDocType;
    bool m_isChanged;
	QString m_mainTitleText;
	
	QString m_stringPID;

    QString m_lastOpenFilter, m_lastSaveFilter;

    QMap<QByteArray, CDocument> m_docTypes;
    QList<QByteArray> m_docTypeCreate;
};

#endif // MAINWINDOW_H
