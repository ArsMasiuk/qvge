/** \file
 * \brief Definition of ogdf::MinCostFlowReinelt class template
 *
 * \author Carsten Gutwenger and Gerhard Reinelt
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

#include <ogdf/graphalg/MinCostFlowModule.h>
#include <ogdf/basic/EpsilonTest.h>

namespace ogdf {

//! Computes a min-cost flow using a network simplex method.
/**
 * @ingroup ga-flow
 */
template<typename TCost>
class MinCostFlowReinelt : public MinCostFlowModule<TCost>
{
public:
	MinCostFlowReinelt()
	  : m_eps()
	{
	}

	using MinCostFlowModule<TCost>::call;

	/**
	* \brief Computes a min-cost flow in the directed graph \p G using a network simplex method.
	*
	* \pre \p G must be connected, \p lowerBound[\a e] <= \p upperBound[\a e]
	*      for all edges \a e, and the sum over all supplies must be zero.
	*
	* @param G is the directed input graph.
	* @param lowerBound gives the lower bound for the flow on each edge.
	* @param upperBound gives the upper bound for the flow on each edge.
	* @param cost gives the costs for each edge.
	* @param supply gives the supply (or demand if negative) of each node.
	* @param flow is assigned the computed flow on each edge.
	* @param dual is assigned the computed dual variables.
	* \return true iff a feasible min-cost flow exists.
	*/
	virtual bool call(
		const Graph &G,                   // directed graph
		const EdgeArray<int> &lowerBound, // lower bound for flow
		const EdgeArray<int> &upperBound, // upper bound for flow
		const EdgeArray<TCost> &cost,     // cost of an edge
		const NodeArray<int> &supply,     // supply (if neg. demand) of a node
		EdgeArray<int> &flow,			  // computed flow
		NodeArray<TCost> &dual) override;   // computed dual variables

	int infinity() const { return std::numeric_limits<int>::max(); }

private:

	struct arctype;

	struct nodetype {
		nodetype *father;     /* ->father in basis tree */
		nodetype *successor;  /* ->successor in preorder */
		arctype *arc_id;      /* ->arc (node,father) */
		bool orientation;     /* false<=>basic arc=(father->node)*/
		TCost dual;             /* value of dual variable */
		int flow;             /* flow in basic arc (node,father) */
		int name;             /* identification of node = node-nr*/
		nodetype *last;       /* last node in subtree */
		int nr_of_nodes;      /* number of nodes in subtree */
	};

	struct arctype {
		arctype *next_arc;    /* -> next arc in list */
		nodetype *tail;       /* -> tail of arc */
		nodetype *head;       /* -> head of arc */
		TCost cost;           /* cost of unit flow */
		int upper_bound;      /* capacity of arc */
		int arcnum;           /* number of arc in input */

		OGDF_NEW_DELETE
	};


	int mcf(
		int mcfNrNodes,
		int mcfNrArcs,
		Array<int> &mcfSupply,
		Array<int> &mcfTail,
		Array<int> &mcfHead,
		Array<int> &mcfLb,
		Array<int> &mcfUb,
		Array<TCost> &mcfCost,
		Array<int> &mcfFlow,
		Array<TCost> &mcfDual,
		TCost *mcfObj
	);

	void start(Array<int> &supply);

	void beacircle(arctype **eplus, arctype **pre, bool *from_ub);
	void beadouble(arctype **eplus, arctype **pre, bool *from_ub);

	EpsilonTest m_eps;

	Array<nodetype> nodes;     /* node space */
	Array<arctype> arcs;       /* arc space */
	//Array<nodetype *> p;    /*used for starting procedure*/

	nodetype *root = nullptr;         /*->root of basis tree*/
	nodetype rootStruct;

	arctype *last_n1 = nullptr;       /*->start for search for entering arc in N' */
	arctype *last_n2 = nullptr;       /*->start for search for entering arc in N''*/
	arctype *start_arc = nullptr;     /* -> initial arc list*/
	arctype *start_b = nullptr;       /* -> first basic arc*/
	arctype *start_n1 = nullptr;      /* -> first nonbasic arc in n'*/
	arctype *start_n2 = nullptr;      /* -> first nonbasic arc in n''*/
	arctype *startsearch = nullptr;   /* ->start of search for basis entering arc */
	arctype *searchend = nullptr;     /* ->end of search for entering arc in bea */
	arctype *searchend_n1 = nullptr;  /*->end of search for entering arc in N' */
	arctype *searchend_n2 = nullptr;  /*->end of search for entering arc in N''*/

