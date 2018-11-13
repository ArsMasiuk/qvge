/** \file
 * \brief Implementation of ogdf::CommonCompactionConstraintGraphBase
 *
 * \author Stephan Beyer, Carsten Gutwenger, Karsten Klein
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

#include <ogdf/orthogonal/internal/CommonCompactionConstraintGraphBase.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {

CommonCompactionConstraintGraphBase::CommonCompactionConstraintGraphBase(const OrthoRep &OR, const PlanRep &PG, OrthoDir arcDir, int costAssoc)
  : m_pOR(&OR)
  , m_pPR(&PG) // only used in detecting cage visibility arcs
  , m_path(*this)
  , m_pathNode(OR)
  , m_edgeToBasicArc(OR, nullptr)
  , m_cost(*this, costAssoc)
  , m_type(*this, ConstraintEdgeType::BasicArc)
  , m_border(*this, false)
  , m_extraNode(*this, false)
  , m_arcDir(arcDir)
  , m_oppArcDir(OR.oppDir(arcDir))
  , m_originalEdge(*this, nullptr)
{
	OGDF_ASSERT(&PG == &(const Graph &)OR);
}

// embeds constraint graph such that all sources and sinks lie in a common
// face
void CommonCompactionConstraintGraphBase::embed() {
	NodeArray<bool> onExternal(*this, false);
	const CombinatorialEmbedding &E = *m_pOR;
	face fExternal = E.externalFace();

	for (adjEntry adj : fExternal->entries) {
		onExternal[m_pathNode[adj->theNode()]] = true;
	}

	// compute lists of sources and sinks
	SList<node> sources, sinks;

	for (node v : nodes) {
		if (onExternal[v]) {
			if (v->indeg() == 0) {
				sources.pushBack(v);
			}
			if (v->outdeg() == 0) {
				sinks.pushBack(v);
			}
		}
	}

	// determine super source and super sink
	node s;
	if (sources.size() > 1) {
		s = newNode();
		for (node v : sources) {
			newEdge(s, v);
		}
	} else {
		s = sources.front();
	}

	node t;
	if (sinks.size() > 1) {
		t = newNode();
		for (node v : sinks) {
			newEdge(v, t);
		}
	} else {
		t = sinks.front();
	}

	edge st = newEdge(s, t);

	bool isPlanar = planarEmbed(*this);
	if (!isPlanar) {
		OGDF_THROW(AlgorithmFailureException);
	}

	delEdge(st);
	if (sources.size() > 1) {
		delNode(s);
	}
	if (sinks.size() > 1) {
		delNode(t);
	}
}

// computes topological numbering on the segments of the constraint graph.
// Usage: If used on the basic (and vertex size) arcs, the numbering can be
//   used in order to serve as sorting criteria for respecting the given
//   embedding, e.g., when computing visibility arcs and allowing edges
//   with length 0.
void CommonCompactionConstraintGraphBase::computeTopologicalSegmentNum(NodeArray<int> &topNum) {
	NodeArray<int> indeg(*this);
	ArrayBuffer<node> sources;

	for (node v : nodes) {
		topNum[v] = 0;
		indeg[v] = v->indeg();
		if (indeg[v] == 0) {
			sources.push(v);
		}
	}

	while (!sources.empty()) {
		node v = sources.popRet();

		for (adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if (e->source() == v) {
				node w = e->target();

				if (topNum[w] < topNum[v] + 1) {
					topNum[w] = topNum[v] + 1;
				}

				if (--indeg[w] == 0) {
					sources.push(w);
				}
			}
		}
	}
}

// remove "arcs" from visibArcs which we already have in the constraint graph
// (as basic arcs)
void CommonCompactionConstraintGraphBase::removeRedundantVisibArcs(SListPure<Tuple2<node, node>> &visibArcs) {
	// bucket sort list of all edges
	SListPure<edge> all;
	allEdges(all);
	parallelFreeSort(*this, all);

	// bucket sort visibArcs
	struct : public BucketFunc<Tuple2<node, node>> {
		int getBucket(const Tuple2<node, node> &t) override {
			return t.x1()->index();
		}
	} bucketSrc;
	visibArcs.bucketSort(0, maxNodeIndex(), bucketSrc);

	struct : public BucketFunc<Tuple2<node, node>> {
		int getBucket(const Tuple2<node, node> &t) override {
			return t.x2()->index();
		}
	} bucketTgt;
	visibArcs.bucketSort(0, maxNodeIndex(), bucketTgt);

	// now, in both lists, arcs are sorted by increasing target index,
	// and arcs with the same target index by increasing source index.
	SListConstIterator<edge> itAll = all.begin();
	SListIterator<Tuple2<node, node>> it, itNext, itPrev;

	// for each arc in visibArcs, we check if it is also contained in list all
	for (it = visibArcs.begin(); it.valid(); it = itNext) {
		// required since we delete from the list we traverse
		itNext = it.succ();
		int i = (*it).x1()->index();
		int j = (*it).x2()->index();

		// skip all arcs with smaller target index
		while (itAll.valid() && (*itAll)->target()->index() < j) {
			++itAll;
		}

		// no more arcs => no more duplicates, so return
		if (!itAll.valid()) break;

		// if target index is j, we also skip all arcs with target index i
		// and source index smaller than i
		while (itAll.valid()
		    && (*itAll)->target()->index() == j
		    && (*itAll)->source()->index() < i) {
			++itAll;
		}

		// no more arcs => no more duplicates, so return
		if (!itAll.valid()) break;

		// if (i,j) is already present, we delete it from visibArcs
		if ((*itAll)->source()->index() == i
		 && (*itAll)->target()->index() == j) {
			// visibArcs.del(it);
			if (itPrev.valid()) {
				visibArcs.delSucc(itPrev);
			} else {
				visibArcs.popFront();
			}
		} else {
			itPrev = it;
		}
	}

	// CHECK for special treatment for cage visibility
	// two cases: input node cage: just compare arbitrary node
	//            merger cage: check first if there are mergers
	itPrev = nullptr;
	for (it = visibArcs.begin(); it.valid(); it = itNext) {
		itNext = it.succ();

		OGDF_ASSERT(!m_path[(*it).x1()].empty());
		OGDF_ASSERT(!m_path[(*it).x1()].empty());

		node boundRepresentant1 = m_path[(*it).x1()].front();
		node boundRepresentant2 = m_path[(*it).x2()].front();
		node en1 = m_pPR->expandedNode(boundRepresentant1);
		node en2 = m_pPR->expandedNode(boundRepresentant2);
		// do not allow visibility constraints in fixed cages
		// due to non-planarity with middle position constraints

		if (en1 != nullptr && en2 != nullptr && en1 == en2) {
			if (itPrev.valid()) {
				visibArcs.delSucc(itPrev);
			} else {
				visibArcs.popFront();
			}
		} else {
			// check if its a genmergerspanning vis arc, merge cases later
			node firstn = nullptr, secondn = nullptr;
			for (node n : m_path[(*it).x1()]) {
				node en = m_pPR->expandedNode(n);
				if (en != nullptr
				 && m_pPR->typeOf(n) == Graph::NodeType::generalizationExpander) {
					firstn = en;
					break;
				}
			}
			for (node n : m_path[(*it).x2()]) {
				node en = m_pPR->expandedNode(n);
				if (en != nullptr
				 && m_pPR->typeOf(n) == Graph::NodeType::generalizationExpander) {
					secondn = en;
					break;
				}
			}
			if (firstn != nullptr && secondn != nullptr && firstn == secondn) {
				if (itPrev.valid()) {
					visibArcs.delSucc(itPrev);
				} else {
					visibArcs.popFront();
				}
			} else {
				itPrev = it;
			}
		}
	}
}

#ifdef OGDF_DEBUG

void CommonCompactionConstraintGraphBase::writeGML(const char *filename) const {
	std::ofstream os(filename);
	writeGML(os);
}

void CommonCompactionConstraintGraphBase::writeGML(const char *filename, const NodeArray<bool> &one) const {
	std::ofstream os(filename);
	writeGML(os, one);
}

void CommonCompactionConstraintGraphBase::writeGML(std::ostream &os) const {
	NodeArray<bool> one(*this, false);
	writeGML(os, one);
}

void CommonCompactionConstraintGraphBase::writeGML(std::ostream &os, const NodeArray<bool> &one) const {
	const Graph &G = *this;
	GraphAttributes GA(G,
	  GraphAttributes::nodeGraphics |
	  GraphAttributes::nodeStyle |
	  GraphAttributes::edgeGraphics |
	  GraphAttributes::edgeLabel |
	  GraphAttributes::edgeStyle);

	GA.directed() = true;

	for (node v : G.nodes) {
		GA.width(v) = GA.height(v) = 30.0;

		if (m_extraNode[v]) {
			GA.label(v) = "0";
			GA.fillColor(v) = one[v] ? "F0F0FF" : "00FFFF";
		} else {
			GA.label(v) = to_string(m_pPR->expandedNode(m_path[v].front())->index());
			GA.fillColor(v) = one[v] ? "FF0F0F" : "FFFF00";
		}
	}

	for (edge e : G.edges) {
		GA.label(e) = getLengthString(e);

		switch (m_type[e]) {
		case ConstraintEdgeType::BasicArc:
			GA.strokeColor(e) = "FF0000"; // red
			break;
		case ConstraintEdgeType::VertexSizeArc:
			GA.strokeColor(e) = "0000FF"; // blue
			break;
		case ConstraintEdgeType::VisibilityArc:
			GA.strokeColor(e) = "00FF00"; // green
			break;
		case ConstraintEdgeType::ReducibleArc:
			GA.strokeColor(e) = "AA00AA"; // dark rose
			break;
		case ConstraintEdgeType::FixToZeroArc:
			GA.strokeColor(e) = "AF00FF"; // violet
			break;
		case ConstraintEdgeType::MedianArc:
			GA.strokeColor(e) = "FF00FF"; // rose
			break;
		}

		/* Maybe for each edge:
		 *   LabelGraphics [
		 *     type "text"
		 *     fill "#000000"
		 *     anchor "w"
		 *   ]
		 */
	}

	GraphIO::writeGML(GA, os);
}

#endif

}
