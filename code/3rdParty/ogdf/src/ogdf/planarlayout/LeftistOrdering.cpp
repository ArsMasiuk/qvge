/** \file
 * \brief Implements the class LeftistOrdering...
 *
 * ... that computes a leftist canonical ordering as described by Badent et al. in More Canonical Ordering
 *
 * \author Martin Gronemann
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

#include <ogdf/planarlayout/LeftistOrdering.h>

using namespace ogdf;

void LeftistOrdering::Partitioning::buildFromResult(const Graph& G, const List<List<node> >& lco)
{
	m_ears.init(lco.size());

	// counter for the partition index
	int currPartIndex = 0;

	// init for all partitions the array
	for (const List<node>& list : lco) {
		// array for the path
		Array<adjEntry>& ear = m_ears[currPartIndex];

		// reserve for all adjEntries from left->v1, ... v_k->right some space
		ear.init(list.size()+1);

		// just for safety ;)
		for (int i = 0; i < list.size()+1; ++i) {
			// reset the array
			ear[i] = nullptr;
		}

		// next partition
		currPartIndex++;
	}

	// index of the partiton
	NodeArray<int> partIndex(G, -1);

	// reset the counter
	currPartIndex = 0;

	// for all partitions
	for (const List<node>& list : lco) {
		// for all nodes of that partition
		for (node v : list) {
			// set the index
			partIndex[v] = currPartIndex;
		}

		// increment the index
		currPartIndex++;
	}

	// reset the part counter
	currPartIndex = 0;

	for (const List<node>& list : lco) {
		// the array
		Array<adjEntry>& ear = m_ears[currPartIndex];

		// counter for the path
		int i = 1;

		// for all nodes of that partition
		for (node v : list) {
			// for all adj Entries
			for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
				// the other node
				node w = adj->twinNode();

				// next one cw
				node w_next = adj->cyclicSucc()->twinNode();

				// prev one cw
				node w_prev = adj->cyclicPred()->twinNode();

				// if that is an edge to G / G_k, skip ti
				if (partIndex[w] > partIndex[v])
					continue;

				// if w is in G_k-1
				if (partIndex[w] < partIndex[v]) {
					// if the next one is in G / G_k this must be the left leg
					if (partIndex[w_next] > partIndex[v]) {
						// put the twin, i.e. from w to v first in the array
						ear[0] = adj->twin();
					}

					// if the prev one is in G / G_k this must be the right leg
					if (partIndex[w_prev] > partIndex[v]) {
						// last element of the path
						ear[list.size()] = adj;
					}
				} else
				if (partIndex[w] == partIndex[v]
				 && partIndex[w_prev] > partIndex[v]) {
					// if the prev is in G / G_k this must be an v_i v_i+1 edge
					ear[i] = adj;
				}

			}
			// increment the counter
			i++;
		}

		// next one
		currPartIndex++;
	}

	// we are fine except for the last guy v_n sitting on top of this mess.
	// that one has no left or right leg yet.
	// figure out the v_1, v_n edge by taking the cyclic succ of the v_1 -> v_2 edge
	adjEntry adj_v1n = getChainAdj(0, 0)->cyclicSucc();

	// set it as path begin
	m_ears[numPartitions()-1][0] = adj_v1n;

	// the end is then the next edge cw
	// notice that this last guy is a singleton anyway
	m_ears[numPartitions()-1][1] = adj_v1n->twin()->cyclicSucc();
}

// computes the leftist canonical order. Requires that G is simple, triconnected and embedded.
// adj_v1n is the adjEntry at v_1 looking towards v_n, the outerface is choosen such that v_2 is the cyclic pred
// of v_n. the result is saved in result, a list of list of nodes, first set is v_1, v_2, last one is v_n.
bool LeftistOrdering::call(const Graph& G, adjEntry adj_v1n, List<List<node> >& result)
{
	// init the is marked array for all adj entries
	m_marked.init(G, false);

	// v1 -> v_n edge
	adjEntry adj_v12 = adj_v1n->cyclicPred();

	// the node v_n
	node v_n = adj_v1n->twinNode();

	// init all the node related arrays
	m_cutFaces.init(G, 0);
	m_cutEdges.init(G, 0);
	m_cutFaces[v_n] = 1;

	// mark v_1 -> v_n
	m_marked[adj_v12] = true;
	m_marked[adj_v12->twin()] = true;

	// initial candidate for the belt
	Candidate v12_candidate;
	v12_candidate.chain.pushBack(adj_v12->twin()); // edge 2->1
	v12_candidate.chain.pushBack(adj_v12);         // edge 1->2
	v12_candidate.chain.pushBack(adj_v12->twin()); // edge 2->1
	v12_candidate.stopper = nullptr;

	// init the belt
	m_belt.pushBack(v12_candidate);

	// init the candidate variable
	// we us an iterator here to keep things simple
	m_currCandidateIt = m_belt.begin();

	// while the belt contains some candidates
	while (!m_belt.empty()) {
		// the next partition
		List<node> P_k;

		// get the next leftmost feasible candidate
		if (!leftmostFeasibleCandidate(P_k))
			return false;

		// update the belt
		updateBelt();

		// save the result.
		result.pushBack(P_k);
	}
	// thats it we are done
	return true;
}

bool LeftistOrdering::leftmostFeasibleCandidate(List<node>& result)
{
	// init found to havent found anything
	bool found = false;

	// repeat
	do {
		// get the real candidate instance
		Candidate& candidate = *m_currCandidateIt;

		// in the chain is one more edge then external vetices
		int p = candidate.chain.size() - 1;

		// we are very lazy here and copy it into an array
		Array<node> z(candidate.chain.size() + 1);

		// copy the nodes into the array
		{
			int j = 0;
			// for all in the candidate chain
			for (List<adjEntry>::const_iterator it = candidate.chain.begin();
					it.valid(); ++it) {
				// copy all z_0 til z_p into the array
				z[j++] = (*it)->theNode();
			}

			// and z_p+1 afterwards
			z[j++] = candidate.chain.back()->twinNode();
		}

		// check if z_0 = z_p+1
		if (z[0] != z[p+1]) {
			// init j with p, i.e. the index of the item before the last one
			int j = p;

			// in reverse order
			while ((j > 0) && !(forbidden(z[j]) || singular(z[j]))) {
				// go one more to the left
				j--;
			}

			if (j > 0) {
				// save z_j as stopper
				candidate.stopper = z[j];
			}

			// check if we found a chain or singleton
			if ((j == 0) || (singular(candidate.stopper) && (p == 1))) {
				//std::cout << "FOUND STH" << std::endl;
				// yes, we found sth.
				found = true;

				// mark all edges of the chain
				for (List<adjEntry>::const_iterator it = candidate.chain.begin(); it.valid(); ++it) {
					m_marked[(*it)->twin()] = true;
				}
			}
		}

		// we havent found anything here
		if (!found) {
			// try the successor of candidate
			m_currCandidateIt++;

			// this should never happen
			if (!m_currCandidateIt.valid()) {
				std::cout << "ILLEGAL INPUT" << std::endl;
				return false;
			}
		}
		// repeat this until we found sth.
	} while (!found);

	// we found something, copy it to the result
	for (List<adjEntry>::const_iterator it = (*m_currCandidateIt).chain.begin();
			it.valid(); ++it) {
		// skip the first one
		if (it == (*m_currCandidateIt).chain.begin())
			continue;

		// copy all z_0 til z_p into the array
		result.pushBack((*it)->theNode());
	}
	return true;
}

// this is uses to check a candidate for a singleton copy
bool LeftistOrdering::isSingletonWith(const Candidate& c,node v) const
{
	// only two please
	if (c.chain.size() > 2)
		return false;

	// check if v_1 is v
	if (c.chain.front()->twinNode() != v)
		return false;

	// no forbidden candidates
	if (forbidden(c.chain.front()->twinNode()))
		return false;

	// last but not least is it singular ?
	// notice that the stopper may not be up to date
	return singular(c.chain.front()->twinNode());
}

void LeftistOrdering::updateBelt()
{
	// get the real candidate instance
	Candidate& candidate = *m_currCandidateIt;

	// check for singleton
	if (candidate.stopper && singular(candidate.stopper)) {
		// while candidate has a succ and that is a copy of the singleton
		while (m_currCandidateIt.succ().valid() &&
				isSingletonWith(*m_currCandidateIt.succ(), candidate.stopper)) {
			// remove it
			m_belt.del(m_currCandidateIt.succ());
		}

		// while candidate has a pred and that is a copy of the singleton
		while (m_currCandidateIt.pred().valid() &&
				isSingletonWith(*m_currCandidateIt.pred(), candidate.stopper)) {
			// remove it
			m_belt.del(m_currCandidateIt.pred());
		}
	}

	// save the iterator to the pred candidate
	List<Candidate>::iterator pred_it = m_currCandidateIt.pred();

	// and to the succ candidate
	List<Candidate>::iterator succ_it = m_currCandidateIt.succ();

	// the succ is a proper one
	if (succ_it.valid()) {
		// get rid of the first edge in the chain
		(*succ_it).chain.popFront();
	}

	// the new candidates
	List<Candidate> extension;

	// compute them
	beltExtension(extension);

	// instead of replacing, we insert all new candidates before the current one
	for (List<Candidate>::const_iterator it = extension.begin(); it.valid(); ++it) {
		m_belt.insertBefore(*it, m_currCandidateIt);
	}

	// now remove the original candidate
	m_belt.del(m_currCandidateIt);

	// if we actually put something in there
	if (!extension.empty()) {
		// set it to the first entry of extension i.e. the succ of the pred of the old one
		m_currCandidateIt = (pred_it.valid() ? pred_it.succ() : m_belt.begin());
	} else {
		// otherwise set it to the succ of the old one.
		// Same as above but for the sake of a reference impl we keep it this way
		m_currCandidateIt = succ_it;
	}


	// if the pred of the original is a proper one
	if (pred_it.valid()) {
		// remove the last edge in its chain
		adjEntry adj_vw = (*pred_it).chain.popBackRet();

		// the node v
		node v = adj_vw->theNode();

		// the node w
		node w = adj_vw->twinNode();

		// check if v is a stopper in pred
		if ((v == (*pred_it).stopper) || (w == (*pred_it).chain.front()->theNode())) {
			// null the stopper
			(*pred_it).stopper = nullptr;

			// and take pred as new candidate
			m_currCandidateIt = pred_it;
		}
	}
}

void LeftistOrdering::beltExtension(List<Candidate>& extension)
{
	// clear the result
	extension.clear();

	// get the real candidate instance
	Candidate& candidate = *m_currCandidateIt;

	// for all edges in the current chain except the first one
	for (List<adjEntry>::const_iterator it = candidate.chain.begin().succ(); it.valid(); ++it) {
		// starting node
		node v_start = (*it)->theNode();

		// end node
		node v_end = (*it)->twinNode();

		// iterator for the adj list of v_start
		adjEntry first = *it;

		// the iterating one for the face
		adjEntry adj_vw;
		do {
			// advance one on the adj list of v_start
			adj_vw = first = first->cyclicSucc();

			// increment the number of cut edges of w
			m_cutEdges[adj_vw->twinNode()]++;

			if (!m_marked[first]) {
				// a new chain to fill
				Candidate newCandidate;

				// repeat
				do {
					// mark the edge
					m_marked[adj_vw] = true;

					// add it to the chain
					newCandidate.chain.pushBack(adj_vw);

					// incr the num cutfaces of w
					m_cutFaces[adj_vw->twinNode()]++;

					// advance adj_vw around the face
					adj_vw = adj_vw->twin()->cyclicPred();

					// until w is v_start or v_end
				} while ((adj_vw->twinNode() != v_start) && (adj_vw->twinNode() != v_end));

				// mark the last one
				m_marked[adj_vw] = true;

				// add it to the chain
				newCandidate.chain.pushBack(adj_vw);

				// add the chain to the extension
				extension.pushBack(newCandidate);
			}

			// until w == v_end
		}  while (adj_vw->twinNode() != v_end);
	}
}
