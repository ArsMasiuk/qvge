/** \file
 * \brief Implementation of Zelikovsky's 11/6-approximation algorithm
 * 	      for the minimum Steiner tree problem.
 *
 * \author Matthias Woste, Stephan Beyer
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

#include <ogdf/basic/List.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorVoronoi.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorEnumeration.h>
#include <ogdf/graphalg/steiner_tree/SaveStatic.h>
#include <ogdf/graphalg/steiner_tree/SaveEnum.h>
#include <ogdf/graphalg/steiner_tree/SaveDynamic.h>
#include <ogdf/graphalg/steiner_tree/Triple.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/graphalg/steiner_tree/common_algorithms.h>

namespace ogdf {

/*!
 * \brief This class implements the 11/6-approximation algorithm by Zelikovsky
 * for the minimum Steiner tree problem along with variants and practical improvements.
 *
 * @ingroup ga-steiner
 *
 * This implementation is based on:
 *
 * (A. Zelikovsky, An 11/6-Approximation Algorithm for the Network Steiner Problem,
 * Algorithmica, volume 9, number 5, pages 463-470, Springer, 1993)
 *
 * (A. Zelikovsky, A faster approximation algorithm for the Steiner problem in graphs,
 * Information Processing Letters, volume 46, number 2, pages 79-83, 1993)
 *
 * (A. Zelikovsky, Better approximation bound for the network and euclidean Steiner
 * tree problems, Technical Report, 2006)
 */
template<typename T>
class MinSteinerTreeZelikovsky: public MinSteinerTreeModule<T> {
public:
	template<typename TYPE> using Save = steiner_tree::Save<TYPE>;
	template<typename TYPE> using Triple = steiner_tree::Triple<TYPE>;

	//! Choice of objective function
	enum class WinCalculation {
		absolute, //!< win=gain-cost
		relative  //!< win=gain/cost
	};

	//! Choice of triple generation
	enum class TripleGeneration {
		exhaustive, //!< try all possibilities
		voronoi, //!< use voronoi regions
		ondemand  //!< generate triples "on the fly", only usable with WinCalculation::absolute
	};

	//! Switches immediate triple dropping
	enum class TripleReduction {
		on, //!< removes triples as soon as their gain is known to be non positive
		off //!< keeps triples all the time
	};

	//! Different methods for obtaining save edges
	enum class SaveCalculation {
		//! Stores explicitly the save edge for every pair of terminals.
		//! Needs O(n^2) space but has fast query times
		staticEnum,
		//! Builds a "weight tree" (save edges are inner nodes, terminals are leaves
		//! and searches save edges via LCA calculation of two nodes
		staticLCATree,
		//! Same as staticLCATree but each time a triple has been contracted
		//! the "weight tree" is updated dynamically rather than completely
		//! new from scratch.  Has the fastest update time
		dynamicLCATree,
		//! Uses staticEnum for the triple generation phase (many queries)
		//! and dynamicLCATree during the contraction phase (few updates)
		hybrid
	};

	//! Enables a heuristic version (for TG exhaustive and voronoi only)
	enum class Pass {
		//! heuristic: evaluate all triples, sort them descending by gain,
		//! traverse sorted triples once, contract when possible
		one,
		//! normal, greedy version
		multi
	};

	MinSteinerTreeZelikovsky(WinCalculation wc = WinCalculation::absolute,
	                         TripleGeneration tg = TripleGeneration::voronoi,
	                         SaveCalculation sc = SaveCalculation::hybrid,
	                         TripleReduction tr = TripleReduction::on,
	                         Pass pass = Pass::multi)
	 : m_winCalculation(wc)
	 , m_tripleGeneration(tg)
	 , m_saveCalculation(sc)
	 , m_tripleReduction(tr)
	 , m_pass(pass)
	 , m_ssspDistances(true)
	{
	}

	virtual ~MinSteinerTreeZelikovsky() { }

	virtual T call(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree) override
	{
		m_triplesGenerated = 0;
		m_tripleLookUps = 0;
		m_triplesContracted = 0;
		return MinSteinerTreeModule<T>::call(G, terminals, isTerminal, finalSteinerTree);
	}

