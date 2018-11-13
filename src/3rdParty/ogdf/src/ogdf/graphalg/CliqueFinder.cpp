/** \file
 * \brief Implementation of a heuristical method to find cliques
 * in a given input graph.
 *
 * \author Karsten Klein
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


#include <ogdf/graphalg/CliqueFinder.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/geometry.h>

#ifdef OGDF_DEBUG
#include <ogdf/fileformats/GraphIO.h>
#endif


namespace ogdf {


//constructor
CliqueFinder::CliqueFinder(const Graph &G) : m_pGraph(&G), m_pCopy(nullptr),
	m_minDegree(2),
	m_numberOfCliques(0),
	m_postProcess(postProcess::Simple),
	m_callByList(false),
	m_pList(nullptr),
	m_density(100)
{
	try {
		m_pCopy = new GraphCopy(G);
		m_copyCliqueNumber.init(*m_pCopy, -1);
		m_usedNode.init(*m_pCopy, false);
	}
	catch (...)
	{
		OGDF_THROW(InsufficientMemoryException);
	}
}

CliqueFinder::~CliqueFinder()
{
	if (m_pCopy != nullptr)
	{
		//we have to uninitialize the nodearray before destroying
		//the graph
		m_copyCliqueNumber.init();
		m_usedNode.init();

		delete m_pCopy;
	}
}



//Call with NodeArray, each clique will be assigned a
//different number, each node gets the number of the
//clique it is contained in, -1 if not a clique member
void CliqueFinder::call(NodeArray<int> &cliqueNumber)
{
	m_callByList = false;
	m_pList = nullptr;
	//First find the cliques: doCall
	doCall(m_minDegree);
	//Then set the result: setResults(cliqueNumber);
	setResults(cliqueNumber);
}

//call with list of node lists, on return these lists contain
//the nodes in each clique that is found
void CliqueFinder::call(List< List<node> > &cliqueLists)
{
	m_callByList = true;
	m_pList = &cliqueLists;
	m_pList->clear();

	//First find the cliques: doCall
	doCall(m_minDegree);
	//setresult is called in doCall for every treated component

	m_pList = nullptr;
}


//minDegree default 2, all other nodes are skipped
//only high values have an impact because we only
//work on triconnected components, skipping all low
//degree nodes (but we make the test graphcopy biconnected
//afterwards)
void CliqueFinder::doCall(int minDegree)
{
	//initialize structures and check preconditions
	m_copyCliqueNumber.init(*m_pCopy, -1);
	m_usedNode.init(*m_pCopy, false);
	makeParallelFreeUndirected(*m_pCopy); //it doesnt make sense to count loops
	makeLoopFree(*m_pCopy);               //or parallel edges

	m_numberOfCliques = 0;
	//We first indentify the biconnected components of
	//the graph to allow the use of the SPQR-tree data
	//Structure. Latter then separates the different
	//triconnected components on which we finally work

	//TODO: delete all copy nodes with degree < minDegree

	int nodeNum = m_pGraph->numberOfNodes();
	//TODO: change for non-cliques, where this is not necessary
	if (nodeNum < minDegree) return; //nothing to find for cliques

	//Special cases:
	//Precondition for SPQR-trees: graph has at least 3 nodes
	//or 2 nodes and at least 3 edges
	//TODO: check this after makebiconnected

	//set values for trivial cases
	if (nodeNum < 3)
	{
		//only set numbers for the special case
		if (nodeNum == 2)
		{
			if (m_pGraph->numberOfEdges() >= 1)  //> 2)
			{
				node w = m_pCopy->firstNode();
				m_copyCliqueNumber[w] = 0;
				w = w->succ();
				m_copyCliqueNumber[w] = 0;
			}
			else
			{
				if (minDegree == 0)
				{
					node w = m_pCopy->firstNode();
					m_copyCliqueNumber[w] = 0;
					w = w->succ();
					m_copyCliqueNumber[w] = 1;
				}
			}
		} else if ( (nodeNum == 1) && (minDegree <= 0)) {
			m_copyCliqueNumber[m_pCopy->firstNode()] = 0;
		}

		return;
	}

	OGDF_ASSERT(m_pCopy != nullptr);

	//save the original edges
	EdgeArray<bool> originalEdge(*m_pCopy, true);
	List<edge> added;


	//we make the copy biconnected, this keeps the triconnected
	//components


	//store the original node degrees:
	//afterwards we want to be able to sort the nodes corresponding
	//to their real degree, not the one with the additional
	//connectivity edges
	NodeArray<int> realDegree(*m_pCopy, -1);//no isolated nodes exist here
	//relative degree, number of high degree neighbours
	NodeArray<int> relDegree(*m_pCopy, 0);//no isolated nodes exist here
	for(node v : m_pCopy->nodes)
	{
		realDegree[v] = v->degree();
		if (v->degree() > 0)
		{
			adjEntry adRun = v->firstAdj();
			while (adRun)
			{
				adjEntry succ = adRun->succ();
				if (adRun->twinNode()->degree() >= minDegree)
					relDegree[v]++;
				adRun = succ;
			}
		}
	}

	makeBiconnected(*m_pCopy, added);

	//TODO: We can decrease node degrees by the number of adjacent
	//low degree nodes to sort them only by number of relevant connections
	//PARTIALLY DONE: relDegree

	//storing the component number, there are no isolated nodes now
	EdgeArray<int> component(*m_pCopy);

	StaticSPQRTree spqrTree(*m_pCopy);

	//Return if there are no R-nodes
	if (spqrTree.numberOfRNodes() == 0)
	{
		//TODO:set node numbers for cliques
		//that are not triconnected
		//each edge is a min. clique for mindegree 1
		//each triangle for mindegree 2?

		return;
	}

	//the degree of the original node
	//within the triconnected component
	NodeArray<int> ccDegree(*m_pCopy, 0);

	for(node v : spqrTree.tree().nodes)
	{
		//we search for dense subgraphs in R-Nodes
		//heuristics:
		//sort the nodes by their degree within the component
		//in descending order, then start cliques by initializing
		//them with the first node and checking the remaining,
		//starting new cliques with nodes that don't fit in the
		//existing cliques (stored in cliqueList)

		if (spqrTree.typeOf(v) == SPQRTree::NodeType::RNode)
		{
			//retrieve the skeleton
			Skeleton &s = spqrTree.skeleton(v);
			Graph &skeletonG = s.getGraph();

			//list of cliques
			List< List<node>* > cliqueList;

			//we insert all nodes into a list to sort them
			List<node> sortList;

			//save the usable edges within the triconnected component
			EdgeArray<bool> usableEdge(*m_pCopy, false);

			//derive the degree of the original node
			//within the triconnected component
			for(node w : skeletonG.nodes)
			{
				node vOrig = s.original(w);

				for(adjEntry adj : w->adjEntries) {
					edge goodEdge = s.realEdge(adj->theEdge());
					bool isGoodEdge = goodEdge != nullptr;
					if (isGoodEdge) isGoodEdge = m_pCopy->original(goodEdge) != nullptr;
#if 0
					if (s.realEdge(eSkel))
#else
					if (isGoodEdge)
#endif
					{
						ccDegree[vOrig]++;
						usableEdge[goodEdge] = true;
					}
				}

				sortList.pushBack(vOrig);
			}

			//sort the nodes corresponding to their degree
			sortList.quicksort(GenericComparer<node, int, false>(ccDegree));

			ListIterator<node> itNode = sortList.begin();

			while(itNode.valid())
			{

				//hier erst vergleichen, ob Knoten Grad > aktcliquengroesse,
				//dann ob mit clique verbunden
				//alternativ koennte man stattdessen fuer jeden gewaehlten
				//Knoten nur noch seine Nachbarn als Kandidaten zulassen
				//hier sollte man mal ein paar Strategien testen, z.B.
				//streng nach Listenordnung vorgehen oder eine "Breitensuche"
				//vom Startknoten aus..
				node vCand = *itNode;

				//node can appear in several 3connected components
				if (m_usedNode[vCand])
				{
					++itNode;
					continue;
				}

				//if there are only "small" degree nodes left, we stop
#if 0
				if (vCand->degree() < minDegree)
#else
				if (ccDegree[vCand] < minDegree)
#endif
					break;

				//successively check the node against the existing
				//clique candidates

				//run through the clique candidates to find a matching
				//node set
				bool setFound = false;
				ListIterator< List<node>* > itCand = cliqueList.begin();
				while (itCand.valid())
				{

					//in the case of cliques, the node needs min degree
					//greater or equal to current clique size
					//TODO: adapt to dense subgraphs
					bool isCand = false;
					if (m_density == 100) {
						isCand = (vCand->degree() >= (*itCand)->size());
					} else {
						isCand = (vCand->degree() >= ceil(m_density*(*itCand)->size()/100.0));
					}
					if (isCand
					// TODO: insert adjacency oracle here to speed
					//       up the check?
					// TODO: check if change from clique to dense subgraph criterion
					//       violates some preconditions for our search
					// but for now:
					 && allAdjacent(vCand, *itCand))
					{
						OGDF_ASSERT(m_usedNode[*itNode] == false);
						(*itCand)->pushBack(*itNode);
						setFound = true;
						m_usedNode[*itNode] = true;

						// sort the clique after insertion of the node
						auto itSearch = itCand.pred();
						if (itSearch.valid()) {
							while (itSearch.valid()
							    && (*itCand)->size() > (*itSearch)->size()) {
								--itSearch;
							}
							// If valid, move behind itSearch, else move to front
							if (itSearch.valid()) {
								cliqueList.moveToSucc(itCand, itSearch);
							} else {
								cliqueList.moveToFront(itCand);
							}
						}
						break;
					}
					// XXX: if list is always sorted, you can break here if not isCand

					++itCand;
				}

				//create a new candidate if necessary
				if (!setFound)
				{
					List<node>* cliqueCandidate = new List<node>();
					itCand = cliqueList.pushBack(cliqueCandidate);
					OGDF_ASSERT(m_usedNode[*itNode] == false);
					cliqueCandidate->pushBack(*itNode);
					m_usedNode[*itNode] = true;

				}

				++itNode;
			}

			//TODO: cliquelist vielleicht durch einen member ersetzen
			//und nicht das delete vergessen!
#ifdef OGDF_DEBUG
#if 0
			int numC1 = cliqueList.size();

			int nodeNum = 0;
			for (List<node> *pL : cliqueList)
			{
				if ( pL->size() > minDegree )
					nodeNum = nodeNum + pL->size();
			}
#endif
			checkCliques(cliqueList, false);
#if 0
			double realTime;
			ogdf::usedTime(realTime);
#endif
#endif
			postProcessCliques(cliqueList, usableEdge);
#ifdef OGDF_DEBUG
#if 0
			realTime = ogdf::usedTime(realTime);

			int nodeNum2 = 0;
			for (List<node> *pL : cliqueList)
			{
				if ( pL->size() > minDegree )
					nodeNum2 = nodeNum2 + pL->size();
			}
			if (nodeNum2 > nodeNum)
			{
				std::cout << "\nAnzahl Cliquen vor PP: " << numC1 << "\n";
				std::cout << "Anzahl Cliquen nach PP: " << cliqueList.size() << "\n";
				std::cout << "Anzahl Knoten in grossen Cliquen: " << nodeNum << "\n";
				std::cout << "Anzahl Knoten danach in grossen Cliquen: " << nodeNum2 << "\n\n";
			}
			std::cout << "Used postprocessing time: " << realTime << "\n" << std::flush;
#endif
			checkCliques(cliqueList, false);
#endif

			//now we run through the list until the remaining node sets
			//are to small to be of interest
			for(List<node> *pCand : cliqueList)
			{
				if ( pCand->size() <= minDegree ) break;

				for(node u : *pCand)
				{
					OGDF_ASSERT(m_copyCliqueNumber[u] == -1);
					m_copyCliqueNumber[u] = m_numberOfCliques;
				}
				m_numberOfCliques++;
			}
			//TODO: only set numbers if return value is not a list
			//of clique node lists
			setResults(cliqueList);

			//free the allocated memory
			for(List<node> *pCl : cliqueList)
			{
				delete pCl;
			}
		}
	}
}

//revisits cliques that are bad candidates and rearranges them,
//using only edges with usableeEdge == true
void CliqueFinder::postProcessCliques(
	List< List<node>* > &cliqueList,
	EdgeArray<bool> &usableEdge)
{
	//TODO:hier aufpassen, das man nicht Knoten ausserhalb des
	//R-Knotens nimmt
	if (m_postProcess == postProcess::None) return;

	//we run over all leftover nodes and try to find cliques
	List<node> leftOver;

	//list of additional cliques
	List< List<node>* > cliqueAdd;

	//First we check the nodes set by the
	//heuristic for dense subgraphs
	//best would be to reinsert them immediatedly after
	//each found subgraph to allow reuse
	if (m_density != 100) {
		for(List<node> *pCand : cliqueList)
		{
			if (pCand->size() > m_minDegree)
			{
				NodeArray<bool> inList(*m_pCopy, false);
				for(node u : *pCand) {
					inList[u] = true;
				}

				ListIterator<node> itNode = pCand->begin();
				while (itNode.valid())
				{
					int adCount = 0; //counts number of nodes adj. to *itNode in itCand
					//check if inGraph degree is high enough
					//and allow reuse otherwise
					adjEntry adE = (*itNode)->firstAdj();
					for (int i = 0; i < (*itNode)->degree(); i++)
					{
						if (usableEdge[adE->theEdge()]
						 && inList[adE->twinNode()]) {
							adCount++;
						}
						adE = adE->cyclicSucc();
					}

					//now delete nodes if connectivity too small
					if (OGDF_GEOM_ET.less(adCount, (int) ceil((pCand->size() - 1)*m_density / 100.0)))
					{
						leftOver.pushBack(*itNode);
						m_usedNode[*itNode] = false;
						inList[*itNode] = false;
						ListIterator<node> itDel = itNode;
						++itNode;
						pCand->del(itDel);
						continue;
					}
					++itNode;
				}
			} else {
				break;
			}
		}
	}

	ListIterator< List<node>* > itCand = cliqueList.begin();
	while (itCand.valid())
	{
		if ((*itCand)->size() <= m_minDegree)
		{
			while (!((*itCand)->empty()))
			{
				node v = (*itCand)->popFrontRet();
				leftOver.pushBack(v);
				m_usedNode[v] = false;
				OGDF_ASSERT(!m_usedNode[v]);
				OGDF_ASSERT(m_copyCliqueNumber[v] == -1);
			}
			ListIterator< List<node>* > itDel = itCand;
			delete *itDel;
			++itCand;
			cliqueList.del(itDel);//del
			//Todo: remove empty lists here
			continue;
		}

		++itCand;
	}

	//now we have all left over nodes in list leftOver
	//vorsicht:  wenn wir hier evaluate benutzen, duerfen wir
	//nicht Knoten hinzunehmen, die schon in Cliquen benutzt sind
	NodeArray<int> value(*m_pCopy);
	for(node u : leftOver)
	{
		node vVal = (u);
		value[vVal] = evaluate(vVal, usableEdge);
	}
	leftOver.quicksort(GenericComparer<node, int, false>(value));

	//now start a new search at the most qualified nodes
	//TODO: Option: wieviele?
	ListIterator<node> itNode = leftOver.begin();
	while (itNode.valid())
	{
		//some nodes can already be assigned in earlier iterations
		if (m_usedNode[*itNode])
		{
			++itNode;
			continue;
		}

		//TODO: hier an dense subgraphs anpassen

		//TODO: this is inefficient because we already
		//ran through the neighbourhood
		//this is the same loop as in evaluate and
		//should not be run twice, but its not efficient
		//to save the neighbour degree values for every
		//run of evaluate

		NodeArray<bool> neighbour(*m_pCopy, false);
		NodeArray<int>  neighbourDegree(*m_pCopy, 0);
		node v = *itNode;
		OGDF_ASSERT(!m_usedNode[v]);
		for(adjEntry adj1 : v->adjEntries)
		{
			if (!usableEdge[adj1->theEdge()]) continue;
			node w = adj1->twinNode();
			if (!m_usedNode[w]) neighbour[w] = true;
		}

		List<node> *neighbours = new List<node>();
		//this loop counts every triangle (two times)
		//TODO: man kann besser oben schon liste neighbours fuellen
		//und hier nur noch darueber laufen
		for(adjEntry adj1 : v->adjEntries)
		{
#if 0
			if (!usableEdge[adj1->theEdge()]) continue;
#endif
			node w = adj1->twinNode();
#if 0
			if (m_usedNode[w]) continue; //dont use paths over other cliques
#endif
			if (!neighbour[w]) continue;
			OGDF_ASSERT(!m_usedNode[w]);

			OGDF_ASSERT(m_copyCliqueNumber[w] == -1);
			neighbours->pushBack(w);
			neighbourDegree[w]++; //connected to v

			for(adjEntry adj2 : w->adjEntries)
			{
				if (!usableEdge[adj2->theEdge()]) continue;
				node u = adj2->twinNode();
				if (m_usedNode[u]) continue; //dont use paths over other cliques
				if (neighbour[u])
				{
					neighbourDegree[w]++;
				}
			}
		}

		//now we have a (dense) set of nodes and we can
		//delete the nodes with the smallest degree up
		//to a TODO certain amount
		neighbours->quicksort(GenericComparer<node, int, false>(neighbourDegree));

#if 0
		neighbours->clear();
#endif
		findClique(v, *neighbours);
#if 0
		ListIterator<node> itDense = neighbours->rbegin();
		while (itDense.valid())
		{
			//TODO: hier die Bedingung an Dichte statt
			//Maximalgrad
			//removes all possible but one clique around v
			if (neighbourDegree[*itDense] < neighbours->size())
				neighbours->del(itDense);

			itDense--;
		}
#endif

		//hier noch usenode setzen und startknoten hinzufuegen
		//we found a dense subgraph
		if (neighbours->size() >= m_minDegree)
		{
			OGDF_ASSERT(allAdjacent(v, neighbours));
			neighbours->pushFront(v);

			for(node vUsed : *neighbours)
			{
				OGDF_ASSERT(m_usedNode[vUsed] == false);
				//TODO: hier gleich die liste checken
				m_usedNode[vUsed] = true;
			}
			cliqueAdd.pushBack(neighbours);
#ifdef OGDF_DEBUG
			OGDF_ASSERT(cliqueOK(neighbours));
#endif
		} else {
			ListIterator<node> itUsed = neighbours->begin();
			while (itUsed.valid())
			{
				ListIterator<node> itDel = itUsed;
#if 0
				delete *itDel;
#endif
				++itUsed;
				neighbours->del(itDel);
#if 0
				neighbours->remove(itDel);
#endif
			}
#if 0
			neighbours->clear();
#endif
			delete neighbours; //hier vielleicht clear?
		}

		++itNode;
	}

	cliqueList.conc(cliqueAdd);
}

int CliqueFinder::evaluate(node v, EdgeArray<bool> &usableEdge)
{
	int value = 0;
	//Simple version: run through the neighbourhood of
	//every node and try to find a triangle path back
	//(triangles over neighbours are mandatory for cliques)
	//we run through the neighbourhood of v up to depth
	//2 and check if we can reach v again
	//erst mal die einfache Variante ohne Zwischenspeicherung
	//der Ergebnisse fuer andere Knoten
	//TODO: Geht das auch effizienter als mit Nodearray?
	NodeArray<bool> neighbour(*m_pCopy, false);
	for(adjEntry adj1 : v->adjEntries)
	{
		if (!usableEdge[adj1->theEdge()]) continue;
		node w = adj1->twinNode();
		if (!m_usedNode[w]) neighbour[w] = true;
	}

	//this loop counts every triangle (twice)
	for(adjEntry adj1 : v->adjEntries)
	{
		if (!usableEdge[adj1->theEdge()]) continue;
		node w = adj1->twinNode();
		if (m_usedNode[w]) continue; //dont use paths over other cliques
		for(adjEntry adj2 : w->adjEntries)
		{
			if (!usableEdge[adj2->theEdge()]) continue;
			node u = adj2->twinNode();
			if (m_usedNode[u]) continue; //dont use paths over other cliques
			if (neighbour[u])
			{
				value++;
			}
		}
	}
	return value;
}

//searchs for a clique around node v in list neighbours
//runs through list and performs numRandom additional
//random runs, permuting the sequence
//the result is returned in the list neighbours
void CliqueFinder::findClique(
	node v,
	List<node> &neighbours,
	int numRandom)
{
	//TODO:should be realdegree
	if (v->degree() < m_minDegree) neighbours.clear();
	List<node> clique;  //used to check clique criteria
	OGDF_ASSERT(!m_usedNode[v]);
	clique.pushBack(v); //mandatory
	//we could assume that neighbours are always neighbours
	//of v and push the first node. too, here
	ListIterator<node> itNode = neighbours.begin();
	while (itNode.valid())
	{
		if ( ((*itNode)->degree() < clique.size()) ||
			((*itNode)->degree() < m_minDegree) )
		{
			ListIterator<node> itDel = itNode;
#if 0
			delete *itDel;
#endif
			++itNode;
			neighbours.del(itDel); //remove

			continue;
		}
		if (allAdjacent((*itNode), &clique))
		{
			clique.pushBack((*itNode));
			++itNode;
		}
		else
		{
			ListIterator<node> itDel = itNode;
			++itNode;
			neighbours.del(itDel);
		}
	}
	//we can stop if the degree is smaller than the clique size
#if 0
	int k;
	for (k = 0; k <numRandom; k++)
	{
		//TODO
	}
#endif
}

void CliqueFinder::setResults(NodeArray<int> & cliqueNum)
{
	//Todo: Assert that cliqueNum is an array on m_pGraph
	//set the clique numbers for the original nodes, -1 if not
	//member of a clique

	for(node v : m_pGraph->nodes)
	{
		node w = m_pCopy->copy(v);
		OGDF_ASSERT(w);
		cliqueNum[v] = m_copyCliqueNumber[w];

	}
}

//set results values from copy node lists to original node
void CliqueFinder::setResults(List< List<node>* > &cliqueLists)
{
	if (!m_callByList) return;
	if (!m_pList)      return;

	//m_pList->clear(); //componentwise
	//run through the clique lists
	for(List<node> *pL : cliqueLists)
	{
		List<node> l_list; //does this work locally?
		//run through the cliques
		for(node v : *pL)
		{
			node u = m_pCopy->original(v);
			if (u)
				l_list.pushBack(u);
		}

		m_pList->pushBack(l_list);
	}
}

//same as above
void CliqueFinder::setResults(List< List<node> > &cliqueLists)
{
#if 0
	//run through the clique lists
	for(List<node> &L : cliqueLists) {
		//run through the cliques
		for(node v : L) {
		}
	}
#endif
	// FIXME: does not do anything, hence commented out
}

//Graph must be parallelfree
//checks if v is adjacent to (min. m_density percent of) all nodes in vList
inline bool CliqueFinder::allAdjacent(node v, List<node>* vList) const
{
	int threshold = int(ceil(max(1.0, ceil((vList->size()*m_density/100.0)))));
	//we do not want to run into some rounding error if m_density == 100
	if (m_density == 100) {if (v->degree() < vList->size()) return false;}
	else if (OGDF_GEOM_ET.less(v->degree(), threshold)) return false;
	if (vList->size() == 0) return true;
	int adCount = 0;
	//Check: can the runtime be improved, e.g. by an adjacency oracle
	//or by running degree times through the list?
	NodeArray<bool> inList(*m_pCopy, false);//(v->graphOf()), false);
	for(node u : *vList)
	{
		inList[u] = true;
	}

	adjEntry adE = v->firstAdj();
	for (int i = 0; i < v->degree(); i++)
	{
		if (inList[adE->twinNode()])
			adCount++;
		adE = adE->cyclicSucc();
	}

	//we do not want to run into some rounding error if m_density == 100
	if (m_density == 100) { if (adCount == vList->size()) return true;}
	else if (OGDF_GEOM_ET.geq(adCount, threshold))
		return true;

	return false;
}

//check

#ifdef OGDF_DEBUG
void CliqueFinder::checkCliques(List< List<node>* > &cliqueList, bool sizeCheck)
{
	//check if size is ok and if all nodes are mutually connected
	for (List<node> *pL : cliqueList) {
		if (sizeCheck) {
			OGDF_ASSERT(pL->size()> m_minDegree);
		}
		OGDF_ASSERT(cliqueOK(pL));
	}
}
#endif

bool CliqueFinder::cliqueOK(List<node> *clique) const
{
	bool result = true;
	NodeArray<int> connect(*m_pCopy, 0);

	for(node v : *clique)
	{
		for(adjEntry adj1 : v->adjEntries)
		{
			connect[adj1->twinNode()]++;

		}
	}
	for(node v : *clique)
	{
		if (m_density == 100)
		{
			if (connect[v] < (clique->size()-1))
				return false;
		}
		else
		{
			//due to the current heuristics, we can not guarantee any value
			//TODO:postprocess and delete all "bad" nodes
#if 0
			double minVal = (clique->size()-1)*m_density/100.0;
			if (OGDF_GEOM_ET.less(connect[v], minVal))
				return false;
#endif
		}
	}

	return result;
}


//output
#ifdef OGDF_DEBUG
void CliqueFinder::writeGraph(Graph &G, NodeArray<int> &cliqueNumber, const char *fileName)
{
	GraphAttributes GA(G, GraphAttributes::nodeGraphics | GraphAttributes::nodeStyle | GraphAttributes::nodeLabel);

	for(node v : G.nodes)
	{
		int num = cliqueNumber[v];
		int col1, col2, col3;
		if (num != -1)
		{
			col1 = abs(((num * 191) + 123) % 256);
			col2 = abs(((num * 131) + 67) % 256);
			col3 = abs(((num * 7) + 17) % 256);
		}
		else col1 = col2 = col3 = 0;

		GA.fillColor(v) = Color(col1,col2,col3);
		GA.label(v) = to_string(num);

	}

	std::ofstream of(fileName);
	GraphIO::writeGML(GA, of);
}
#endif

}
