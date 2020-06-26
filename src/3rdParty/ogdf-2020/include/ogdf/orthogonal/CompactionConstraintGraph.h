/** \file
 * \brief Declares CompactionConstraintGraph.
 *
 * I.e. a representation of constraint graphs (dependency graphs)
 * used in compaction algorithms.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/orthogonal/internal/RoutingChannel.h>
#include <ogdf/orthogonal/internal/CommonCompactionConstraintGraphBase.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/planarity/PlanRep.h>

namespace ogdf {

//! Class implementing template-parameter-independent behaviour of ogdf::CompactionConstraintGraph.
class CompactionConstraintGraphBase : public CommonCompactionConstraintGraphBase
{
public:
	//! \name Edge property getters
	//! @{

	//! Returns true if e is vertical edge in PlanRepUML hierarchy
	bool verticalGen(edge e) const {return m_verticalGen[e];}

	//! Returns true if e is basic arc of vertical edge in PlanRepUML hierarchy
	//! @pre e is arc in the constraint graph
	bool verticalArc(edge e) const {return m_verticalArc[e];}

	//! @}

	//! Triggers alignment (=>some special edge handling to support al.)
	void align(bool b) {m_align = b;}

	//! Returns if arc is important for alignment.
	//! These are the arcs representing node to gen. merger edges
	bool alignmentArc(edge e) const {return m_alignmentArc[e];}

	edge pathToOriginal(node v) {return m_pathToEdge[v];}

protected:
	//! Construction
	CompactionConstraintGraphBase(const OrthoRep &OR,
		const PlanRep &PG,
		OrthoDir arcDir,
		int costGen = 1,
		int costAssoc = 1, bool align = false);

	int m_edgeCost[2];

	//test fuer vorkomp. der Generalisierungen
	EdgeArray<bool> m_verticalGen; //!< generalization that runs vertical relative to hierarchy
	EdgeArray<bool> m_verticalArc; //!< arc corresponding to such an edge

	//! Basic arcs that have to be short for alignment (node to gen expander)
	EdgeArray<bool> m_alignmentArc;

	NodeArray<edge> m_pathToEdge; //!< save the (single!) edge (segment) for a pathNode

private:
	//set special costs for node to merger generalizations
	bool m_align;

	void insertPathVertices(const PlanRep &PG);
	void dfsInsertPathVertex(
		node v,
		node pathVertex,
		NodeArray<bool> &visited,
		const NodeArray<node> &genOpposite);

	void insertBasicArcs(const PlanRep &PG);
};


/**
 * Represents a constraint graph used for compaction
 *
 *   - Vertices: maximally connected horiz. (resp. vert.) paths.
 *   - Basic arcs: paths connected by edges of opposite direction.
 *   - Vertex size arcs: care for minimum size of cages.
 *   - Visibility arcs: paths seeing each other.
 *
 * Each edge has a (minimum) length and cost.
 */
template<class ATYPE>
class CompactionConstraintGraph : public CompactionConstraintGraphBase
{
public:
	//! Construction
	CompactionConstraintGraph(const OrthoRep &OR,
		const PlanRep &PG,
		OrthoDir arcDir,
		ATYPE sep,
		int costGen = 1,
		int costAssoc = 1,
		bool align = false) :
			CompactionConstraintGraphBase(OR, PG, arcDir, costGen, costAssoc, align)
	{
		OGDF_ASSERT(&(const Graph &)PG == &(const Graph &)OR);

		m_length   .init((Graph&)*this, sep);
		m_extraOfs .init((Graph&)*this, 0);
		m_extraRep .init((Graph&)*this, nullptr);

		m_sep       = sep;

		m_centerPriority = true; //should centering of single edges have prio. to gen. length
		m_genToMedian = true;  //should outgoing merger gen. be drawn to merger median

		initializeCosts();
	}

	//! Returns length of edge \p e
	//! @pre \p e is an edge in the constraint graph
	ATYPE length(edge e) const {
		return m_length[e];
	}

	//! Returns extraNode position, change to save mem, only need some entries
	ATYPE extraOfs(node v) const {
		return m_extraOfs[v];
	}

	//! Gets centerPriority (center single edges?)
	bool centerPriority() {return m_centerPriority;}
	//! Sets centerPriority (center single edges?)
	void centerPriority(bool b) { m_centerPriority = b;}

	//! Computes the total costs for coordintes given by pos, i.e.,
	//! the sum of the weighted lengths of edges in the constraint graph.
	ATYPE computeTotalCosts(const NodeArray<ATYPE> &pos) const;

	//! Inserts arcs for respecting sizes of vertices and achieving desired
	//! placement of generalizations if vertices are represented by variable
	//! cages; also corrects length of arcs belonging to cages which are
	//! adjacent to a corner; takes routing channels into account.
	void insertVertexSizeArcs(
		const PlanRep &PG,
		const NodeArray<ATYPE> &sizeOrig,
		const RoutingChannel<ATYPE> &rc);

	//! Inserts arcs for respecting sizes of vertices and achieving desired
	//! placement of generalizations if vertices are represented by tight cages.
	//! Also corrects length of arcs belonging to cages which are adjacent to
	//! a corner; takes special distances between edge segments attached at
	//! a vertex (delta's and epsilon's) into account.
	void insertVertexSizeArcs(
		const PlanRep &PG,
		const NodeArray<ATYPE> &sizeOrig,
		const MinimumEdgeDistances<ATYPE> &minDist);

	//! Inserts arcs connecting segments which can see each other in a drawing
	//! of the associated planarized representation PG which is given by
	//! posDir and posOppDir.
	void insertVisibilityArcs(
		const PlanRep &PG,  //!< associated planarized representation
		const NodeArray<ATYPE> &posDir, //!< position of segment containing vertex in PG
		const NodeArray<ATYPE> &posOppDir //!< position of orthogonal segment containing vertex in PG
	);

	void insertVisibilityArcs(
		const PlanRep &PG,
		const NodeArray<ATYPE> &posDir,
		const NodeArray<ATYPE> &posOrthDir,
		const MinimumEdgeDistances<ATYPE> &minDist);

	//! Sets min separation for multi edge original
	void setMinimumSeparation(const PlanRep &PG,
		const NodeArray<int> &coord,
		const MinimumEdgeDistances<ATYPE> &minDist);

	//! Performs feasibility test for position assignment pos, i.e., checks if
	//! the segment positions given by pos fulfill the constraints in the
	//! compaction constraint graph
	//! (for debuging only)
	bool isFeasible(const NodeArray<ATYPE> &pos);

	//! Returns the separation value
	ATYPE separation() const {return m_sep;}

	//! Return PG result for flowcompaction
	bool areMulti(edge e1, edge e2) const;

private:
	//! Represents an interval on the sweep line
	struct Interval
	{
		Interval(node v, ATYPE low, ATYPE high) {
			m_low = low;
			m_high = high;
			m_pathNode = v;
		}

