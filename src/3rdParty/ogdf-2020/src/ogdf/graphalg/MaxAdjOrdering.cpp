/** \file
 * \brief Calculate one or all Maximum Adjacency Ordering(s) of a given simple undirected graph.
 *
 * \author Sebastian Semper
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

#include <ogdf/graphalg/MaxAdjOrdering.h>


namespace ogdf {

MaxAdjOrdering::MaxAdjOrdering()
{
}

MaxAdjOrdering::~MaxAdjOrdering()
{
}

void MaxAdjOrdering::calc(const Graph *G,
                          ListPure<node> *MAO)
{
	//node count
	int n = G->numberOfNodes();

	//store unsorted nodes
	List<node> unsortedNodes;
	G->allNodes(unsortedNodes);

	//neighbourhood counter
	NodeArray<int> r{*G, 0};

	//currently maximal node
	node curMaxNode(*(unsortedNodes.begin()));
	int curMaxVal = 0;

	//last added node
	node lastAdded;

	//add n vertices to M
	for (int i = 0; i < n; i++) {
		//add current maximum to end of MAO
		MAO->pushBack(curMaxNode);

		//store the last added
		lastAdded = curMaxNode;

		//delete it from unsorted nodes
		unsortedNodes.del(unsortedNodes.search(lastAdded));

		//set maximal node to first unsorted
		if (i < n-1) {
			curMaxNode = unsortedNodes.front();
			curMaxVal = r[curMaxNode];
			for (auto& u : unsortedNodes) {
				if (curMaxVal < r[u]) {
					curMaxVal = r[u];
					curMaxNode = u;
				}
			}
		}

		//edges to iterate over
		for (adjEntry adj : lastAdded->adjEntries) {
			edge e = adj->theEdge();

			//node at the other side
			node end(e->opposite(lastAdded));

			//search it in unsorted nodes
			ListIterator<node> endIt(unsortedNodes.search(end));

			//proceed if unsorted
			if (endIt.valid()) {
				//reset neighbourhood
				r[end]++;

				//reset maximal node
				if (r[end] > curMaxVal) {
					curMaxVal = r[end];
					curMaxNode = end;
				}
			}
		}
	}
}

void MaxAdjOrdering::calcBfs(const Graph *G,
                             ListPure<node> *MAO)
{
	//node count
	int n = G->numberOfNodes();
	if (n < 1) {
		return;
	}

	//store unsorted nodes
	ListPure<node> unsortedNodes;
	G->allNodes(unsortedNodes);

	//store tied nodes
	ListPure<node> tiedNodes;
	int curMaxTie = 0;
	node curMaxTieNode(*(unsortedNodes.begin()));

	//neighbourhood counter
	NodeArray<int> r{*G, 0};

	//currently maximal node
	node curMaxNode(*(unsortedNodes.begin()));
	int curMaxVal = 0;

	//last added node
	node lastAdded;

	//add n vertices to M
	for (int i = 0; i < n; i++) {
		//add current maximum to end of MAO
		//MAO->pushBack(curMaxNode);
		MAO->pushBack(curMaxTieNode);

		//store the last added
		lastAdded = MAO->back();

		//delete it from unsorted nodes
		unsortedNodes.del(unsortedNodes.search(lastAdded));

		//set maximal node to first unsorted
		if (i < n-1) {
			//reset the tied nodes
			tiedNodes.clear();
			tiedNodes.pushBack(unsortedNodes.front());
			curMaxTieNode = unsortedNodes.front();
			curMaxTie = 0;

			//reset the currently maximal node
			curMaxNode = unsortedNodes.front();
			curMaxVal = r[curMaxNode];

			for (auto& u : unsortedNodes) {
				//if we find a node tha currently also attains the maximum, we add it to the tiedNodes
				if ((r[u] == curMaxVal)&&(u != tiedNodes.front())) {
					tiedNodes.pushBack(u);
				}
				//if the maximum changes we need to clear the tied nodes
				if (curMaxVal < r[u]) {
					tiedNodes.clear();
					tiedNodes.pushBack(u);
					curMaxVal = r[u];
					curMaxNode = u;
				}
			}
			//now calc the lex-bfs-value for every tied node
			for (auto& t : tiedNodes) {
				int tieVal = 0;
				for (adjEntry adj : t->adjEntries) {
					ListIterator<node> opIt = MAO->search(adj->twinNode());
					if (opIt.valid()) {
						OGDF_ASSERT(MAO->size() - MAO->pos(opIt) < 31);
						tieVal += 1 << (MAO->size() - MAO->pos(opIt));
					}
				}
				//update the currently maximum tied node
				if (tieVal > curMaxTie) {
					curMaxTieNode = t;
					curMaxTie = tieVal;
				}
			}
		}

		//edges to iterate over
		for (adjEntry adj : lastAdded->adjEntries) {
			edge e = adj->theEdge();
			//node at the other side
			node end(e->opposite(lastAdded));

			//search it in unsorted nodes
			ListIterator<node> endIt(unsortedNodes.search(end));

			//proceed if unsorted
			if (endIt.valid()) {
				//reset neighbourhood
				r[end]++;
				bool tieUpdated = 0;
				if (r[end] == curMaxVal) {
					tiedNodes.pushBack(end);
					tieUpdated = 1;
				}
				//reset maximal node
				if (r[end] > curMaxVal) {
					tiedNodes.clear();
					tiedNodes.pushBack(end);
					curMaxVal = r[end];
					curMaxNode = end;
					tieUpdated = 1;
				}

				//if anything changed we need to recalc all tied nodes
				//TODO only do what is neccesary
				if (tieUpdated) {
					curMaxTieNode = end;
					curMaxTie = 0;
					for (auto& t : tiedNodes) {
						int tieVal = 0;
						for (adjEntry adjT : t->adjEntries) {
							ListIterator<node> opIt = MAO->search(adjT->twinNode());
							if (opIt.valid()) {
								OGDF_ASSERT(MAO->size() - MAO->pos(opIt) < 31);
								tieVal += 1 << (MAO->size() - MAO->pos(opIt));
							}
						}
						//update the currently maximum tied node
						if (tieVal > curMaxTie) {
							curMaxTieNode = t;
							curMaxTie = tieVal;
						}
					}
				}
			}
		}

		Logger::slout(Logger::Level::Minor) << "Tied nodes with maximal tie value -" << curMaxTie << "- among the nodes: ";
		for (auto& t : tiedNodes) {
			Logger::slout(Logger::Level::Minor) << t->index() << ",";
		}
		Logger::slout(Logger::Level::Minor) <<" and  node "<< curMaxTieNode->index() << " wins." << std::endl;

	}
}

void MaxAdjOrdering::calc(const Graph *G,
                          ListPure<node> *MAO,
                          ListPure<ListPure<edge>> *Forests)
{
	//node count
	int n(G->numberOfNodes());

	//store unsorted nodes
	List<node> unsortedNodes;
	G->allNodes(unsortedNodes);

	//neighbourhood counter
	NodeArray<int> r{*G, 0};

	//currently maximal node
	node curMaxNode(*(unsortedNodes.begin()));
	int curMaxVal(0);

	//last added node
	node lastAdded;

	//add n vertices to M
	for (int i = 0; i < n; i++) {
		//add current maximum to end of MAO
		MAO->pushBack(curMaxNode);

		//store the last added
		lastAdded = curMaxNode;

		//delete it from unsorted nodes
		unsortedNodes.del(unsortedNodes.search(lastAdded));

		//set maximal node to currently maximal unsorted
		if (i < n-1) {
			curMaxNode = unsortedNodes.front();
			curMaxVal = r[curMaxNode];
			for (auto& u : unsortedNodes) {
				if (curMaxVal < r[u]) {
					curMaxVal = r[u];
					curMaxNode = u;
				}
			}
		}

		//edges to iterate over
		for (adjEntry adj : lastAdded->adjEntries) {
			edge e = adj->theEdge();
			//node at the other side
			node end(adj->twinNode());

			//search it in unsorted nodes
			ListIterator<node> endIt(unsortedNodes.search(end));

			//proceed if unsorted
			if (endIt.valid()) {
				//correct neighbourhood
				int r_(++r[end]);

				if (r_ > curMaxVal) {
					curMaxVal = r_;
					curMaxNode = end;
				}
				if (r_ >= Forests->size()) {
					Forests->pushBack(ListPure<edge>());
				}
				(*(Forests->get(r_-1))).pushBack(e);
			}
		}
	}
}

void MaxAdjOrdering::calc(const Graph *G,
                          node s,
                          ListPure<node> *MAO)
{
	//node count
	int n = G->numberOfNodes();

	//store unsorted nodes
	List<node> unsortedNodes;
	G->allNodes(unsortedNodes);

	//neighbourhood counter
	NodeArray<int> r{*G, 0};

	//currently maximal node
	node curMaxNode = s;
	int curMaxVal = 0;

	//last added node
	node lastAdded;

	//add n vertices to M
	for (int i = 0; i < n; i++) {
		//add current maximum to end of MAO
		MAO->pushBack(curMaxNode);

		//store the last added
		lastAdded = curMaxNode;

		//delete it from unsorted nodes
		unsortedNodes.del(unsortedNodes.search(lastAdded));

		if (i < n-1) {
			curMaxNode = unsortedNodes.front();
			curMaxVal = r[curMaxNode];
			for (auto& u : unsortedNodes) {
				if (curMaxVal < r[u]) {
					curMaxVal = r[u];
					curMaxNode = u;
				}
			}
		}

		//edges to iterate over
		for (adjEntry adj : lastAdded->adjEntries) {
			edge e = adj->theEdge();
			//node at the other side
			node end(e->opposite(lastAdded));

			//search it in unsorted nodes
			ListIterator<node> endIt(unsortedNodes.search(end));

			//proceed if unsorted
			if (endIt.valid()) {
				//correct neighbourhood
				r[end]++;

				if (r[end] > curMaxVal) {
					curMaxVal = r[end];
					curMaxNode = end;
				}
			}
		}
	}
}

void MaxAdjOrdering::calc(const Graph *G,
                          node s,
                          ListPure<node> *MAO,
                          ListPure<ListPure<edge>> *Forests)
{
	//node count
	int n = G->numberOfNodes();

	//store unsorted nodes
	List<node> unsortedNodes;
	G->allNodes(unsortedNodes);

	//neighbourhood counter
	NodeArray<int> r{*G, 0};

	//currently maximal node
	node curMaxNode = s;
	int curMaxVal = 0;

	//last added node
	node lastAdded;

	//add n vertices to M
	for (int i = 0; i < n; i++) {
		//add current maximum to end of MAO
		MAO->pushBack(curMaxNode);

		//store the last added
		lastAdded = curMaxNode;

		//delete it from unsorted nodes
		unsortedNodes.del(unsortedNodes.search(lastAdded));

		//set maximal node to first unsorted
		if (i < n-1) {
			curMaxNode = unsortedNodes.front();
			curMaxVal = r[curMaxNode];
			for (auto& u : unsortedNodes) {
				if (curMaxVal < r[u]) {
					curMaxVal = r[u];
					curMaxNode = u;
				}
			}
		}

		//edges to iterate over
		for (adjEntry adj : lastAdded->adjEntries) {
			edge e = adj->theEdge();
			//node at the other side
			node end(e->opposite(lastAdded));

			//search it in unsorted nodes
			ListIterator<node> endIt(unsortedNodes.search(end));

			//proceed if unsorted
			if (endIt.valid()) {
				//correct neighbourhood
				int r_(++r[end]);

				if (r[end] > curMaxVal) {
					curMaxVal = r[end];
					curMaxNode = end;
				}

				if (r_ >= Forests->size()) {
					Forests->pushBack(ListPure<edge>());
				}
				(*(Forests->get(r_-1))).pushBack(e);
			}
		}
	}
}

void MaxAdjOrdering::calcAll(const Graph *G,
                             ListPure<ListPure<node>> *MAOs)
{
	//initialize backtrackstack
	ListPure<node> nodes;
	G->allNodes(nodes);

	//first step in recursion. every node is an option for the first one
	//in the ordering. so start the recursion once for every node
	for (node it : nodes) {
		ListPure<node> start;
		ListPure<node> unsorted = nodes;
		unsorted.del(unsorted.search(it));
		start.pushBack(it);
		m_calcAllMAOs_recursion(G->numberOfNodes(),
		                        start,
		                        unsorted,
		                        NodeArray<int>{*G, 0},
		                        MAOs);
	}
}


void MaxAdjOrdering::m_calcAllMAOs_recursion(int n,
                                             ListPure<node> currentOrder,
                                             ListPure<node> currentUnsorted,
                                             NodeArray<int> r,
                                             ListPure<ListPure<node>> *MAOs)
{
	if (currentUnsorted.empty()) {
		//one MAO is done!
		MAOs->pushBack(currentOrder);

		//go back up into recursion
		return;
	}

	//store the last node in current order
	node lastAdded = currentOrder.back();

	//if we want all maos, we have to store ALL possible next nodes
	ListPure<node> maxNodes;

	//choose the first maxValue as the first value in the unsorted
	int maxValue(r[(currentUnsorted.front())]);

	for (auto& u : currentUnsorted) {
		if (maxValue < r[u]) {
			maxValue = r[u];
		}
	}

	//add all nodes that have this value
	for (node it : currentUnsorted) {
		if (r[it] == maxValue) {
			maxNodes.pushBack(it);
		}
	}

	//edges to iterate over
	for (adjEntry adj : lastAdded->adjEntries) {
		edge e = adj->theEdge();
		//node at the other side
		node end(e->opposite(lastAdded));
		ListIterator<node> endIt(currentUnsorted.search(end));

		//if is unsorted
		if (endIt.valid()) {
			node endNode(*endIt);

			//increase value of neighborhood
			r[endNode]++;

			//if it is the current maximum, add it to the list
			if (r[endNode] == maxValue) {
				maxNodes.pushBack(end);
			}

			//if it is larger than the current maximum
			if (r[endNode] > maxValue) {
				//reset maximum value
				maxValue = r[endNode];

				//clear maximum list
				maxNodes.clear();

				//add the current node
				maxNodes.pushBack(end);
			}
		}
	}

	//go deeper into recursion for every possible node in maxNodes
	for (node it : maxNodes) {
		ListPure<node> nextOrder = currentOrder;
		ListPure<node> nextUnsorted = currentUnsorted;

		//the current node is the next one in the next calculated order
		nextOrder.pushBack(it);

		//the current node needs to be removed from the unsorted for the next step
		nextUnsorted.del(nextUnsorted.search(it));
		m_calcAllMAOs_recursion(n,
		                        nextOrder,
		                        nextUnsorted,
		                        r,
		                        MAOs);
	}
}

void MaxAdjOrdering::calcAll(const Graph *G,
                             ListPure<ListPure<node>> *MAOs,
                             ListPure<ListPure<ListPure<edge>>> *Fs)
{
	//initialize backtrackstack
	ListPure<node> nodes;
	G->allNodes(nodes);

	//first step in recursion. every node is an option for the first one
	//in the ordering. so start the recursion once for every node
	//but just leave the forests empty at first
	for (node it : nodes) {
		ListPure<node> start;
		ListPure<node> unsorted = nodes;
		unsorted.del(unsorted.search(it));
		start.pushBack(it);
		m_calcAllMAOs_recursion(G->numberOfNodes(),
		                        start,
		                        ListPure<ListPure<edge>>(),
		                        unsorted,
		                        NodeArray<int>{*G, 0},
		                        MAOs,
		                        Fs);
	}
}

void MaxAdjOrdering::m_calcAllMAOs_recursion(int n,
                                             ListPure<node> currentOrder,
                                             ListPure<ListPure<edge>> currentForest,
                                             ListPure<node> currentUnsorted,
                                             NodeArray<int> r,
                                             ListPure<ListPure<node>> *MAOs,
                                             ListPure<ListPure<ListPure<edge>>> *Fs)
{
	if (currentUnsorted.empty()) {
		//one MAO is done!
		MAOs->pushBack(currentOrder);
		Fs->pushBack(currentForest);
		//go back up into recursion
		return;
	}

	//store the last node in current order
	node lastAdded = currentOrder.back();

	//if we want all maos, we have to store ALL possible next nodes
	ListPure<node> maxNodes;

	//choose the first maxValue as the first value in the unsorted
	int maxValue(r[(currentUnsorted.front())]);
	for (auto& u : currentUnsorted) {
		if (maxValue < r[u]) {
			maxValue = r[u];
		}
	}

	//add all nodes that have this value
	for (node it : currentUnsorted) {
		if (r[it] == maxValue) {
			maxNodes.pushBack(it);
		}
	}

	//edges to iterate over
	for (adjEntry adj : lastAdded->adjEntries) {
		edge e = adj->theEdge();
		//node at the other side
		node end(e->opposite(lastAdded));
		ListIterator<node> endIt(currentUnsorted.search(end));

		//if is unsorted
		if (endIt.valid()) {
			//increase value of neighborhood and store it
			int r_(++r[(*endIt)]);

			//if it is the current maximum, add it to the list
			if (r_ == maxValue) {
				maxNodes.pushBack(end);
			}

			//if it is larger than the current maximum
			if (r_ > maxValue) {
				//reset maximum value
				maxValue = r_;

				//clear maximum list
				maxNodes.clear();

				//add the current node
				maxNodes.pushBack(end);
			}

			//depending on the last node - populate the forest accordingly
			if (r_ >= currentForest.size()) {
				currentForest.pushBack(ListPure<edge>());
			}
			(*currentForest.get(r_-1)).pushBack(e);
		}
	}

	//go deeper into recursion for every possible node in maxNodes
	for (node it : maxNodes) {
		ListPure<node> nextOrder = currentOrder;
		ListPure<node> nextUnsorted = currentUnsorted;

		//the current node needs to be removed from the unsorted for the next step
		nextUnsorted.del(nextUnsorted.search(it));

		//the current node is the next one in the next calculated order
		nextOrder.pushBack(it);

		m_calcAllMAOs_recursion(n,
		                        nextOrder,
		                        currentForest,
		                        nextUnsorted,
		                        r,
		                        MAOs,
		                        Fs);
	}
}

bool MaxAdjOrdering::testIfMAO(const Graph *G, ListPure<node> *Ordering)
{
	unsigned int i = 0;
	unsigned int n = Ordering->size();
	NodeArray<unsigned int> r(*G,0);
	node op;
	ListPure<node> tested;
	for (auto& o : *Ordering) {
		tested.pushBack(o);
		for (adjEntry adj : o->adjEntries) {
			edge e = adj->theEdge();
			op = e->opposite(o);
			//check if edge goes to the right
			if (!tested.search(op).valid()) {
				//increment the neighbourhood counter
				r[op]++;
			}
		}
		if (i < n-1) {
			/**go through all following nodes and check if
			 * neighbourhood is bigger than then one in the
			 * ordering. If yes - return false because no MAO.
			 */
			for (ListIterator<node> next = Ordering->get(i+1); next != Ordering->end(); next++) {
				if (r[*next] > r[*(Ordering->get(i+1))]) {
					return 0;
				}
			}
		}
		i++;
	}
	return 1;
}

