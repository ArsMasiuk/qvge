/** \file
 * \brief Declarations for DOT Parser
 *
 * \author Łukasz Hanuszczak
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

#include <ogdf/fileformats/DotParser.h>
#include <ogdf/fileformats/Utils.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {

namespace dot {

/**
 * Frees a singly linked list without using recursion.
 * Should be called from within list destructors.
 *
 * @tparam T list element type
 * @param list A pointer to the list to be deleted.
 */
template<typename T>
static void destroyList(T *list) {
	delete list->head;
	for (T *next, *element = list->tail; element != nullptr; element = next) {
		next = element->tail;
		element->tail = nullptr; // don't recurse deeply on delete
		delete element;
	}
}

Ast::Graph::Graph(
	const bool &paramStrict,
	const bool &dir,
	std::string *idString,
	StmtList *statementList)
: strict(paramStrict), directed(dir), id(idString), statements(statementList)
{
}


Ast::Graph::~Graph()
{
	delete id;
	delete statements;
}


Ast::StmtList::StmtList(
	Stmt *headSTMT,
	StmtList *tailStatementList)
: head(headSTMT), tail(tailStatementList)
{
}


Ast::StmtList::~StmtList()
{
	destroyList(this);
}


Ast::Stmt::~Stmt()
{
}


Ast::NodeStmt::NodeStmt(
	NodeId *nodeID,
	AttrList *attrList)
: nodeId(nodeID), attrs(attrList)
{
}


Ast::NodeStmt::~NodeStmt()
{
	delete nodeId;
	delete attrs;
}


Ast::EdgeStmt::EdgeStmt(
	EdgeLhs *edgeLHS,
	EdgeRhs *edgeRHS,
	AttrList *attrList)
: lhs(edgeLHS), rhs(edgeRHS), attrs(attrList)
{
}


Ast::EdgeStmt::~EdgeStmt()
{
	delete lhs;
	delete rhs;
	delete attrs;
}


Ast::AsgnStmt::AsgnStmt(
	const std::string &lhsString,
	const std::string &rhsString)
: lhs(lhsString), rhs(rhsString)
{
}


Ast::AsgnStmt::~AsgnStmt()
{
}


Ast::AttrStmt::AttrStmt(
	const Type &paramType,
	AttrList *attrList)
: type(paramType), attrs(attrList)
{
}


Ast::AttrStmt::~AttrStmt()
{
	delete attrs;
}


Ast::Subgraph::Subgraph(
	std::string *idString,
	StmtList *statementList)
: id(idString), statements(statementList)
{
}


Ast::Subgraph::~Subgraph()
{
	delete id;
	delete statements;
}


Ast::EdgeLhs::~EdgeLhs()
{
}


Ast::EdgeRhs::EdgeRhs(
	EdgeLhs *headEdgeLHS,
	EdgeRhs *tailEdgeRHS)
: head(headEdgeLHS), tail(tailEdgeRHS)
{
}


Ast::EdgeRhs::~EdgeRhs()
{
	destroyList(this);
}


Ast::NodeId::NodeId(
	const std::string &idString,
	Port *paramPort)
: id(idString), port(paramPort)
{
}


Ast::NodeId::~NodeId()
{
	delete port;
}


Ast::Port::Port(
	std::string *idString,
	CompassPt *compassPT)
: id(idString), compassPt(compassPT)
{
}


Ast::Port::~Port()
{
	delete id;
	delete compassPt;
}


Ast::CompassPt::CompassPt(
	const Type &paramType)
: type(paramType)
{
}


Ast::CompassPt::~CompassPt()
{
}


Ast::AttrList::AttrList(
	AList *headAList,
	AttrList *tailAttrList)
: head(headAList), tail(tailAttrList)
{
}


Ast::AttrList::~AttrList()
{
	destroyList(this);
}


Ast::AList::AList(
	AsgnStmt *headAsgnStmt,
	AList *tailAList)
: head(headAsgnStmt), tail(tailAList)
{
}


Ast::AList::~AList()
{
	destroyList(this);
}


Ast::Ast(const Tokens &tokens)
: m_tokens(tokens), m_tend(m_tokens.end()), m_graph(nullptr)
{
}