		ATYPE m_low, m_high; //!< lower and upper bound
		node m_pathNode;     //!< corresponding segment

		//! output operator
		friend std::ostream &operator<<(std::ostream &os,
			const Interval &interval)
		{
			os << "[" << interval.m_low << "," << interval.m_high <<
				";" << interval.m_pathNode << "]";
			return os;
		}
	};

	//! Comparer class used for sorting segments by increasing position
	//! (given by segPos) such that two overlapping segments come in the
	//! order imposed by the embedding (given by secSort: segment which
	//! comes first has secSort = 0, the other has 1)
	class SegmentComparer
	{
	public:
		SegmentComparer(const NodeArray<ATYPE> &segPos,
			const NodeArray<int> &secSort) {
			m_pPos = &segPos;
			m_pSec = &secSort;
		}

		int compare(const node &x, const node &y) const {
			ATYPE d = (*m_pPos)[x] - (*m_pPos)[y];
			if (d < 0)
				return -1;
			else if (d > 0)
				return 1;
			else
				return (*m_pSec)[x] - (*m_pSec)[y];
		}

		OGDF_AUGMENT_COMPARER(node)
	private:
		const NodeArray<ATYPE> *m_pPos;
		const NodeArray<int>    *m_pSec;
	};

	virtual string getLengthString(edge e) const override {
		return to_string(m_length[e]);
	}

	void setBasicArcsZeroLength(const PlanRep &PG);
	void resetGenMergerLengths(const PlanRep &PG, adjEntry adjFirst);
	void setBoundaryCosts(adjEntry cornerDir,adjEntry cornerOppDir);

	bool checkSweepLine(const List<Interval> &sweepLine) const;

	ATYPE m_sep;

	EdgeArray<ATYPE> m_length; //!< length of an edge

	NodeArray<ATYPE> m_extraOfs; //!< offset of extra node to its rep, should change this

	//! \name Cost settings
	//! @{

	// we make vertex size arcs more expensive than basic arcs in order
	// to get small cages
	// should be replaced by option/value dependent on e.g. degree
	int m_vertexArcCost;  //!< get small cages
	int m_bungeeCost;     //!< middle position distance penalty
	int m_MedianArcCost;  //!< draw merger gen at median of incoming generalizations
	int m_doubleBendCost; //!< try to minimize double bends
	bool m_genToMedian;   //!< draw outgoing generalization from merger above ingoing gen.
	//this does not work if generalization costs are set very small by the user
	//because there must be a minimum cost for centering
	bool m_centerPriority; //!< should centering be more expensive than generalizations

	//factor of costs relative to generalization
	static const int c_vertexArcFactor;
	static const int c_bungeeFactor;
	static const int c_doubleBendFactor; //!< double bends cost factor*vertexArcCost
	static const int c_MedianFactor;     //!< median arcs cost  factor*vertexArcCost

	//! @}

protected:
	//! Node \p v has no representation in drawing, only internal representation
	void setExtra(node v, node rep, ATYPE ofs) {
		m_extraNode[v] = true;
		m_extraRep[v] = rep;
		m_extraOfs[v] = ofs;
	}

	void initializeCosts()
	{
		// we make vertex size arcs more expensive than basic arcs in order
		// to get small cages; not necessary if cage size fixed in improvement
		// cost should be dependend on degree
		// Z.B. DURCH OPTION ODER WERT; DER VON DER ZAHL ADJAZENTER KANTEN ABHAENGIG IST
		// should be derived by number of edges times something
		int costGen = m_edgeCost[static_cast<int>(Graph::EdgeType::generalization)];

		m_vertexArcCost = c_vertexArcFactor*costGen; //spaeter aus Kompaktierungsmodul uebergeben
		if (m_centerPriority)
			m_bungeeCost = c_bungeeFactor*costGen+1;//-1;//for distance to middle position,
		else
			m_bungeeCost = c_bungeeFactor*4+1;//-1;//for distance to middle position,
		//addition value should be < gen cost!!!
		m_MedianArcCost = c_MedianFactor*m_vertexArcCost;
		m_doubleBendCost = c_doubleBendFactor*m_vertexArcCost;
	}
};

//initialization of static members
template<class ATYPE>
const int CompactionConstraintGraph<ATYPE>::c_vertexArcFactor = 20;
template<class ATYPE>
const int CompactionConstraintGraph<ATYPE>::c_bungeeFactor = 20;
template<class ATYPE>
const int CompactionConstraintGraph<ATYPE>::c_doubleBendFactor = 20; //double bends cost mxxx*vertexArcCost
//factor *VertexArcCost, costs for pulling generalization to median position at merger
template<class ATYPE>
const int CompactionConstraintGraph<ATYPE>::c_MedianFactor = 10*c_doubleBendFactor;


// allow 0-length for sides of generalization merger cages
template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::resetGenMergerLengths(
	const PlanRep &PG,
	adjEntry adjFirst)
{
	adjEntry adj = adjFirst;
	int faceSize = 0;

	do {
		if ((m_pOR->direction(adj) == m_arcDir ||
			m_pOR->direction(adj) == m_oppArcDir) &&
			(PG.typeOf(adj->theNode()) == Graph::NodeType::dummy ||
			PG.typeOf(adj->twinNode()) == Graph::NodeType::dummy))
		{
			m_length[m_edgeToBasicArc[adj]] = 0;
		}

		adj = adj->faceCycleSucc();
		faceSize++;
	} while(adj != adjFirst);

	//generalization position section
	//pull upper generalization to median of merger cage's incoming lower generalizations

	if (m_genToMedian
	 && (m_pOR->direction(adjFirst) == m_arcDir
	  || m_pOR->direction(adjFirst) == m_oppArcDir)) {
		int numIncoming = faceSize - 3;
		int median = (numIncoming / 2) + 1;

		// if (numIncoming == 2) ... just the middle position
		node upper = m_pathNode[adjFirst->theNode()];
		if (PG.typeOf(adjFirst->theNode()) != Graph::NodeType::generalizationMerger) {
			OGDF_THROW(AlgorithmFailureException);
		}

		node vMin;
		if (m_pOR->direction(adjFirst) == m_arcDir) {
			vMin = adjFirst->faceCyclePred()->theNode();
		} else {
			vMin = adjFirst->faceCycleSucc()->theNode();
		}

		adj = adjFirst->faceCycleSucc(); // target right or left boundary, depending on drawing direction
		for (int i = 0; i < median; i++) {
			adj = adj->faceCycleSucc();
		}

		node lower = m_pathNode[adj->theNode()];
		node vCenter = newNode();
		setExtra(vCenter, vMin, 0);

		// it seems we dont need the helper, as only source-adjEntries lying on
		// the outer face are considered later, but keep it in mind
#if 0
		edge helper = newEdge(m_pathNode[vMin], vCenter);
		m_length[helper] = 0;
		m_cost[helper] = 0;
		m_type[helper] = ConstraintEdgeType::ReducibleArc;
#endif

		edge e1 = newEdge(vCenter,upper);
		m_length[e1] = 0;
		m_cost[e1]   = m_MedianArcCost;
		m_type[e1]   = ConstraintEdgeType::MedianArc;

		edge e2 = newEdge(vCenter,lower);
		m_length[e2] = 0;
		m_cost[e2]   = m_MedianArcCost;
		m_type[e2]   = ConstraintEdgeType::MedianArc;
	}
}