	//int artvalue;          /*cost and upper_bound of artificial arc */
	TCost m_maxCost = std::numeric_limits<TCost>::lowest(); // maximum of the cost of all input arcs

	int nn = 0;                /*number of original nodes*/
	int mm = 0;                /*number of original arcs*/
};

}

// Implementation

namespace ogdf {

// computes min-cost-flow, call front-end for mcf()
// returns true if a feasible minimum cost flow could be found
template<typename TCost>
bool MinCostFlowReinelt<TCost>::call(
	const Graph &G,
	const EdgeArray<int> &lowerBound,
	const EdgeArray<int> &upperBound,
	const EdgeArray<TCost> &cost,
	const NodeArray<int> &supply,
	EdgeArray<int> &flow,
	NodeArray<TCost> &dual)
{
	OGDF_ASSERT(this->checkProblem(G, lowerBound, upperBound, supply));

	const int n = G.numberOfNodes();
	const int m = G.numberOfEdges();

	// assign indices 0, ..., n-1 to nodes in G
	// (this is not guaranteed for v->index() )
	NodeArray<int> vIndex(G);
	// assigning supply
	Array<int> mcfSupply(n);

	int i = 0;
	for(node v : G.nodes) {
		mcfSupply[i] = supply[v];
		vIndex[v] = ++i;
	}


	// allocation of arrays for arcs
	Array<int> mcfTail(m);
	Array<int> mcfHead(m);
	Array<int> mcfLb(m);
	Array<int> mcfUb(m);
	Array<TCost> mcfCost(m);
	Array<int> mcfFlow(m);
	Array<TCost> mcfDual(n+1); // dual[n] = dual variable of root struct

	// set input data in edge arrays
	int nSelfLoops = 0;
	i = 0;
	for (edge e : G.edges) {
		// We handle self-loops in the network already in the front-end
		// (they are just set to the lower bound below when copying result)
		if (e->isSelfLoop()) {
			++nSelfLoops;
			continue;
		}

		mcfTail[i] = vIndex[e->source()];
		mcfHead[i] = vIndex[e->target()];
		mcfLb[i] = lowerBound[e];
		mcfUb[i] = upperBound[e];
		mcfCost[i] = cost[e];

		++i;
	}


	int retCode = 0; // return (error or success) code
	TCost objVal;  // value of flow

	// call actual min-cost-flow function
	// mcf does not support single nodes
	if (n > 1) {
		//mcf does not support single edges
		if (m < 2) {
			if (m == 1) {
				edge eFirst = G.firstEdge();
				flow[eFirst] = lowerBound[eFirst];
			}
		} else {
			retCode = mcf(n, m - nSelfLoops, mcfSupply, mcfTail, mcfHead, mcfLb, mcfUb, mcfCost, mcfFlow, mcfDual, &objVal);
		}
	}

	// copy resulting flow for return
	i = 0;
	for (edge e : G.edges) {
		if (e->isSelfLoop()) {
			flow[e] = lowerBound[e];
			continue;
		}

		flow[e] = mcfFlow[i];
		if (retCode == 0) {
			OGDF_ASSERT(flow[e] >= lowerBound[e]);
			OGDF_ASSERT(flow[e] <= upperBound[e]);
		}
		++i;
	}

	// copy resulting dual values for return
	i = 0;
	for (node v : G.nodes) {
		dual[v] = mcfDual[i];
		++i;
	}

	// successful if retCode == 0
	return retCode == 0;
}


template<typename TCost>
void MinCostFlowReinelt<TCost>::start(Array<int> &supply)
{
	// determine intial basis tree and initialize data structure

	/* initialize artificial root node */
	root->father = root;
	root->successor = &nodes[1];
	root->arc_id = nullptr;
	root->orientation = false;
	root->dual = 0;
	root->flow = 0;
	root->nr_of_nodes = nn + 1;
	root->last = &nodes[nn];
	root->name = nn + 1;
	// artificials = nn; moved to mcf() [CG]
	TCost highCost = 1 + (nn+1) * m_maxCost;

	for (int i = 1; i <= nn; ++i) {   /* for every node an artificial arc is created */
		arctype *ep = new arctype;
		if (supply[i - 1] >= 0) {
			ep->tail = &nodes[i];
			ep->head = root;
		} else {
			ep->tail = root;
			ep->head = &nodes[i];
		}
		ep->cost = highCost;
		ep->upper_bound = infinity();
		ep->arcnum = mm + i - 1;
		ep->next_arc = start_b;
		start_b = ep;
		nodes[i].father = root;
		if (i < nn)
			nodes[i].successor = &nodes[i+1];
		else
			nodes[i].successor = root;
		if (supply[i - 1] < 0) {
			nodes[i].orientation = false;
			nodes[i].dual = -highCost;
		} else {
			nodes[i].orientation = true;
			nodes[i].dual = highCost;
		}
		nodes[i].flow = abs(supply[i - 1]);
		nodes[i].nr_of_nodes = 1;
		nodes[i].last = &nodes[i];
		nodes[i].arc_id = ep;
	}  /* for i */
	start_n1 = start_arc;
}  /*start*/


// circle variant for determine basis entering arc
template<typename TCost>
void MinCostFlowReinelt<TCost>::beacircle(
	arctype **eplus,
	arctype **pre,
	bool *from_ub)
{
	//the first arc with negative reduced costs is taken, but the search is
	//started at the successor of the successor of eplus in the last iteration

	bool found = false;   /* true<=>entering arc found */

	*pre = startsearch;
	if (*pre != nullptr) {
		*eplus = (*pre)->next_arc;
	} else {
		*eplus = nullptr;
	}
	searchend = *eplus;

	if (!*from_ub) {

		while (*eplus != nullptr && !found) { /* search in n' for an arc with negative reduced costs */
			if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found) {  /* entering arc still not found */
			/* search in n'' */
			*from_ub = true;
			*eplus = start_n2;
			*pre = nullptr;

			while (*eplus != nullptr && !found) {
				if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}
			}  /* while */


			if (!found) {   /* search again in n' */
				*from_ub = false;
				*eplus = start_n1;
				*pre = nullptr;

				while (*eplus != searchend && !found) {
					if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}
				}  /* while */

			}  /* search in n'' */
		}  /* serch again in n' */
	}  /* if from_ub */
	else {  /* startsearch in n'' */

		while (*eplus != nullptr && !found) {
			if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found) {   /* search now in n' */
			*from_ub = false;
			*eplus = start_n1;
			*pre = nullptr;

			while (*eplus != nullptr && !found) {
				if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}
			}  /* while */


			if (!found) {   /* search again in n'' */
				*from_ub = true;
				*eplus = start_n2;
				*pre = nullptr;

				while (*eplus != searchend && !found) {
					if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}
				}  /* while */

			}  /* search in n' */
		}  /* search in n'' */
	}  /* from_ub = true */



	if (!found) {
		*pre = nullptr;
		*eplus = nullptr;
	} else {
		startsearch = (*eplus)->next_arc;
	}

}  /* beacircle */


