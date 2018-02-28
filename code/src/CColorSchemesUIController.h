#ifndef CCOLORSCHEMESUICONTROLLER_H
#define CCOLORSCHEMESUICONTROLLER_H

#include <QObject>
#include <QMenu>
#include <QList>

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
	void colorSchemeApplied(CEditorScene* scene);

private Q_SLOTS:
	void onMenuTriggered(QAction *action);

private:
    QMenu m_menu;
    CEditorScene *m_scene = NULL;

	struct Scheme
	{
		QString name;
		QColor bgColor, gridColor;
		QColor nodeColor, nodeStrokeColor, nodeLabelColor;
		QColor edgeColor, edgeLabelColor;
	};

	QList<Scheme> m_schemes;

	void addScheme(const Scheme& scheme);
	void applyScheme(const Scheme& scheme);
};

#endif // CCOLORSCHEMESUICONTROLLER_H