// set cost of edges on the cage boundary to 0
template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::setBoundaryCosts(
	adjEntry cornerDir,
	adjEntry cornerOppDir)
{
#if 0
	adjEntry adj;
	for (adj = cornerDir; m_pOR->direction(adj) == m_arcDir; adj = adj->faceCycleSucc())
		m_cost[m_edgeToBasicArc[adj]] = 0;
	for (adj = cornerOppDir; m_pOR->direction(adj) == m_oppArcDir; adj = adj->faceCycleSucc())
		m_cost[m_edgeToBasicArc[adj]] = 0;
#endif
	//test for multi separation
	adjEntry adj;
	for (adj = cornerDir; m_pOR->direction(adj) == m_arcDir; adj = adj->faceCycleSucc())
	{
		m_cost[m_edgeToBasicArc[adj]] = 0;

		if (m_pathNode[adj->twin()->cyclicSucc()->theNode()] &&
			(m_pOR->direction(adj->faceCycleSucc()) == m_arcDir)
			)
			m_originalEdge[m_pathNode[adj->twin()->cyclicSucc()->theNode()]] =
				m_pPR->original(adj->twin()->cyclicSucc()->theEdge());

	}
	for (adj = cornerOppDir; m_pOR->direction(adj) == m_oppArcDir; adj = adj->faceCycleSucc())
	{
		m_cost[m_edgeToBasicArc[adj]] = 0;

		if (m_pathNode[adj->twin()->cyclicSucc()->theNode()])
			m_originalEdge[m_pathNode[adj->twin()->cyclicSucc()->theNode()]] =
				m_pPR->original(adj->twin()->cyclicSucc()->theEdge());
	}
}


//
// insert arcs required for respecting vertex sizes, sizes of routing channels
// and position of attached generalizations
// vertices are represented by variable cages
template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::insertVertexSizeArcs(
	const PlanRep &PG,
	const NodeArray<ATYPE> &sizeOrig,
	const RoutingChannel<ATYPE> &rc)
{

	// segments in constraint graph are sides sMin and sMax; other two side
	// are sides in which adjacency entries run in direction m_arcDir and
	// m_oppArcDir
	OrthoDir dirMin = OrthoRep::prevDir(m_arcDir);
	OrthoDir dirMax = OrthoRep::nextDir(m_arcDir);

	const ATYPE overhang = rc.overhang();

	for(node v : PG.nodes)
	{
		if (PG.expandAdj(v) == nullptr) continue;

		if(PG.typeOf(v) == Graph::NodeType::generalizationMerger)
		{
			resetGenMergerLengths(PG,PG.expandAdj(v));

		}
		else // high/low-degree expander
		{
			ATYPE size = sizeOrig[v];

			const OrthoRep::VertexInfoUML &vi = *m_pOR->cageInfo(v);

			// determine routing channels rcMin and rcMax
			ATYPE rcMin = overhang + rc(v,dirMin);
			ATYPE rcMax = overhang + rc(v,dirMax);

			adjEntry cornerDir    = vi.m_corner[static_cast<int>(m_arcDir)];
			adjEntry cornerOppDir = vi.m_corner[static_cast<int>(m_oppArcDir)];
			adjEntry cornerMin    = vi.m_corner[static_cast<int>(dirMin)];
			adjEntry cornerMax    = vi.m_corner[static_cast<int>(dirMax)];

			// set cost of edges on the cage boundary to 0
			setBoundaryCosts(cornerDir,cornerOppDir);

			const OrthoRep::SideInfoUML &sDir = vi.m_side[static_cast<int>(m_arcDir)];
			const OrthoRep::SideInfoUML &sOppDir = vi.m_side[static_cast<int>(m_oppArcDir)];

			// correct lengths of edges within cage adjacent to corners
			if(sDir.totalAttached() > 0) {
				m_length[m_edgeToBasicArc[cornerDir]] = rcMin;
				m_length[m_edgeToBasicArc[cornerMax->faceCyclePred()]] = rcMax;
			} else {
				// if no edges are attached at this side we need no constraint
				m_length[m_edgeToBasicArc[cornerDir]] = 0;
				delEdge(m_edgeToBasicArc[cornerDir]);
			}

			if(sOppDir.totalAttached() > 0) {
				m_length[m_edgeToBasicArc[cornerOppDir]] = rcMax;
				m_length[m_edgeToBasicArc[cornerMin->faceCyclePred()]] = rcMin;
			} else {
				// if no edges are attached at this side we need no constraint
				m_length[m_edgeToBasicArc[cornerOppDir]] = 0;
				delEdge(m_edgeToBasicArc[cornerOppDir]);
			}


			// insert arcs for respecting vertex size / position of generalizations
			node vMin = m_pathNode[cornerDir->theNode()];
			node vMax = m_pathNode[cornerOppDir->theNode()];

			// any attached generalizations ?
			if (sDir.m_adjGen == nullptr && sOppDir.m_adjGen == nullptr)
			{
				// no, only one arc for vertex size + routing channels
				edge e = newEdge(vMin,vMax);
				m_length[e] = rcMin + size + rcMax - 2*overhang;
				m_cost  [e] = 2*m_vertexArcCost;
				m_type  [e] = ConstraintEdgeType::VertexSizeArc;

			} else {
				// yes, then two arcs for each side with an attached generalization
				ATYPE minHalf = size/2;
				ATYPE maxHalf = size - minHalf;
				ATYPE lenMin = rcMin + minHalf - overhang;
				ATYPE lenMax = maxHalf + rcMax - overhang;

				if (sDir.m_adjGen != nullptr) {
					node vCenter = m_pathNode[sDir.m_adjGen->theNode()];
					edge e1 = newEdge(vMin,vCenter);
					m_length[e1] = lenMin;
					m_cost  [e1] = m_vertexArcCost;
					m_type  [e1] = ConstraintEdgeType::VertexSizeArc;
					edge e2 = newEdge(vCenter,vMax);
					m_length[e2] = lenMax;
					m_cost  [e2] = m_vertexArcCost;
					m_type  [e2] = ConstraintEdgeType::VertexSizeArc;
				}

				if (sOppDir.m_adjGen != nullptr) {
					node vCenter = m_pathNode[sOppDir.m_adjGen->theNode()];
					edge e1 = newEdge(vMin,vCenter);
					m_length[e1] = lenMin;
					m_cost  [e1] = m_vertexArcCost;
					m_type  [e1] = ConstraintEdgeType::VertexSizeArc;
					edge e2 = newEdge(vCenter,vMax);
					m_length[e2] = lenMax;
					m_cost  [e2] = m_vertexArcCost;
					m_type  [e2] = ConstraintEdgeType::VertexSizeArc;
				}
			}
		}
	}
}

