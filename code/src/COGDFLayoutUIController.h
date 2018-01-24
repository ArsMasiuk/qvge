#ifndef COGDFLAYOUTUICONTROLLER_H
#define COGDFLAYOUTUICONTROLLER_H

#include <QObject>

class qvgeMainWindow;
class CNodeEditorScene;


class COGDFLayoutUIController : public QObject
{
    Q_OBJECT
public:
    explicit COGDFLayoutUIController(qvgeMainWindow *parent, CNodeEditorScene *scene);

private Q_SLOTS:
    void doPlanarLayout();
    void doLinearLayout();
    void doBalloonLayout();
    void doCircularLayout();
    void doFMMMLayout();
	void doPSLLayout();

private:
    qvgeMainWindow *m_parent;
    CNodeEditorScene *m_scene;
};

#endif // COGDFLAYOUTUICONTROLLER_H
