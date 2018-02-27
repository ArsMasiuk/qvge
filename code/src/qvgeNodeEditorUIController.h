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
class IFileSerializer;


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
	bool doExport(const IFileSerializer &exporter);
	void exportFile();
	void exportPDF();
	void exportDOT();

	void onSelectionChanged();
    void onSceneChanged();
	void onSceneHint(const QString& text);
	void onSceneStatusChanged(int status);
	void sceneEditMode(QAction*);
	void onEditModeChanged(int mode);

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

    class QSint::Slider2d *m_sliderView;

    QLabel *m_statusLabel;

	QString m_lastExportPath;

	QAction *cutAction;
	QAction *copyAction;
	QAction *pasteAction;
	QAction *delAction;
	QAction *linkAction;
	QAction *unlinkAction;

	QActionGroup *m_editModesGroup;
	QAction *modeDefaultAction;
	QAction *modeNodesAction;
	QAction *modeEdgesAction;

	QAction *zoomAction;
	QAction *unzoomAction;
	QAction *resetZoomAction;
	QAction *resetZoomAction2;
	QAction *fitZoomAction;

    QAction *gridAction;
    QAction *gridSnapAction;
    QAction *actionShowLabels;

	class COGDFLayoutUIController *m_ogdfController;
	bool m_showNewGraphDialog = true;
};