// doublecircle variant for determine basis entering arc
template<typename TCost>
void MinCostFlowReinelt<TCost>::beadouble(
	arctype **eplus,
	arctype **pre,
	bool *from_ub)
{
	/* search as in procedure beacircle, but in each list the search started is
	at the last movement
	*/
	bool found = false;   /* true<=>entering arc found */

	if (!*from_ub) {
		*pre = last_n1;
		if (*pre != nullptr)
			*eplus = (*pre)->next_arc;
		else
			*eplus = nullptr;
		searchend_n1 = *eplus;

		while (*eplus != nullptr && !found) {  /* search in n' for an arc with negative reduced costs */
			if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found) {  /* entering arc still not found */
			/* search in n'' beginning at the last movement */
			*from_ub = true;
			*pre = last_n2;
			if (*pre != nullptr)
				*eplus = (*pre)->next_arc;
			else
				*eplus = nullptr;
			searchend_n2 = *eplus;

			while (*eplus != nullptr && !found) {
				if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}

			}  /* while */

			if (!found) {   /* entering arc still not found */
				/* search in n'' in the first part of the list */
				*eplus = start_n2;
				*pre = nullptr;

				while (*eplus != searchend_n2 && !found) {
					if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}

				}  /* while */


				if (!found) {
					/* search again in n' in the first part of the list*/
					*from_ub = false;
					*eplus = start_n1;
					*pre = nullptr;

					while (*eplus != searchend_n1 && !found) {
						if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
							found = true;
						} else {
							*pre = *eplus;   /* save predecessor */
							*eplus = (*eplus)->next_arc;  /* go to next arc */
						}
					}  /* while */
				}  /* first part n' */
			}  /* first part n'' */
		}  /* second part n'' */
	}  /* if from_ub */
	else {  /* startsearch in n'' */
		*pre = last_n2;
		if (*pre != nullptr)
			*eplus = (*pre)->next_arc;
		else
			*eplus = nullptr;
		searchend_n2 = *eplus;

		while (*eplus != nullptr && !found) {
			if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found) {   /* search now in n' beginning at the last movement */
			*from_ub = false;
			*pre = last_n1;
			if (*pre != nullptr)
				*eplus = (*pre)->next_arc;
			else
				*eplus = nullptr;
			searchend_n1 = *eplus;

			while (*eplus != nullptr && !found) {
				if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}
			}  /* while */


			if (!found) {   /* search now in n' in the first part */
				*eplus = start_n1;
				*pre = nullptr;

				while (*eplus != searchend_n1 && !found) {
					if (m_eps.less((*eplus)->cost + (*eplus)->head->dual, (*eplus)->tail->dual)) {
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}
				}  /* while */


				if (!found) {   /* search again in n'' in the first part */
					*from_ub = true;
					*eplus = start_n2;
					*pre = nullptr;

					while (*eplus != searchend_n2 && !found) {
						if (m_eps.less((*eplus)->tail->dual, (*eplus)->head->dual + (*eplus)->cost)) {
							found = true;
						} else {
							*pre = *eplus;   /* save predecessor */
							*eplus = (*eplus)->next_arc;   /* go to next arc */
						}
					}  /* while */
				}  /* first part of n'' */
			}  /* first part of n' */
		}  /* second part of n' */
	}  /* from_ub = true */



	if (!found) {
		*pre = nullptr;
		*eplus = nullptr;
		return;
	}

	if (*from_ub)
		last_n2 = (*eplus)->next_arc;
	else
		last_n1 = (*eplus)->next_arc;

}  /* beadouble */


