/** \file
 * \brief implementation of the class ExtractKuratowskis
 *
 * \author Jens Schmidt
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


#include <ogdf/planarity/ExtractKuratowskis.h>


namespace ogdf {

std::ostream &operator<<(std::ostream &os, const KuratowskiWrapper::SubdivisionType &obj) {
	switch (obj) {
		case KuratowskiWrapper::SubdivisionType::A:   os << "A";       break;
		case KuratowskiWrapper::SubdivisionType::AB:  os << "AB";      break;
		case KuratowskiWrapper::SubdivisionType::AC:  os << "AC";      break;
		case KuratowskiWrapper::SubdivisionType::AD:  os << "AD";      break;
		case KuratowskiWrapper::SubdivisionType::AE1: os << "AE1";     break;
		case KuratowskiWrapper::SubdivisionType::AE2: os << "AE2";     break;
		case KuratowskiWrapper::SubdivisionType::AE3: os << "AE3";     break;
		case KuratowskiWrapper::SubdivisionType::AE4: os << "AE4";     break;
		case KuratowskiWrapper::SubdivisionType::B:   os << "B";       break;
		case KuratowskiWrapper::SubdivisionType::C:   os << "C";       break;
		case KuratowskiWrapper::SubdivisionType::D:   os << "D";       break;
		case KuratowskiWrapper::SubdivisionType::E1:  os << "E1";      break;
		case KuratowskiWrapper::SubdivisionType::E2:  os << "E2";      break;
		case KuratowskiWrapper::SubdivisionType::E3:  os << "E3";      break;
		case KuratowskiWrapper::SubdivisionType::E4:  os << "E4";      break;
		case KuratowskiWrapper::SubdivisionType::E5:  os << "E5";      break;
	}
	return os;
}

//! Copy paths to subdivision (auxiliary method)
template<typename TPath, typename TEdges>
inline void copyPathsToSubdivision(std::initializer_list<TPath> paths, TEdges &edges) {
	for (const auto &path : paths) {
		for (edge e : path) {
			edges.pushBack(e);
		}
	}
}

//! Extracts all possible paths with backtracking using given edges and special constraints
class DynamicBacktrack {
public:
	//! Constructor
	DynamicBacktrack(
			const Graph& g,
			const NodeArray<int>& dfi,
			const EdgeArray<int>& flags)
			:	m_flags(flags),
				m_dfi(dfi),
				m_parent(g,nullptr) {
	}

	//! Reinitializes backtracking with new constraints. All paths will be traversed again.
	/** Startedges are either only \p startInclude or not \p startExclude, all startedges
	 * have to contain the flag \p startFlag, if \p startFlag != 0. The \p start- and \p endnode
	 * of extracted paths is given, too.
	 */
	void init(
			const node start,
			const node end,
			const bool less,
			const int flag,
			const int startFlag,
			const edge startInclude,
			const edge startExlude);

	//! Returns next possible path from \a start- to \p endnode, if exists.
	/** The path is returned to \p list. After that a process image is made,
	 * allowing to pause backtracking and extracting further paths later.
	 * \p endnode returns the last traversed node.
	 */
	bool addNextPath(SListPure<edge>& list, node& endnode);

	//! Returns next possible path under constraints from \a start- to \p endnode, if exists.
	/** All paths avoid edges in \p exclude, except if on an edge with flag \p exceptOnEdge.
	 * The NodeArray \p nodeflags is used to mark visited nodes. Only that part of the path,
	 * which doesn't contain \p exclude-edges is finally added.
	 * The path is returned to \p list. After that a process image is made,
	 * allowing to pause backtracking and extracting further paths later.
	 * \p endnode returns the last traversed node.
	 */
	bool addNextPathExclude(SListPure<edge>& list,
							node& endnode,
							const NodeArray<int>& nodeflags,
							int exclude,
							int exceptOnEdge);

	// avoid automatic creation of assignment operator
	//! Assignment is not defined!
	DynamicBacktrack &operator=(const DynamicBacktrack &);

	//! Marks an edge with three Flags: externalPath, pertinentPath and/or singlePath
	enum class KuratowskiFlag {
		externalPath		= 0x00001, // external paths, e.g. stopX->Ancestor
		pertinentPath		= 0x00002, // pertinent paths, e.g. wNode->V
		singlePath			= 0x00004, // marker for one single path
	};

protected:
	//! Flags, that partition the edges into pertinent and external subgraphs
	const EdgeArray<int>& m_flags;
	//! The one and only DFI-NodeArray
	const NodeArray<int>& m_dfi;

	//! Start node of backtracking
	node m_start;
	//! Identifies endnodes
	node m_end;
	//! Iff true, DFI of endnodes has to be < \a DFI[end], otherwise the only valid endnode is \a end
	bool m_less;
	//! Every traversed edge has to be signed with this flag
	int m_flag;

	//! Saves the parent edge for each node in path
	NodeArray<adjEntry> m_parent;

	//! Backtracking stack. A nullptr-element indicates a return from a child node
	ArrayBuffer<adjEntry> stack;
};

inline int operator & (int lhs, DynamicBacktrack::KuratowskiFlag rhs) {
	return lhs & static_cast<int>(rhs);
}

inline int operator |= (int& lhs, DynamicBacktrack::KuratowskiFlag rhs) {
	lhs |= static_cast<int>(rhs);
	return lhs;
}

inline int operator &= (int& lhs, DynamicBacktrack::KuratowskiFlag rhs) {
	lhs &= static_cast<int>(rhs);
	return lhs;
}

inline int operator ~ (DynamicBacktrack::KuratowskiFlag rhs) {
	return ~static_cast<int>(rhs);
}

// reinitializes backtracking. all paths will be traversed again. startedges are
// either startInclude or not startExclude, all startedges have to contain the flag
// startFlag
void DynamicBacktrack::init(
				const node start,
				const node end,
				const bool less,
				const int flag,
				const int startFlag = 0,
				const edge startInclude = nullptr,
				const edge startExlude = nullptr)
{
	OGDF_ASSERT(start!=nullptr);
	OGDF_ASSERT(end!=nullptr);
	m_start = start;
	m_end = end;
	m_less = less;
	m_flag = flag;

	// init stack
	stack.clear();
	if (startInclude == nullptr) {
		for (adjEntry adj : start->adjEntries) {
			if (((m_flags[adj->theEdge()] & startFlag) == startFlag) &&
				adj->theEdge() != startExlude) {
				stack.push(nullptr);
				stack.push(adj);
			}
		}
	} else {
		for (adjEntry adj : start->adjEntries) {
			if (adj->theEdge() == startInclude &&
				(m_flags[adj->theEdge()] & startFlag) == startFlag) {
				stack.push(nullptr);
				stack.push(adj);
			}
		}
	}

	// init array parent
	if (!stack.empty()) {
		m_parent.fill(nullptr);
		m_parent[start] = stack.top();
	}
}

