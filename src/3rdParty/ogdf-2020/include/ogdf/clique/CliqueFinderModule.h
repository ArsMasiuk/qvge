/** \file
 * \brief Declares ogdf::CliqueFinderModule class.
 *
 * \author Karsten Klein, Max Ilsen
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
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/GraphAttributes.h>

namespace ogdf {

//! Finds cliques.
/**
 * @ingroup ga-induced
 *
 * A CliqueFinderModule can be called on a graph to retrieve (disjoint) cliques.
 */
class OGDF_EXPORT CliqueFinderModule {

public:
	/**
	 * Creates a CliqueFinderModule.
	 *
	 * By default, it searches for cliques containing at least three nodes.
	 * This setting can be changed with #setMinSize().
	 */
	explicit CliqueFinderModule()
		: m_pGraph(nullptr)
		, m_pCopy(nullptr)
		, m_minDegree(2)
	{ }

	virtual ~CliqueFinderModule() { }

	//! Searches for cliques and sets the clique index number for each node.
	/**
	 * Each clique will be assigned a different number. Each node gets the
	 * number of the clique it is contained in or -1 if the node is not a clique
	 * member.
	 * @param G The graph on which the clique finding algorithm is called.
	 * @param cliqueNumber The clique number for each node.
	 */
	void call(const Graph &G, NodeArray<int> &cliqueNumber);

	//! Searches for cliques and returns the list of cliques.
	/**
	 * Each clique is represented by a list of member nodes in the list of
	 * cliques \p cliqueLists.
	 * @param G The graph on which the clique finding algorithm is called.
	 * @param cliqueLists The list of cliques.
	 */
	void call(const Graph &G, List<List<node>*> &cliqueLists);

	//! @{
	//! \name Parameter Setters

	//! Sets the minimum size of a clique.
	void setMinSize(int i) { m_minDegree = max(0, i-1); }

	//! @}
	//! \name Helper Functions
	//! @{

	/**
	 * Uses a list of cliques to get the clique number of each node.
	 *
	 * @param G Graph the cliques belong to.
	 * @param cliqueLists List of lists, each one representing a clique.
	 * @param cliqueNumber Is assigned the clique number for each node.
	 */
	static void cliqueListToNumber(const Graph &G,
			const List<List<node>*> &cliqueLists,
			NodeArray<int> &cliqueNumber);

	/**
	 * Uses the clique number for each node to create a list of cliques.
	 *
	 * @param G Graph the cliques belong to.
	 * @param cliqueNumber The clique number for each node.
	 * @param cliqueLists Is assigned a list of lists, each one representing a
	 * clique.
	 */
	static void cliqueNumberToList(const Graph &G,
			const NodeArray<int> &cliqueNumber,
			List<List<node>*> &cliqueLists);

	/**
	 * Labels and colors nodes in the given GraphAttributes according to their
	 * clique number.
	 *
	 * Note that the coordinates of the nodes are not changed: A separate layout
	 * algorithm has to be used to this end.
	 *
	 * @param G Graph the cliques belong to.
	 * @param cliqueNumber The clique number for each node.
	 * @param GA Is assigned the node colors and labels.
	 */
	static void cliqueGraphAttributes(const Graph &G,
			const NodeArray<int> &cliqueNumber,
			GraphAttributes &GA);

	/**
	 * Checks whether density times the number of possible edges exist between
	 * clique members.
	 *
	 * @param G Graph the cliques belong to.
	 * @param clique The clique to check.
	 * @param density The fraction in [0,1] of possible edges between clique
	 * members that have to exist in order for the check to succeed.
	 * @return Whether the check succeeded.
	 */
	static bool cliqueOK(const Graph &G,
			List<node> *clique,
			double density = 1.0);

	//! @}

private:
	//! Initializes member variables and calls doCall().
	/**
	 * @param G The graph in which to search for cliques.
	 */
	void beginCall(const Graph &G);

	/**
	 * Sets the results of doCall() using #m_copyCliqueNumber.
	 *
	 * @param cliqueNumber Is assigned the clique number for each node.
	 */
	void setResults(NodeArray<int> &cliqueNumber);

	/**
	 * Sets the results of doCall() using #m_copyCliqueNumber.
	 * @warning The caller is responsible for deleting the list pointers.
	 *
	 * @param cliqueLists Is assigned a list of pointers, each one pointing to a
	 * list of nodes representing a clique.
	 */
	void setResults(List<List<node>*> &cliqueLists);

	//! Frees memory after doCall().
	void endCall();

	/**
	 * Checks whether finding cliques in #m_pCopy is trivial, i.e. n < 3 or
	 * n < #m_minDegree, and sets #m_copyCliqueNumber if that is the case.
	 * @returns true iff #m_pCopy is a trivial instance.
	 */
	bool handleTrivialCases();

	const Graph *m_pGraph; //!< The original Graph in which cliques are searched.

protected:
	GraphCopy *m_pCopy; //!< Copy of #m_pGraph without self-loops and multi-edges.

	NodeArray<int> m_copyCliqueNumber; //!< The clique number for each node in #m_pCopy.

	int m_minDegree; //!< Minimum degree of the nodes in a found clique.

	/**
	 * Find cliques in #m_pCopy. The found cliques are noted in
	 * #m_copyCliqueNumber. Clique nodes get a number >= 0, all other nodes -1.
	 */
	virtual void doCall() = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
