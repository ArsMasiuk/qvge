/** \file
 * \brief Declaration of the class ExtractKuratowskis
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

#include <ogdf/planarity/boyer_myrvold/BoyerMyrvoldPlanar.h>
#include <ogdf/planarity/boyer_myrvold/FindKuratowskis.h>

namespace ogdf {

//! Wrapper-class for Kuratowski Subdivisions containing the minortype and edgelist
class OGDF_EXPORT KuratowskiWrapper {
public:
	//! Constructor
	KuratowskiWrapper() { }

	//! Returns true, iff subdivision is a K3,3-minor
	inline bool isK33() const { return subdivisionType != SubdivisionType::E5; }
	//! Returns true, iff subdivision is a K5-minor
	inline bool isK5() const { return subdivisionType == SubdivisionType::E5; }

	//! Possible minortypes of a Kuratowski Subdivision
	enum class SubdivisionType {
		A=0,
		AB=1,
		AC=2,
		AD=3,
		AE1=4,
		AE2=5,
		AE3=6,
		AE4=7,
		B=8,
		C=9,
		D=10,
		E1=11,
		E2=12,
		E3=13,
		E4=14,
		E5=15
	};
	//! Minortype of the Kuratowski Subdivision
	SubdivisionType subdivisionType;

	//! The node which was embedded while the Kuratowski Subdivision was found
	node V;

	//! Contains the edges of the Kuratowski Subdivision
	SListPure<edge> edgeList;
};

OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const KuratowskiWrapper::SubdivisionType &obj);

//! Extracts multiple Kuratowski Subdivisions
/**
 * @ingroup ga-planarity
 *
 * \pre Graph has to be simple.
 */
class ExtractKuratowskis {
public:
	//! Constructor
	explicit ExtractKuratowskis(BoyerMyrvoldPlanar& bm);
	//! Destructor
	~ExtractKuratowskis() { }

	//! Extracts all Kuratowski Subdivisions and adds them to \p output (without bundles)
	void extract(
			const SListPure<KuratowskiStructure>& allKuratowskis,
			SList<KuratowskiWrapper>& output);

	//! Extracts all Kuratowski Subdivisions and adds them to \p output (with bundles)
	void extractBundles(
			const SListPure<KuratowskiStructure>& allKuratowskis,
			SList<KuratowskiWrapper>& output);

	//! Enumeration over Kuratowski Type none, K33, K5
	enum class KuratowskiType {
		none	= 0, //!< no kuratowski subdivision exists
		K33		= 1, //!< a K3,3 subdivision exists
		K5		= 2  //!< a K5 subdivision exists
	};

	//! Checks, if \p list forms a valid Kuratowski Subdivision and returns the type
	/**
	 * @return Returns the following value:
	 *           - none = no Kuratowski
	 *           - K33 = the K3,3
	 *           - K5 = the K5
	 */
	static KuratowskiType whichKuratowski(
			const Graph& m_g,
			const NodeArray<int>& dfi,
			const SListPure<edge>& list);

	//! Checks, if edges in Array \p edgenumber form a valid Kuratowski Subdivision and returns the type
	/**
	 * \pre The numer of edges has to be 1 for used edges, otherwise 0.
	 * @return Returns the following value:
	 *           - none = no Kuratowski
	 *           - K33 = the K3,3
	 *           - K5 = the K5
	 */
	static KuratowskiType whichKuratowskiArray(
			const Graph& g,
#if 0
			const NodeArray<int>& m_dfi,
#endif
			EdgeArray<int>& edgenumber);

	//! Returns true, iff the Kuratowski is not already contained in output
	static bool isANewKuratowski(
			const Graph& g,
			const SListPure<edge>& kuratowski,
			const SList<KuratowskiWrapper>& output);
	//! Returns true, iff the Kuratowski is not already contained in output
	/** \pre Kuratowski Edges are all edges != 0 in the Array.
	 */
	static bool isANewKuratowski(
#if 0
			const Graph& g,
#endif
			const EdgeArray<int>& test,
			const SList<KuratowskiWrapper>& output);

