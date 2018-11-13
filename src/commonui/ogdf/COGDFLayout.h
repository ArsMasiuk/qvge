#ifndef COGDFLAYOUT_H
#define COGDFLAYOUT_H

// stl
#include <string>

// qt
#include <QVariant>

// qvge
class CNodeEditorScene;

// ogdf
namespace ogdf
{
class LayoutModule;
class Graph;
class GraphAttributes;
}


class COGDFLayout
{
public:
    COGDFLayout();

    static void doLayout(ogdf::LayoutModule& layout, CNodeEditorScene &scene);

    static void graphTopologyToScene(const ogdf::Graph &G, const ogdf::GraphAttributes &GA, CNodeEditorScene &scene);
    static void graphToScene(const ogdf::Graph &G, const ogdf::GraphAttributes &GA, CNodeEditorScene &scene);

    static bool loadGraph(const std::string &filename, CNodeEditorScene &scene);
};

#endif // COGDFLAYOUT_H
