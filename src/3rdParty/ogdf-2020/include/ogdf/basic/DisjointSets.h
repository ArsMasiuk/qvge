/** \file
 * \brief Implementation of disjoint sets data structures (union-find functionality).
 *
 * \author Andrej Dudenhefner
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

#include <cstring>
#include <ogdf/basic/exceptions.h>

namespace ogdf {

#define OGDF_DISJOINT_SETS_INTERMEDIATE_PARENT_CHECK

//! Defines options for linking two sets.
enum class LinkOptions {
	Naive = 0, //!< Naive Link
	Index = 1, //!< Link by index (default)
	Size = 2,  //!< Link by size
	Rank = 3   //!< Link by rank
};

//! Defines options for compression search paths.
enum class CompressionOptions {
	PathCompression = 0, //!< Path Compression
	PathSplitting = 1, //!< Path Splitting (default)
	PathHalving = 2, //!< Path Halving
	Type1Reversal = 4, //!< Reversal of type 1
	Collapsing = 5, //!< Collapsing
	Disabled = 6 //!< No Compression
};

//! Defines options for interleaving find/link operations in quickUnion.
enum class InterleavingOptions {
	Disabled = 0, //!< No Interleaving (default)
	Rem = 1, //!< Rem's Algorithm (only compatible with LinkOptions::Index)
	Tarjan = 2, //!< Tarjan's and van Leeuwen's Algorithm (only compatible with LinkOptions::Rank)
	Type0Reversal = 3, //!< Interleaved Reversal of Type 0 (only compatible with LinkOptions::Naive)
	SplittingCompression = 4 //!< Interleaved Path Splitting Path Compression (only compatible with LinkOptions::Index)
};

namespace disjoint_sets {

struct AnyOption {};
template<LinkOptions linkOption> struct LinkOption : AnyOption {};
template<CompressionOptions compressionOption> struct CompressionOption : AnyOption {};
template<InterleavingOptions interleavingOption> struct InterleavingOption : AnyOption {};

}

//! A Union/Find data structure for maintaining disjoint sets.
template <LinkOptions linkOption = LinkOptions::Index,
          CompressionOptions compressionOption = CompressionOptions::PathSplitting,
          InterleavingOptions interleavingOption = InterleavingOptions::Disabled>
class DisjointSets
{
static_assert(interleavingOption != InterleavingOptions::Rem || linkOption == LinkOptions::Index, "Rem's Algorithm requires linking by index.");
static_assert(interleavingOption != InterleavingOptions::Tarjan || linkOption == LinkOptions::Rank, "Tarjan and van Leeuwen's Algorithm requires linking by rank.");
static_assert(interleavingOption != InterleavingOptions::Type0Reversal || linkOption == LinkOptions::Naive, "Interleaved Reversal Type 0 requires naïve linking.");
static_assert(interleavingOption != InterleavingOptions::SplittingCompression || linkOption == LinkOptions::Index, "Interleaved Path Splitting Path Compression requires linking by index.");
private:
	int m_numberOfSets; //!< Current number of disjoint sets.
	int m_numberOfElements; //!< Current number of elements.
	int m_maxNumberOfElements; //!< Maximum number of elements (array size) adjusted dynamically.

	// Arrays parents, elements, parameters, siblings map a set id to its properties.

	int *m_parents; //!< Maps set id to parent set id.
	int *m_parameters; //!< Maps set id to rank/size.
	int *m_siblings; //!< Maps set id to sibling set id.

	//find
	int find(disjoint_sets::CompressionOption<CompressionOptions::PathCompression>,int set);
	int find(disjoint_sets::CompressionOption<CompressionOptions::PathSplitting>,int set);
	int find(disjoint_sets::CompressionOption<CompressionOptions::PathHalving>,int set);
	int find(disjoint_sets::CompressionOption<CompressionOptions::Type1Reversal>,int set);
	int find(disjoint_sets::CompressionOption<CompressionOptions::Collapsing>,int set);
	int find(disjoint_sets::CompressionOption<CompressionOptions::Disabled>,int set);

	//link
	int link(disjoint_sets::LinkOption<LinkOptions::Naive>,int set1,int set2);
	int link(disjoint_sets::LinkOption<LinkOptions::Index>,int set1,int set2);
	int link(disjoint_sets::LinkOption<LinkOptions::Size>,int set1,int set2);
	int link(disjoint_sets::LinkOption<LinkOptions::Rank>,int set1,int set2);

	//quickUnion
	bool quickUnion(disjoint_sets::LinkOption<LinkOptions::Index>,disjoint_sets::InterleavingOption<InterleavingOptions::Rem>,int set1,int set2);
	bool quickUnion(disjoint_sets::LinkOption<LinkOptions::Index>,disjoint_sets::InterleavingOption<InterleavingOptions::SplittingCompression>,int set1,int set2);
	bool quickUnion(disjoint_sets::LinkOption<LinkOptions::Rank>,disjoint_sets::InterleavingOption<InterleavingOptions::Tarjan>,int set1,int set2);
	bool quickUnion(disjoint_sets::AnyOption,disjoint_sets::InterleavingOption<InterleavingOptions::Disabled>,int set1,int set2);
	bool quickUnion(disjoint_sets::LinkOption<LinkOptions::Naive>,disjoint_sets::InterleavingOption<InterleavingOptions::Type0Reversal>,int set1,int set2);

public:
	//! Creates an empty DisjointSets structure.
	/**
	* \param maxNumberOfElements Expected number of Elements.
	*/
	explicit DisjointSets(int maxNumberOfElements = (1<<15) )
	{
		this->m_numberOfSets=0;
		this->m_numberOfElements=0;
		this->m_maxNumberOfElements = maxNumberOfElements;
		this->m_parents = new int[this->m_maxNumberOfElements];
		this->m_parameters = (linkOption==LinkOptions::Rank || linkOption==LinkOptions::Size) ? new int[this->m_maxNumberOfElements] : nullptr;
		this->m_siblings = (compressionOption==CompressionOptions::Collapsing) ? new int[this->m_maxNumberOfElements] : nullptr;
	}

	DisjointSets(const DisjointSets&) = delete;

	DisjointSets& operator=(const DisjointSets&) = delete;

	~DisjointSets()
	{
		delete[] this->m_parents;
		delete[] this->m_parameters;
		delete[] this->m_siblings;
	}

	//! Returns the id of the largest superset of \p set and compresses the path according to ::CompressionOptions.
	/**
	* \param set Set.
	* \return Superset id
	* \pre \p set is a non negative properly initialized id.
	*/
	int find(int set)
	{
		OGDF_ASSERT(set >= 0);
		OGDF_ASSERT(set < m_numberOfElements);
		return find(disjoint_sets::CompressionOption<compressionOption>(), set);
	}

	//! Returns the id of the largest superset of \p set.
	/**
	* \param set Set.
	* \return Superset id
	* \pre \p set is a non negative properly initialized id.
	*/
	int getRepresentative(int set) const
	{
		OGDF_ASSERT(set >= 0);
		OGDF_ASSERT(set < m_numberOfElements);
		while (set!=m_parents[set]) set=m_parents[set];
		return set;
	}

	//! Initializes a singleton set.
	/**
	* \return Set id of the initialized singleton set.
	*/
	int makeSet()
	{
		if (this->m_numberOfElements==this->m_maxNumberOfElements)
		{
			int *parents = this->m_parents;
			this->m_parents = new int[this->m_maxNumberOfElements * 2];
			memcpy(this->m_parents,parents,sizeof(int)*this->m_maxNumberOfElements);
			delete[] parents;

			if (this->m_parameters != nullptr)
			{
				int *parameters = this->m_parameters;
				this->m_parameters = new int[this->m_maxNumberOfElements*2];
				memcpy(this->m_parameters,parameters,sizeof(int)*this->m_maxNumberOfElements);
				delete[] parameters;
			}

			if (this->m_siblings != nullptr)
			{
				int *siblings = this->m_siblings;
				this->m_siblings = new int[this->m_maxNumberOfElements*2];
				memcpy(this->m_siblings,siblings,sizeof(int)*this->m_maxNumberOfElements);
				delete[] siblings;
			}
			this->m_maxNumberOfElements*=2;
		}
		this->m_numberOfSets++;
		int id = this->m_numberOfElements++;
		this->m_parents[id]=id;
		//Initialize size/ rank/ sibling.
		if (linkOption == LinkOptions::Size) this->m_parameters[id]=1;
		else if (linkOption == LinkOptions::Rank) this->m_parameters[id]=0;
		if (compressionOption == CompressionOptions::Collapsing) this->m_siblings[id] = -1;
		return id;
	}

	//! Unions \p set1 and \p set2.
	/**
	* \pre \p set1 and \p set2 are maximal disjoint sets.
	* \return Set id of the union.
	*/
	int link(int set1, int set2)
	{
		OGDF_ASSERT(set1 == getRepresentative(set1));
		OGDF_ASSERT(set2 == getRepresentative(set2));
		if (set1==set2) return -1;
		this->m_numberOfSets--;
		return linkPure(set1, set2);
	}

	//! Unions the maximal disjoint sets containing \p set1 and \p set2.
	/**
	* \return True, if the maximal sets containing \p set1 and \p set2 were disjoint und joined correctly. False otherwise.
	*/
	bool quickUnion(int set1, int set2)
	{
		if (set1==set2) return false;
		bool result = quickUnion(disjoint_sets::LinkOption<linkOption>(),disjoint_sets::InterleavingOption<interleavingOption>(), set1, set2);
		m_numberOfSets -= result;
		return result;
	}

	//! Returns the current number of disjoint sets.
	int getNumberOfSets() { return m_numberOfSets; }

	//! Returns the current number of elements.
	int getNumberOfElements() {return m_numberOfElements; }