// returns the next possible path from start to endnode, if exists.
// endnode returns the last traversed node.
bool DynamicBacktrack::addNextPath(SListPure<edge>& list, node& endnode) {
	node v = nullptr;

	while (!stack.empty()) {
		// backtrack
		adjEntry adj = stack.popRet();

		// return from a child node: delete parent
		if (adj==nullptr) {
			// go to parent and delete visited flag
			node temp = v;
			v = m_parent[temp]->theNode();
			m_parent[temp] = nullptr;
			continue;
		}

		// get and mark node
		v = adj->twinNode();
		m_parent[v] = adj;

		// path found
		if ((m_less && m_dfi[v]<m_dfi[m_end]) || (!m_less && v==m_end))
		{
			// extract path
			endnode = v;
			list.clear();
			list.pushBack(adj->theEdge());
			while(adj->theNode() != m_start) {
				adj = m_parent[adj->theNode()];
				list.pushBack(adj->theEdge());
			}

			// in a following call of this method we'll have to reconstruct the actual
			// state, therefore delete the last nullptrs and visited flags on stack
			while (!stack.empty() && stack.top()==nullptr) {
				stack.pop();
				node temp = v;
				v = m_parent[temp]->theNode();
				m_parent[temp] = nullptr;
			}

			// return bool, if path found
			return true;
		}

		// push all possible child-nodes
		for (adjEntry adjV : v->adjEntries) {
			// if edge is signed and target node was not visited before
			if ((m_flags[adjV->theEdge()] & m_flag) && (m_parent[adjV->twinNode()] == nullptr)) {
				stack.push(nullptr);
				stack.push(adjV);
			}
		}
	}
	return false;
}

// returns the next possible path from start to endnode, if exists.
// endnode returns the last traversed node. all paths avoid "exclude"-nodes, except if
// on an edge with flag "exceptOnEdge". only the part of the path, that doesn't
// contain "exclude"-nodes is finally added. Here also the startedges computed in init()
// are considered to match these conditions.
bool DynamicBacktrack::addNextPathExclude(
				SListPure<edge>& list,
				node& endnode,
				const NodeArray<int>& nodeflags,
				int exclude,
				int exceptOnEdge) {
	node v = nullptr;

	while (!stack.empty()) {
		// backtrack
		adjEntry adj = stack.popRet();

		// return from a child node: delete parent
		if (adj==nullptr) {
			// go to parent and delete visited flag
			node temp = v;
			v = m_parent[temp]->theNode();
			m_parent[temp] = nullptr;
			continue;
		}

		// get and mark node
		v = adj->twinNode();

		// check if startedges computed in init() match th conditions
		if (nodeflags[v]==exclude && !(m_flags[adj->theEdge()] & exceptOnEdge)) {
			OGDF_ASSERT(stack.top()==nullptr);
			stack.pop();
			continue;
		}
		m_parent[v] = adj;

		// path found
		if ((m_less && m_dfi[v] < m_dfi[m_end]) || (!m_less && v==m_end))
		{
			// extract path vice versa until the startnode or an exclude-node is found
			endnode = v;
			list.clear();
			OGDF_ASSERT(nodeflags[v] != exclude);
			list.pushBack(adj->theEdge());
			while (adj->theNode() != m_start && nodeflags[adj->theNode()] != exclude) {
				adj = m_parent[adj->theNode()];
				list.pushBack(adj->theEdge());
			}

			// in a following call of this method we'll have to reconstruct the actual
			// state, therefore delete the last nullptrs and visited flags on stack
			while (!stack.empty() && stack.top()==nullptr) {
				stack.pop();
				node temp = v;
				v = m_parent[temp]->theNode();
				m_parent[temp] = nullptr;
			}

			// return bool, if path found
			return true;
		}

		// push all possible child-nodes
		for(adjEntry adjV : v->adjEntries) {
			node x = adjV->twinNode();
			edge e = adjV->theEdge();
			// if edge is signed and target node was not visited before
			if ((m_flags[e] & m_flag)
			 && m_parent[x] == nullptr
			 // don't allow exclude-nodes, if not on an except-edge
			 && (nodeflags[x] != exclude
			  || (m_flags[e] & exceptOnEdge))) {
				stack.push(nullptr);
				stack.push(adj);
			}
		}
	}
	return false;
}

// class ExtractKuratowski
ExtractKuratowskis::ExtractKuratowskis(BoyerMyrvoldPlanar& bm) :
	BMP(bm),
	m_g(bm.m_g),
	m_embeddingGrade(bm.m_embeddingGrade),
	m_avoidE2Minors(bm.m_avoidE2Minors),

	m_wasHere(m_g,0),

	// initialize Members of BoyerMyrvoldPlanar
	m_dfi(bm.m_dfi),
	m_nodeFromDFI(bm.m_nodeFromDFI),
	m_adjParent(bm.m_adjParent)
{
	OGDF_ASSERT(m_embeddingGrade == BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited ||
				m_embeddingGrade > 0);
	// if only structures are limited, subdivisions must not be limited
	if (bm.m_limitStructures) m_embeddingGrade = static_cast<int>(BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited);
	m_nodeMarker = 0;

	// flip Graph and merge virtual with real nodes, if not already done
	bm.flipBicomp(1,-1,m_wasHere,true,true);
}

// returns the type of Kuratowski subdivision in list (none, K33 or K5)
ExtractKuratowskis::KuratowskiType ExtractKuratowskis::whichKuratowski(
					const Graph& m_g,
					const NodeArray<int>& /*m_dfi*/,
					const SListPure<edge>& list) {
	OGDF_ASSERT(!list.empty());
	EdgeArray<int> edgenumber(m_g,0);

	// count edges
	for (edge e : list) {
		if (edgenumber[e] == 1) {
			return ExtractKuratowskis::KuratowskiType::none;
		}
		edgenumber[e] = 1;
	}

	return whichKuratowskiArray(m_g,/*m_dfi,*/edgenumber);
}

