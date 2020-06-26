/** \file
 * \brief MMM is a Multilevel Graph drawing Algorithm that can use different modules.
 *
 * \author Gereon Bartel
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



#include <ogdf/energybased/multilevel_mixer/ModularMultilevelMixer.h>
#include <ogdf/energybased/multilevel_mixer/SolarMerger.h>
#include <ogdf/energybased/multilevel_mixer/BarycenterPlacer.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include <ogdf/energybased/SpringEmbedderGridVariant.h>

#ifdef OGDF_MMM_LEVEL_OUTPUTS
#include <sstream>
#endif


namespace ogdf {

ModularMultilevelMixer::ModularMultilevelMixer()
{
	// options
	m_times              = 1;
	m_fixedEdgeLength    = -1.0f;
	m_fixedNodeSize      = -1.0f;
	m_coarseningRatio    = 1.0;
	m_levelBound         = false;
	m_randomize          = false;

	// module options
	setMultilevelBuilder(new SolarMerger);
	setInitialPlacer    (new BarycenterPlacer);
	setLevelLayoutModule(new SpringEmbedderGridVariant);
}


void ModularMultilevelMixer::call(GraphAttributes &GA)
{   //ensure consistent behaviour of the two call Methods
	MultilevelGraph MLG(GA);
	call(MLG);
	MLG.exportAttributes(GA);
}


void ModularMultilevelMixer::call(MultilevelGraph &MLG)
{
	const Graph &G = MLG.getGraph();

	m_errorCode = erc::None;
	clock_t time = clock();
	if ((!m_multilevelBuilder || !m_initialPlacement) && !m_oneLevelLayoutModule) {
		OGDF_THROW(AlgorithmFailureException);
	}

	if (m_fixedEdgeLength > 0.0) {
		for(edge e : G.edges) {
			MLG.weight(e, m_fixedEdgeLength);
		}
	}

	if (m_fixedNodeSize > 0.0) {
		for(node v : G.nodes) {
			MLG.radius(v, m_fixedNodeSize);
		}
	}

	if (m_multilevelBuilder && m_initialPlacement)
	{
		double lbound = 16.0 * log(double(G.numberOfNodes()))/log(2.0);
		m_multilevelBuilder->buildAllLevels(MLG);

		//Part for experiments: Stop if number of levels too high
#ifdef OGDF_MMM_LEVEL_OUTPUTS
		int nlevels = m_multilevelBuilder->getNumLevels();
#endif
		if (m_levelBound
		 && m_multilevelBuilder->getNumLevels() > lbound) {
			m_errorCode = erc::LevelBound;
			return;
		}
		if (m_randomize)
		{
			for(node v : G.nodes) {
				MLG.x(v, (float)randomDouble(-1.0, 1.0));
				MLG.y(v, (float)randomDouble(-1.0, 1.0));
			}
		}

		while(MLG.getLevel() > 0)
		{
			if (m_oneLevelLayoutModule) {
				for(int i = 1; i <= m_times; i++) {
					m_oneLevelLayoutModule->call(MLG.getGraphAttributes());
				}
			}

#ifdef OGDF_MMM_LEVEL_OUTPUTS
			//Debugging output
			std::stringstream ss;
			ss << nlevels--;
			string s;
			ss >> s;
			s = "LevelLayout" + s;
			string fs(s);
			fs += ".gml";
			MLG.writeGML(fs.c_str());
#endif

			MLG.moveToZero();

			int nNodes = G.numberOfNodes();
			m_initialPlacement->placeOneLevel(MLG);
			m_coarseningRatio = double(G.numberOfNodes()) / nNodes;

#ifdef OGDF_MMM_LEVEL_OUTPUTS
			//debug only
			s = s + "_placed.gml";
			MLG.writeGML(s.c_str());
#endif
		}
	}

	//Final level

	if(m_finalLayoutModule ||  m_oneLevelLayoutModule)
	{
		LayoutModule &lastLayoutModule = *(m_finalLayoutModule ? m_finalLayoutModule : m_oneLevelLayoutModule);

		for(int i = 1; i <= m_times; i++) {
			lastLayoutModule.call(MLG.getGraphAttributes());
		}
	}

	time = clock() - time;
}

}
