/*!\file
 * \author Frank Baumann
 * \brief the osi master.
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/lib/abacus/lpmaster.h>

namespace abacus {


//! The OSI LP master.
/**
 * An instance of the class LpMasterOsi is used to store all
 * OSI specific parameters and global data. The master instance of
 * Master keeps a pointer to an instance of this class.
 */
class OGDF_EXPORT LpMasterOsi : public LpMaster {
	friend class OsiIF;
public:

	//! The constructor.
	/**
	 * \param master The master of the optimization.
	 */
	LpMasterOsi(Master *master) : LpMaster(master) { }

	//! The destructor.
	virtual ~LpMasterOsi();

	//! Initializes the LP solver specific Parameters.
	virtual void initializeLpParameters() override;

	//! Sets default values of the LP solver specific Parameters.
	virtual void setDefaultLpParameters() override { }

	//! Prints the settings of the LP solver specific Parameters.
	virtual void printLpParameters() const override { }

	//! Prints LP solver specific Statistics.
	virtual void outputLpStatistics() const override { }
};

}
