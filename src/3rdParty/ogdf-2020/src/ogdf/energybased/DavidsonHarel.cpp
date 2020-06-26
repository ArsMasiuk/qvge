/** \file
 * \brief  Iimplementation of class DavidsonHarel
 *
 * This class realizes the Davidson Harel Algorithm for
 * automtatic graph drawing. It minimizes the energy
 * of the drawing using simulated annealing. This file
 * contains the main simulated annealing algorithm and
 * the fnction for computing the next candidate layout
 * that should be considered.
 *
 * \author
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

//TODO: in addition to the layout size, node sizes should be used in
//the initial radius computation in case of "all central" layouts with
//huge nodes
//the combinations for parameters should be checked: its only useful
//to have a slow shrinking if you have enough time to shrink down to
//small radius

namespace ogdf {

const int DavidsonHarel::m_defaultTemp = 1000;
const double DavidsonHarel::m_defaultRadius = 100.0;
const int DavidsonHarel::m_iterationMultiplier = 25;  //best//30;ori
const double DavidsonHarel::m_coolingFactor = 0.80;  //0.75;ori
const double DavidsonHarel::m_shrinkFactor = 0.8;

//initializes internal data and the random number generator
DavidsonHarel::DavidsonHarel():
m_temperature(m_defaultTemp),
m_shrinkingFactor(m_shrinkFactor),
m_diskRadius(m_defaultRadius),
m_energy(0.0),
m_numberOfIterations(0)
{
	srand((unsigned)time(nullptr));
}


//allow resetting in between subsequent calls
void DavidsonHarel::initParameters()
{
	m_diskRadius = m_defaultRadius;
	m_energy = 0.0;
	//m_numberOfIterations = 0; //is set in member function
	m_shrinkingFactor = m_shrinkFactor;

}


void DavidsonHarel::setStartTemperature(int startTemp)
{
	OGDF_ASSERT(startTemp >= 0);
	m_temperature=startTemp;
}

void DavidsonHarel::setNumberOfIterations(int steps)
{
	OGDF_ASSERT(steps >= 0);
	m_numberOfIterations = steps;
}

//whenever an energy function is added, the initial energy of the new function
//is computed and added to the initial energy of the layout
void DavidsonHarel::addEnergyFunction(EnergyFunction *F, double weight)
{
	m_energyFunctions.pushBack(F);
	OGDF_ASSERT(weight >= 0);
	m_weightsOfEnergyFunctions.pushBack(weight);
	F->computeEnergy();
	m_energy += F->energy();
}

List<string> DavidsonHarel::returnEnergyFunctionNames()
{
	List<string> names;
	for (EnergyFunction *f : m_energyFunctions)
		names.pushBack(f->getName());
	return names;
}

List<double> DavidsonHarel::returnEnergyFunctionWeights()
{
	List<double> weights;
	for(double x : m_weightsOfEnergyFunctions)
		weights.pushBack(x);
	return weights;
}

//newVal is the energy value of a candidate layout. It is accepted if it is lower
//than the previous energy of the layout or if m_fineTune is not tpFine and
//the difference to the old energy divided by the temperature is smaller than a
//random number between zero and one
bool DavidsonHarel::testEnergyValue(double newVal)
{
	bool accepted = true;
	if(newVal > m_energy) {
		accepted = false;

		double testval = exp((m_energy-newVal)/ m_temperature);
		double compareVal = randNum(); // number between 0 and 1

		if(compareVal < testval)
			accepted = true;

	}
	return accepted;
}

//divides number returned by rand by RAND_MAX to get number between zero and one
inline double DavidsonHarel::randNum() const
{
	double val = rand();
	val /= RAND_MAX;
	return val;
}

//chooses random vertex and a new random position for it on a circle with radius m_diskRadius
//around its previous position
node DavidsonHarel::computeCandidateLayout(
const GraphAttributes &AG,
DPoint &newPos) const
{
	int randomPos = randomNumber(0,m_nonIsolatedNodes.size()-1);
	node v = *(m_nonIsolatedNodes.get(randomPos));
	double oldx = AG.x(v);
	double oldy = AG.y(v);
	double randomAngle = randNum() * 2.0 * Math::pi;
	newPos.m_y = oldy+sin(randomAngle)*m_diskRadius;
	newPos.m_x = oldx+cos(randomAngle)*m_diskRadius;
#ifdef OGDF_DEBUG
	double dist = sqrt((newPos.m_x - oldx)*(newPos.m_x - oldx)+(newPos.m_y-oldy)*(newPos.m_y-oldy));
	OGDF_ASSERT(dist > 0.99 * m_diskRadius);
	OGDF_ASSERT(dist < 1.01 * m_diskRadius);
#endif
	return v;
}

//chooses the initial radius of the disk as half the maximum of width and height of
//the initial layout or depending on the value of m_fineTune
void DavidsonHarel::computeFirstRadius(const GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	node vFirst = G.firstNode();
	double minX = AG.x(vFirst);
	double minY = AG.y(vFirst);
	double maxX = minX;
	double maxY = minY;
	for(node v : G.nodes) {
		Math::updateMin(minX, AG.x(v));
		Math::updateMax(maxX, AG.x(v));
		Math::updateMin(minY, AG.y(v));
		Math::updateMax(maxY, AG.y(v));
	}
	// compute bounding box of current layout
	// make values nonzero
	double w = maxX-minX+1.0;
	double h = maxY-minY+1.0;

	double ratio = h/w;

	double W = sqrt(G.numberOfNodes() / ratio);

	m_diskRadius = W / 5.0;//allow to move by a significant part of current layout size
	Math::updateMax(m_diskRadius, max(maxX-minX, maxY-minY) / 5.0);

	//TODO: also use node sizes
#if 0
	double lengthSum(0.0);
	for(node v : m_G.nodes) {
		const DRectIntersection &i = shape(v);
		lengthSum += i.width();
		lengthSum += i.width();
		}
		lengthSum /= (2*m_G.numberOfNodes());
		// lengthSum is now the average of all lengths and widths
#endif

	//change the initial radius depending on the settings
	//this is legacy crap
#if 0
	double divo = 2.0;
	if (m_fineTune == tpCoarse) {
		m_diskRadius = 1000.0;
		divo = 0.5;
	}
	if (m_fineTune == tpFine) {
		m_diskRadius = 10.0;
		divo = 15.0;
	}
#if 0
	Math::updateMax(m_diskRadius, max(maxX-minX,maxY-minY));
#endif
	m_diskRadius = max(maxX-minX,maxY-minY);
	m_diskRadius /= divo;
#endif
}

//steps through all energy functions and adds the initial energy computed by each
//function for the start layout
void DavidsonHarel::computeInitialEnergy()
{
	OGDF_ASSERT(!m_energyFunctions.empty());
	ListIterator<EnergyFunction*> it;
	ListIterator<double> it2;
	it2 = m_weightsOfEnergyFunctions.begin();
	for(it = m_energyFunctions.begin(); it.valid() && it2.valid(); it=it.succ(), it2 = it2.succ())
		m_energy += (*it)->energy() * (*it2);
}

//the vertices with degree zero are placed below all other vertices on a horizontal
// line centered with repect to the rest of the drawing
void DavidsonHarel::placeIsolatedNodes(GraphAttributes &AG) const {
	double minX = 0.0;
	double minY = 0.0;
	double maxX = 0.0;

	if (!m_nonIsolatedNodes.empty()) {
		//compute a rectangle that includes all non-isolated vertices
		node vFirst = m_nonIsolatedNodes.front();
		minX = AG.x(vFirst);
		minY = AG.y(vFirst);
		maxX = minX;
		double maxY = minY;
		for (node v : m_nonIsolatedNodes) {
			double xVal = AG.x(v);
			double yVal = AG.y(v);
			double halfHeight = AG.height(v) / 2.0;
			double halfWidth = AG.width(v) / 2.0;
			if (xVal - halfWidth < minX) minX = xVal - halfWidth;
			if (xVal + halfWidth > maxX) maxX = xVal + halfWidth;
			if (yVal - halfHeight < minY) minY = yVal - halfHeight;
			if (yVal + halfHeight > maxY) maxY = yVal + halfHeight;
		}
	}

	// compute the width and height of the largest isolated node
	List<node> isolated;
	const Graph &G = AG.constGraph();
	double maxWidth = 0;
	double maxHeight = 0;
	for (node v : G.nodes)
	if (v->degree() == 0) {
		isolated.pushBack(v);
		if (AG.height(v) > maxHeight) maxHeight = AG.height(v);
		if (AG.width(v) > maxWidth) maxWidth = AG.width(v);
	}
	// The nodes are placed on a line in the middle under the non isolated vertices.
	// Each node gets a box sized 2 maxWidth.
	double boxWidth = 2.0*maxWidth;
	double commonYCoord = minY - (1.5*maxHeight);
	double XCenterOfDrawing = minX + ((maxX - minX) / 2.0);
	double startXCoord = XCenterOfDrawing - 0.5*(isolated.size()*boxWidth);
	double xcoord = startXCoord;
	for (node v : isolated) {
		AG.x(v) = xcoord;
		AG.y(v) = commonYCoord;
		xcoord += boxWidth;
	}
}

//this is the main optimization routine with the loop that lowers the temperature
//and the disk radius geometrically until the temperature is zero. For each
//temperature, a certain number of new positions for a random vertex are tried
void DavidsonHarel::call(GraphAttributes &AG)
{
	initParameters();

	m_shrinkingFactor = m_shrinkFactor;

	OGDF_ASSERT(!m_energyFunctions.empty());

	const Graph &G = AG.constGraph();
	//compute the list of vertices with degree greater than zero
	G.allNodes(m_nonIsolatedNodes);
	ListIterator<node> it,itSucc;
	for(it = m_nonIsolatedNodes.begin(); it.valid(); it = itSucc) {
		itSucc = it.succ();
		if((*it)->degree() == 0) m_nonIsolatedNodes.del(it);
	}
	if(G.numberOfEdges() > 0) { //else only isolated nodes
		computeFirstRadius(AG);
		computeInitialEnergy();
		if(m_numberOfIterations == 0)
			m_numberOfIterations = m_nonIsolatedNodes.size() * m_iterationMultiplier;
		//this is the main optimization loop
		while(m_temperature > 0) {
			//iteration loop for each temperature
			for(int ic = 1; ic <= m_numberOfIterations; ic ++) {
				DPoint newPos;
				//choose random vertex and new position for vertex
				node v = computeCandidateLayout(AG,newPos);
				//compute candidate energy and decide if new layout is chosen
				ListIterator<double> it2 = m_weightsOfEnergyFunctions.begin();
				double newEnergy = 0.0;
				for(EnergyFunction *f : m_energyFunctions) {
					newEnergy += f->computeCandidateEnergy(v,newPos) * (*it2);
					++it2;
				}
				OGDF_ASSERT(newEnergy >= 0.0);
				//this tests if the new layout is accepted. If this is the case,
				//all energy functions are informed that the new layout is accepted
				if(testEnergyValue(newEnergy)) {
					for(EnergyFunction *f : m_energyFunctions)
						f->candidateTaken();
					AG.x(v) = newPos.m_x;
					AG.y(v) = newPos.m_y;
					m_energy = newEnergy;
				}
			}
			//lower the temperature and decrease the disk radius
			m_temperature = (int)floor(m_temperature*m_coolingFactor);
			m_diskRadius *= m_shrinkingFactor;
		}
	}
	//if there are zero degree vertices, they are placed using placeIsolatedNodes
	if(m_nonIsolatedNodes.size() != G.numberOfNodes())
		placeIsolatedNodes(AG);
}

}
