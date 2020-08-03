/** \file
 * \brief implementation of the class BoyerMyrvoldPlanar
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


#include <ogdf/planarity/boyer_myrvold/BoyerMyrvoldPlanar.h>
#include <ogdf/planarity/boyer_myrvold/BoyerMyrvoldInit.h>
#include <ogdf/planarity/boyer_myrvold/FindKuratowskis.h>


namespace ogdf {

const int BoyerMyrvoldPlanar::DirectionCCW = 0;
const int BoyerMyrvoldPlanar::DirectionCW  = 1;

// constructor
BoyerMyrvoldPlanar::BoyerMyrvoldPlanar(
	Graph& g,
	bool bundles,
	int embeddingGrade, // see enumeration EmbeddingGrade for options
	bool limitStructures, // limits number of structures to embeddingGrade
	SListPure<KuratowskiStructure>& output,
	double randomness, // creates a random DFS-Tree, if in [0,1[
	                   // a value of 1 will always chose the edges with highest cost not be deleted
	bool avoidE2Minors, // avoids multiple identical minors (type AE2/E2)
	bool extractSubgraph, // will extract planar subgraph
	const EdgeArray<int> *edgeCosts) // costs for removing each edge
:
m_g(g),
	m_bundles(bundles),
	m_embeddingGrade(embeddingGrade),
	m_limitStructures(limitStructures),
	m_randomness(randomness),
	m_avoidE2Minors(avoidE2Minors),
	m_edgeCosts(edgeCosts),
	m_extractSubgraph(extractSubgraph),

	// BoyerMyrvoldInit members
	m_realVertex(g,nullptr),
	m_dfi(g,0),
	m_nodeFromDFI(-g.numberOfNodes(),g.numberOfNodes(),nullptr),
	m_adjParent(g,nullptr),
	m_leastAncestor(g), // doesn't need initialization
	m_edgeType(g,BoyerMyrvoldEdgeType::Undefined),
	m_lowPoint(g), // doesn't need initialization
	m_separatedDFSChildList(g),
	m_pNodeInParent(g), // doesn't need initialization

	// Walkup & Walkdown members
	m_visited(g,0),
	m_flipped(g,false),
	m_backedgeFlags(g),
	m_pertinentRoots(g),
	m_output(output)
{
	m_rand.seed(rand());
	m_link[BoyerMyrvoldPlanar::DirectionCCW].init(g,nullptr);
	m_link[BoyerMyrvoldPlanar::DirectionCW].init(g,nullptr);
	m_beforeSCE[BoyerMyrvoldPlanar::DirectionCCW].init(g,nullptr);
	m_beforeSCE[BoyerMyrvoldPlanar::DirectionCW].init(g,nullptr);
	m_output.clear();
	// apply this only, if FIND-procedure will be called
	if (m_embeddingGrade > EmbeddingGrade::doNotFind) {
		m_pointsToRoot.init(g,nullptr);
		m_visitedWithBackedge.init(g,nullptr);
		m_numUnembeddedBackedgesInBicomp.init(g,0);
		m_highestSubtreeDFI.init(g); // doesn't need initialization
	}
	m_flippedNodes = 0;
}


// walk upon external face in the given direction, see getSucessorOnExternalFace
// the difference is, that all inactive vertices are skipped, i.e. the returned node
// is active in relation to the node with dfi v
// in the special case of degree-one nodes the direction is not changed
// info returns the dynamic nodetype of the endnode
node BoyerMyrvoldPlanar::activeSuccessor(node w, int& direction, int v, int& info) const
{
	OGDF_ASSERT(w!=nullptr);
	OGDF_ASSERT(w->degree()>0);
	OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCW][w]!=nullptr);
	OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCCW][w]!=nullptr);
	node next;
	adjEntry adj;

	do {
		adj = m_link[direction][w];
		next = adj->theNode();
		OGDF_ASSERT(next!=nullptr);
		OGDF_ASSERT(next->degree()>0);
		OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCW][next]!=nullptr);
		OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCCW][next]!=nullptr);

		if (w->degree() > 1)
			direction = adj==beforeShortCircuitEdge(next,BoyerMyrvoldPlanar::DirectionCCW)->twin();
		w=next;
		info = infoAboutNode(next,v);

	} while (info==0); // until not inactive
	return next;
}

// merges adjEntries of virtual node w and associated real vertex x according to
// given outgoing directions x_dir and w_dir.
void BoyerMyrvoldPlanar::mergeBiconnectedComponent(ArrayBuffer<int>& stack) {
	bool doEmbed = m_embeddingGrade != EmbeddingGrade::doNotEmbed;

	const int w_dir = stack.popRet(); // outgoing direction of w
	const int x_dir = stack.popRet(); // outgoing direction of x
	int tmp = stack.popRet();
	const node w = m_nodeFromDFI[tmp]; // virtual DFS-Successor of x
	const node w_child = m_nodeFromDFI[-tmp]; // real unique DFS-Child of bicomp with root w
	const node x = m_realVertex[w];

	// set new external face neighbors and save adjEntry, where edges will be merged
	adjEntry mergeEntry = nullptr;
	Direction dir;
	if (doEmbed) {
		dir = (x_dir == BoyerMyrvoldPlanar::DirectionCCW) ? Direction::before : Direction::after;
		mergeEntry = beforeShortCircuitEdge(x, !x_dir)->twin();
	}
	m_link[!x_dir][x] = m_link[!w_dir][w];
	m_beforeSCE[!x_dir][x] = m_beforeSCE[!w_dir][w];

	// merge real and virtual nodes, flip biconnected component root if neccesary
	// not neccesary if onlyPlanar
	OGDF_ASSERT(!m_flipped[w_child]);
	adjEntry adj = w->firstAdj();
	if (doEmbed) {
		if (x_dir == w_dir) {
			// if not flipped
			if (dir == Direction::after) {
				mergeEntry = mergeEntry->cyclicSucc();
				dir = Direction::before;
			}
		} else {
			// if flipped:
			// set unique DFS-child of associated bicomp root node to "flipped"
			m_flipped[w_child] = true;
			++m_flippedNodes;
			if (dir == Direction::before) {
				mergeEntry = mergeEntry->cyclicPred();
				dir = Direction::after;
			}
		}
	}

	// merge adjEntries
	adjEntry temp;
	while (adj != nullptr) {
		temp = adj->succ();
		edge e = adj->theEdge();
		OGDF_ASSERT(e->source() != x);
		OGDF_ASSERT(e->target() != x);
		// this allows also self-loops when moving adjacency entries
		if (e->source() == w) {
			if (!doEmbed) { m_g.moveSource(e, x); }
			else { m_g.moveSource(e, mergeEntry, dir); }
		} else {
			if (!doEmbed) { m_g.moveTarget(e, x); }
			else { m_g.moveTarget(e, mergeEntry, dir); }
		}
		adj = temp;
	}

	// remove w from pertinent roots of x
	OGDF_ASSERT( !doEmbed || !m_pertinentRoots[x].empty() );
	OGDF_ASSERT( m_pertinentRoots[x].front() == w );
	m_pertinentRoots[x].popFront();

	// consider x's unique dfs-successor in pertinent bicomp:
	// remove this successor from separatedChildList of x using
	// saved pointer pNodeInParent in constant time
	OGDF_ASSERT( !doEmbed || !m_separatedDFSChildList[x].empty() );
	OGDF_ASSERT( m_pNodeInParent[w_child].valid() );
	m_separatedDFSChildList[x].del(m_pNodeInParent[w_child]);

	// delete virtual vertex, it must not contain any edges any more
	OGDF_ASSERT( w->firstAdj() == nullptr );
	m_nodeFromDFI[m_dfi[w]] = nullptr;
	m_g.delNode(w);
}

void BoyerMyrvoldPlanar::embedBackedges(
	const node v,
	const int v_dir,
	const node w,
	const int w_dir)
{
	OGDF_ASSERT( v != nullptr );
	OGDF_ASSERT( w != nullptr );
	OGDF_ASSERT( !m_backedgeFlags[w].empty() );
	OGDF_ASSERT( m_link[BoyerMyrvoldPlanar::DirectionCCW][v] != nullptr );
	OGDF_ASSERT( m_link[BoyerMyrvoldPlanar::DirectionCW][v] != nullptr );
	OGDF_ASSERT( m_link[BoyerMyrvoldPlanar::DirectionCCW][w] != nullptr );
	OGDF_ASSERT( m_link[BoyerMyrvoldPlanar::DirectionCW][w] != nullptr );

	bool doEmbed = m_embeddingGrade != EmbeddingGrade::doNotEmbed;

	adjEntry mergeEntryV, mergeEntryW;
	Direction insertv, insertw;
	if (doEmbed) {
		// if one edge is a short circuit edge, compute the former underlying adjEntry
		// the adjEntry of v, used for inserting backedges
		mergeEntryV = beforeShortCircuitEdge(v, v_dir)->twin();
		insertv = (v_dir == BoyerMyrvoldPlanar::DirectionCCW) ? Direction::after : Direction::before;
		// the adjEntry of w, used for inserting backedges
		mergeEntryW = beforeShortCircuitEdge(w, !w_dir)->twin();
		insertw = (w_dir == BoyerMyrvoldPlanar::DirectionCCW) ? Direction::before : Direction::after;

	}

	// the first/last (last iff !doEmbed) backedge in the backedgeFlags-list will be the new external face adjEntry
	adjEntry saveBack = doEmbed ? m_backedgeFlags[w].front() : m_backedgeFlags[w].back();
	for (adjEntry adj: m_backedgeFlags[w]) {
		// embed backedge
		edge e = adj->theEdge();
		OGDF_ASSERT( e->isIncident(w) );

		// insert backedge to v (and backedge to w iff doEmbed)
		if (e->source() == w) {
			if (doEmbed) {
				m_g.moveTarget(e, mergeEntryV, insertv);
				m_g.moveSource(e, mergeEntryW, insertw);
			} else {
				m_g.moveTarget(e, v);
			}
		} else {
			if (doEmbed) {
				m_g.moveSource(e, mergeEntryV, insertv);
				m_g.moveTarget(e, mergeEntryW, insertw);
			} else {
				m_g.moveSource(e, v);
			}
		}
	}

	// set external face link for this backedge and delete out-dated short
	// circuit links
	m_link[v_dir][v] = saveBack->twin();
	m_beforeSCE[v_dir][v] = nullptr;
	m_link[!w_dir][w] = saveBack;
	m_beforeSCE[!w_dir][w] = nullptr;

	// decrease counter of backedges per bicomp
	if (m_embeddingGrade > EmbeddingGrade::doNotFind) {
		node bicompRoot = m_pointsToRoot[m_backedgeFlags[w].front()->theEdge()];
		m_numUnembeddedBackedgesInBicomp[bicompRoot] -= m_backedgeFlags[w].size();
		OGDF_ASSERT( m_extractSubgraph || m_numUnembeddedBackedgesInBicomp[bicompRoot] >= 0 );
	}

	// delete BackedgeFlags
	m_backedgeFlags[w].clear();
}


// create short circuit edge from node v with direction v_dir to node w with outgoing
// direction w_dir.
void BoyerMyrvoldPlanar::createShortCircuitEdge(
	const node v,
	const int v_dir,
	const node w,
	const int w_dir)
{
	// save former neighbors
	if (m_beforeSCE[v_dir][v]==nullptr) m_beforeSCE[v_dir][v]=m_link[v_dir][v];
	if (m_beforeSCE[!w_dir][w]==nullptr) m_beforeSCE[!w_dir][w]=m_link[!w_dir][w];
	// set new short circuit edge
	adjEntry temp = m_beforeSCE[!w_dir][w]->twin();
	m_link[!w_dir][w] = m_beforeSCE[v_dir][v]->twin();
	m_link[v_dir][v] = temp;
}


// Walkup
// finds pertinent subgraph for descendant w of v.
// marks visited nodes with marker and returns the last traversed node.
node BoyerMyrvoldPlanar::walkup(
	const node v,
	const node w,
	const int marker,
	const edge back)
{
	const int i = m_dfi[v];
	node x = w;
	node y = w;
	node temp;
	int x_dir = BoyerMyrvoldPlanar::DirectionCW;
	int y_dir = BoyerMyrvoldPlanar::DirectionCCW;

	while (m_visited[x]!=marker && m_visited[y]!=marker)
	{
		m_visited[x] = marker;
		m_visited[y] = marker;
		if (m_embeddingGrade > EmbeddingGrade::doNotFind) {
			m_visitedWithBackedge[x] = back;
			m_visitedWithBackedge[y] = back;
		}

		// is x or y root vertex?
		if (m_realVertex[x] != nullptr) {
			temp=x;
		} else if (m_realVertex[y] != nullptr) {
			temp=y;
		} else temp=nullptr;

		if (temp != nullptr) {
			// put pertinent root into the list of its non-virtual vertex.
			// the insert-position is either front or back of the list, this
			// depends on the external activity of the pertinent root's
			// biconnected component.

			x = m_realVertex[temp];
			y = x;

			OGDF_ASSERT(m_extractSubgraph || m_visited[x]==marker || m_pertinentRoots[x].empty());
			// push pertinent root
			if (m_lowPoint[m_nodeFromDFI[-m_dfi[temp]]] < i) {
				m_pertinentRoots[x].pushBack(temp);
			} else m_pertinentRoots[x].pushFront(temp);
			// found v, finish walkup and return last traversed node
			if (x==v) {
				m_visited[x] = marker;
				return temp;
			}
		} else {
			// traverse to external face successors
			x = successorOnExternalFace(x,x_dir);
			y = successorOnExternalFace(y,y_dir);
		}
	}

	// return last traversed node
	return (m_visited[x] == marker) ? x : y;
}


// Walkdown
// for DFS-child w of the current processed vertex v': embed all backedges
// to the virtual node v of v'
// returns 1, iff the embedding process found a stopping configuration
int BoyerMyrvoldPlanar::walkdown(
	const int i, // dfi of rootvertex v'
	const node v, // v is virtual node of v'
	FindKuratowskis *findKuratowskis)
{
	ArrayBuffer<int> stack;
	node stopX = nullptr;

	bool stoppingNodesFound = 0; // 0=false,1=true,2=break

	// in both directions
	// j=current outgoing direction of current embedded node v
	for (int j: {BoyerMyrvoldPlanar::DirectionCCW, BoyerMyrvoldPlanar::DirectionCW}) {
		int w_dir = j; // direction of traversal of node w

		node w = successorOnExternalFace(v,w_dir); // current node

		while (w != v) {
			// assert, that CCW[] and CW[] return that adjEntry of the neighbor
			OGDF_ASSERT(beforeShortCircuitEdge(w,w_dir)->twinNode()==w);

			// if backedgeFlag is set
			if (!m_backedgeFlags[w].empty()) {
				while (!stack.empty()) {
					mergeBiconnectedComponent(stack);
				}
				embedBackedges(v, j, w, w_dir);
			}

			// if pertinentRoots of w is not empty
			if (!m_pertinentRoots[w].empty()) {
				// append pertinent root of w and direction of entry in w to stack
				// y is root of pertinent child bicomp
				node root = m_pertinentRoots[w].front();

				if(m_extractSubgraph && root->degree() == 0) {
					m_pertinentRoots[w].popFront();
					continue;
				}

				stack.push(m_dfi[root]);

				// append outgoing direction of entry in w to stack
				OGDF_ASSERT(w->degree() > 0);
				stack.push(w_dir);

				// get active successor in pertinent bicomp
				// variables for recognizing the right direction after descending to a bicomp
				int x_dir = BoyerMyrvoldPlanar::DirectionCCW;
				int y_dir = BoyerMyrvoldPlanar::DirectionCW;
				int infoX, infoY; // gives information about the type of endnode in that direction
				node x = activeSuccessor(root,x_dir,i,infoX);
				node y = activeSuccessor(root,y_dir,i,infoY);

				OGDF_ASSERT(x != root);
				OGDF_ASSERT(y != root);
				createShortCircuitEdge(root,BoyerMyrvoldPlanar::DirectionCCW,x,x_dir);
				createShortCircuitEdge(root,BoyerMyrvoldPlanar::DirectionCW,y,y_dir);

				// push counterclockwise resp. clockwise active successor
				// in pertinent bicomp
				if (infoX == infoY) {
					// if both attributes are externally active and non-pertinent,
					// save stopping nodes
					if (infoX==3) {
						if(!m_extractSubgraph) {
							OGDF_ASSERT(x != y);
							if (m_embeddingGrade <= EmbeddingGrade::doNotFind) return true;
						}

						// extract Kuratowskis
						stoppingNodesFound = 1;
						if(!m_extractSubgraph) {
							// check, if we have found enough kuratowski structures
							if (m_embeddingGrade > 0 &&
									findKuratowskis->getAllKuratowskis().size() >= m_embeddingGrade) {
								return 2;
							}
							findKuratowskis->addKuratowskiStructure(m_nodeFromDFI[i],root,x,y);
						}

						// go to the pertinent starting node on father bicomp
						stack.pop(); // delete new w_dir from stack
						w = m_realVertex[m_nodeFromDFI[stack.popRet()]]; // x itself
						// refresh pertinentRoots information
						m_pertinentRoots[w].popFront();

						// if more pertinent child bicomps exist on the same root,
						// let the walkdown either embed it or find a new kuratowski structure
						while (!stack.empty() && !pertinent(w)) {
							// last real root
							node lastActiveNode = w;

							// not in V-bicomp:
							// go to the unvisited active node on father bicomp
							w_dir = stack.popRet(); // outgoing direction of w
							x_dir = stack.popRet(); // outgoing direction of x
							w = m_nodeFromDFI[stack.top()]; // w, virtual node

							node otherActiveNode = m_link[!w_dir][w]->theNode();

							OGDF_ASSERT(otherActiveNode == constActiveSuccessor(w,!w_dir,i,infoX));
							OGDF_ASSERT(externallyActive(otherActiveNode,i));
							OGDF_ASSERT(lastActiveNode == m_link[w_dir][w]->theNode());
							if (pertinent(otherActiveNode)) {
								// push adapted information about actual bicomp in stack
								stack.push(x_dir);
								stack.push(!w_dir);
								// go on with walkdown on unvisited active node
								w_dir = !w_dir;
								break;
							} else {
								// delete old root
								stack.pop();
								// if there are two stopping vertices, that are not pertinent
								// there could be another kuratowski structure
								if (!m_extractSubgraph && lastActiveNode != otherActiveNode &&
										wNodesExist(w,lastActiveNode,otherActiveNode)) {
									// check, if we have found enough kuratowski structures
									if (m_embeddingGrade > 0 &&
											findKuratowskis->getAllKuratowskis().size() >= m_embeddingGrade) {
										return 2;
									}
									// different stopping nodes:
									// try to extract kuratowski structure and put the two
									// stopping nodes in the right traversal order
									if (w_dir==BoyerMyrvoldPlanar::DirectionCCW) {
										findKuratowskis->addKuratowskiStructure(m_nodeFromDFI[i],
														w,lastActiveNode,otherActiveNode);
									} else {
										findKuratowskis->addKuratowskiStructure(m_nodeFromDFI[i],
														w,otherActiveNode,lastActiveNode);
									}
								}

								// refresh pertinentRoots information
								w = m_realVertex[w]; // x
								m_pertinentRoots[w].popFront();
								w_dir = x_dir;
							}
						}
					}
					// if both attributes are the same: minimize flips
					else if (w_dir==BoyerMyrvoldPlanar::DirectionCCW) {
						w = x;
						w_dir = x_dir;
						stack.push(BoyerMyrvoldPlanar::DirectionCCW);

					} else {
						w = y;
						w_dir = y_dir;
						stack.push(BoyerMyrvoldPlanar::DirectionCW);
					}
				} else if (infoX <= infoY) {
					// push x
					w=x; w_dir=x_dir;
					stack.push(BoyerMyrvoldPlanar::DirectionCCW);

				} else {
					// push y
					w=y; w_dir=y_dir;
					stack.push(BoyerMyrvoldPlanar::DirectionCW);
				}

			} else if (inactive(w,i)) {
				// w is an inactive vertex
				w = successorOnExternalFace(w,w_dir);

			} else {
				// w must be a stopping vertex
				OGDF_ASSERT(externallyActive(w,i));
				OGDF_ASSERT(m_lowPoint[m_nodeFromDFI[-m_dfi[v]]] < i);

				// embed shortCircuitEdge
				/*if (stack.empty())*/ createShortCircuitEdge(v,j,w,w_dir);

				// only save single stopping nodes, if we don't have already one
				if (j==BoyerMyrvoldPlanar::DirectionCCW) {
					stopX = w;
				} else if (w != stopX) {
					OGDF_ASSERT(stopX!=nullptr);

					if (m_embeddingGrade <= EmbeddingGrade::doNotFind) return false;
					// check, if some backedges were not embedded (=> nonplanar)
					// note, that this is performed at most one time per virtual root
					if (m_numUnembeddedBackedgesInBicomp[v] > 0) {
						// some backedges are left on this bicomp
						stoppingNodesFound = 1;
						// check, if we have found enough kuratowski structures
						if(!m_extractSubgraph) {
							if (m_embeddingGrade > 0 &&
								findKuratowskis->getAllKuratowskis().size() >= m_embeddingGrade) {
								return 2;
							}
							findKuratowskis->addKuratowskiStructure(m_nodeFromDFI[i], v, stopX, w);
						}
					}
				}
				break;
			}
		}

		stack.clear();
	}

	return stoppingNodesFound;
}