Ast::~Ast()
{
	delete m_graph;
}


bool Ast::build()
{
	Iterator it = m_tokens.begin();
	delete m_graph;
	m_graph = parseGraph(it, it);
	return m_graph != nullptr;
}


Ast::Graph *Ast::root() const
{
	return m_graph;
}


Ast::EdgeStmt *Ast::parseEdgeStmt(
	Iterator curr, Iterator &rest)
{
	EdgeLhs *lhs;
	if(!((lhs = parseNodeId(curr, curr)) ||
	     (lhs = parseSubgraph(curr, curr)))) {
		return nullptr;
	}

	EdgeRhs *rhs = parseEdgeRhs(curr, curr);
	if(!rhs) {
		delete lhs;
		return nullptr;
	}

	AttrList *attrs = parseAttrList(curr, curr);

	rest = curr;
	return new EdgeStmt(lhs, rhs, attrs);
}


Ast::EdgeRhs *Ast::parseEdgeRhs(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend || (curr->type != Token::Type::edgeOpDirected &&
	                      curr->type != Token::Type::edgeOpUndirected)) {
		return nullptr;
	}
	curr++;

	EdgeLhs *head;
	if(!((head = parseSubgraph(curr, curr)) ||
	     (head = parseNodeId(curr, curr)))) {
		return nullptr;
	}

	EdgeRhs *tail = parseEdgeRhs(curr, curr);

	rest = curr;
	return new EdgeRhs(head, tail);
}


Ast::NodeStmt *Ast::parseNodeStmt(
	Iterator curr, Iterator &rest)
{
	NodeId *nodeId = parseNodeId(curr, curr);
	if(!nodeId) {
		return nullptr;
	}

	AttrList *attrs = parseAttrList(curr, curr);

	rest = curr;
	return new NodeStmt(nodeId, attrs);
}


Ast::NodeId *Ast::parseNodeId(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend || curr->type != Token::Type::identifier) {
		return nullptr;
	}
	std::string id = *(curr->value);
	curr++;

	Port *port = parsePort(curr, curr);

	rest = curr;
	return new NodeId(id, port);
}


Ast::CompassPt *Ast::parseCompassPt(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend || curr->type != Token::Type::identifier) {
		return nullptr;
	}
	const std::string &str = *(curr->value);
	curr++;
	if(str == "n") {
		rest = curr;
		return new CompassPt(CompassPt::Type::n);
	}
	if(str == "ne") {
		rest = curr;
		return new CompassPt(CompassPt::Type::ne);
	}
	if(str == "e") {
		rest = curr;
		return new CompassPt(CompassPt::Type::e);
	}
	if(str == "se") {
		rest = curr;
		return new CompassPt(CompassPt::Type::se);
	}
	if(str == "s") {
		rest = curr;
		return new CompassPt(CompassPt::Type::s);
	}
	if(str == "sw") {
		rest = curr;
		return new CompassPt(CompassPt::Type::sw);
	}
	if(str == "w") {
		rest = curr;
		return new CompassPt(CompassPt::Type::w);
	}
	if(str == "nw") {
		rest = curr;
		return new CompassPt(CompassPt::Type::nw);
	}
	if(str == "c") {
		rest = curr;
		return new CompassPt(CompassPt::Type::c);
	}
	if(str == "_") {
		rest = curr;
		return new CompassPt(CompassPt::Type::wildcard);
	}
	return nullptr;
}


Ast::Port *Ast::parsePort(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend || curr->type != Token::Type::colon) {
		return nullptr;
	}
	curr++;

	CompassPt *compass = parseCompassPt(curr, curr);
	if(compass) {
		rest = curr;
		return new Port(nullptr, compass);
	}

	std::string *id = curr->value;
	curr++;

	if(curr != m_tend && curr->type == Token::Type::colon) {
		curr++;

		compass = parseCompassPt(curr, curr);
		if(compass) {
			rest = curr;
			return new Port(id, compass);
		}

		// Compass parsing not succeeded, "put back" the colon.
		curr--;
	}

	rest = curr;
	return new Port(id, nullptr);
}


