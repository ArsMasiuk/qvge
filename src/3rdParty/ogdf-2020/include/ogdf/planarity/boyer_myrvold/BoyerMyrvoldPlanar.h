/** \file
 * \brief Declaration of the class BoyerMyrvoldPlanar
 *
 * \author Jens Schmidt
 *
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

#include <random>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/SList.h>

namespace ogdf {

//! Type of edge
enum class BoyerMyrvoldEdgeType {
	Undefined=0, //!< undefined
	Selfloop=1, //!< selfloop
	Back=2, //!< backedge
	Dfs=3, //!< DFS-edge
	DfsParallel=4, //!< parallel DFS-edge
	BackDeleted=5 //!< deleted backedge
};

class KuratowskiStructure;
class FindKuratowskis;
namespace boyer_myrvold {
class BoyerMyrvoldInit;
}

//! This class implements the extended BoyerMyrvold planarity embedding algorithm
class BoyerMyrvoldPlanar
{
	friend class BoyerMyrvold;
	friend class boyer_myrvold::BoyerMyrvoldInit;
	friend class FindKuratowskis;
	friend class ExtractKuratowskis;

public:
	//! Direction for counterclockwise traversal
	const static int DirectionCCW;

	//! Direction for clockwise traversal
	const static int DirectionCW;

	//! Denotes the different embedding options
	enum class EmbeddingGrade {
		doNotEmbed=-3, // and not find any kuratowski subdivisions
		doNotFind=-2, // but embed
		doFindUnlimited=-1, // and embed
		doFindZero=0 // and embed
	};

	//! Constructor, for parameters see BoyerMyrvold
	BoyerMyrvoldPlanar(
		Graph& g,
		bool bundles,
		int embeddingGrade,
		bool limitStructures,
		SListPure<KuratowskiStructure>& output,
		double randomness,
		bool avoidE2Minors,
		bool extractSubgraph,
		const EdgeArray<int> *edgeCosts = nullptr);

	//! Constructor, for parameters see BoyerMyrvold
	BoyerMyrvoldPlanar(
		Graph& g,
		bool bundles,
		EmbeddingGrade embeddingGrade,
		bool limitStructures,
		SListPure<KuratowskiStructure>& output,
		double randomness,
		bool avoidE2Minors,
		bool extractSubgraph,
		const EdgeArray<int> *edgeCosts = nullptr)
	: BoyerMyrvoldPlanar(g, bundles, static_cast<int>(embeddingGrade), limitStructures, output, randomness, avoidE2Minors, extractSubgraph, edgeCosts)
	{}

	//! Starts the embedding algorithm
	bool start();

	//! Flips all nodes of the bicomp with unique, real, rootchild c as necessary
	/** @param c is the unique rootchild of the bicomp
	 * @param marker is the value which marks nodes as visited
	 * @param visited is the array containing visiting information
	 * @param wholeGraph Iff true, all bicomps of all connected components will be traversed
	 * @param deleteFlipFlags Iff true, the flipping flags will be deleted after flipping
	 */
	void flipBicomp(
		int c,
		int marker,
		NodeArray<int>& visited,
		bool wholeGraph,
		bool deleteFlipFlags);

	// avoid automatic creation of assignment operator
	//! Assignment operator is undefined!
	BoyerMyrvoldPlanar &operator=(const BoyerMyrvoldPlanar &);


	//! Seeds the random generator for performing a random DFS.
	//! If this method is never called the random generator will be seeded by a value
	//! extracted from the global random generator.
	void seed(const std::minstd_rand rand) {
		m_rand = rand;
	}

