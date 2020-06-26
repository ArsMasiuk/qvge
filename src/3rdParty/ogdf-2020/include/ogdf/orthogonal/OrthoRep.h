/** \file
 * \brief Declaration of orthogonal representation of planar graphs.
 *
 * \author Carsten Gutwenger, Karsten Klein
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

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/tuples.h>


namespace ogdf {

class OGDF_EXPORT PlanRep;
class OGDF_EXPORT PlanRepUML;


// type for bends (convex or reflex)
enum class OrthoBendType : char { convexBend = '0', reflexBend = '1' };

// type of (orthogonal) directions
// horizontal: East or West
// vertical:   North or South
enum class OrthoDir {
	North     = 0,
	East      = 1,
	South     = 2,
	West      = 3,
	Undefined = 4
};

// Option bits for orthogonal layouts, UML alignment, compaction scaling, progressive shape computation
enum class UMLOpt {OpAlign = 0x0001, OpScale = 0x0002, OpProg = 0x0004};

inline int operator | (int lhs, UMLOpt rhs) { return lhs | static_cast<int>(rhs); }
inline int operator ~ (UMLOpt rhs) { return ~static_cast<int>(rhs); }
inline int operator & (int lhs, UMLOpt rhs) { return lhs & static_cast<int>(rhs); }
inline int operator += (int &lhs, UMLOpt rhs) { lhs += static_cast<int>(rhs); return lhs; }

//! Represents the bends on an edge e consisting of vertical and horizontal segments
class OGDF_EXPORT BendString
{
public:
	// constructs empty bend string
	BendString() {
		m_pBend = nullptr;
		m_len = 0;
	}

	// constructs bend string as given by str
	// Precond.: str is a 0 terminated C++ string consisting of '0's and '1's
	explicit BendString(const char *str) {
		init(str);
	}

	// constructs bend string consisting of n c's
	// Precond.: c is '0' or '1'
	BendString(char c, size_t n) {
		init(c,n);
	}

	// copy constructor
	BendString(const BendString &bs) {
		init(bs);
	}

	// copy constructor (move semantics)
	BendString(BendString &&bs) : m_pBend(bs.m_pBend), m_len(bs.m_len) {
		bs.m_pBend = nullptr;
		bs.m_len = 0;
	}


	// destructor
	~BendString() {
		delete[] m_pBend;
	}


	// returns i'th character in bend string
	char operator[](size_t i) const {
		// range check
		OGDF_ASSERT(i < m_len);
		return m_pBend[i];
	}

	char &operator[](size_t i) {
		// range check
		OGDF_ASSERT(i < m_len);
		return m_pBend[i];
	}

	// returns number of characters in bend string
	size_t size() const {
		return m_len;
	}

	// returns a pointer to the 0 terminated string
	// or a 0-pointer if empty
	const char *toString() const {
		return m_pBend;
	}

	// sets bend string to the string given by str
	// Precond.: str is a 0 terminated C++ string consisting of '0's and '1's
	void set(const char *str) {
		delete[] m_pBend;
		init(str);
	}

	// sets bend string to the string consisting of n c's
	// Precond.: c is '0' or '1'
	void set(char c, size_t n) {
		delete[] m_pBend;
		init(c,n);
	}
	void set(OrthoBendType obt, size_t n) {
		delete[] m_pBend;
		init(static_cast<int>(obt),n);
	}


	// sets bend string to the empty bend string
	void set() {
		delete[] m_pBend;
		m_pBend = nullptr;
		m_len = 0;
	}


	// assignment operator
	BendString &operator=(const BendString &bs) {
		delete[] m_pBend;
		init(bs);
		return *this;
	}

	// assignment operator (move semantics)
	BendString &operator=(BendString &&bs) {
		if (&bs != this) {
			delete[] m_pBend;
			m_pBend = bs.m_pBend;
			m_len = bs.m_len;
			bs.m_pBend = nullptr;
			bs.m_len = 0;
		}
		return *this;
	}

	BendString &operator+=(const char *str) {
		return this->operator+=(BendString(str));
	}

	BendString &operator+=(const BendString &bs) {
		char* temp = new char[m_len+bs.m_len+1];

		m_len = m_len + bs.m_len;

		if (m_len == 0)
		{
			*temp = 0;//temp = 0;
		}
		else
		{
			char *p = temp;
			if (m_pBend != nullptr)
			{
				const char *str = m_pBend;
				while ((*p++ = *str++) != 0) ;
			}
			else {*p = '0'; p++;}
			if (bs.m_len > 0)
			{
				p--;
				const char *str1 = bs.m_pBend;
				while ((*p++ = *str1++) != 0) ;
			}
		}

		delete[] m_pBend;
		m_pBend = temp;

		return *this;
	}

	// output operator
	// example output: "001101001" or ""
	friend std::ostream &operator<<(std::ostream &os, const BendString &bs) {
		if (bs.size() == 0)
			os << "\"\"";
		else
			os << "\"" << bs.m_pBend << "\"";
		return os;
	}

private:
	void init(const char *str);
	void init(char c, size_t n);
	void init(const BendString &bs);


	// poiner to 0 terminated character string
	char *m_pBend;
	// length of string (number of characters without ending 0)
	size_t m_len;
};

//! Orthogonal representation of an embedded graph
class OGDF_EXPORT OrthoRep
{
public:
	//! Information about a side of a vertex in UML diagrams
	struct SideInfoUML {
		// adjacency entry of generalization attached at the side
		// (or 0 if none)
		adjEntry m_adjGen;
		// number of attached edges which have corresponding edges in the
		// original graph to the left (index 0) or right of the
		// attached generalization. If no generalization is attached,
		// m_nAttached[0] is the total number of attached edges.
		int m_nAttached[2];

		// constructor
		SideInfoUML() {
			m_adjGen = nullptr;
			m_nAttached[0] = m_nAttached[1] = 0;
		}

		// returns the total number of edges attached at this side
		int totalAttached() const {
			int nGen = (m_adjGen == nullptr) ? 0 : 1;
			return nGen + m_nAttached[0] + m_nAttached[1];
		}


		// output operator for debugging
		friend std::ostream &operator<<(std::ostream &os, const SideInfoUML &si)
		{
			os << "{ " << si.m_nAttached[0] <<
				", " << si.m_adjGen <<
				", " << si.m_nAttached[1] << " }";
			return os;
		}
	};

	//only for debugging purposes
	adjEntry externalAdjEntry() const {return m_adjExternal;}
	adjEntry alignAdjEntry() const {return m_adjAlign;}

	//! Further information about the cages of vertices in UML diagrams
	struct VertexInfoUML {
		// side information (North, East, South, West corresponds to
		// left, top, right, bottom)
		SideInfoUML m_side[4];
		// m_corner[dir] is adjacency entry in direction dir starting at
		// a corner
		adjEntry m_corner[4];

		// constructor
		VertexInfoUML() {
#ifdef OGDF_DEBUG
			m_corner[0] = m_corner[1] = m_corner[2] = m_corner[3] = nullptr;
#endif
		}
		OGDF_NEW_DELETE
	};


	// construction

	// dummy
	OrthoRep() { m_pE = nullptr; }
	// associates orthogonal representation with embedding E
	explicit OrthoRep(CombinatorialEmbedding &E);

	// destruction
	~OrthoRep() {
		freeCageInfoUML();
	}

	// reinitialization: associates orthogonal representation with embedding E
	void init(CombinatorialEmbedding &E);


	//
	// access functions
	//

	// returns associated embedding
	operator const CombinatorialEmbedding &() const {
		return *m_pE;
	}

	// returns associated graph
	operator const Graph &() const {
		return *m_pE;
	}

	// returns angle between adj and its successor
	// (return value is angle divided by 90 degree)
	int angle(adjEntry adj) const {
		return m_angle[adj];
	}

	int &angle(adjEntry adj) {
		return m_angle[adj];
	}


	// returns bend string of adjacency entry adj
	const BendString &bend(adjEntry adj) const {
		return m_bends[adj];
	}

	BendString &bend(adjEntry adj) {
		return m_bends[adj];
	}

	// returns direction of adjacency entry
	OrthoDir direction(adjEntry adj) const {
		return m_dir[adj];
	}


	// returns cage info
	const VertexInfoUML *cageInfo(node v) const {
		return m_umlCageInfo[v];
	}

	// returns cage info
	VertexInfoUML *cageInfo(node v) {
		return m_umlCageInfo[v];
	}



	//
	// update functions
	//

	// normalizes the orthogonal representation, i.e., sets an artficial
	// vertex on each bend
	void normalize();

	// checks if the orth. repr. is normalized, i.e., each bend string is empty
	bool isNormalized() const;

	// removes rectangular ears (pattern "100") by splitting edges and faces.
	// Afterwards, each internal face is a rectangle and the external face
	// contains no rectangular ear (but is not necessarily the complement of
	// a rectangle).
	// Precond.: The orth. repr. is normalized and contains no 0-degree angles
	void dissect();
	// same as dissect, attempting to save artificial nodes and allow preprocessing
	void dissect2(PlanRep* PG = nullptr);
	// variant for use with simple PlanRep
	void gridDissect(PlanRep* PG);
	// undoes a previous dissect() by removing dissection edges and unsplitting
	// vertices
	void undissect(bool align = false);


	// assigns consistent directions to adjacency entries
	void orientate();

	// assigns consistent directions to adj. entries such that most
	// generalizations are directed in preferedDir
	void orientate(const PlanRep &PG, OrthoDir preferedDir);

	// assigns consistent directions to adjacency entries,
	// assigning dir to adj (fixing all others)
	void orientate(adjEntry adj, OrthoDir dir);

	// returns true iff orientate() has been called before.
	// If not, direction() may not be called since adj. entry array is not
	// initialized!
	bool isOrientated() const {
		return m_dir.valid();
	}


	// rotates all directions of adjacency entries by r
	void rotate(int r);


	// computes further information about cages
	void computeCageInfoUML(const PlanRep &PG/*, double sep*/);


	// checks if the orth. repr. is a legal shape description, i.e., if there
	// is an orthogonal embedding realizing this shape (if 0-degree angles are
	// present, the condition is necessary but not sufficient).
	// If false is returned, error contains a description of the reason.
	bool check(string &error) const;


	//
	// static members
	//

	// exchanges '1'->'0' and vice versa
	static char flip(char c) {
		return (c == '0') ? '1' : '0';
	}

	//! Returns the opposite OrthoDir
	static OrthoDir oppDir(OrthoDir d) {
		return static_cast<OrthoDir>((static_cast<int>(d) + 2) & 3);
	}

	//! Returns the next OrthoDir (in a clockwise manner)
	static OrthoDir nextDir(OrthoDir d) {
		return static_cast<OrthoDir>((static_cast<int>(d) + 1) & 3);
	}

	//! Returns the previous OrthoDir (in a clockwise manner)
	static OrthoDir prevDir(OrthoDir d) {
		return static_cast<OrthoDir>((static_cast<int>(d) + 3) & 3);
	}

	friend std::ostream &operator<<(std::ostream &os, const OrthoRep &op) {
		const Graph& E = op;
		for(edge e : E.edges)
		{
			os << e <<": src angle "<<op.angle(e->adjSource())<<" bend "<<op.bend(e->adjSource())
				<<"\n"<<" tgt angle "<<op.angle(e->adjTarget())<<" bend "<<op.bend(e->adjTarget())

				<<"\n";
		}
		return os;
	}



