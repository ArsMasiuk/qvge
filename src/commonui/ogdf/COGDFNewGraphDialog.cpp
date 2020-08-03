#include "COGDFNewGraphDialog.h"
#include "ui_COGDFNewGraphDialog.h"
#include "COGDFLayout.h"

#include <qvge/CNodeEditorScene.h>

#include <ogdf/basic/graph_generators.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/misclayout/CircularLayout.h>
#include <ogdf/planarity/PlanarizationLayout.h>

#include <QTimer>


enum GraphTypes
{
    Random,
    Simple,
    Tree,
    RoundTree,
    PlanarTree,
    Wheel,
    Petersen,
    PlanarPetersen
};


COGDFNewGraphDialog::COGDFNewGraphDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::COGDFNewGraphDialog)
{
    ui->setupUi(this);

    QStringList graphTypes;
    graphTypes << tr("Random Graph");
    graphTypes << tr("Simple Graph");
    graphTypes << tr("Tree Graph");
    graphTypes << tr("Round Tree Graph");
    graphTypes << tr("Planar Tree Graph");
    graphTypes << tr("Wheel Graph");
    graphTypes << tr("Petersen Graph");
    graphTypes << tr("Planar Petersen Graph");

    ui->List->addItems(graphTypes);

    ui->List->setCurrentRow(0);
}


COGDFNewGraphDialog::~COGDFNewGraphDialog()
{
    delete ui;
}


void COGDFNewGraphDialog::on_List_itemActivated(QListWidgetItem *item)
{
	if (item)
		accept();
}


void COGDFNewGraphDialog::on_List_currentRowChanged(int currentRow)
{
    // hide all items
    ui->Nodes->setEnabled(true);
    ui->Edges->setEnabled(true);
    ui->Jumps->setEnabled(true);

    switch (currentRow)
    {
    case Tree:
    case RoundTree:
    case PlanarTree:
    case Wheel:
    case Petersen:
    case PlanarPetersen:
        ui->Edges->setEnabled(false);
        break;
    }

    switch (currentRow)
    {
    case Random:
    case Simple:
    case Tree:
    case RoundTree:
    case PlanarTree:
    case Wheel:
        ui->Jumps->setEnabled(false);
        break;
    }
}


bool COGDFNewGraphDialog::exec(CNodeEditorScene &scene)
{
    QTimer::singleShot(0, this, SLOT(raise()));

    if (QDialog::exec() == Rejected)
        return false;

    // create graph
    ogdf::Graph G;
    ogdf::GraphAttributes GA(G, ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics);

    ogdf::FMMMLayout fmmm;
    ogdf::CircularLayout circle;
    ogdf::PlanarizationLayout planar;

    switch (ui->List->currentRow())
    {
    case Random:
        ogdf::randomGraph(G, ui->Nodes->value(), ui->Edges->value());
        fmmm.call(GA);
        break;

    case Simple:
        ogdf::randomSimpleGraph(G, ui->Nodes->value(), ui->Edges->value());
        fmmm.call(GA);
        break;

    case Tree:
        ogdf::randomTree(G, ui->Nodes->value());
        fmmm.call(GA);
        break;

    case RoundTree:
        ogdf::randomTree(G, ui->Nodes->value());
        circle.call(GA);
        break;

    case PlanarTree:
        ogdf::randomTree(G, ui->Nodes->value());
        planar.call(GA);
        break;

    case Wheel:
        ogdf::wheelGraph(G, ui->Nodes->value());
        circle.call(GA);
        break;

    case Petersen:
        ogdf::petersenGraph(G, ui->Nodes->value() / 2, ui->Jumps->value());
        fmmm.call(GA);
        break;

    case PlanarPetersen:
        ogdf::petersenGraph(G, ui->Nodes->value() / 2, ui->Jumps->value());
        planar.call(GA);
        break;
    }

	scene.addUndoState();

    COGDFLayout::graphTopologyToScene(G, GA, scene);

	scene.addUndoState();

    return true;
}

