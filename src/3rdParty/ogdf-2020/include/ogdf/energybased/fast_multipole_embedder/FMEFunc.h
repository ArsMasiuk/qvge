/** \file
 * \brief Definitions of various auxiliary classes for FME layout.
 *
 * \author Martin Gronemann
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/energybased/fast_multipole_embedder/ArrayGraph.h>
#include <ogdf/energybased/fast_multipole_embedder/LinearQuadtree.h>
#include <ogdf/energybased/fast_multipole_embedder/LinearQuadtreeExpansion.h>
#include <ogdf/energybased/fast_multipole_embedder/LinearQuadtreeBuilder.h>
#include <ogdf/energybased/fast_multipole_embedder/WSPD.h>
#include <ogdf/energybased/fast_multipole_embedder/FMEKernel.h>
#include <list>

namespace ogdf {
namespace fast_multipole_embedder {


//! struct for distributing subtrees to the threads
struct FMETreePartition
{
	std::list<LinearQuadtree::NodeID> nodes;
	uint32_t pointCount;

	template<typename Func>
	void for_loop(Func& func)
	{
		for (LinearQuadtree::NodeID id : nodes)
			func(id);
	}
};

struct FMENodeChainPartition
{
	uint32_t begin;
	uint32_t numNodes;
};


//! the main global options for a run
struct FMEGlobalOptions
{
	float preProcTimeStep;				//!< time step factor for the preprocessing step
	float preProcEdgeForceFactor;		//!< edge force factor for the preprocessing step
	uint32_t preProcMaxNumIterations;	//!< number of iterations the preprocessing is applied

	float timeStep;						//!< time step factor for the main step
	float edgeForceFactor;				//!< edge force factor for the main step
	float repForceFactor;				//!< repulsive force factor for the main step
	float normEdgeLength;				//!< average edge length when desired edge length are normalized
	float normNodeSize;					//!< average node size when node sizes are normalized
	uint32_t maxNumIterations;			//!< maximum number of iterations in the main step
	uint32_t minNumIterations;			//!< minimum number of iterations to be done regardless of any other conditions

	bool doPrepProcessing;				//!< enable preprocessing
	bool doPostProcessing;				//!< enable postprocessing

	double stopCritForce;				//!< stopping criteria
	double stopCritAvgForce;				//!< stopping criteria
	double stopCritConstSq;				//!< stopping criteria

	uint32_t multipolePrecision;
};


//! forward decl of local context struct
struct FMELocalContext;

//! Global Context
struct FMEGlobalContext
{
	FMELocalContext** pLocalContext;		//!< all local contexts
	uint32_t numThreads;					//!< number of threads, local contexts
	ArrayGraph* pGraph;						//!< pointer to the array graph
	LinearQuadtree* pQuadtree;				//!< pointer to the quadtree
	LinearQuadtreeExpansion* pExpansion;	//!< pointer to the coeefficients
	WSPD* pWSPD;							//!< pointer to the well separated pairs decomposition
	float* globalForceX;					//!< the global node force x array
	float* globalForceY;					//!< the global node force y array
	FMEGlobalOptions* pOptions;				//!< pointer to the global options
	bool earlyExit;							//!< var for the main thread to notify the other threads that they are done
	float scaleFactor;						//!< var
	float coolDown;
	float min_x;							//!< global point, node min x coordinate for bounding box calculations
	float max_x;							//!< global point, node max x coordinate for bounding box calculations
	float min_y;							//!< global point, node min y coordinate for bounding box calculations
	float max_y;							//!< global point, node max y coordinate for bounding box calculations
	double currAvgEdgeLength;
};

//! Local thread Context
struct FMELocalContext
{
	FMEGlobalContext* pGlobalContext;		//!< pointer to the global context
	float* forceX;							//!< local force array for all nodes, points
	float* forceY;							//!< local force array for all nodes, points
	double maxForceSq;						//!< local maximum force
	double avgForce;						//!< local maximum force
	float min_x;							//!< global point, node min x coordinate for bounding box calculations
	float max_x;							//!< global point, node max x coordinate for bounding box calculations
	float min_y;							//!< global point, node min y coordinate for bounding box calculations
	float max_y;							//!< global point, node max y coordinate for bounding box calculations
	double currAvgEdgeLength;
	FMETreePartition treePartition;			  //!< tree partition assigned to the thread
	FMENodeChainPartition innerNodePartition; //!< chain of inner nodes assigned to the thread
	FMENodeChainPartition leafPartition;	  //!< chain of leaf nodes assigned to the thread

	LinearQuadtree::NodeID firstInnerNode;    //!< first inner nodes the thread prepared
	LinearQuadtree::NodeID lastInnerNode;     //!< last inner nodes the thread prepared
	uint32_t numInnerNodes;					  //!< number of inner nodes the thread prepared

	LinearQuadtree::NodeID firstLeaf;		  //!< first leaves the thread prepared
	LinearQuadtree::NodeID lastLeaf;		  //!< last leaves the thread prepared
	uint32_t numLeaves;						  //!< number of leaves the thread prepared
};

//! creates a min max functor for the x coords of the node
static inline min_max_functor<float> min_max_x_function(FMELocalContext* pLocalContext)
{
	return min_max_functor<float>(pLocalContext->pGlobalContext->pGraph->nodeXPos(), pLocalContext->min_x, pLocalContext->max_x);
}

//! creates a min max functor for the y coords of the node
static inline min_max_functor<float> min_max_y_function(FMELocalContext* pLocalContext)
{
	return min_max_functor<float>(pLocalContext->pGlobalContext->pGraph->nodeYPos(), pLocalContext->min_y, pLocalContext->max_y);
}

class LQMortonFunctor
{
public:
	inline LQMortonFunctor ( FMELocalContext* pLocalContext )
	{
		x = pLocalContext->pGlobalContext->pGraph->nodeXPos();
		y = pLocalContext->pGlobalContext->pGraph->nodeYPos();
		s = pLocalContext->pGlobalContext->pGraph->nodeSize();
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		translate_x = -quadtree->minX();
		translate_y = -quadtree->minY();
		scale = quadtree->scaleInv();
	}

	inline uint32_t operator()(void) const
	{
		return quadtree->numberOfPoints();
	}

	inline void operator()(uint32_t i)
	{
		LinearQuadtree::LQPoint& p = quadtree->point(i);
		uint32_t ref = p.ref;
		p.mortonNr = mortonNumber<uint64_t, uint32_t>((uint32_t)((x[ref] + translate_x)*scale), (uint32_t)((y[ref] + translate_y)*scale));
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	float translate_x;
	float translate_y;
	double scale;
	float* x;
	float* y;
	float* s;
};


//! Point-to-Multipole functor
struct p2m_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;

	p2m_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e) : tree(t), expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		uint32_t numPointsInLeaf = tree.numberOfPoints(nodeIndex);
		uint32_t firstPointOfLeaf = tree.firstPoint(nodeIndex);
		for (uint32_t pointIndex=firstPointOfLeaf; pointIndex < (firstPointOfLeaf+numPointsInLeaf); pointIndex++)
		{
			expansions.P2M(pointIndex, nodeIndex);
		}
	}
};


//! creates a Point-to-Multipole functor
static inline p2m_functor p2m_function(FMELocalContext* pLocalContext)
{
	return p2m_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion);
}


//! Multipole-to-Multipole functor
struct m2m_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;

	m2m_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e) : tree(t), expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID parent, LinearQuadtree::NodeID child)
	{
		expansions.M2M(child, parent);
	}

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		tree.forall_children(pair_call(*this, nodeIndex))(nodeIndex);
	}
};


//! creates Multipole-to-Multipole functor
static inline m2m_functor m2m_function(FMELocalContext* pLocalContext)
{
	return m2m_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion);
}


//! Multipole-to-Local functor
struct m2l_functor
{
	LinearQuadtreeExpansion& expansions;

	m2l_functor(LinearQuadtreeExpansion& e) : expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndexSource, LinearQuadtree::NodeID nodeIndexReceiver)
	{
		expansions.M2L(nodeIndexSource, nodeIndexReceiver);
	}
};


//! creates Multipole-to-Local functor
static inline m2l_functor m2l_function(FMELocalContext* pLocalContext)
{
	return m2l_functor(*pLocalContext->pGlobalContext->pExpansion);
}


//! Local-to-Local functor
struct l2l_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;

	l2l_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e) : tree(t), expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID parent, LinearQuadtree::NodeID child)
	{
		expansions.L2L(parent, child);
	}

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		tree.forall_children(pair_call(*this, nodeIndex))(nodeIndex);
	}
};


//! creates Local-to-Local functor
static inline l2l_functor l2l_function(FMELocalContext* pLocalContext)
{
	return l2l_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion);
}


//! Local-to-Point functor
struct l2p_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;
	float* fx;
	float* fy;

	l2p_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e, float* x, float* y) : tree(t), expansions(e), fx(x), fy(y) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndex, LinearQuadtree::PointID pointIndex)
	{
		expansions.L2P(nodeIndex, pointIndex, fx[pointIndex], fy[pointIndex]);
	}

	inline void operator()(LinearQuadtree::PointID pointIndex)
	{
		LinearQuadtree::NodeID nodeIndex = tree.pointLeaf(pointIndex);
		this->operator ()(nodeIndex, pointIndex);
	}
};


//! creates Local-to-Point functor
static inline l2p_functor l2p_function(FMELocalContext* pLocalContext)
{
	return l2p_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion, pLocalContext->forceX, pLocalContext->forceY);
}


//! Local-to-Point functor
struct p2p_functor
{
	const LinearQuadtree& tree;
	float* fx;
	float* fy;

	p2p_functor(const LinearQuadtree& t, float* x, float* y) : tree(t), fx(x), fy(y) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndexA, LinearQuadtree::NodeID nodeIndexB)
	{
		uint32_t offsetA = tree.firstPoint(nodeIndexA);
		uint32_t offsetB = tree.firstPoint(nodeIndexB);
		uint32_t numPointsA = tree.numberOfPoints(nodeIndexA);
		uint32_t numPointsB = tree.numberOfPoints(nodeIndexB);
		eval_direct_fast(tree.pointX() + offsetA, tree.pointY() + offsetA,
		                 tree.pointSize() + offsetA,
		                 fx + offsetA, fy + offsetA, numPointsA,
		                 tree.pointX() + offsetB, tree.pointY() + offsetB,
		                 tree.pointSize() + offsetB,
		                 fx + offsetB, fy + offsetB, numPointsB);
	}

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		uint32_t offset = tree.firstPoint(nodeIndex);
		uint32_t numPoints = tree.numberOfPoints(nodeIndex);
		eval_direct_fast(tree.pointX() + offset, tree.pointY() + offset, tree.pointSize() + offset, fx + offset, fy + offset, numPoints);
	}
};


//! creates Local-to-Point functor
static inline p2p_functor p2p_function(FMELocalContext* pLocalContext)
{
	return p2p_functor(*pLocalContext->pGlobalContext->pQuadtree, pLocalContext->forceX, pLocalContext->forceY);
}


//! The partitioner which partitions the quadtree into subtrees and partitions the sequence of inner nodes and leaves
class LQPartitioner
{
public:
	explicit LQPartitioner( FMELocalContext* pLocalContext )
	: numPointsPerThread(0)
	, numThreads(pLocalContext->pGlobalContext->numThreads)
	, currThread(0)
	, tree(pLocalContext->pGlobalContext->pQuadtree)
	, localContexts(pLocalContext->pGlobalContext->pLocalContext)
	{ }

	void partitionNodeChains()
	{
		uint32_t numInnerNodesPerThread = tree->numberOfInnerNodes() / numThreads;
		if (numInnerNodesPerThread < 25)
		{
			localContexts[0]->innerNodePartition.begin = tree->firstInnerNode();
			localContexts[0]->innerNodePartition.numNodes =  tree->numberOfInnerNodes();
			for (uint32_t i=1; i< numThreads; i++)
			{
				localContexts[i]->innerNodePartition.numNodes = 0;
			}
		} else
		{

			LinearQuadtree::NodeID curr = tree->firstInnerNode();
			currThread = 0;
			localContexts[0]->innerNodePartition.begin = curr;
			localContexts[0]->innerNodePartition.numNodes = 0;
			for (uint32_t i=0; i< tree->numberOfInnerNodes(); i++)
			{
				localContexts[currThread]->innerNodePartition.numNodes++;
				curr = tree->nextNode(curr);
				if (localContexts[currThread]->innerNodePartition.numNodes >= numInnerNodesPerThread && currThread < numThreads - 1)
				{
					currThread++;
					localContexts[currThread]->innerNodePartition.numNodes = 0;
					localContexts[currThread]->innerNodePartition.begin = curr;
				}
			}

		}

		uint32_t numLeavesPerThread = tree->numberOfLeaves() / numThreads;
		if (numLeavesPerThread < 25)
		{
			localContexts[0]->leafPartition.begin = tree->firstLeaf();
			localContexts[0]->leafPartition.numNodes =  tree->numberOfLeaves();
			for (uint32_t i=1; i< numThreads; i++)
			{
				localContexts[i]->leafPartition.numNodes = 0;
			}
		} else
		{
			LinearQuadtree::NodeID curr = tree->firstLeaf();
			currThread = 0;
			localContexts[0]->leafPartition.begin = curr;
			localContexts[0]->leafPartition.numNodes = 0;
			for (uint32_t i=0; i< tree->numberOfLeaves(); i++)
			{
				localContexts[currThread]->leafPartition.numNodes++;
				curr = tree->nextNode(curr);
				if (localContexts[currThread]->leafPartition.numNodes >= numLeavesPerThread && currThread < numThreads - 1)
				{
					currThread++;
					localContexts[currThread]->leafPartition.numNodes = 0;
					localContexts[currThread]->leafPartition.begin = curr;
				}
			}
		}
	}

	void partition()
	{
		partitionNodeChains();
		currThread = 0;
		numPointsPerThread = tree->numberOfPoints() / numThreads;
		for (uint32_t i=0; i < numThreads; i++)
		{
			localContexts[i]->treePartition.nodes.clear();
			localContexts[i]->treePartition.pointCount = 0;
		}
		if (numThreads>1)
			newPartition();
	}

	void newPartition(uint32_t nodeID)
	{
		uint32_t bound = tree->numberOfPoints() / (numThreads*numThreads);

		if (tree->isLeaf(nodeID) || tree->numberOfPoints(nodeID) < bound)
			l_par.push_back(nodeID);
		else
			for (uint32_t i = 0; i < tree->numberOfChilds(nodeID); i++)
				newPartition(tree->child(nodeID, i));
	}

	void newPartition()
	{
		l_par.clear();
		newPartition(tree->root());
		uint32_t bound = (tree->numberOfPoints() / (numThreads)) + (tree->numberOfPoints() / (numThreads*numThreads*2));
		while (!l_par.empty())
		{
			FMETreePartition* partition = currPartition();
			uint32_t v = l_par.front();
			if (((partition->pointCount + tree->numberOfPoints(v)) <= bound) ||
				(currThread==numThreads-1))
			{
				partition->pointCount += tree->numberOfPoints(v);
				partition->nodes.push_back(v);
				tree->nodeFence(v);
				l_par.pop_front();
			} else
			{
				currThread++;
			}
		}
	}

	FMETreePartition* currPartition()
	{
		return &localContexts[currThread]->treePartition;
	}

private:
	uint32_t numPointsPerThread;
	uint32_t numThreads;
	uint32_t currThread;
	std::list<uint32_t> l_par;
	LinearQuadtree* tree;
	FMELocalContext** localContexts;
};


class LQPointUpdateFunctor
{
public:
	inline explicit LQPointUpdateFunctor ( FMELocalContext* pLocalContext )
	{
		x = pLocalContext->pGlobalContext->pGraph->nodeXPos();
		y = pLocalContext->pGlobalContext->pGraph->nodeYPos();
		s = pLocalContext->pGlobalContext->pGraph->nodeSize();
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
	}

	inline uint32_t operator()(void) const
	{
		return quadtree->numberOfPoints();
	}

	inline void operator()(uint32_t i)
	{
		LinearQuadtree::LQPoint& p = quadtree->point(i);
		uint32_t ref = p.ref;
		quadtree->setPoint(i, x[ref], y[ref], s[ref]);
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	float* x;
	float* y;
	float* s;
};


/*!
 * Computes the coords and size of the i-th node in the LinearQuadtree
 */