bool MaxAdjOrdering::testIfMAOBfs(const Graph *G, ListPure<node> *Ordering)
{
	unsigned int i = 0;
	NodeArray<unsigned int> r(*G,0);
	NodeArray<unsigned int> nbh(*G,0);
	node op;
	ListPure<node> tested;
	for (auto& o : *Ordering) {
		for (adjEntry adj : o->adjEntries) {
			edge e = adj->theEdge();
			op = e->opposite(o);
			//check if edge goes to the right
			if (!tested.search(op).valid()) {
				//increment the neighbourhood counter
				r[op]++;
			}
		}
		for (ListIterator<node> next = Ordering->get(i); next.valid();next++) {
			nbh[*next] *= 2;
			if (G->searchEdge(o,*next)) {
				nbh[*next]++;
			}
		}
		tested.pushBack(o);
		/**go through all following nodes and check if
		 * neighbourhood is bigger than then one in the
		 * ordering. If yes - return false because no MAO.
		 */
		for (ListIterator<node> next = Ordering->get(i+2); next.valid(); next++) {
			if (r[*next] > r[*(Ordering->get(i+1))]) {
				return 0;
			}

			if ((nbh[*next] > nbh[*(Ordering->get(i+1))])&&(r[*next] == r[*(Ordering->get(i+1))])) {
				return 0;
			}
		}

		i++;
	}
	return 1;
}