	/*!
	 * \brief For the 3-restricted case, it is sufficient to compute an SSSP from every terminal
	 *  instead of doing a full APSP. In case a full APSP is faster, use this method.
	 * @param force True to force APSP instead of SSSP.
	 */
	void forceAPSP(bool force = true)
	{
		m_ssspDistances = !force;
	}

	//! Sets type of gain calculation \see MinSteinerTreeZelikovsky::WinCalculation
	void winCalculation(WinCalculation wc)
	{
		m_winCalculation = wc;
	}

	//! Returns type of gain calculation currently in use \see MinSteinerTreeZelikovsky::WinCalculation
	WinCalculation winCalculation() const
	{
		return m_winCalculation;
	}

	//! Sets type of triple generation \see MinSteinerTreeZelikovsky::TripleGeneration
	void tripleGeneration(TripleGeneration tg)
	{
		m_tripleGeneration = tg;
	}

	//! Returns type of triple generation currently in use \see MinSteinerTreeZelikovsky::TripleGeneration
	TripleGeneration tripleGeneration() const
	{
		return m_tripleGeneration;
	}

	//! Sets type of triple reduction \see MinSteinerTreeZelikovsky::TripleReduction
	void tripleReduction(TripleReduction tr)
	{
		m_tripleReduction = tr;
	}

	//! Returns type of triple reduction currently in use \see MinSteinerTreeZelikovsky::TripleReduction
	TripleReduction tripleReduction() const
	{
		return m_tripleReduction;
	}

	//! Sets type of save calculation \see MinSteinerTreeZelikovsky::SaveCalculation
	void saveCalculation(SaveCalculation sv)
	{
		m_saveCalculation = sv;
	}

	//! Returns type of save calculation currently in use \see MinSteinerTreeZelikovsky::SaveCalculation
	SaveCalculation saveCalculation() const
	{
		return m_saveCalculation;
	}

	//! Sets type of pass \see MinSteinerTreeZelikovsky::Pass
	void pass(Pass p) {
		m_pass = p;
	}

	//! Returns type of pass currently in use \see MinSteinerTreeZelikovsky::Pass
	Pass pass() const
	{
		return m_pass;
	}

	//! Returns the number of generated triples
	long numberOfGeneratedTriples() const
	{
		return m_triplesGenerated;
	}

	//! Returns the number of contracted triples
	long numberOfContractedTriples() const
	{
		return m_triplesContracted;
	}

	//! Returns the number of triple lookups during execution time
	long numberOfTripleLookUps() const
	{
		return m_tripleLookUps;
	}

protected:
	/*!
	 * \brief Builds a minimum Steiner tree given a weighted graph and a list of terminals \see MinSteinerTreeModule::call
	 * @param G The weighted input graph
	 * @param terminals The list of terminal nodes
	 * @param isTerminal A bool array of terminals
	 * @param finalSteinerTree The final Steiner tree
	 * @return The objective value (sum of edge costs) of the final Steiner tree
	 */
	virtual T computeSteinerTree(
		const EdgeWeightedGraph<T> &G,
		const List<node> &terminals,
		const NodeArray<bool> &isTerminal,
		EdgeWeightedGraphCopy<T> *&finalSteinerTree) override;

	/*!
	 * \brief Computes the distance matrix for the original graph
	 */
	void computeDistanceMatrix();

	/*!
	 * \brief Add a found triple to the triples list. (Just a helper to avoid code duplication.)
	 */
	inline void generateTriple(node u, node v, node w, node center, const T &minCost, const Save<T> &save)
	{
		const double gain = save.gain(u, v, w);
		const double win = calcWin(gain, minCost);
		if (tripleReduction() == TripleReduction::off
		 || win > 0) {
			++m_triplesGenerated;
			OGDF_ASSERT(center);
			Triple<T> triple(u, v, w, center, minCost, win);
			m_triples.pushBack(triple);
		}
	}