Ast::AttrStmt *Ast::parseAttrStmt(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend) {
		return nullptr;
	}

	AttrStmt::Type type;
	switch(curr->type) {
	case Token::Type::graph:
		type = AttrStmt::Type::graph;
		break;
	case Token::Type::node:
		type = AttrStmt::Type::node;
		break;
	case Token::Type::edge:
		type = AttrStmt::Type::edge;
		break;
	default:
		return nullptr;
	}
	curr++;

	AttrList *attrs = parseAttrList(curr, curr);
	if(!attrs) {
		return nullptr;
	}

	rest = curr;
	return new AttrStmt(type, attrs);
}


Ast::AsgnStmt *Ast::parseAsgnStmt(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend || curr->type != Token::Type::identifier) {
		return nullptr;
	}
	std::string lhs = *(curr->value);
	curr++;

	if(curr == m_tend || curr->type != Token::Type::assignment) {
		return nullptr;
	}
	curr++;

	if(curr == m_tend || curr->type != Token::Type::identifier) {
		return nullptr;
	}
	std::string rhs = *(curr->value);
	curr++;

	rest = curr;
	return new AsgnStmt(lhs, rhs);
}


Ast::Subgraph *Ast::parseSubgraph(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend) {
		return nullptr;
	}

	// Optional "subgraph" keyword and optional identifier.
	std::string *id = nullptr;
	if(curr->type == Token::Type::subgraph) {
		curr++;
		if(curr == m_tend) {
			return nullptr;
		}
		if(curr->type == Token::Type::identifier) {
			id = new std::string(*(curr->value));
			curr++;
		}
	}

	if(curr == m_tend || curr->type != Token::Type::leftBrace) {
		delete id;
		return nullptr;
	}
	curr++;

	StmtList *stmts = parseStmtList(curr, curr);

	if(curr == m_tend || curr->type != Token::Type::rightBrace) {
		delete id;
		delete stmts;
		return nullptr;
	}
	curr++;

	rest = curr;
	return new Subgraph(id, stmts);
}


Ast::Stmt *Ast::parseStmt(
	Iterator curr, Iterator &rest)
{
	Stmt *stmt;
	if((stmt = parseEdgeStmt(curr, curr)) ||
	   (stmt = parseAttrStmt(curr, curr)) ||
	   (stmt = parseAsgnStmt(curr, curr)) ||
	   (stmt = parseNodeStmt(curr, curr)) ||
	   (stmt = parseSubgraph(curr, curr))) {
		rest = curr;
		return stmt;
	}

	return nullptr;
}


Ast::StmtList *Ast::parseStmtList(
	Iterator curr, Iterator &rest)
{
	if (curr == m_tend) {
		return nullptr;
	}

	ArrayBuffer<Stmt*> stmts;
	Stmt *head;

	// Collect statements iteratively (a recursive implementation would cause
	// stack overflows).
	do {
		head = parseStmt(curr, curr);

		if (head != nullptr) {
			stmts.push(head);

			// Optional semicolon.
			if (curr != m_tend && curr->type == Token::Type::semicolon) {
				curr++;
			}
		}
	} while (curr != m_tend && head != nullptr);

	// Build StmtList from statements.
	StmtList *stmtList = nullptr;
	while (!stmts.empty()) {
		stmtList = new StmtList(stmts.popRet(), stmtList);
	}

	rest = curr;
	return stmtList;
}


