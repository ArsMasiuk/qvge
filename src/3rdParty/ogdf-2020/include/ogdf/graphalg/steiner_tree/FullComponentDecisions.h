/** \file
 * \brief Definition of the FullComponentDecisions class
 *
 * \author Stephan Beyer
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

namespace ogdf {
namespace steiner_tree {

//! Contains rules of thumb to decide which (sub-)algorithms to use
//! for the generation of full components.
struct FullComponentDecisions {
	/**
	 * Computes the ratio of edges to potential edges in a simple graph
	 * @param n number of nodes
	 * @param m number of edges
	 */
	static inline double computeDensity(int n, int m) {
		double density(2.0 * m);
		density /= n;
		density /= n - 1;
		return density;
	}

	/**
	 * Returns true iff the rule of thumb predicts to call
	 * Dijkstra on all terminals instead of the algorithm by Floyd
	 * @param n number of nodes
	 * @param m number of edges
	 * @param t number of terminals
	 */
	static bool shouldUseAllTerminalDijkstra(int n, int m, int t) {
		double coverage(t);
		coverage /= n;
		if (coverage < 0.07) {
			return true;
		}

		double density{computeDensity(n, m)};
		if (density > 0.5) {
			return false;
		}
		if (density > 0.1 && coverage > 0.3) {
			return false;
		}
		return true;
	}

	/**
	 * Returns true iff the rule of thumb predicts to call
	 * Dijkstra on all nodes instead of the algorithm by Floyd
	 * @param n number of nodes
	 * @param m number of edges
	 */
	static inline bool shouldUseAllNodeDijkstra(int n, int m) {
		return computeDensity(n, m) <= 0.15;
	}

	/**
	 * Returns true iff the rule of thumb predicts to use
	 * multiple Dijkstra calls instead of the algorithm by Floyd
	 * @param k maximum number of terminals in components
	 * @param n number of nodes
	 * @param m number of edges
	 * @param t number of terminals
	 */
	static bool shouldUseDijkstra(int k, int n, int m, int t) {
		if (k == 3) {
			return shouldUseAllTerminalDijkstra(n, m, t);
		}
		return shouldUseAllNodeDijkstra(n, m);
	}

	/**
	 * Returns true iff the rule of thumb predicts to use
	 * the algorithm by Erickson et al instead of the
	 * Dreyfus-Wagner algorithm.
	 * @param n number of nodes
	 * @param m number of edges
	 */
	static inline bool shouldUseErickson(int n, int m) {
		return computeDensity(n, m) < 0.0029;
	}
};

}
}
