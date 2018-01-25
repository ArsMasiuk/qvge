#ifndef COGDFLAYOUT_H
#define COGDFLAYOUT_H


class CNodeEditorScene;

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

    static void graphToScene(const ogdf::Graph &G, const ogdf::GraphAttributes &GA, CNodeEditorScene &scene);
};

#endif // COGDFLAYOUT_H