Ast::Graph *Ast::parseGraph(
	Iterator curr, Iterator &rest)
{
	if(curr == m_tend) {
		return nullptr;
	}

	bool strict = false;
	bool directed = false;
	std::string *id = nullptr;

	if(curr->type == dot::Token::Type::strict) {
		strict = true;
		curr++;
	}

	if(curr == m_tend) {
		return nullptr;
	}

	switch(curr->type) {
	case Token::Type::graph:
		directed = false;
		break;
	case Token::Type::digraph:
		directed = true;
		break;
	default:
		GraphIO::logger.lout() << "Unexpected token \""
		          << Token::toString(curr->type)
		          << "\" at "
		          << curr->row << ", " << curr->column << "." << std::endl;
		return nullptr;
	}
	curr++;

	if(curr == m_tend) {
		return nullptr;
	}

	if(curr->type == Token::Type::identifier) {
		id = new std::string(*(curr->value));
		curr++;
	}

	if(curr == m_tend || curr->type != Token::Type::leftBrace) {
#if 0
		GraphIO::logger.lout() << "Expected \""
							   << Token::toString(Token::Type::leftBrace)
							   << ", found \"" << Token::toString(curr->type)
							   << "\" at " << curr->row << ", " << curr->column
							   << ".\n";
#endif
		delete id;
		return nullptr;
	}
	curr++;

	StmtList *statements = parseStmtList(curr, curr);

	if(curr == m_tend || curr->type != Token::Type::rightBrace) {
		GraphIO::logger.lout() << "Expected \""
		          << Token::toString(Token::Type::rightBrace)
		          << ", found \""
		          << Token::toString(curr->type)
		          << "\" at "
		          << curr->row << ", " << curr->column << "." << std::endl;
		delete id;
		delete statements;
		return nullptr;
	}
	curr++;

	rest = curr;
	return new Graph(strict, directed, id, statements);
}


Ast::AttrList *Ast::parseAttrList(
	Iterator curr, Iterator &rest)
{

	ArrayBuffer<AList*> subLists;
	bool doContinue = false;

	do {
		doContinue = curr != m_tend && curr->type == Token::Type::leftBracket;
		AList *head = nullptr;

		if(doContinue) {
			curr++;
			head = parseAList(curr, curr);

			doContinue = curr != m_tend && curr->type == Token::Type::rightBracket;
		}

		if(doContinue) {
			curr++;
			subLists.push(head);
			rest = curr;
		} else {
			delete head;
		}
	} while(doContinue);

	AttrList *result = nullptr;

	while(!subLists.empty()) {
		result = new AttrList(subLists.popRet(), result);
	}

	return result;
}


Ast::AList *Ast::parseAList(
	Iterator curr, Iterator &rest)
{
	ArrayBuffer<AsgnStmt*> statements;
	AsgnStmt *head = nullptr;

	do {
		head = parseAsgnStmt(curr, curr);

		if(head != nullptr) {
			// Optional comma.
			if(curr != m_tend && curr->type == Token::Type::comma) {
				curr++;
			}

			statements.push(head);
			rest = curr;
		}
	} while(head != nullptr);

	AList *result = nullptr;

	while(!statements.empty()) {
		result = new AList(statements.popRet(), result);
	}

	return result;
}


static bool readBends(
	const std::string &str,
	DPolyline &polyline)
{
	// First, just trim every unnecessary charater - we don't treat DOT's
	// spline as spline but just set of bending points. One can always
	// implement B-splines and then generate bending points.
	std::string fixed(str);
	for(auto &elem : fixed) {
		if(elem == ',' || elem == ';' ||
		   elem == 'e' || elem == 'p')
		{
			elem = ' ';
		}
	}

	std::istringstream is(fixed);

	double x, y;
	polyline.clear();
	while(is >> x && is >> y) {
		polyline.pushBack(DPoint(x, y));
	}

	return true;
}


