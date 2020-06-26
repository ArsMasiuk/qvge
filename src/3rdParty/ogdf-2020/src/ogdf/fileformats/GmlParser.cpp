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

#include <unordered_map>
#include <memory>

#include <ogdf/fileformats/GmlParser.h>
#include <ogdf/fileformats/Utils.h>


namespace ogdf {

namespace gml {


Parser::Parser(std::istream &is, bool doCheck)
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
		setError("Cannot obtain min and max node id.");
		return;
	}
	m_mapToNode.init(minId,maxId,nullptr);
}


void Parser::createObjectTree(std::istream &is, bool doCheck)
{
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
	m_objectTree = parseList(ObjectType::Eof);

	delete[] m_rLineBuffer;
}


Object *Parser::parseList(ObjectType closingKey)
{
	Object *firstSon = nullptr;
	Object **pPrev = &firstSon;

	for( ; ; ) {
		ObjectType symbol = getNextSymbol();

		if (symbol == closingKey || symbol == ObjectType::Error)
			return firstSon;

		if (symbol != ObjectType::Key) {
			setError("key expected");
			return firstSon;
		}

		Key key = m_keySymbol;

		symbol = getNextSymbol();
		Object *object = nullptr;

		switch (symbol) {
		case ObjectType::IntValue:
			object = new Object(key,m_intSymbol);
			break;

		case ObjectType::DoubleValue:
			object = new Object(key,m_doubleSymbol);
			break;

		case ObjectType::StringValue: {
			size_t len = strlen(m_stringSymbol)+1;
			char *pChar = new char[len];
			if (pChar == nullptr) OGDF_THROW(InsufficientMemoryException);

#ifdef _MSC_VER
			strcpy_s(pChar, len, m_stringSymbol);
#else
			strcpy(pChar, m_stringSymbol);
#endif
			object = new Object(key,pChar); }
			break;

		case ObjectType::ListBegin:
			object = new Object(key);
			object->pFirstSon = parseList(ObjectType::ListEnd);
			break;

		case ObjectType::ListEnd:
			setError("unexpected end of list");
			return firstSon;

		case ObjectType::Key:
			setError("unexpected key");
			return firstSon;

		case ObjectType::Eof:
			setError("missing value");
			return firstSon;

		case ObjectType::Error:
			return firstSon;

		// one of the cases above has to occur
		default:
			OGDF_ASSERT(false);
		}

		*pPrev = object;
		pPrev = &object->pBrother;
	}

	return firstSon;
}


void Parser::destroyObjectList(Object *object)
{
	Object *nextObject;
	for(; object; object = nextObject) {
		nextObject = object->pBrother;

		if (object->valueType == ObjectType::StringValue)
			delete[] const_cast<char *>(object->stringValue);

		else if (object->valueType == ObjectType::ListBegin)
			destroyObjectList(object->pFirstSon);

		delete object;
	}
}


Parser::~Parser()
{
	// we have to delete all objects and allocated char arrays in string values
	destroyObjectList(m_objectTree);
}


bool Parser::getLine()
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


ObjectType Parser::getNextSymbol()
{
	*m_pStore = m_cStore;

	// eat whitespace
	for(; *m_pCurrent && isspace((int)*m_pCurrent); ++m_pCurrent) ;

	// get new line if required
	if (*m_pCurrent == 0 && !getLine()) {
		return ObjectType::Eof;
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

		return ObjectType::StringValue;
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
					return ObjectType::Error;
				}
		}

		m_keySymbol = toKey(pStart);
		return ObjectType::Key;

	} else if (*pStart == '[') {
		return ObjectType::ListBegin;

	} else if (*pStart == ']') {
		return ObjectType::ListEnd;

	} else if (*pStart == '-' || isdigit((int)*pStart)) { // int or double
		char *p = pStart+1;
		while(isdigit((int)*p)) ++p;

		if (*p == '.') { // double
			// check to be done

			m_doubleSymbol = atof(pStart);
			return ObjectType::DoubleValue;

		} else { // int
			if (*p != 0) {
				setError("malformed number");
				return ObjectType::Error;
			}

			m_intSymbol = atoi(pStart);
			return ObjectType::IntValue;
		}
	}

	setError("unknown symbol");

	return ObjectType::Error;
}


