/** \file
 * \brief Declares ogdf::CliqueFinderSPQR class.
 *
 * \author Max Ilsen
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

#include <ogdf/clique/CliqueFinderModule.h>

namespace ogdf {

//! Finds cliques using SPQR trees.
/**
 * @ingroup ga-induced
 *
 * The class CliqueFinderSPQR can be called on a graph to retrieve (disjoint)
 * cliques. It searches for cliques in a graph by first dividing it into its
 * triconnected components using an SPQR tree and then using a given clique
 * finder in each R-node.
 *
 * @tparam CF The type of the clique finder to use on R-nodes.
 */
class OGDF_EXPORT CliqueFinderSPQR : public CliqueFinderModule {

public:
	/**
	 * Creates a new CliqueFinderSPQR.
	 * @param cliqueFinder The clique finder to use on R-nodes.
	 * During doCall(), its min size parameter is set to the min size of this.
	 */
	explicit CliqueFinderSPQR(CliqueFinderModule &cliqueFinder)
		: CliqueFinderModule()
		, m_cliqueFinder(cliqueFinder)
	{ }

protected:
	//! @copydoc CliqueFinderModule::doCall
	void doCall() override;

private:
	CliqueFinderModule &m_cliqueFinder; //!< The clique finder to use on R-nodes.
};

}