static bool readAttribute(
	GraphAttributes &GA, const node &v,
	const Ast::AsgnStmt &stmt)
{
	const long flags = GA.attributes();

	std::istringstream ss(stmt.rhs);
	switch(toAttribute(stmt.lhs)) {
	case Attribute::Id:
		if(flags & GraphAttributes::nodeId) {
			ss >> GA.idNode(v);
		}
		break;
	case Attribute::Label:
		if(flags & GraphAttributes::nodeLabel) {
			GA.label(v) = stmt.rhs;
		}
		break;
	case Attribute::Template:
		if(flags & GraphAttributes::nodeTemplate) {
			GA.templateNode(v) = stmt.rhs;
		}
		break;
	case Attribute::Width:
		if(flags & GraphAttributes::nodeGraphics) {
			// sscanf(stmt.rhs.c_str(), "%lf", &GA.width(v));
			ss >> GA.width(v);
		}
		break;
	case Attribute::Height:
		if(flags & GraphAttributes::nodeGraphics) {
			// sscanf(stmt.rhs.c_str(), "%lf", &GA.height(v));
			ss >> GA.height(v);
		}
		break;
	case Attribute::Weight:
		if (flags & GraphAttributes::nodeWeight) {
			ss >> GA.weight(v);
		}
		break;
	case Attribute::Shape:
		if(flags & GraphAttributes::nodeGraphics) {
			GA.shape(v) = toShape(stmt.rhs);
		}
		break;
	case Attribute::Position:
		if(flags & GraphAttributes::nodeGraphics) {
			// sscanf(stmt.rhs.c_str(), "%lf,%lf", &GA.x(v), &GA.y(v));
			ss >> GA.x(v) >> TokenIgnorer(',') >> GA.y(v);
			if(flags & GraphAttributes::threeD) {
				ss >> TokenIgnorer(',') >> GA.z(v);
			}
		}
		break;
	case Attribute::LabelPosition:
		if(flags & GraphAttributes::nodeLabelPosition) {
			ss >> GA.xLabel(v) >> TokenIgnorer(',') >> GA.yLabel(v);
			if(flags & GraphAttributes::threeD) {
				ss >> TokenIgnorer(',') >> GA.zLabel(v);
			}
		}
		break;
	case Attribute::Stroke:
		if(flags & GraphAttributes::nodeStyle) {
			GA.strokeColor(v) = stmt.rhs;
			// TODO: color literals.
		}
		break;
	case Attribute::StrokeWidth:
		if(flags & GraphAttributes::nodeStyle) {
			ss >> GA.strokeWidth(v);
		}
		break;
	case Attribute::FillBackground:
		if(flags & GraphAttributes::nodeStyle) {
			GA.fillBgColor(v) = stmt.rhs;
		}
		break;
	case Attribute::FillPattern:
		if(flags & GraphAttributes::nodeStyle) {
			string help;
			ss >> help;
			GA.fillPattern(v) = fromString<FillPattern>(help);
		}
		break;
	case Attribute::Type:
		if(flags & GraphAttributes::nodeType) {
			int help;
			ss >> help;
			GA.type(v) = Graph::NodeType(help);
		}
		break;
	case Attribute::StrokeType:
		if(flags & GraphAttributes::nodeStyle) {
			string help;
			ss >> help;
			GA.strokeType(v) = fromString<StrokeType>(help);
		}
		break;
	case Attribute::Fill:
		if(flags & GraphAttributes::nodeStyle) {
			GA.fillColor(v) = stmt.rhs;
			// TODO: color literals.
		}
		break;
	default:
		GraphIO::logger.lout(Logger::Level::Minor) << "Attribute \"" << stmt.lhs
		          << "\" is  not supported by node or incorrect. Ignoring." << std::endl;
	}

	return true;
}


static bool readAttribute(
	GraphAttributes &GA, const edge &e,
	const Ast::AsgnStmt &stmt)
{
	const long flags = GA.attributes();

	std::istringstream ss(stmt.rhs);
	switch(toAttribute(stmt.lhs)) {
	case Attribute::Label:
		if(flags & GraphAttributes::edgeLabel) {
			GA.label(e) = stmt.rhs;
		}
		break;
	case Attribute::Weight:
		if(flags & GraphAttributes::edgeDoubleWeight) {
			ss >> GA.doubleWeight(e);
		} else if (flags & GraphAttributes::edgeIntWeight) {
			ss >> GA.intWeight(e);
		}
		break;
	case Attribute::Position:
		if(flags & GraphAttributes::edgeGraphics) {
			readBends(stmt.rhs, GA.bends(e));
		}
		break;
	case Attribute::Stroke:
		if(flags & GraphAttributes::edgeStyle) {
			GA.strokeColor(e) = stmt.rhs;
			// TODO: color literals.
		}
		break;
	case Attribute::StrokeWidth:
		if(flags & GraphAttributes::edgeStyle) {
			ss >> GA.strokeWidth(e);
		}
		break;
	case Attribute::StrokeType:
		if(flags & GraphAttributes::edgeStyle) {
			string help;
			ss >> help;
			GA.strokeType(e) = fromString<StrokeType>(help);
		}
		break;
	case Attribute::Type:
		if(flags & GraphAttributes::edgeType) {
			string help;
			ss >> help;
			GA.type(e) = dot::toEdgeType(help);
		}
		break;
	case Attribute::Arrow:
		if(flags & GraphAttributes::edgeArrow) {
			int help;
			ss >> help;
			GA.arrowType(e) = EdgeArrow(help);
		}
		break;
	case Attribute::Dir:
		if (flags & GraphAttributes::edgeArrow) {
			GA.arrowType(e) = dot::toArrow(stmt.rhs);
		}
		break;
	case Attribute::SubGraphs:
		if (flags & GraphAttributes::edgeSubGraphs) {
			int sg;
			while(ss >> sg) {
				GA.addSubGraph(e, sg);
			}
		}
		break;
	default:
		GraphIO::logger.lout(Logger::Level::Minor) << "Attribute \"" << stmt.lhs << "\" is not supported by edge or incorrect. Ignoring." << std::endl;
	}

	return true;
}