private:
	void orientateFace(adjEntry adj, OrthoDir dir);
	void freeCageInfoUML();

	// associated combinatorial embedding
	CombinatorialEmbedding *m_pE;

	// * 90 deg = angle between e and its successor
	AdjEntryArray<int> m_angle;
	// bends on edge e
	AdjEntryArray<BendString> m_bends;
	// direction of adjacency entries
	AdjEntryArray<OrthoDir> m_dir;

	// information about cages of original vertices; used for orthogonal
	// representations of PlanRep's
	NodeArray<VertexInfoUML *> m_umlCageInfo;

	// The following members are used for undoing dissection
	EdgeArray<bool> m_dissectionEdge; // = true iff dissection edge
	//check if special gener. sons alignment edge
	EdgeArray<bool> m_alignmentEdge; // = true iff alignment edge
	// contains all nodes created by splitting non-dissection edges while
	// dissect()
	ArrayBuffer<node> m_splitNodes;
	// stores adjacency entry on external face for restoring in undissect()
	adjEntry m_adjExternal;
	// stores adjacency entry on preliminary external face in alignment case
	adjEntry m_adjAlign;
	//starts dissection phase for special pattern 1 replacement before standard dissection
	bool m_preprocess;
	//special pattern after pattern 1
	bool m_pattern2;
};

}