	// avoid automatic creation of assignment operator
	//! Assignment operator is undefined!
	ExtractKuratowskis &operator=(const ExtractKuratowskis &);

protected:
	//! Link to class BoyerMyrvoldPlanar
	BoyerMyrvoldPlanar& BMP;

	//! Input graph
	const Graph& m_g;

	//! Some parameters, see BoyerMyrvold for further instructions
	int m_embeddingGrade;
	//! Some parameters, see BoyerMyrvold for further instructions
	const bool m_avoidE2Minors;

	//! Value used as marker for visited nodes etc.
	/** Used during Backtracking and the extraction of some specific minortypes
	 */
	int m_nodeMarker;
	//! Array maintaining visited bits on each node
	NodeArray<int> m_wasHere;

	//! The one and only DFI-NodeArray
	const NodeArray<int>& m_dfi;

	//! Returns appropriate node from given DFI
	const Array<node>& m_nodeFromDFI;

	//! The adjEntry which goes from DFS-parent to current vertex
	const NodeArray<adjEntry>& m_adjParent;

	//! Adds external face edges to \p list
	inline void addExternalFacePath(
						SListPure<edge>& list,
						const SListPure<adjEntry>& externPath) {
		SListConstIterator<adjEntry> itExtern;
		for (itExtern = externPath.begin(); itExtern.valid(); ++itExtern) {
			list.pushBack((*itExtern)->theEdge());
		}
	}

	//! Returns ::adjEntry of the edge between node \p high and a special node
	/** The special node is that node with the lowest DFI not less than the DFI of \p low.
	 */
	inline adjEntry adjToLowestNodeBelow(node high, int low);

	//! Adds DFS-path from node \p bottom to node \p top to \p list
	/** \pre Each virtual node has to be merged.
	 */
	inline void addDFSPath(SListPure<edge>& list, node bottom, node top);
	//! Adds DFS-path from node \p top to node \p bottom to \p list
	/** \pre Each virtual node has to be merged.
	 */
	inline void addDFSPathReverse(SListPure<edge>& list, node bottom, node top);

	//! Separates \p list1 from edges already contained in \p list2
	inline void truncateEdgelist(SListPure<edge>& list1, const SListPure<edge>& list2);