class LQCoordsFunctor
{
public:
	inline explicit LQCoordsFunctor(	FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
	}

	inline uint32_t operator()(void) const
	{
		return quadtree->numberOfNodes();
	}

	inline void operator()( uint32_t i )
	{
		quadtree->computeCoords(i);
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
};


/*!
 * Converts the multipole expansion coefficients from all nodes which are well separated from the i-th node
 * to local expansion coefficients and adds them to the local expansion coefficients of the i-th node
 */
class M2LFunctor
{
public:
	inline explicit M2LFunctor( FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
		wspd = pLocalContext->pGlobalContext->pWSPD;
	}

	inline uint32_t operator()(void) const
	{
		return quadtree->numberOfNodes();
	}

	inline void operator()(uint32_t i)
	{
		uint32_t currEntryIndex = wspd->firstPairEntry(i);
		for (uint32_t k = 0; k < wspd->numWSNodes(i); k++)
		{
			uint32_t j = wspd->wsNodeOfPair(currEntryIndex, i);
			quadtreeExp->M2L(j, i);
			currEntryIndex = wspd->nextPair(currEntryIndex, i);
		}
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
	WSPD* wspd;
};


/*!
 * Calculates the repulsive forces acting between all nodes inside the cell of the i-th LinearQuadtree node.
 */
class NDFunctor
{
public:
	inline explicit NDFunctor( FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
		forceArrayX = pLocalContext->forceX;
		forceArrayY = pLocalContext->forceY;
	}

