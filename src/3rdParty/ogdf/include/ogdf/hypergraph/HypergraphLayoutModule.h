/** \file
 * \brief Declaration of an interface for hypergraph layout
 *        algorithms. Any hypergraph layout must follow this prescription.
 *
 * \author Ondrej Moris
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

#include <ogdf/hypergraph/HypergraphAttributes.h>

namespace ogdf {

/**
 * \brief Interface of hypergraph layout algorithms.
 *
 */
class OGDF_EXPORT HypergraphLayoutModule
{
public:

	//! Initializes a layout module.
	HypergraphLayoutModule()
	{
	}

	virtual ~HypergraphLayoutModule()
	{
	}

	/**
	 * \brief Computes a layout of hypergraph given by \p HA.
	 *
	 * This method is the actual algorithm call and must be implemented by
	 * derived classes.
	 * @param HA is the input hypergraph attributes class.
	 */
	virtual void call(HypergraphAttributes &HA) = 0;

	/**
	 * \brief Computes a layout of a hypergraph given by \p HA.
	 *
	 * @param HA is the input hypergraph attributes class.
	 */
	void operator()(HypergraphAttributes &HA)
	{
		call(HA);
	}

	OGDF_MALLOC_NEW_DELETE;
};

}
