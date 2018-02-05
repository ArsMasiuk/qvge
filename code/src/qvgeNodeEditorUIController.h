/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QAction>
#include <QLabel>
#include <QSettings>

#include <slider2d.h>

class qvgeMainWindow;

class CNodeEditorScene;
class CEditorView;


class qvgeNodeEditorUIController : public QObject 
{
	Q_OBJECT

public:
	qvgeNodeEditorUIController(qvgeMainWindow *parent);
	~qvgeNodeEditorUIController();

	void doReadSettings(QSettings& settings);
	void doWriteSettings(QSettings& settings);

	bool loadFromFile(const QString &fileName, const QString &format);
	bool saveToFile(const QString &fileName, const QString &format);

    void onNewDocumentCreated();

private Q_SLOTS:
	void exportFile();
	void exportPDF();
	void exportDOT();

	void onSelectionChanged();
    void onSceneChanged();

	void onZoomChanged(double currentZoom);
	void zoom();
	void unzoom();
	void resetZoom();

	void sceneCrop();
    void sceneOptions();

    void onNavigatorShown();

private:
	void createMenus();
	void createPanels();
    void createNavigator();

private:
	qvgeMainWindow *m_parent;
	CNodeEditorScene *m_editorScene;
	CEditorView *m_editorView;

    class COGDFLayoutUIController *m_ogdfController;

    class QSint::Slider2d *m_sliderView;

    QLabel *m_statusLabel;

	QAction *cutAction;
	QAction *copyAction;
	QAction *pasteAction;
	QAction *delAction;
	QAction *unlinkAction;

	QAction *zoomAction;
	QAction *unzoomAction;
	QAction *resetZoomAction, *resetZoomAction2;
	QAction *fitZoomAction;

    QAction *gridAction;
    QAction *gridSnapAction;
    QAction *actionShowLabels;
};
