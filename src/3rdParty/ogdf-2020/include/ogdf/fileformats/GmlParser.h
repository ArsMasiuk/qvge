/** \file
 * \brief Declaration of classes gml::Object and gml::Parser.
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

#pragma once

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/fileformats/GML.h>

namespace ogdf {

namespace gml {

//! Represents node in GML parse tree
struct OGDF_EXPORT Object {
	Object *pBrother; // brother of node in tree
	Key key; // tag of node
	ObjectType valueType; // type of node

	// the entry in the union is selected according to m_valueType:
	//   IntValue -> m_intValue
	//   DoubleValue -> m_doubleValue
	//   StringValue -> m_stringValue
	//   ListBegin -> m_pFirstSon (in case of a list, m_pFirstSon is pointer
	//     to first son and the sons are chained by m_pBrother)
	union {
		int intValue;
		double doubleValue;
		const char *stringValue;
		Object *pFirstSon;
	};

	// construction
	Object(Key k, int value) : pBrother(nullptr), key(k),
		valueType(ObjectType::IntValue), intValue(value)  { }

	Object(Key k, double value) : pBrother(nullptr), key(k),
		valueType(ObjectType::DoubleValue), doubleValue(value)  { }

	Object(Key k, const char *value) : pBrother(nullptr), key(k),
		valueType(ObjectType::StringValue), stringValue(value)  { }

	Object(Key k) : pBrother(nullptr), key(k),
		valueType(ObjectType::ListBegin), pFirstSon(nullptr)  { }

	OGDF_NEW_DELETE
};


//! Reads GML file and constructs GML parse tree
class OGDF_EXPORT Parser {
	std::istream *m_is;
	bool m_error;

	char *m_rLineBuffer, *m_lineBuffer, *m_pCurrent, *m_pStore, m_cStore;

	int m_intSymbol;
	double m_doubleSymbol;
	const char *m_stringSymbol;
	Key m_keySymbol;
	string m_longString;

	Object *m_objectTree; // root node of GML parse tree

	bool m_doCheck;
	Array<node> m_mapToNode;
	Object  *m_graphObject;

public:
	// construction: creates object tree
	// sets m_error flag if an error occured
	explicit Parser(std::istream &is, bool doCheck = false);

	//! Destruction: destroys object tree
	~Parser();

	// true <=> an error in GML files has been detected
	bool error() const { return m_error; }

	// creates graph from GML parse tree
	bool read(Graph &G);
	// creates attributed graph from GML parse tree
	bool read(Graph &G, GraphAttributes &GA);

	//read only cluster part of object tree and create cluster graph structure
	bool readCluster(Graph &G, ClusterGraph &CG, ClusterGraphAttributes *ACG = nullptr);

protected:

	//! Reads cluster subtree information recursively
	bool recursiveClusterRead(
		Object* clusterObject,
		ClusterGraph& CG,
		cluster c,
		ClusterGraphAttributes* ACG = nullptr);

private:
	void createObjectTree(std::istream &is, bool doCheck);
	void setError(const char *errorString, Logger::Level level = Logger::Level::Default);

	Object *parseList(ObjectType closingKey);
	ObjectType getNextSymbol();
	bool getLine();

	Object *getNodeIdRange(int &minId,int &maxId);
	void readLineAttribute(Object *object, DPolyline &dpl);

	void destroyObjectList(Object *object);
};

}

}