Object *Parser::getNodeIdRange(int &minId,int &maxId)
{
	maxId = 0;
	minId = std::numeric_limits<int>::max();

	Object *graphObject = m_objectTree;
	for(; graphObject; graphObject = graphObject->pBrother)
		if (graphObject->key == Key::Graph) break;

	if (!graphObject || graphObject->valueType != ObjectType::ListBegin) {
		return nullptr;
	}

	Object *son = graphObject->pFirstSon;
	for(; son; son = son->pBrother) {
		if (son->key == Key::Node && son->valueType == ObjectType::ListBegin) {
			Object *nodeSon = son->pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->pBrother) {
				if (nodeSon->key == Key::Id &&
					nodeSon->valueType == ObjectType::IntValue) {
					Math::updateMin(minId, nodeSon->intValue);
					Math::updateMax(maxId, nodeSon->intValue);
				}
			}
		}
	}

	return graphObject;
}


string toString(ObjectType type) {
	switch (type) {
	case ObjectType::StringValue:
		return "string";
	case ObjectType::IntValue:
		return "integer";
	case ObjectType::DoubleValue:
		return "real";
	case ObjectType::ListBegin:
		return "list";
	default:
		return "unknown";
	}
}

// Map {int,double,string} to their internal GML representation
template<typename T>
struct GmlType {
	static ObjectType type;
	static T get_attr(Object*);
};
template<>
ObjectType GmlType<int>::type = ObjectType::IntValue;
template<>
int GmlType<int>::get_attr(Object *obj) { return obj->intValue; };
template<>
ObjectType GmlType<double>::type = ObjectType::DoubleValue;
template<>
double GmlType<double>::get_attr(Object *obj) { return obj->doubleValue; };
template<>
ObjectType GmlType<string>::type = ObjectType::StringValue;
template<>
string GmlType<string>::get_attr(Object *obj) { return obj->stringValue; };

// Interface for handlers, so we can abstract away the difference between simple and list handlers
struct IHandler {
	virtual void handle(Object *obj) = 0;
	virtual ~IHandler() = default;
};

// A handler for non-list data
class BasicHandler : public IHandler {
public:
	BasicHandler(GraphAttributes* GA = nullptr) : m_requiredGAs(0), m_GA(GA) { }
	virtual ~BasicHandler() = default;

	BasicHandler& eachInt(std::function<bool(const int&)> handler) { m_handleInt = handler; return *this; }
	BasicHandler& eachDouble(std::function<bool(const double&)> handler) { m_handleDouble = handler; return *this; }
	BasicHandler& eachString(std::function<bool(const string&)> handler) { m_handleString = handler; return *this; }
	BasicHandler& each(std::function<bool(Object*)> handler) { m_handleObject = handler; return *this; }

	BasicHandler& storeInt(long gattr, std::function<void(const int&)> save) { m_requiredGAs = gattr; m_saveInt = save; return *this; }
	BasicHandler& storeDouble(long gattr, std::function<void(const double&)> save) { m_requiredGAs = gattr; m_saveDouble = save; return *this; }
	BasicHandler& storeString(long gattr, std::function<void(const string&)> save) { m_requiredGAs = gattr; m_saveString = save; return *this; }
	BasicHandler& store(long gattr, std::function<void(Object*)> save) { m_requiredGAs = gattr; m_saveObject = save; return *this; }