template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::setBasicArcsZeroLength(
	const PlanRep &PG)
{
	for(edge e : PG.edges)
	{
		edge arc = m_edgeToBasicArc[e];
		if (arc == nullptr) continue;

		node v = e->source();
		node w = e->target();
		if ( ((PG.typeOf(v) == Graph::NodeType::dummy) && (PG.typeOf(w) == Graph::NodeType::dummy) &&
			(v->degree() == 2) && w->degree() == 2) &&
			(m_pOR->angle(e->adjSource()) == m_pOR->angle(e->adjTarget()) ) && //no uturns
			(PG.typeOf(e) != Graph::EdgeType::generalization)
			)
		{
			m_length[arc] = 0;
			m_type[arc] = ConstraintEdgeType::FixToZeroArc;
			//we make fixtozero arcs as expensive as possible
			m_cost[arc] = m_doubleBendCost;
		}
	}
}



//
// insert arcs required for respecting vertex sizes
// and position of attached generalizations
// vertices are represented by tight cages
template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::insertVertexSizeArcs(
	const PlanRep &PG,
	const NodeArray<ATYPE> &sizeOrig,
	const MinimumEdgeDistances<ATYPE> &minDist)
{
	// set the length of all basic arcs corresponding to inner edge segments
	// to 0
	setBasicArcsZeroLength(PG);

	// segments in constraint graph are sides sMin and sMax; other two side
	// are sides in which adjacency entries run in direction m_arcDir and
	// m_oppArcDir
	OrthoDir dirMin = OrthoRep::prevDir(m_arcDir);
	OrthoDir dirMax = OrthoRep::nextDir(m_arcDir);

	for(node v : PG.nodes)
	{
		if (PG.expandAdj(v) == nullptr) continue;

		if(PG.typeOf(v) == Graph::NodeType::generalizationMerger)
		{
			resetGenMergerLengths(PG,PG.expandAdj(v));

		}
		else // high/low-degree expander
		{
			ATYPE size = sizeOrig[v];
			const OrthoRep::VertexInfoUML &vi = *m_pOR->cageInfo(v);

			adjEntry cornerDir    = vi.m_corner[static_cast<int>(m_arcDir)];
			adjEntry cornerOppDir = vi.m_corner[static_cast<int>(m_oppArcDir)];
			adjEntry cornerMin    = vi.m_corner[static_cast<int>(dirMin)];
			adjEntry cornerMax    = vi.m_corner[static_cast<int>(dirMax)];

			adjEntry adj = cornerDir, last = cornerMax->faceCyclePred();
			if(adj != last) {
				m_length[m_edgeToBasicArc[adj]]  = minDist.epsilon(v,m_arcDir,0);
				m_length[m_edgeToBasicArc[last]] = minDist.epsilon(v,m_arcDir,1);
				int i = 0;
				for(adj = adj->faceCycleSucc(); adj != last; adj = adj->faceCycleSucc()) {
					if (PG.typeOf(adj->cyclicPred()->theEdge()) == Graph::EdgeType::generalization)
						i++;
					m_length[m_edgeToBasicArc[adj]] = minDist.delta(v,m_arcDir,i);
				}
			}

			adj = cornerOppDir, last = cornerMin->faceCyclePred();
			if(adj != last) {
				m_length[m_edgeToBasicArc[adj]]  = minDist.epsilon(v,m_oppArcDir,0);
				m_length[m_edgeToBasicArc[last]] = minDist.epsilon(v,m_oppArcDir,1);
				int i = 0;
				for(adj = adj->faceCycleSucc(); adj != last; adj = adj->faceCycleSucc()) {
					if (PG.typeOf(adj->cyclicPred()->theEdge()) == Graph::EdgeType::generalization)
						i++;
					m_length[m_edgeToBasicArc[adj]] = minDist.delta(v,m_oppArcDir,i);
				}
			}


			// insert arcs for respecting vertex size / position of generalizations
			node vMin = m_pathNode[cornerDir->theNode()];
			node vMax = m_pathNode[cornerOppDir->theNode()];

			const OrthoRep::SideInfoUML &sDir = vi.m_side[static_cast<int>(m_arcDir)];
			const OrthoRep::SideInfoUML &sOppDir = vi.m_side[static_cast<int>(m_oppArcDir)];

			// any attached generalizations ?
			if (sDir.m_adjGen == nullptr && sOppDir.m_adjGen == nullptr)
			{
				//check for single edge case => special treatment
				//generic case could handle all numbers

				if (sDir.totalAttached() == 1 || sOppDir.totalAttached() == 1)
				{
					//first, insert a new center node and connect it
					ATYPE lenMin = size/2;
					ATYPE lenMax = size - lenMin;
					node vCenter = newNode();
					setExtra(vCenter, cornerDir->theNode(), lenMin);

					edge e1 = newEdge(vMin,vCenter);
					m_length[e1] = lenMin;
					m_cost[e1]   = m_vertexArcCost;
					m_type[e1]   = ConstraintEdgeType::VertexSizeArc;
					edge e2 = newEdge(vCenter,vMax);
					m_length[e2] = lenMax;
					m_cost[e2]   = m_vertexArcCost;
					m_type[e2]   = ConstraintEdgeType::VertexSizeArc;

					if (sDir.totalAttached() == 1)
					{

						//then, insert a moveable node connecting center
						//and outgoing segment
						node vBungee = newNode();
						//+1 should fix the fixzerolength problem
						setExtra(vBungee, cornerDir->theNode(), minDist.epsilon(v,m_arcDir,0) );

						edge eToBungee = newEdge(vMin, vBungee);
						m_type[eToBungee] = ConstraintEdgeType::MedianArc; //BasicArc; // XXX: is this ok?
						m_cost[eToBungee] = 0; // XXX: what about this?
						m_length[eToBungee] = minDist.epsilon(v,m_arcDir,0);

						edge eBungeeCenter = newEdge(vBungee, vCenter);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeCenter] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeCenter] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeCenter] = 0;

						//attention: pathnode construct works only if degree 1
						edge eBungeeOut =  newEdge(vBungee, m_pathNode[cornerDir->twinNode()]);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeOut] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeOut] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeOut] = 0;
#if 0
						//connect outgoing segment and "right" segment, maybe obsolete
						edge eFromOut = newEdge(m_pathNode[cornerDir->twinNode()], vMax);
						m_type[eFromOut] = ConstraintEdgeType::BasicArc; // XXX
						m_cost[eFromOut] = 0; // XXX
						m_length[eFromOut] = minDist.epsilon(v,m_arcDir,1);