	inline uint32_t operator()(void) const
	{
		return quadtree->numberOfDirectNodes();
	}

	inline void operator()(uint32_t i)
	{
		uint32_t nodeI = quadtree->directNode(i);
		uint32_t offset = quadtree->firstPoint(nodeI);
		uint32_t numPoints = quadtree->numberOfPoints(nodeI);
		eval_direct_fast(quadtree->pointX() + offset, quadtree->pointY() + offset, quadtree->pointSize() + offset, forceArrayX + offset, forceArrayY + offset, numPoints);
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
	float* forceArrayX;
	float* forceArrayY;
};


/*!
 * Calculates the repulsive forces acting between all nodes of the direct interacting cells of the i-th node
 */
class D2DFunctor
{
public:
	inline explicit D2DFunctor( FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
		forceArrayX = pLocalContext->forceX;
		forceArrayY = pLocalContext->forceY;
	}

	inline uint32_t operator()(void) const
	{
		return quadtree->numberOfDirectPairs();
	}

	inline void operator()(uint32_t i)
	{
		uint32_t nodeA = quadtree->directNodeA(i);
		uint32_t nodeB = quadtree->directNodeB(i);
		uint32_t offsetA = quadtree->firstPoint(nodeA);
		uint32_t offsetB = quadtree->firstPoint(nodeB);
		uint32_t numPointsA = quadtree->numberOfPoints(nodeA);
		uint32_t numPointsB = quadtree->numberOfPoints(nodeB);
		eval_direct_fast(quadtree->pointX() + offsetA, quadtree->pointY() + offsetA,
		                 quadtree->pointSize() + offsetA,
		                 forceArrayX + offsetA, forceArrayY + offsetA, numPointsA,
		                 quadtree->pointX() + offsetB, quadtree->pointY() + offsetB,
		                 quadtree->pointSize() + offsetB,
		                 forceArrayX + offsetB, forceArrayY + offsetB, numPointsB);
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
	float* forceArrayX;
	float* forceArrayY;
};

enum class FMEEdgeForce {
	SubRep    = 0x2,
	DivDegree = 0x8
};

inline int operator & (int lhs, FMEEdgeForce rhs) {
	return lhs & static_cast<int>(rhs);
}

template<unsigned int FLAGS>
class EdgeForceFunctor
{
public:
	inline explicit EdgeForceFunctor( FMELocalContext* pLocalContext )
	{
		pGraph = pLocalContext->pGlobalContext->pGraph;
		x = pGraph->nodeXPos();
		y = pGraph->nodeYPos();
		edgeInfo = pGraph->edgeInfo();
		nodeInfo = pGraph->nodeInfo();
		desiredEdgeLength = pGraph->desiredEdgeLength();
		nodeSize = pGraph->nodeSize();
		forceArrayX = pLocalContext->forceX;
		forceArrayY = pLocalContext->forceY;
	}

