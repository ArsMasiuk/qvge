/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CEDITORVIEW_H
#define CEDITORVIEW_H

#include <QGraphicsView>
#include <QPaintEvent>

class CEditorScene;

class CEditorView : public QGraphicsView
{
	Q_OBJECT
public:
	typedef QGraphicsView Super;

	CEditorView(CEditorScene *scene, QWidget *parent = NULL);
	CEditorView(QWidget *parent = NULL);
	virtual ~CEditorView();

	// zoom
	double getZoom() const { return m_currentZoom; }
	// target is % value to zoom to (1.0 = 100% = reset zoom, 0.25 = 25% = 1/4 etc.)
	void zoomTo(double target);
	// factor if relative % value to zoom by (2 = 2x from current etc.)
	void zoomBy(double factor);

	void fitToView();

	// reimp
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void wheelEvent(QWheelEvent *e);

	void paintEvent(QPaintEvent * event)
	{
		QPaintEvent *newEvent = new QPaintEvent(event->region().boundingRect());
		QGraphicsView::paintEvent(newEvent);
		delete newEvent;
	}

Q_SIGNALS:
	void scaleChanged(double);

private:
	Qt::ContextMenuPolicy m_menuModeTmp;
	bool m_interactiveTmp = false;

	double m_currentZoom;

private Q_SLOTS:
	void restoreContextMenu();
};

#endif // CEDITORVIEW_H
