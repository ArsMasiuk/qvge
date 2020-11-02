#pragma once

#include <QObject>

class CMainWindow;
class CNodeEditorScene;


class CGVGraphLayoutUIController : public QObject
{
    Q_OBJECT

public:
    explicit CGVGraphLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene);

	void setPathToGraphviz(const QString &pathToGraphviz);
	void setDefaultEngine(const QString &engine);

	// file IO (think: to move?)
	bool loadGraph(const QString &filename, CNodeEditorScene &scene, QString* lastError = nullptr);

Q_SIGNALS:
	void loadFinished();
	void layoutFinished();

private Q_SLOTS:
	void doDotLayout()		{ doLayout("dot", *m_scene); }
	void doNeatoLayout()	{ doLayout("neato", *m_scene); }
	void doFDPLayout()		{ doLayout("fdp", *m_scene); }
	void doSFDPLayout()		{ doLayout("sfdp", *m_scene); }
	void doTwopiLayout()	{ doLayout("twopi", *m_scene); }
	void doCircularLayout() { doLayout("circo", *m_scene); }

private:
	bool doLayout(const QString &engine, CNodeEditorScene &scene);

    CMainWindow *m_parent = nullptr;
    CNodeEditorScene *m_scene = nullptr;

	QString m_pathToGraphviz;
	QString m_defaultEngine;
};