// returns the type of Kuratowski subdivision in list (none, K33 or K5)
// the edgenumber has to be 1 for used edges, otherwise 0
ExtractKuratowskis::KuratowskiType ExtractKuratowskis::whichKuratowskiArray(
	const Graph& m_g,
#if 0
	const NodeArray<int> &m_dfi,
#endif
	EdgeArray<int>& edgenumber)
{
	NodeArray<int> nodenumber(m_g, 0);

#ifdef OGDF_DEBUG
	for (edge e : m_g.edges)
		OGDF_ASSERT(edgenumber[e] == 0 || edgenumber[e] == 1);
#endif

	// count incident nodes
	int allEdges = 0;
	for (edge e : m_g.edges) {
		if (edgenumber[e] == 1) {
			++allEdges;
			++nodenumber[e->source()];
			++nodenumber[e->target()];
		}
	}
	if (allEdges < 9) {
		return ExtractKuratowskis::KuratowskiType::none;
	}

	node K33Nodes[6];
	node K5Nodes[5];

	int degree3nodes = 0;
	int degree4nodes = 0;
	for (node v : m_g.nodes) {
		if (nodenumber[v] > 4 || nodenumber[v] == 1) {
			return ExtractKuratowskis::KuratowskiType::none;
		}
		if (nodenumber[v] == 3) {
			K33Nodes[degree3nodes] = v;
			++degree3nodes;
		} else if (nodenumber[v] == 4) {
			K5Nodes[degree4nodes] = v;
			++degree4nodes;
		}
	}

	// check on K3,3
	int paths = 0;
	if (degree3nodes == 6) {
		if (degree4nodes > 0) {
			return ExtractKuratowskis::KuratowskiType::none;
		}

		int K33Partition[6] = { 0, -1, -1, -1, -1, -1 };
		bool K33Links[6][6] = {
			{ 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 }
		};

		for (int i = 0; i<6; ++i) {
			for(adjEntry adj : K33Nodes[i]->adjEntries) {
				edge e = adj->theEdge();
				if (edgenumber[e] > 0) { // not visited
					edgenumber[e] = -2; // visited
					node v = e->opposite(K33Nodes[i]);
					// traverse nodedegree-2 path until degree-3 node found
					while (nodenumber[v] != 3) {
						nodenumber[v] = -2; // visited
						edge ed = nullptr;
						for(adjEntry adjV : v->adjEntries) {
							ed = adjV->theEdge();
							if (edgenumber[ed] > 0) break;
						}
						OGDF_ASSERT(ed != nullptr);
						OGDF_ASSERT(edgenumber[ed] > 0);
						edgenumber[ed] = -2; // visited
						v = ed->opposite(v);
					}
					int ii;
					for (ii = 0; ii < 6; ++ii) if (K33Nodes[ii] == v) break;
					OGDF_ASSERT(ii >= 0);
					OGDF_ASSERT(ii <= 5);
					if (K33Partition[i] != K33Partition[ii]) {
						++paths;
						if (K33Partition[ii] == -1) K33Partition[ii] = !K33Partition[i];
						if (!K33Links[i][ii]) {
							K33Links[i][ii] = true;
						} else {
							return ExtractKuratowskis::KuratowskiType::none;
						}
					} else {
						return ExtractKuratowskis::KuratowskiType::none;
					}
				}
			}
		}
		return paths == 9 ? ExtractKuratowskis::KuratowskiType::K33 : ExtractKuratowskis::KuratowskiType::none;
	} else if (degree4nodes == 5) {
		// check on K5
		if (degree3nodes > 0) {
			return ExtractKuratowskis::KuratowskiType::none;
		}
		for (auto &K5Node : K5Nodes) {
			for(adjEntry adj : K5Node->adjEntries) {
				edge e = adj->theEdge();
				if (edgenumber[e] > 0) { // not visited
					edgenumber[e] = -2; // visited
					node v = e->opposite(K5Node);
					// traverse nodedegree-2 path until degree-4 node found
					while (nodenumber[v] != 4) {
						nodenumber[v] = -2; // visited
						edge ed = nullptr;
						for(adjEntry adjV : v->adjEntries) {
							ed = adjV->theEdge();
							if (edgenumber[ed] > 0) break;
						}
						OGDF_ASSERT(ed != nullptr);
						if (edgenumber[ed] <= 0) break;
						edgenumber[ed] = -2; // visited
						v = ed->opposite(v);
					}
					if (nodenumber[v] == 4) ++paths;
				}
			}
		}
		return paths == 10 ? ExtractKuratowskis::KuratowskiType::K5 : ExtractKuratowskis::KuratowskiType::none;
	}
	return ExtractKuratowskis::KuratowskiType::none;
}


// returns true, if kuratowski EdgeArray isn't already contained in output
bool ExtractKuratowskis::isANewKuratowski(
#if 0
		const Graph& g,
#endif
		const EdgeArray<int>& test,
		const SList<KuratowskiWrapper>& output)
{
	for (auto kw : output) {
		bool differentEdgeFound = false;
		for (edge e : kw.edgeList) {
			if (!test[e]) {
				differentEdgeFound = true;
				break;
			}
		}
		if (!differentEdgeFound) {
			Logger::slout() << "Kuratowski is already in list as subdivisiontype " << kw.subdivisionType << std::endl;
			return false;
		}
	}
	return true;
}

// returns true, if kuratowski edgelist isn't already contained in output
bool ExtractKuratowskis::isANewKuratowski(
		const Graph& g,
		const SListPure<edge>& kuratowski,
		const SList<KuratowskiWrapper>& output)
{
	EdgeArray<int> test(g,0);
	for (edge e : kuratowski) test[e] = 1;
	return isANewKuratowski(/*g,*/test,output);
}

// returns adjEntry of the edge between node high and that node
// with the lowest DFI not less than the DFI of low
inline adjEntry ExtractKuratowskis::adjToLowestNodeBelow(node high, int low)
{
	int result = 0;
	adjEntry resultAdj = nullptr;
	for (adjEntry adj : high->adjEntries) {
		int temp = m_dfi[adj->twinNode()];
		if (temp >= low && (result == 0 || temp < result)) {
			result = temp;
			resultAdj = adj->twin();
		}
	}
	return result == 0 ? nullptr : resultAdj;
}

// add DFS-path from node bottom to node top to edgelist
// each virtual node has to be merged
inline void ExtractKuratowskis::addDFSPath(
	SListPure<edge>& list,
	node bottom,
	node top)
{
	if (bottom == top) return;
	adjEntry adj = m_adjParent[bottom];
	list.pushBack(adj->theEdge());
	while (adj->theNode() != top) {
		adj = m_adjParent[adj->theNode()];
		list.pushBack(adj->theEdge());
	}
}

// the same as above but list is reversed
inline void ExtractKuratowskis::addDFSPathReverse(
						SListPure<edge>& list,
						node bottom,
						node top) {
	if (bottom == top) return;
	adjEntry adj = m_adjParent[bottom];
	list.pushFront(adj->theEdge());
	while (adj->theNode() != top) {
		adj = m_adjParent[adj->theNode()];
		list.pushFront(adj->theEdge());
	}
}

// separate list1 from edges already contained in list2
inline void ExtractKuratowskis::truncateEdgelist(
					SListPure<edge>& list1,
					const SListPure<edge>& list2)
{
	SListConstIterator<edge> it = list2.begin();
	while (!list1.empty() && it.valid() && list1.front() == *it) {
		list1.popFront();
		++it;
	}
}

// extracts a type A minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorA(
				SList<KuratowskiWrapper>& output,
				const KuratowskiStructure& k,
#if 0
				const WInfo& info,
#endif
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	OGDF_ASSERT(k.RReal != k.V);
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper A;

	// add all external face edges
	addExternalFacePath(A.edgeList,k.externalFacePath);

	// add the path from v to u, this is only possible after computation of pathX and pathY
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(A.edgeList,k.V,endnodeX);
	} else {
		addDFSPath(A.edgeList,k.V,endnodeY);
	}

	copyPathsToSubdivision({pathX, pathY, pathW}, A.edgeList);
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,A.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,A.edgeList,output));
	A.subdivisionType = KuratowskiWrapper::SubdivisionType::A;
	A.V = k.V;
	output.pushBack(A);
}

// extracts a type B minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorB(
				SList<KuratowskiWrapper>& output,
#if 0
				NodeArray<int>& nodeflags,
				const int nodemarker,