// Min Cost Flow Function
template<typename TCost>
int MinCostFlowReinelt<TCost>::mcf(
	int mcfNrNodes,
	int mcfNrArcs,
	Array<int> &supply,
	Array<int> &mcfTail,
	Array<int> &mcfHead,
	Array<int> &mcfLb,
	Array<int> &mcfUb,
	Array<TCost> &mcfCost,
	Array<int> &mcfFlow,
	Array<TCost> &mcfDual,
	TCost *mcfObj)
{
	int i;
	int low,up;

	/* 1: Allocations (malloc's no longer required) */

	root = &rootStruct;

	/* 2: Initializations */

	/* Number of nodes/arcs */
	nn = mcfNrNodes;
	OGDF_ASSERT(nn >= 2);
	mm = mcfNrArcs;
	OGDF_ASSERT(mm >= 2);

	// number of artificial basis arcs
	int artificials = nn;


	/* Node space and pointers to nodes */
	nodes.init(nn+1);
	nodes[0].name = -1; // for debuggin, should not occur(?)
	for(i = 1; i <= nn; ++i)
		nodes[i].name = i;

	/* Arc space and arc data */
	arcs.init(mm+1);

	TCost lb_cost = 0; // cost of lower bound
	m_maxCost = 0;
	int from = mcfTail[0]; // name of tail (input)
	int toh = mcfHead[0];  // name of head (input)
	low = mcfLb[0];
	up = mcfUb[0];
	TCost c = mcfCost[0]; // cost (input)
	if (from <= 0 || from > nn || toh <= 0 || toh > nn || up < 0 || low > up || low < 0) {
		return 4;
	}
	TCost abs_c = c < 0 ? -c : c;
	if (abs_c > m_maxCost) {
		m_maxCost = abs_c;
	}

	start_arc = &arcs[1];
	start_arc->tail = &nodes[from];
	start_arc->head = &nodes[toh];
	start_arc->cost = c;
	start_arc->upper_bound = up - low;
	start_arc->arcnum = 0;
	supply[from - 1] -= low;
	supply[toh - 1] += low;
	lb_cost += start_arc->cost * low;

	arctype *e = start_arc;

	int lower; // lower bound (input)
	for (lower = 2; lower <= mm; ++lower) {
		from = mcfTail[lower-1];
		toh = mcfHead[lower-1];
		low = mcfLb[lower-1];
		up = mcfUb[lower-1];
		c = mcfCost[lower-1];
		if (from <= 0 || from > nn || toh <= 0 || toh > nn || up < 0 || low > up || low < 0) {
			return 4;
		}
		abs_c = c < 0 ? -c : c;
		if (abs_c > m_maxCost) {
			m_maxCost = abs_c;
		}

		arctype *ep = &arcs[lower];
		e->next_arc = ep;
		ep->tail = &nodes[from];
		ep->head = &nodes[toh];
		ep->cost = c;
		ep->upper_bound = up - low;
		ep->arcnum = lower-1;
		supply[from-1] -= low;
		supply[toh-1] += low;
		lb_cost += ep->cost * low;
		e = ep;
	}

	e->next_arc = nullptr;
	// feasible = true <=> feasible solution exists
	bool feasible = true;


	/* 3: Starting solution */

	start_n1 = nullptr;
	start_n2 = nullptr;
	start_b = nullptr;

	start(supply);

	int step = 1;   /* initialize iteration counter */

	/* 4: Iteration loop */

	/* 4.1: Determine basis entering arc */

	// finished = true <=> iteration finished
	bool finished = false;
	// from_ub = true <=> entering arc at upper bound
	bool from_ub = false;
	startsearch = start_n1;
#if 0
	startsearchpre = nullptr;
#endif
	last_n1 = nullptr;
	last_n2 = nullptr;
	nodetype *np; // general nodeptr

	do {
		arctype *eplus; // ->basis entering arc
		arctype *pre;   // ->predecessor of eplus in list
		beacircle(&eplus, &pre, &from_ub);

		if (eplus == nullptr) {
			finished = true;
		} else {

			nodetype *iplus = eplus->tail; // -> tail of basis entering arc
			nodetype *jplus = eplus->head; // -> head of basis entering arc

			/* 4.2: Determine leaving arc and maximal flow change */

			int delta = eplus->upper_bound; // maximal flow change
			nodetype *iminus = nullptr; // -> tail of basis leaving arc
			nodetype *p1 = iplus;
			nodetype *p2 = jplus;

			bool to_ub;   // to_ub = true <=> leaving arc goes to upperbound
			bool xchange; // xchange = true <=> exchange iplus and jplus
			while (p1 != p2) {
				if (p1->nr_of_nodes <= p2->nr_of_nodes) {
					np = p1;
					if (from_ub == np->orientation) {
						if (delta > np->arc_id->upper_bound - np->flow) {
							iminus = np;
							delta = np->arc_id->upper_bound - np->flow;
							xchange = false;
							to_ub = true;
						}
					}
					else if (delta > np->flow) {
						iminus = np;
						delta = np->flow;
						xchange = false;
						to_ub = false;
					}
					p1 = np->father;
					continue;
				}
				np = p2;
				if (from_ub != np->orientation) {
					if (delta > np->arc_id->upper_bound - np->flow) {
						iminus = np;
						delta = np->arc_id->upper_bound - np->flow;
						xchange = true;
						to_ub = true;
					}
				}
				else if (delta > np->flow) {
					iminus = np;
					delta = np->flow;
					xchange = true;
					to_ub = false;
				}
				p2 = np->father;
			}
			// paths from iplus and jplus to root meet at w
			nodetype *w = p1;
			nodetype *iw;
			nodetype *jminus;  // -> head of basis leaving arc

			arctype *eminus; /// ->basis leaving arc
			if (iminus == nullptr) {
				to_ub = !from_ub;
				eminus = eplus;
				iminus = iplus;
				jminus = jplus;
			}
			else {
				if (xchange) {
					iw = jplus;
					jplus = iplus;
					iplus = iw;
				}
				jminus = iminus->father;
				eminus = iminus->arc_id;
			}

			// artif_to_lb = true <=> artif. arc goes to lower bound
			bool artif_to_lb = false;
			if (artificials > 1) {
				if (iminus == root || jminus == root) {
					if (jplus != root && iplus != root) {
						artificials--;
						artif_to_lb = true;
					}
					else if (eminus == eplus) {
						if (from_ub) {
							artificials--;
							artif_to_lb = true;
						} else
							artificials++;
					}
				}
				else {
					if (iplus == root || jplus == root)
						artificials++;
				}
			}

			/* 4.3: Update of data structure */

			TCost sigma; // change of dual variables

			if (eminus == eplus) {
				if (from_ub) delta = -delta;

				bool s_orientation;
				s_orientation = eminus->tail == iplus;

				np = iplus;
				while (np != w) {
					if (np->orientation == s_orientation) {
						np->flow -= delta;
					}
					else {
						np->flow += delta;
					}
					np = np->father;
				}

				np = jplus;
				while (np != w) {
					if (np->orientation == s_orientation) {
						np->flow += delta;
					}
					else {
						np->flow -= delta;
					}
					np = np->father;
				}

			} else {
				/* 4.3.2.1 : initialize sigma */

				if (eplus->tail == iplus)
					sigma = eplus->cost + jplus->dual - iplus->dual;
				else
					sigma = jplus->dual - iplus->dual - eplus->cost;

				// 4.3.2.2 : find new succ. of jminus if current succ. is iminus

				nodetype *newsuc = jminus->successor; // -> new successor
				if (newsuc == iminus) {
					for (i= 1; i <= iminus->nr_of_nodes; ++i) {
						newsuc = newsuc->successor;
					}
				}

				/* 4.3.2.3 : initialize data for iplus */

				nodetype *s_father = jplus; // save area
				bool s_orientation = (eplus->tail != jplus);

				// eplus_ori = true <=> eplus = (iplus,jplus)
				bool eplus_ori = s_orientation;

				int s_flow;
				if (from_ub) {
					s_flow = eplus->upper_bound - delta;
					delta = -delta;
				} else {
					s_flow = delta;
				}

				arctype *s_arc_id = eminus;
				int oldnumber = 0;
				nodetype *nd = iplus;     // -> current node
				nodetype *f = nd->father; // ->father of nd

				/* 4.3.2.4 : traverse subtree under iminus */

				while (nd != jminus) {
					nodetype *pred = f; // ->predecessor of current node
					while (pred->successor != nd) pred = pred->successor;
					nodetype *lastnode = nd; // -> last node of subtree
					i = 1;
					int non = nd->nr_of_nodes - oldnumber;
					while (i < non) {
						lastnode = lastnode->successor;
						lastnode->dual += sigma;
						i++;
					}
					nd->dual += sigma;
					pred->successor = lastnode->successor;

					if (nd != iminus) lastnode->successor = f;
					else lastnode->successor = jplus->successor;

					nodetype *w_father = nd; // save area
					arctype *w_arc_id = nd->arc_id; // save area

					bool w_orientation;
					w_orientation = nd->arc_id->tail != nd;

					int w_flow;
					if (w_orientation == eplus_ori) {
						w_flow = nd->flow + delta;
					}
					else {
						w_flow = nd->flow - delta;
					}

					nd->father = s_father;
					nd->orientation = s_orientation;
					nd->arc_id = s_arc_id;
					nd->flow = s_flow;
					s_father = w_father;
					s_orientation = w_orientation;
					s_arc_id = w_arc_id;
					s_flow = w_flow;

					oldnumber = nd->nr_of_nodes;
					nd = f;
					f = f->father;

				}

				jminus->successor = newsuc;
				jplus->successor = iplus;

				// 4.3.2.5: assign new nr_of_nodes in path from iminus to iplus

				oldnumber = iminus->nr_of_nodes;
				np = iminus;
				while (np != iplus) {
					np->nr_of_nodes = oldnumber - np->father->nr_of_nodes;
					np = np->father;
				}

				iplus->nr_of_nodes = oldnumber;

				// 4.3.2.6: update flows and nr_of_nodes in path from jminus to w

				np = jminus;
				while (np != w) {
					np->nr_of_nodes -= oldnumber;
					if (np->orientation != eplus_ori) {
						np->flow += delta;
					}
					else {
						np->flow -= delta;
					}
					np = np->father;
				}

				// 4.3.2.7 update flows and nr_of_nodes in path from jplus to w

				np = jplus;
				while (np != w) {
					np->nr_of_nodes += oldnumber;
					if (np->orientation == eplus_ori) {
						np->flow += delta;
					}
					else {
						np->flow -= delta;
					}
					np = np->father;
				}

			}

			/* 4.4: Update lists B, N' and N'' */

			if (eminus == eplus) {
				if (!from_ub) {
					if (pre == nullptr)
						start_n1 = eminus->next_arc;
					else
						pre->next_arc = eminus->next_arc;

					eminus->next_arc = start_n2;
					start_n2 = eminus;
				} else {
					if (pre == nullptr)
						start_n2 = eminus->next_arc;
					else
						pre->next_arc = eminus->next_arc;
					eminus->next_arc = start_n1;
					start_n1 = eminus;
				}
			} else {
				TCost wcost = eminus->cost;
				int wub = eminus->upper_bound;
				int wnum = eminus->arcnum;
				nodetype *w_head = eminus->head;
				nodetype *w_tail = eminus->tail;
				eminus->tail = eplus->tail;
				eminus->head = eplus->head;
				eminus->upper_bound = eplus->upper_bound;
				eminus->arcnum = eplus->arcnum;
				eminus->cost = eplus->cost;
				eplus->tail = w_tail;
				eplus->head = w_head;
				eplus->upper_bound = wub;
				eplus->cost = wcost;
				eplus->arcnum = wnum;
				arctype *ep = eplus;

				if (pre != nullptr)
					pre->next_arc = ep->next_arc;
				else {
					if (from_ub) start_n2 = ep->next_arc;
					else start_n1 = ep->next_arc;
				}

				if (to_ub) {
					ep->next_arc = start_n2;
					start_n2 = ep;
				} else {
					if (!artif_to_lb) {
						ep->next_arc = start_n1;
						start_n1 = ep;
					}
				}
			}

			step++;

			/* 4.5: Eliminate artificial arcs and artificial root node */

			if (artificials == 1) {
				artificials = 0;
				nodetype *nd = root->successor;
				arctype *e1 = nd->arc_id;

				if (nd->flow>0) {
					feasible = false;
					finished = true;
				} else {
					feasible = true;
					if (e1 == start_b) {
						start_b = e1->next_arc;
					} else {
						e = start_b;
						while (e->next_arc != e1)
							e = e->next_arc;
						e->next_arc = e1->next_arc;
					}

					iw = root;
					root = root->successor;
					root->father = root;
					sigma = root->dual;

					np = root;
					while (np->successor != iw) {
						np->dual -= sigma;
						np = np->successor;
					}

					np->dual -= sigma;
					np->successor = root;

				}

			}

		}

	} while (!finished);

	/* 5: Return results */

	/* Feasible solution? */
	if (artificials != 0
	 && feasible) {
		np = root->successor;
		do {
			if (np->father == root
			 && np->flow > 0) {
				feasible = false;
				np = root;
			}
			else
				np = np->successor;
		} while (np != root);

		arctype *ep = start_n2;
		while (ep != nullptr && feasible) {
			if (ep == nullptr)
				break;
			if (ep->tail == root && ep->head == root)
				feasible = false;
			ep = ep->next_arc;
		}
	}

	int retValue = 0;

	if (feasible) {
		/* Objective function value */
		TCost zfw = 0; // current total cost
		np = root->successor;
		while (np != root) {
			if (np->flow != 0) {
				zfw += np->flow * np->arc_id->cost;
			}
			np = np->successor;
		}
		arctype *ep = start_n2;
		while (ep != nullptr) {
			zfw += ep->cost * ep->upper_bound;
			ep = ep->next_arc;
		}
		*mcfObj = zfw + lb_cost;

		/* Dual variables */
		// CG: removed computation of duals
		np = root->successor;
		while (np != root) {
			mcfDual[np->name-1] = np->dual;
			np = np->successor;
		}
		mcfDual[root->name-1] = root->dual;

		/* Arc flows */
		for (i = 0; i < mm; ++i)
			mcfFlow[i] = mcfLb[i];

		np = root->successor;
		while (np != root) {
			// flow on artificial arcs has to be 0 to be ignored! [CG]
			OGDF_ASSERT(np->arc_id->arcnum < mm || np->flow == 0);

			if (np->arc_id->arcnum < mm)
				mcfFlow[np->arc_id->arcnum] += np->flow;

			np = np->successor;
		}

		ep = start_n2;
		while (ep != nullptr) {
			mcfFlow[ep->arcnum] += ep->upper_bound;
			ep = ep->next_arc;
		}

	} else {
		retValue = 10;
	}

	// deallocate artificial arcs
	for(i = 1; i <= nn; ++i)
#if 0
		delete p[i]->arc_id;
#endif
		delete nodes[i].arc_id;

	return retValue;
}

}
