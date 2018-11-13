/** \file
 * \brief Implementation of GML parser (class GmlParser)
 * (used for parsing and reading GML files)
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

#include <ogdf/fileformats/GmlParser.h>
#include <ogdf/basic/HashArray.h>


namespace ogdf {

GmlParser::GmlParser(std::istream &is, bool doCheck)
{
	m_objectTree = nullptr;

	if (!is) {
		setError("Cannot open file.");
		return;
	}

	createObjectTree(is,doCheck);

	int minId, maxId;
	m_graphObject = getNodeIdRange(minId, maxId);
	if (!m_graphObject) {
		setError("Cannot open file.");
		return;
	}
	m_mapToNode.init(minId,maxId,nullptr);
}


void GmlParser::createObjectTree(std::istream &is, bool doCheck)
{
	initPredefinedKeys();
	m_error = false;

	m_is = &is;
	m_doCheck = doCheck; // indicates more extensive checking

	// initialize line buffer (note: GML specifies a maximal line length
	// of 254 characters!)
	m_rLineBuffer = new char[256];
	if (m_rLineBuffer == nullptr) OGDF_THROW(InsufficientMemoryException);

	*m_rLineBuffer = '\n';
	m_lineBuffer = m_rLineBuffer+1;

	m_pCurrent = m_pStore = m_lineBuffer;
	m_cStore = 0; // forces getNextSymbol() to read first line

	// create object tree
	m_objectTree = parseList(GmlObjectType::Eof, GmlObjectType::ListEnd);

	delete[] m_rLineBuffer;
}

// we use predefined id constants for all relevant keys
// this allows us to use efficient switch() statements in read() methods
void GmlParser::initPredefinedKeys()
{
	m_hashTable.fastInsert("id",             GmlParserPredefinedKey::Id);
	m_hashTable.fastInsert("label",          GmlParserPredefinedKey::Label);
	m_hashTable.fastInsert("Creator",        GmlParserPredefinedKey::Creator);
	m_hashTable.fastInsert("name",           GmlParserPredefinedKey::Name);
	m_hashTable.fastInsert("graph",          GmlParserPredefinedKey::Graph);
	m_hashTable.fastInsert("version",        GmlParserPredefinedKey::Version);
	m_hashTable.fastInsert("directed",       GmlParserPredefinedKey::Directed);
	m_hashTable.fastInsert("node",           GmlParserPredefinedKey::Node);
	m_hashTable.fastInsert("edge",           GmlParserPredefinedKey::Edge);
	m_hashTable.fastInsert("graphics",       GmlParserPredefinedKey::Graphics);
	m_hashTable.fastInsert("x",              GmlParserPredefinedKey::X);
	m_hashTable.fastInsert("y",              GmlParserPredefinedKey::Y);
	m_hashTable.fastInsert("w",              GmlParserPredefinedKey::W);
	m_hashTable.fastInsert("h",              GmlParserPredefinedKey::H);
	m_hashTable.fastInsert("type",           GmlParserPredefinedKey::Type);
	m_hashTable.fastInsert("width",          GmlParserPredefinedKey::Width);
	m_hashTable.fastInsert("source",         GmlParserPredefinedKey::Source);
	m_hashTable.fastInsert("target",         GmlParserPredefinedKey::Target);
	m_hashTable.fastInsert("arrow",          GmlParserPredefinedKey::Arrow);
	m_hashTable.fastInsert("Line",           GmlParserPredefinedKey::Line);
	m_hashTable.fastInsert("line",           GmlParserPredefinedKey::line);
	m_hashTable.fastInsert("point",          GmlParserPredefinedKey::Point);
	m_hashTable.fastInsert("generalization", GmlParserPredefinedKey::Generalization);
	m_hashTable.fastInsert("subgraph",       GmlParserPredefinedKey::SubGraph);
	m_hashTable.fastInsert("fill",           GmlParserPredefinedKey::Fill);
	m_hashTable.fastInsert("cluster",        GmlParserPredefinedKey::Cluster);
	m_hashTable.fastInsert("rootcluster",    GmlParserPredefinedKey::Root);
	m_hashTable.fastInsert("vertex",         GmlParserPredefinedKey::Vertex);
	m_hashTable.fastInsert("color",		     GmlParserPredefinedKey::Color);
	m_hashTable.fastInsert("height",		 GmlParserPredefinedKey::Height);
	m_hashTable.fastInsert("stipple",        GmlParserPredefinedKey::Stipple);  //linestyle
	m_hashTable.fastInsert("pattern",        GmlParserPredefinedKey::Pattern); //brush pattern
	m_hashTable.fastInsert("lineWidth",      GmlParserPredefinedKey::LineWidth);//line width
	m_hashTable.fastInsert("template",       GmlParserPredefinedKey::Template);//line width
	m_hashTable.fastInsert("weight",         GmlParserPredefinedKey::EdgeWeight);

	// further keys get id's starting with NextPredefKey
	m_num = GmlParserPredefinedKey::NextPredefKey;
}


GmlObject *GmlParser::parseList(int closingKey,
	int /* errorKey */)
{
	GmlObject *firstSon = nullptr;
	GmlObject **pPrev = &firstSon;

	for( ; ; ) {
		int symbol = getNextSymbol();

		if (symbol == closingKey || symbol == GmlObjectType::Error)
			return firstSon;

		if (symbol != GmlObjectType::Key) {
			setError("key expected");
			return firstSon;
		}

		GmlKey key = m_keySymbol;

		symbol = getNextSymbol();
		GmlObject *object = nullptr;

		switch (symbol) {
		case GmlObjectType::IntValue:
			object = new GmlObject(key,m_intSymbol);
			break;

		case GmlObjectType::DoubleValue:
			object = new GmlObject(key,m_doubleSymbol);
			break;

		case GmlObjectType::StringValue: {
			size_t len = strlen(m_stringSymbol)+1;
			char *pChar = new char[len];
			if (pChar == nullptr) OGDF_THROW(InsufficientMemoryException);

#ifdef _MSC_VER
			strcpy_s(pChar, len, m_stringSymbol);
#else
			strcpy(pChar, m_stringSymbol);
#endif
			object = new GmlObject(key,pChar); }
			break;

		case GmlObjectType::ListBegin:
			object = new GmlObject(key);
			object->m_pFirstSon = parseList(GmlObjectType::ListEnd, GmlObjectType::Eof);
			break;

		case GmlObjectType::ListEnd:
			setError("unexpected end of list");
			return firstSon;

		case GmlObjectType::Key:
			setError("unexpected key");
			return firstSon;

		case GmlObjectType::Eof:
			setError("missing value");
			return firstSon;

		case GmlObjectType::Error:
			return firstSon;

		// one of the cases above has to occur
		default:
			OGDF_ASSERT(false);
		}

		*pPrev = object;
		pPrev = &object->m_pBrother;
	}

	return firstSon;
}