bool MaxAdjOrdering::testIfAllMAOs(const Graph *G,
                                   ListPure<ListPure<node>> *Orderings,
                                   ListPure<ListPure<node>> *Perms)
{
	ListPure<node> nodes;
	G->allNodes(nodes);
	int n = nodes.size();
	ListPure<node> testPerm;

	for (auto& p : *Perms) {
		testPerm.clear();
		//generate nodelist of G from permutation
		for (int i = 0; i < n; i++) {
			int index = (*(p.get(i)))->index();

			testPerm.pushBack(*(nodes.get(index)));
		}

		//check if testPerm is a MAO - this way we find all MAOs
		if (testIfMAO(G,&testPerm)) {
			//if we don't find the testPerm in our provided list, we will not have generated
			//all MAOs
			ListIterator<ListPure<node>> pIt = Orderings->search(testPerm);
			if (!pIt.valid())
				return 0;
		}
		else {
			//if we find the testPerm in the list we did calculate to many MAOs
			ListIterator<ListPure<node>> pIt = Orderings->search(testPerm);
			if (pIt.valid())
				return 0;
		}
	}

	return 1;
}

void MaxAdjOrdering::visualize(GraphAttributes *GA,
                               ListPure<node> *MAO)
{
	const Graph& G = GA->constGraph();
	List<node> nodes;
	G.allNodes(nodes);

	LinearLayout layout(600,*MAO);
	layout.setCustomOrder(1);
	layout.call(*GA);

	int k = 1;
	for (auto& n : *MAO) {
		GA->height(n) = 15;
		GA->width(n) = 15;
		GA->label(n) = std::to_string(k++);
		GA->shape(n) = Shape::Ellipse;
		GA->strokeColor(n) = Color(Color::Name::Black);
		GA->fillColor(n) = Color(Color::Name::Red);
	}
}

void MaxAdjOrdering::visualize(GraphAttributes *GA,
                               ListPure<node> *MAO,
                               ListPure<ListPure<edge>> *F)
{
	const Graph& G = GA->constGraph();
	List<node> nodes;
	G.allNodes(nodes);

	LinearLayout layout(140*nodes.size(),*MAO);
	layout.setCustomOrder(1);
	layout.call(*GA);

	int k = 1;
	for (auto& n : *MAO) {
		GA->height(n) = 15;
		GA->width(n) = 30;
		GA->label(n) = std::to_string(k++) + std::string(",") + std::to_string(n->index()+1);
		GA->shape(n) = Shape::Ellipse;
		GA->strokeColor(n) = Color(Color::Name::Black);
		GA->fillColor(n) = Color(Color::Name::Red);
	}

	k = 1;
	for (auto& f : *F) {
		for (auto& e : f) {
			GA->strokeWidth(e) = 2.f * static_cast<float>(k);
			GA->arrowType(e) = EdgeArrow::None;
		}
		k++;
	}
}

}
