/** \file
 * \brief Declares class DavidsonHarel which implements the
 * Davidson-Harel approach for drawing graphs.
 *
 * \author Rene Weiskircher
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

#include <ogdf/energybased/davidson_harel/EnergyFunction.h>

namespace ogdf {

//! The Davidson-Harel approach for drawing graphs.
class OGDF_EXPORT DavidsonHarel
{
	using EnergyFunction = davidson_harel::EnergyFunction;
public:

	//! Creates an instance of Davidsen-Harel base class.
	DavidsonHarel();

	~DavidsonHarel() { }

	//! Sets the start temperature to \p startTemp.
	void setStartTemperature(int startTemp);

	//! Sets the number of iterations for each temperature step to \p steps.
	void setNumberOfIterations(int steps);

	//! Adds an energy function \p F with a certain weight.
	void addEnergyFunction(EnergyFunction *F, double weight);

	//! Returns a list of the names of the energy functions.
	List<string> returnEnergyFunctionNames();

	//! Returns a list of the weights of the energy functions.
	List<double> returnEnergyFunctionWeights();

	//! Calls the Davidson-Harel method for graph \p GA.
	void call(GraphAttributes &GA);

private:
	//! The default starting temperature.
	const static int m_defaultTemp;
	//! The default starting radius.
	const static double m_defaultRadius;
	//! Per default, the number of iterations per temperature are set as a constant multiple of the number of vertices.
	const static int m_iterationMultiplier;
	//! The fraction by which the temperature is lowered after a temperature step is finished.
	const static double m_coolingFactor;
	//! the constant by which the radius of the circle around each vertex is shrunk when the temperature is lowered
	const static double m_shrinkFactor;

	int m_temperature;          //!< The temperature during the annealing process.
	double m_shrinkingFactor;   //!< The factor for radius.
	double m_diskRadius;        //!< The radius of the disk around the old position of a vertex where the new position will be.
	double m_energy;            //!< The current energy of the system.
	int m_numberOfIterations;   //!< The number of iterations per temperature step.

	List<EnergyFunction*> m_energyFunctions; //!< The list of the energy functions.
	List<double> m_weightsOfEnergyFunctions; //!< The list of the weights for the energy functions.

	List<node> m_nonIsolatedNodes; //!< The list of nodes with degree greater 0.

	//! Resets the parameters for subsequent runs.
	void initParameters();

	//! Randomly computes a node and a new position for that node.
	node computeCandidateLayout(const GraphAttributes &, DPoint &) const;

	//! Tests if new energy value satisfies annealing property (only better if m_fineTune).
	bool testEnergyValue(double newVal);

	//! Computes a random number between zero and one
	double randNum() const;

	//! Computes the first disk radius as the half the diamter of the enclosing rectangle.
	void computeFirstRadius(const GraphAttributes &AG);

	//! Computes the energy of the initial layout and stores it in #m_energy.
	void computeInitialEnergy();

	//! Computes positions for the vertices of degree zero.
	void placeIsolatedNodes(GraphAttributes &AG) const;

	//! Fake assignment operator (dummy to avoid copying)
	DavidsonHarel& operator=(const DavidsonHarel &dh);
	//! Fake copy constructor (dummy to avoid copying)
	DavidsonHarel(const DavidsonHarel &) { }
};

}
