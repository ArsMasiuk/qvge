#pragma once

#include <QObject>

class CMainWindow;
class CNodeEditorScene;


class CGVGraphLayoutUIController : public QObject
{
    Q_OBJECT

public:
    explicit CGVGraphLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene);

private Q_SLOTS:
    void doDotLayout();

private:
	bool doLayout(const QString &engine, CNodeEditorScene &scene);

    CMainWindow *m_parent = nullptr;
    CNodeEditorScene *m_scene = nullptr;
};
