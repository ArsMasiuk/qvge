#ifndef COGDFLAYOUTUICONTROLLER_H
#define COGDFLAYOUTUICONTROLLER_H

#include <QObject>

class CMainWindow;
class CNodeEditorScene;


class COGDFLayoutUIController : public QObject
{
    Q_OBJECT
public:
    explicit COGDFLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene);

private Q_SLOTS:
    void doPlanarLayout();
    void doLinearLayout();
    void doBalloonLayout();
    void doCircularLayout();
    void doFMMMLayout();
	void doPSLLayout();
	void doSugiyamaLayout();

private:
    CMainWindow *m_parent;
    CNodeEditorScene *m_scene;
};

#endif // COGDFLAYOUTUICONTROLLER_H