#endif
					}
					if ( sOppDir.totalAttached() == 1 && m_pathNode[cornerOppDir->twinNode()] != vMin )
					{

						//then, insert a moveable node connecting center
						//and outgoing segment
						node vBungee = newNode();
						//+1 for not fixzerolength
						setExtra(vBungee, cornerDir->theNode(), minDist.epsilon(v,m_oppArcDir,0) );

						edge eToBungee = newEdge(vMin, vBungee);
						m_type[eToBungee] = ConstraintEdgeType::MedianArc;//BasicArc; // XXX: is this ok?
						m_cost[eToBungee] = 0; // XXX: what about this?
						m_length[eToBungee] = minDist.epsilon(v,m_oppArcDir,0);

						edge eBungeeCenter = newEdge(vBungee, vCenter);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeCenter] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeCenter] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeCenter] = 0;

						//attention: pathnode construct works only if degree 1, sometimes not at all
						edge eBungeeOut =  newEdge(vBungee, m_pathNode[cornerOppDir->twinNode()]);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeOut] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeOut] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeOut] = 0;
#if 0
						//connect outgoing segment and "right" segment, maybe obsolete
						edge eFromOut = newEdge(m_pathNode[cornerOppDir->twinNode()], vMax);
						m_type[eFromOut] = ConstraintEdgeType::BasicArc; // XXX
						m_cost[eFromOut] = 0; // XXX
						m_length[eFromOut] = minDist.epsilon(v,m_oppArcDir,0);
#endif
					}
				} else {
					// no, only one arc for vertex size + routing channels
					edge e = newEdge(vMin,vMax);
					m_length[e] = size;
					m_cost[e]   = 2*m_vertexArcCost;
					m_type[e]   = ConstraintEdgeType::VertexSizeArc;
				}
			} else {
				// yes, then two arcs for each side with an attached generalization
				ATYPE lenMin = size/2;
				ATYPE lenMax = size - lenMin;

#if 0
				ATYPE len = size/2;
#endif

				if (sDir.m_adjGen != nullptr) {
					node vCenter = m_pathNode[sDir.m_adjGen->theNode()];
					edge e1 = newEdge(vMin,vCenter);
					m_length[e1] = lenMin;
					m_cost  [e1] = m_vertexArcCost;
					m_type  [e1] = ConstraintEdgeType::VertexSizeArc;
					edge e2 = newEdge(vCenter,vMax);
					m_length[e2] = lenMax;
					m_cost  [e2] = m_vertexArcCost;
					m_type  [e2] = ConstraintEdgeType::VertexSizeArc;
				}
				else
				{
					if (sDir.totalAttached() == 1)
					{
						node vCenter = newNode();//m_pathNode[sOppDir.m_adjGen->theNode()];  //newNode();
						setExtra(vCenter, cornerDir->theNode(), lenMin);

						edge e1 = newEdge(vMin,vCenter);
						m_length[e1] = lenMin;
						m_cost[e1]   = m_vertexArcCost;
						m_type[e1]   = ConstraintEdgeType::VertexSizeArc;
						edge e2 = newEdge(vCenter,vMax);
						m_length[e2] = lenMax;
						m_cost[e2]   = m_vertexArcCost;
						m_type[e2]   = ConstraintEdgeType::VertexSizeArc;

						//then, insert a moveable node connecting center
						//and outgoing segment
						node vBungee = newNode();
						//+1 for not fixzerolength
						setExtra(vBungee, cornerDir->theNode(), minDist.epsilon(v,m_arcDir,0) );

						edge eToBungee = newEdge(vMin, vBungee);
						m_type[eToBungee] = ConstraintEdgeType::MedianArc;//BasicArc; // XXX: is this ok?
						m_cost[eToBungee] = 0; // XXX: what about this?
						m_length[eToBungee] = minDist.epsilon(v,m_arcDir,0);

						edge eBungeeCenter = newEdge(vBungee, vCenter);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeCenter] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeCenter] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeCenter] = 0;

						//attention: pathnode construct works only if degree 1
						edge eBungeeOut =  newEdge(vBungee, m_pathNode[cornerDir->twinNode()]);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeOut] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeOut] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeOut] = 0;

					}
				}
				if (sOppDir.m_adjGen != nullptr) {
					node vCenter = m_pathNode[sOppDir.m_adjGen->theNode()];
					edge e1 = newEdge(vMin,vCenter);
					m_length[e1] = lenMin;
					m_cost  [e1] = m_vertexArcCost;
					m_type  [e1] = ConstraintEdgeType::VertexSizeArc;
					edge e2 = newEdge(vCenter,vMax);
					m_length[e2] = lenMax;
					m_cost  [e2] = m_vertexArcCost;
					m_type  [e2] = ConstraintEdgeType::VertexSizeArc;
				}
				else
				{
					//special case single edge to middle position
					if ( sOppDir.totalAttached() == 1 && m_pathNode[cornerOppDir->twinNode()] != vMin )
					{
						node vCenter = newNode();//m_pathNode[sDir.m_adjGen->theNode()];//newNode();
						setExtra(vCenter, cornerDir->theNode(), lenMin);

						edge e1 = newEdge(vMin,vCenter);
						m_length[e1] = lenMin;
						m_cost[e1]   = m_vertexArcCost;
						m_type[e1]   = ConstraintEdgeType::VertexSizeArc;
						edge e2 = newEdge(vCenter,vMax);
						m_length[e2] = lenMax;
						m_cost[e2]   = m_vertexArcCost;
						m_type[e2]   = ConstraintEdgeType::VertexSizeArc;
						//then, insert a moveable node connecting center
						//and outgoing segment
						node vBungee = newNode();
						//+1 for not fixzerolength
						setExtra(vBungee, cornerDir->theNode(), minDist.epsilon(v,m_oppArcDir,0) );

						edge eToBungee = newEdge(vMin, vBungee);
						m_type[eToBungee] = ConstraintEdgeType::MedianArc;//BasicArc; // XXX: is this ok?
						m_cost[eToBungee] = 0; // XXX: what about this?
						m_length[eToBungee] = minDist.epsilon(v,m_oppArcDir,0);

						edge eBungeeCenter = newEdge(vBungee, vCenter);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeCenter] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeCenter] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeCenter] = 0;

						//attention: pathnode construct works only if degree 1
						edge eBungeeOut =  newEdge(vBungee, m_pathNode[cornerOppDir->twinNode()]);
						//bungee, center and outgoing segment may build column if in the middle
						m_type[eBungeeOut] = ConstraintEdgeType::MedianArc;
						m_cost[eBungeeOut] = m_bungeeCost; // XXX: what about this?
						m_length[eBungeeOut] = 0;
					}
				}
			}

			// set cost of edges on the cage boundary to 0
			setBoundaryCosts(cornerDir,cornerOppDir);
		}
	}
#if 0
	if (m_arcDir == OrthoDir::East) writeGML("eastvertexsizeinserted.gml");
	else writeGML("northvertexsizeinserted.gml");
#endif
}


// computes the total costs for coordintes given by pos, i.e.,
// the sum of the weighted lengths of edges in the constraint graph.
template<class ATYPE>
ATYPE CompactionConstraintGraph<ATYPE>::computeTotalCosts(
	const NodeArray<ATYPE> &pos) const
{
	ATYPE c = 0;
	for(edge e : edges) {
		c += cost(e) * (pos[e->target()] - pos[e->source()]);
	}

	return c;
}