void GmlParser::destroyObjectList(GmlObject *object)
{
	GmlObject *nextObject;
	for(; object; object = nextObject) {
		nextObject = object->m_pBrother;

		if (object->m_valueType == GmlObjectType::StringValue)
			delete[] const_cast<char *>(object->m_stringValue);

		else if (object->m_valueType == GmlObjectType::ListBegin)
			destroyObjectList(object->m_pFirstSon);

		delete object;
	}
}


GmlParser::~GmlParser()
{
	// we have to delete all objects and allocated char arrays in string values
	destroyObjectList(m_objectTree);
}


bool GmlParser::getLine()
{
	do {
		if (m_is->eof()) return false;
		(*m_is) >> std::ws;  // skip whitespace like spaces for indentation
		m_is->getline(m_lineBuffer,255);
		if (m_is->fail())
			return false;
		for(m_pCurrent = m_lineBuffer;
			*m_pCurrent && isspace((int)*m_pCurrent); ++m_pCurrent) ;
	} while (*m_pCurrent == '#' || *m_pCurrent == 0);

	return true;
}


int GmlParser::getNextSymbol()
{
	*m_pStore = m_cStore;

	// eat whitespace
	for(; *m_pCurrent && isspace((int)*m_pCurrent); ++m_pCurrent) ;

	// get new line if required
	if (*m_pCurrent == 0 && !getLine()) {
		return GmlObjectType::Eof;
	}

	// identify start of current symbol
	char *pStart = m_pCurrent;

	// we currently do not support strings with line breaks!
	if (*pStart == '\"')
	{ // string
		m_stringSymbol = ++m_pCurrent;
		char *pWrite = m_pCurrent;
		auto update = [&] {
			while(*m_pCurrent != 0 && *m_pCurrent != '\"') {
				if (*m_pCurrent == '\\') {
					switch(*(m_pCurrent+1)) {
					case 0:
						*m_pCurrent = 0;
						break;
					case '\\':
						*pWrite++ = '\\';
						m_pCurrent += 2;
						break;
					case '\"':
						*pWrite++ = '\"';
						m_pCurrent += 2;
						break;
					default:
						// just copy the escape sequence as is
						*pWrite++ = *m_pCurrent++;
						*pWrite++ = *m_pCurrent++;
					}
				} else {
					*pWrite++ = *m_pCurrent++;
				}
			}
		};
		update();

		if (*m_pCurrent == 0)
		{
			*pWrite = 0;
			m_longString = (pStart+1);
			while(getLine())
			{
				m_pCurrent = pWrite = m_lineBuffer;
				update();

				if (*m_pCurrent == 0) {
					*pWrite = 0;
					m_longString += m_lineBuffer;

				} else {
					m_cStore = *(m_pStore = m_pCurrent);
					++m_pCurrent;
					*pWrite = 0;
					m_longString += m_lineBuffer;
					break;
				}
			}
			m_stringSymbol = m_longString.c_str();

		} else {
			m_cStore = *(m_pStore = m_pCurrent);
			++m_pCurrent;
			*pWrite = 0;
		}

		return GmlObjectType::StringValue;
	}

	// identify end of current symbol
	while(*m_pCurrent != 0 && !isspace((int)*m_pCurrent)) ++m_pCurrent;

	m_cStore = *(m_pStore = m_pCurrent);
	*m_pCurrent = 0;

	if(isalpha((int)*pStart)) { // key

		// check if really a correct key (error if not)
		if (m_doCheck) {
			for (char *p = pStart+1; *p; ++p)
				if (!(isalpha((int)*p) || isdigit((int)*p))) {
					setError("malformed key");
					return GmlObjectType::Error;
				}
		}

		m_keySymbol = hashString(pStart);
		return GmlObjectType::Key;

	} else if (*pStart == '[') {
		return GmlObjectType::ListBegin;

	} else if (*pStart == ']') {
		return GmlObjectType::ListEnd;

	} else if (*pStart == '-' || isdigit((int)*pStart)) { // int or double
		char *p = pStart+1;
		while(isdigit((int)*p)) ++p;

		if (*p == '.') { // double
			// check to be done

			m_doubleSymbol = atof(pStart);
			return GmlObjectType::DoubleValue;

		} else { // int
			if (*p != 0) {
				setError("malformed number");
				return GmlObjectType::Error;
			}

			m_intSymbol = atoi(pStart);
			return GmlObjectType::IntValue;
		}
	}

	setError("unknown symbol");

	return GmlObjectType::Error;
}


