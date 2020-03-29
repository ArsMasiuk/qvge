/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CEditorView.h"
#include "CEditorScene.h"

#include <QMouseEvent> 
#include <QScrollBar> 
#include <QGuiApplication>
#include <QDebug> 


CEditorView::CEditorView(QWidget *parent)
	: Super(parent),
	m_menuModeTmp(Qt::PreventContextMenu)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
	setViewportUpdateMode(BoundingRectViewportUpdate);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	setDragMode(RubberBandDrag);

    setRenderHint(QPainter::Antialiasing);
	setOptimizationFlags(DontSavePainterState);
    setOptimizationFlags(DontAdjustForAntialiasing);

	setFocus();

	connect(&m_scrollTimer, SIGNAL(timeout()), this, SLOT(onScrollTimeout()));
	m_scrollTimer.setInterval(100);
}


CEditorView::CEditorView(CEditorScene *scene, QWidget *parent): CEditorView(parent)
{
	setScene(scene);
}


CEditorView::~CEditorView()
{
}


// zoom

void CEditorView::zoomTo(double target)
{
	QTransform mat;
	mat.scale(target, target);
	setTransform(mat);
	
	m_currentZoom = target;

	Q_EMIT scaleChanged(m_currentZoom);
}


void CEditorView::zoomBy(double factor)
{
	double target = m_currentZoom * factor;

	zoomTo(target);
}


void CEditorView::fitToView()
{
	m_zoomBeforeFit = m_currentZoom;
	m_dxyBeforeFit = getCenter();

	fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);

	m_currentZoom = matrix().m11();

	Q_EMIT scaleChanged(m_currentZoom);
}


void CEditorView::fitSelectedToView()
{
	auto items = scene()->selectedItems();
	if (items.isEmpty())
		return;

	m_zoomBeforeFit = m_currentZoom;
	m_dxyBeforeFit = getCenter();

	QRectF r;
	for (const auto item : items)
	{
		r |= item->sceneBoundingRect();
	}

	fitInView(r, Qt::KeepAspectRatio);

	m_currentZoom = matrix().m11();

	Q_EMIT scaleChanged(m_currentZoom);
}


void CEditorView::zoomBack()
{
	zoomTo(m_zoomBeforeFit);
	centerOn(m_dxyBeforeFit);
}


QPointF CEditorView::getCenter() const
{
	return mapToScene(viewport()->rect().center());
}


void CEditorView::restoreContextMenu()
{
	setContextMenuPolicy(m_menuModeTmp);
}


void CEditorView::onScrollTimeout()
{
	if (QGuiApplication::mouseButtons() & Qt::LeftButton)
	{
		auto globTopLeft = viewport()->mapToGlobal(QPoint(viewport()->x(), viewport()->y()));
		QRect globRect = QRect(globTopLeft, viewport()->size());

		if (QCursor::pos().x() > globRect.right())
		{
			int dx = QCursor::pos().x() - globRect.right();
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
		}
		else
			if (QCursor::pos().x() < globRect.left())
			{
				int dx = globRect.left() - QCursor::pos().x();
				horizontalScrollBar()->setValue(horizontalScrollBar()->value() - dx);
			}

		if (QCursor::pos().y() > globRect.bottom())
		{
			int dy = QCursor::pos().y() - globRect.bottom();
			verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
		}
		else
			if (QCursor::pos().y() < globRect.top())
			{
				int dy = globRect.top() - QCursor::pos().y();
				verticalScrollBar()->setValue(verticalScrollBar()->value() - dy);
			}
	}
}


// reimp

#if defined Q_OS_WIN && !defined Q_OS_CYGWIN		// Windows-conform panning & context menu


void CEditorView::mousePressEvent(QMouseEvent *e)
{
	Super::mousePressEvent(e);

	if (e->buttons() == Qt::LeftButton)
	{
		m_scrollTimer.start();
	}
}


void CEditorView::mouseMoveEvent(QMouseEvent *e)
{
	// enable RMB pan
	if (e->buttons() == Qt::RightButton)
	{
		if (dragMode() != ScrollHandDrag)
		{
			m_menuModeTmp = contextMenuPolicy();
			setContextMenuPolicy(Qt::PreventContextMenu);

			m_dragModeTmp = dragMode();
			setDragMode(ScrollHandDrag);

			m_interactiveTmp = isInteractive();
			setInteractive(false);

			QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
			Super::mousePressEvent(&fake);
		}
	}

	Super::mouseMoveEvent(e);

	// else check LMB selection
	if (e->buttons() == Qt::LeftButton)
	{
		//auto globTopLeft = viewport()->mapToGlobal(QPoint(viewport()->x(), viewport()->y()));
		//QRect globRect = QRect(globTopLeft, viewport()->size());
		//if (QCursor::pos().x() > globRect.right())
		//{
		//	scrollContentsBy(-5, 0);
		//	invalidateScene();
		//	//viewport()->repaint();
		//}
	}
}


