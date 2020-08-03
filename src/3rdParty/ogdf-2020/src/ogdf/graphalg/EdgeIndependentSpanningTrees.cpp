/** \file
 * \brief Implementation of ogdf::EdgeIndependentSpanningTrees methods
 *
 * \author Manuel Fiedler
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

#include <ogdf/basic/Graph.h>
#include <ogdf/graphalg/EdgeIndependentSpanningTrees.h>

namespace ogdf {

unsigned int EdgeIndependentSpanningTrees::createVals(const Solution &f, unsigned int j, std::vector<edge> &tree) const {
	tree.erase(tree.begin(), tree.end());
	unsigned int t = 0;
	for (edge e : m_G->edges) {
		if (f[e].first == j || f[e].second == j) {
			tree.push_back(e);
			++t;
		}
	}
	return t;
}

bool EdgeIndependentSpanningTrees::nextSpanningTree(unsigned int &t, std::vector<edge> &tree) const {
	edge e;
	if (tree.empty()) {
		// no edges used, begin with new subgraph
		e = m_G->firstEdge();
	} else {
		e = tree.back()->succ(); // iterate
		tree.pop_back();
		t--;
	}

	unsigned int n = m_G->numberOfNodes();

	while (true) {
		while (e == nullptr) { // end of edgeList
			if (!tree.empty()) { // if other edges exist, iterate them through backtracking
				e = tree.back();
				e = e->succ();
				--t;
				tree.pop_back();
			} else {
				// first edge has undergone all iterations, no other trees exist
				return false;
			}
		}

		if (t == 0 // first edge
		 || !pathExists(tree, e->source(), e->target(), t)) { // or no path exists (so the edge does not close a cycle and can be added
			t++;
			tree.push_back(e);
		}
		if (t == n - 1) { // tree found
			return true;
		}
		e = e->succ();
	}
	return false;
}

bool EdgeIndependentSpanningTrees::pathExists(const std::vector<edge> &tree, node v1, node v2, unsigned int t) const {
	if (v1 == v2) {
		return true;
	}
	NodeArray<unsigned int> usedNodes(*m_G, 0);
	EdgeArray<bool> usedEdges(*m_G, false);
	usedNodes[v1] = 1;

	bool changed = true;
	while (changed) {
		changed = false;
		for (node nv : m_G->nodes) {
			if (usedNodes[nv] == 1) { // node connected to v1, but environement of node not yet checked
				usedNodes[nv] = 2;
				for (adjEntry badj : nv->adjEntries) {
					if (isInSubGraph(tree, badj->theEdge(), t) && !usedEdges[badj->theEdge()]) {
						usedEdges[badj->theEdge()] = true;
						node nt = badj->twinNode();
						if (nt == v2) {
							return true;
						}
						if (usedNodes[nt] == 0) {
							changed = true;
							usedNodes[nt] = 1;
						}
					}
				}
			}
		}
	}
	return false;
}

bool EdgeIndependentSpanningTrees::isInSubGraph(const std::vector<edge> &sub, const edge &e, unsigned int t) const {
	for (unsigned int i = 0; i < t; ++i) {
		if (sub[i] == e) {
			return true;
		}
	}
	return false;
}

List<EdgeIndependentSpanningTrees::Solution> EdgeIndependentSpanningTrees::findAll(unsigned int k) const {
	List<Solution> retvec;

	findDo(k, [&](Solution &f) {
		// check for all existing spanning trees, if new one is permutation of it
		for (auto& ret : retvec) {
			if (!checkNewTree(f, ret, k)) {
				return true;
			}
		}

		retvec.pushBack(std::move(f));
		return true;
	});

	return retvec;
}

List<EdgeIndependentSpanningTrees::Solution> EdgeIndependentSpanningTrees::findAllPerm(unsigned int k) const {
	List<Solution> retvec;

	findDo(k, [&](Solution &f) {
		retvec.pushBack(std::move(f));
		return true;
	});

	return retvec;
}

bool EdgeIndependentSpanningTrees::findOne(unsigned int k, Solution &f) const {
	bool found = false;

	findDo(k, [&](Solution &solution) {
		f = std::move(solution);
		found = true;
		return false;
	});

	return found;
}

void EdgeIndependentSpanningTrees::findDo(unsigned int k, std::function<bool(Solution&)> func) const {
	OGDF_ASSERT(k > 0);

	if (m_G->numberOfNodes() - m_G->numberOfEdges() > 1) {
		return;
	}

	Solution f;
	NodeArray<adjEntry> parent(*m_G);

	if (createInitialSpanningTrees(f, k)) { // can create initial spanning trees
		while (iterate(f, 1, k)) { // go to next k spanning tree combination
			std::vector<NodeArray<adjEntry>> parents;
			bool checkind = true;

			for (unsigned int j = 1; j <= k; ++j) {
				if (createParentRel(f, j, parent)) {
					parents.push_back(parent);
				} else {
					checkind = false;
					break;
				}
			}

			// invoke func if spanning trees are independent
			if (checkind && checkIndependence(parents, k) && !func(f)) {
				// end search if func returns false
				return;
			}
		}
	}
}

// returns true, if unequal for all permutations
bool EdgeIndependentSpanningTrees::checkNewTree(const Solution &f1, const Solution &f2, unsigned int k) const {
	// creates vector [1,...,k]
	std::vector<unsigned int> perm(k);
	for (unsigned int i = 0; i < k; ++i) {
		perm[i] = i + 1;
	}

	// iterate through all permutations: if one permutation is equal, return false
	do {
		if (!checkOnePermUnequal(f1, f2, perm)) {
			return false;
		}
	} while (std::next_permutation(perm.begin(), perm.end()));
	if (!checkOnePermUnequal(f1, f2, perm)) {
		return false;
	}

	return true;
}

bool EdgeIndependentSpanningTrees::checkOnePermUnequal(const Solution &f1, const Solution &f2,
		const std::vector<unsigned int> &perm) const {
	for (unsigned int i = 0; i < perm.size(); ++i) {
		for (edge e : m_G->edges) {
			if ((f1[e].first == i + 1 || f1[e].second == i + 1)
			 && (f2[e].first != perm[i] && f2[e].second != perm[i])) {
				return true;
			}
			if ((f2[e].first == perm[i] || f2[e].second == perm[i])
			 && (f1[e].first != i + 1 && f1[e].second != i + 1)) {
				return true;
			}
		}
	}
	return false;
}

bool EdgeIndependentSpanningTrees::checkIndependence(const std::vector<NodeArray<adjEntry>> &parents,
		unsigned int k) const {
	if (parents.size() != k) { // contains incorrect number of subgraphs
		return false;
	}

	// iterate through each possible combination of (0,...,k-1)
	for (unsigned int i = 0; i < k; ++i) {
		for (unsigned int j = i + 1; j < k; ++j) {
			for (node v : m_G->nodes) {
				if (v != m_root) {
					if (!checkTwoPathIndependence(parents, v, i, j)) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

// checks whether the paths from v to root are indipendent in the subgraphs described by p1 and p2
bool EdgeIndependentSpanningTrees::checkTwoPathIndependence(const std::vector<NodeArray<adjEntry>> &parents,
		node v, unsigned int p1, unsigned int p2) const {
	if (p1 == p2) {
		return false;
	}

	OGDF_ASSERT(v != m_root);

	// iterate through both paths
	for (node v1 = v; v1 != m_root; v1 = parents[p1][v1]->twinNode()) { // iterate through path 1
		for (node v2 = v; v2 != m_root; v2 = parents[p2][v2]->twinNode()) { // iterate through path 2
			if (parents[p1][v1]->theEdge() == parents[p2][v2]->theEdge()) { // edges in both paths are equal
				return false;
			}
		}
	}

	return true; // both paths completely checked, no edge found twice
}

bool EdgeIndependentSpanningTrees::createParentRel(const Solution &f, unsigned int j, NodeArray<adjEntry> &parent) const {
	parent.fill(nullptr);

	enum class UseType {
		NotUsed,
		UsedWithoutNeighborhood,
		UsedWithNeighborhood
	};

	NodeArray<UseType> used(*m_G, UseType::NotUsed);
	used[m_root] = UseType::UsedWithoutNeighborhood;

	while (true) {
		bool same = true;
		for (node v : m_G->nodes) {
			if (used[v] == UseType::UsedWithoutNeighborhood) {
				// v is in subgraph, but neighborhood is not
				used[v] = UseType::UsedWithNeighborhood; // afterwards, neighborhood is in subgraph

				for (adjEntry badj : v->adjEntries) { // check neighborhood
					node t = badj->twinNode();
					if (f[badj->theEdge()].first == j || f[badj->theEdge()].second == j) {
						// edge in subgraph j
						if (used[t] != UseType::NotUsed) {
							// edge points towards node already in subgraph
							if (v == m_root || (parent[v])->theEdge() != badj->theEdge()) {
								// edge not reused
								// subgraph is not a tree
								return false;
							}
						} else {
							// edge points towards node not yet in subgraph
							parent[t] = badj->twin(); // add edge
							used[t] = UseType::UsedWithoutNeighborhood; // node now used
							same = false; // subgraph has changed
						}
					}
				}
			}
		}

		if (same) { // if subgraph has not changed, end
			break;
		}
	}
	for (node v : m_G->nodes) { // if all nodes are used, the subgraph is spanning
		if (used[v] == UseType::NotUsed) {
			return false;
		}
	}
	return true;
}

bool EdgeIndependentSpanningTrees::findAndInsertNextTree(Solution &f, unsigned int &t, unsigned int j, std::vector<edge> &tree) const {
	while (nextSpanningTree(t, tree)) {
		if (insertNewTree(f, t, j, tree)) {
			return true; // possible combination found
		}
	}

	return false; // nothing more found
}

bool EdgeIndependentSpanningTrees::createInitialSpanningTrees(Solution &f, unsigned int k) const {
	f.init(*m_G, std::pair<unsigned int, unsigned int>(0, 0));
	std::vector<edge> tree;

	unsigned int j = 1;
	unsigned int t = 0;

	while (j <= k) { // iterate through 1,...,k through backtracking
		while (!findAndInsertNextTree(f, t, j, tree)) {
			// all spanning trees used, track back
			if (j == 1) {
				// first spanning tree cannot iterate further, all combinations used!
				return false;
			} else {
				clearTree(f, j);
				--j; // track back
				t = createVals(f, j, tree); // start from tree j
			}
		}
		++j; // iterate to next value in 1,...,k
	}

	return true;
}

void EdgeIndependentSpanningTrees::clearTree(Solution &f, unsigned int j) const {
	for (edge e : m_G->edges) {
		// clear f from j
		if (f[e].second == j) {
			f[e].second = 0;
			if (f[e].first != 0) {
				f[e].second = f[e].first;
				f[e].first = 0;
			}
		} else if (f[e].first == j) {
			f[e].first = 0;
		}
	}
}

bool EdgeIndependentSpanningTrees::insertNewTree(Solution &f, unsigned int t, unsigned int j, std::vector<edge> &tree) const {
	clearTree(f, j);

	for (unsigned int i = 0; i < tree.size(); ++i) { // enter tree in f
		if (f[tree[i]].first != j && f[tree[i]].second != j) {
			if (f[tree[i]].second == 0) {
				f[tree[i]].second = j;
			} else if (f[tree[i]].first == 0) {
				f[tree[i]].first = f[tree[i]].second;
				f[tree[i]].second = j;
			} else {
				// both places are occupied, go to next spanning tree
				return false;
			}
		}

		if (f[tree[i]].first > f[tree[i]].second) {
			std::swap(f[tree[i]].first, f[tree[i]].second);
		}
	}
	return true;
}

// iterates through all possible combinations from {(0,0),...,(0,0)} to {(k-1,k),...,(k-1,k)}
bool EdgeIndependentSpanningTrees::iterate(Solution &f, unsigned int j, unsigned int k) const {
	std::vector<edge> tree;
	unsigned int t = createVals(f, j, tree);

	if (j == k) { // last spantree, iterate till end
		return findAndInsertNextTree(f, t, k, tree);
	} else {
		while (true) {
			if (iterate(f, j + 1, k)) { // use algorithm for next element in 1,...,k
				return true;
			}
			for (unsigned int i = j; i <= k; ++i) {
				clearTree(f, i);
			}

			if (!findAndInsertNextTree(f, t, j, tree)) {
				// all spanning trees used, track back
				return false;
			}
		}
	}
	return false; // all spanning trees used, return false
}

// determines if all possible combinations have been used
bool EdgeIndependentSpanningTrees::isFinished(const Solution &f, unsigned int k) const {
	for (edge e : m_G->edges) {
		// tests if equal to {(k-1,k),...,(k-1,k)}
		if (f[e].first != k - 1 || f[e].second != k) {
			return false;
		}
	}
	return true;
}

}