static bool readAttribute(
	ClusterGraphAttributes &CA, const cluster &c,
	const Ast::AsgnStmt &stmt)
{
	const long flags = CA.attributes();

	std::istringstream ss(stmt.rhs);
	switch(toAttribute(stmt.lhs)) {
	case Attribute::Label:
		if (flags & ClusterGraphAttributes::clusterLabel) {
			CA.label(c) = stmt.rhs;
		}
		break;
	case Attribute::Template:
		if (flags & ClusterGraphAttributes::clusterTemplate) {
			CA.templateCluster(c) = stmt.rhs;
		}
		break;
	case Attribute::Position:
		if (flags & ClusterGraphAttributes::clusterGraphics) {
			ss >> CA.x(c) >> TokenIgnorer(',') >> CA.y(c);
		}
		break;
	case Attribute::Width:
		if (flags & ClusterGraphAttributes::clusterGraphics) {
			ss >> CA.width(c);
		}
		break;
	case Attribute::Height:
		if (flags & ClusterGraphAttributes::clusterGraphics) {
			ss >> CA.height(c);
		}
		break;
	case Attribute::StrokeType:
		if (flags & ClusterGraphAttributes::clusterStyle) {
			string help;
			ss >> help;
			CA.strokeType(c) = fromString<StrokeType>(help);
		}
		break;
	case Attribute::Fill:
		if (flags & ClusterGraphAttributes::clusterStyle) {
			CA.fillColor(c) = stmt.rhs;
		}
		break;
	case Attribute::Stroke:
		if (flags & ClusterGraphAttributes::clusterStyle) {
			CA.strokeColor(c) = stmt.rhs;
		}
		break;
	case Attribute::StrokeWidth:
		if (flags & ClusterGraphAttributes::clusterStyle) {
			ss >> CA.strokeWidth(c);
		}
		break;
	case Attribute::FillPattern:
		if (flags & ClusterGraphAttributes::clusterStyle) {
			string help;
			ss >> help;
			CA.fillPattern(c) = fromString<FillPattern>(help);
		}
		break;
	case Attribute::FillBackground:
		if (flags & ClusterGraphAttributes::clusterStyle) {
			CA.fillBgColor(c) = stmt.rhs;
		}
		break;
	default:
		GraphIO::logger.lout(Logger::Level::Minor) << "Attribute \"" << stmt.lhs
	              << "\" is not supported by cluster or incorrect. Ignoring." << std::endl;
	}
	return true;
}


template <typename G, typename T>
static inline bool readAttributes(
	G &GA, T elem,
	Ast::AttrList *attrs)
{
	for(; attrs; attrs = attrs->tail) {
		for(Ast::AList *alist = attrs->head; alist; alist = alist->tail) {
			if(!readAttribute(GA, elem, *(alist->head))) {
				return false;
			}
		}
	}

	return true;
}


template <typename G, typename T>
static inline bool readAttributes(
	G &GA, T elem,
	const std::vector<Ast::AttrList *> &defaults)
{
	for(Ast::AttrList *p : defaults)
	{
		if(!readAttributes(GA, elem, p)) {
			return false;
		}
	}

	return true;
}


