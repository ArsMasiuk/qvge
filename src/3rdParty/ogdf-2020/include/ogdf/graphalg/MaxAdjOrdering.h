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

#pragma once

#include <ogdf/basic/basic.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/graphics.h>
#include <ogdf/misclayout/LinearLayout.h>

namespace ogdf {

/**
 * \brief Calculate one or all Maximum Adjacency Ordering(s) of a given simple undirected graph.
 *
 *
 * The class MaxAdjOrdering provides one algorithm to calculate a MAO or all MAOs of a given graph.
 * It returns a ListPure of nodes or a list of ListPures that contain the ordering.
 *
 */
class OGDF_EXPORT MaxAdjOrdering{
private:
	/**
	* \brief This method gets called recursively to generate all MAOs
	* via backtracking
	* @param n Number of nodes
	* @param currentOrder The partial MAO to this point
	* @param currentUnsorted Nodes that are still left to sort
	* @param r Values on the nodes that count the edges to the partial MAO
	* @param MAOs The list of all MAOs to this point
	*/
	void m_calcAllMAOs_recursion(
	                             int n,
	                             ListPure<node> currentOrder,
	                             ListPure<node> currentUnsorted,
	                             NodeArray<int> r,
	                             ListPure<ListPure<node>> *MAOs
	                            );

	/**
	* \brief This method gets called recursively to generate all MAOs and their
	* induced forest decompositions of the edges
	* via backtracking
	* @param n Number of nodes
	* @param currentOrder The partial MAO to this point
	* @param currentForest The partial forest decomposition to this point
	* @param currentUnsorted Nodes that are still left to sort
	* @param r Values on the nodes that count the edges to the partial MAO
	* @param MAOs The list of all MAOs to this point
	* @param Fs The list of all forests to this point
	*/
	void m_calcAllMAOs_recursion(
	                             int n,
	                             ListPure<node> currentOrder,
	                             ListPure<ListPure<edge>> currentForest,
	                             ListPure<node> currentUnsorted,
	                             NodeArray<int> r,
	                             ListPure<ListPure<node>> *MAOs,
	                             ListPure<ListPure<ListPure<edge>>> *Fs
	                            );
public:
	/**
	* \brief Standard constructor
	*/
	MaxAdjOrdering();
	/**
	* \brief Standard destructor
	*/
	~MaxAdjOrdering();

	/**
	* \brief Calculates one MAO starting with the node with index 0
	* @param G is the Graph to work on
	* @param MAO is the pointer to a list that stores the MAO
	*/
	void calc(
	          const Graph *G,
	          ListPure<node> *MAO
	         );
	/**
	* \brief Calculates one MAO starting with the node with index 0
	* and lex-bfs tie breaking.
	* @param G is the Graph to work on
	* @param MAO is the pointer to a list that stores the MAO
	*/
	void calcBfs(
	             const Graph *G,
	             ListPure<node> *MAO
	            );

	/**
	* \brief Calculates one MAO starting with a given node
	* @param G is the Graph to work on
	* @param s Node to start the MAO with
	* @param MAO is the pointer to a list that stores the MAO
	*/
	void calc(
	          const Graph *G,
	          node s,
	          ListPure<node> *MAO
	         );

	/**
	* \brief Calculates one MAO starting with the node with index 0
	* @param G is the Graph to work on
	* @param MAO is the pointer to a list that stores the MAO
	* @param Forests is the pointer to a list that stores the forest decomposition associated with the MAO
	*/
	void calc(
	         const Graph *G,
	         ListPure<node> *MAO,
	         ListPure<ListPure<edge>> *Forests
	         );

	/**
	* \brief Calculates one MAO starting with a given node
	* @param G is the Graph to work on
	* @param s Node to start the MAO with
	* @param MAO is the pointer to a list that stores the MAO
	* @param Forests is the pointer to a list that stores the forest decomposition associated with the MAO
	*/
	void calc(
	          const Graph *G,
	          node s,
	          ListPure<node> *MAO,
	          ListPure<ListPure<edge>> *Forests
	         );

	/**
	* \brief Calculates all MAOs of a given graph
	* @param G is the Graph to work on
	* @param MAOs List of all MAOs. So it must be a list of lists
	*/
	void calcAll(
	             const Graph *G,
	             ListPure<ListPure<node>> *MAOs
	            );

	/**
	* \brief Calculates all MAOs including their associated forest decompositions of a given graph
	* @param G is the graph to work on
	* @param MAOs List of all MAOs. So it must be a list of lists of vertices.
	* @param Fs List of all forest decompositions. So it must be a list of lists of lists of edges.
	*/
	void calcAll(
	             const Graph *G,
	             ListPure<ListPure<node>> *MAOs,
	             ListPure<ListPure<ListPure<edge>>> *Fs
	            );
	/**
	 * \brief Test if a given ordering is a MAO
	 * @param G is the graph to work on
	 * @param Ordering is a ListPure that contains a permutation of the nodes
	*/
	bool testIfMAO(
	               const Graph *G,
	               ListPure< node> *Ordering
	              );

	/**
	 * \brief Test if a given ordering is a MAO that follows lex-bfs tie breaking
	 * @param G is the graph to work on
	 * @param Ordering is a ListPure that contains a permutation of the nodes
	*/
	bool testIfMAOBfs(
	                  const Graph *G,
	                  ListPure< node> *Ordering
	                 );


	/**
	 * @brief testIfAllMAOs checks all permutations (must be provided) if they are a MAO and if
	 * yes searches this ordering in the provided list. If one permutation is no MAO
	 * it still gets searched to rule out any false elements in \p Perms. So we make sure we generated
	 * all MAOs of \p G.
	 * @param G is the graph to work on
	 * @param Orderings contains Lists that are supposedly MAOs
	 * @param Perms contains all permutations of a graph with the same size as \p G
	 * @return
	 */
	bool testIfAllMAOs(
	                   const Graph *G,
	                   ListPure<ListPure<node>> *Orderings,
	                   ListPure<ListPure<node>> *Perms
	                  );
	/**
	* \brief Convenient way to visualize a MAO with the LinearLayout class.
	* @param GA Graphattributes of the Graph to paint
	* @param MAO Mao to use for ordering the nodes
	*/
	void visualize(
	               GraphAttributes *GA,
	               ListPure<node> *MAO
	              );

	/**
	* \brief Convenient way to visualize a MAO with the LinearLayout class.
	* @param GA Graphattributes of the Graph to paint
	* @param MAO Mao to use for ordering the nodes
	* @param F A Forest can also be included
	*/
	void visualize(
	               GraphAttributes *GA,
	               ListPure<node> *MAO,
	               ListPure<ListPure<edge>> *F
	              );
	};

}
