#pragma once

#include <QObject>

class CMainWindow;
class CEditorScene;


class CGVGraphLayoutUIController : public QObject
{
    Q_OBJECT

public:
    explicit CGVGraphLayoutUIController(CMainWindow *parent, CEditorScene *scene);

	void setPathToGraphviz(const QString &pathToGraphviz);
	void setDefaultEngine(const QString &engine);

	// file IO (think: to move?)
	bool loadGraph(const QString &filename, CEditorScene &scene, QString* lastError = nullptr);

Q_SIGNALS:
	void loadFinished();
	void layoutFinished();

public Q_SLOTS:
	void runGraphvizTest(const QString &graphvizPath);

private Q_SLOTS:
	void doDotLayout()		{ doLayout("dot", *m_scene); }
	void doNeatoLayout()	{ doLayout("neato", *m_scene); }
	void doFDPLayout()		{ doLayout("fdp", *m_scene); }
	void doSFDPLayout()		{ doLayout("sfdp", *m_scene); }
	void doTwopiLayout()	{ doLayout("twopi", *m_scene); }
	void doCircularLayout() { doLayout("circo", *m_scene); }

private:
	bool doLayout(const QString &engine, CEditorScene &scene);
	bool doRunDOT(const QString &engine, const QString &dotFilePath, QString &plainFilePath, QString* lastError /*= nullptr*/);
	QString errorNotWritable(const QString &path) const;
	QString errorCannotRun(const QString &path) const;
	QString errorCannotFinish(const QString &path) const;

    CMainWindow *m_parent = nullptr;
    CEditorScene *m_scene = nullptr;

	QString m_pathToGraphviz;
	QString m_defaultEngine;
};