GmlKey GmlParser::hashString(const string &str)
{
	GmlKey key = m_hashTable.insertByNeed(str,-1);
	if(key->info() == -1) key->info() = m_num++;

	return key;
}


GmlObject *GmlParser::getNodeIdRange(int &minId,int &maxId)
{
	minId = maxId = 0;

	GmlObject *graphObject = m_objectTree;
	for(; graphObject; graphObject = graphObject->m_pBrother)
		if (id(graphObject) == GmlParserPredefinedKey::Graph) break;

	if (!graphObject || graphObject->m_valueType != GmlObjectType::ListBegin) {
		return nullptr;
	}

	bool first = true;
	GmlObject *son = graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {
		if (id(son) == GmlParserPredefinedKey::Node && son->m_valueType == GmlObjectType::ListBegin) {

			GmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
				if (id(nodeSon) == GmlParserPredefinedKey::Id ||
					nodeSon->m_valueType == GmlObjectType::IntValue)
				{
					int nodeSonId = nodeSon->m_intValue;
					if (first) {
						minId = maxId = nodeSonId;
						first = false;
					} else {
						if (nodeSonId < minId) minId = nodeSonId;
						if (nodeSonId > maxId) maxId = nodeSonId;
					}
				}
			}
		}
	}

	return graphObject;
}


