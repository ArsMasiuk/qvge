#ifndef CGRAPHINTERFACE_H
#define CGRAPHINTERFACE_H

#include "IGraphInterface.h"

class CNodeEditorScene;


class CGraphInterface: public IGraphInterface
{
public:
    CGraphInterface() {}
    explicit CGraphInterface(CNodeEditorScene& scene);
    virtual ~CGraphInterface() {}

    void setScene(CNodeEditorScene& scene) {
        m_scene = &scene;
    }

    // interface (to move out?)
    virtual CNode* addNode(const QString& nodeId);    
	virtual CNode* getNode(const QString& nodeId, bool autoCreate = false);

	virtual CEdge* addEdge(const QString& edgeId, const QString& startNodeId, const QString& endNodeId);
	virtual CEdge* getEdge(const QString& edgeId);
	virtual bool setEdgeAttr(const QString& edgeId, const QByteArray& attrId, const QVariant& value);

    virtual QList<CEdge*> getEdges() const;
    virtual QList<CNode*> getNodes() const;

private:
    CNodeEditorScene *m_scene = nullptr;
};

#endif // CGRAPHINTERFACE_H