#endif
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper B;

	// find ExternE-struct suitable for wNode
	SListIterator<ExternE> itExternW = info.externEStart;
	while ((*itExternW).theNode != info.w) {
		++itExternW;
	}
	OGDF_ASSERT(itExternW.valid());
	OGDF_ASSERT((*itExternW).theNode == info.w);
	ExternE& externE(*itExternW);
	OGDF_ASSERT(externE.theNode == pathW.front()->source() ||
				externE.theNode == pathW.front()->target());

	// check, if a external path sharing the first pathW-edge exists
	SListIterator<node> itEnd = externE.endnodes.begin();
	SListIterator<SListPure<edge> > itPath = externE.externalPaths.begin();
	for (int start : externE.startnodes) {
		if (start != m_dfi[pathW.front()->opposite(info.w)]) {
			++itEnd;
			++itPath;
			continue;
		}

		// if path was preprocessed, copy path
		node endnodeWExtern = *itEnd;
		if (!(*itPath).empty()) {
			B.edgeList = (*itPath);
		} else {
			// else traverse external Path starting with z. forbid edges starting at W,
			// that are different from the edge w->z.
			adjEntry adj = adjToLowestNodeBelow(endnodeWExtern, start);
			B.edgeList.pushFront(adj->theEdge());
			addDFSPathReverse(B.edgeList,adj->theNode(),info.w);

			// copy list
			*itPath = B.edgeList;
		}

		// truncate pathZ from edges already contained in pathW
		OGDF_ASSERT(B.edgeList.front() == pathW.front());
		truncateEdgelist(B.edgeList,pathW);

		// add external face edges
		addExternalFacePath(B.edgeList,k.externalFacePath);

		// compute dfi-minimum and maximum of all three paths to node Ancestor u
		// add the dfs-path from minimum to maximum
		node min,max;
		if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
			min = endnodeX;
			max = endnodeY;
		} else {
			min = endnodeY;
			max = endnodeX;
		}
		if (m_dfi[endnodeWExtern] < m_dfi[min]) {
			min = endnodeWExtern;
		} else {
			if (m_dfi[endnodeWExtern] > m_dfi[max]) max = endnodeWExtern;
		}
		addDFSPath(B.edgeList,max,min);

		copyPathsToSubdivision({pathX, pathY, pathW}, B.edgeList);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,B.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,B.edgeList,output));
		if (info.minorType & WInfo::MinorType::A) {
			B.subdivisionType = KuratowskiWrapper::SubdivisionType::AB;
		} else {
			B.subdivisionType = KuratowskiWrapper::SubdivisionType::B;
		}
		B.V = k.V;
		output.pushBack(B);
		B.edgeList.clear();

//		break;
	}
}

// extracts a type B minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorBBundles(
				SList<KuratowskiWrapper>& output,
				NodeArray<int>& nodeflags,
				const int nodemarker,
				const KuratowskiStructure& k,
				EdgeArray<int>& flags,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	KuratowskiWrapper B;
	OGDF_ASSERT(flags[pathW.back()] & DynamicBacktrack::KuratowskiFlag::pertinentPath);

	// check, if pertinent pathW (w->u) traverses node z
	if (!(flags[pathW.back()] & DynamicBacktrack::KuratowskiFlag::externalPath)) return;

	// mark single pathW in flags, so that pathW and the externalPath
	// don't interfere later
	for (edge e : pathW) {
		flags[e] |= DynamicBacktrack::KuratowskiFlag::singlePath;
		nodeflags[e->source()] = nodemarker;
		nodeflags[e->target()] = nodemarker;
	}

	// traverse all possible external Paths out of z. forbid edges starting at W,
	// that are different from the edge w->z
	node endnodeWExtern;
	DynamicBacktrack backtrackExtern(m_g,m_dfi,flags);
	backtrackExtern.init(info.w, k.V, true,
	                     static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
	                     static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),pathW.back(),nullptr);
	while (backtrackExtern.addNextPathExclude(B.edgeList,
	                                          endnodeWExtern,
	                                          nodeflags,
	                                          nodemarker,
	                                          static_cast<int>(DynamicBacktrack::KuratowskiFlag::singlePath))) {
		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			break;

		// add external face edges
		addExternalFacePath(B.edgeList,k.externalFacePath);

		// compute dfi-minimum and maximum of all three paths to node Ancestor u
		// add the dfs-path from minimum to maximum
		node min,max;
		if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
			min = endnodeX;
			max = endnodeY;
		} else {
			min = endnodeY;
			max = endnodeX;
		}
		if (m_dfi[endnodeWExtern] < m_dfi[min]) {
			min = endnodeWExtern;
		} else if (m_dfi[endnodeWExtern] > m_dfi[max]) {
			max = endnodeWExtern;
		}
		addDFSPath(B.edgeList,max,min);

		copyPathsToSubdivision({pathX, pathY, pathW}, B.edgeList);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,B.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,B.edgeList,output));
		if (info.minorType & WInfo::MinorType::A) {
			B.subdivisionType = KuratowskiWrapper::SubdivisionType::AB;
		} else {
			B.subdivisionType = KuratowskiWrapper::SubdivisionType::B;
		}
		B.V = k.V;
		output.pushBack(B);
		B.edgeList.clear();
	}

	// delete marked single pathW
	for (edge e : pathW) {
		flags[e] &= ~DynamicBacktrack::KuratowskiFlag::singlePath;
	}
}

// extracts a type C minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorC(
				SList<KuratowskiWrapper>& output,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper C;
	SListPure<edge> tempC;

	// the case, that px is above stopX
	OGDF_ASSERT(info.pxAboveStopX || info.pyAboveStopY);

	// add the path from v to u, this is only possible after computation
	// of pathX and pathY
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(tempC,k.V,endnodeX);
	} else {
		addDFSPath(tempC,k.V,endnodeY);
	}

	// add highestFacePath of wNode
	OGDF_ASSERT(info.highestXYPath->size() >= 2);
	for (auto itE = info.highestXYPath->begin() + 1; itE != info.highestXYPath->end(); ++itE) {
		tempC.pushBack((*itE)->theEdge());
	}

	// the case, that px is above stopX
	if (info.pxAboveStopX) {
		C.edgeList = tempC;

		// add the external face path edges except the path from py/stopY to R
		node end;
		if (info.pyAboveStopY) {
			end = info.highestXYPath->top()->theNode();
		} else {
			end = k.stopY;
		}
		for (auto adj : k.externalFacePath) {
			C.edgeList.pushBack(adj->theEdge());
			if (adj->theNode() == end) break;
		}

		copyPathsToSubdivision({pathX, pathY, pathW}, C.edgeList);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,C.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,C.edgeList,output));
		if (info.minorType & WInfo::MinorType::A) {
			C.subdivisionType = KuratowskiWrapper::SubdivisionType::AC;
		} else {
			C.subdivisionType = KuratowskiWrapper::SubdivisionType::C;
		}
		C.V = k.V;
		output.pushBack(C);
		C.edgeList.clear();
	}

	// the case, that py is above stopY
	if (info.pyAboveStopY) {
		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			return;

		C.edgeList = tempC;

		// add the external face path edges except the path from px/stopX to R
		node start;
		if (info.pxAboveStopX) {
			start = (*info.highestXYPath)[0]->theNode();
		} else {
			start = k.stopX;
		}
		bool after = false;
		for (auto adj : k.externalFacePath) {
			if (after) {
				C.edgeList.pushBack(adj->theEdge());
			} else if (adj->theNode() == start) {
				after = true;
			}
		}

		copyPathsToSubdivision({pathX, pathY, pathW}, C.edgeList);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,C.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,C.edgeList,output));
		if (info.minorType & WInfo::MinorType::A) {
			C.subdivisionType = KuratowskiWrapper::SubdivisionType::AC;
		} else {
			C.subdivisionType = KuratowskiWrapper::SubdivisionType::C;
		}
		C.V = k.V;
		output.pushBack(C);
	}
}

