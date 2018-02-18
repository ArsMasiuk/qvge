#ifndef CCOLORSCHEMESUICONTROLLER_H
#define CCOLORSCHEMESUICONTROLLER_H

#include <QObject>
#include <QMenu>

class CEditorScene;


class CColorSchemesUIController : public QObject
{
    Q_OBJECT
public:
    explicit CColorSchemesUIController(QObject *parent = nullptr);

    void setScene(CEditorScene* scene) {
        m_scene = scene;
    }

    QMenu* getSchemesMenu() {
        return &m_menu;
    }

Q_SIGNALS:

private Q_SLOTS:
    void applyBW();
    void applyInverse();
	void applySolarizedLight();
	void applyBlueOrange();
	void applyForest();

private:
    QMenu m_menu;
     CEditorScene *m_scene = NULL;
};

#endif // CCOLORSCHEMESUICONTROLLER_H
