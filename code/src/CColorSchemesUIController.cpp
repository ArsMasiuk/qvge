#include "CColorSchemesUIController.h"

#include <qvge/CEditorScene.h>


CColorSchemesUIController::CColorSchemesUIController(QObject *parent) : QObject(parent)
{
    m_menu.addAction(tr("Grayscale"), this, SLOT(applyBW()));
    m_menu.addAction(tr("Inverse Grayscale"), this, SLOT(applyInverse()));
    m_menu.addAction(tr("Solarized Light"), this, SLOT(applySolarizedLight()));
	m_menu.addAction(tr("Blue & Orange"), this, SLOT(applyBlueOrange()));
	m_menu.addAction(tr("Forest"), this, SLOT(applyForest()));
}


void CColorSchemesUIController::applyBW()
{
    if (m_scene)
    {
        m_scene->setBackgroundBrush(Qt::white);
        m_scene->setGridPen(QColor(Qt::gray));
        m_scene->setClassAttribute("node", "color", QColor(Qt::lightGray));
        m_scene->setClassAttribute("node", "stroke.color", QColor(Qt::black));
        m_scene->setClassAttribute("node", "label.color", QColor(Qt::black));
        m_scene->setClassAttribute("edge", "color", QColor(Qt::darkGray));
        m_scene->setClassAttribute("edge", "label.color", QColor(Qt::gray));

        m_scene->addUndoState();
    }
}


void CColorSchemesUIController::applyInverse()
{
    if (m_scene)
    {
        m_scene->setBackgroundBrush(Qt::black);
        m_scene->setGridPen(QColor(Qt::darkGray));
        m_scene->setClassAttribute("node", "color", QColor(Qt::darkGray));
        m_scene->setClassAttribute("node", "stroke.color", QColor(Qt::white));
        m_scene->setClassAttribute("node", "label.color", QColor(Qt::white));
        m_scene->setClassAttribute("edge", "color", QColor(Qt::gray));
        m_scene->setClassAttribute("edge", "label.color", QColor(Qt::lightGray));

        m_scene->addUndoState();
    }
}


void CColorSchemesUIController::applySolarizedLight()
{
	if (m_scene)
	{
		m_scene->setBackgroundBrush(QColor("#fdf6e3"));
		m_scene->setGridPen(QColor("#eee8d5"));

		m_scene->setClassAttribute("node", "color", QColor("#e0dbcb"));
		m_scene->setClassAttribute("node", "stroke.color", QColor("#073642"));
		m_scene->setClassAttribute("node", "label.color", QColor("#657b83"));

		m_scene->setClassAttribute("edge", "color", QColor("#556058"));
		m_scene->setClassAttribute("edge", "label.color", QColor("#808000"));

		m_scene->addUndoState();
	}
}


void CColorSchemesUIController::applyBlueOrange()
{
	if (m_scene)
	{
		m_scene->setBackgroundBrush(QColor("#ffffff"));
		m_scene->setGridPen(QColor("#eeeeee"));

		m_scene->setClassAttribute("node", "color", QColor("#55aaff"));
		m_scene->setClassAttribute("node", "stroke.color", QColor("#ffffff"));
		m_scene->setClassAttribute("node", "label.color", QColor("#444444"));

		m_scene->setClassAttribute("edge", "color", QColor("#ffaa00"));
		m_scene->setClassAttribute("edge", "label.color", QColor("#55aa7f"));

		m_scene->addUndoState();
	}
}


void CColorSchemesUIController::applyForest()
{
	if (m_scene)
	{
		m_scene->setBackgroundBrush(QColor("#e3e6bb"));
		m_scene->setGridPen(QColor("#eeeeee"));

		m_scene->setClassAttribute("node", "color", QColor("#aaff7f"));
		m_scene->setClassAttribute("node", "stroke.color", QColor("#8d4600"));
		m_scene->setClassAttribute("node", "label.color", QColor("#343400"));

		m_scene->setClassAttribute("edge", "color", QColor("#aaaa7f"));
		m_scene->setClassAttribute("edge", "label.color", QColor("#55aa00"));

		m_scene->addUndoState();
	}
}