bool GmlParser::read(Graph &G)
{
	G.clear();

	int minId = m_mapToNode.low();
	int maxId = m_mapToNode.high();
	int notDefined = minId-1; //indicates not defined id key

	GmlObject *son = m_graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother)
	{
		switch(id(son))
		{
		case GmlParserPredefinedKey::Node: {
			if (son->m_valueType != GmlObjectType::ListBegin) break;

			// set attributes to default values
			int vId = notDefined;

			// read all relevant attributes
			GmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
				if (id(nodeSon) == GmlParserPredefinedKey::Id &&
					nodeSon->m_valueType == GmlObjectType::IntValue)
				{
					vId = nodeSon->m_intValue;
				}
			}

			// check if everything required is defined correctly
			if (vId == notDefined) {
				setError("node id not defined");
				return false;
			}

			// create new node if necessary
			if (m_mapToNode[vId] == nullptr) m_mapToNode[vId] = G.newNode(); }
			break;

		case GmlParserPredefinedKey::Edge: {
			if (son->m_valueType != GmlObjectType::ListBegin) break;

			// set attributes to default values
			int sourceId = notDefined, targetId = notDefined;

			// read all relevant attributes
			GmlObject *edgeSon = son->m_pFirstSon;
			for(; edgeSon; edgeSon = edgeSon->m_pBrother) {

				switch(id(edgeSon)) {
				case GmlParserPredefinedKey::Source:
					if(sourceId == notDefined) {
						if (edgeSon->m_valueType != GmlObjectType::IntValue) break;
						sourceId = edgeSon->m_intValue;
						break;
					} else {
						setError("ambiguous source encountered");
						return false;
					}

				case GmlParserPredefinedKey::Target:
					if(targetId == notDefined) {
						if (edgeSon->m_valueType != GmlObjectType::IntValue) break;
						targetId = edgeSon->m_intValue;
						break;
					} else {
						setError("ambiguous target encountered");
						return false;
					}
				}
			}

			// check if everything required is defined correctly
			if (sourceId == notDefined || targetId == notDefined) {
				setError("source or target id not defined");
				return false;

			} else if (sourceId < minId || maxId < sourceId ||
				targetId < minId || maxId < targetId) {
				setError("source or target id out of range");
				return false;
			}

			// create adjacent nodes if necessary and new edge
			if (m_mapToNode[sourceId] == nullptr) m_mapToNode[sourceId] = G.newNode();
			if (m_mapToNode[targetId] == nullptr) m_mapToNode[targetId] = G.newNode();

			G.newEdge(m_mapToNode[sourceId],m_mapToNode[targetId]);
			}
			break;
		}
	}

	return true;
}