	inline uint32_t operator()(void) const
	{
		return pGraph->numEdges();
	}

	inline void operator()(uint32_t i)
	{
		const EdgeAdjInfo& e_info = edgeInfo[i];
		const NodeAdjInfo& a_info = nodeInfo[e_info.a];
		const NodeAdjInfo& b_info = nodeInfo[e_info.b];

		float d_x = x[e_info.a] - x[e_info.b];
		float d_y = y[e_info.a] - y[e_info.b];
		float d_sq = d_x*d_x + d_y*d_y;

		float f = (float)(logf(d_sq)*0.5f-logf(desiredEdgeLength[i]));

		float fa = f*0.25f;
		float fb = f*0.25f;

		if (FLAGS & FMEEdgeForce::DivDegree)
		{
			fa = (float)(fa/((float)a_info.degree));
			fb = (float)(fb/((float)b_info.degree));
		}

		if (FLAGS & FMEEdgeForce::SubRep)
		{
			fa += (nodeSize[e_info.b] / d_sq);
			fb += (nodeSize[e_info.a] / d_sq);
		}
		forceArrayX[e_info.a] -= fa*d_x;
		forceArrayY[e_info.a] -= fa*d_y;
		forceArrayX[e_info.b] += fb*d_x;
		forceArrayY[e_info.b] += fb*d_y;
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	float* x;
	float* y;
	EdgeAdjInfo* edgeInfo;
	NodeAdjInfo* nodeInfo;

	ArrayGraph* pGraph;
	float* desiredEdgeLength;
	float* nodeSize;
	float* forceArrayX;
	float* forceArrayY;
};


template<unsigned int FLAGS>
static inline EdgeForceFunctor<FLAGS> edge_force_function( FMELocalContext* pLocalContext )
{
	return EdgeForceFunctor<FLAGS>( pLocalContext );
}


enum class FMECollect
{
	NoFactor        = 0x00,
	EdgeFactor      = 0x01,
	RepulsiveFactor = 0x02,
	EdgeFactorRep   = 0x04,
	Tree2GraphOrder = 0x08,
	ZeroThreadArray = 0x10
};

inline int operator & (int lhs, FMECollect rhs) {
	return lhs & static_cast<int>(rhs);
}

inline constexpr int operator | (int lhs, FMECollect rhs) {
	return lhs | static_cast<int>(rhs);
}

inline constexpr int operator | (FMECollect lhs, FMECollect rhs) {
	return static_cast<int>(lhs) | static_cast<int>(rhs);
}

template<unsigned int FLAGS>
class CollectForceFunctor
{
public:

	inline explicit CollectForceFunctor( FMELocalContext* pLocalContext )
	{
		numContexts = pLocalContext->pGlobalContext->numThreads;
		globalContext = pLocalContext->pGlobalContext;
		localContexts = pLocalContext->pGlobalContext->pLocalContext;
		globalArrayX = globalContext->globalForceX;
		globalArrayY = globalContext->globalForceY;
		pGraph = pLocalContext->pGlobalContext->pGraph;
		if (FLAGS & FMECollect::EdgeFactor)
			factor = pLocalContext->pGlobalContext->pOptions->edgeForceFactor;
		else
		if (FLAGS & FMECollect::RepulsiveFactor)
			factor = pLocalContext->pGlobalContext->pOptions->repForceFactor;
		else
		if (FLAGS & FMECollect::EdgeFactorRep)
			factor = pLocalContext->pGlobalContext->pOptions->preProcEdgeForceFactor;
		else
			factor = 1.0;
	}

	inline uint32_t operator()(void) const
	{
		return pGraph->numNodes();
	}


	inline void operator()(uint32_t i)
	{
		float sumX = 0.0f;
		float sumY = 0.0f;
		for (uint32_t j=0; j < numContexts; j++)
		{
			float* localArrayX = localContexts[j]->forceX;
			float* localArrayY = localContexts[j]->forceY;
			sumX += localArrayX[i];
			sumY += localArrayY[i];
			if (FLAGS & FMECollect::ZeroThreadArray)
			{
				localArrayX[i] = 0.0f;
				localArrayY[i] = 0.0f;
			}
		}

		if (FLAGS & FMECollect::Tree2GraphOrder) {
			i = globalContext->pQuadtree->refOfPoint(i);
		}
		if (FLAGS & FMECollect::RepulsiveFactor
		 && pGraph->nodeInfo(i).degree > 100) {
			// prevent some evil effects
			sumX /= (float)pGraph->nodeInfo(i).degree;
			sumY /= (float)pGraph->nodeInfo(i).degree;
		}
		globalArrayX[i] += sumX*factor;
		globalArrayY[i] += sumY*factor;
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	ArrayGraph*	pGraph;
	FMEGlobalContext* globalContext;
	FMELocalContext** localContexts;
	float* globalArrayX;
	float* globalArrayY;
	uint32_t numContexts;
	float factor;
};


template<unsigned int FLAGS>
static inline CollectForceFunctor<FLAGS> collect_force_function( FMELocalContext* pLocalContext )
{
	return CollectForceFunctor<FLAGS>( pLocalContext );
}


static constexpr int TIME_STEP_NORMAL = 0x1;
static constexpr int TIME_STEP_PREP = 0x2;
static constexpr int ZERO_GLOBAL_ARRAY = 0x4;
static constexpr int USE_NODE_MOVE_RAD = 0x8;

template<unsigned int FLAGS>
class NodeMoveFunctor
{
public:
#if 1
	inline explicit NodeMoveFunctor(FMELocalContext* pLocalContext)
#else
	inline NodeMoveFunctor(FMELocalContext* pLocalContext,
	                       float* xCoords, float* yCoords,
	                       float ftimeStep,
	                       float* globalForceArray)
	  : x(xCoords), y(yCoords), timeStep(ftimeStep), forceArray(globalForceArray)
#endif
	{
		if (FLAGS & TIME_STEP_NORMAL)
			timeStep = pLocalContext->pGlobalContext->pOptions->timeStep * pLocalContext->pGlobalContext->coolDown;
		else
		if (FLAGS & TIME_STEP_PREP)
			timeStep = pLocalContext->pGlobalContext->pOptions->preProcTimeStep;
		else
			timeStep = 1.0;
		pGraph = pLocalContext->pGlobalContext->pGraph;
		x = pGraph->nodeXPos();
		y = pGraph->nodeYPos();
		nodeMoveRadius = pGraph->nodeMoveRadius();
		forceArrayX = pLocalContext->pGlobalContext->globalForceX;
		forceArrayY = pLocalContext->pGlobalContext->globalForceY;
		localContext = pLocalContext;
		currentEdgeLength = pLocalContext->pGlobalContext->pGraph->desiredEdgeLength();
	}

#if 0
	inline void operator()(void) const
	{
		return pGraph->numNodes();
	};
#endif

