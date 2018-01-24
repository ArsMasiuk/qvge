/** \file
 * \brief Declaration of classes GmlObject and GmlParser.
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

#include <ogdf/basic/Hashing.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>

namespace ogdf {

using GmlKey = HashElement<string,int>*;

namespace GmlObjectType {
	const static int IntValue = 0;
	const static int DoubleValue = 1;
	const static int StringValue = 2;
	const static int ListBegin = 3;
	const static int ListEnd = 4;
	const static int Key = 5;
	const static int Eof = 6;
	const static int Error = 7;
};

namespace GmlParserPredefinedKey {
	const static int Id             = 0;
	const static int Label          = 1;
	const static int Creator        = 2;
	const static int Name           = 3;
	const static int Graph          = 4;
	const static int Version        = 5;
	const static int Directed       = 6;
	const static int Node           = 7;
	const static int Edge           = 8;
	const static int Graphics       = 9;
	const static int X              = 10;
	const static int Y              = 11;
	const static int W              = 12;
	const static int H              = 13;
	const static int Type           = 14;
	const static int Width          = 15;
	const static int Source         = 16;
	const static int Target         = 17;
	const static int Arrow          = 18;
	const static int Line           = 19;
	const static int Point          = 20;
	const static int Generalization = 21;
	const static int SubGraph       = 22;
	const static int Fill           = 23;
	const static int Cluster        = 24;
	const static int Root           = 25;
	const static int Vertex         = 26;
	const static int Color          = 27;
	const static int Height         = 28;
	const static int Stipple        = 29;
	const static int Pattern        = 30;
	const static int line           = 31; // different from Line, but why?
	const static int LineWidth      = 32;
	const static int Template       = 33;
	const static int EdgeWeight     = 34;
	const static int NextPredefKey  = 35;
}

//! Represents node in GML parse tree
struct OGDF_EXPORT GmlObject {
	GmlObject *m_pBrother; // brother of node in tree
	GmlKey m_key; // tag of node
	int m_valueType; // type of node

	// the entry in the union is selected according to m_valueType:
	//   IntValue -> m_intValue
	//   DoubleValue -> m_doubleValue
	//   StringValue -> m_stringValue
	//   ListBegin -> m_pFirstSon (in case of a list, m_pFirstSon is pointer
	//     to first son and the sons are chained by m_pBrother)
	union {
		int m_intValue;
		double m_doubleValue;
		const char *m_stringValue;
		GmlObject *m_pFirstSon;
	};

	// construction
	GmlObject(GmlKey key, int intValue) : m_pBrother(nullptr), m_key(key),
		m_valueType(GmlObjectType::IntValue), m_intValue(intValue)  { }

	GmlObject(GmlKey key, double doubleValue) : m_pBrother(nullptr), m_key(key),
		m_valueType(GmlObjectType::DoubleValue), m_doubleValue(doubleValue)  { }

	GmlObject(GmlKey key, const char *stringValue) : m_pBrother(nullptr), m_key(key),
		m_valueType(GmlObjectType::StringValue), m_stringValue(stringValue)  { }

	GmlObject(GmlKey key) : m_pBrother(nullptr), m_key(key),
		m_valueType(GmlObjectType::ListBegin), m_pFirstSon(nullptr)  { }

	OGDF_NEW_DELETE
};


//! Reads GML file and constructs GML parse tree
class OGDF_EXPORT GmlParser {
	Hashing<string,int> m_hashTable; // hash table for tags
	int m_num;

	std::istream *m_is;
	bool m_error;
	string m_errorString;

	char *m_rLineBuffer, *m_lineBuffer, *m_pCurrent, *m_pStore, m_cStore;

	int m_intSymbol;
	double m_doubleSymbol;
	const char *m_stringSymbol;
	GmlKey m_keySymbol;
	string m_longString;

	GmlObject *m_objectTree; // root node of GML parse tree

	bool m_doCheck;
	Array<node> m_mapToNode;
	GmlObject  *m_graphObject;

public:
	// construction: creates object tree
	// sets m_error flag if an error occured
	explicit GmlParser(std::istream &is, bool doCheck = false);

	//! Destruction: destroys object tree
	~GmlParser();

	// returns id of object
	int id(GmlObject *object) const { return object->m_key->info(); }

	// true <=> an error in GML files has been detected
	bool error() const { return m_error; }
	// returns error message
	const string &errorString() const { return m_errorString; }

	// creates graph from GML parse tree
	bool read(Graph &G);
	// creates attributed graph from GML parse tree
	bool read(Graph &G, GraphAttributes &AG);
#if 0
	//creates clustergraph from GML parse tree
	bool read(Graph &G, ClusterGraph & CG);
#endif
	//read only cluster part of object tree and create cluster graph structure
	bool readCluster(Graph &G, ClusterGraph &CG, ClusterGraphAttributes *ACG = nullptr);

protected:

	//read all cluster tree information
	bool clusterRead(
		GmlObject* rootCluster,
		ClusterGraph& CG,
		ClusterGraphAttributes* ACG = nullptr);

	//! Reads cluster subtree information recursively
	bool recursiveClusterRead(
		GmlObject* clusterObject,
		ClusterGraph& CG,
		cluster c,
		ClusterGraphAttributes* ACG = nullptr);

	bool readClusterAttributes(
		GmlObject* cGraphics,
		cluster c,
		ClusterGraphAttributes& ACG);

private:
	void createObjectTree(std::istream &is, bool doCheck);
	void initPredefinedKeys();
	void setError(const char *errorString);

	GmlObject *parseList(int closingKey, int errorKey);
	int getNextSymbol();
	bool getLine();

	GmlKey hashString(const string &str);

	GmlObject *getNodeIdRange(int &minId,int &maxId);
	void readLineAttribute(GmlObject *object, DPolyline &dpl);

	void destroyObjectList(GmlObject *object);

	void indent(std::ostream &os, int d);
	void output(std::ostream &os, GmlObject *object, int d);
};

}