bool GmlParser::read(Graph &G, GraphAttributes &AG)
{
	OGDF_ASSERT(&G == &(AG.constGraph()));

	G.clear();

	int minId = m_mapToNode.low();
	int maxId = m_mapToNode.high();
	int notDefined = minId-1; //indicates not defined id key

	HashArray<string,Shape> strToShape(Shape::Rect);
	strToShape["rectangle"]        = Shape::Rect;
	strToShape["rect"]             = Shape::Rect;
	strToShape["roundedRect"]      = Shape::RoundedRect;
	strToShape["oval"]             = Shape::Ellipse;
	strToShape["ellipse"]          = Shape::Ellipse;
	strToShape["triangle"]         = Shape::Triangle;
	strToShape["pentagon"]         = Shape::Pentagon;
	strToShape["hexagon"]          = Shape::Hexagon;
	strToShape["octagon"]          = Shape::Octagon;
	strToShape["rhomb"]            = Shape::Rhomb;
	strToShape["trapeze"]          = Shape::Trapeze;
	strToShape["parallelogram"]    = Shape::Parallelogram;
	strToShape["invTriangle"]      = Shape::InvTriangle;
	strToShape["invTrapeze"]       = Shape::InvTrapeze;
	strToShape["invParallelogram"] = Shape::InvParallelogram;
	strToShape["image"]            = Shape::Image;

	DPolyline bends;

	GmlObject *son = m_graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {

		switch(id(son)) {
		case GmlParserPredefinedKey::Node: {
			if (son->m_valueType != GmlObjectType::ListBegin) break;

			// set attributes to default values
			int vId = notDefined;
			double x = 0, y = 0, w = 0, h = 0;
			string label;
			string templ;
			string fill;  // the fill color attribute
			string line;  // the line color attribute
			string shape; //the shape type
			float lineWidth = 1.0f; //node line width
			int pattern = 1; //node brush pattern
			int stipple = 1; //line style pattern
			int weight = 0; // node weight

			// read all relevant attributes
			GmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
				switch(id(nodeSon)) {
				case GmlParserPredefinedKey::Id:
					if(nodeSon->m_valueType != GmlObjectType::IntValue) break;
					vId = nodeSon->m_intValue;
					break;

				case GmlParserPredefinedKey::Graphics:
					if (nodeSon->m_valueType != GmlObjectType::ListBegin) break;

					for(GmlObject *graphicsObject = nodeSon->m_pFirstSon; graphicsObject;
						graphicsObject = graphicsObject->m_pBrother)
					{
						switch(id(graphicsObject)) {
						case GmlParserPredefinedKey::X:
							if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) break;
							x = graphicsObject->m_doubleValue;
							break;

						case GmlParserPredefinedKey::Y:
							if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) break;
							y = graphicsObject->m_doubleValue;
							break;

						case GmlParserPredefinedKey::W:
							if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) break;
							w = graphicsObject->m_doubleValue;
							break;

						case GmlParserPredefinedKey::H:
							if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) break;
							h = graphicsObject->m_doubleValue;
							break;

						case GmlParserPredefinedKey::Fill:
							if(graphicsObject->m_valueType != GmlObjectType::StringValue) break;
							fill = graphicsObject->m_stringValue;
							break;

						case GmlParserPredefinedKey::line:
							if(graphicsObject->m_valueType != GmlObjectType::StringValue) break;
							line = graphicsObject->m_stringValue;
							break;

						case GmlParserPredefinedKey::LineWidth:
							if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) break;
							lineWidth = (float)graphicsObject->m_doubleValue;
							break;

						case GmlParserPredefinedKey::Type:
							if(graphicsObject->m_valueType != GmlObjectType::StringValue) break;
							shape = graphicsObject->m_stringValue;
							break;
						case GmlParserPredefinedKey::Pattern: //fill style
							if(graphicsObject->m_valueType != GmlObjectType::IntValue) break;
							pattern = graphicsObject->m_intValue;
							break;
						case GmlParserPredefinedKey::Stipple: //line style
							if(graphicsObject->m_valueType != GmlObjectType::IntValue) break;
							stipple = graphicsObject->m_intValue;
						}
					}
					break;

				case GmlParserPredefinedKey::Template:
					if (nodeSon->m_valueType != GmlObjectType::StringValue) break;
					templ = nodeSon->m_stringValue;
					break;

				case GmlParserPredefinedKey::Label:
					if (nodeSon->m_valueType != GmlObjectType::StringValue) break;
					label = nodeSon->m_stringValue;
					break;

				case GmlParserPredefinedKey::EdgeWeight: //sic!
					if (nodeSon->m_valueType != GmlObjectType::IntValue) break;
					weight = nodeSon->m_intValue;
					break;
				}
			}

			// check if everything required is defined correctly
			if (vId == notDefined) {
				setError("node id not defined");
				return false;
			}

			// create new node if necessary and assign attributes
			if (m_mapToNode[vId] == nullptr) m_mapToNode[vId] = G.newNode();
			node v = m_mapToNode[vId];
			if (AG.has(GraphAttributes::nodeGraphics))
			{
				AG.x(v) = x;
				AG.y(v) = y;
				AG.width (v) = w;
				AG.height(v) = h;
				AG.shape(v) = strToShape[shape];
			}
			if (AG.has(GraphAttributes::nodeLabel))
				AG.label(m_mapToNode[vId]) = label;
			if (AG.has(GraphAttributes::nodeTemplate))
				AG.templateNode(m_mapToNode[vId]) = templ;
			if (AG.has(GraphAttributes::nodeId))
				AG.idNode(m_mapToNode[vId]) = vId;
			if (AG.has(GraphAttributes::nodeWeight))
				AG.weight(m_mapToNode[vId]) = weight;
			if (AG.has(GraphAttributes::nodeStyle))
			{
				AG.fillColor(m_mapToNode[vId]) = fill;
				AG.strokeColor(m_mapToNode[vId]) = line;
				AG.fillPattern(m_mapToNode[vId]) = intToFillPattern(pattern);
				AG.strokeType(m_mapToNode[vId]) = intToStrokeType(stipple);
				AG.strokeWidth(m_mapToNode[vId]) = lineWidth;
			}
							}
							//Todo: line style set stipple value
							break;

		case GmlParserPredefinedKey::Edge: {
			string arrow; // the arrow type attribute
			string fill;  //the color fill attribute
			int stipple = 1;  //the line style
			float lineWidth = 1.0f;
			double edgeWeight = 1.0;
			int subGraph = 0; //edgeSubGraphs attribute
			string label; // label attribute

			if (son->m_valueType != GmlObjectType::ListBegin) break;

			// set attributes to default values
			int sourceId = notDefined, targetId = notDefined;
			Graph::EdgeType umlType = Graph::EdgeType::association;

			// read all relevant attributes
			GmlObject *edgeSon = son->m_pFirstSon;
			for(; edgeSon; edgeSon = edgeSon->m_pBrother) {

				switch(id(edgeSon)) {
				case GmlParserPredefinedKey::Source:
					if (edgeSon->m_valueType != GmlObjectType::IntValue) break;
					sourceId = edgeSon->m_intValue;
					break;

				case GmlParserPredefinedKey::Target:
					if (edgeSon->m_valueType != GmlObjectType::IntValue) break;
					targetId = edgeSon->m_intValue;
					break;

				case GmlParserPredefinedKey::SubGraph:
					if (edgeSon->m_valueType != GmlObjectType::IntValue) break;
					subGraph = edgeSon->m_intValue;
					break;

				case GmlParserPredefinedKey::Label:
					if (edgeSon->m_valueType != GmlObjectType::StringValue) break;
					label = edgeSon->m_stringValue;
					break;

				case GmlParserPredefinedKey::Graphics:
					if (edgeSon->m_valueType != GmlObjectType::ListBegin) break;

					for(GmlObject *graphicsObject = edgeSon->m_pFirstSon; graphicsObject;
						graphicsObject = graphicsObject->m_pBrother)
					{
						if(id(graphicsObject) == GmlParserPredefinedKey::Line &&
							graphicsObject->m_valueType == GmlObjectType::ListBegin)
						{
							readLineAttribute(graphicsObject->m_pFirstSon,bends);
						}
						if(id(graphicsObject) == GmlParserPredefinedKey::Arrow &&
							graphicsObject->m_valueType == GmlObjectType::StringValue)
							arrow = graphicsObject->m_stringValue;
						if(id(graphicsObject) == GmlParserPredefinedKey::Fill &&
							graphicsObject->m_valueType == GmlObjectType::StringValue)
							fill = graphicsObject->m_stringValue;
						if (id(graphicsObject) == GmlParserPredefinedKey::Stipple && //line style
							graphicsObject->m_valueType == GmlObjectType::IntValue)
							stipple = graphicsObject->m_intValue;
						if (id(graphicsObject) == GmlParserPredefinedKey::LineWidth && //line width
							graphicsObject->m_valueType == GmlObjectType::DoubleValue)
							lineWidth = (float)graphicsObject->m_doubleValue;
						if (id(graphicsObject) == GmlParserPredefinedKey::EdgeWeight &&
							graphicsObject->m_valueType == GmlObjectType::DoubleValue)
							edgeWeight = graphicsObject->m_doubleValue;
					}
					break;

				case GmlParserPredefinedKey::Generalization:
					if (edgeSon->m_valueType != GmlObjectType::IntValue) break;
					umlType = (edgeSon->m_intValue == 0) ?
						Graph::EdgeType::association : Graph::EdgeType::generalization;
					break;

				}
			}

			// check if everything required is defined correctly
			if (sourceId == notDefined || targetId == notDefined) {
				setError("source or target id not defined");
				return false;

			} else if (sourceId < minId || maxId < sourceId ||
				targetId < minId || maxId < targetId) {
					setError("source or target id out of range");
					return false;
			}

			// create adjacent nodes if necessary and new edge
			if (m_mapToNode[sourceId] == nullptr) m_mapToNode[sourceId] = G.newNode();
			if (m_mapToNode[targetId] == nullptr) m_mapToNode[targetId] = G.newNode();

			edge e = G.newEdge(m_mapToNode[sourceId],m_mapToNode[targetId]);
			if (AG.has(GraphAttributes::edgeGraphics))
				AG.bends(e).conc(bends);
			if (AG.has(GraphAttributes::edgeType))
				AG.type(e) = umlType;
			if(AG.has(GraphAttributes::edgeSubGraphs))
				AG.subGraphBits(e) = subGraph;
			if (AG.has(GraphAttributes::edgeLabel))
				AG.label(e) = label;

			if (AG.has(GraphAttributes::edgeArrow)) {
				if (arrow == "none")
					AG.arrowType(e) = EdgeArrow::None;
				else if (arrow == "last")
					AG.arrowType(e) = EdgeArrow::Last;
				else if (arrow == "first")
					AG.arrowType(e) = EdgeArrow::First;
				else if (arrow == "both")
					AG.arrowType(e) = EdgeArrow::Both;
				else
					AG.arrowType(e) = EdgeArrow::Undefined;
			}

			if (AG.has(GraphAttributes::edgeStyle))
			{
				AG.strokeColor(e) = fill;
				AG.strokeType(e) = intToStrokeType(stipple);
				AG.strokeWidth(e) = lineWidth;
			}

			if (AG.has(GraphAttributes::edgeDoubleWeight))
				AG.doubleWeight(e) = edgeWeight;


			break; }
		case GmlParserPredefinedKey::Directed: {
			if(son->m_valueType != GmlObjectType::IntValue) break;
			AG.directed() = son->m_intValue > 0;
			break; }
		}
	}

	return true;
}