	/*!
	 * \brief Generates triples using the given full 3-component generator
	 * @param save data structure for calculation save edges
	 * @param fcg the chosen full 3-component generator
	 */
	inline void generateTriples(const Save<T> &save, const steiner_tree::Full3ComponentGeneratorModule<T> &fcg)
	{
		fcg.call(*m_originalGraph, *m_terminals, *m_isTerminal, m_distance, m_pred,
		  [this, &save](node u, node v, node w, node center, T minCost) {
			generateTriple(u, v, w, center, minCost, save);
		});
	}

	/*!
	 * \brief Generates triples according to the chosen option \see TripleGeneration
	 * @param save data structure for calculation save edges
	 */
	inline void generateTriples(const Save<T> &save)
	{
		OGDF_ASSERT(tripleGeneration() != TripleGeneration::ondemand);
		if (tripleGeneration() == TripleGeneration::voronoi) {
			steiner_tree::Full3ComponentGeneratorVoronoi<T> fcg;
			generateTriples(save, fcg);
		} else {
			OGDF_ASSERT(tripleGeneration() == TripleGeneration::exhaustive);
			steiner_tree::Full3ComponentGeneratorEnumeration<T> fcg;
			generateTriples(save, fcg);
		}
	}

	/*!
	 * \brief Contracts a triple and updates the save data structure
	 * @param triple triple to be contracted
	 * @param save save data structure
	 * @param isNewTerminal true for nodes to be interpreted as terminals
	 */
	void contractTriple(const Triple<T> &triple, Save<T> &save, NodeArray<bool> &isNewTerminal)
	{
		++m_triplesContracted;
		save.update(triple);
		isNewTerminal[triple.z()] = true;
	}

	/*!
	 * \brief Contraction phase for algorithm generating triples on demand \see MinSteinerTreeZelikovsky::ondemand
	 * @param save save data structure
	 * @param isNewTerminal true for nodes to be interpreted as terminals
	 */
	void tripleOnDemand(Save<T> &save, NodeArray<bool> &isNewTerminal);

	/**
	 * \brief Find the best triple for a given nonterminal center
	 * @param center the center node we want to find a better triple for
	 * @param save save data structure
	 * @param maxTriple the improved triple (output parameter)
	 * @return True iff maxTriple is updated
	 */
	bool findBestTripleForCenter(node center, const Save<T> &save, Triple<T> &maxTriple) const
	{
		bool updated = false; // return value

		// find s0, nearest terminal to center
		T best = std::numeric_limits<T>::max();
		node s0 = nullptr;
		for (node s : *m_terminals) {
			T tmp = m_distance[s][center];
			if (best > tmp) {
				best = tmp;
				s0 = s;
			}
		}
		OGDF_ASSERT(s0);
		OGDF_ASSERT(m_pred[s0][center]);

		// find s1 maximizing save(s0, s1) - d(center, s1)
		node s1 = nullptr;
		T save1Val(0);
		for (node s : *m_terminals) {
			if (s != s0
			 && m_pred[s][center] != nullptr) {
				OGDF_ASSERT(m_distance[s][center] != std::numeric_limits<T>::max());
				T tmpVal = save.saveWeight(s, s0);
				T tmp = tmpVal - m_distance[s][center];
				if (!s1 || best < tmp) {
					best = tmp;
					s1 = s;
					save1Val = tmpVal;
				}
			}
		}
		if (s1) {
			OGDF_ASSERT(m_pred[s1][center]);
			node s2 = nullptr;
			T save2Val(0);
			const edge save1 = save.saveEdge(s0, s1);
			for (node s : *m_terminals) {
				if (s != s0
				 && s != s1
				 && m_pred[s][center] != nullptr) {
					OGDF_ASSERT(m_distance[s][center] != std::numeric_limits<T>::max());
					const edge tmp = save.saveEdge(s0, s);
					save2Val = save.saveWeight(tmp == save1 ? s1 : s0, s);
					T tmpWin = save1Val + save2Val - m_distance[s0][center] - m_distance[s1][center] - m_distance[s][center];
					if (!s2 || best < tmpWin) {
						best = tmpWin;
						s2 = s;
					}
				}
			}

			if (s2 // it may happen that s2 does not exist
			 && best > maxTriple.win()) { // best win is better than previous best; also positive
				OGDF_ASSERT(m_pred[s2][center]);
				maxTriple.s0(s0);
				maxTriple.s1(s1);
				maxTriple.s2(s2);
				maxTriple.z(center);
				maxTriple.win(best);
				//maxTriple.cost(save1Val + save2Val - win); not needed
				updated = true;
			}
		}
		return updated;
	}