// embed graph m_g node by node in descending DFI-order beginning with dfi i
bool BoyerMyrvoldPlanar::embed()
{
	bool nonplanar=false; // true, if graph is not planar

#if 0
	FindKuratowskis findKuratowskis(this);
#else
	FindKuratowskis* findKuratowskis =
		(m_embeddingGrade <= EmbeddingGrade::doNotFind) ? nullptr : new FindKuratowskis(this);
#endif

	for (int i = m_nodeFromDFI.high(); i >= 1; --i)
	{
		const node v = m_nodeFromDFI[i];

		// call Walkup
		// for all sources of backedges of v: find pertinent subgraph

		for(adjEntry adj : v->adjEntries) {
			node w = adj->twinNode(); // dfs-descendant of v
			edge e = adj->theEdge();
			if (m_dfi[w] > i && m_edgeType[e] == BoyerMyrvoldEdgeType::Back) {
				m_backedgeFlags[w].pushBack(adj);

				node x = walkup(v,w,i,e);
				if (m_embeddingGrade <= EmbeddingGrade::doNotFind) continue;

				// divide children bicomps
				if (m_realVertex[x] == v) {
					// x is a (virtual) root vertex.
					m_pointsToRoot[e] = x;
					OGDF_ASSERT(m_numUnembeddedBackedgesInBicomp[x] == 0);
				} else {
					// Set x to the (virtual) root of its bicomp.
					x = m_pointsToRoot[m_visitedWithBackedge[x]];
					m_pointsToRoot[e] = x;
					OGDF_ASSERT(m_numUnembeddedBackedgesInBicomp[x] >= 1);
				}
				// Increase the number of backedges leading to x's child bicomp.
				++m_numUnembeddedBackedgesInBicomp[x];
			}
		}

		// call Walkdown
		// for every pertinent subtrees with children w of v as roots
		// embed all backedges to v
		SListPure<node>& pert(m_pertinentRoots[v]);
		while (!pert.empty()) {
			OGDF_ASSERT(pert.front()->degree()==1);
			int result = walkdown(i,pert.popFrontRet(),findKuratowskis);
			if (!m_extractSubgraph) {
				if(result == 2) {
					m_output = findKuratowskis->getAllKuratowskis();
					delete findKuratowskis;
					return false;
				} else if (result == 1) {
					// found stopping configuration
					nonplanar = true;
					if (m_embeddingGrade <= EmbeddingGrade::doNotFind) return false;
				}
			}
		}

		// if !embed, check, if there are any backedges left
		if (!m_extractSubgraph && m_embeddingGrade <= EmbeddingGrade::doNotFind) {
			for(adjEntry adj : v->adjEntries) {
				if (m_edgeType[adj->theEdge()] == BoyerMyrvoldEdgeType::Back &&
						m_dfi[adj->twinNode()] > m_dfi[v])
				{
					delete findKuratowskis;
					return false; // nonplanar
				}
			}
		}
	}

	// embed and flip bicomps, if necessary
	if (nonplanar) {
		if(findKuratowskis)
			m_output = findKuratowskis->getAllKuratowskis();
	} else
		postProcessEmbedding(); // flip graph and embed self-loops, etc.

	delete findKuratowskis;
	return !nonplanar;
}