private:
	//! Unions \p set1 and \p set2 w/o decreasing the \a numberOfSets
	/**
	 * \pre \p set1 and \p set2 are maximal disjoint sets.
	 * \return Set id of the union
	 */
	int linkPure(int set1, int set2)
	{
		int superset = link(disjoint_sets::LinkOption<linkOption>(), set1, set2);
		//Collapse subset tree.
		if (compressionOption == CompressionOptions::Collapsing)
		{
			int subset = set1 == superset ? set2 : set1;
			int id = subset;
			while (this->m_siblings[id] != -1)
			{
				id = this->m_siblings[id];
				this->m_parents[id]=superset;
			}
			this->m_siblings[id] = this->m_siblings[superset];
			this->m_siblings[superset] = subset;
		}
		return superset;
	}
};


//find
template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::find(disjoint_sets::CompressionOption<CompressionOptions::PathCompression>,int set)
{
	int parent = m_parents[set];
	if (set==parent)
	{
		return set;
	}
	else
	{
		parent = find(disjoint_sets::CompressionOption<CompressionOptions::PathCompression>(),parent);
		m_parents[set]=parent;
		return parent;
	}
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::find(disjoint_sets::CompressionOption<CompressionOptions::PathHalving>,int set)
{
	while (set!=m_parents[set])
	{
		int parent = m_parents[set];
		int grandParent = m_parents[parent];
		m_parents[set]=grandParent;
		set = grandParent;
	}
	return set;
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::find(disjoint_sets::CompressionOption<CompressionOptions::PathSplitting>,int set)
{
	int parent = m_parents[set];
	int grandParent = m_parents[parent];
	while (parent!=grandParent)
	{
		m_parents[set]=grandParent;
		set = parent;
		parent = grandParent;
		grandParent = m_parents[grandParent];
	}
	return parent;
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::find(disjoint_sets::CompressionOption<CompressionOptions::Type1Reversal>,int set)
{
	int root = set;
	set = m_parents[root];

	while (set!=m_parents[set])
	{
		int parent = m_parents[set];
		m_parents[set] = root;
		set = parent;
	}
	m_parents[root] = set;
	return set;
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::find(disjoint_sets::CompressionOption<CompressionOptions::Disabled>,int set)
{
	while (set!=m_parents[set]) set=m_parents[set];
	return set;
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::find(disjoint_sets::CompressionOption<CompressionOptions::Collapsing>,int set)
{
	return m_parents[set];
}


//quickUnion
template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<linkOption,compressionOption,interleavingOption>::
     quickUnion(disjoint_sets::AnyOption,
                disjoint_sets::InterleavingOption<InterleavingOptions::Disabled>,
                int set1, int set2)
{
#ifdef OGDF_DISJOINT_SETS_INTERMEDIATE_PARENT_CHECK
	if (m_parents[set1]==m_parents[set2]) return false;
#endif
	set1 = find(set1);
	set2 = find(set2);
	if (set1 != set2)
	{
		linkPure(set1,set2);
		return true;
	}
	return false;
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<linkOption,compressionOption,interleavingOption>::
     quickUnion(disjoint_sets::LinkOption<LinkOptions::Naive>,
                disjoint_sets::InterleavingOption<InterleavingOptions::Type0Reversal>,
                int set1, int set2)
{
#ifdef OGDF_DISJOINT_SETS_INTERMEDIATE_PARENT_CHECK
	if (m_parents[set1]==m_parents[set2]) return false;
#endif
	int root = set2;
	int set = set2;
	int parent = m_parents[set];
	m_parents[set]=root;
	while (set != parent)
	{
		if (parent == set1)
		{
			m_parents[root]=set1;
			return false;
		}
		set = parent;
		parent = m_parents[set];
		m_parents[set]=root;
	}

	set = set1;
	parent = m_parents[set];
	while (true)
	{
		if (parent == root) return false;
		m_parents[set] = root;
		if (parent == set){
			return true;
		}
		set = parent;
		parent = m_parents[set];
	}
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<linkOption,compressionOption,interleavingOption>::
     quickUnion(disjoint_sets::LinkOption<LinkOptions::Index>,
                disjoint_sets::InterleavingOption<InterleavingOptions::Rem>,
                int set1, int set2)
{
	int r_x = set1; int r_y = set2;
	int p_r_x =m_parents[r_x];
	int p_r_y =m_parents[r_y];
	while (p_r_x != p_r_y)
	{
		if (p_r_x < p_r_y)
		{
			if (r_x == p_r_x)
			{
				m_parents[r_x]=p_r_y;
				return true;
			}
			m_parents[r_x]=p_r_y;
			r_x = p_r_x;
			p_r_x = m_parents[r_x];
		}
		else
		{
			if (r_y == p_r_y)
			{
				m_parents[r_y]=p_r_x;
				return true;
			}
			m_parents[r_y]=p_r_x;
			r_y = p_r_y;
			p_r_y = m_parents[r_y];
		}
	}
	return false;
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<linkOption,compressionOption,interleavingOption>::
     quickUnion(disjoint_sets::LinkOption<LinkOptions::Index>,
                disjoint_sets::InterleavingOption<InterleavingOptions::SplittingCompression>,
                int set1, int set2)
{
#ifdef OGDF_DISJOINT_SETS_INTERMEDIATE_PARENT_CHECK
	if (m_parents[set1]==m_parents[set2]) return false;
#endif
	int set = set1;

	if (set1 < set2)
	{
		set = set2;
		set2 = set1;
		set1 = set;
	}

	//!Use path splitting to compress the path of set1 and get the root
	set = m_parents[set];
	int parent = m_parents[set];
	int grandParent = m_parents[parent];
	while (parent!=grandParent)
	{
		m_parents[set]=grandParent;
		set = parent;
		parent = grandParent;
		grandParent = m_parents[grandParent];
	}
	m_parents[set1]=parent;
	int root = parent;

	//!Redirect all nodes with smaller indices on the path of set2 to the root
	set = set2;
	parent = m_parents[set];
	while (true)
	{
		if (parent < root)
		{
			m_parents[set]=root;
			if (set == parent){
				return true;
			}
			set=parent;
			parent = m_parents[set];
		}
		else if (parent > root)
		{
			m_parents[root]=parent;
			m_parents[set1]=parent;
			m_parents[set2]=parent;
			return true;
		}
		else return false;
	}
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<linkOption,compressionOption,interleavingOption>::
     quickUnion(disjoint_sets::LinkOption<LinkOptions::Rank>,
                disjoint_sets::InterleavingOption<InterleavingOptions::Tarjan>,
                int set1, int set2)
{
	int r_x = set1; int r_y = set2;
	int p_r_x = m_parents[r_x];
	int p_r_y = m_parents[r_y];
	while (p_r_x != p_r_y)
	{
		if (m_parameters[p_r_x]<=m_parameters[p_r_y])
		{
			if (r_x==p_r_x)
			{
				if (m_parameters[p_r_x] == m_parameters[p_r_y]
				 && p_r_y == m_parents[p_r_y])
				{
					m_parameters[p_r_y]++;
				}
				m_parents[r_x]=m_parents[p_r_y];
				return true;
			}
			m_parents[r_x]=p_r_y;
			r_x = p_r_x;
			p_r_x = m_parents[r_x];
		}
		else
		{
			if (r_y==p_r_y)
			{
				m_parents[r_y]=m_parents[p_r_x];
				return true;
			}
			m_parents[r_y]=p_r_x;
			r_y = p_r_y;
			p_r_y = m_parents[r_y];
		}
	}
	return false;
}


//link
template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::link(disjoint_sets::LinkOption<LinkOptions::Index>,int set1,int set2)
{
	if (set1<set2)
	{
		m_parents[set1]=set2;
		return set2;
	}
	else
	{
		m_parents[set2]=set1;
		return set1;
	}
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::link(disjoint_sets::LinkOption<LinkOptions::Rank>,int set1,int set2)
{
	int parameter1 = m_parameters[set1];
	int parameter2 = m_parameters[set2];

	if (parameter1<parameter2)
	{
		m_parents[set1]=set2;
		return set2;
	}
	else if (parameter1>parameter2)
	{
		m_parents[set2]=set1;
		return set1;
	}
	else
	{
		m_parents[set1]=set2;
		m_parameters[set2]++;
		return set2;
	}
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::link(disjoint_sets::LinkOption<LinkOptions::Size>,int set1,int set2)
{
	int parameter1 = m_parameters[set1];
	int parameter2 = m_parameters[set2];

	if (parameter1<parameter2)
	{
		m_parents[set1]=set2;
		m_parameters[set2]+=parameter1;
		return set2;
	}
	else
	{
		m_parents[set2]=set1;
		m_parameters[set1]+=parameter2;
		return set1;
	}
}

template <LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<linkOption,compressionOption,interleavingOption>::link(disjoint_sets::LinkOption<LinkOptions::Naive>,int set1,int set2)
{
	m_parents[set1]=set2;
	return set2;
}

}