protected:
	//! \name Methods for Walkup and Walkdown
	//! @{

	//! Checks whether node \p w is pertinent. \p w has to be non-virtual.
	inline bool pertinent(node w) const {
		OGDF_ASSERT(w!=nullptr);
		return m_dfi[w] > 0 && (!m_backedgeFlags[w].empty() || !m_pertinentRoots[w].empty());
	}

	//! Checks whether real node \p w is internally active while embedding node with DFI \p v
	inline bool internallyActive(node w, int v) const {
		OGDF_ASSERT(w!=nullptr);
		return m_dfi[w] > 0 && pertinent(w) && !externallyActive(w, v);
	}

	//! Checks whether real node \p w is externally active while embedding node with DFI \p v
	inline bool externallyActive(node w, int v) const {
		OGDF_ASSERT(w!=nullptr);
		if (m_dfi[w] <= 0) return false;
		if (m_leastAncestor[w] < v) return true;
		return !m_separatedDFSChildList[w].empty() && m_lowPoint[m_separatedDFSChildList[w].front()] < v;
	}

	//! Checks whether real node \p w is inactive while embedding node with DFI \p v
	inline bool inactive(node w, int v) {
		OGDF_ASSERT(w!=nullptr);
		if (m_dfi[w] <= 0) return true;
		if (!m_backedgeFlags[w].empty() || !m_pertinentRoots[w].empty()
			|| m_leastAncestor[w] < v) return false;
		return m_separatedDFSChildList[w].empty() || m_lowPoint[m_separatedDFSChildList[w].front()] >= v;
	}

	//! Checks all dynamic information about a node \p w while embedding node with DFI \p v
	/**
	 * @return This method returns the following values:
	 *   - 0 = inactive
	 *   - 1 = internallyActive
	 *   - 2 = pertinent and externallyActive
	 *   - 3 = externallyActive and not pertinent
	 */
	inline int infoAboutNode(node w, int v) const {
		OGDF_ASSERT(w!=nullptr);
		if (m_dfi[w] <= 0) return 0;
		if (!m_pertinentRoots[w].empty() || !m_backedgeFlags[w].empty()) {
			// pertinent
			if (m_leastAncestor[w] < v) return 2;
			if (m_separatedDFSChildList[w].empty()) return 1;
			return m_lowPoint[m_separatedDFSChildList[w].front()] < v ? 2 : 1;
		} else {
			// not pertinent
			if (m_leastAncestor[w] < v) return 3;
			if (m_separatedDFSChildList[w].empty()) return 0;
			return m_lowPoint[m_separatedDFSChildList[w].front()] < v ? 3 : 0;
		}
	}

	//! Walks upon external face in the given \p direction starting at \p w
	/** If none of the bicomps has been flipped then CW = clockwise and
	 * CCW = counterclockwise holds. In general, the traversaldirection could have
	 * been changed due to flipped components. If this occurs, the
	 * traversaldirection is flipped.
	 */
	inline node successorOnExternalFace(node w, int& direction) const {
		OGDF_ASSERT(w!=nullptr);
		OGDF_ASSERT(w->degree()>0);
		OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCW][w]!=nullptr);
		OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCCW][w]!=nullptr);
		adjEntry adj = m_link[direction][w];
		OGDF_ASSERT(adj->theNode()!=nullptr);

		if (w->degree() > 1) direction =
				adj==beforeShortCircuitEdge(adj->theNode(),BoyerMyrvoldPlanar::DirectionCCW)->twin();
		OGDF_ASSERT(direction || adj==beforeShortCircuitEdge(adj->theNode(),BoyerMyrvoldPlanar::DirectionCW)->twin());
		return adj->theNode();
	}

	//! Walks upon external face in given \p direction avoiding short circuit edges
	inline node successorWithoutShortCircuit(node w, int& direction) {
		OGDF_ASSERT(w!=nullptr);
		OGDF_ASSERT(w->degree()>0);
		OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCW][w]!=nullptr);
		OGDF_ASSERT(m_link[BoyerMyrvoldPlanar::DirectionCCW][w]!=nullptr);
		adjEntry adj = beforeShortCircuitEdge(w,direction);
		OGDF_ASSERT(adj->theNode()!=nullptr);

		if (w->degree() > 1) direction =
				adj==beforeShortCircuitEdge(adj->theNode(),BoyerMyrvoldPlanar::DirectionCCW)->twin();
		OGDF_ASSERT(direction || adj==beforeShortCircuitEdge(adj->theNode(),BoyerMyrvoldPlanar::DirectionCW)->twin());
		return adj->theNode();
	}

	//! Returns the successornode on the external face in given \p direction
	/** \p direction is not changed.
	 */
	inline node constSuccessorOnExternalFace(node v, int direction) {
		OGDF_ASSERT(v!=nullptr);
		OGDF_ASSERT(v->degree()>0);
		return m_link[direction][v]->theNode();
	}

	//! Walks upon external face in \p direction avoiding short circuit edges
	/** \p direction is not changed.
	 */
	inline node constSuccessorWithoutShortCircuit(node v, int direction) const {
		OGDF_ASSERT(v!=nullptr);
		OGDF_ASSERT(v->degree()>0);
		return beforeShortCircuitEdge(v,direction)->theNode();
	}

	//! Returns underlying former adjEntry, if a short circuit edge in \p direction of \p v exists
	/** Otherwise the common edge is returned. In every case the returned adjEntry
	 * points to the targetnode.
	 */
	inline adjEntry beforeShortCircuitEdge(node v, int direction) const {
		OGDF_ASSERT(v!=nullptr);
		return (m_beforeSCE[direction][v]==nullptr) ? m_link[direction][v] : m_beforeSCE[direction][v];
	}

	//! Walks upon external face in the given \p direction starting at \p w until an active vertex is reached
	/** Returns dynamical typeinformation \p info of that endvertex.
	 */
	node activeSuccessor(node w, int& direction, int v, int& info) const;

	//! Walks upon external face in the given \p direction (without changing it) until an active vertex is reached
	/** Returns dynamical typeinformation \p info of that endvertex. But does not change the \p direction.
	 */
	inline node constActiveSuccessor(node w, int direction, int v, int& info) const {
		return activeSuccessor(w,direction,v,info);
	}

	//! Checks, if one ore more wNodes exist between the two stopping vertices \p stopx and \p stopy
	/** The node \p root is root of the bicomp containing the stopping vertices
	 */
	inline bool wNodesExist(node root, node stopx, node stopy) const {
		OGDF_ASSERT(root != stopx);
		OGDF_ASSERT(root != stopy);
		OGDF_ASSERT(stopx != stopy);
		int dir = BoyerMyrvoldPlanar::DirectionCCW;
		bool between = false;
		while (root != nullptr) {
			root = successorOnExternalFace(root,dir);
			if (between && pertinent(root)) {
				return true;
			}
			if (root == stopx || root == stopy) {
				if (between) {
					return false;
				}
				between = true;
			}
		}
		return false;
	}

	//! Prints informations about node \p v
	inline void printNodeInfo(node v) {
		std::cout
		  << "\nprintNodeInfo(" << m_dfi[v] << "): "
		  << "CCW=" << m_dfi[constSuccessorOnExternalFace(v, BoyerMyrvoldPlanar::DirectionCCW)]
		  << ",CW=" << m_dfi[constSuccessorOnExternalFace(v, BoyerMyrvoldPlanar::DirectionCW)]
		  << "\tCCWBefore=" << m_dfi[constSuccessorWithoutShortCircuit(v, BoyerMyrvoldPlanar::DirectionCCW)]
		  << ",CWBefore=" << m_dfi[constSuccessorWithoutShortCircuit(v, BoyerMyrvoldPlanar::DirectionCW)]
		  << "\tadjList: ";
		adjEntry adj;
		for (adj = v->firstAdj(); adj; adj = adj->succ()) {
			std::cout << adj->twinNode() << " ";
		}
	}

	//! Merges the last two biconnected components saved in \p stack.
	//! Embeds them iff #m_embeddingGrade != EmbeddingGrade::doNotEmbed.
	void mergeBiconnectedComponent(ArrayBuffer<int>& stack);

	//! Links (and embeds iff #m_embeddingGrade != EmbeddingGrade::doNotEmbed) backedges from node
	//! \p v with direction \p v_dir to node \p w with direction \p w_dir.
	void embedBackedges(const node v, const int v_dir, const node w, const int w_dir);

	//! Creates a short circuit edge from node \p v with direction \p v_dir to node \p w with direction \p w_dir
	void createShortCircuitEdge(const node v, const int v_dir,
								const node w, const int w_dir);

	//! Walkup: Builds the pertinent subgraph for the backedge \p back.
	/** \p back is the backedge between nodes \p v and \p w. \p v is the current node to embed.
	 * All visited nodes are marked with value \p marker. The Function returns the last traversed node.
	 */
	node walkup(const node v, const node w, const int marker, const edge back);

	//! Walkdown: Embeds all backedges with DFI \p i as targetnode to node \p v
	/**
	 * @param i is the DFI of the current vertex to embed
	 * @param v is the virtual node being the root of the bicomp attached to \p i
	 * @param findKuratowskis collects information in order to extract Kuratowski Subdivisions later
	 * @return 1, iff the embedding process found a stopping configuration
	 */
	int walkdown(const int i, const node v, FindKuratowskis* findKuratowskis);

	//! Merges unprocessed virtual nodes such as the dfs-roots with their real counterpart
	void mergeUnprocessedNodes();

	//! Postprocessing of the graph, so that all virtual vertices are embedded and all bicomps are flipped
	/** In addition, embedding steps for parallel edges and self-loops are implemented.
	 */
	void postProcessEmbedding();

	//! Starts the embedding phase, which embeds #m_g node by node in descending DFI-order.
	/** Returns true, if graph is planar, false otherwise.
	 */
	bool embed();

	//! @}

	//! Input graph, which can be altered
	Graph& m_g;

	//! \name Some parameters... see BoyerMyrvold for further options
	//! @{
	const bool m_bundles;
	const int m_embeddingGrade;
	const bool m_limitStructures;
	const double m_randomness;
	const bool m_avoidE2Minors;
	const EdgeArray<int> *m_edgeCosts;
	std::minstd_rand m_rand;
	//! @}

	//! Flag for extracting a planar subgraph instead of testing for planarity
	bool m_extractSubgraph = true;

	//! The whole number of bicomps, which have to be flipped
	int m_flippedNodes;

	//! \name Members from BoyerMyrvoldInit
	//! @{

	//! Link to non-virtual vertex of a virtual Vertex.
	/** A virtual vertex has negative DFI of the DFS-Child of related non-virtual Vertex
	 */
	NodeArray<node> m_realVertex;

	//! The one and only DFI-NodeArray
	NodeArray<int> m_dfi;

	//! Returns appropriate node from given DFI
	Array<node> m_nodeFromDFI;

	//! Links to opposite adjacency entries on external face in clockwise resp. ccw order
	/** m_link[0]=CCW, m_link[1]=CW
	 */
	NodeArray<adjEntry> m_link[2];

	//! Links for short circuit edges.
	/** If short circuit edges are introduced, the former adjEntries to the neighbors
	 * have to be saved here for embedding and merging purposes. If there is no
	 * short circuit edge, this adjEntry is nullptr.
	 */
	NodeArray<adjEntry> m_beforeSCE[2];

	//! The adjEntry which goes from DFS-parent to current vertex
	NodeArray<adjEntry> m_adjParent;

	//! The DFI of the least ancestor node over all backedges
	/** If no backedge exists, the least ancestor is the DFI of that node itself
	 */
	NodeArray<int> m_leastAncestor;

	//! Contains the type of each edge
	EdgeArray<BoyerMyrvoldEdgeType> m_edgeType;

	//! The lowpoint of each node
	NodeArray<int> m_lowPoint;

	//! The highest DFI in a subtree with node as root
	NodeArray<int> m_highestSubtreeDFI;

	//! A list to all separated DFS-children of node
	/** The list is sorted by lowpoint values (in linear time)
	*/
	NodeArray<ListPure<node> > m_separatedDFSChildList;

	//! Pointer to node contained in the DFSChildList of his parent, if exists.
	/** If node isn't in list or list doesn't exist, the pointer is set to nullptr.
	*/
	NodeArray<ListIterator<node> > m_pNodeInParent;

	//! @}
	//! \name Members for Walkup and Walkdown
	//! @{

	//! This Array keeps track of all vertices that are visited by current walkup
	NodeArray<int> m_visited;

	//! Identifies the rootnode of the child bicomp the given backedge points to
	EdgeArray<node> m_pointsToRoot;

	/**
	 * Stores for each (real) non-root vertex v with which backedge it was
	 * visited during the walkup. This is done to later identify the root vertex
	 * of the bicomp v belongs to.
	 */
	NodeArray<edge> m_visitedWithBackedge;

	/**
	 * Stores for each (virtual) bicomp root how many backedges to its bicomp
	 * still have to be embedded. The value is set during the walkup, and it is
	 * used and decreased while embedding backedges during the walkdown.
	 */
	NodeArray<int> m_numUnembeddedBackedgesInBicomp;

	//! Iff true, the node is the root of a bicomp which has to be flipped.
	/** The DFS-child of every bicomp root vertex is unique. if a bicomp
	 * is flipped, this DFS-child is marked to check whether the bicomp
	 * has to be flipped or not.
	 */
	NodeArray<bool> m_flipped;

	//! Holds information, if node is the source of a backedge.
	/** This information refers to the adjEntries on the targetnode
	 * and is used during the walkdown
	 */
	NodeArray<SListPure<adjEntry> > m_backedgeFlags;

	//! List of virtual bicomp roots, that are pertinent to the current embedded node
	NodeArray<SListPure<node> > m_pertinentRoots;

	//! Data structure for the Kuratowski subdivisions, which will be returned
	SListPure<KuratowskiStructure>& m_output;

	//! @}
};

inline bool operator > (int lhs, BoyerMyrvoldPlanar::EmbeddingGrade rhs) {
	return lhs > static_cast<int>(rhs);
}

inline bool operator == (int lhs, BoyerMyrvoldPlanar::EmbeddingGrade rhs) {
	return lhs == static_cast<int>(rhs);
}

inline bool operator != (int lhs, BoyerMyrvoldPlanar::EmbeddingGrade rhs) {
	return lhs != static_cast<int>(rhs);
}

inline bool operator <= (int lhs, BoyerMyrvoldPlanar::EmbeddingGrade rhs) {
	return lhs <= static_cast<int>(rhs);
}

}
