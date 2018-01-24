/** \file
 * \brief Contains diverse enumerations and string constants.
 *
 * \author Christian Wolf, Carsten Gutwenger
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

#include <ogdf/fileformats/Ogml.h>

namespace ogdf
{

/// This vector contains the real names of all OGML tags
const string Ogml::s_tagNames[TAG_NUM] = {
	//"none"
	"bool",
	"composed",
	"constraint",
	"constraints",
	"content",
	"data",
	"default",
	"edge",
	"edgeRef",
	"edgeStyle",
	"edgeStyleTemplate",
	"template",
	"endpoint",
	"fill",
	"font",
	"graph",
	"graphStyle",
	"int",
	"label",
	"labelRef",
	"labelStyle",
	"labelStyleTemplate",
	"template",
	"layout",
	"line",
	"location",
	"node",
	"nodeRef",
	"nodeStyle",
	"nodeStyleTemplate",
	"template",
	"num",
	"ogml",
	"point",
	"port",
	"segment",
	"shape",
	"source",
	"sourceStyle",
	"string",
	"structure",
	"styles",
	"styleTemplates",
	"target",
	"targetStyle",
	"text",
	"image"
};


// This vector contains the real names of all OGML attributes.
const string Ogml::s_attributeNames[ATT_NUM] = {
	"xmlns",
	"textAlign",
	"verticalAlign",
	"angle",
	"color",
	"decoration",
	"defaultEdgeTemplate",
	"defaultLabelTemplate",
	"defaultNodeTemplate",
	"family",
	"height",
	"id",			// id attribute
	"idRef",		// attribute idRef of elements source, target, nodeRef, nodeStyle
	"idRef",		// attribute idRef of elements edgeRef, edgeStyle
	"idRef",		// attribute idRef of elements edgeRef, edgeStyle
	"idRef",		// attribute idRef of element endpoint
	"idRef",		// attribute idRef of element endpoint
	"idRef",		// attribute idRef of subelement template of element nodeStyle
	"idRef",		// attribute idRef of subelement template of element edgeStyle
	"idRef",		// attribute idRef of subelement template of element labelStyle
	"idRef",		// attribute idRef of subelement endpoint of element segment
	"name",
	"type",			// attribute type of subelement line of tag nodeStyleTemplate
	"type",			// attribute type of subelement shape of tag nodeStyleTemplate
	"pattern",
	"patternColor",
	"rotation",
	"size",
	"stretch",
	"style",
	"transform",
	"type",			// attribute type of subelements source-/targetStyle of tag edgeStyleTemplate
	"uri",
	"value",
	"value",
	"value",
	"variant",
	"weight",
	"width",
	"x",
	"y",
	"z",
	"type",
	"disabled"
};


// This vector contains the real names of all OGML values of attributes.
const string Ogml::s_attributeValueNames[ATT_VAL_NUM] = {
	"any",					// for any attributeValue
	"bold",
	"black",
	"bool",
	"bottom",
	"box",
	"capitalize",
	"center",
	"circle",
	"condensed",
	"dash",
	"demiBold",
	"dot",
	"dashDot",
	"dashDotDot",
	"ellipse",
	"expanded",
	"extraCondensed",
	"extraExpanded",
	"oBox",
	"oCircle",
	"oRhomb",
	"oTriangle",
	"arrow",
	"groove",
	"hexagon",
	"hex",
	"id",
	"nodeId",				// attribute idRef of elements source, target, nodeRef, nodeStyle
	"edgeId",				// attribute idRef of elements edgeRef, edgeStyle
	"labelId",				// attribute idRef of elements edgeRef, edgeStyle
	"sourceId",				// attribute idRef of element endpoint
	"targetId",				// attribute idRef of element endpoint
	"nodeStyleTemplateId",	// attribute idRef of subelement template of element nodeStyle
	"edgeStyleTemplateId",	// attribute idRef of subelement template of element edgeStyle
	"labelStyleTemplateId",	// attribute idRef of subelement template of element labelStyle
	"pointId",				// attribute idRef of subelement endpoint of element segment
	"image",
	"inset",
	"int",
	"invTriangle",
	"invTrapeze",
	"invParallelogram",
	"italic",
	"justify",
	"left",
	"light",
	"lineThrough",
	"lowercase",
	"middle"
	"noFill",
	"none",
	"normal",
	"num",
	"oblique",
	"oct",
	"octagon",
	"outset",
	"overline",
	"parallelogram",
	"pentagon",
	"rect",
	"regular",
	"rhomb",
	"ridge",
	"right",
	"roundedRect",
	"semiCondensed",
	"semiExpanded",
	"smallCaps",
	"solid",
	"dense1",
	"dense2",
	"dense3",
	"dense4",
	"dense5",
	"dense6",
	"dense7",
	"hor",
	"ver",
	"cross",
	"bDiag",
	"fDiag",
	"diagCross",
	"string",
	"tee",
	"top",
	"trapeze",
	"triangle",
	"ultraCondensed",
	"ultraExpanded",
	"underline",
	"uppercase",
	"uri",
	"vee",
	"Alignment",
	"Anchor",
	"Sequence"
};


const string Ogml::s_graphTypeS[GRAPH_TYPE_NUM] = {
	"graph",
	"clusterGraph",
	"compoundGraph",
	"corruptCompoundGraph"
};

}