//
// insertion of visibility arcs

// checks if intervals on the sweep line are in correct order
template<class ATYPE>
bool CompactionConstraintGraph<ATYPE>::checkSweepLine(const List<Interval> &sweepLine) const
{
	if (sweepLine.empty())
		return true;

	ListConstIterator<Interval> it = sweepLine.begin();

	if((*it).m_high < (*it).m_low)
		return false;

	ATYPE x = (*it).m_low;

	for(++it; it.valid(); ++it) {
		if((*it).m_high < (*it).m_low)
			return false;
		if ((*it).m_high > x)
			return false;
		x = (*it).m_low;
	}

	return true;
}


template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::insertVisibilityArcs(
	const PlanRep &PG,
	const NodeArray<ATYPE> &posDir,
	const NodeArray<ATYPE> &posOrthDir
	)
{
	MinimumEdgeDistances<ATYPE> minDist(PG, m_sep);

	for(node v : PG.nodes)
	{
		if(PG.expandAdj(v) == nullptr) continue;

		for(int d = 0; d < 4; ++d) {
			minDist.delta(v,OrthoDir(d),0) = m_sep;//currentSeparation;
			minDist.delta(v,OrthoDir(d),1) = m_sep;//currentSeparation;
		}
	}

	insertVisibilityArcs(PG,posDir,posOrthDir,minDist);
}



// inserts arcs connecting segments which can see each other in a drawing
// of the associated planarized representation PG which is given by
// posDir and posOppDir.

