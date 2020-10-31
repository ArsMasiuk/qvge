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

	// file IO (think: to move?)
	bool loadGraph(const QString &filename, CNodeEditorScene &scene, QString* lastError = nullptr);

private Q_SLOTS:
    void doDotLayout();

private:
	bool doLayout(const QString &engine, CNodeEditorScene &scene);

    CMainWindow *m_parent = nullptr;
    CNodeEditorScene *m_scene = nullptr;

	QString m_pathToGraphviz;
};
