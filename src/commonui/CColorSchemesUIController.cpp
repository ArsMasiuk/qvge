#include "CColorSchemesUIController.h"

#include <qvge/CEditorScene.h>


CColorSchemesUIController::CColorSchemesUIController(QObject *parent) : QObject(parent)
{
	{	
		Scheme scheme{ tr("Grayscale") };
		scheme.bgColor = Qt::white;
		scheme.gridColor = Qt::gray;
		scheme.nodeColor = Qt::lightGray;
		scheme.nodeStrokeColor = Qt::black;
		scheme.nodeLabelColor = Qt::black;
		scheme.edgeColor = Qt::darkGray;
		scheme.edgeLabelColor = Qt::gray;

		addScheme(scheme);
	}

	{	
		Scheme scheme{ tr("Inverse Grayscale") };
		scheme.bgColor = Qt::black;
		scheme.gridColor = Qt::darkGray;
		scheme.nodeColor = Qt::darkGray;
		scheme.nodeStrokeColor = Qt::white;
		scheme.nodeLabelColor = Qt::white;
		scheme.edgeColor = Qt::gray;
		scheme.edgeLabelColor = Qt::lightGray;

		addScheme(scheme);
	}

	{
		Scheme scheme{ tr("Solarized Light") };
		scheme.bgColor = QColor("#fdf6e3");
		scheme.gridColor = QColor("#eee8d5");
		scheme.nodeColor = QColor("#e0dbcb");
		scheme.nodeStrokeColor = QColor("#073642");
		scheme.nodeLabelColor = QColor("#657b83");
		scheme.edgeColor = QColor("#556058");
		scheme.edgeLabelColor = QColor("#808000");

		addScheme(scheme);
	}

	{
		Scheme scheme{ tr("Blue && Orange") };
		scheme.bgColor = QColor("#ffffff");
		scheme.gridColor = QColor("#eeeeee");
		scheme.nodeColor = QColor("#55aaff");
		scheme.nodeStrokeColor = QColor("#ffffff");
		scheme.nodeLabelColor = QColor("#444444");
		scheme.edgeColor = QColor("#ffaa00");
		scheme.edgeLabelColor = QColor("#55aa7f");

		addScheme(scheme);
	}

	{
		Scheme scheme{ tr("Forest") };
		scheme.bgColor = QColor("#e3e6bb");
		scheme.gridColor = QColor("#eeeeee");
		scheme.nodeColor = QColor("#aaff7f");
		scheme.nodeStrokeColor = QColor("#8d4600");
		scheme.nodeLabelColor = QColor("#343400");
		scheme.edgeColor = QColor("#aaaa7f");
		scheme.edgeLabelColor = QColor("#55aa00");

		addScheme(scheme);
	}

	{
		Scheme scheme{ tr("Sunny Spring") };
		scheme.bgColor = QColor("#f3ffe1");
		scheme.gridColor = QColor("#eeeeee");
		scheme.nodeColor = QColor("#b4ba00");
		scheme.nodeStrokeColor = QColor("#b4ba00");
		scheme.nodeLabelColor = QColor("#111111");
		scheme.edgeColor = QColor("#ba4400");
		scheme.edgeLabelColor = QColor("#267536");

		addScheme(scheme);
	}

    {
        Scheme scheme{ tr("Night Sky") };
        scheme.bgColor = QColor("#000640");
        scheme.gridColor = QColor("#070f5a");
        scheme.nodeColor = QColor("#000000");
        scheme.nodeStrokeColor = QColor("#6f73c0");
        scheme.nodeLabelColor = QColor("#dcdcdc");
        scheme.edgeColor = QColor("#6f73c0");
        scheme.edgeLabelColor = QColor("#aad6ff");

        addScheme(scheme);
    }

	connect(&m_menu, SIGNAL(triggered(QAction*)), this, SLOT(onMenuTriggered(QAction*)));
}


void CColorSchemesUIController::onMenuTriggered(QAction *action)
{
	int index = action->data().toInt();
	applyScheme(m_schemes.at(index));
}


void CColorSchemesUIController::addScheme(const Scheme& scheme)
{
	int index = m_schemes.size();
	m_schemes << scheme;
	auto action = m_menu.addAction(scheme.name);
	action->setData(index);
}


void CColorSchemesUIController::applyScheme(const Scheme& scheme)
{
	if (m_scene)
	{
		m_scene->setBackgroundBrush(scheme.bgColor);
		m_scene->setGridPen(scheme.gridColor);
		m_scene->setClassAttribute("node", "color", scheme.nodeColor);
		m_scene->setClassAttribute("node", "stroke.color", scheme.nodeStrokeColor);
		m_scene->setClassAttribute("node", "label.color", scheme.nodeLabelColor);
		m_scene->setClassAttribute("edge", "color", scheme.edgeColor);
		m_scene->setClassAttribute("edge", "label.color", scheme.edgeLabelColor);

		m_scene->addUndoState();

		Q_EMIT colorSchemeApplied(m_scene);
	}
}
