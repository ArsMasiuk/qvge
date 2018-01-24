/** \file
 * \brief Implementation of class UpSAT, which implements
 *        the upward-planarity testing formulations based on
 * 		  satisfiability (Chimani, Zeranski, 2012+)
 *
 * \author Robert Zeranski
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

#include <ogdf/upward/internal/UpSAT.h>

using namespace Minisat;

namespace ogdf {

UpSAT::UpSAT(Graph &G)
  : feasibleOriginalEdges(false)
  , m_G(G)
  , N(m_G)
  , M(m_G)
  , D(m_G)
  , tau  (m_G.numberOfNodes(), std::vector<int>(m_G.numberOfNodes(), -1))
  , sigma(m_G.numberOfEdges(), std::vector<int>(m_G.numberOfEdges(), -1))
  , mu   (m_G.numberOfEdges(), std::vector<int>(m_G.numberOfNodes(), -1))
{
	numberOfVariables = 0;
	numberOfClauses   = 0;
	int cnt = 0;
	for (node v : m_G.nodes) {
		N[v] = cnt++;
	}
	cnt = 0;
	for (edge e : m_G.edges) {
		M[e] = cnt++;
	}
}

UpSAT::UpSAT(GraphCopy &G, bool _feasibleOriginalEdges)
  : UpSAT(G)
{
	feasibleOriginalEdges = _feasibleOriginalEdges;
}

void UpSAT::computeDominatingEdges()
{
	NodeArray<bool> visit(m_G);
	for (edge e : m_G.edges) {
		for (node v : m_G.nodes) {
			visit[v] = false;
		}
		List<node> bfs;
		bfs.pushBack(e->target());
		while (!bfs.empty()) {
			node x = bfs.popFrontRet();
			for(adjEntry adj : x->adjEntries) {
				edge f = adj->theEdge();
				if (f->source() == x) {
					if (!visit[f->target()]) bfs.pushBack(f->target());
					visit[f->target()] = true;
					D[e].pushBack(f);
				}
			}
		}
	}
}

void UpSAT::computeTauVariables()
{
	for (node v : m_G.nodes) {
		for (node w : m_G.nodes) {
			if (N[v] < N[w]) {
				tau[N[v]][N[w]] = ++numberOfVariables;
			} else {
				tau[N[v]][N[w]] = 0;
			}
		}
	}
}

void UpSAT::computeSigmaVariables()
{
	for (edge e : m_G.edges) {
		sigma[M[e]][M[e]] = 0;
		ListConstIterator<edge> it(D[e].begin());
		while (it.valid()) {
			edge f = *it;
			sigma[M[e]][M[f]] = 0;
			sigma[M[f]][M[e]] = 0;
			++it;
		}
	}
	for (edge e : m_G.edges) {
		for (edge f : m_G.edges) {
			if (sigma[M[e]][M[f]] == -1
			 && M[e] < M[f]) {
				sigma[M[e]][M[f]] = ++numberOfVariables;
				sigma[M[f]][M[e]] = -2;
			}
		}
	}
}

void UpSAT::computeMuVariables()
{
	for (edge e : m_G.edges) {
		for (node v : m_G.nodes) {
			mu[M[e]][N[v]] = ++numberOfVariables;
		}
	}
}

void UpSAT::reset()
{
	numberOfVariables = 0;
	numberOfClauses   = 0;
	for (edge e : m_G.edges) {
		for (node v : m_G.nodes) {
			mu[M[e]][N[v]] = -1;
		}
	}
	for (edge e : m_G.edges) {
		for (edge f : m_G.edges) {
			sigma[M[e]][M[f]] = -1;
		}
	}
	for (node u : m_G.nodes) {
		for (node v : m_G.nodes) {
			tau[N[u]][N[v]] = -1;
		}
	}
	m_F.reset();
}

void UpSAT::ruleTauTransitive()
{
	for (node u : m_G.nodes) {
		for (node v : m_G.nodes) {
			if (u != v) {
				int w1;
				if (N[u] < N[v]) {
					w1 = -tau[N[u]][N[v]];
				} else {
					w1 = tau[N[v]][N[u]];
				}
				for (node w : m_G.nodes) {
					if ((v != w)
					 && (w != u)) {
						int w2, w3;
						if (N[v] < N[w]) {
							w2 = -tau[N[v]][N[w]];
						} else {
							w2 = tau[N[w]][N[v]];
						}
						if (N[u] < N[w]) {
							w3 = tau[N[u]][N[w]];
						} else {
							w3 = -tau[N[w]][N[u]];
						}
						clause c = m_F.newClause();
						c->addMultiple(3, w1, w2, w3);
						m_F.finalizeClause(c);
						++numberOfClauses;
					}
				}
			}
		}
	}
}

void UpSAT::ruleSigmaTransitive()
{
	for (edge e : m_G.edges) {
		for (edge f : m_G.edges) {
			if ((e != f)
			 && (sigma[M[e]][M[f]] != 0)) {
				int w1;
				if (M[e] < M[f]) {
					w1 = -sigma[M[e]][M[f]];
				} else {
					w1 = sigma[M[f]][M[e]];
				}
				for (edge g : m_G.edges) {
					if ((f != g)
					 && (e != g)
					 && (sigma[M[f]][M[g]] != 0)
					 && (sigma[M[e]][M[g]] != 0)) {
						int w2, w3;
						if (M[f] < M[g]) {
							w2 = -sigma[M[f]][M[g]];
						} else {
							w2 = sigma[M[g]][M[f]];
						}
						if (M[e] < M[g]) {
							w3 = sigma[M[e]][M[g]];
						} else {
							w3 = -sigma[M[g]][M[e]];
						}
						clause c = m_F.newClause();
						c->addMultiple(3, w1, w2, w3);
						m_F.finalizeClause(c);
						++numberOfClauses;
					}
				}
			}
		}
	}
}

void UpSAT::ruleUpward()
{
	if (!feasibleOriginalEdges) {
		for (edge e : m_G.edges) {
			node u = e->source();
			node v = e->target();
			int w1;
			if (N[u] < N[v]) {
				w1 = tau[N[u]][N[v]];
			} else {
				w1 = -tau[N[v]][N[u]];
			}
			clause c = m_F.newClause();
			c->add(w1);
			m_F.finalizeClause(c);
			++numberOfClauses;
		}
	} else {
		GraphCopy &GC = (GraphCopy &) m_G;
		for (edge e : GC.original().edges) {
			node u = GC.copy(e->source());
			node v = GC.copy(e->target());
			int w1;
			if (N[u] < N[v]) {
				w1 = tau[N[u]][N[v]];
			} else {
				w1 = -tau[N[v]][N[u]];
			}
			clause c = m_F.newClause();
			c->add(w1);
			m_F.finalizeClause(c);
			++numberOfClauses;
		}
	}
}

void UpSAT::rulePlanarity()
{
	for (edge e : m_G.edges) {
		for (edge f : m_G.edges) {
			if (e != f
			 && (e->source() == f->target()
			  || e->source() == f->source()
			  || e->target() == f->target())) {
				for (edge g : m_G.edges) {
					if (f != g
					 && e != g) {
						node v = e->commonNode(f);
						node x = g->source();
						node y = g->target();
						if (v != x
						 && v != y
						 && sigma[M[e]][M[g]] != 0
						 && sigma[M[f]][M[g]] != 0) {
							int w1, w2, w3, w4;
							if (N[x] < N[v]) {
								w1 = -tau[N[x]][N[v]];
							} else {
								w1 = tau[N[v]][N[x]];
							}
							if (N[v] < N[y]) {
								w2 = -tau[N[v]][N[y]];
							} else {
								w2 = tau[N[y]][N[v]];
							}
							if (M[e] < M[g]) {
								w3 =  sigma[M[e]][M[g]];
							} else {
								w3 = -sigma[M[g]][M[e]];
							}
							if (M[f] < M[g]) {
								w4 =  sigma[M[f]][M[g]];
							} else {
								w4 = -sigma[M[g]][M[f]];
							}
							clause c1 = m_F.newClause();
							clause c2 = m_F.newClause();
							c1->addMultiple(4, w1, w2, w3, -w4);
							c2->addMultiple(4, w1, w2, -w3, w4);
							m_F.finalizeClause(c1);
							m_F.finalizeClause(c2);
							numberOfClauses += 2;
						}
					}
				}
			}
		}
	}
}

void UpSAT::ruleTutte()
{
	for (edge e : m_G.edges) {
		for (edge f : m_G.edges) {
			node e1 = e->source();
			node e2 = e->target();
			node f1 = f->source();
			node f2 = f->target();
			if (e != f
			 && e1 != f1
			 && e1 != f2
			 && e2 != f1
			 && e2 != f2
			 && sigma[M[e]][M[f]] != 0) {
				int w1, w2, w3, w4, w5, w6;
				if (N[e1] < N[f1]) {
					w1 = tau[N[e1]][N[f1]];
				} else {
					w1 = -tau[N[f1]][N[e1]];
				}
				if (N[e2] < N[f2]) {
					w2 = tau[N[e2]][N[f2]];
				} else {
					w2 = -tau[N[f2]][N[e2]];
				}
				if (N[f1] < N[e2]) {
					w3 = tau[N[f1]][N[e2]];
				} else {
					w3 = -tau[N[e2]][N[f1]];
				}
				w4 = mu[M[e]][N[f1]];
				w5 = mu[M[f]][N[e2]];
				w6 = mu[M[e]][N[f2]];
				clause c1 = m_F.newClause();
				clause c2 = m_F.newClause();
				clause c3 = m_F.newClause();
				clause c4 = m_F.newClause();
				c1->addMultiple(5, -w1, -w2, -w3, w4, w5);
				c2->addMultiple(5, -w1, -w2, -w3, -w4, -w5);
				c3->addMultiple(4, -w1, w2, w4, -w6);
				c4->addMultiple(4, -w1, w2, -w4, w6);
				m_F.finalizeClause(c1);
				m_F.finalizeClause(c2);
				m_F.finalizeClause(c3);
				m_F.finalizeClause(c4);
				numberOfClauses += 4;
			}
		}
	}
}

void UpSAT::ruleFixed(const Model &model)
{
	for (node u : m_G.nodes) {
		for (node v : m_G.nodes) {
			if (N[u] >= N[v]) {
				continue;
			}
			clause c = m_F.newClause();
			int w1 = tau[N[u]][N[v]];
			if (model.getValue(w1)) {
				c->add(w1);
			} else {
				c->add(-w1);
			}
			m_F.finalizeClause(c);
			++numberOfClauses;
		}
	}
}

bool UpSAT::HL(bool embed, adjEntry& externalToItsRight, NodeArray<int> *nodeOrder)
{
	computeDominatingEdges();
	computeTauVariables();
	computeMuVariables();
	computeSigmaVariables();
	int var = (m_G.numberOfNodes()*m_G.numberOfNodes() - m_G.numberOfNodes())/2 + m_G.numberOfEdges()*m_G.numberOfNodes();
	m_F.newVars(var);
	ruleTauTransitive();
	ruleUpward();
	ruleTutte();
	Model model;
	bool result = m_F.solve(model);

	if (!result) {
		return result;
	}

	var = (m_G.numberOfNodes()*m_G.numberOfNodes() - m_G.numberOfNodes())/2;
	for (edge e : m_G.edges) {
		for (edge f : m_G.edges) {
			if (M[e] < M[f]
			 && sigma[M[e]][M[f]] != 0) {
				++var;
			}
		}
	}
	m_F.reset();
	m_F.newVars(var);
	ruleFixed(model);
	ruleTauTransitive();
	ruleSigmaTransitive();
	rulePlanarity();

	Model embModel;
	m_F.solve(embModel);

	if (embed) {
		embedFromModel(embModel, externalToItsRight);
	}
	if (nodeOrder) {
		writeNodeOrder(embModel,nodeOrder);
	}

	return result;
}

bool UpSAT::FPSS(NodeArray<int> *nodeOrder)
{
	computeDominatingEdges();
	computeTauVariables();
	computeMuVariables();
	computeSigmaVariables();
	int var = (m_G.numberOfNodes()*m_G.numberOfNodes() - m_G.numberOfNodes())/2 + m_G.numberOfEdges()*m_G.numberOfNodes();
	m_F.newVars(var);
	ruleTauTransitive();
	ruleUpward();
	ruleTutte();
	Model model;
	bool result = m_F.solve(model);
	if(nodeOrder) writeNodeOrder(model,nodeOrder);
	return result;
}

bool UpSAT::OE(bool embed, adjEntry& externalToItsRight, NodeArray<int> *nodeOrder)
{
	computeDominatingEdges();
	computeTauVariables();
	computeSigmaVariables();

	ruleTauTransitive();
	ruleSigmaTransitive();
	ruleUpward();
	rulePlanarity();

	m_F.newVars(numberOfVariables);
	Model model;
	bool result = m_F.solve(model);

	if (result && embed) {
		embedFromModel(model, externalToItsRight);
	}
	if (nodeOrder) {
		writeNodeOrder(model, nodeOrder);
	}

	return result;
}

int UpSAT::getNumberOfVariables()     { return numberOfVariables; }
long long UpSAT::getNumberOfClauses() { return numberOfClauses;   }

void UpSAT::sortBySigma(List<adjEntry> &adjList, const Model &model)
{
	for (int i = 1; i < adjList.size(); ++i) {
		ListIterator<adjEntry> it = adjList.begin();
		for (int j = 1; j < i; ++j) {
			++it;
		}
		adjEntry adjFirst = *it;
		adjEntry adjMin = adjFirst;
		adjEntry adj;
		while ((++it).valid()) {
			adj = *it;
			edge f = adjMin->theEdge();
			edge e = adj->theEdge();

			if ((M[e] < M[f]) ? model.getValue(sigma[M[e]][M[f]]) : !model.getValue(sigma[M[f]][M[e]])) {
				adjMin = adj;
			}
#if 0
			int var;
			bool isNegated = M[e] > M[f];
			if (!isNegated) {
				var = sigma[M[e]][M[f]];
			} else {
				var = sigma[M[f]][M[e]];
			}
			if (isNegated != model.getValue(var)) {
				adjMin = adj;
			}
#endif
		}
		if (adjMin != adjFirst) {
			ListIterator<adjEntry> itCnt = adjList.get(i-1);
			int posMin = i-1;
			while (itCnt.valid()) {
				if (*itCnt == adjMin) break;
				++posMin;
				++itCnt;
			}
			const ListIterator<adjEntry> itFirst = adjList.get(i-1);
			const ListIterator<adjEntry> itMin   = adjList.get(posMin);
			adjList.exchange(itFirst, itMin);
		}
	}
}

void UpSAT::embedFromModel(const Model &model, adjEntry& externalToItsRight)
{
	node lowest = m_G.firstNode();
	for (node v : m_G.nodes) {
		if (v->degree() > 2) {
			List<adjEntry> inc;
			List<adjEntry> out;
			for(adjEntry adj : v->adjEntries) {
				if (adj->theEdge()->source() == v) {
					out.pushBack(adj);
				} else {
					inc.pushBack(adj);
				}
			}
			if (inc.size() > 1) sortBySigma(inc,model);
			if (out.size() > 1) sortBySigma(out,model);
			List<adjEntry> finalList;
			ListIterator<adjEntry> it = inc.rbegin();
			while (it.valid()) {
				finalList.pushBack(*it);
				it--;
			}
			it = out.begin();
			while (it.valid()) {
				finalList.pushBack(*it);
				++it;
			}
			m_G.sort(v,finalList);
		}
		if (v != lowest
		 && ((N[v] < N[lowest])
		  ? model.getValue(tau[N[v]][N[lowest]])
		  : !model.getValue(tau[N[lowest]][N[v]]))) {
			lowest = v;
		}
		if (v == lowest) {
			externalToItsRight = v->lastAdj();
		}
	}
	OGDF_ASSERT(hasSingleSource(m_G));
	OGDF_ASSERT(externalToItsRight->theNode()->indeg() == 0);
}

bool UpSAT::testUpwardPlanarity(NodeArray<int> *nodeOrder/* = NULL*/) { return FPSS(nodeOrder); }
bool UpSAT::embedUpwardPlanar(adjEntry& externalToItsRight, NodeArray<int> *nodeOrder/* = NULL*/)   { return HL(true, externalToItsRight, nodeOrder);   }

class UpSAT::Comp {
public:
	Comp(const Model &MM, const NodeArray<int> &NN, const std::vector<std::vector<int>> &ttau)
	  : N(NN)
	  , model(MM)
	  , tau(ttau)
	{
	}
private:
	const NodeArray<int> &N;
	const Model &model;
	const std::vector<std::vector<int>> &tau;
public:
	bool less(node u, node v) const {
		return (N[u] < N[v]) ? model.getValue(tau[N[u]][N[v]]) : !model.getValue(tau[N[v]][N[u]]);
	}
};

void UpSAT::writeNodeOrder(const Model &model, NodeArray<int> *nodeOrder)
{
	List<node> list;
	m_G.allNodes(list);
	Comp CC(model, N, tau);
	list.quicksort(CC);
	int i = 0;
	for (auto v : list) {
		(*nodeOrder)[v] = i++;
	}
}

}