static inline bool readStatements(
	Parser &P,
	Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes *CA,
	const SubgraphData &data,
	Ast::StmtList *stmts)
{
	for(; stmts; stmts = stmts->tail) {
		if(!stmts->head->read(P, G, GA, C, CA, data)) {
			return false;
		}
	}

	return true;
}


bool Ast::Graph::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes *CA)
{
	if(GA) {
		GA->directed() = directed;
	}

	std::set<node> subgraphNodes;
	std::vector<AttrList *> nodeDefaults, edgeDefaults;
	return readStatements(
		P, G, GA, C, CA,
		SubgraphData(
			// Root cluster.
			C ? C->rootCluster() : nullptr,
			nodeDefaults, edgeDefaults, subgraphNodes),
		statements);
}


bool Ast::NodeStmt::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes* /*unused parameter*/,
	const SubgraphData &data)
{
	const node v = P.requestNode(G, GA, C, data, nodeId->id);
	data.nodes.insert(v);
	return GA ? readAttributes(*GA, v, attrs) : true;
}


static inline bool cross(
	ogdf::Graph &G, GraphAttributes *GA,
	ClusterGraph* /*unused parameter*/, ClusterGraphAttributes* /*unused parameter*/,
	const std::vector<Ast::AttrList *> &defaults, Ast::AttrList *attrs,
	const std::set<ogdf::node> &lnodes, const std::set<ogdf::node> &rnodes)
{
	for(node vl : lnodes)
	{
		for(node vr : rnodes)
		{
			const edge e = G.newEdge(vl, vr);
			if(GA && !(readAttributes(*GA, e, defaults) &&
			           readAttributes(*GA, e, attrs))) {
				return false;
			}
		}
	}

	return true;
}

bool Ast::EdgeStmt::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes *CA,
	const SubgraphData &data)
{
	Ast::EdgeLhs *edgeLhs = this->lhs;

	std::set<node> lnodes;
	edgeLhs->read(P, G, GA, C, CA, data.withNodes(lnodes));

	for(Ast::EdgeRhs *edgeRhs = this->rhs; edgeRhs; edgeRhs = edgeRhs->tail) {
		std::set<node> rnodes;
		edgeRhs->head->read(P, G, GA, C, CA, data.withNodes(rnodes));

		if(!cross(G, GA, C, CA, data.edgeDefaults, attrs, lnodes, rnodes)) {
			return false;
		}

		// Append left side nodes to the result and make right node left ones.
		data.nodes.insert(lnodes.begin(), lnodes.end());
		std::swap(lnodes, rnodes);
		edgeLhs = edgeRhs->head;
	}

	return true;
}


bool Ast::AsgnStmt::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes* /*unused parameter*/,
	ClusterGraph* /*unused parameter*/, ClusterGraphAttributes *CA,
	const SubgraphData &data)
{
	return CA ? readAttribute(*CA, data.rootCluster, *this) : true;
}


bool Ast::AttrStmt::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes* /*unused parameter*/,
	ClusterGraph* /*unused parameter*/, ClusterGraphAttributes *CA,
	const SubgraphData &data)
{
	switch(type) {
	case Type::graph:
		return CA ? readAttributes(*CA, data.rootCluster, attrs) : true;
	case Type::node:
		data.nodeDefaults.push_back(attrs);
		return true;
	case Type::edge:
		data.edgeDefaults.push_back(attrs);
		return true;
	default:
		return false;
	}
}


bool Ast::Subgraph::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes *CA,
	const SubgraphData &data)
{
	// We pass a copy of defaults to subgraph because those parameters should
	// be local for given subgraph.
	std::vector<AttrList *> nodeDefaults(data.nodeDefaults);
	std::vector<AttrList *> edgeDefaults(data.edgeDefaults);
	SubgraphData newData = data.withDefaults(nodeDefaults, edgeDefaults);

	// Create new cluster if subgraph identifier is given and it starts with
	// pattern "cluster". Otherwise, subgraph is not considered as a cluster
	// (as stated in DOT manual).
	const std::string patt = "cluster";
	if(C && id && id->compare(0, patt.length(), patt) == 0) {
		return readStatements(
			P, G, GA, C, CA,
			newData.withCluster(C->newCluster(newData.rootCluster)),
			statements);
	}

	return readStatements(P, G, GA, C, CA, newData, statements);
}


