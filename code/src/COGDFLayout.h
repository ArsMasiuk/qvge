#ifndef COGDFLAYOUT_H
#define COGDFLAYOUT_H


class CNodeEditorScene;

namespace ogdf
{
class LayoutModule;
}


class COGDFLayout
{
public:
    COGDFLayout();

    static void doLayout(ogdf::LayoutModule& layout, CNodeEditorScene &scene);
};

#endif // COGDFLAYOUT_H
