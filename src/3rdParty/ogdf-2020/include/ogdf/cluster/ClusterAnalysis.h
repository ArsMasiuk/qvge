/** \file
 * \brief Declaration of the ClusterAnalysis class for the Branch&Cut algorithm
 * for c-planarity testing via an extension to complete connectivity.
 * The computation of all values is done in an initial step, the results
 * are therefore static, later update of the c-graph does not trigger changes.
 *
 * The inner and outer activity status for vertices wrt the clusters is
 * detected and stored for static retrieval.
 * Bag index numbers are computed for vertices that represent their
 * bag affiliation for each cluster.
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

#pragma once

#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/basic/Skiplist.h>
#include <ogdf/basic/HashArray.h>

namespace ogdf {

/***
 * @ingroup ga-cplanarity
 *
 * Although most parts of the code are written with efficiency in mind,
 * this class is meant to be used in a static one-time analysis way,
 * not for dynamic checks over and over again.
 */
class ClusterAnalysis{

public:
	static const int IsNotActiveBound;
	static const int DefaultIndex; // Vertex index for independent bags, used to detect processing status.

	//! Constructor. Performs all analyses and in case indyBags is
	//! set to true, also computes a partition into independently
	//! solvable subproblems for cluster planarization (if applicable).
	explicit ClusterAnalysis(const ClusterGraph &C, bool indyBags = false);
	//! Additionally allows to forbid storing lists of outer active vertices.
	ClusterAnalysis(const ClusterGraph &C, bool oalists, bool indyBags);
	~ClusterAnalysis();

	// Quantitative
	//! Returns number of outeractive vertices of cluster c.
	// @param c is the cluster for which the active vertices are counted
	int outerActive(cluster c) const;

	//! Returns number of inneractive vertices of cluster c.
	// @param c is the cluster for which the active vertices are counted
	int innerActive(cluster c) const;

	//! Returns the highest (smallest) level depth for which a vertex
	//! is inner or outer active.
	int minIOALevel(node v) const {
		return min(minIALevel(v), minOALevel(v));
	}
	//! Returns the highest (smallest) level depth for which a vertex
	//! is inner active, only initialized if vertex is inner active.
	int minIALevel(node v) const {
		return m_ialevel[v];
	}
	//! Returns the highest (smallest) level depth for which a vertex
	//! is outer active, only initialized if vertex is outer active.
	int minOALevel(node v) const {
		return m_oalevel[v];
	}

	// Qualitative
	//! Returns outer activity status for vertex \p v wrt cluster \p c.
	/**
	*  @param c is the cluster for which vertex v's activity status is stored.
	*  @param v is the vertex for which the activity status is returned.
	*/
	bool isOuterActive(node v, cluster c) const;
	bool isInnerActive(node v, cluster c) const;

	//! Returns list of edges for cluster c with lca c.
	List<edge>& lcaEdges(cluster c);

	//! Returns list of outeractive vertices for cluster \p c. The result
	//! is only valid if lists are stored, i.e. m_storeoalists is true.
	List<node>& oaNodes(cluster c);

	//! Returns bag index number for a vertex \p v in cluster \p c.
	int bagIndex(node v, cluster c);

	//! Returns number of bags for cluster \p c.
	int numberOfBags(cluster c) const;

#if 0
	//TODO
	void reInit(const ClusterGraph &C);
#endif

	//! Returns independent bag index number for a vertex \p v.
	//! @pre indyBags parameter in constructor was set to true, i.e. indyBags were computed.
	int indyBagIndex(node v);

	//! Returns number of independent bags in clustergraph, -1 in case no independent bags
	//! were computed. Ascending consecutive numbers are assigned, starting from 0.
	int numberOfIndyBags() {return m_numIndyBags;}

	//! Returns root cluster of independent bag. Note that this
	//! cluster either has direct vertex members or more than one child.
	//! @param i is the independent bag number for which the root is returned.
	cluster indyBagRoot(int i);

protected:
	//! Compute bags per cluster and store result as vertex-bag
	//! index in m_bagIndex.
	void computeBags();

	//! Compute independent bags per cluster and store result
	//! as vertex-indyBag index in m_indyBagNumber.
	void computeIndyBags();

private:
	//! Runs through a list of vertices (starting with the one \p nodeIT points to)
	//! which is expected to be a full list of cluster vertices in \p c. Depending on
	//! outer activity and bag index number of the vertices, independent bags
	//! are detected and a corresponding index is assigned accordingly for each vertex.
	//! If omitChildBags is set to true, already processed vertices are skipped.
	void partitionCluster(ListConstIterator<node> & nodeIt, cluster c,
			HashArray<int, List<node> > & bagNodes, HashArray<int, bool> & indyBag,
			Skiplist<int*> &indexNumbers, Array<cluster> & bagRoots);
	void init(); //!< Initialize the structures, performs analyses.
	void cleanUp(); //!< Deletes dynamically allocated structures.
	const ClusterGraph* m_C;
	// we keep data structures to save inner/outer activity status
	// instead of computing them on the fly when needed
	// keep number of activity defining adjacent edges
	NodeArray< ClusterArray<int> *> m_iactive;
	NodeArray< ClusterArray<int> *> m_oactive;

	//! We store the bag affiliation of the vertices for each cluster.
	//! A value of -1 indicates that the vertex is not a member of the cluster.
	NodeArray< ClusterArray<int> *> m_bagindex;

	NodeArray<int> m_ialevel;
	NodeArray<int> m_oalevel;

	ClusterArray<int>* m_oanum; //!< Number of outer active vertices
	ClusterArray<int>* m_ianum; //!< Number of inner active vertices
	ClusterArray<int>* m_bags;  //!< Number of bags per cluster (stored even if vertex list is not stored)

	//! For each cluster we store the outeractive vertices.
	//! In case you want to save space, set m_storeoalists to false.
	ClusterArray<List<node> >* m_oalists;
	const bool m_storeoalists; //!< If set to true (default) lists of outeractive vertices are stored.

	ClusterArray<List<edge> >* m_lcaEdges; //!< For each cluster c we store the edges with lca c.

	//! If true, a node partition into independent bags is computed which can
	//! be used for dividing the input instance into smaller problems wrt cluster planarization.
	const bool m_indyBags;
	NodeArray<int> m_indyBagNumber;  //!< Each independent bag has a different number.
	int m_numIndyBags; //<! Number of independent bags in clustergraph
	cluster* m_indyBagRoots; //<! Root clusters of independent bags (only when computed).
};
}