//ersetze mindist.delta durch min(m_sep, mindist.delta) fuer skalierung
template<class ATYPE>
void CompactionConstraintGraph<ATYPE>::insertVisibilityArcs(
	const PlanRep &PG,
	const NodeArray<ATYPE> &posDir,
	const NodeArray<ATYPE> &posOrthDir,
	const MinimumEdgeDistances<ATYPE> &minDist)
{
	OrthoDir segDir    = OrthoRep::prevDir(m_arcDir);
	OrthoDir segOppDir = OrthoRep::nextDir(m_arcDir);

	// list of visibility arcs which have to be inserted
	SListPure<Tuple2<node,node> > visibArcs;

	// lower and upper bound of segments
	NodeArray<ATYPE> low(getGraph()), lowReal(getGraph()), high(getGraph());
	NodeArray<ATYPE> segPos(getGraph(), 0); // position of segments
	NodeArray<int>   topNum(getGraph()/*,0*/); // secondary sorting criteria for segments

	// compute position and lower/upper bound of segments
	// We have to take care that segments cannot be shifted one upon the other,
	// e.g., if we have two segments (l1,h1) and (l2,h2) with l2 > h2 and
	// the distance l2-h2 is smaller than separation, the segments can see
	// each other. We do this by enlarging the lower bound of all segments
	// by separation if this lower bound is realized by a bend.
	//
	// Note: Be careful at segments attached at a vertex which are closer
	// than separation to each other. Possible solution: Remove visibility
	// arcs of segments which are connected by orthogonal segments to the
	// same vertex and bend in opposite directions.
	for(node v : nodes) {

		//special case nodes
		if (m_path[v].empty()) continue;

		SListConstIterator<node> it = m_path[v].begin();

		segPos[v] = posDir[*it];
		low[v] = high[v] = posOrthDir[*it];
		node nodeLow = *it;
		for(++it; it.valid(); ++it) {
			ATYPE x = posOrthDir[*it];
			if (x < low [v]) {
				low [v] = x;
				nodeLow = *it;
			}
			if (x > high[v]) high[v] = x;
		}
		lowReal[v] = low[v];
		Graph::NodeType typeLow = PG.typeOf(nodeLow);
		if(typeLow == Graph::NodeType::dummy || typeLow == Graph::NodeType::generalizationExpander) {
#if 0
			bool subtractSep = true;
			if (nodeLow->degree() == 2) {
				adjEntry adjFound = nullptr;
				for(adjEntry adj : nodeLow->adjEntries) {
					if(m_pOR->direction(adj) == m_arcDir || m_pOR->direction(adj) == m_oppArcDir) {
						adjFound = adj;
						break;
					}
				}
				if (adjFound) {
					for(adjEntry adj2 = adjFound->faceCycleSucc();
						m_pOR->direction(adj2) == m_pOR->direction(adjFound);
					adj2 = adj2->twin()->faceCycleSucc()) ;
					if(posDir[adjFound->theNode()] == posDir[adj2->twinNode()])
					subtractSep = false;
				}
			}
			if (subtractSep)
				low[v] -= m_sep;
#else
			low[v] -= m_sep;
#endif
		}
	}

	// correct "-= m_sep" ...
	OrthoDir dirMin = OrthoRep::prevDir(m_arcDir);
	OrthoDir dirMax = OrthoRep::nextDir(m_arcDir);
	bool isCaseA = (m_arcDir == OrthoDir::East || m_arcDir ==  OrthoDir::South);
	const int angleAtMin = (m_arcDir ==  OrthoDir::East || m_arcDir ==  OrthoDir::South) ? 3 : 1;
	const int angleAtMax = (m_arcDir ==  OrthoDir::East || m_arcDir ==  OrthoDir::South) ? 1 : 3;

	for(node v : PG.nodes)
	{
		if(PG.expandAdj(v) == nullptr) continue;
		const OrthoRep::VertexInfoUML &vi = *m_pOR->cageInfo(v);

		int i = 0;
		adjEntry adj;

		for (adj = (isCaseA) ? vi.m_corner[static_cast<int>(dirMin)]->faceCycleSucc()->faceCycleSucc() : vi.m_corner[static_cast<int>(dirMin)]->faceCycleSucc();
			m_pOR->direction((isCaseA) ? adj : adj->faceCycleSucc()) == dirMin; //m_pOR->direction(adj) == dirMin;
			adj = adj->faceCycleSucc())
		{
			adjEntry adjCross = adj->cyclicPred();
			adjEntry adjTwin = adjCross->twin();

			adjEntry adjPred = adj->faceCyclePred();
			ATYPE delta = (isCaseA) ?
				min(abs(posOrthDir[adjPred->theNode()] - posOrthDir[adjPred->twinNode()]), m_sep) :
				min(abs(posOrthDir[adj->theNode()] - posOrthDir[adj->twinNode()]), m_sep);
			ATYPE boundary = (isCaseA) ?
				min(posOrthDir[adjPred->theNode()], posOrthDir[adjPred->twinNode()]) :
				min(posOrthDir[adj->theNode()],     posOrthDir[adj->twinNode()]);

			if (PG.typeOf(adjCross->theEdge()) == Graph::EdgeType::generalization)
			{
				if (isCaseA) {
					if(PG.typeOf(adjTwin->theNode()) == Graph::NodeType::generalizationExpander &&
						m_pOR->angle(adjTwin) == 2)
					{
						node s1 = m_pathNode[adjTwin->theNode()];
						node s2 = m_pathNode[adjTwin->cyclicSucc()->twinNode()];
						low[s1] = lowReal[s1] - delta; // minDist.delta(v,dirMin,i);
						low[s2] = lowReal[s2] - delta; //minDist.delta(v,dirMin,i);
					}
					++i;
				} else {
					++i;
					if(PG.typeOf(adjTwin->theNode()) == Graph::NodeType::generalizationExpander &&
						m_pOR->angle(adjTwin->cyclicPred()) == 2)
					{
						node s1 = m_pathNode[adjTwin->theNode()];
						node s2 = m_pathNode[adjTwin->cyclicPred()->twinNode()];
						low[s1] = lowReal[s1] - delta; //minDist.delta(v,dirMin,i);
						low[s2] = lowReal[s2] - delta; //minDist.delta(v,dirMin,i);
					}
				}
				continue;
			}

			//we save the current direction and stop if we run in opposite
			OrthoDir runDir = m_pOR->direction(adjCross);
			// if -> while
			while (PG.typeOf(adjTwin->theNode()) == Graph::NodeType::dummy &&
				adjTwin->theNode()->degree() == 2 &&
				m_pOR->angle(adjTwin) == angleAtMin)
			{
				// We handle the case if an edge segment adjacent to a vertex
				// is separated by less than separation from edge segments above.
				node s = m_edgeToBasicArc[adjCross]->source();
				if(lowReal[s] != low[s])
				{
					if(low[s] >= boundary) // nothing to do?
						break;
					low[s] = boundary;
#if 0
					low[s] += m_sep - delta; //minDist.delta(v,dirMin,i);
#endif

					// If the compaction has eliminated bends, we can have the situation
					// that segment s has length 0 and the next segment s' (following the
					// edge) is at the same position (the edge arc has length 0).
					// In this case, the low-value of s' must be lowered (low[s'] := lowReal[s']
					// is approproate). The same situation can appear several times in a
					// row.
					//collect chains of segments compacted to zero length
					for( ; ; ) { //while(true/*lowReal[s] == high[s]*/) {
						do {
							adjCross = adjCross->faceCycleSucc();
						} while(m_pOR->direction(adjCross) == segDir ||
							m_pOR->direction(adjCross) == segOppDir);

						if(adjCross->theNode()->degree() != 2) // no longer a bend point?
							break;

						node sNext = m_edgeToBasicArc[adjCross]->opposite(s);

						if(segPos[sNext] != segPos[s])
							break;

						low[sNext] = lowReal[sNext];  //?
						s = sNext;
					}
				}

				adjTwin = adjCross->twin(); // update of twin for while
				//check if we have to stop
				if (runDir != m_pOR->direction(adjCross)) break;
			}
		}

		i = 0;
		for (adj = (isCaseA) ? vi.m_corner[static_cast<int>(dirMax)]->faceCycleSucc() : vi.m_corner[static_cast<int>(dirMax)]->faceCycleSucc()->faceCycleSucc();
			m_pOR->direction((isCaseA) ? adj->faceCycleSucc() : adj) == dirMax; // m_pOR->direction(adj) == dirMax;
			adj = adj->faceCycleSucc())
		{
			adjEntry adjCross = adj->cyclicPred();
			adjEntry adjTwin = adjCross->twin();

#if 0
			ATYPE delta = -posOrthDir[adj->twinNode()] + posOrthDir[adj->theNode()];
#endif
			adjEntry adjPred = adj->faceCyclePred();
			ATYPE delta = (isCaseA) ?
				min(abs(posOrthDir[adj->twinNode()] - posOrthDir[adj->theNode()]), m_sep) :
				min(abs(posOrthDir[adjPred->theNode()] - posOrthDir[adjPred->twinNode()]), m_sep);
			ATYPE boundary = (isCaseA) ?
				min(posOrthDir[adj->twinNode()], posOrthDir[adj->theNode()]) :
				min(posOrthDir[adjPred->theNode()],     posOrthDir[adjPred->twinNode()]);

			if (PG.typeOf(adjCross->theEdge()) == Graph::EdgeType::generalization)
			{
				if (isCaseA) {
					++i;
					if(PG.typeOf(adjTwin->theNode()) == Graph::NodeType::generalizationExpander &&
						m_pOR->angle(adjTwin->cyclicPred()) == 2)
					{
						node s1 = m_pathNode[adjTwin->theNode()];
						node s2 = m_pathNode[adjTwin->cyclicPred()->twinNode()];
						low[s1] = lowReal[s1] - delta; //minDist.delta(v,dirMax,i);
						low[s2] = lowReal[s2] - delta; //minDist.delta(v,dirMax,i);
					}
				} else {
					if(PG.typeOf(adjTwin->theNode()) == Graph::NodeType::generalizationExpander &&
						m_pOR->angle(adjTwin) == 2)
					{
						node s1 = m_pathNode[adjTwin->theNode()];
						node s2 = m_pathNode[adjTwin->cyclicSucc()->twinNode()];
						low[s1] = lowReal[s1] - delta; //minDist.delta(v,dirMax,i);
						low[s2] = lowReal[s2] - delta; //minDist.delta(v,dirMax,i);
					}
					++i;
				}
				continue;
			}


			//we save the current direction and stop if we run in opposite
			OrthoDir runDir = m_pOR->direction(adjCross);
			// if -> while
			while (PG.typeOf(adjTwin->theNode()) == Graph::NodeType::dummy &&
				adjTwin->theNode()->degree() == 2 &&
				m_pOR->angle(adjTwin) == angleAtMax)
			{
				node s = m_edgeToBasicArc[adjCross]->target();
				if(lowReal[s] != low[s])
				{
					if(low[s] >= boundary) // nothing to do?
						break;
					low[s] = boundary;
#if 0
					low[s] += m_sep - delta; //minDist.delta(v,dirMax,i);
#endif

					// If the compaction has eliminated bends, we can have the situation
					// that segment s has length 0 and the next segment s' (following the
					// edge) is at the same position (the edge arc has length 0).
					// In this case, the low-value of s' must be lowered (low[s'] := lowReal[s']
					// is approproate). The same situation can appear several times in a
					// row.
					//collect chains of segments compacted to zero length
					for( ; ; ) /*lowReal[s] == high[s]*/
					{
						do
						{
							adjCross = adjCross->faceCycleSucc();
						} while(m_pOR->direction(adjCross) == segDir ||
								m_pOR->direction(adjCross) == segOppDir);

						if(adjCross->theNode()->degree() != 2) // no longer a bend point?
							break;

						node sNext = m_edgeToBasicArc[adjCross]->opposite(s);

						if(segPos[sNext] != segPos[s])
							break;

						low[sNext] = lowReal[sNext];//was: low[s]
						s = sNext;
					}
				}

				adjTwin = adjCross->twin(); // update of twin for while

				//check if we have to stop
				if (runDir != m_pOR->direction(adjCross)) break;
			}
		}
	}

	// compute topological numbering of segments as second sorting criteria
	// in order to process overlapping segments in the order imposed by the
	// embedding
	computeTopologicalSegmentNum(topNum);


	// sort segments
	SegmentComparer cmpBySegPos(segPos,topNum);
	List<node> sortedPathNodes;
	allNodes(sortedPathNodes);
	sortedPathNodes.quicksort(cmpBySegPos);

	// add segments in the order given by sortedPathNodes to sweep line
	List<Interval> sweepLine;

	ListIterator<node> itV;
	for(itV = sortedPathNodes.begin(); itV.valid(); ++itV)
	{
		//special case nodes
		if (m_path[*itV].empty()) continue;
		OGDF_HEAVY_ASSERT(checkSweepLine(sweepLine));

		node v = *itV;
		ListIterator<Interval> it;
		for(it = sweepLine.begin(); it.valid(); ++it) {
			if ((*it).m_low < high[v])
				break;
		}

		if (!it.valid()) {
			sweepLine.pushBack(Interval(v,low[v],high[v]));
			continue;
		}

		if((*it).m_high <= low[v]) {
			sweepLine.insertBefore(Interval(v,low[v],high[v]),it);
			continue;
		}

		ListIterator<Interval> itUp = it, itSucc;
		// we store if itUp will be deleted in order not to
		// access the deleted iterator later
		bool isItUpDel = ( ((*itUp).m_low >= low[v]) && ((*itUp).m_high <= high[v]) );

		for(; it.valid() && (*it).m_low >= low[v]; it = itSucc) {
			itSucc = it.succ();
			if ((*it).m_high <= high[v]) {
				visibArcs.pushBack(Tuple2<node,node>((*it).m_pathNode,v));
				sweepLine.del(it);
			}
		}

		if (it == itUp && (*it).m_high > high[v]) {
			node w = (*it).m_pathNode;
			sweepLine.insertAfter(Interval(w,(*it).m_low,low[v]),it);
			(*it).m_low = high[v];
			sweepLine.insertAfter(Interval(v,low[v],high[v]),it);
			visibArcs.pushBack(Tuple2<node,node>(w,v));

		} else {
			if ( (!isItUpDel) && itUp != it && (*itUp).m_low < high[v]) {
				(*itUp).m_low = high[v];
				visibArcs.pushBack(Tuple2<node,node>((*itUp).m_pathNode,v));
			}
			if (it.valid()) {
				if ((*it).m_high > low[v]) {
					(*it).m_high = low[v];
					visibArcs.pushBack(Tuple2<node,node>((*it).m_pathNode,v));
				}
				sweepLine.insertBefore(Interval(v,low[v],high[v]),it);

			} else {
				sweepLine.pushBack(Interval(v,low[v],high[v]));
			}
		}

	}

	// remove all arcs from visibArcs that are already in the constraint graph
	removeRedundantVisibArcs(visibArcs);

	// compute original adjacency entry corresponding to a segment
	// We use this in order to omit visibility arcs between segments which
	// belong to the same edge if they can see each other from the same side
	// of the edge; if they see each other from different sides the arc is
	// essential!
	NodeArray<adjEntry> correspEdge(getGraph(),nullptr);

	for(node v : PG.nodes) {
		node seg = m_pathNode[v];
		for(adjEntry adj : v->adjEntries) {
			if(m_pOR->direction(adj) != segDir) continue;
			edge eAdj = adj->theEdge();
			edge eOrig = PG.original(eAdj);
			if (eOrig == nullptr) continue;
			if (adj == eAdj->adjSource())
				correspEdge[seg] = eOrig->adjSource();
			else
				correspEdge[seg] = eOrig->adjTarget();
		}
	}

	// remove visibility arcs between ...
	SListIterator<Tuple2<node,node> > itT, itTSucc, itTPred;
	for(itT = visibArcs.begin(); itT.valid(); itT = itTSucc) {
		itTSucc = itT.succ();
		node v = (*itT).x1(), w = (*itT).x2();

		// remove arcs which connect segments belonging to the same edge
		if (correspEdge[v] && (correspEdge[v] == correspEdge[w]))
		{
			if (itTPred.valid())
				visibArcs.delSucc(itTPred);
			else
				visibArcs.popFront();
		}

		else
			itTPred = itT;
	}



	for(itT = visibArcs.begin(); itT.valid(); ++itT) {
		//CHECK if
		node v = (*itT).x1(), w = (*itT).x2();
		if (!(m_extraNode[v] || m_extraNode[w])) {
			//CHECK if
			node boundRepresentant1 = m_path[v].front();
			node boundRepresentant2 = m_path[w].front();
			node en1 = m_pPR->expandedNode(boundRepresentant1);
			node en2 = m_pPR->expandedNode(boundRepresentant2);
			//do not insert visibility in cages
			if (!( ( en1 && en2 ) && ( en1 == en2) ))
			{
				edge e = newEdge(v,w);

				//hier vielleicht multiedges abfangen: length auf max(min(sep, dists), minDist.sep)

				m_length[e] = max(m_sep, minDist.separation()); //m_sep;
				m_cost  [e] = 0;
				m_type  [e] = ConstraintEdgeType::VisibilityArc;
#if 0
				writeGML("visibilityinserted.gml");
#endif
			}
		}
	}

	OGDF_HEAVY_ASSERT(checkSweepLine(sweepLine));
}

