/** \file
 * \brief Tests for UML layout algorithms.
 *
 * \author Tilo Wiedera
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

#include <ogdf/uml/PlanarizationLayoutUML.h>

#include "layout_helpers.h"

class PLUMock : public LayoutModule {
	PlanarizationLayoutUML layout;

public:
	virtual void call(GraphAttributes &attr) override {
		const Graph& G = attr.constGraph();
		GraphCopy copyG(G);
		UMLGraph umlGraph(copyG, attr.attributes());

		layout.call(umlGraph);

		for (node v : G.nodes) {
			node w = copyG.copy(v);
			attr.x(v) = umlGraph.x(w);
			attr.y(v) = umlGraph.y(w);
		}

		for (edge e : G.edges) {
			attr.bends(e) = umlGraph.bends(copyG.copy(e));
		}
	}
};

go_bandit([] {
	describeLayout<PLUMock>("PlanarizationLayoutUML", GraphAttributes::edgeType | GraphAttributes::nodeType, {GraphProperty::simple, GraphProperty::sparse}, true);
});