// extracts a type D minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorD(
				SList<KuratowskiWrapper>& output,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper D;

	// add the path from v to u, this is only possible after computation of pathX and pathY
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(D.edgeList,k.V,endnodeX);
	} else {
		addDFSPath(D.edgeList,k.V,endnodeY);
	}

	// add the external face path edges except the part from R to the nearest of
	// the two nodes stopX and px resp. the part to stopY/py
	node start;
	if (info.pxAboveStopX) {
		start = (*info.highestXYPath)[0]->theNode();
	} else {
		start = k.stopX;
	}
	node end;
	if (info.pyAboveStopY) {
		end = info.highestXYPath->top()->theNode();
	} else {
		end = k.stopY;
	}
	bool between = false;
	for (auto adj : k.externalFacePath) {
		node temp = adj->theNode();
		if (between) D.edgeList.pushBack(adj->theEdge());
		if (temp == start) {
			between = true;
		} else if (temp == end) {
			between = false;
		}
	}

	// add highestFacePath of wNode
	OGDF_ASSERT(info.highestXYPath->size() >= 2);
	for (auto itE = info.highestXYPath->begin() + 1; itE != info.highestXYPath->end(); ++itE) {
		D.edgeList.pushBack((*itE)->theEdge());
	}

	// add path from first zNode to R
	OGDF_ASSERT(!info.zPath->empty());
	for (auto itE = info.zPath->begin() + 1; itE != info.zPath->end(); ++itE) {
		D.edgeList.pushBack((*itE)->theEdge());
	}

	copyPathsToSubdivision({pathX, pathY, pathW}, D.edgeList);
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,D.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,D.edgeList,output));
	if (info.minorType & WInfo::MinorType::A) {
		D.subdivisionType = KuratowskiWrapper::SubdivisionType::AD;
	} else {
		D.subdivisionType = KuratowskiWrapper::SubdivisionType::D;
	}
	D.V = k.V;
	output.pushBack(D);
}

// extracts a subtype E1 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE1(
				SList<KuratowskiWrapper>& output,
				int before,
#if 0
				const node z,
#endif
				const node px,
				const node py,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	OGDF_ASSERT(before == -1 || before == 1);
	KuratowskiWrapper E1;

	// add highestFacePath of wNode
	for (auto it = info.highestXYPath->begin() + 1; it != info.highestXYPath->end(); ++it) {
		E1.edgeList.pushBack((*it)->theEdge());
	}

	if (before == -1) {
		// z is before w on external face path

		copyPathsToSubdivision({pathY}, E1.edgeList);

		// add the path from v to u, this is only possible after computation of
		// pathX and pathY
		if (m_dfi[endnodeZ] < m_dfi[endnodeY]) {
			addDFSPath(E1.edgeList,k.V,endnodeZ);
		} else {
			addDFSPath(E1.edgeList,k.V,endnodeY);
		}

		// add the external face path edges except the part from stopY/py to R
		node stop;
		if (info.pyAboveStopY) {
			stop = py;
		} else {
			stop = k.stopY;
		}
		for (auto adj : k.externalFacePath) {
			E1.edgeList.pushBack(adj->theEdge());
			if (adj->theNode() == stop) break;
		}
	} else {
		// z is after w on external face path

		// if minor A occurs, add the dfs-path from node RReal to V, that isn't anymore
		// involved because of removing pathY
		if (k.RReal != k.V) addDFSPath(E1.edgeList,k.RReal,k.V);

		copyPathsToSubdivision({pathX}, E1.edgeList);

		// add the path from v to u, this is only possible after computation of
		// pathX and pathY
		if (m_dfi[endnodeZ] < m_dfi[endnodeX]) {
			addDFSPath(E1.edgeList,k.V,endnodeZ);
		} else {
			addDFSPath(E1.edgeList,k.V,endnodeX);
		}

		// add the external face path edges except the part from stopX/px to R
		node start;
		if (info.pxAboveStopX) {
			start = px;
		} else {
			start = k.stopX;
		}
		bool after = false;
		for (auto adj : k.externalFacePath) {
			if (after) {
				E1.edgeList.pushBack(adj->theEdge());
			} else if (adj->theNode() == start) {
				after = true;
			}
		}
	}

	copyPathsToSubdivision({pathW, pathZ}, E1.edgeList);
	// push this subdivision to kuratowskilist
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E1.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E1.edgeList,output));
	if (info.minorType & WInfo::MinorType::A) {
		E1.subdivisionType = KuratowskiWrapper::SubdivisionType::AE1;
	} else {
		E1.subdivisionType = KuratowskiWrapper::SubdivisionType::E1;
	}
	E1.V = k.V;
	output.pushBack(E1);
}

// extracts a subtype E2 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE2(
				SList<KuratowskiWrapper>& output,
#if 0
				int before,
				const node z,
				const node px,
				const node py,
#endif
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				//const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ/*,
				const node endnodeZ*/
				)
{
	OGDF_ASSERT(!m_avoidE2Minors);

	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E2;

	// add the path from v to u
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(E2.edgeList,k.V,endnodeX);
	} else {
		addDFSPath(E2.edgeList,k.V,endnodeY);
	}

	// add the external face path edges
	for (auto adj : k.externalFacePath) {
		E2.edgeList.pushBack(adj->theEdge());
	}

	copyPathsToSubdivision({pathX, pathY, pathZ}, E2.edgeList);
	// push this subdivision to kuratowskilist
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E2.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E2.edgeList,output));
	if (info.minorType & WInfo::MinorType::A) {
		E2.subdivisionType = KuratowskiWrapper::SubdivisionType::AE2;
	} else {
		E2.subdivisionType = KuratowskiWrapper::SubdivisionType::E2;
	}
	E2.V = k.V;
	output.pushBack(E2);
}