// merge unprocessed virtual nodes such as the dfs-roots
void BoyerMyrvoldPlanar::mergeUnprocessedNodes()
{
	node v = m_g.firstNode();
	while (v) {
		node next = v->succ();
		if (m_dfi[v] < 0) {
			node w = m_realVertex[v];
			adjEntry adj = v->firstAdj();
			// copy all adjEntries to non-virtual node
			while (adj) {
				edge e = adj->theEdge();
				adj = adj->succ();
				if (e->source()==v) {
					m_g.moveSource(e,w);
				} else m_g.moveTarget(e,w);
			}
			m_nodeFromDFI[m_dfi[v]]=nullptr;
			m_g.delNode(v);
		}
		v = next;
	}
}


// flips all nodes of the bicomp with unique real root-child c as necessary.
// in addition all connected components with dfs-root c without virtual
// nodes are allowed. this function can be used to reverse the flip, too!
// marker has to be an non-existing int in array visited.
// if wholeGraph ist true, all bicomps of all connected components will be traversed.
// if deleteFlipFlags ist true, the flipping flags will be deleted after flipping
void BoyerMyrvoldPlanar::flipBicomp(
	int c,
	int marker,
	NodeArray<int>& visited,
	bool wholeGraph,
	bool deleteFlipFlags)
{
	if (m_flippedNodes == 0) {
		if (wholeGraph) mergeUnprocessedNodes();
		return;
	}

	ArrayBuffer<int> stack; // stack for dfs-traversal

	if (wholeGraph) {
		mergeUnprocessedNodes();
		for (int i = 1; i <= m_g.numberOfNodes(); ++i)
			stack.push(-i);
	}

	// flip bicomps, if the flipped-flag is set
	bool flip;
	stack.push(-c); // negative numbers: flip=false, otherwise flip=true
	while (!stack.empty()) {
		int stackTop = stack.popRet();
		node v;
		if (stackTop < 0) {
			flip = false;
			v = m_nodeFromDFI[-stackTop];
		} else {
			flip = true;
			v = m_nodeFromDFI[stackTop];
		}
		if (wholeGraph) {
			if (visited[v]==marker) continue;
			// mark visited nodes
			visited[v] = marker;
		}

		// flip adjEntries of node, if necessary
		if (m_flipped[v]) {
			flip = !flip;

			// don't do this, if all flips on nodes of this bicomp will be reversed
			if (deleteFlipFlags) {
				m_flipped[v] = false;
				--m_flippedNodes;
				OGDF_ASSERT(m_flippedNodes >= 0);
			}
		}
		if (flip) {
			m_g.reverseAdjEdges(v);
			if (deleteFlipFlags) {
				adjEntry tmp = m_link[BoyerMyrvoldPlanar::DirectionCCW][v];
				m_link[BoyerMyrvoldPlanar::DirectionCCW][v] = m_link[BoyerMyrvoldPlanar::DirectionCW][v];
				m_link[BoyerMyrvoldPlanar::DirectionCW][v] = tmp;

				tmp = m_beforeSCE[BoyerMyrvoldPlanar::DirectionCCW][v];
				m_beforeSCE[BoyerMyrvoldPlanar::DirectionCCW][v] = m_beforeSCE[BoyerMyrvoldPlanar::DirectionCW][v];
				m_beforeSCE[BoyerMyrvoldPlanar::DirectionCW][v] = tmp;
			}
		}

		// go along the dfs-edges
		for(adjEntry adj : v->adjEntries) {
			int temp = m_dfi[adj->twinNode()];
			OGDF_ASSERT(m_edgeType[adj->theEdge()] != BoyerMyrvoldEdgeType::Undefined);
			if (temp > m_dfi[v] && m_edgeType[adj->theEdge()]==BoyerMyrvoldEdgeType::Dfs) {
				stack.push(flip ? temp : -temp);
			}
		}
	}
}