// performs feasibility test for position assignment pos, i.e., checks if
// the segment positions given by pos fulfill the constraints in the
// compaction constraint graph
template<class ATYPE>
bool CompactionConstraintGraph<ATYPE>::isFeasible(
	const NodeArray<ATYPE> &pos)
{
	for(edge e : getGraph().edges) {
		node v = m_path[e->source()].front();
		node w = m_path[e->target()].front();
		if (pos[w] - pos[v] < length(e)) {
			std::cout << "feasibility check failed for edge " << e << std::endl;
			std::cout << "  representatives: " << v << ", " << w << std::endl;
			std::cout << "  length: " << length(e) << std::endl;
			std::cout << "  actual distance: " << pos[w] - pos[v] << std::endl;
			std::cout << "  type of " << e << ": ";
			switch(m_type[e]) {
			case ConstraintEdgeType::BasicArc: std::cout << "basic arc" << std::endl;
				break;
			case ConstraintEdgeType::VertexSizeArc: std::cout << "vertex-size arc" << std::endl;
				break;
			case ConstraintEdgeType::VisibilityArc: std::cout << "visibility arc" << std::endl;
				break;
			case ConstraintEdgeType::MedianArc: std::cout << "median arc" << std::endl;
				break;
			case ConstraintEdgeType::ReducibleArc: std::cout << "reducible arc" <<std::endl;
				break;
			case ConstraintEdgeType::FixToZeroArc: std::cout << "fixtozero arc" <<std::endl;

			}
			return false;
		}
	}

	return true;
}

}
