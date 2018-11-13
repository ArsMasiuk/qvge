#include "CNodesFactorDialog.h"
#include "ui_CNodesFactorDialog.h"

#include <qvge/CNode.h>
#include <qvge/CNodeEditorScene.h>


CNodesFactorDialog::CNodesFactorDialog(): ui(new Ui::CNodesFactorDialog)
{
	ui->setupUi(this);
}


CNodesFactorDialog::~CNodesFactorDialog()
{
}


int CNodesFactorDialog::exec(CNodeEditorScene& scene)
{
	m_scene = &scene;

	auto nodes = scene.getSelectedNodes();
	if (nodes.isEmpty())
		return QDialog::Rejected;

	ui->FactorX->setValue(100);
	ui->FactorY->setValue(100);

	m_sourceMap.clear();
	QRectF boundingRect;

	for (auto node : nodes)
	{
		m_sourceMap[node] = node->pos();
		boundingRect = boundingRect.united(QRectF(node->pos(), QSizeF(1,1)));
	}

	m_sourceCenter = boundingRect.center();

	return QDialog::exec();
}


void CNodesFactorDialog::on_FactorX_valueChanged(int v)
{
	double dv = (double)v / 100.0;

	for (auto it = m_sourceMap.begin(); it != m_sourceMap.end(); ++it)
	{
		double dx = it.value().x() - m_sourceCenter.x();
		dx *= dv;
		double newx = dx + m_sourceCenter.x();
		 
		it.key()->setPos(newx, it.key()->pos().y());
	}
}


void CNodesFactorDialog::on_FactorY_valueChanged(int v)
{
	double dv = (double)v / 100.0;

	for (auto it = m_sourceMap.begin(); it != m_sourceMap.end(); ++it)
	{
		double dy = it.value().y() - m_sourceCenter.y();
		dy *= dv;
		double newy = dy + m_sourceCenter.y();

		it.key()->setPos(it.key()->pos().x(), newy);
	}
}