//the clustergraph has to be initialized on G!!,
//no clusters other then root cluster may exist, which holds all nodes
bool GmlParser::readCluster(Graph &G, ClusterGraph &CG, ClusterGraphAttributes *ACG)
{
	OGDF_ASSERT(&CG.constGraph() == &G);

	// now we need the cluster object
	GmlObject *rootObject;
	for(rootObject = m_objectTree;
	    rootObject && id(rootObject) != GmlParserPredefinedKey::Root;
	    rootObject = rootObject->m_pBrother);

	// we have to check if the file does really contain clusters
	// otherwise, rootcluster will suffice
	if (rootObject == nullptr) {
		return true;
	}
	if (id(rootObject) != GmlParserPredefinedKey::Root) {
		setError("missing rootcluster key");
		return false;
	}

	return rootObject->m_valueType == GmlObjectType::ListBegin && clusterRead(rootObject, CG, ACG);
}


//read all cluster tree information
bool GmlParser::clusterRead(
	GmlObject* rootCluster,
	ClusterGraph& CG,
	ClusterGraphAttributes* ACG)
{

	//the root cluster is only allowed to hold child clusters and
	//nodes in a list

	if (rootCluster->m_valueType != GmlObjectType::ListBegin) return false;

	// read all clusters and nodes
	GmlObject *rootClusterSon = rootCluster->m_pFirstSon;

	for(; rootClusterSon; rootClusterSon = rootClusterSon->m_pBrother)
	{
		switch(id(rootClusterSon))
		{
		case GmlParserPredefinedKey::Cluster:
			{
				//we could delete this, but we avoid the call
				if (rootClusterSon->m_valueType != GmlObjectType::ListBegin) return false;
				// set attributes to default values
				//we currently do not set any values
				cluster c = CG.newCluster(CG.rootCluster());

				//recursively read cluster
				recursiveClusterRead(rootClusterSon, CG, c);

			}
			break;
		case GmlParserPredefinedKey::Vertex: //direct root vertices
			{
				if (rootClusterSon->m_valueType != GmlObjectType::StringValue) return false;
				string vIDString = rootClusterSon->m_stringValue;

				//we only allow a vertex id as string identification
				if ((vIDString[0] != 'v') &&
					(!isdigit((int)vIDString[0])))return false; //do not allow labels
				//if old style entry "v"i
				if (!isdigit((int)vIDString[0])) //should check prefix?
					vIDString[0] = '0'; //leading zero to allow conversion
				int vID = std::stoi(vIDString);

				OGDF_ASSERT(m_mapToNode[vID] != nullptr);

				//we assume that no node is already assigned ! Changed:
				//all new nodes are assigned to root
				//CG.reassignNode(mapToNode[vID], CG.rootCluster());
				//it seems that this may be unnessecary, TODO check
				CG.reassignNode(m_mapToNode[vID], CG.rootCluster());
#if 0
				char* vIDChar = new char[vIDString.length()+1];
				for (int ind = 1; ind < vIDString.length(); ind++)
					vIDChar
#endif
			}
		}
	}

	return true;
}

