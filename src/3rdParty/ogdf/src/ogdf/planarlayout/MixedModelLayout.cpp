/** \file
 * \brief Implementation of Mixed-Model layout algorithm.
 *
 * \author Carsten Gutwenger
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


#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/augmentation/PlanarAugmentation.h>
#include <ogdf/augmentation/PlanarAugmentationFix.h>
#include <ogdf/planarlayout/BiconnectedShellingOrder.h>
#include <ogdf/planarity/SimpleEmbedder.h>

#include <ogdf/planarlayout/mixed_model_layout/MixedModelBase.h>


namespace ogdf {


void MMOrder::init(
	PlanRep &PG,
	ShellingOrderModule &compOrder,
	adjEntry adjExternal)
{
	compOrder.callLeftmost(PG, m_lmc, adjExternal);
	m_left .init(1,m_lmc.length());
	m_right.init(1,m_lmc.length());
}


MixedModelLayout::MixedModelLayout()
{
	m_augmenter.reset(new PlanarAugmentation);
	m_compOrder.reset(new BiconnectedShellingOrder);
	m_crossingsBeautifier.reset(new MMDummyCrossingsBeautifier);
	m_embedder.reset(new SimpleEmbedder);
}


void MixedModelLayout::doCall(
	PlanRep &PG,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	MixedModelBase mm(PG,gridLayout);

	if(fixEmbedding) {
		OGDF_ASSERT(PG.representsCombEmbedding());
		PlanarAugmentationFix fixAugmenter;
		mm.computeOrder(fixAugmenter, nullptr, adjExternal, *m_compOrder);
	} else
		mm.computeOrder(*m_augmenter,m_embedder.get(),nullptr,*m_compOrder);

	mm.assignIopCoords();
	mm.placeNodes();
	mm.postprocessing1();
	mm.setBends();
	mm.postprocessing2();

	m_crossingsBeautifier->call(PG,gridLayout);

	int xmin, ymin;
	gridLayout.computeBoundingBox(xmin,boundingBox.m_x,ymin,boundingBox.m_y);
}

}
