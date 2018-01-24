/** \file
 * \brief Implementation of Block and BlockOrder classes
 *
 * \author Paweł Schmidt, Carsten Gutwenger
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

#include <ogdf/layered/BlockOrder.h>

namespace ogdf {

BlockOrder::BlockOrder(Hierarchy& hierarchy, bool longEdgesOnly)
  : m_GC(hierarchy.operator const ogdf::GraphCopy &().original())
  , m_ranks(m_GC,0)
  , m_storedPerm()
  , m_currentPerm()
  , m_bestPerm()
  , m_currentPermInv()
  , m_storedCrossings(std::numeric_limits<int>::max())
  , m_bestCrossings(std::numeric_limits<int>::max())
  , m_Blocks()
  , m_NodeBlocks(m_GC,nullptr)
  , m_EdgeBlocks(m_GC,nullptr)
  , m_isActiveEdge(m_GC,false)
  , m_activeBlocksCount(0)
  , m_hierarchy(hierarchy)
  , m_levels(0,-1,nullptr)
  , m_verticalStepsBound(0)
{
	doInit(longEdgesOnly);
}


void BlockOrder::doInit(bool longEdgesOnly)
{
	const GraphCopy &GC = m_hierarchy;
	NodeArray<bool> nodesInCC(m_GC,false);

	unsigned int countBlocks = 0;

	int minLvl = 0;
	int maxLvl = m_hierarchy.maxRank();

	m_nNodesOnLvls.init(minLvl, maxLvl, 0);

	// one block for every node
	for(node v : GC.nodes) {
		//m_GC may contain nodes from another CC
		if (GC.original(v) != nullptr) {
			m_ranks[GC.original(v)] = m_hierarchy.rank(v);
			nodesInCC[GC.original(v)] = true;
			++countBlocks;
			m_nNodesOnLvls[m_hierarchy.rank(v)] += 1;
		}
	}

	//one block for every long edge
	for(edge e : m_GC.edges) {
		node src = e->source();
		node tgt = e->target();

		if (nodesInCC[src] && nodesInCC[tgt]) {
			int top = min(m_ranks[src], m_ranks[tgt]);
			int bot = max(m_ranks[src], m_ranks[tgt]);
			if (top + 1 < bot || !longEdgesOnly) {
				++countBlocks;
			}
		}
	}

	//m_blocksCount = countBlocks;

	m_Blocks.init(countBlocks);
	m_storedPerm.init(countBlocks);
	m_bestPerm.init(countBlocks);
	m_currentPerm.init(countBlocks);
	m_currentPermInv.init(countBlocks);

	int i = 0;
	for(node v : GC.nodes) {
		node vOrig = GC.original(v);
		if (vOrig != nullptr) {
			m_Blocks[i] = m_NodeBlocks[vOrig] = new Block(vOrig);
			m_Blocks[i]->m_index = i;
			m_Blocks[i]->m_lower = m_Blocks[i]->m_upper = m_ranks[vOrig];
			++i;
			m_activeBlocksCount += 1;
		}
	}

	for(edge e : m_GC.edges) {
		node src = e->source();
		node tgt = e->target();

		if (nodesInCC[src] && nodesInCC[tgt]) {
			int top = min(m_ranks[src], m_ranks[tgt]);
			int bot = max(m_ranks[src], m_ranks[tgt]);

			if (top + 1 < bot || !longEdgesOnly) {
				m_Blocks[i] = m_EdgeBlocks[e] = new Block(e);
				m_Blocks[i]->m_index = i;
				m_Blocks[i]->m_upper = top + 1;
				m_Blocks[i]->m_lower = bot - 1;
				++i;
			}
			if (top + 1 < bot) {
				m_isActiveEdge[e] = true;
				m_activeBlocksCount += 1;
			} else
				m_isActiveEdge[e] = false;
		}
	}
}


Block::Block(edge e)
: m_NeighboursIncoming(1)
, m_InvertedIncoming(1)
, m_NeighboursOutgoing(1)
, m_InvertedOutgoing(1)
, m_Edge(e)
, m_isEdgeBlock(true)
, m_isNodeBlock(false)
{ }


Block::Block(node v)
: m_NeighboursIncoming(v->indeg())
, m_InvertedIncoming(v->indeg())
, m_NeighboursOutgoing(v->outdeg())
, m_InvertedOutgoing(v->outdeg())
, m_Node(v)
, m_isEdgeBlock(false)
, m_isNodeBlock(true)
{ }


void BlockOrder::sortAdjacencies()
{

	Block *processedBlock; // A in paper

	EdgeArray<int> p(m_GC, 0);  // array P in paper
	EdgeArray<int> longEdgeP(m_GC, 0); // additional P for long edges
	// explanation:
	// Bachmaier et al. in their paper convert long edge u ---> v to three blocks and two edges: u -> [block for edge] -> v.
	// I decided to use edges and vertices from original graph (thus I am able to use Node- and EdgeArrays).
	// It implies that (block for a long) edge u ---> v is traversed four times
	// (i.e. twice when blocks for vertices u and v are processed, and then twice, when block for this edge is processed)
	// and since that, some information can be overwritten and lost.
	// To avoid this, for any long edge u--->v , array p stores information about source vertex u, and longEdgeP stores information about target vertex v
	// Later an attempt to delete this array can be made since long edge block have exactly one neighbour incoming and one outgoing.

	//Array to store number of items inserted to arrays of neighbours in N+
	// These arrays store next free position in arrays N+ and  N-
	Array<int> NPlusItemsCount(0, m_Blocks.high(), 0);
	Array<int> NMinusItemsCount(0, m_Blocks.high(), 0);

	// foreach A in B
	for (int i = 0; i < m_activeBlocksCount; ++i) {
		processedBlock = m_Blocks[m_currentPermInv[i]];

		if (processedBlock->isVertexBlock()) {
			node v = processedBlock->m_Node;

			//foreach s in { (u,v) in E' | v=upper(A)}
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if (v == e->target()) {
					// if e is a short edge
					if (!m_isActiveEdge[e]) {

						node u = e->source();
						Block *blockOfU = m_NodeBlocks[u];

						// insert v at the next free position j of N+[u]
						int j = NPlusItemsCount[blockOfU->m_index];
						NPlusItemsCount [blockOfU->m_index] += 1;
						blockOfU->m_NeighboursOutgoing[j] = processedBlock->m_index;


						if (m_currentPerm[processedBlock->m_index] < m_currentPerm[blockOfU->m_index]) {
							//first traversal of e
							p[e] = j;
						} else {
							//second traversal of e
							blockOfU->m_InvertedOutgoing[j] = p[e];
							processedBlock->m_InvertedIncoming[p[e]] = j;
						}
					} else {
						// e is a long edge
						Block *blockOfU = m_EdgeBlocks[e];

						// insert v at the next free position j of N+[u]
						int j = NPlusItemsCount[blockOfU->m_index];
						NPlusItemsCount[blockOfU->m_index] += 1;
						blockOfU->m_NeighboursOutgoing[j] = processedBlock->m_index;

						// here we count "edges" outgoing from block u
						// thus we should use longEdgeP
						if (m_currentPerm[processedBlock->m_index] < m_currentPerm[blockOfU ->m_index]) {
							//first traversal of e
							longEdgeP[e] = j;
						} else {
							//second traversal of e
							blockOfU->m_InvertedOutgoing[j] = longEdgeP[e];
							processedBlock->m_InvertedIncoming[longEdgeP[e]] = j;
						}
					}
				}
			}

			node w = processedBlock->m_Node;
			// foreach s in { (w,x) in E' | w = lower(A) }
			for(adjEntry adj : w->adjEntries) {
				edge e = adj->theEdge();
				if (w == e->source()) {
					// if e is a short edge
					if (!m_isActiveEdge[e]) {

						node x = e->target();
						Block *blockOfX = m_NodeBlocks[x];

						// insert w at the next free position j of N-[x]
						int j = NMinusItemsCount[blockOfX->m_index];
						NMinusItemsCount[blockOfX->m_index] += 1;
						blockOfX->m_NeighboursIncoming[j] = processedBlock->m_index;

						if (m_currentPerm[processedBlock->m_index] < m_currentPerm[blockOfX->m_index]) {
							// first traversal of e
							p[e] = j;
						} else {
							//second traversal of e
							blockOfX->m_InvertedIncoming[j] = p[e];
							processedBlock->m_InvertedOutgoing [p[e]] = j;
						}
					} else {
						// e is a long edge
						Block *blockOfX = m_EdgeBlocks[e];

						// insert v at the next free position j of N-(x)
						int j = NMinusItemsCount[blockOfX->m_index];
						NMinusItemsCount[blockOfX->m_index] += 1;
						blockOfX->m_NeighboursIncoming[j] = processedBlock->m_index;

						// here we count "edges" incoming to edge block
						// thus we use p instead of LongEdgeP

						if (m_currentPerm[processedBlock->m_index] < m_currentPerm[blockOfX->m_index]) {
							//first traversal of e
							p[e] = j;
						} else {
							//second traversal of e
							blockOfX->m_InvertedIncoming[j] = p[e];
							processedBlock->m_InvertedOutgoing[p[e]] = j;
						}
					}
				}
			}
		}

		if (processedBlock->isEdgeBlock()) {

			edge e = processedBlock->m_Edge;

			node u = e->source();
			node x = e->target();

			Block *blockOfU = m_NodeBlocks[u];
			Block *blockOfX = m_NodeBlocks[x];


			// first foreach loop
			// edge incoming to block
			{
				// insert v at the next free position j of N+[u]
				int j = NPlusItemsCount[blockOfU->m_index];
				NPlusItemsCount[blockOfU->m_index] += 1;
				blockOfU->m_NeighboursOutgoing[j] = processedBlock->m_index;


				if (m_currentPerm[processedBlock->m_index] < m_currentPerm[blockOfU->m_index]) {
					// first traversal of e
					p[e] = j;
				} else {
					// second traversal of e
					blockOfU->m_InvertedOutgoing[j] = p[e];
					processedBlock->m_InvertedIncoming[p[e]] = j;
				}
			}

			// second foreach loop
			// edge outgoing from block
			{
				// insert w at the next free position j of N-[x]

				int j = NMinusItemsCount[blockOfX->m_index];
				NMinusItemsCount[blockOfX->m_index] += 1;
				blockOfX->m_NeighboursIncoming[j] = processedBlock->m_index ;

				if (m_currentPerm[processedBlock->m_index] < m_currentPerm[blockOfX->m_index]) {
					//first traversal of e
					longEdgeP[e] = j;
				} else {
					//second traversal of e
					blockOfX->m_InvertedIncoming[j] = longEdgeP[e];
					processedBlock->m_InvertedOutgoing[longEdgeP[e]] = j;
				}
			}
		}
	}
}

void BlockOrder::deconstruct()
{
	for (auto &elem : m_Blocks) {
		delete elem;
	}
	for (auto &elem : m_levels) {
		delete elem;
	}
}


void BlockOrder::updateAdjacencies(Block *blockOfA, Block *blockOfB, Direction d)
{
	Array<int> &NdA = (d == Direction::Minus) ? blockOfA->m_NeighboursIncoming
	                                          : blockOfA->m_NeighboursOutgoing;
	Array<int> &IdA = (d == Direction::Minus) ? blockOfA->m_InvertedIncoming
	                                          : blockOfA->m_InvertedOutgoing;
	Array<int> &NdB = (d == Direction::Minus) ? blockOfB->m_NeighboursIncoming
	                                          : blockOfB->m_NeighboursOutgoing;
	Array<int> &IdB = (d == Direction::Minus) ? blockOfB->m_InvertedIncoming
	                                          : blockOfB->m_InvertedOutgoing;

	int i = 0, j = 0;

	int r = NdA.size();
	int s = NdB.size();

	while (i < r && j < s) {
		// if pi (block(xi)) < pi(block (yj))
		if (m_currentPerm[NdA[i]] < m_currentPerm[NdB[j]])
			++i;
		else if (m_currentPerm[NdA[i]] > m_currentPerm[NdB[j]])
			++j;
		else {
			Block *z = m_Blocks[NdA[i]]; // = yj;
			if (d == Direction::Plus) {

				z->m_NeighboursIncoming.swap(IdA[i], IdB[j]);
				z->m_InvertedIncoming.swap(IdA[i], IdB[j]);
			} else {
				z->m_NeighboursOutgoing.swap(IdA[i], IdB[j]);
				z->m_InvertedOutgoing.swap(IdA[i], IdB[j]);
			}
			IdA[i] +=  1;
			IdB[j] += -1;
			++i; ++j;
		}
	}
}


int BlockOrder::uswap(Block *blockOfA, Block *blockOfB, Direction d, int level)
{
	Array<int> &NdA = (d == Direction::Minus) ? blockOfA->m_NeighboursIncoming
	                                          : blockOfA->m_NeighboursOutgoing;
	Array<int> &NdB = (d == Direction::Minus) ? blockOfB->m_NeighboursIncoming
	                                          : blockOfB->m_NeighboursOutgoing;

	int nextLevel;
	if (d == Direction::Minus)
		for (nextLevel = level - 1; m_nNodesOnLvls[nextLevel] == 0; --nextLevel);
	else
		for (nextLevel = level + 1; m_nNodesOnLvls[nextLevel] == 0; ++nextLevel);


	int c = 0;
	int i = 0;
	int j = 0;

	int r = NdA.size();
	int s = NdB.size();

	if ((d == Direction::Minus && nextLevel < blockOfA->m_upper && nextLevel < blockOfB->m_upper)
	 || (d == Direction::Plus  && nextLevel > blockOfA->m_lower && nextLevel > blockOfB->m_lower)) {
		while (i < r && j < s) {
			if (m_currentPerm[NdA[i]] < m_currentPerm[NdB[j]]) {
				c = c + (s - j);
				i += 1;
			} else if (m_currentPerm[NdA[i]] > m_currentPerm[NdB[j]]) {
				c = c - (r - i);
				j += 1;
			} else {
				c = c + (s - j) - (r - i);
				i += 1;
				j += 1;
			}
		}
	} else
	if ((d == Direction::Minus && nextLevel >= blockOfA->m_upper)
	 || (d == Direction::Plus  && nextLevel <= blockOfA->m_lower)) {
		// edge in block A is an internal edge
		int pi = m_currentPerm[blockOfA->m_index];
		for (j = 0; j < s && m_currentPerm[NdB[j]] < pi ;)
			++j;
		// j is the number of blocks on level level that are neighbours of b and are before a in current permutation
		c = s - 2 * j;
	} else {
		// edge in block B is an internal edge
		int pi = m_currentPerm[blockOfB->m_index];
		for (i = 0 ; i < r && m_currentPerm[NdA[i]] < pi;)
			++i;
		// j is the number of blocks on level level that are neighbours of a and are before b in current permutation
		c = 2 * i - s;
	}
	return c;
}


int BlockOrder::siftingSwap(Block *blockOfA, Block *blockOfB)
{
	int Delta = 0;

	if (blockOfA->m_upper > blockOfB->m_lower || blockOfA->m_lower < blockOfB->m_upper) {
		// empty intersection of A.levels and B.levels - no change in number of crossings
		Delta = 0;
	} else {
		// intersection of A.levels and B.levels is nonempty
		// top = max(upper(A),upper(B))
		int top = (blockOfA->m_upper > blockOfB->m_upper) ? blockOfA->m_upper
		                                                  : blockOfB->m_upper;
		// bottom = min(lower(A),lower(B))
		int bottom = (blockOfA->m_lower < blockOfB->m_lower) ? blockOfA->m_lower
		                                                     : blockOfB->m_lower;

		Delta += uswap(blockOfA, blockOfB, Direction::Minus, top);
		if (top == blockOfA->m_upper && top == blockOfB->m_upper)
			updateAdjacencies(blockOfA, blockOfB, Direction::Minus);

		Delta += uswap(blockOfA, blockOfB, Direction::Plus, bottom);
		if (bottom == blockOfA->m_lower && bottom == blockOfB->m_lower)
			updateAdjacencies(blockOfA, blockOfB, Direction::Plus);
	}

	// swap positions of A and B in permutation
	int c = m_currentPerm[blockOfA->m_index];
	int d = m_currentPerm[blockOfB->m_index];

	m_currentPermInv[c] = blockOfB->m_index;
	m_currentPermInv[d] = blockOfA->m_index;

	m_currentPerm[blockOfA->m_index] +=  1;
	m_currentPerm[blockOfB->m_index] += -1;

	return Delta;
}


int BlockOrder::siftingStep(Block *blockOfA)
{

	// new order with A put to front
	int positionOfA = m_storedPerm[blockOfA->m_index];
	for (int i = 0; i < m_storedPerm.size(); ++i) {
		if (m_storedPerm[i] < positionOfA && m_storedPerm[i] != -1)
			m_currentPerm[i] = m_storedPerm[i] + 1;
		else
			m_currentPerm[i] = m_storedPerm[i];
	}
	m_currentPerm[blockOfA->m_index] = 0;

	for (int i = 0; i < m_currentPerm.size(); ++i) {
		if (m_currentPerm[i] != -1)
			m_currentPermInv[m_currentPerm[i]] = i;
	}
	sortAdjacencies();

	int chi = 0;
	int bestChi = 0;
	int bestPos = 0;

	int oldChi = 0;

	for (int p = 1; p < m_activeBlocksCount; ++p) {
		chi = chi + siftingSwap(blockOfA, m_Blocks[m_currentPermInv[p]]);
		if (chi < bestChi) {
			bestChi = chi;
			bestPos = p;
		}
		if (p == positionOfA) {
			oldChi = chi;
		}
	}

	//return B'[1]< ... < B[bestPos] < A < B[bestPos+1] <...
	for (int i = 0; i < bestPos; ++i) {
		m_storedPerm[m_currentPermInv[i]] = i;
	}
	for (int i = bestPos; i < m_activeBlocksCount; ++i) {
		m_storedPerm[m_currentPermInv[i]] = i + 1;
	}
	m_storedPerm[blockOfA->m_index] = bestPos;

	return bestChi - oldChi;
}


void BlockOrder::globalSifting(int rho, int nRepeats, int *pNumCrossings)
{
	Array<int> storedPermInv(m_activeBlocksCount);
	int p = 0;

	for (auto &elem : m_storedPerm) {
		elem = -1;
	}

	for (int i = 0; i < m_Blocks.size(); ++i)
		if (m_Blocks[i]->isVertexBlock()
		 || (m_Blocks[i]->isEdgeBlock() && m_isActiveEdge[m_Blocks[i]->m_Edge])) {
			storedPermInv[p] = i;
			m_storedPerm[i] = p;
			++p;
		}
	m_bestCrossings = std::numeric_limits<int>().max();



	while (rho-- > 0) {
		storedPermInv.permute(0, m_activeBlocksCount - 1);

		for (int i = 0; i < m_activeBlocksCount; ++i)
			m_storedPerm[storedPermInv[i]] = i;

		int times = nRepeats;
		while (times-- > 0) {
			for (auto &block : m_Blocks) {
				if (block->isVertexBlock()
				 || (block->isEdgeBlock() && m_isActiveEdge[block->m_Edge])) {
					siftingStep(block);
				}
			}

			buildHierarchy();
			if (m_storedCrossings < m_bestCrossings) {
				for (int b = 0; b < m_bestPerm.size(); ++b) {
					m_bestPerm[b] = m_storedPerm[b];
				}
				m_bestCrossings = m_storedCrossings;
			}
		}
	}

	//restore the best permutation
	for (int i = 0; i < m_storedPerm.size(); ++i) {
		m_storedPerm[i] = m_bestPerm[i];
	}
	m_storedCrossings = m_bestCrossings;
	buildHierarchy();
	if(pNumCrossings)
		*pNumCrossings = m_storedCrossings;
}


void BlockOrder::buildDummyNodesLists()
{
	const GraphCopy &GC = m_hierarchy;
	NodeArray<bool> mark(GC, false);

	NodeArray<int> ranks(GC);

	for (auto b : m_Blocks) {
		if (b->isVertexBlock()) {
			node v = b->m_Node;
			int rank = m_ranks[v];
			b->m_nodes.init(rank, rank, nullptr);

		} else if (m_isActiveEdge[b->m_Edge]) 	{
			//m_Blocks[i] is EdgeBlock
			b->m_nodes.init(b->m_upper, b->m_lower, nullptr);
		}
	}

	// init m_nodes for vertex blocks
	for(node v : GC.nodes) {
		ranks[v] = m_hierarchy.rank(v);
		node vOrig = GC.original(v);
		if (vOrig != nullptr) {
			m_NodeBlocks[vOrig]->m_nodes[m_ranks[vOrig]] = v;
			mark[v] = true;
		}
	}

	//init m_nodes for edge blocks
	for(node v : GC.nodes) {
		if (m_hierarchy.isLongEdgeDummy(v)
		 && !mark[v]) {
			// find the edge from original graph corresponding to this node
			node low = v;
			node high = v;

			List<node> nodesInBlock;
			nodesInBlock.pushBack(v);
			while (!mark[low]) {
				for (adjEntry adj : low->adjEntries) {
					edge e = adj->theEdge();
					if (low == e->source()) {
						low = e->target();
						nodesInBlock.pushBack(low);
						break;
					}
				}
			}
			while (!mark[high]) {
				for (adjEntry adj : high->adjEntries) {
					edge e = adj->theEdge();
					if (high == e->target()) {
						high = e->source();
						nodesInBlock.pushBack(high);
						break;
					}
				}
			}

			Block *edgeBlock = m_EdgeBlocks[m_GC.searchEdge(GC.original(high),
			                                                GC.original(low))];
			for (node u : nodesInBlock) {
				if (!mark[u]) {
					edgeBlock->m_nodes[ranks[u]] = u;
					mark[u] = true;
				}
			}
		}
	}
}


void BlockOrder::buildLevels()
{
	Array<int> storedPermInv(m_activeBlocksCount);

	for (int i = 0; i < m_storedPerm.size(); ++i) {
		if (m_storedPerm[i] != -1)
			storedPermInv[m_storedPerm[i]] = i;
	}


	m_pos = NodeArray<int>(m_hierarchy,0);
	for (auto &level : m_levels) {
		delete level;
	}

	m_levels.init(0);

	// find maximum level index
	int maxLevel = 0;

	for (int i = 0; i < m_activeBlocksCount; ++i)
		maxLevel = max<int>(maxLevel, m_Blocks[storedPermInv[i]]->m_lower);

	// number of nodes (on each level)
	Array<int> levelNodes(0, maxLevel, 0);

	for (int i = 0; i < m_activeBlocksCount; ++i)
		for (int level = m_Blocks[storedPermInv[i]]->m_upper; level <= m_Blocks[storedPermInv[i]]->m_lower; ++level)
			levelNodes[level] += 1;


	m_levels.init(0, maxLevel);
	for (int i = 0; i <= maxLevel; ++i)
		m_levels[i] = new ArrayLevel(levelNodes[i]);

	Array<int> itemsOnLevelCtr(0, maxLevel, 0);

	for (int i = 0; i < m_activeBlocksCount; ++i){
		Block *b = m_Blocks[storedPermInv[i]];
		for (int level = b->m_upper; level <= b->m_lower; ++level) {
			ArrayLevel &lvl = *m_levels[level];
			lvl[itemsOnLevelCtr[level]] = b->m_nodes[level];
			m_pos[b->m_nodes[level]] = itemsOnLevelCtr[level];
			++itemsOnLevelCtr[level];
		}
	}
}

// copied from HierarchyLevels
void BlockOrder::buildAdjNodes()
{
	m_nSet = NodeArray<int>(m_hierarchy,0);
	m_lowerAdjNodes = NodeArray<Array<node> >(m_hierarchy);
	m_upperAdjNodes = NodeArray<Array<node> >(m_hierarchy);

	const GraphCopy &GC = m_hierarchy;
	for(node v : GC.nodes) {
		m_lowerAdjNodes[v].init(v->indeg());
		m_upperAdjNodes[v].init(v->outdeg());
	}


	for (int i = 0; i<= high(); ++i){
		if (i > 0) {
			const ArrayLevel &lowerLevel = *m_levels[i-1];

			for(int j = 0; j <= lowerLevel.high(); ++j)
				m_nSet[lowerLevel[j]] = 0;
		}

		if (i < high()) {
			const ArrayLevel &upperLevel = *m_levels[i+1];

			for(int j = 0; j <= upperLevel.high(); ++j)
				m_nSet[upperLevel[j]] = 0;
		}

		const ArrayLevel &level = *m_levels[i];
		for(int j = 0; j <= level.high(); ++j) {
			node v = level[j];
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if (e->source() == v) {
					(m_lowerAdjNodes[e->target()])[m_nSet[e->target()]++] = v;
				} else {
					(m_upperAdjNodes[e->source()])[m_nSet[e->source()]++] = v;
				}
			}
		}
	}
}


int BlockOrder::localCountCrossings(const Array<int> &levels)
{

	if (levels.size() < 2) return 0;
	// set m_currentPerm
	for (auto &level : m_levels) {
		delete level;
	}

	Graph G;

	Array<int> storedPermInv(m_Blocks.size());
	m_currentPermInv.init(m_Blocks.size());

	for (int i = 0; i < m_Blocks.size(); ++i) {
		m_currentPerm[i] = m_storedPerm[i];
		if (m_storedPerm[i] != -1) {
			storedPermInv[m_storedPerm[i]] = i;
			m_currentPermInv[m_storedPerm[i]] = i;
		}
	}


	sortAdjacencies();

	Array<unsigned int> itemsOnLevelCtr(0, levels.high(), 0);

	// create vertices used to calculate number of crossings
	for (int i = 0; i < m_Blocks.size(); ++i) {
		Block *block = m_Blocks[i];
		if (m_storedPerm[i] != -1) {
			block->m_nodes.init(levels[0], levels[levels.high()], nullptr);

			for (int j = 0; j < levels.size(); ++j) {
				int currentLevel = levels[j];
				if (block->m_upper <= currentLevel && block->m_lower >= currentLevel) {
					itemsOnLevelCtr[j] += 1;
					block->m_nodes[currentLevel] = G.newNode();
				}
			}
		} else {
			edge currentEdge = block->m_Edge;
			Block *targetBlock = m_NodeBlocks[currentEdge->target()];
			for (int level = levels.low(); level <= levels.high(); ++level) {
				if (block->m_upper <= levels[level] && levels[level] <= block->m_lower && targetBlock->m_nodes[levels[level]] == nullptr) {
					itemsOnLevelCtr[level] += 1;
					targetBlock->m_nodes[levels[level]] = G.newNode();
				}
			}
		}
	}

	m_levels.init(levels.size());
	for (int j = 0; j < levels.size(); ++j)
		m_levels[j] = new ArrayLevel(itemsOnLevelCtr[j]);

	m_pos.init(G);
	m_upperAdjNodes.init(G);

	itemsOnLevelCtr.init(0, levels.high(), 0);

	// build m_levels and m_pos
	for (int i = 0; i < m_activeBlocksCount; ++i) {
		Block *block = m_Blocks[storedPermInv[i]];
		for (int j = 0; j < levels.size(); ++j) {
			if (block->m_nodes[levels[j]] != nullptr) {
				(*m_levels[j])[itemsOnLevelCtr[j]] = block->m_nodes[levels[j]];
				m_pos[block->m_nodes[levels[j]]] = itemsOnLevelCtr[j];
				itemsOnLevelCtr[j] += 1;
			}
		}
	}

	// build m_upperAdjNodes
	for (int i = 0; i < m_activeBlocksCount; ++i) {
		Block *block = m_Blocks[storedPermInv[i]];
		for (int j = 0; j < levels.high(); ++j) {
			int currentLevel = levels[j];

			int nextLevel = levels[j+1];
			if (block->m_upper <= currentLevel && block->m_lower >= currentLevel) 	{
				if (nextLevel > block->m_lower) 	{
					Array<node> &upperAdj = m_upperAdjNodes[block->m_nodes[currentLevel]];
					upperAdj.init(block->m_NeighboursOutgoing.size());
					for (int k = 0; k < block->m_NeighboursOutgoing.size(); ++k) {
						upperAdj[k] = m_Blocks[block->m_NeighboursOutgoing[k]]->m_nodes[nextLevel];
					}
				} else {
					m_upperAdjNodes[block->m_nodes[currentLevel]]
						.init(0, 0, block->m_nodes[nextLevel]);
				}
			}
		}
	}
	return calculateCrossings();
}


int BlockOrder::verticalSwap(Block *b, int level)
{
	int Delta = 0;

	int minLvl = level;
	int maxLvl = level;

	for (int i = m_nNodesOnLvls.low(); i <= m_nNodesOnLvls.high(); ++i) {
		if (m_nNodesOnLvls[i] > 0) {
			minLvl = i;
			break;
		}
	}
	for (int i = m_nNodesOnLvls.high(); i >= m_nNodesOnLvls.low(); --i) {
		if (m_nNodesOnLvls[i] > 0) {
			maxLvl = i;
			break;
		}
	}

	Array<int> levels;
	if (level % 2 == 0) {
		int ctr = 0;
		if (minLvl <= level - 2 && level - 2 <= maxLvl) ++ctr;
		if (minLvl <= level - 1 && level - 1 <= maxLvl) ++ctr;
		if (minLvl <= level     && level     <= maxLvl) ++ctr;
		if (minLvl <= level + 1 && level + 1 <= maxLvl) ++ctr;
		if (minLvl <= level + 2 && level + 2 <= maxLvl) ++ctr;
		levels.init(ctr);
		ctr = 0;
		if (minLvl <= level - 2 && level - 2 <= maxLvl) levels[ctr++] = level - 2;
		if (minLvl <= level - 1 && level - 1 <= maxLvl) levels[ctr++] = level - 1;
		if (minLvl <= level     && level     <= maxLvl) levels[ctr++] = level;
		if (minLvl <= level + 1 && level + 1 <= maxLvl) levels[ctr++] = level + 1;
		if (minLvl <= level + 2 && level + 2 <= maxLvl) levels[ctr++] = level + 2;
	} else {
		int ctr = 0;
		if (minLvl <= level - 3 && level - 3 <= maxLvl) ++ctr;
		if (minLvl <= level - 1 && level - 1 <= maxLvl) ++ctr;
		if (minLvl <= level + 1 && level + 1 <= maxLvl) ++ctr;

		levels.init(ctr);
		ctr = 0;
		if (minLvl <= level - 3 && level - 3 <= maxLvl) levels[ctr++] = level - 3;
		if (minLvl <= level - 1 && level - 1 <= maxLvl) levels[ctr++] = level - 1;
		if (minLvl <= level + 1 && level + 1 <= maxLvl) levels[ctr++] = level + 1;
	}

	Delta -= localCountCrossings(levels);

	// phi(B) = level
	m_nNodesOnLvls[b->m_upper] += -1;
	b->m_upper = b->m_lower = level;
	m_nNodesOnLvls[level] += 1;

	Array<int> nextExistingLvl(m_nNodesOnLvls.low(), m_nNodesOnLvls.high(), -1);
	int last = std::numeric_limits<int>::max();
	for (int i = nextExistingLvl.high(); i >= nextExistingLvl.low(); --i) {
		nextExistingLvl[i] = last;
		if (m_nNodesOnLvls[i] > 0)
			last = i;
	}

	node v = b->m_Node;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		Block *BlockOfE = m_EdgeBlocks[e];
		if (v == e->source()) {
			BlockOfE->m_upper = level + 1;
		} else {
			// v == e->target()
			BlockOfE->m_lower = level - 1;
		}
	}

	for(edge e : m_GC.edges) {
		if (m_EdgeBlocks[e] != nullptr) {
			Block *blockOfE = m_EdgeBlocks[e];

			int top = blockOfE->m_upper;
			int bot = blockOfE->m_lower;

			int lvl = nextExistingLvl[top - 1];

			if (top <= lvl && lvl <= bot) {
				// block is active
				if (!m_isActiveEdge[e]) {
					int sourcePos = m_storedPerm[m_NodeBlocks[e->source()]->m_index];
					int targetPos = m_storedPerm[m_NodeBlocks[e->target()]->m_index];

					m_storedPerm[blockOfE->m_index] = (sourcePos + targetPos) / 2;
					m_activeBlocksCount += 1;
					m_isActiveEdge[e] = true;
				}
			} else {
				//block is inactive
				if (m_isActiveEdge[e]) {
					m_storedPerm[blockOfE->m_index] = -1;
					m_activeBlocksCount -= 1;
					m_isActiveEdge[e] = false;
				}
			}
		}
	}

	// bucketsort
	Array<List<int> > buckets(-1, m_storedPerm.size());
	for (int i = 0; i < m_storedPerm.size(); ++i) {
		buckets[m_storedPerm[i]].pushBack(i);
		m_storedPerm[i] = -1;
	}
	int ctr = 0;
	for (int i = 0; i <= buckets.high(); ++i) {
		while (!buckets[i].empty()) {
			int ind = buckets[i].front();
			buckets[i].popFront();
			m_storedPerm[ind] = ctr;
			ctr += 1;
		}
	}

	// calculate crossings once again
	for (int i = m_nNodesOnLvls.low(); i <= m_nNodesOnLvls.high(); ++i) {
		if (m_nNodesOnLvls[i] > 0) {
			minLvl = i;
			break;
		}
	}
	for (int i = m_nNodesOnLvls.high(); i >= m_nNodesOnLvls.low(); --i) {
		if (m_nNodesOnLvls[i] > 0) {
			maxLvl = i;
			break;
		}
	}

	if (level % 2 == 0) {
		ctr = 0;
		if (minLvl <= level - 2 && level - 2 <= maxLvl) ctr++;
		if (minLvl <= level     && level     <= maxLvl) ctr++;
		if (minLvl <= level + 2 && level + 2 <= maxLvl) ctr++;
		levels.init(ctr);

		ctr = 0;
		if (minLvl <= level - 2 && level - 2 <= maxLvl) levels[ctr++] = level - 2;
		if (minLvl <= level     && level     <= maxLvl) levels[ctr++] = level;
		if (minLvl <= level + 2 && level + 2 <= maxLvl) levels[ctr++] = level + 2;
	} else {
		ctr = 0;
		if (minLvl <= level - 3 && level - 3 <= maxLvl) ctr++;
		if (minLvl <= level - 1 && level - 1 <= maxLvl) ctr++;
		if (minLvl <= level     && level     <= maxLvl) ctr++;
		if (minLvl <= level + 1 && level + 1 <= maxLvl) ctr++;
		levels.init(ctr);

		ctr = 0;
		if (minLvl <= level - 3 && level - 3 <= maxLvl) levels[ctr++] = level - 3;
		if (minLvl <= level - 1 && level - 1 <= maxLvl) levels[ctr++] = level - 1;
		if (minLvl <= level     && level     <= maxLvl) levels[ctr++] = level;
		if (minLvl <= level + 1 && level + 1 <= maxLvl) levels[ctr++] = level + 1;
	}

	Delta += localCountCrossings(levels);

	// do horizontal steps
	v = b->m_Node;
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (m_isActiveEdge[e]) {
			int delta = siftingStep(m_EdgeBlocks[e]);
			Delta += delta;
		}
	}

	int delta = siftingStep(b);
	Delta += delta;

	return Delta;
}


void BlockOrder::verticalStep(Block *b)
{
	int maxLevel = 0;
	//normalize levels to 2,4,6,8,...
	for (auto currentBlock : m_Blocks) {
		if (currentBlock->isVertexBlock()) {
			currentBlock->m_upper = 2 + 2 * (currentBlock->m_upper);
			currentBlock->m_lower = 2 + 2 * (currentBlock->m_lower);
		} else {
			// currentBlock->isEdgeBlock()
			edge e = currentBlock->m_Edge;
			currentBlock->m_upper = m_NodeBlocks[e->source()]->m_lower + 1;
			currentBlock->m_lower = m_NodeBlocks[e->target()]->m_upper - 1;
		}

		if (currentBlock->m_lower > maxLevel)
			maxLevel = currentBlock->m_lower;
	}

	m_nNodesOnLvls.init(1, maxLevel + 1, 0);

	for (auto &block : m_Blocks) {
		if (block->isVertexBlock()) {
			m_nNodesOnLvls[block->m_upper] += 1;
		}
	}

	int lMin = 1;
	int lMax = maxLevel + 1;
	node v = b->m_Node;
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (v == e->source()) {
			lMax = min<int>(lMax, m_NodeBlocks[e->target()]->m_upper - 1);
		} else {
		  // v == e->target()
			lMin = max<int>(lMin, m_NodeBlocks[e->source()]->m_lower + 1);
		}
	}
	if (lMin < b->m_upper - m_verticalStepsBound)
		lMin = b->m_upper - m_verticalStepsBound;
	if (lMax > b->m_lower + m_verticalStepsBound)
		lMax = b->m_lower + m_verticalStepsBound;

	Array<int> startingPerm(0, m_storedPerm.high(), -1);

	Array<int> startingM_upper(0, m_Blocks.high(), 0);
	Array<int> startingM_lower(0, m_Blocks.high(), 0);

	int startingActiveBlocksCount;
	EdgeArray<bool> startingM_isActiveEdge;

	Array<int> bestPerm(m_storedPerm.size());

	Array<int> bestM_upper(0, m_Blocks.high(), 0);
	Array<int> bestM_lower(0, m_Blocks.high(), 0);

	int bestActiveBlocksCount = 0;
	EdgeArray<bool> bestM_isActiveEdge;

	int bestChi = std::numeric_limits<int>::max();
	int currentChi = 0;


	// store starting embedding
	for (int i = 0; i < m_Blocks.size(); ++i) {
		startingPerm[i]    = m_storedPerm[i];
		startingM_lower[i] = m_Blocks[i]->m_lower;
		startingM_upper[i] = m_Blocks[i]->m_upper;
	}
	startingActiveBlocksCount = m_activeBlocksCount;
	startingM_isActiveEdge = m_isActiveEdge;

	// for upward
	for (int level = b->m_upper + 1; level >= lMin; --level) {
		int Delta = verticalSwap(b, level);
		currentChi += Delta;
		if (currentChi < bestChi) {
			// store currentEmbedding as best found
			for (int i = 0; i < m_Blocks.size(); ++i) {
				bestPerm[i]    = m_storedPerm[i];
				bestM_lower[i] = m_Blocks[i]->m_lower;
				bestM_upper[i] = m_Blocks[i]->m_upper;
			}

			bestActiveBlocksCount = m_activeBlocksCount;
			bestM_isActiveEdge = m_isActiveEdge;
			bestChi = currentChi;
		}
	}

	// restore starting embedding
	for (int i = 0; i < m_Blocks.size(); ++i) {
		m_storedPerm[i] = startingPerm[i];
		m_Blocks[i]->m_lower = startingM_lower[i];
		m_Blocks[i]->m_upper = startingM_upper[i];
	}

	currentChi = 0;

	m_activeBlocksCount = startingActiveBlocksCount;
	m_isActiveEdge = startingM_isActiveEdge;

	m_nNodesOnLvls.init(1, maxLevel + 1, 0);

	for (auto &block : m_Blocks) {
		if (block->isVertexBlock()) {
			m_nNodesOnLvls[block->m_upper] += 1;
		}
	}

	// for downward
	for (int level = b->m_lower + 1; level <= lMax; ++level) {
		int Delta = verticalSwap(b, level);
		currentChi += Delta;
		if (currentChi < bestChi) {
			// store currentEmbedding as best found
			for (int i = 0; i < m_Blocks.size(); ++i) {
				bestPerm[i]    = m_storedPerm[i];
				bestM_lower[i] = m_Blocks[i]->m_lower;
				bestM_upper[i] = m_Blocks[i]->m_upper;
			}

			bestActiveBlocksCount = m_activeBlocksCount;
			bestM_isActiveEdge = m_isActiveEdge;
			bestChi = currentChi;
		}
	}

	// restore best
	for (int i = 0; i < m_Blocks.size(); ++i) {
		m_storedPerm[i] = bestPerm[i];
		m_Blocks[i]->m_lower = bestM_lower[i];
		m_Blocks[i]->m_upper = bestM_upper[i];
	}
	currentChi = bestChi;

	m_activeBlocksCount = bestActiveBlocksCount;
	m_isActiveEdge = bestM_isActiveEdge;

	// delete empty levels
	m_nNodesOnLvls.init(1, maxLevel + 1, 0);

	for (auto &block : m_Blocks) {
		if (block->isVertexBlock()) {
			m_nNodesOnLvls[block->m_upper] += 1;
		}
	}

	int p = 0;
	Array<int> normalizedLvl(1, maxLevel + 1);
	for (int i = 1; i <= maxLevel + 1; ++i) {
		if (m_nNodesOnLvls[i] > 0) {
			normalizedLvl[i] = p;
			++p;
		}
	}

	for (auto currentBlock : m_Blocks) {
		if (currentBlock->isVertexBlock()) {
			currentBlock->m_upper = normalizedLvl[currentBlock->m_upper];
			currentBlock->m_lower = normalizedLvl[currentBlock->m_lower];
		} else { // currentBlock->isEdgeBlock()
			edge e = currentBlock->m_Edge;
			currentBlock->m_upper = m_NodeBlocks[e->source()]->m_lower  + 1;
			currentBlock->m_lower = m_NodeBlocks[e->target()]->m_upper  - 1;
		}
	}
}


void BlockOrder::gridSifting(int nRepeats)
{
#if 0
	while (rho-- > 0)
#endif
	{
		Array<int> storedPermInv(0, m_Blocks.high(), -1);
		m_storedPerm.init(0, m_Blocks.high(), -1);
		int p = 0;

		for (int i = 0; i < m_Blocks.size(); ++i) {
			if (m_Blocks[i]->isVertexBlock()
			 || (m_Blocks[i]->isEdgeBlock() && m_isActiveEdge[m_Blocks[i]->m_Edge])) {
				storedPermInv[p] = i;
				m_storedPerm[i] = p;
				++p;
			}
		}

		// initialize with random permutation
		storedPermInv.permute(0, m_activeBlocksCount - 1);

		for (int i = 0; i < m_activeBlocksCount; ++i)
			m_storedPerm[storedPermInv[i]] = i;


		int times = nRepeats;
		while (times-- > 0) {
			for(node v : m_GC.nodes) {
				Block *currentBlock = m_NodeBlocks[v];
				if (currentBlock != nullptr) {
					verticalStep(currentBlock);
				}
			}
		}
	}

	// reassign m_hierarchy!
	m_ranks.init(m_GC, 0);
	EdgeArray<edge> auxCopy(m_GC);
	List<node> nodes;
	for(node v : m_GC.nodes) {
		if (m_NodeBlocks[v] != nullptr) {
			m_ranks[v] = m_NodeBlocks[v]->m_upper;
			nodes.pushBack(v);
		}
	}
	m_hierarchy.createEmpty(m_GC);
	m_hierarchy.initByNodes(nodes, auxCopy, m_ranks);

	// build levels
	buildHierarchy();
	return;
}

}