	//! Extracts minortype A and adds it to list \p output
	void extractMinorA(
			SList<KuratowskiWrapper>& output,
			const KuratowskiStructure& k,
#if 0
			const WInfo& info,
#endif
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Extracts minortype B and adds it to list \p output (no bundles)
	void extractMinorB(
			SList<KuratowskiWrapper>& output,
			//NodeArray<int>& nodeflags,
			//const int nodemarker,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Extracts minortype B and adds it to list \p output (with bundles)
	void extractMinorBBundles(
			SList<KuratowskiWrapper>& output,
			NodeArray<int>& nodeflags,
			const int nodemarker,
			const KuratowskiStructure& k,
			EdgeArray<int>& flags,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Extracts minortype C and adds it to list \p output
	void extractMinorC(
			SList<KuratowskiWrapper>& output,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Extracts minortype D and adds it to list \p output
	void extractMinorD(
			SList<KuratowskiWrapper>& output,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Extracts minortype E and adds it to list \p output (no bundles)
	void extractMinorE(
			SList<KuratowskiWrapper>& output,
			bool firstXPath,
			bool firstPath,
			bool firstWPath,
			bool firstWOnHighestXY,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Extracts minortype E and adds it to list \p output (bundles)
	void extractMinorEBundles(
			SList<KuratowskiWrapper>& output,
			bool firstXPath,
			bool firstPath,
			bool firstWPath,
			bool firstWOnHighestXY,
			NodeArray<int>& nodeflags,
			const int nodemarker,
			const KuratowskiStructure& k,
			EdgeArray<int>& flags,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW);
	//! Checks for minortype E1
	inline bool isMinorE1(
			int before,
			bool firstXPath,
			bool firstYPath) const
	{
		return (before == -1 && firstXPath)
		    || (before == 1 && firstYPath);
	}
	//! Extracts minorsubtype E1 and adds it to list \p output
	void extractMinorE1(
			SList<KuratowskiWrapper>& output,
			int before,
#if 0
			const node z,
#endif
			const node px,
			const node py,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW,
			const SListPure<edge>& pathZ,
			const node endnodeZ);
	//! Checks for minortype E2 preconditions
	inline bool checkMinorE2(
			bool firstWPath,
			bool firstWOnHighestXY) const
	{
		return !m_avoidE2Minors
		    && firstWPath
		    && firstWOnHighestXY;
	}
	//! Checks for minortype E2
	inline bool isMinorE2(
			const node endnodeX,
			const node endnodeY,
			const node endnodeZ) const
	{
		return m_dfi[endnodeZ] > m_dfi[endnodeX]
		    && m_dfi[endnodeZ] > m_dfi[endnodeY];
	}
	//! Extracts minorsubtype E2 and adds it to list \p output
	void extractMinorE2(
			SList<KuratowskiWrapper>& output,
#if 0
			int before,
			const node z,
			const node px,
			const node py,
#endif
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
#if 0
			const SListPure<edge>& pathW,
#endif
			const SListPure<edge>& pathZ
#if 0
			, const node endnodeZ
#endif
			);
	//! Checks for minortype E3
	inline bool isMinorE3(
			const node endnodeX,
			const node endnodeY,
			const node endnodeZ) const
	{
		return endnodeX != endnodeY
		    && (m_dfi[endnodeX] > m_dfi[endnodeZ]
		     || m_dfi[endnodeY] > m_dfi[endnodeZ]);
	}
	//! Extracts minorsubtype E3 and adds it to list \p output
	void extractMinorE3(
			SList<KuratowskiWrapper>& output,
			int before,
			const node z,
			const node px,
			const node py,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW,
			const SListPure<edge>& pathZ,
			const node endnodeZ);
	//! Checks for minortype E4
	inline bool isMinorE4(
			const node px,
			const node py,
			const KuratowskiStructure& k,
			const WInfo& info) const
	{
		return (px != k.stopX && !info.pxAboveStopX)
		    || (py != k.stopY && !info.pyAboveStopY);
	}
	//! Extracts minorsubtype E4 and adds it to list \p output
	void extractMinorE4(
			SList<KuratowskiWrapper>& output,
			int before,
			const node z,
			const node px,
			const node py,
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW,
			const SListPure<edge>& pathZ,
			const node endnodeZ);
	//! Checks for minortype E5 (K5)
	inline bool isMinorE5(
			const node px,
			const node py,
			const KuratowskiStructure& k,
			const node endnodeX,
			const node endnodeY,
			const node endnodeZ) const
	{
		return px == k.stopX
		    && py == k.stopY
		    && k.V == k.RReal
		    && ((endnodeX == endnodeY
		      && m_dfi[endnodeZ] <= m_dfi[endnodeX])
		     || (endnodeX == endnodeZ
		      && m_dfi[endnodeY] <= m_dfi[endnodeX])
		     || (endnodeY == endnodeZ
		      && m_dfi[endnodeX] <= m_dfi[endnodeY]));
	}
	//! Extracts minorsubtype E5 and adds it to list \p output
	void extractMinorE5(
			SList<KuratowskiWrapper>& output,
#if 0
			int before,
			const node z,
			const node px,
			const node py,
#endif
			const KuratowskiStructure& k,
			const WInfo& info,
			const SListPure<edge>& pathX,
			const node endnodeX,
			const SListPure<edge>& pathY,
			const node endnodeY,
			const SListPure<edge>& pathW,
			const SListPure<edge>& pathZ,
			const node endnodeZ);
};

}