	/*!
	 * \brief Contraction phase for the original version of the algorithm \see MinSteinerTreeZelikovsky::multi
	 * @param save save data structure
	 * @param isNewTerminal true for nodes to be interpreted as terminals
	 */
	void multiPass(Save<T> &save, NodeArray<bool> &isNewTerminal);

	/*!
	 * \brief Contraction phase for the one pass heuristic \see MinSteinerTreeZelikovsky::one
	 * @param save save data structure
	 * @param isNewTerminal true for nodes to be interpreted as terminals
	 */
	void onePass(Save<T> &save, NodeArray<bool> &isNewTerminal);

	/*!
	 * \brief Calculate the win
	 */
	double calcWin(double gain, T cost) const
	{
		switch (winCalculation()) {
		case WinCalculation::relative:
			return gain / cost - 1.0;
		case WinCalculation::absolute:
		default:
			return gain - cost;
		}
	}

	void generateInitialTerminalSpanningTree(EdgeWeightedGraphCopy<T> &steinerTree)
	{
		// generate complete graph
		for (node v : *m_terminals) {
			steinerTree.newNode(v);
		}
		for (node u : steinerTree.nodes) {
			const NodeArray<T> &dist = m_distance[steinerTree.original(u)];
			for (node v = u->succ(); v; v = v->succ()) {
				steinerTree.newEdge(u, v, dist[steinerTree.original(v)]);
			}
		}
		// compute MST
		makeMinimumSpanningTree(steinerTree, steinerTree.edgeWeights());
	}

private:
	WinCalculation m_winCalculation; //!< Chosen option for gain calculation \see WinCalculation
	TripleGeneration m_tripleGeneration; //!< Chosen option for triple generation \see TripleGeneration
	SaveCalculation m_saveCalculation; //!< Chosen option for save calculation \see SaveCalculation
	TripleReduction m_tripleReduction; //!< Chosen option for triple reduction \see TripleReduction
	Pass m_pass; //!< Chosen option for pass \see Pass
	bool m_ssspDistances; //!< True iff we only compute SSSP from terminals instead of APSP for full component construction

	const EdgeWeightedGraph<T> *m_originalGraph; //!< The original edge-weighted graph
	const NodeArray<bool> *m_isTerminal; //!< Incidence vector for terminal nodes
	const List<node> *m_terminals; //!< List of terminal nodes
	NodeArray<NodeArray<T>> m_distance; //!< The distance matrix
	NodeArray<NodeArray<edge>> m_pred; //!< The predecessor matrix
	List<Triple<T>> m_triples; //!< The list of triples during the algorithm

	long m_triplesGenerated; //!< Number of generated triples
	long m_triplesContracted; //!< Number of contracted triples
	long m_tripleLookUps; //!< Number of triple lookups

};