bool GmlParser::readClusterAttributes(
	GmlObject* cGraphics,
	cluster c,
	ClusterGraphAttributes& ACG)
{
	string label;
	string fill;  // the fill color attribute
	string line;  // the line color attribute
	float lineWidth = 1.0f; //node line width
	int    pattern = 1; //node brush pattern
	int    stipple = 1; //line style pattern

	// read all relevant attributes
	GmlObject *graphicsObject = cGraphics->m_pFirstSon;
	for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother)
	{
		switch(id(graphicsObject))
		{
		case GmlParserPredefinedKey::X:
			if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) return false;
			ACG.x(c) = graphicsObject->m_doubleValue;
			break;

		case GmlParserPredefinedKey::Y:
			if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) return false;
			ACG.y(c) = graphicsObject->m_doubleValue;
			break;

		case GmlParserPredefinedKey::Width:
			if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) return false;
			ACG.width(c) = graphicsObject->m_doubleValue;
			break;

		case GmlParserPredefinedKey::Height:
			if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) return false;
			ACG.height(c) = graphicsObject->m_doubleValue;
			break;
		case GmlParserPredefinedKey::Fill:
			if(graphicsObject->m_valueType != GmlObjectType::StringValue) return false;
			ACG.fillColor(c) = graphicsObject->m_stringValue;
			break;
		case GmlParserPredefinedKey::Pattern:
			if(graphicsObject->m_valueType != GmlObjectType::IntValue) return false;
			pattern = graphicsObject->m_intValue;
			break;
			//line style
		case GmlParserPredefinedKey::Color: // line color
			if(graphicsObject->m_valueType != GmlObjectType::StringValue) return false;
			ACG.strokeColor(c) = graphicsObject->m_stringValue;
			break;

		case GmlParserPredefinedKey::Stipple:
			if(graphicsObject->m_valueType != GmlObjectType::IntValue) return false;
			stipple = graphicsObject->m_intValue;
			break;
		case GmlParserPredefinedKey::LineWidth:
			if(graphicsObject->m_valueType != GmlObjectType::DoubleValue) return false;
			lineWidth =	(float)graphicsObject->m_doubleValue;
			break;
			//TODO: backgroundcolor
			//case stylePredefKey:
			//case boderwidthPredefKey:
		}
	}

	//Hier eigentlich erst abfragen, ob clusterattributes setzbar in ACG,
	//dann setzen
	ACG.setStrokeType(c, intToStrokeType(stipple)); //defaulting 1
	ACG.strokeWidth(c) = lineWidth;
	ACG.setFillPattern(c, intToFillPattern(pattern));

	return true;
}

