/** \file
 * \brief Implementation for DavidsonHarelLayout
 *
 * This is the frontend for the DavidsonHarel optimization
 * function. It adds the energy functions to the problem and
 * sets their weights. It also contains functions for setting
 * and returning the parameters that influence the quality of
 * the solution and the speed of the optimization process.
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

#include <ogdf/energybased/DavidsonHarel.h>
#include <ogdf/energybased/DavidsonHarelLayout.h>
#include <ogdf/energybased/davidson_harel/Repulsion.h>
#include <ogdf/energybased/davidson_harel/Attraction.h>
#include <ogdf/energybased/davidson_harel/Overlap.h>
#include <ogdf/energybased/davidson_harel/Planarity.h>
#include <ogdf/energybased/davidson_harel/PlanarityGrid.h>

#define DEFAULT_REPULSION_WEIGHT 1e6
#define DEFAULT_ATTRACTION_WEIGHT 1e2
#define DEFAULT_OVERLAP_WEIGHT 100
#define DEFAULT_PLANARITY_WEIGHT 500
#define DEFAULT_ITERATIONS 0
#define DEFAULT_START_TEMPERATURE 500

namespace ogdf {

struct InputValueInvalid : Exception { InputValueInvalid() : Exception(__FILE__, __LINE__) { } };

struct WeightLessThanZeroException : InputValueInvalid { };

struct IterationsNonPositive : InputValueInvalid { };

struct TemperatureNonPositive : InputValueInvalid { };


DavidsonHarelLayout::DavidsonHarelLayout()
{
	m_repulsionWeight = DEFAULT_REPULSION_WEIGHT;
	m_attractionWeight = DEFAULT_ATTRACTION_WEIGHT;
	m_nodeOverlapWeight = DEFAULT_OVERLAP_WEIGHT;
	m_planarityWeight = DEFAULT_PLANARITY_WEIGHT;
	m_numberOfIterations = DEFAULT_ITERATIONS;
	m_itAsFactor = false;
	m_startTemperature = DEFAULT_START_TEMPERATURE;
	m_speed    = SpeedParameter::Medium;
	m_multiplier = 2.0;
	m_prefEdgeLength = 0.0;
	m_crossings = false;
}


void DavidsonHarelLayout::fixSettings(SettingsParameter sp)
{
	double r, a, p, o;
	switch (sp) {
	case SettingsParameter::Standard:
		r = 900; a = 250; o = 1450; p = 300; m_crossings = false;
		break;
	case SettingsParameter::Repulse:
		r = 9000; a = 250; o = 1450; p = 300; m_crossings = false;
		break;
	case SettingsParameter::Planar:
		r = 900; a = 250; o = 1450; p = 3000; m_crossings = true;
		break;
	default:
		OGDF_THROW_PARAM(AlgorithmFailureException, AlgorithmFailureCode::IllegalParameter);
	}
	setRepulsionWeight(r);
	setAttractionWeight(a);
	setNodeOverlapWeight(o);
	setPlanarityWeight(p);
}

void DavidsonHarelLayout::setSpeed(SpeedParameter sp)
{
	m_speed = sp;
	m_numberOfIterations = 0;
}

void DavidsonHarelLayout::setRepulsionWeight(double w)
{
	if(w < 0) throw WeightLessThanZeroException();
	else m_repulsionWeight = w;
}


void DavidsonHarelLayout::setAttractionWeight(double w)
{
	if(w < 0) throw WeightLessThanZeroException();
	else m_attractionWeight = w;
}


void DavidsonHarelLayout::setNodeOverlapWeight(double w)
{
	if(w < 0) throw WeightLessThanZeroException();
	else m_nodeOverlapWeight = w;
}


void DavidsonHarelLayout::setPlanarityWeight(double w)
{
	if(w < 0) throw WeightLessThanZeroException();
	else m_planarityWeight = w;
}


void DavidsonHarelLayout::setNumberOfIterations(int w)
{
	if(w < 0) throw IterationsNonPositive();
	else m_numberOfIterations = w;
}


void DavidsonHarelLayout::setStartTemperature (int w)
{
	if(w < 0) throw TemperatureNonPositive();
	else m_startTemperature = w;
}


//this sets the parameters of the class DavidsonHarel, adds the energy functions and
//starts the optimization process
void DavidsonHarelLayout::call(GraphAttributes &AG)
{
	// all edges straight-line
	AG.clearAllBends();

	DavidsonHarel dh;
	davidson_harel::Repulsion rep(AG);
	davidson_harel::Attraction atr(AG);
	davidson_harel::Overlap over(AG);
	davidson_harel::Planarity plan(AG);
#if 0
	PlanarityGrid plan(AG);
	PlanarityGrid2 plan(AG);
	NodeIntersection ni(AG);
#endif

	// Either use a fixed value...
	if (OGDF_GEOM_ET.greater(m_prefEdgeLength, 0.0))
	{
		atr.setPreferredEdgelength(m_prefEdgeLength);
	}
	// ...or set it depending on vertex sizes
	else atr.reinitializeEdgeLength(m_multiplier);


	dh.addEnergyFunction(&rep,m_repulsionWeight);
	dh.addEnergyFunction(&atr,m_attractionWeight);
	dh.addEnergyFunction(&over,m_nodeOverlapWeight);
	if (m_crossings) dh.addEnergyFunction(&plan,m_planarityWeight);
#if 0
	dh.addEnergyFunction(&ni,2000.0);

	dh.setNumberOfIterations(m_numberOfIterations);
	dh.setStartTemperature(m_startTemperature);
#endif
	const Graph& G = AG.constGraph();
	//TODO: Number of iterations always depending on size
	if (m_numberOfIterations == 0)
	{
		switch (m_speed)  //todo: function setSpeedParameters
		{
		case SpeedParameter::Fast:
			m_numberOfIterations = max(75, 3*G.numberOfNodes());
			m_startTemperature = 400;
			break;
		case SpeedParameter::Medium:
			m_numberOfIterations = 10*G.numberOfNodes();
			m_startTemperature = 1500;
			break;
		case SpeedParameter::HQ:
			m_numberOfIterations = 2500*G.numberOfNodes(); //should be: isolate
			m_startTemperature = 2000;
			break;
		default:
			OGDF_THROW_PARAM(AlgorithmFailureException, AlgorithmFailureCode::IllegalParameter);
		}
	} else {
		if (m_itAsFactor)
			dh.setNumberOfIterations(200+m_numberOfIterations*G.numberOfNodes());
		else
			dh.setNumberOfIterations(m_numberOfIterations);
	}
	dh.setStartTemperature(m_startTemperature);
	dh.call(AG);
}

}