	void handle(Object *obj) override {
		bool handled = false;
		bool saved = false;
		// Make sure the parsed data is of the expected type
		if (m_handleInt) {
			if (obj->valueType == ObjectType::IntValue) {
				handled = m_handleInt(obj->intValue);
			}
			if (!handled && !m_handleDouble && obj->valueType == ObjectType::DoubleValue) {
				handled = m_handleInt(obj->doubleValue);
				if (handled) {
					Logger::slout(Logger::Level::Minor) << "Expected integer attribute for " << toString(obj->key) << ", found float. Read may have lost precision!";
				}
			}
		}
		if (!handled && m_handleDouble) {
			if (obj->valueType == ObjectType::DoubleValue) {
				handled = m_handleDouble(obj->doubleValue);
			}
			if (!handled && obj->valueType == ObjectType::IntValue) {
				handled = m_handleDouble(obj->intValue);
			}
		}
		if (!handled && m_handleString) {
			if (obj->valueType == ObjectType::StringValue) {
				handled = m_handleString(obj->stringValue);
			}
			if (!handled && obj->valueType == ObjectType::IntValue) {
				handled = m_handleString(to_string(obj->intValue));
			}
			if (!handled && obj->valueType == ObjectType::DoubleValue) {
				handled = m_handleString(to_string(obj->doubleValue));
			}
		}
		if (!handled && m_handleObject) {
			handled = m_handleObject(obj);
		}
		if (m_requiredGAs > 0 && m_GA != nullptr && m_GA->has(m_requiredGAs)) {
			if (m_saveInt) {
				if (obj->valueType == ObjectType::IntValue) {
					m_saveInt(obj->intValue);
					saved = true;
				}
				if (!saved && !m_saveDouble && obj->valueType == ObjectType::DoubleValue) {
					m_saveInt(obj->doubleValue);
					Logger::slout(Logger::Level::Minor) << "Expected integer attribute for " << toString(obj->key) << ", found float. Read may have lost precision!";
					saved = true;
				}
			}
			if (!saved && m_saveDouble) {
				if (obj->valueType == ObjectType::DoubleValue) {
					m_saveDouble(obj->doubleValue);
					saved = true;
				}
				if (!saved && obj->valueType == ObjectType::IntValue) {
					m_saveDouble(obj->intValue);
					saved = true;
				}
			}
			if (!saved && m_saveString) {
				if (obj->valueType == ObjectType::StringValue) {
					m_saveString(obj->stringValue);
					saved = true;
				}
				if (!saved && obj->valueType == ObjectType::IntValue) {
					m_saveString(to_string(obj->intValue));
					saved = true;
				}
				if (!saved && obj->valueType == ObjectType::DoubleValue) {
					m_saveString(to_string(obj->doubleValue));
					saved = true;
				}
			}
			if (!saved && m_saveObject) {
				m_saveObject(obj);
				saved = true;
			}
		}

		// If we had no callbacks registered, applicable, or our general callback returned false,
		// we are simply discarding the read attribute. Inform the user of this.
		if (!handled && !saved) {
			Logger::slout(Logger::Level::Minor) << "Ignoring unused attribute " << toString(obj->key) << "!";
		}
	}

private:
	long m_requiredGAs;
	GraphAttributes* m_GA;
	std::function<bool(const int&)> m_handleInt;
	std::function<bool(const double&)> m_handleDouble;
	std::function<bool(const string&)> m_handleString;
	std::function<bool(Object*)> m_handleObject;
	std::function<void(const int&)> m_saveInt;
	std::function<void(const double&)> m_saveDouble;
	std::function<void(const string&)> m_saveString;
	std::function<void(Object*)> m_saveObject;

};


// Handle attributes in a user-defined way
class CustomHandler : public IHandler {
public:
	CustomHandler() { }

	void each(std::function<void(Object*)> handler) { m_handle = handler; }

	void handle(Object *obj) override {
		m_handle(obj);
	}

private:
	std::function<void(Object*)> m_handle;
};

// Handle GML list attributes. Can use other list handlers recursively.
class ListHandler : public IHandler {
public:
	ListHandler(GraphAttributes* GA = nullptr) : m_GA(GA) { }

	virtual ~ListHandler() {
		for (auto p : m_handlers) {
			delete p.second;
		}
	}