//recursively read cluster subtree information
bool GmlParser::recursiveClusterRead(GmlObject* clusterObject,
								ClusterGraph& CG,
								cluster c,
								ClusterGraphAttributes* ACG)
{

	//for direct root cluster sons, this is checked twice...
	if (clusterObject->m_valueType != GmlObjectType::ListBegin) return false;

	GmlObject *clusterSon = clusterObject->m_pFirstSon;

	for(; clusterSon; clusterSon = clusterSon->m_pBrother)
	{
		//we dont read the attributes, therefore look only for
		//id and sons
		switch(id(clusterSon))
		{
			case GmlParserPredefinedKey::Cluster:
				{
					if (clusterSon->m_valueType != GmlObjectType::ListBegin) return false;

					cluster cson = CG.newCluster(c);
					//recursively read child cluster
					recursiveClusterRead(clusterSon, CG, cson, ACG);
				}
				break;
			case GmlParserPredefinedKey::Label:
				if (ACG != nullptr) {
					if (clusterSon->m_valueType != GmlObjectType::StringValue) {
						return false;
					}
					ACG->label(c) = clusterSon->m_stringValue;
				}
				break;
			case GmlParserPredefinedKey::Template:
				if (ACG != nullptr) {
					if (clusterSon->m_valueType != GmlObjectType::StringValue) {
						return false;
					}
					ACG->templateCluster(c) = clusterSon->m_stringValue;
				}
				break;
			case GmlParserPredefinedKey::Graphics: //read the info for cluster c
				if (ACG != nullptr) {
					if (clusterSon->m_valueType != GmlObjectType::ListBegin) {
						return false;
					}
					readClusterAttributes(clusterSon, c, *ACG);
				}
				break;
			case GmlParserPredefinedKey::Vertex: //direct cluster vertex entries
				{
					if (clusterSon->m_valueType != GmlObjectType::StringValue) return false;
					string vIDString = clusterSon->m_stringValue;

					if ((vIDString[0] != 'v') &&
						(!isdigit((int)vIDString[0])))return false; //do not allow labels
					if (!isdigit((int)vIDString[0])) //should check prefix?
						vIDString[0] = '0'; //leading zero to allow conversion
					int vID = std::stoi(vIDString);

					OGDF_ASSERT(m_mapToNode[vID] != nullptr);

					// all nodes are already assigned to root
					CG.reassignNode(m_mapToNode[vID], c);
				}
		}
	}

	return true;
}

void GmlParser::readLineAttribute(GmlObject *object, DPolyline &dpl)
{
	dpl.clear();
	for(; object; object = object->m_pBrother) {
		if (id(object) == GmlParserPredefinedKey::Point &&
			object->m_valueType == GmlObjectType::ListBegin)
		{
			DPoint dp;

			GmlObject *pointObject = object->m_pFirstSon;
			for (; pointObject; pointObject = pointObject->m_pBrother) {
				if (pointObject->m_valueType != GmlObjectType::DoubleValue) continue;
				if (id(pointObject) == GmlParserPredefinedKey::X)
					dp.m_x = pointObject->m_doubleValue;
				else if (id(pointObject) == GmlParserPredefinedKey::Y)
					dp.m_y = pointObject->m_doubleValue;
			}

			dpl.pushBack(dp);
		}
	}
}


void GmlParser::setError(const char *errorString)
{
	m_error = true;
	m_errorString = errorString;
}


void GmlParser::indent(std::ostream &os, int d)
{
	for(int i = 1; i <= d; ++i)
		os << " ";
}

void GmlParser::output(std::ostream &os, GmlObject *object, int d)
{
	for(; object; object = object->m_pBrother) {
		indent(os,d); os << object->m_key->key();

		switch(object->m_valueType) {
		case GmlObjectType::IntValue:
			os << " " << object->m_intValue << "\n";
			break;

		case GmlObjectType::DoubleValue:
			os << " " << object->m_doubleValue << "\n";
			break;

		case GmlObjectType::StringValue:
			os << " \"" << object->m_stringValue << "\"\n";
			break;

		case GmlObjectType::ListBegin:
			os << "\n";
			output(os, object->m_pFirstSon, d+2);
			break;
		case GmlObjectType::ListEnd:
			break;
		case GmlObjectType::Key:
			break;
		case GmlObjectType::Eof:
			break;
		case GmlObjectType::Error:
			break;
		}
	}
}

}