template<typename T>
T MinSteinerTreeZelikovsky<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	OGDF_ASSERT(tripleGeneration() != TripleGeneration::ondemand // combinations that only work with ondemand:
	 || (winCalculation() == WinCalculation::absolute
	  && saveCalculation() != SaveCalculation::hybrid
	  && pass() != Pass::one));

	m_originalGraph = &G;
	m_terminals = &terminals;
	m_isTerminal = &isTerminal;

	// build the complete terminal graph and a distance matrix
	NodeArray<bool> isNewTerminal(G, false);
	for (node v : *m_terminals) {
		isNewTerminal[v] = true;
	}

	if (terminals.size() >= 3) {
		computeDistanceMatrix();

		// init terminal-spanning tree and its save-edge data structure
		EdgeWeightedGraphCopy<T> steinerTree; // the terminal-spanning tree to be modified
		steinerTree.createEmpty(G);
		generateInitialTerminalSpanningTree(steinerTree);

		Save<T> *save = nullptr;
		switch (saveCalculation()) {
		case SaveCalculation::staticEnum:
			save = new steiner_tree::SaveEnum<T>(steinerTree);
			break;
		case SaveCalculation::staticLCATree:
			save = new steiner_tree::SaveStatic<T>(steinerTree);
			break;
		case SaveCalculation::dynamicLCATree:
		case SaveCalculation::hybrid:
			save = new steiner_tree::SaveDynamic<T>(steinerTree);
			break;
		}
		OGDF_ASSERT(save);

		if (tripleGeneration() == TripleGeneration::ondemand) { // ondemand triple generation
			tripleOnDemand(*save, isNewTerminal);
		} else { // exhaustive or voronoi
			// triple generation phase
			if (saveCalculation() == SaveCalculation::hybrid) {
				steiner_tree::SaveEnum<T> saveTriple(steinerTree);
				generateTriples(saveTriple);
			} else {
				generateTriples(*save);
			}
			// contraction phase
			if (pass() == Pass::multi) {
				multiPass(*save, isNewTerminal);
			} else { // pass() == one
				onePass(*save, isNewTerminal);
			}
		}
		delete save;

		// cleanup
		m_triples.clear();
	}

	// obtain final Steiner Tree using (MST-based) Steiner tree approximation algorithm
	return steiner_tree::obtainFinalSteinerTree(G, isNewTerminal, isTerminal, finalSteinerTree);
}

template<typename T>
void MinSteinerTreeZelikovsky<T>::computeDistanceMatrix()
{
	if (m_ssspDistances) {
		MinSteinerTreeModule<T>::allTerminalShortestPaths(*m_originalGraph, *m_terminals, *m_isTerminal, m_distance, m_pred);
	} else {
		MinSteinerTreeModule<T>::allPairShortestPaths(*m_originalGraph, *m_isTerminal, m_distance, m_pred);
	}
}

template<typename T>
void MinSteinerTreeZelikovsky<T>::tripleOnDemand(Save<T> &save, NodeArray<bool> &isNewTerminal)
{
	Triple<T> maxTriple;
	ArrayBuffer<node> nonterminals;
	MinSteinerTreeModule<T>::getNonterminals(nonterminals, *m_originalGraph, *m_isTerminal);
	do {
		maxTriple.win(0);
		for (node center : nonterminals) {
			if (findBestTripleForCenter(center, save, maxTriple)) {
				++m_triplesGenerated;
			}
		}

		if (maxTriple.win() > 0) {
			contractTriple(maxTriple, save, isNewTerminal);
		}
	} while (maxTriple.win() > 0);
}


template<typename T>
void MinSteinerTreeZelikovsky<T>::onePass(Save<T> &save, NodeArray<bool> &isNewTerminal)
{
	m_triples.quicksort(GenericComparer<Triple<T>, double>([](const Triple<T>& x) -> double { return -x.win(); }));

	for (const Triple<T> &t : m_triples) {
		++m_tripleLookUps;
		if (calcWin(double(save.gain(t.s0(), t.s1(), t.s2())), t.cost()) > 0) {
			contractTriple(t, save, isNewTerminal);
		}
	}
}


template<typename T>
void MinSteinerTreeZelikovsky<T>::multiPass(Save<T> &save, NodeArray<bool> &isNewTerminal)
{
	double win = 0;
	ListIterator<Triple<T>> maxTriple;

	do {
		win = 0;
		ListIterator<Triple<T>> nextIt;
		for (ListIterator<Triple<T>> it = m_triples.begin(); it.valid(); it = nextIt) {
			nextIt = it.succ();
			++m_tripleLookUps;
			Triple<T> &t = *it;
			t.win(calcWin(double(save.gain(t.s0(), t.s1(), t.s2())), t.cost()));
			if (t.win() > win) {
				win = t.win();
				maxTriple = it;
			} else {
				if (tripleReduction() == TripleReduction::on
				 && t.win() <= 0) {
					m_triples.del(it);
				}
			}
		}

		if (win > 0) {
			contractTriple(*maxTriple, save, isNewTerminal);
			m_triples.del(maxTriple);
		}
	} while (win > 0);
}

}