bool Ast::NodeId::read(
	Parser &P,
	ogdf::Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes* /*unused parameter*/,
	const SubgraphData &data)
{
	data.nodes.insert(P.requestNode(G, GA, C, data, id));
	return true;
}


Parser::Parser(std::istream &in) : m_in(in), m_nodeId(nullptr)
{
}


node Parser::requestNode(
	Graph &G, GraphAttributes *GA, ClusterGraph *C,
	const SubgraphData &data,
	const std::string &id)
{
	node v;
	// Ugly and slow, fix it somehow in the future.
	if(!m_nodeId[id]) {
		v = m_nodeId[id] = G.newNode();
		if(C) {
			C->reassignNode(v, data.rootCluster);
		}

		// We also set default attributes when a node is requested for the
		// first time. This may sound strange but Graphviz's DOT tool behaves
		// exactly the same so I guess this is a right place to use these.
		if(GA) {
			if(GA->has(GraphAttributes::nodeLabel)) {
				GA->label(v) = id;
			}
			readAttributes(*GA, v, data.nodeDefaults);
		}
	} else {
		v = m_nodeId[id];
	}

	// So, the question is: where to put a node if it can be declared with
	// edge statement and edge statements with common node may appear in
	// various clusters? Accoring to my own tests (using Graphviz's DOT tool)
	// it is simple: put it in the deepest possible cluster in which it shows
	// up. And this is achieved by the line below - if a node is requested on
	// a level that is deeper than currently assigned one, then we reassign
	// cluster.
	if(C && data.rootCluster->depth() > C->clusterOf(v)->depth()) {
		C->reassignNode(v, data.rootCluster);
	}

	return v;
}


bool Parser::readGraph(
	Graph &G, GraphAttributes *GA,
	ClusterGraph *C, ClusterGraphAttributes *CA)
{
	m_nodeId.clear();
	G.clear();
	if(C) {
		C->clear();
	}

	Lexer lexer(m_in);
	if(!lexer.tokenize()) {
		return false;
	}

	Ast ast(lexer.tokens());
	return ast.build() && ast.root()->read(*this, G, GA, C, CA);
}


bool Parser::read(Graph &G)
{
	return readGraph(G, nullptr, nullptr, nullptr);
}


bool Parser::read(Graph &G, GraphAttributes &GA)
{
	return readGraph(G, &GA, nullptr, nullptr);
}


bool Parser::read(Graph &G, ClusterGraph &C)
{
	return readGraph(G, nullptr, &C, nullptr);
}


bool Parser::read(Graph &G, ClusterGraph &C, ClusterGraphAttributes &CA)
{
	return readGraph(G, &CA, &C, &CA);
}


SubgraphData::SubgraphData(
	cluster root,
	std::vector<Ast::AttrList *> &nodeDefaultsVector,
	std::vector<Ast::AttrList *> &edgeDefaultsVector,
	std::set<node> &nodeSet)
:
	rootCluster(root),
	nodeDefaults(nodeDefaultsVector),
	edgeDefaults(edgeDefaultsVector),
	nodes(nodeSet)
{
}


SubgraphData SubgraphData::withCluster(
	cluster newRootCluster) const
{
	return SubgraphData(newRootCluster, nodeDefaults, edgeDefaults, nodes);
}


SubgraphData SubgraphData::withDefaults(
	std::vector<Ast::AttrList *> &newNodeDefaults,
	std::vector<Ast::AttrList *> &newEdgeDefaults) const
{

	return SubgraphData(rootCluster, newNodeDefaults, newEdgeDefaults, nodes);
}


SubgraphData SubgraphData::withNodes(
	std::set<node> &newNodes) const
{
	return SubgraphData(rootCluster, nodeDefaults, edgeDefaults, newNodes);
}

}

}