// extracts a subtype E3 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE3(
				SList<KuratowskiWrapper>& output,
				int before,
				const node z,
				const node px,
				const node py,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E3;
	OGDF_ASSERT(endnodeX != endnodeY);

	copyPathsToSubdivision({pathZ}, E3.edgeList);

	// add highestFacePath px <-> py
	for (auto it = info.highestXYPath->begin() + 1; it != info.highestXYPath->end(); ++it) {
		E3.edgeList.pushBack((*it)->theEdge());
	}

	// check, if endnodeX or endnodeY is descendant
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		OGDF_ASSERT(m_dfi[endnodeZ] < m_dfi[endnodeY]);

		// add the path from v to u
		if (m_dfi[endnodeX] < m_dfi[endnodeZ]) {
			addDFSPath(E3.edgeList,k.V,endnodeX);
		} else {
			addDFSPath(E3.edgeList,k.V,endnodeZ);
		}

		// add the external face path edges except max(px,stopX)<->min(z,w) and v<->nearest(py,stopY)
		node start1,end1,start2;
		if (info.pxAboveStopX) {
			start1 = k.stopX;
		} else {
			start1 = px;
		}
		if (before<=0) {
			end1 = z;
		} else {
			end1 = info.w;
		}
		if (info.pyAboveStopY) {
			start2 = py;
		} else {
			start2 = k.stopY;
		}
		bool between = false;
		for (auto adj : k.externalFacePath) {
			node temp = adj->theNode();
			if (!between) E3.edgeList.pushBack(adj->theEdge());
			if (temp == start1) {
				between = true;
			} else if (temp == start2) {
				break;
			} else if (temp == end1) {
				between = false;
			}
		}
	} else {
		OGDF_ASSERT(m_dfi[endnodeZ] < m_dfi[endnodeX]);

		// add the path from v to u
		if (m_dfi[endnodeY] < m_dfi[endnodeZ]) {
			addDFSPath(E3.edgeList,k.V,endnodeY);
		} else {
			addDFSPath(E3.edgeList,k.V,endnodeZ);
		}

		// add the external face path edges except v<->min(px,stopX) and max(w,z)<->nearest(py,stopY)
		node end1,start2,end2;
		if (info.pxAboveStopX) {
			end1 = px;
		} else {
			end1 = k.stopX;
		}
		if (before>0) {
			start2 = z;
		} else {
			start2 = info.w;
		}
		if (info.pyAboveStopY) {
			end2 = k.stopY;
		} else {
			end2 = py;
		}
		bool between = true;
		for (auto adj : k.externalFacePath) {
			node temp = adj->theNode();
			if (!between) E3.edgeList.pushBack(adj->theEdge());
			if (temp == end1) {
				between = false;
			} else if (temp == start2) {
				between = true;
			} else if (temp == end2) {
				between = false;
			}
		}
	}

	copyPathsToSubdivision({pathX, pathY, pathW}, E3.edgeList);
	// push this subdivision to kuratowskilist
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E3.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E3.edgeList,output));
	if (info.minorType & WInfo::MinorType::A) {
		E3.subdivisionType = KuratowskiWrapper::SubdivisionType::AE3;
	} else {
		E3.subdivisionType = KuratowskiWrapper::SubdivisionType::E3;
	}
	E3.V = k.V;
	output.pushBack(E3);
}

// extracts a subtype E4 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE4(
				SList<KuratowskiWrapper>& output,
				int before,
				const node z,
				const node px,
				const node py,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E4;
	SListPure<edge> tempE4;
	OGDF_ASSERT((px != k.stopX && !info.pxAboveStopX) ||
				(py != k.stopY && !info.pyAboveStopY));

	copyPathsToSubdivision({pathZ}, tempE4);

	// add highestFacePath px <-> py
	for (auto it = info.highestXYPath->begin() + 1; it != info.highestXYPath->end(); ++it) {
		tempE4.pushBack((*it)->theEdge());
	}

	// compute dfi-minimum and maximum of all three paths to node Ancestor u
	// add the dfs-path from minimum to maximum
	node min,max;
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		min = endnodeX;
		max = endnodeY;
	} else {
		min = endnodeY;
		max = endnodeX;
	}
	if (m_dfi[endnodeZ] < m_dfi[min]) {
		min = endnodeZ;
	} else if (m_dfi[endnodeZ] > m_dfi[max]) {
		max = endnodeZ;
	}
	addDFSPath(tempE4,max,min);

	if (px != k.stopX && !info.pxAboveStopX) {
		E4.edgeList = tempE4;

		// add the external face path edges except max(w,z)<->min(py,stopY)
		node start,end;
		if (before<=0) {
			start = info.w;
		} else {
			start = z;
		}
		if (info.pyAboveStopY) {
			end = k.stopY;
		} else {
			end = py;
		}
		bool between = false;
		for (auto adj : k.externalFacePath) {
			node temp = adj->theNode();
			if (!between) E4.edgeList.pushBack(adj->theEdge());
			if (temp == start) {
				between = true;
			} else if (temp == end) {
				between = false;
			}
		}

		copyPathsToSubdivision({pathX, pathY, pathW}, E4.edgeList);
		// push this subdivision to kuratowski-list
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E4.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E4.edgeList,output));
		if (info.minorType & WInfo::MinorType::A) {
			E4.subdivisionType = KuratowskiWrapper::SubdivisionType::AE4;
		} else {
			E4.subdivisionType = KuratowskiWrapper::SubdivisionType::E4;
		}
		E4.V = k.V;
		output.pushBack(E4);
	}

	if (py != k.stopY && !info.pyAboveStopY) {
		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			return;

		E4.edgeList = tempE4;

		// add the external face path edges except max(px,stopX)<->min(w,z)
		node start,end;
		if (info.pxAboveStopX) {
			start = k.stopX;
		} else {
			start = px;
		}
		if (before <= 0) {
			end = z;
		} else {
			end = info.w;
		}

		bool between = false;
		for (auto adj : k.externalFacePath) {
			node temp = adj->theNode();
			if (!between) E4.edgeList.pushBack(adj->theEdge());
			if (temp == start) {
				between = true;
			} else if (temp == end) {
				between = false;
			}
		}

		copyPathsToSubdivision({pathX, pathY, pathW}, E4.edgeList);
		// push this subdivision to kuratowski-list
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E4.edgeList) == ExtractKuratowskis::KuratowskiType::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E4.edgeList,output));
		if (info.minorType & WInfo::MinorType::A) {
			E4.subdivisionType = KuratowskiWrapper::SubdivisionType::AE4;
		} else {
			E4.subdivisionType = KuratowskiWrapper::SubdivisionType::E4;
		}
		E4.V = k.V;
		output.pushBack(E4);
	}
}

// extracts a subtype E5 minor (the only minortype which represents a K5).
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE5(
				SList<KuratowskiWrapper>& output,
				/*int before,
				const node z,
				const node px,
				const node py,*/
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E5;
#if 0
	OGDF_ASSERT(px == k.stopX);
	OGDF_ASSERT(py == k.stopY);
	OGDF_ASSERT(z == info.w);
	OGDF_ASSERT(k.V == k.RReal);
#endif
	OGDF_ASSERT((endnodeX == endnodeY && m_dfi[endnodeZ] <= m_dfi[endnodeX])
	         || (endnodeX == endnodeZ && m_dfi[endnodeY] <= m_dfi[endnodeX])
	         || (endnodeY == endnodeZ && m_dfi[endnodeX] <= m_dfi[endnodeY]));

	// compute dfi-minimum of all three paths to node Ancestor u and
	// add the dfs-path from minimum to V
	node min;
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		min = endnodeX;
	} else if (m_dfi[endnodeY] < m_dfi[endnodeZ]) {
		min = endnodeY;
	} else {
		min = endnodeZ;
	}
	addDFSPath(E5.edgeList,k.V,min);

	copyPathsToSubdivision({pathZ}, E5.edgeList);

	// add highestFacePath px <-> py
	for (auto it = info.highestXYPath->begin() + 1; it != info.highestXYPath->end(); ++it)
		E5.edgeList.pushBack((*it)->theEdge());

	// add the external face path edges
	for (auto adj : k.externalFacePath) {
		E5.edgeList.pushBack(adj->theEdge());
	}

	copyPathsToSubdivision({pathX, pathY, pathW}, E5.edgeList);
	// push this subdivision to kuratowski-list
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E5.edgeList) == ExtractKuratowskis::KuratowskiType::K5);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E5.edgeList,output));
	E5.subdivisionType = KuratowskiWrapper::SubdivisionType::E5;
	E5.V = k.V;
	output.pushBack(E5);
}