// postprocess the embedding, so that all unprocessed virtual vertices are
// merged with their non-virtual counterpart. Furthermore all bicomps
// are flipped, if necessary and parallel edges and self-loops are embedded.
void BoyerMyrvoldPlanar::postProcessEmbedding()
{
	ArrayBuffer<int> stack; // stack for dfs-traversal
	node v,w;
	adjEntry adj;
	int temp;

	mergeUnprocessedNodes();

	// flip bicomps, if the flipped-flag is set, i.e. postprocessing in
	// reverse dfi-order
	bool flip;
	for(int i=1; i<=m_g.numberOfNodes(); ++i) {
		if (m_visited[m_nodeFromDFI[i]] == -1) continue;
		stack.push(-i); // negative numbers: flip=false, otherwise flip=true

		while (!stack.empty()) {
			temp = stack.popRet();
			if (temp < 0) {
				flip=false;
				v = m_nodeFromDFI[-temp];
			} else {
				flip=true;
				v = m_nodeFromDFI[temp];
			}
			if (m_visited[v]==-1) continue;
			// mark visited nodes with visited[v]==-1
			m_visited[v] = -1;

			// flip adjEntries of node, if necessary
			if (m_flipped[v]) {
				m_flipped[v] = false;
				flip = !flip;
			}
			if (flip) m_g.reverseAdjEdges(v);

			adj=v->firstAdj();
			while (adj) {
				w = adj->twinNode();
				BoyerMyrvoldEdgeType tempType = m_edgeType[adj->theEdge()];
				if (tempType==BoyerMyrvoldEdgeType::Dfs) {
					// go along the dfs-edges
					stack.push(flip ? m_dfi[w] : -m_dfi[w]);
					adj=adj->succ();
				} else if (tempType==BoyerMyrvoldEdgeType::Selfloop) {
					// embed self-loops
					m_g.moveAdjBefore(adj->twin(),adj);
					adj=adj->succ();
				} else if (tempType==BoyerMyrvoldEdgeType::DfsParallel &&
							m_adjParent[v]!=nullptr &&
							w == m_adjParent[v]->theNode()) {
					// embed edges that are parallel to dfs-edges
					// it is only possible to deal with the parallel edges to the
					// parent, since children nodes could be flipped later
					adjEntry tmp = adj->succ();
					m_g.moveAdjAfter(adj,m_adjParent[v]->twin());
					m_g.moveAdjBefore(adj->twin(),m_adjParent[v]);
					adj = tmp;
				} else adj=adj->succ();
			}
		}
	}
}


// tests Graph m_g for planarity
// if graph should be embedded, a planar embedding or a kuratowski subdivision
// of m_g is returned in addition, depending on whether m_g is planar
bool BoyerMyrvoldPlanar::start()
{
	boyer_myrvold::BoyerMyrvoldInit bmi(this);
	bmi.computeDFS();
	bmi.computeLowPoints();
	bmi.computeDFSChildLists();

	// call the embedding procedure
	return embed();
}


}
