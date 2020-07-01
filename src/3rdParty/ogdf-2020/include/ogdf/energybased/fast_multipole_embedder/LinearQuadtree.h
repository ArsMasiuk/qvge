/** \file
 * \brief Declaration of class LinearQuadtree.
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

#include <ogdf/energybased/fast_multipole_embedder/FastUtils.h>
#include <ogdf/energybased/fast_multipole_embedder/FMEFunctional.h>

#define OGDF_LQ_M2L_MIN_BOUND 8
#define OGDF_LQ_WSPD_BRANCH_BOUND 16
#define OGDF_LQ_WSPD_BOUND 25

namespace ogdf {
namespace fast_multipole_embedder {

class LinearQuadtreeBuilder;
class WSPD;

class LinearQuadtree
{
	friend class LinearQuadtreeBuilder;
	friend class LinearQuadtreeBuilderList;
public:
	using NodeID = unsigned int;
	using PointID = unsigned int;

	struct LQPoint				// 16 byte
	{
		MortonNR mortonNr;		// 8 byte
		uint32_t node;			// 4 byte
		uint32_t ref;			// 4 byte
	};

	struct LQNode				// 27 byte
	{
		uint32_t level;			// 4 byte
		NodeID next;			// 4 byte
		NodeID child[4];		// 4 byte *4 = 16 byte
		uint32_t numChilds;		// 4 byte
		PointID firstPoint;		// 4 byte
		uint32_t numPoints;		// 4 byte
		bool fence;				// 1 byte
	};

	struct LQWSPair
	{
		LQWSPair(NodeID c, NodeID d) : a(c), b(d) {};
		NodeID a;
		NodeID b;
	};

	struct is_fence_condition_functor
	{
		const LinearQuadtree& tree;
		is_fence_condition_functor(const LinearQuadtree& t) : tree(t) { }
		inline bool operator()(NodeID u) {	return tree.isFence(u); }
	};

	//! creator
	inline is_fence_condition_functor is_fence_condition() const { 	return is_fence_condition_functor(*this); }


	struct is_leaf_condition_functor
	{
		const LinearQuadtree& tree;
		is_leaf_condition_functor(const LinearQuadtree& t) : tree(t) { }
		inline bool operator()(NodeID u) {	return tree.isLeaf(u); }
	};

	//! creator
	inline is_leaf_condition_functor is_leaf_condition() const { return is_leaf_condition_functor(*this); }


	//! simple functor for iterating over all nodes
	template<typename F>
	struct forall_tree_nodes_functor
	{
		const LinearQuadtree& tree;
		F func;
		NodeID begin;
		uint32_t numNodes;

		forall_tree_nodes_functor(const LinearQuadtree& t, F f, NodeID b, uint32_t num) : tree(t), func(f), begin(b), numNodes(num) { }

		inline void operator()()
		{
			NodeID it = begin;
			for (uint32_t i=0; i<numNodes; i++)
			{
				func(it);
				it = tree.nextNode(it);
			}
		}
	};

	//! creator
	template<typename F>
	inline forall_tree_nodes_functor<F> forall_tree_nodes(F f, NodeID begin, uint32_t num) const
	{
		return forall_tree_nodes_functor<F>(*this, f, begin, num);
	}

	//! simple functor for iterating over all children of a node
	template<typename F>
	struct forall_children_functor
	{
		const LinearQuadtree& tree;
		F func;

		forall_children_functor(const LinearQuadtree& t, F f) : tree(t), func(f) { }

		inline void operator()(NodeID u)
		{
			if (tree.isLeaf(u)) return;
			for (uint32_t i = 0; i < tree.numberOfChilds(u); i++) func(tree.child(u, i));
		}
	};

	//! creator
	template<typename F>
	inline forall_children_functor<F> forall_children(F f) const
	{
		return forall_children_functor<F>(*this, f);
	}


	//! simple functor for iterating over all points of a node
	template<typename Func>
	struct forall_points_functor
	{
		const LinearQuadtree& tree;
		Func func;

		forall_points_functor(const LinearQuadtree& t, const Func& f) : tree(t), func(f) { }

		inline void operator()(NodeID u)
		{
			PointID firstPoint = tree.firstPoint(u);
			PointID end = firstPoint + tree.numberOfPoints(u);
			for (PointID i = firstPoint; i < end; i++)
				func(i);
		}
	};

	//! creator
	template<typename Func>
	inline forall_points_functor<Func> forall_points(const Func& func) const
	{
		return forall_points_functor<Func>(*this, func);
	}

	//! functor for iterating over all ordered pairs of children of a node
	template<typename F>
	struct forall_ordered_pairs_of_children_functor
	{
		const LinearQuadtree& tree;
		F func;

		forall_ordered_pairs_of_children_functor(const LinearQuadtree& t, F f) : tree(t), func(f) { }

		inline void operator()(NodeID u)
		{
			if (tree.isLeaf(u)) return;
			for (uint32_t i = 0; i < tree.numberOfChilds(u); i++)
				for (uint32_t j = i+1; j < tree.numberOfChilds(u); j++)
					func(tree.child(u, i), tree.child(u, j));
		}
	};

	//! creator
	template<typename F>
	inline forall_ordered_pairs_of_children_functor<F> forall_ordered_pairs_of_children(F f)  const
	{
		return forall_ordered_pairs_of_children_functor<F>(*this, f);
	}

	//! top down traversal of the subtree of a given node
	template<typename F, typename CondType = true_condition>
	struct top_down_traversal_functor
	{
		const LinearQuadtree& tree;
		F func;
		CondType cond;

		top_down_traversal_functor(const LinearQuadtree& t, F f) : tree(t), func(f) { }
		top_down_traversal_functor(const LinearQuadtree& t, F f, CondType c) : tree(t), func(f), cond(c) { }

		inline void operator()(NodeID u)
		{
			if (cond(u)) {	func(u); tree.forall_children(*this)(u); };
		}
	};

	//! creator
	template<typename F>
	inline top_down_traversal_functor<F> top_down_traversal(F f) const
	{
		return top_down_traversal_functor<F>(*this, f);
	}

	//! creator
	template<typename F, typename Cond>
	inline top_down_traversal_functor<F, Cond> top_down_traversal(F f, Cond cond) const
	{
		return top_down_traversal_functor<F, Cond>(*this, f, cond);
	}


	//! bottom up traversal of the subtree of a given node
	template<typename F, typename CondType = true_condition>
	struct bottom_up_traversal_functor
	{
		const LinearQuadtree& tree;
		F func;
		CondType cond;

		bottom_up_traversal_functor(const LinearQuadtree& t, F f) : tree(t), func(f) { }
		bottom_up_traversal_functor(const LinearQuadtree& t, F f, CondType c) : tree(t), func(f), cond(c) { }

		inline void operator()(NodeID u)
		{
			if (cond(u)) { tree.forall_children(*this)(u); func(u); };
		}
	};

	//! creator
	template<typename F>
	inline bottom_up_traversal_functor<F> bottom_up_traversal(F f) const
	{
		return bottom_up_traversal_functor<F>(*this, f);
	}

	//! creator
	template<typename F, typename Cond>
	inline bottom_up_traversal_functor<F, Cond> bottom_up_traversal(F f, Cond cond) const
	{
		return bottom_up_traversal_functor<F, Cond>(*this, f, cond);
	}


	template<typename WSPairFuncType, typename DPairFuncType, typename DNodeFuncType, typename BranchCondType = true_condition>
	struct wspd_functor
	{
		const LinearQuadtree& tree;
		WSPairFuncType WSFunction;
		DPairFuncType DPairFunction;
		DNodeFuncType DNodeFunction;
		BranchCondType BranchCondFunction;

		wspd_functor(const LinearQuadtree& t, WSPairFuncType& wsf, DPairFuncType& dpf, DNodeFuncType& dnf)
		  : tree(t), WSFunction(wsf),  DPairFunction(dpf),  DNodeFunction(dnf) { }

		wspd_functor(const LinearQuadtree& t, WSPairFuncType& wsf, DPairFuncType& dpf, DNodeFuncType& dnf, BranchCondType& bc)
		  : tree(t), WSFunction(wsf),  DPairFunction(dpf),  DNodeFunction(dnf), BranchCondFunction(bc) { }

		inline void operator()(NodeID u)
		{
			if (BranchCondFunction(u))
			{
				if (tree.isLeaf(u) || tree.numberOfPoints(u) <= OGDF_LQ_WSPD_BOUND)
				{
					if (tree.numberOfPoints(u) > 1)
						DNodeFunction(u);
				} else
				{
					tree.forall_children(*this)(u);
					tree.forall_ordered_pairs_of_children(*this)(u);
				}
			}
		}


		inline void operator()(NodeID u, NodeID v)
		{
			if (tree.isWS(u, v))
			{
				if (tree.numberOfPoints(u) < OGDF_LQ_M2L_MIN_BOUND && tree.numberOfPoints(v) < OGDF_LQ_M2L_MIN_BOUND)
					DPairFunction(u, v);
				else
					WSFunction(u, v);
			}
			else
			{
				if ((tree.numberOfPoints(u) <= OGDF_LQ_WSPD_BRANCH_BOUND && tree.numberOfPoints(v) <= OGDF_LQ_WSPD_BRANCH_BOUND) ||
						tree.isLeaf(u) || tree.isLeaf(v))
				{
					DPairFunction(u, v);
				}
				else
				{
					if (tree.level(u) >= tree.level(v))
						tree.forall_children(pair_call(*this, v))(u);
					else
						tree.forall_children(pair_call(*this, u))(v);
				}
			}
		}
	};

	template<typename A, typename B, typename C, typename ConditionType>
	inline wspd_functor<A, B, C, ConditionType> forall_well_separated_pairs(A a, B b, C c, ConditionType cond)
	{
		return wspd_functor<A, B, C, ConditionType>(*this, a, b, c, cond);
	}

	template<typename A, typename B, typename C>
	inline wspd_functor<A, B, C> forall_well_separated_pairs(A a, B b, C c)
	{
		return wspd_functor<A, B, C>(*this, a, b, c);
	}

	struct StoreWSPairFunctor
	{
		LinearQuadtree& tree;
		StoreWSPairFunctor(LinearQuadtree& t) : tree(t) { }
		inline void operator()(NodeID a, NodeID b) { tree.addWSPD(a, b); }
	};

	StoreWSPairFunctor inline StoreWSPairFunction() { return StoreWSPairFunctor(*this); }


	struct StoreDirectPairFunctor
	{
		LinearQuadtree& tree;
		StoreDirectPairFunctor(LinearQuadtree& t) : tree(t) { }
		inline void operator()(NodeID a, NodeID b) { tree.addDirectPair(a, b); }
	};

	StoreDirectPairFunctor inline StoreDirectPairFunction() { return StoreDirectPairFunctor(*this); }


	struct StoreDirectNodeFunctor
	{
		LinearQuadtree& tree;
		StoreDirectNodeFunctor(LinearQuadtree& t) : tree(t) { }
		inline void operator()(NodeID a) { tree.addDirect(a); }
	};

	StoreDirectNodeFunctor inline StoreDirectNodeFunction() { return StoreDirectNodeFunctor(*this); }

	inline NodeID level(NodeID nodeID) const
	{
		return m_tree[nodeID].level;
	}

	inline NodeID nextNode(NodeID nodeID) const
	{
		return m_tree[nodeID].next;
	}

	inline void setNextNode(NodeID nodeID, NodeID next)
	{
		m_tree[nodeID].next = next;
	}

	inline void setLevel(NodeID nodeID, uint32_t level)
	{
		m_tree[nodeID].level = level;
	}

	inline PointID firstPoint(NodeID nodeID) const
	{
		return m_tree[nodeID].firstPoint;
	}

	inline void setFirstPoint(NodeID nodeID, PointID firstPoint)
	{
		m_tree[nodeID].firstPoint = firstPoint;
	}

	inline LQPoint& point(PointID pointID)
	{
		return m_points[pointID];
	}

	inline const LQPoint& point(PointID pointID) const
	{
		return m_points[pointID];
	}

	inline MortonNR mortonNr(PointID point) const
	{
		return m_points[point].mortonNr;
	}

	//! returns the number of children of node \p nodeID. for an inner node this is 1..4 and
	//! can be accessed by child(i). For a leaf the number of points in this leaf is returned
	//! starting with point child(0)
	inline uint32_t numberOfChilds(NodeID nodeID) const
	{
		return m_tree[nodeID].numChilds;
	}

	//! sets the number of children of a node
	inline void setNumberOfChilds(NodeID nodeID, uint32_t numChilds)
	{
		m_tree[nodeID].numChilds = numChilds;
	}

	//! returns the \p i th child index of node \p nodeID
	inline NodeID child(NodeID nodeID, uint32_t i) const
	{
		return m_tree[nodeID].child[i];
	}

	//! sets the \p i th child index of node \p nodeID
	inline void setChild(NodeID nodeID, uint32_t i, NodeID c)
	{
		m_tree[nodeID].child[i] = c;
	}

	//! returns true if the given node index is a leaf
	inline bool isLeaf(NodeID nodeID) const
	{
		return !m_tree[nodeID].numChilds;
	}

	//! sets the fence flag for node \p nodeID
	inline bool isFence(NodeID nodeID) const
	{
		return m_tree[nodeID].fence;
	}

	//! returns the number of points contained in the subtree of node \p nodeID
	inline uint32_t numberOfPoints(NodeID nodeID) const
	{
		return m_tree[nodeID].numPoints;
	}

	//! sets the number of nodes containted in node \p nodeID
	inline void setNumberOfPoints(NodeID nodeID, uint32_t numPoints)
	{
		m_tree[nodeID].numPoints = numPoints;
	}

	//! returns the index of the root
	inline NodeID root() const
	{
		return m_root;
	}

	//! returns the number of points in this tree
	inline uint32_t numberOfPoints() const
	{
		return m_numPoints;
	}

	//! returns the number of nodes in this tree
	inline uint32_t numberOfNodes() const
	{
		return m_numInnerNodes + m_numLeaves;
	}

	//! the upper bound for a compressed quadtree (2*numPoints)
	inline uint32_t maxNumberOfNodes() const
	{
		return m_maxNumNodes;
	}

	//! resets the tree
	void clear();

	//! constructor. required tree mem will be allocated
	LinearQuadtree(uint32_t n, float* origXPos, float* origYPos, float* origSize);

	//! destructor. tree mem will be released
	~LinearQuadtree(void);

	uint64_t sizeInBytes() const;

	inline NodeID pointLeaf(PointID point) const
	{
		return m_points[point].node;
	}

	inline void setPointLeaf(PointID point, NodeID leaf)
	{
		m_points[point].node = leaf;
	}

	inline float pointX(PointID point) const { return m_pointXPos[point]; }

	inline float pointY(PointID point) const { return m_pointYPos[point]; }

	inline float pointSize(PointID point) const { return m_pointSize[point]; }

	inline float* pointX() const { return m_pointXPos; }

	inline float* pointY() const { return m_pointYPos; }

	inline float* pointSize() const { return m_pointSize; }

	inline float nodeX(NodeID nodeID) const { return m_nodeXPos[nodeID]; }

	inline void setNodeX(NodeID nodeID, float x) { m_nodeXPos[nodeID] = x; }

	inline float nodeY(NodeID nodeID) const {	return m_nodeYPos[nodeID]; }

	inline void setNodeY(NodeID nodeID, float y) { m_nodeYPos[nodeID] = y; }

	inline float nodeSize(NodeID nodeID) const { return m_nodeSize[nodeID]; }

	inline void setNodeSize(NodeID nodeID, float size) { m_nodeSize[nodeID] = size; }

	void setPoint(PointID id, float x, float y, uint32_t ref)
	{
		m_pointXPos[id] = x;
		m_pointYPos[id] = y;
		m_points[id].ref = ref;
	}

	void updatePointPositionSize(PointID id)
	{
		uint32_t ref = m_points[id].ref;
		m_pointXPos[id] = m_origXPos[ref];
		m_pointYPos[id] = m_origYPos[ref];
		m_pointSize[id] = m_origSize[ref];
	}

	void setPoint(PointID id, float x, float y, float r, uint32_t ref)
	{
		m_pointXPos[id] = x;
		m_pointYPos[id] = y;
		m_pointSize[id] = r;
		m_points[id].ref = ref;
	}

	void setPoint(PointID id, float x, float y, float r)
	{
		m_pointXPos[id] = x;
		m_pointYPos[id] = y;
		m_pointSize[id] = r;
	}

	inline uint32_t refOfPoint(PointID id) const
	{
		return m_points[id].ref;
	}

	inline NodeID nodeOfPoint(PointID id) const
	{
		return m_points[id].node;
	}

	inline void nodeFence(NodeID nodeID)
	{
		m_tree[nodeID].fence = true;
	}

	inline bool isWS(NodeID a, NodeID b) const
	{
		float s = 0.00000001f;
		float dx = nodeX(a) - nodeX(b);
		float dy = nodeY(a) - nodeY(b);
		float d_sq = dx*dx+dy*dy;
		float size = max(nodeSize(a), nodeSize(b));
		return d_sq > (s * 0.5 + 1) * (s * 0.5 + 1) * 2 * size * size;
	}

	void computeWSPD();

	void computeWSPD(NodeID n);

	inline NodeID firstInnerNode() const { return m_firstInner; }

	inline uint32_t numberOfInnerNodes() const { return m_numInnerNodes; }

	inline NodeID firstLeaf() const { return m_firstLeaf; }

	inline uint32_t numberOfLeaves() const { return m_numLeaves; }


	uint32_t numberOfWSP() const { return m_numWSP; }

	uint32_t numberOfDirectPairs() const { return m_numNotWSP; }

	uint32_t numberOfDirectNodes() const { return m_numDirectNodes; }

	inline NodeID directNode(uint32_t i) const { return m_directNodes[i]; }

	inline NodeID directNodeA(uint32_t i) const { return m_notWspd[i].a; }

	inline NodeID directNodeB(uint32_t i) const { return m_notWspd[i].b; }


	WSPD* wspd() const{ return m_WSPD; };

	void init(float min_x, float min_y, float max_x, float max_y);
	inline float minX() const { return m_min_x; }
	inline float minY() const { return m_min_y; }
	inline float maxX() const { return m_max_x; }
	inline float maxY() const { return m_max_y; }
	inline double scaleInv() const { return m_scaleInv; }


	inline void computeCoords(NodeID nodeIndex)
	{
		uint32_t ix, iy;
		uint32_t level = this->level(nodeIndex);
		float s = (float)(m_cellSize * (1 << level));
		this->setNodeSize(nodeIndex, s);
		MortonNR mnr = this->mortonNr(this->firstPoint(nodeIndex));
		mnr = mnr >> (level * 2);
		mnr = mnr << (level * 2);
		mortonNumberInv<uint64_t, uint32_t>(mnr, ix, iy);
		this->setNodeX(nodeIndex, (float)((m_sideLengthPoints*((float)ix)-0.5)/m_sideLengthGrid + (float)m_min_x + (float)s*0.5f));
		this->setNodeY(nodeIndex, (float)((m_sideLengthPoints*((float)iy)-0.5)/m_sideLengthGrid + (float)m_min_y + (float)s*0.5f));
	}

	LQPoint* pointArray() { return m_points; }

	PointID findFirstPointInCell(PointID somePointInCell) const;

private:
	//! helper function for allocating the array's
	void allocate(uint32_t n);

	//! helper function for releasing memory
	void deallocate();

	inline void initLeaf(NodeID leaf, PointID firstPoint, uint32_t numPoints, NodeID next)
	{
		m_tree[leaf].numChilds = 0;
		m_tree[leaf].next = next;
		m_tree[leaf].fence = 0;
		m_tree[leaf].level = 0;
		m_tree[leaf].firstPoint = firstPoint;
		m_tree[leaf].numPoints = numPoints;
	}

	inline void initInnerNode(NodeID nodeID, NodeID leftChild, NodeID rightChild, uint32_t level, NodeID next)
	{
		m_tree[nodeID].numChilds = 2;
		m_tree[nodeID].child[0] = leftChild;
		m_tree[nodeID].child[1] = rightChild;
		m_tree[nodeID].next = next;
		m_tree[nodeID].fence = 0;
		m_tree[nodeID].level = level;
		m_tree[nodeID].firstPoint = leftChild;
		m_tree[nodeID].numPoints = rightChild - leftChild;
	}

	//! appends one child index. Assumes childCount < 4 and not leaf
	inline void nodeAppendChild(NodeID nodeID, NodeID child)
	{
		m_tree[nodeID].child[m_tree[nodeID].numChilds++] = child;
		m_tree[nodeID].numPoints += m_tree[child].numPoints;
	}

	//! appends an successing point by simply increasing childcount of a leaf. Assumes isLeaf
	inline void leafAppendPoint(NodeID leaf, PointID point)
	{
		m_points[point].node = leaf;
		m_tree[leaf].numPoints++;
	}

	//! adds a well-separated pair to the wspd
	void addWSPD(NodeID s, NodeID t);

	//! add a direct pair to the array list of direct pairs
	void addDirectPair(NodeID s, NodeID t);

	//! add a direct node to the array list of direct nodes
	void addDirect(NodeID s);

	//! the x coordinate of the left most point
	float m_min_x;

	//! the y coordinate of the bottom most point
	float m_min_y;

	//! the x coordinate of the right most point
	float m_max_x;

	//! the y coordinate of the top most point
	float m_max_y;

	//! the height and width of a grid cell
	double m_cellSize;

	//! the inverse scale to transform
	double m_scaleInv;

	//! the maximum of height and width
	double m_sideLengthPoints;

	//! the resulting side length of the grid (constant)
	double m_sideLengthGrid;

	//! point x coord in graph order
	float*	 m_origXPos;

	//! point y coord in graph order
	float*	 m_origYPos;

	//! point size in graph order
	float*	 m_origSize;

	//! point x coord in tree order
	float*   m_pointXPos;

	//! point y coord in tree order
	float*	 m_pointYPos;

	//! point size in tree order
	float*   m_pointSize;

	//! node x coord
	float*   m_nodeXPos;


	//! node y coord
	float*   m_nodeYPos;

	//! node size
	float*   m_nodeSize;

	//! the main tree array containing all nodes (including leaves)
	LQNode*  m_tree;

	//! the maximum number of nodes (2*n here instead of 2*n-1)
	uint32_t m_maxNumNodes;

	//! the point order in tree order
	LQPoint* m_points;

	//! number of points this quadtree is based on
	uint32_t m_numPoints;

	uint32_t m_numWSP;

	LQWSPair* m_notWspd;
	uint32_t m_numNotWSP;

	NodeID* m_directNodes;
	uint32_t m_numDirectNodes;

	//! the wspd of this quadtree
	WSPD* m_WSPD;

	//! the root of the tree
	NodeID m_root;

	//! first leaf in the leaf chain
	NodeID m_firstLeaf;

	//! number of leaves in the chain
	uint32_t m_numLeaves;

	//! first inner node in the inner node chain
	NodeID m_firstInner;

	//! number of inner nodes in the chain
	uint32_t m_numInnerNodes;

};

inline bool LQPointComparer(const LinearQuadtree::LQPoint& a, const LinearQuadtree::LQPoint& b)
{
	return a.mortonNr < b.mortonNr;
}

}
}