	inline void operator()(uint32_t i)
	{
		float d_x = forceArrayX[i]* timeStep;
		float d_y = forceArrayY[i]* timeStep;
		double dsq = (d_x*d_x + d_y*d_y);
		double d = sqrt(dsq);

		localContext->maxForceSq = max<double>(localContext->maxForceSq, (double)dsq );
		localContext->avgForce += d;
		if (d < FLT_MAX)
		{
			x[i] += d_x;
			y[i] += d_y;
			if (FLAGS & ZERO_GLOBAL_ARRAY)
			{
				forceArrayX[i] = 0.0;
				forceArrayY[i] = 0.0;
			} else
			{
				forceArrayX[i] = d_x;
				forceArrayY[i] = d_y;
			}
		} else
		{
			forceArrayX[i] = 0.0;
			forceArrayY[i] = 0.0;
		}
	}

	inline void operator()(uint32_t begin, uint32_t end)
	{
		for (uint32_t i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	float timeStep;
	float* x;
	float* y;
	float* forceArrayX;
	float* forceArrayY;
	float* nodeMoveRadius;
	float* currentEdgeLength;
	ArrayGraph* pGraph;
	FMELocalContext* localContext;
};


template<unsigned int FLAGS>
static inline NodeMoveFunctor<FLAGS> node_move_function( FMELocalContext* pLocalContext )
{
	return NodeMoveFunctor<FLAGS>( pLocalContext );
}


template<typename TYP>
inline void for_loop_array_set(uint32_t threadNr, uint32_t numThreads, TYP* a, uint32_t n, TYP value)
{
	uint32_t s = n/numThreads;
	uint32_t o = s*threadNr;
	if (threadNr == numThreads-1)
		s = s + (n % numThreads);

	for (uint32_t i = 0; i < s; i++ ) { a[o+i] = value; }
}

}
}
