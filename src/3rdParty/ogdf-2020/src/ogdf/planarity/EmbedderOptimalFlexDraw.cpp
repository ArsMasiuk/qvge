/** \file
 * \brief The algorithm computes a planar embedding with minimum cost.
 *
 * \author Dzmitry Sledneu
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

#include <ogdf/basic/FaceArray.h>
#include <ogdf/planarity/EmbedderOptimalFlexDraw.h>
#include <ogdf/graphalg/MinCostFlowReinelt.h>

namespace ogdf {

EmbedderOptimalFlexDraw::EmbedderOptimalFlexDraw()
{
	m_minCostFlowComputer.reset(new MinCostFlowReinelt<int>);
}

void EmbedderOptimalFlexDraw::createNetwork(
	node parent,
	node mu,
	int bends,
	NodeArray<int> cost[],
	Skeleton &skeleton,
	EdgeArray<node> &edgeNode,
	Graph &N,
	EdgeArray<int> &upper,
	EdgeArray<int> &perUnitCost,
	NodeArray<int> &supply)
{
	Graph skeletonGraph = skeleton.getGraph();
	ConstCombinatorialEmbedding skeletonEmbedding(skeletonGraph);
	NodeArray<node> vertexNode(skeletonGraph);
	FaceArray<node> faceNode(skeletonEmbedding);

	for (node v: skeletonGraph.nodes) {
		vertexNode[v] = N.newNode();
		supply[vertexNode[v]] = 4 - skeleton.original(v)->degree() - v->degree();
	}

	if (parent != nullptr) {
		node s = skeleton.referenceEdge()->source();
		node t = skeleton.referenceEdge()->target();
		supply[vertexNode[s]] = 2 - s->degree();
		supply[vertexNode[t]] = 2 - t->degree();
	}

	for (edge e: skeletonGraph.edges) {
		if (skeleton.isVirtual(e)) {
			edgeNode[e] = N.newNode();
			PertinentGraph H;
			skeleton.owner().pertinentGraph(skeleton.twinTreeNode(e), H);
			node s = H.original(e)->source();
			node t = H.original(e)->target();
			supply[edgeNode[e]] = s->degree() + t->degree() - 2;
		}
	}

	for (face f: skeletonEmbedding.faces) {
		faceNode[f] = N.newNode();
		supply[faceNode[f]] = 4;
	}

	if (parent != nullptr) {
		face f1 = nullptr;
		face f2 = nullptr;
		for (adjEntry adj : skeletonEmbedding.externalFace()->entries) {
			if (adj->theEdge() == skeleton.referenceEdge()) {
				f1 = skeletonEmbedding.rightFace(adj);
				f2 = skeletonEmbedding.leftFace(adj);
				break;
			}
		}
		PertinentGraph H;
		skeleton.owner().pertinentGraph(mu, H);
		node s = skeleton.referenceEdge()->source();
		node t = skeleton.referenceEdge()->target();
		supply[faceNode[f1]] =  H.original(s)->degree() + H.original(t)->degree() - 2 + bends;
		supply[faceNode[f2]] = -bends;
	} else {
		supply[faceNode[skeletonEmbedding.externalFace()]] = -4;
	}

	for (face f: skeletonEmbedding.faces) {
		for (adjEntry adj = f->firstAdj(); adj != nullptr; adj = adj->succ()) {
			edge e1 = N.newEdge(faceNode[f], vertexNode[adj->theNode()]);
			upper[e1] = 1;
			perUnitCost[e1] = 0;
			edge e2 = N.newEdge(vertexNode[adj->theNode()], faceNode[f]);
			upper[e2] = 1;
			perUnitCost[e2] = 0;
		}
	}

	for (face f: skeletonEmbedding.faces) {
		for (adjEntry adj = f->firstAdj(); adj != nullptr; adj = adj->succ()) {
			edge e = N.newEdge(edgeNode[adj->theEdge()], faceNode[f]);
			upper[e] = std::numeric_limits<int>::max();
			perUnitCost[e] = 0;
		}
	}

	for (face f: skeletonEmbedding.faces) {
		for (adjEntry adj = f->firstAdj(); adj != nullptr; adj = adj->succ()) {
			if (skeleton.isVirtual(adj->theEdge())) {
				node twinNode = skeleton.twinTreeNode(adj->theEdge());
				edge e0 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e0] = 1;
				perUnitCost[e0] = cost[0][twinNode];
				edge e1 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e1] = 1;
				perUnitCost[e0] = cost[1][twinNode] - cost[0][twinNode];
				edge e2 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e2] = 1;
				perUnitCost[e2] = cost[2][twinNode] - cost[1][twinNode];
				edge e3 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e3] = 1;
				perUnitCost[e3] = cost[3][twinNode] - cost[2][twinNode];
				for (adjEntry adjTwin: twinNode->adjEntries) {
					if (adjTwin->twinNode() != twinNode) {
						perUnitCost[e0] -= cost[0][adjTwin->twinNode()];
						perUnitCost[e1] -= cost[0][adjTwin->twinNode()];
						perUnitCost[e2] -= cost[0][adjTwin->twinNode()];
						perUnitCost[e3] -= cost[0][adjTwin->twinNode()];
					}
				}
			} else {
				edge e0 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e0] = 1;
				perUnitCost[e0] = m_cost[0][adj->theEdge()];
				edge e1 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e1] = 1;
				perUnitCost[e1] = m_cost[1][adj->theEdge()] - m_cost[0][adj->theEdge()];
				edge e2 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e2] = 1;
				perUnitCost[e2] = m_cost[2][adj->theEdge()] - m_cost[1][adj->theEdge()];
				edge e3 = N.newEdge(faceNode[f], edgeNode[adj->theEdge()]);
				upper[e3] = 1;
				perUnitCost[e3] = m_cost[3][adj->theEdge()] - m_cost[2][adj->theEdge()];
			}
		}
	}
}

void EmbedderOptimalFlexDraw::optimizeOverEmbeddings(
	StaticPlanarSPQRTree &T,
	node parent,
	node mu,
	int bends,
	NodeArray<int> cost[],
	NodeArray<long long> embedding[])
{
	cost[bends][mu] = std::numeric_limits<int>::max();
	long long embeddingsCount = T.numberOfNodeEmbeddings(mu);
	for (long long currentEmbedding = 0; currentEmbedding < embeddingsCount; ++currentEmbedding) {

		T.embed(mu, currentEmbedding);

		Skeleton &skeleton = T.skeleton(mu);
		Graph skeletonGraph = skeleton.getGraph();
		ConstCombinatorialEmbedding skeletonEmbedding(skeletonGraph);
		NodeArray<node> vertexNode(skeletonGraph);
		EdgeArray<node> edgeNode(skeletonGraph);
		FaceArray<node> faceNode(skeletonEmbedding);

		Graph N;
		EdgeArray<int> upper(N);
		EdgeArray<int> perUnitCost(N);
		NodeArray<int> supply(N);

		createNetwork(
			parent,
			mu,
			bends,
			cost,
			skeleton,
			edgeNode,
			N,
			upper,
			perUnitCost,
			supply);

		EdgeArray<int> lower(N, 0);
		EdgeArray<int> flow(N);
		NodeArray<int> dual(N);

		m_minCostFlowComputer->call(N, lower, upper, perUnitCost, supply, flow, dual);

		int currentCost = 0;
		for (edge e = N.firstEdge(); e != nullptr; e = e->succ())
			currentCost += perUnitCost[e] * flow[e];

		for (adjEntry adj = mu->firstAdj(); adj != nullptr; adj = adj->succ())
			currentCost += cost[0][adj->twinNode()];

		if (currentCost < cost[bends][mu]) {
			cost[bends][mu] = currentCost;
			embedding[bends][mu] = currentEmbedding;
		}
	}
}

void EmbedderOptimalFlexDraw::computePrincipalSplitComponentCost(
	StaticPlanarSPQRTree &T,
	NodeArray<int> cost[],
	NodeArray<long long> embedding[],
	node parent,
	node mu)
{
	for (adjEntry adj = mu->firstAdj(); adj != nullptr; adj = adj->succ())
		if (adj->twinNode() != parent)
			computePrincipalSplitComponentCost(T, cost, embedding, mu, adj->twinNode());

	for (int bends = 0; bends < 4; ++bends)
		optimizeOverEmbeddings(T, parent, mu, bends, cost, embedding);
}

void EmbedderOptimalFlexDraw::doCall(Graph &G, adjEntry &adjExternal)
{
	StaticPlanarSPQRTree T(G);

	NodeArray<int> cost[4];
	NodeArray<long long> embedding[4];
	for (int bends = 0; bends < 4; ++bends) {
		cost[bends].init(T.tree());
		embedding[bends].init(T.tree());
	}

	int minCost = std::numeric_limits<int>::max();
	node minCostRoot = nullptr;
	long long minCostEmbedding = 0;

	for (node root = T.tree().firstNode(); root != nullptr; root = root->succ()) {

		T.rootTreeAt(root);

		for (adjEntry adj = root->firstAdj(); adj != nullptr; adj = adj->succ())
			computePrincipalSplitComponentCost(T, cost, embedding, root, adj->twinNode());

		optimizeOverEmbeddings(T, nullptr, root, 0, cost, embedding);

		if (cost[0][root] < minCost) {
			minCost = cost[0][root];
			minCostEmbedding = embedding[0][root];
			minCostRoot = root;
		}

	}

	OGDF_ASSERT(minCostRoot != nullptr);
	T.rootTreeAt(minCostRoot);
	T.embed(minCostRoot, minCostEmbedding);

	for (adjEntry adj = minCostRoot->firstAdj(); adj != nullptr; adj = adj->succ())
		computePrincipalSplitComponentCost(T, cost, embedding, minCostRoot, adj->twinNode());

	Skeleton &skeleton = T.skeleton(minCostRoot);
	Graph skeletonGraph = skeleton.getGraph();
	ConstCombinatorialEmbedding skeletonEmbedding(skeletonGraph);
	EdgeArray<node> edgeNode(skeletonGraph);

	Graph N;
	EdgeArray<int> upper(N);
	EdgeArray<int> perUnitCost(N);
	NodeArray<int> supply(N);

	createNetwork(
		nullptr,
		minCostRoot,
		0,
		cost,
		skeleton,
		edgeNode,
		N,
		upper,
		perUnitCost,
		supply);

	EdgeArray<int> lower(N, 0);
	EdgeArray<int> flow(N);
	NodeArray<int> dual(N);

	m_minCostFlowComputer->call(N, lower, upper, perUnitCost, supply, flow, dual);

	for (node mu = T.tree().firstNode(); mu != nullptr; mu = mu->succ()) {

		if (mu == minCostRoot)
			continue;

		int bends = 0;
		for (adjEntry adj = edgeNode[T.skeleton(mu).referenceEdge()]->firstAdj(); adj != nullptr; adj = adj->succ())
			bends += abs(flow[adj->theEdge()]);

		T.embed(mu, embedding[bends][mu]);
	}

	T.embed(G);
	ConstCombinatorialEmbedding graphEmbedding(G);
	adjExternal = graphEmbedding.externalFace()->firstAdj();
}

}