void CEditorView::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
	{
		m_scrollTimer.stop();
	}

	// disable RMB pan
	if (e->button() == Qt::RightButton && !e->buttons() && (dragMode() == ScrollHandDrag))
	{
		QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
		Super::mouseReleaseEvent(&fake);

		setDragMode(m_dragModeTmp);

		setInteractive(m_interactiveTmp);

		QTimer::singleShot(100, this, SLOT(restoreContextMenu()));
	}
	//else
	{
		Super::mouseReleaseEvent(e);
	}
}

#else	// Linux/Unix/etc.

void CEditorView::mousePressEvent(QMouseEvent *e)
{
	m_moved = false;

	// enable RMB pan
	if (e->button() == Qt::RightButton)
	{
		if (dragMode() != ScrollHandDrag)
		{
			m_menuModeTmp = contextMenuPolicy();
			setContextMenuPolicy(Qt::PreventContextMenu);

			setDragMode(ScrollHandDrag);

			m_interactiveTmp = isInteractive();
			setInteractive(false);

			QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
			Super::mousePressEvent(&fake);

			return;
		}
	}

	Super::mousePressEvent(e);
}


void CEditorView::mouseMoveEvent(QMouseEvent *e)
{
	m_moved = true;

	Super::mouseMoveEvent(e);

	// check LMB selection
	//if (e->buttons() == Qt::LeftButton)
	//{
	//	onLeftClickMouseMove(e);
	//}
}


void CEditorView::mouseReleaseEvent(QMouseEvent *e)
{
	// disabel RMB pan
	if (e->button() == Qt::RightButton && dragMode() == ScrollHandDrag)
	{
		QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
		Super::mouseReleaseEvent(&fake);

		setDragMode(RubberBandDrag);

		setInteractive(m_interactiveTmp);

		setContextMenuPolicy(m_menuModeTmp);

		if (!m_moved)
		{
			QMouseEvent fake(QEvent::MouseButtonPress, e->pos(), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
			Super::mousePressEvent(&fake);

			Super::mouseReleaseEvent(e);

			QContextMenuEvent fake2(QContextMenuEvent::Mouse, e->pos());
			contextMenuEvent(&fake2);
		}

		return;
	}

	Super::mouseReleaseEvent(e);
}

#endif


void CEditorView::wheelEvent(QWheelEvent *e)
{
	// original taken from
	// http://blog.automaton2000.com/2014/04/mouse-centered-zooming-in-qgraphicsview.html

	if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier
		&& e->angleDelta().x() == 0) 
	{
		QPoint  pos = e->pos();
		QPointF posf = this->mapToScene(pos);

		double by = 1.0;
		double angle = e->angleDelta().y();

		if (angle > 0) { by = 1 + (angle / 360 * 0.5); }
		else 
			if (angle < 0) { by = 1 - (-angle / 360 * 0.5); }

		//this->scale(by, by);
		zoomBy(by);

		double w = this->viewport()->width();
		double h = this->viewport()->height();

		double wf = this->mapToScene(QPoint(w - 1, 0)).x() - this->mapToScene(QPoint(0, 0)).x();
		double hf = this->mapToScene(QPoint(0, h - 1)).y() - this->mapToScene(QPoint(0, 0)).y();

		double lf = posf.x() - pos.x() * wf / w;
		double tf = posf.y() - pos.y() * hf / h;

		/* try to set viewport properly */
		this->ensureVisible(lf, tf, wf, hf, 0, 0);

		QPointF newPos = this->mapToScene(pos);

		/* readjust according to the still remaining offset/drift
		* I don't know how to do this any other way */
		this->ensureVisible(QRectF(QPointF(lf, tf) - newPos + posf, QSizeF(wf, hf)), 0, 0);

		e->accept();
	}

	if ((e->modifiers() & Qt::ControlModifier) != Qt::ControlModifier) {
		/* no scrolling while control is held */
		Super::wheelEvent(e);
	}
}

