/** \file
 * \brief Definition of a Triple used in contraction-based approximation
 *        algorithm for the minimum Steiner tree problem
 *
 * \author Matthias Woste
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

#include <ogdf/basic/Graph.h>

namespace ogdf {
namespace steiner_tree {

/*!
 * \brief This class represents a triple used by various contraction-based minimum Steiner tree approximations.
 */
template<typename T>
class Triple {
public:
	explicit Triple(const node s0 = nullptr, const node s1 = nullptr, const node s2 = nullptr, const node z = nullptr, T cost = 0, double win = 0)
	  : m_s0(s0)
	  , m_s1(s1)
	  , m_s2(s2)
	  , m_z(z)
	  , m_cost(cost)
	  , m_win(win)
	{
	}

	node s0() const
	{
		return m_s0;
	}
	node s1() const
	{
		return m_s1;
	}
	node s2() const
	{
		return m_s2;
	}
	node z() const
	{
		return m_z;
	}
	T cost() const
	{
		return m_cost;
	}
	double win() const
	{
		return m_win;
	}

	void s0(node u)
	{
		m_s0 = u;
	}
	void s1(node u)
	{
		m_s1 = u;
	}
	void s2(node u)
	{
		m_s2 = u;
	}
	void z(node u)
	{
		m_z = u;
	}
	void cost(T c)
	{
		m_cost = c;
	}
	void win(double w)
	{
		m_win = w;
	}

private:
	node m_s0, m_s1, m_s2; //!< terminal nodes
	node m_z; //!< center node of the triple
	T m_cost; //!< edge costs of the triple in the original graph
	double m_win; //!< the win of the triple at some point of time (used as cache)
};

}
}