	BasicHandler& attribute(Key key) {
		BasicHandler* handler = new BasicHandler(m_GA);
		m_handlers[key] = handler;
		return *handler;
	}

	ListHandler& listAttribute(Key key) {
		ListHandler* handler = new ListHandler(m_GA);
		m_handlers[key] = handler;
		return *handler;
	}

	CustomHandler& customAttribute(Key key) {
		CustomHandler* handler = new CustomHandler();
		m_handlers[key] = handler;
		return *handler;
	}

	ListHandler& beforeEach(std::function<void()> init) {
		m_beforeEach = init;
		return *this;
	}
	ListHandler& afterEach(std::function<bool()> check) {
		m_afterEach = check;
		return *this;
	}

	void handle(Object *obj) override {
		if (m_beforeEach) m_beforeEach();

		if (obj->valueType == ObjectType::ListBegin) {
			Object *son = obj->pFirstSon;
			for(; son; son = son->pBrother) {
				if (m_handlers.find(son->key) != m_handlers.end()) {
					m_handlers[son->key]->handle(son);
				} else {
					Logger::slout(Logger::Level::Minor) << "Ignoring unused attribute " << toString(son->key) << "!\n";
				}
			}

		} else {
			Logger::slout(Logger::Level::Default) << "Unexpected type for attribute " << toString(obj->key) << ": Found "
				<< toString(obj->valueType) << ", expected " << toString(ObjectType::ListBegin) << ".\n";
		}

		if (m_afterEach && !m_afterEach()) return;
	}

private:
	GraphAttributes* m_GA;
	std::unordered_map<Key, IHandler*> m_handlers;
	std::function<void()> m_beforeEach;
	std::function<bool()> m_afterEach;
};


bool Parser::read(Graph &G)
{
	GraphAttributes GA(G, 0l);
	return read(G, GA);
}