// extracts a type E minor through splitting in different subtypes.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE(
				SList<KuratowskiWrapper>& output,
				bool firstXPath,
				bool firstYPath,
				bool firstWPath,
				bool firstWOnHighestXY,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// find external paths for each extern node z on the lower external face
	OGDF_ASSERT(info.externEStart.valid());
	OGDF_ASSERT(info.externEEnd.valid());

	int before = -1; // -1= before, 0=equal, 1=after
	node px = (*info.highestXYPath)[0]->theNode();
	node py = info.highestXYPath->top()->theNode();

	SListPure<edge> pathZ;
	SListConstIterator<node> itZEndnode;
	SListConstIterator<int> itZStartnode;
	SListConstIterator<SListPure<edge> > itEPath;

	// consider only the nodes between px and py
	for (SListConstIterator<ExternE> it = info.externEStart; it.valid(); ++it) {
		const ExternE& externE(*it);
		node z = externE.theNode;

		if (z == info.w) {
			OGDF_ASSERT(z == pathW.front()->source() || z == pathW.front()->target());
			// z = wNode
			before = 0;

			itZStartnode = externE.startnodes.begin();
			itEPath = externE.externalPaths.begin();
			for (itZEndnode = externE.endnodes.begin(); itZEndnode.valid();
			     ++itZEndnode, ++itZStartnode, ++itEPath) {
				node endnodeZ = *itZEndnode;
				SListPure<edge>& externalPath(const_cast<SListPure<edge>& >(*itEPath));

				if (!externalPath.empty()) {
					// get preprocessed path
					pathZ = externalPath;
				} else {
					adjEntry temp = adjToLowestNodeBelow(endnodeZ,*itZStartnode);
					pathZ.clear();
					pathZ.pushFront(temp->theEdge());
					addDFSPathReverse(pathZ,temp->theNode(),z);

					// copy path
					externalPath = pathZ;
				}

				// minortype E2 on z=wNode
				if (checkMinorE2(firstWPath, firstWOnHighestXY)
				 && isMinorE2(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
					               endnodeX,pathY,endnodeY,/*pathW,*/pathZ/*,endnodeZ*/);
				}

				// truncate pathZ from edges already contained in pathW
				truncateEdgelist(pathZ,pathW);

				// minortype E3 on z=wNode
				if (isMinorE3(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4 on z=wNode
				if (isMinorE4(px, py, k, info)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}

				// minortype E5 (K5)
				if (isMinorE5(px, py, k, endnodeX, endnodeY, endnodeZ)
				 // check, if pathZ shares no edge with pathW
				 && *itZStartnode != m_dfi[pathW.front()->opposite(z)]) {
					extractMinorE5(output,/*before,z,px,py,*/k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
			}
		} else {
			// z != wNode, check position of node z
			if (z == info.firstExternEAfterW) before = 1;
			OGDF_ASSERT(before != 0);
			OGDF_ASSERT(z != pathW.front()->source());
			OGDF_ASSERT(z != pathW.front()->target());

			itZStartnode = externE.startnodes.begin();
			for (itZEndnode = externE.endnodes.begin();
			     itZEndnode.valid();
			     ++itZEndnode, ++itZStartnode) {
				node endnodeZ = *itZEndnode;

				adjEntry temp = adjToLowestNodeBelow(endnodeZ,*itZStartnode);
				pathZ.clear();
				pathZ.pushFront(temp->theEdge());
				addDFSPathReverse(pathZ,temp->theNode(),z);

				// split in minorE-subtypes

				// minortype E1
				if (isMinorE1(before, firstXPath, firstYPath)) {
					extractMinorE1(output,before,/*z,*/px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E2
				if (checkMinorE2(firstWPath, firstWOnHighestXY)
				 && isMinorE2(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
					               endnodeX,pathY,endnodeY,/*pathW,*/pathZ/*,endnodeZ*/);
				}
				// minortype E3
				if (isMinorE3(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4
				if (isMinorE4(px, py, k, info)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
			}
		}

		// check if last node was reached
		if (it == info.externEEnd) break;
	}
}

// extracts a type E minor through splitting in different subtypes.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorEBundles(
				SList<KuratowskiWrapper>& output,
				bool firstXPath,
				bool firstYPath,
				bool firstWPath,
				bool firstWOnHighestXY,
				NodeArray<int>& nodeflags,
				const int nodemarker,
				const KuratowskiStructure& k,
				EdgeArray<int>& flags,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// perform backtracking for each extern node z on the lower external face
	OGDF_ASSERT(info.externEStart.valid());
	OGDF_ASSERT(info.externEEnd.valid());
	SListPure<edge> pathZ;
	node endnodeZ;
	int before = -1; // -1= before, 0=equal to wNode, 1=after
	node px = (*info.highestXYPath)[0]->theNode();
	node py = info.highestXYPath->top()->theNode();
	DynamicBacktrack backtrackZ(m_g,m_dfi,flags);

	// mark all nodes of the single pathW in flags, so that pathW and
	// the externalPath don't interfere later
	for (edge e : pathW) {
		flags[e] |= DynamicBacktrack::KuratowskiFlag::singlePath;
		nodeflags[e->source()] = nodemarker;
		nodeflags[e->target()] = nodemarker;
	}

	// consider only the nodes between px and py
	for (auto it = info.externEStart; it.valid(); ++it) {
		node z = (*it).theNode;

		if (z == info.w) {
			OGDF_ASSERT(z == pathW.back()->source() || z == pathW.back()->target());
			// z = wNode
			before = 0;

			// minortype E2 on z=wNode
			// on the first pathW: consider all pathsZ
			if (checkMinorE2(firstWPath, firstWOnHighestXY)) {
				backtrackZ.init(z, k.V, true,
				                static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
				                static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
				                nullptr, nullptr);
				while (backtrackZ.addNextPath(pathZ,endnodeZ)) {
					if (isMinorE2(endnodeX, endnodeY, endnodeZ)) {
						extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
						               endnodeX,pathY,endnodeY/*,pathW*/,pathZ/*,endnodeZ*/);
					}
				}
			}

			backtrackZ.init(z,k.V,true, static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
							static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),nullptr,nullptr);
			while (backtrackZ.addNextPathExclude(pathZ,endnodeZ,
						nodeflags,nodemarker, static_cast<int>(DynamicBacktrack::KuratowskiFlag::singlePath))) {
				// minortype E3 on z=wNode
				if (isMinorE3(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4 on z=wNode
				if (isMinorE4(px, py, k, info)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E5 (K5)
				if (isMinorE5(px, py, k, endnodeX, endnodeY, endnodeZ)
				 && pathZ.back() != pathW.back()
				 && pathZ.back()->isIncident(z)) {
					extractMinorE5(output,/*before,z,px,py,*/k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
			}
		} else {
			// z != wNode, check position of node z
			if (z == info.firstExternEAfterW) before = 1;
			OGDF_ASSERT(before != 0);
			OGDF_ASSERT(z != pathW.back()->source());
			OGDF_ASSERT(z != pathW.back()->target());

			backtrackZ.init(z,k.V,true, static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
							static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),nullptr,nullptr);
			while (backtrackZ.addNextPath(pathZ,endnodeZ)) {
				// split in minorE-subtypes

				// minortype E1
				if (isMinorE1(before, firstXPath, firstYPath)) {
					extractMinorE1(output,before,/*z,*/px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E2
				if (checkMinorE2(firstWPath, firstWOnHighestXY)
				 && isMinorE2(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
					               endnodeX,pathY,endnodeY,/*pathW,*/pathZ/*,endnodeZ*/);
				}
				// minortype E3
				if (isMinorE3(endnodeX, endnodeY, endnodeZ)) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4
				if (isMinorE4(px, py, k, info)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
					               endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
			}
		}

		// check if last node was reached
		if (it == info.externEEnd) break;

		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			break;
	}

	// delete marked single pathW
	for (edge e : pathW) {
		flags[e] &= ~DynamicBacktrack::KuratowskiFlag::singlePath;
	}
}

// extracts all kuratowski subdivisions and adds them to output
void ExtractKuratowskis::extract(
				const SListPure<KuratowskiStructure>& allKuratowskis,
				SList<KuratowskiWrapper>& output)
{
	SListPure<edge> pathX,pathY;

	// consider all different kuratowski structures
	for (const KuratowskiStructure& k : allKuratowskis) {
		// compute all possible external paths of stopX and stopY (pathX,pathY)
		bool firstXPath = true;
		auto itXStartnode = k.stopXStartnodes.begin();
		for (node endnodeX : k.stopXEndnodes) {
			pathX.clear();
			adjEntry temp = adjToLowestNodeBelow(endnodeX,*(itXStartnode++));
			pathX.pushBack(temp->theEdge());
			addDFSPath(pathX,temp->theNode(),k.stopX);

			bool firstYPath = true;
			auto itYStartnode = k.stopYStartnodes.begin();
			for (node endnodeY : k.stopYEndnodes) {
				pathY.clear();
				temp = adjToLowestNodeBelow(endnodeY,*(itYStartnode++));
				pathY.pushBack(temp->theEdge());
				addDFSPath(pathY,temp->theNode(),k.stopY);

				// if minor A occurs, other minortypes are possible with adding
				// the dfs-path from node RReal to V
				if (k.RReal != k.V) addDFSPath(pathY,k.RReal,k.V);

				// consider all possible wNodes
				ArrayBuffer<adjEntry>* oldHighestXYPath = nullptr;
				for (const WInfo &info : k.wNodes) {
					// compute all possible internal paths of this wNode
					bool firstWPath = true; // avoid multiple identical subdivisions in E2
					for (const auto &pathW : info.pertinentPaths) {
						OGDF_ASSERT(!pathX.empty());
						OGDF_ASSERT(!pathY.empty());
						OGDF_ASSERT(!pathW.empty());

						// extract minor A
						if (info.minorType & WInfo::MinorType::A)
							extractMinorA(output,k,/*info,*/pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor B
						if (info.minorType & WInfo::MinorType::B) {
							++m_nodeMarker;
							extractMinorB(output,/*m_wasHere,++m_nodeMarker,*/k,
										info,pathX,endnodeX,pathY,endnodeY,pathW);
						}

						// extract minor C
						if (info.minorType & WInfo::MinorType::C)
							extractMinorC(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor D
						if (info.minorType & WInfo::MinorType::D)
							extractMinorD(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor E including all subtypes
						if (info.minorType & WInfo::MinorType::E) {
							extractMinorE(output,firstXPath,firstYPath,firstWPath,
										oldHighestXYPath!=info.highestXYPath,k,info,
										pathX,endnodeX,pathY,endnodeY,pathW);
						}

						if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
									output.size() >= m_embeddingGrade)
							return;
						firstWPath = false;
						// break;
					}
					oldHighestXYPath = info.highestXYPath;
				}
				firstYPath = false;
				// break;
			}
			firstXPath = false;
			// break;
		}
	}
}

// extracts all kuratowski subdivisions and adds them to output
void ExtractKuratowskis::extractBundles(
				const SListPure<KuratowskiStructure>& allKuratowskis,
				SList<KuratowskiWrapper>& output)
{
	SListPure<edge> pathX,pathY,pathW;
	node endnodeX,endnodeY;

	EdgeArray<int> flags(m_g,0);
	DynamicBacktrack backtrackX(m_g,m_dfi,flags);
	DynamicBacktrack backtrackY(m_g,m_dfi,flags);
	DynamicBacktrack backtrackW(m_g,m_dfi,flags);

	// consider all different kuratowski structures
	for (const KuratowskiStructure& k : allKuratowskis) {
		// create pertinent and external flags
		for (edge s : k.pertinentSubgraph) {
			flags[s] |= DynamicBacktrack::KuratowskiFlag::pertinentPath;
		}
		for (edge s : k.externalSubgraph) {
			flags[s] |= DynamicBacktrack::KuratowskiFlag::externalPath;
		}

		// compute all possible external paths of stopX and stopY (pathX,pathY)
		bool firstXPath = true;
		backtrackX.init(k.stopX,k.V,true, static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
						static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),nullptr,nullptr);
		while (backtrackX.addNextPath(pathX,endnodeX)) {
			bool firstYPath = true;
			backtrackY.init(k.stopY,k.V,true, static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),
							static_cast<int>(DynamicBacktrack::KuratowskiFlag::externalPath),nullptr,nullptr);
			while (backtrackY.addNextPath(pathY,endnodeY)) {

				// if minor A occurs, other minortypes are possible with adding
				// the dfs-path from node RReal to V
				if (k.RReal != k.V) addDFSPath(pathY,k.RReal,k.V);

				// consider all possible wNodes
				ArrayBuffer<adjEntry>* oldHighestXYPath = nullptr;
				for (const WInfo& info : k.wNodes) {
					// compute all possible internal paths of this wNode
					bool firstWPath = true; // avoid multiple identical subdivisions in E2
					backtrackW.init(info.w,k.V,false, static_cast<int>(DynamicBacktrack::KuratowskiFlag::pertinentPath),
									static_cast<int>(DynamicBacktrack::KuratowskiFlag::pertinentPath),nullptr,nullptr);
					node dummy;
					while (backtrackW.addNextPath(pathW,dummy)) {
						OGDF_ASSERT(!pathX.empty());
						OGDF_ASSERT(!pathY.empty());
						OGDF_ASSERT(!pathW.empty());

						// extract minor A
						if (info.minorType & WInfo::MinorType::A)
							extractMinorA(output,k,/*info,*/pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor B
						if (info.minorType & WInfo::MinorType::B)
							extractMinorBBundles(output,m_wasHere,++m_nodeMarker,k,flags,
										info,pathX,endnodeX,pathY,endnodeY,pathW);

						// extract minor C
						if (info.minorType & WInfo::MinorType::C)
							extractMinorC(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor D
						if (info.minorType & WInfo::MinorType::D)
							extractMinorD(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor E including all subtypes
						if (info.minorType & WInfo::MinorType::E) {
							extractMinorEBundles(output,firstXPath,firstYPath,firstWPath,
										oldHighestXYPath!=info.highestXYPath,
										m_wasHere,++m_nodeMarker,k,flags,info,
										pathX,endnodeX,pathY,endnodeY,pathW);
						}

						if (m_embeddingGrade > BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited &&
									output.size() >= m_embeddingGrade)
							return;
						firstWPath = false;
						// break;
					}
					oldHighestXYPath = info.highestXYPath;
				}
				firstYPath = false;
				// break;
			}
			firstXPath = false;
			// break;
		}

		// delete pertinent and external flags
		for (edge s : k.pertinentSubgraph) {
			flags[s] = 0;
		}
		for (edge s : k.externalSubgraph) {
			flags[s] = 0;
		}
	}
}


}