bool Parser::read(Graph &G, GraphAttributes &GA)
{
	// Abort if setup failed.
	if (m_error) return false;

	OGDF_ASSERT(&G == &(GA.constGraph()));

	G.clear();

	int minId = m_mapToNode.low();
	int maxId = m_mapToNode.high();

	DPolyline bends;

	/*
	 * Use the AST parsed from the GML file to read concrete values.
	 * For this, we make use of the IHandler implementations above in order to share common error handling code.
	 * Every attribute gets registered as its own key in its parent attribute's handler. In the end, this structure
	 * gets passed the root `graph` list attribute of our GML tree.
	 * The `.each()` methods are called on every attribute with the given identifying key (which may be a total number
	 * of zero times!). The `.store()` methods call the given lambda to save a graph attribute for the same entries,
	 * if all the necessary graph attributes (as supplied by a bitmask) are enabled.
	 * Passing closure lambdas to these objects, capturing exterior (local) variables, is used to keep track of
	 * information on the currently handled node and edge.
	 */
	ListHandler lh(&GA);

	lh.attribute(Key::Directed).eachInt([&](const int& i) {
		GA.directed() = i > 0;
		return true;
	});

	// These two variables are reset for every read node, read and written by the closures passed to the handler.
	bool nodeIdDef{false};
	node v{nullptr};
	ListHandler &nh = lh.listAttribute(Key::Node).beforeEach([&] {
		nodeIdDef = false;
		v = G.newNode();
	}).afterEach([&] {
		// check if everything required is defined correctly
		if (!nodeIdDef) {
			setError("node id not defined");
			return false;
		}
		return true;
	});

	nh.attribute(Key::Id).eachInt([&](const int& i) {
		m_mapToNode[i] = v;
		nodeIdDef = true;
		return true;
	}).storeInt(GraphAttributes::nodeId, [&](const int& i) { GA.idNode(v) = i; });
	nh.attribute(Key::Template).storeString(GraphAttributes::nodeTemplate, [&](const string& s) { GA.templateNode(v) = s; });
	nh.attribute(Key::Label).storeString(GraphAttributes::nodeLabel, [&](const string& s) { GA.label(v) = s; });
	nh.attribute(Key::Weight).storeInt(GraphAttributes::nodeWeight, [&](const int& i) { GA.weight(v) = i; });
	nh.attribute(Key::Type) \
	  .storeString(GraphAttributes::nodeType, [&](const string& s) { GA.type(v) = toNodeType(s); }) \
	  .storeInt(GraphAttributes::nodeType, [&](const int& i) { GA.type(v) = Graph::NodeType(i); });
	ListHandler &ngh = nh.listAttribute(Key::Graphics);
	ngh.attribute(Key::X).storeDouble(GraphAttributes::nodeGraphics, [&](const double& d) { GA.x(v) = d; });
	ngh.attribute(Key::Y).storeDouble(GraphAttributes::nodeGraphics, [&](const double& d) { GA.y(v) = d; });
	ngh.attribute(Key::Z).storeDouble(GraphAttributes::nodeGraphics | GraphAttributes::threeD, [&](const double& d) { GA.z(v) = d; });
	ngh.attribute(Key::W).storeDouble(GraphAttributes::nodeGraphics, [&](const double& d) { GA.width(v) = d; });
	ngh.attribute(Key::H).storeDouble(GraphAttributes::nodeGraphics, [&](const double& d) { GA.height(v) = d; });
	ngh.attribute(Key::Fill).storeString(GraphAttributes::nodeStyle, [&](const string& s) { GA.fillColor(v) = s; });
	ngh.attribute(Key::FillBg).storeString(GraphAttributes::nodeStyle, [&](const string& s) { GA.fillBgColor(v) = s; });
	ngh.attribute(Key::Outline).storeString(GraphAttributes::nodeStyle, [&](const string& s) { GA.strokeColor(v) = s; });
	ngh.attribute(Key::LineWidth).storeDouble(GraphAttributes::nodeStyle, [&](const double& d) { GA.strokeWidth(v) = d; });
	ngh.attribute(Key::Type).storeString(GraphAttributes::nodeGraphics, [&](const string& s) { GA.shape(v) = fromString<Shape>(s); });
	ngh.attribute(Key::Pattern).storeString(GraphAttributes::nodeStyle, [&](const string& s) { GA.fillPattern(v) = fromString<FillPattern>(s); });
	ngh.attribute(Key::Stipple).storeString(GraphAttributes::nodeStyle, [&](const string& s) { GA.strokeType(v) = fromString<StrokeType>(s); });
	ListHandler &nglh = ngh.listAttribute(Key::Label);
	nglh.attribute(Key::X).storeDouble(GraphAttributes::nodeLabelPosition, [&] (const double& d) { GA.xLabel(v) = d; });
	nglh.attribute(Key::Y).storeDouble(GraphAttributes::nodeLabelPosition, [&] (const double& d) { GA.yLabel(v) = d; });
	nglh.attribute(Key::Z).storeDouble(GraphAttributes::nodeLabelPosition | GraphAttributes::threeD, [&] (const double& d) { GA.zLabel(v) = d; });


	edge e{nullptr};
	bool sourceIdDef{false}, targetIdDef{false};
	ListHandler &eh = lh.listAttribute(Key::Edge).beforeEach([&] {
		// Start off by making our edge a selfloop on the first node.
		// During reading, we update its source and target to what the file defines.
		e = G.newEdge(G.firstNode(), G.firstNode());
		sourceIdDef = targetIdDef = false;
	}).afterEach([&] {
		// check if everything required is defined correctly
		if (!sourceIdDef) {
			setError("edge source id not defined");
			return false;
		}
		if (!targetIdDef) {
			setError("edge target id not defined");
			return false;
		}
		return true;
	});

	eh.attribute(Key::Source).eachInt([&](const int& i) {
		if (sourceIdDef) {
			setError("two sources for one edge");
			return false;
		}
		if (i < minId || maxId < i) {
			setError("source id out of range");
			return false;
		}
		OGDF_ASSERT(m_mapToNode[i] != nullptr);
		G.moveSource(e, m_mapToNode[i]);
		sourceIdDef = true;
		return true;
	});
	eh.attribute(Key::Target).eachInt([&](const int& i) {
		if(targetIdDef) {
			setError("two targets for one edge");
			return false;
		}
		if (i < minId || maxId < i) {
			setError("target id out of range");
			return false;
		}
		OGDF_ASSERT(m_mapToNode[i] != nullptr);
		G.moveTarget(e, m_mapToNode[i]);
		targetIdDef = true;
		return true;
	});
	eh.attribute(Key::SubGraph).storeInt(GraphAttributes::edgeSubGraphs, [&](const int& i) { GA.addSubGraph(e, i); });
	eh.attribute(Key::Label).storeString(GraphAttributes::edgeLabel, [&](const string& s) { GA.label(e) = s; });
	eh.attribute(Key::Weight).storeDouble(GraphAttributes::edgeDoubleWeight, [&](const double& d) { GA.doubleWeight(e) = d; });
	eh.attribute(Key::EdgeIntWeight).storeInt(GraphAttributes::edgeIntWeight, [&](const int& i) { GA.intWeight(e) = i; });
	ListHandler &egh = eh.listAttribute(Key::Graphics);
	// Use a custom parser for the bend list, as a list-based one would require more intermediary variables.
	egh.attribute(Key::Bends).store(GraphAttributes::edgeGraphics, [&](const Object *list) {
		readLineAttribute(list->pFirstSon, bends);
		EpsilonTest eps;
		DPoint src(GA.x(e->source()), GA.y(e->source()));
		while (eps.equal(bends.front().distance(src), 0.0)) {
			bends.popFront();
		}
		DPoint tgt(GA.x(e->target()), GA.y(e->target()));
		while (eps.equal(bends.back().distance(tgt), 0.0)) {
			bends.popBack();
		}
		GA.bends(e) = bends;
		return true;
	});
	egh.attribute(Key::Arrow).storeString(GraphAttributes::edgeArrow, [&](const string& s) { GA.arrowType(e) = toArrow(s); });
	egh.attribute(Key::Fill).storeString(GraphAttributes::edgeStyle, [&](const string& s) { GA.strokeColor(e) = s; });
	egh.attribute(Key::Stipple).storeString(GraphAttributes::edgeStyle, [&](const string& s) { GA.strokeType(e) = fromString<StrokeType>(s); });
	egh.attribute(Key::LineWidth).storeDouble(GraphAttributes::edgeStyle, [&](const double& d) { GA.strokeWidth(e) = d; });
	eh.attribute(Key::Generalization).storeInt(GraphAttributes::edgeType, [&](const int& i) { GA.type(e) = Graph::EdgeType(i); });

	// Run the handler on the `graph` key.
	lh.handle(m_graphObject);

	return !m_error;
}


//the clustergraph has to be initialized on G!!,
//no clusters other then root cluster may exist, which holds all nodes
bool Parser::readCluster(Graph &G, ClusterGraph &CG, ClusterGraphAttributes *ACG)
{
	if (m_error) return false;

	OGDF_ASSERT(&CG.constGraph() == &G);

	// now we need the cluster object
	Object *rootObject;
	for(rootObject = m_objectTree;
	    rootObject && rootObject->key != Key::Root;
	    rootObject = rootObject->pBrother);

	// we have to check if the file does really contain clusters
	// otherwise, rootcluster will suffice
	if (rootObject == nullptr) {
		return true;
	}
	if (rootObject->key != Key::Root) {
		setError("missing rootcluster key");
		return false;
	}

	return rootObject->valueType == ObjectType::ListBegin && recursiveClusterRead(rootObject, CG, CG.rootCluster(), ACG);
}


//recursively read cluster subtree information
bool Parser::recursiveClusterRead(Object* clusterObject,
								ClusterGraph& CG,
								cluster c,
								ClusterGraphAttributes* ACG)
{

	ListHandler lh(ACG);
	bool clusterIdDef{false};

	lh.customAttribute(Key::Cluster).each([&](Object* subClusterObject) {
		cluster subCluster = CG.newCluster(c);
		recursiveClusterRead(subClusterObject, CG, subCluster, ACG);
	});
	lh.attribute(Key::Id).eachInt([&](const int&) {
		clusterIdDef = true;
		return true;
	});
	lh.attribute(Key::Vertex).eachString([&](const string& s) {
		string vIDString = s;
		//we only allow a vertex id as string identification
		if ((vIDString[0] != 'v') &&
			(!isdigit((int)vIDString[0])))return false; //do not allow labels
		if (!isdigit((int)vIDString[0])) //should check prefix?
			vIDString[0] = '0'; //leading zero to allow conversion
		int vID = std::stoi(vIDString);

		OGDF_ASSERT(m_mapToNode[vID] != nullptr);

		// all nodes are already assigned to root
		CG.reassignNode(m_mapToNode[vID], c);
		return true;
	});

	lh.attribute(Key::Label).storeString(ClusterGraphAttributes::clusterLabel, [&](const string& s) { ACG->label(c) = s; });
	lh.attribute(Key::Template).storeString(ClusterGraphAttributes::clusterTemplate, [&](const string& s) { ACG->templateCluster(c) = s; });

	ListHandler &gh = lh.listAttribute(Key::Graphics);
	gh.attribute(Key::X).storeDouble(ClusterGraphAttributes::clusterGraphics, [&](const double& d) { ACG->x(c) = d; });
	gh.attribute(Key::Y).storeDouble(ClusterGraphAttributes::clusterGraphics, [&](const double& d) { ACG->y(c) = d; });
	gh.attribute(Key::Width).storeDouble(ClusterGraphAttributes::clusterGraphics, [&](const double& d) { ACG->width(c) = d; });
	gh.attribute(Key::Height).storeDouble(ClusterGraphAttributes::clusterGraphics, [&](const double& d) { ACG->height(c) = d; });
	gh.attribute(Key::Fill).storeString(ClusterGraphAttributes::clusterStyle, [&](const string& s) { ACG->fillColor(c) = s; });
	gh.attribute(Key::Pattern).storeString(ClusterGraphAttributes::clusterStyle, [&](const string& s) { ACG->fillPattern(c) = fromString<FillPattern>(s); });
	gh.attribute(Key::Color).storeString(ClusterGraphAttributes::clusterStyle, [&](const string& s) { ACG->strokeColor(c) = s; });
	gh.attribute(Key::LineWidth).storeDouble(ClusterGraphAttributes::clusterStyle, [&](const double& d) { ACG->strokeWidth(c) = d; });
	gh.attribute(Key::Stipple).storeString(ClusterGraphAttributes::clusterStyle, [&](const string& s) { ACG->strokeType(c) = fromString<StrokeType>(s); });
	gh.attribute(Key::FillBg).storeString(ClusterGraphAttributes::clusterStyle, [&](const string& s) { ACG->fillBgColor(c) = s; });

	lh.handle(clusterObject);

	if (!clusterIdDef && c != CG.rootCluster()) {
		setError("cluster id not defined");
		return false;
	}


	return true;
}

void Parser::readLineAttribute(Object *object, DPolyline &dpl)
{
	dpl.clear();
	for(; object; object = object->pBrother) {
		if (object->key == Key::Point &&
			object->valueType == ObjectType::ListBegin)
		{
			DPoint dp;

			Object *pointObject = object->pFirstSon;
			for (; pointObject; pointObject = pointObject->pBrother) {
				if (pointObject->valueType != ObjectType::DoubleValue) continue;
				if (pointObject->key == Key::X)
					dp.m_x = pointObject->doubleValue;
				else if (pointObject->key == Key::Y)
					dp.m_y = pointObject->doubleValue;
			}

			dpl.pushBack(dp);
		}
	}
}


void Parser::setError(const char *errorString, Logger::Level level)
{
	Logger::slout(level) << errorString;
	m_error = true;
}

}

}
