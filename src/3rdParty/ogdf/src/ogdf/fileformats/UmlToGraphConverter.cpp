/** \file
 * \brief Implementation of the class UmlToGraphConverter
 *
 * \author Dino Ahr
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


#include <ogdf/fileformats/UmlToGraphConverter.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {

UmlToGraphConverter::UmlToGraphConverter(std::istream &is)
{
	// Create parser and get reference to hash table
	m_xmlParser = new XmlParser(is);

	// Fill hash table of the parser with predefined info indices
	initializePredefinedInfoIndices();

	// Create the parse tree
	if (m_xmlParser->createParseTree() == false) {
		GraphIO::logger.lout() << "Could not create XML parse tree!" << std::endl;
		return; // parse error
	}

	// Create the uml model graph
	m_modelGraph = new UmlModelGraph();
	if (!createModelGraph(*m_modelGraph)) {
		GraphIO::logger.lout() << "Could not create UML model graph." << std::endl;
		return;
	}

	// Create the uml diagram graphs
	if (!createDiagramGraphs()){
		GraphIO::logger.lout() << "Could not create UML diagram graphs." << std::endl;
		return;
	}

	// Create the diagram graph in UMLGraph format
	if (!createDiagramGraphsInUMLGraphFormat(m_diagramGraphsInUMLGraphFormat)) {
		GraphIO::logger.lout() << "Could not create diagram graph in UML graph format." << std::endl;
		return;
	}
}

UmlToGraphConverter::~UmlToGraphConverter()
{
	// Delete diagram graphs in UMLGraph format
	for (UMLGraph *umlg : m_diagramGraphsInUMLGraphFormat) {
		const Graph & associatedGraph = (const Graph &)(*umlg);
		delete umlg;
		delete &associatedGraph;
	}
	m_diagramGraphsInUMLGraphFormat.clear();


	// Delete diagram graphs
	for (UmlDiagramGraph *dg : m_diagramGraphs) {
		delete dg;
	}
	m_diagramGraphs.clear();

	// Destroy model graph
	delete m_modelGraph;

	// Destroy parser
	delete m_xmlParser;
}

void UmlToGraphConverter::initializePredefinedInfoIndices()
{
	m_xmlParser->addNewHashElement("XMI",                        static_cast<int>(PredefinedInfoIndex::xmi));
	m_xmlParser->addNewHashElement("XMI.content",			     static_cast<int>(PredefinedInfoIndex::xmiContent));
	m_xmlParser->addNewHashElement("xmi.id",                     static_cast<int>(PredefinedInfoIndex::xmiId));
	m_xmlParser->addNewHashElement("UML:Model",                  static_cast<int>(PredefinedInfoIndex::umlModel));
	m_xmlParser->addNewHashElement("UML:Namespace.ownedElement", static_cast<int>(PredefinedInfoIndex::umlNamespaceOwnedElement));
	m_xmlParser->addNewHashElement("UML:Class",					 static_cast<int>(PredefinedInfoIndex::umlClass));
	m_xmlParser->addNewHashElement("name",						 static_cast<int>(PredefinedInfoIndex::name));
	m_xmlParser->addNewHashElement("UML:Generalization",		 static_cast<int>(PredefinedInfoIndex::umlGeneralization));
	m_xmlParser->addNewHashElement("child",						 static_cast<int>(PredefinedInfoIndex::child));
	m_xmlParser->addNewHashElement("parent",					 static_cast<int>(PredefinedInfoIndex::parent));
	m_xmlParser->addNewHashElement("UML:Association",			 static_cast<int>(PredefinedInfoIndex::umlAssociation));
	m_xmlParser->addNewHashElement("UML:Association.connection", static_cast<int>(PredefinedInfoIndex::umlAssociationConnection));
	m_xmlParser->addNewHashElement("UML:AssociationEnd",		 static_cast<int>(PredefinedInfoIndex::umlAssociationEnd));
	m_xmlParser->addNewHashElement("type",		                 static_cast<int>(PredefinedInfoIndex::type));
	m_xmlParser->addNewHashElement("UML:Diagram",		         static_cast<int>(PredefinedInfoIndex::umlDiagram));
	m_xmlParser->addNewHashElement("UML:Diagram.element",		 static_cast<int>(PredefinedInfoIndex::rootUmlDiagramElement));
	m_xmlParser->addNewHashElement("UML:DiagramElement",		 static_cast<int>(PredefinedInfoIndex::umlDiagramElement));
	m_xmlParser->addNewHashElement("geometry",					 static_cast<int>(PredefinedInfoIndex::geometry));
	m_xmlParser->addNewHashElement("subject",					 static_cast<int>(PredefinedInfoIndex::subject));
	m_xmlParser->addNewHashElement("UML:Package",				 static_cast<int>(PredefinedInfoIndex::umlPackage));
	m_xmlParser->addNewHashElement("UML:Interface",				 static_cast<int>(PredefinedInfoIndex::umlInterface));
	m_xmlParser->addNewHashElement("UML:Dependency",			 static_cast<int>(PredefinedInfoIndex::umlDependency));
	m_xmlParser->addNewHashElement("client",			         static_cast<int>(PredefinedInfoIndex::client));
	m_xmlParser->addNewHashElement("supplier",			         static_cast<int>(PredefinedInfoIndex::supplier));
	m_xmlParser->addNewHashElement("diagramType",			     static_cast<int>(PredefinedInfoIndex::diagramType));
	m_xmlParser->addNewHashElement("ClassDiagram",			     static_cast<int>(PredefinedInfoIndex::classDiagram));
	m_xmlParser->addNewHashElement("ModuleDiagram",			     static_cast<int>(PredefinedInfoIndex::moduleDiagram));

}

void UmlToGraphConverter::printIdToNodeMappingTable(std::ofstream &os)
{
	// Header
	os << "\n--- Content of Hash table: m_m_idToNode ---\n" << std::endl;

	// Get iterator
	HashConstIterator<int, node> it;

	// Traverse table
	for( it = m_idToNode.begin(); it.valid(); ++it){
		os << "\"" << it.key() << "\" has index "
			<< m_modelGraph->getNodeLabel(it.info()) << std::endl;
	}
}

void UmlToGraphConverter::printDiagramsInUMLGraphFormat(std::ofstream &os)
{
	// Traverse diagrams
	for (UMLGraph *diagram : m_diagramGraphsInUMLGraphFormat)
	{
		// Get underlying graphs
		const Graph &G = (const Graph &)*diagram;
		const GraphAttributes &AG = *diagram;

		// Nodes
		os << "Classes:" << std::endl;
		for(node v : G.nodes)
		{
			os << "\t" << AG.label(v);

			os << " with geometry ("
				 << AG.x(v) << ", "
				 << AG.y(v) << ", "
				 << AG.width(v) << ", "
				 << AG.height(v) << ")";

			os << std::endl;
		}

		// Edges
		os << "Relations:" << std::endl;
		for(edge e : G.edges)
		{
			os << "\t";

			if (AG.type(e) == Graph::EdgeType::association)
				os << "Association between ";
			if (AG.type(e) == Graph::EdgeType::generalization)
				os << "Generalization between ";

			os << AG.label(e->source()) << " and "
				 << AG.label(e->target()) << std::endl;
		}

		os << "---------------------------------------------------------------\n\n" << std::endl;
	}
}

bool UmlToGraphConverter::createModelGraph(UmlModelGraph &modelGraph){
	// Message
	//std::cout << "Creating model graph..." << std::endl;

	// Check root element (must be <XMI>)
	if (m_xmlParser->getRootTag().m_pTagName->info() != static_cast<int>(PredefinedInfoIndex::xmi)) {
		GraphIO::logger.lout() << "Root tag is not <XMI>" << std::endl;
		return false;
	}

	// Find first <UML:Namespace.ownedElement>; this is the father tag
	Array<int> path(3);
	path[0] = static_cast<int>(PredefinedInfoIndex::xmiContent);
	path[1] = static_cast<int>(PredefinedInfoIndex::umlModel);
	path[2] = static_cast<int>(PredefinedInfoIndex::umlNamespaceOwnedElement);
	const XmlTagObject *fatherTag;
	string rootPackageName("");
	if (!m_xmlParser->traversePath(m_xmlParser->getRootTag(), path, fatherTag)) {
		GraphIO::logger.lout() << "Path xmiContent, umlModel, umlNamespaceOwnedElement not found!" << std::endl;
		return false;
	}

	// Traverse packages and insert classifier nodes
	if (!traversePackagesAndInsertClassifierNodes(
		*fatherTag,
		rootPackageName,
		modelGraph))
	{
		return false;
	}

	// Note that first alle nodes have to be inserted into the model graph
	// and after that the edges should be inserted. The reason is that it
	// is possible that edges are specified prior to that one or both nodes
	// have been created.


	// Traverse packages and insert association edges
	if (!traversePackagesAndInsertAssociationEdges(*fatherTag, modelGraph))
	{
		return false;
	}

	// Traverse packages and insert generalization edges
	if (!traversePackagesAndInsertGeneralizationEdges(*fatherTag, modelGraph))
	{
		return false;
	}

	// Insert dependency edges
	if (!insertDependencyEdges(*fatherTag, modelGraph))
	{
		return false;
	}

	return true;
}

bool UmlToGraphConverter::traversePackagesAndInsertClassifierNodes(
	const XmlTagObject &currentRootTag,
	const string &currentPackageName,
	UmlModelGraph &modelGraph)
{
	// We proceed in a DFS manner. As long as we are inside a package
	// and there is a subpackage inside, we dive into that subpackage
	// by calling this function recursively (with a new rootTag). Along
	// this we also construct the appropriate package name.
	//
	// If we arrive at a level where either all subpackages have been
	// already traversed or no subpackage is inside we proceed to find
	// the classifiers contained at the current level.
	//
	// At the moment we consider classed and interfaces.
	//
	// TODO: In Java it is possible that there classes contained in other classe,
	//       This is currently not detected.

	// Identify contained packages (<UML:Package>)
	const XmlTagObject *packageSon = nullptr;
	m_xmlParser->findSonXmlTagObject(currentRootTag, static_cast<int>(PredefinedInfoIndex::umlPackage), packageSon);
	while(packageSon != nullptr){

		// Create new name for the subpackage
		const XmlAttributeObject *nameAttribute;
		m_xmlParser->findXmlAttributeObject(*packageSon, static_cast<int>(PredefinedInfoIndex::name), nameAttribute);
		OGDF_ASSERT(nameAttribute != nullptr);
		string subPackageName = currentPackageName;
		if (currentPackageName.length() != 0){
			subPackageName += "::";
		}
		subPackageName += nameAttribute->m_pAttributeValue->key();

		// Find son umlNamespaceOwnedElement which indicates a nested package
		// if nonexistent then continue
		const XmlTagObject *newRootTag;
		if(m_xmlParser->findSonXmlTagObject(*packageSon, static_cast<int>(PredefinedInfoIndex::umlNamespaceOwnedElement), newRootTag)){

			// Call this function recursively
			if (!traversePackagesAndInsertClassifierNodes(*newRootTag, subPackageName, modelGraph))
			{
				// Something went wrong
				return false;
			}
		}

		// Next package (will be put into packageSon)
		m_xmlParser->findBrotherXmlTagObject(*packageSon, static_cast<int>(PredefinedInfoIndex::umlPackage), packageSon);
	}

	// Identify contained classes (<UML:Class>)
	if (!insertSpecificClassifierNodes(currentRootTag, currentPackageName, static_cast<int>(PredefinedInfoIndex::umlClass), modelGraph))
	{
		// Something went wrong
		return false;
	}

	// Identify contained interfaces (<UML:Interface>)
	if (!insertSpecificClassifierNodes(currentRootTag, currentPackageName, static_cast<int>(PredefinedInfoIndex::umlInterface), modelGraph))
	{
		// Something went wrong
		return false;
	}

	return true;

}

bool UmlToGraphConverter::insertSpecificClassifierNodes(const XmlTagObject &currentRootTag,
															const string currentPackageName,
															int desiredClassifier,
															UmlModelGraph &modelGraph)
{
	const XmlTagObject *classifierSon;
	m_xmlParser->findSonXmlTagObject(currentRootTag, desiredClassifier, classifierSon);
	while (classifierSon != nullptr){

		// Use the infoIndex of value of attribute xmi.id as reference to the node
		// it is unique for each classifier and is used to reference it in the
		// relation specifications
		const XmlAttributeObject *xmiIdAttr;

		// Did not find attribute xmi.id of classifier
		if (!m_xmlParser->findXmlAttributeObject(*classifierSon, static_cast<int>(PredefinedInfoIndex::xmiId), xmiIdAttr)) {
			GraphIO::logger.lout() << "Did not find attribute xmi.id of classifier." << std::endl;
			return false;
		}

		// We get an unique node id by the value of attribute xmi.id
		int nodeId = xmiIdAttr->m_pAttributeValue->info();

		// Find out name of the classifier
		const XmlAttributeObject *nameAttr;

		// Did not find name attribute
		if (!m_xmlParser->findXmlAttributeObject(*classifierSon, static_cast<int>(PredefinedInfoIndex::name), nameAttr)) {
			GraphIO::logger.lout() << "Did not find name attribute of classifier." << std::endl;
			return false;
		}

		// Name of the classifier is contained in the tag value
		HashedString *nodeName = nameAttr->m_pAttributeValue;

		// Create classifier name by prefixing it with the package name
		string nodeNameString = currentPackageName;
		if (currentPackageName.length() != 0){
			nodeNameString += "::";
		}
		nodeNameString += nodeName->key();

		// Check if node already exists
		if (m_idToNode.lookup(nodeId) != nullptr) {
			GraphIO::logger.lout() << "Node already exists." << std::endl;
			return false;
		}

		// Create a node for the graph
		node newNode = modelGraph.newNode();
		modelGraph.label(newNode) = nodeNameString;
		modelGraph.type(newNode) = Graph::NodeType::vertex;

		// Put node into hash table
		m_idToNode.fastInsert(nodeId, newNode);

		// Proceed with next class (will be put into classifierSon)
		m_xmlParser->findBrotherXmlTagObject(*classifierSon, desiredClassifier, classifierSon);
	}

	return true;
}

bool UmlToGraphConverter::traversePackagesAndInsertAssociationEdges(const XmlTagObject &currentRootTag,
																		UmlModelGraph &modelGraph)
{
	// The traversion of the packages is identical with this of
	// traversePackagesAndInsertClassifierNodes

	// Identify contained packages (<UML:Package>)
	const XmlTagObject *packageSon;
	m_xmlParser->findSonXmlTagObject(currentRootTag, static_cast<int>(PredefinedInfoIndex::umlPackage), packageSon);
	while (packageSon != nullptr){

		// Find son umlNamespaceOwnedElement
		// if nonexistent then continue
		const XmlTagObject *newRootTag;

		if (m_xmlParser->findSonXmlTagObject(*packageSon, static_cast<int>(PredefinedInfoIndex::umlNamespaceOwnedElement), newRootTag))
		{
			// Call this function recursively
			if (!traversePackagesAndInsertAssociationEdges(*newRootTag, modelGraph))
			{
				return false;
			}
		}

		// Next package
		m_xmlParser->findBrotherXmlTagObject(*packageSon, static_cast<int>(PredefinedInfoIndex::umlPackage), packageSon);
	}

	// Find all associations (<UML:Association>)
	const XmlTagObject *associationSon;
	m_xmlParser->findSonXmlTagObject(currentRootTag, static_cast<int>(PredefinedInfoIndex::umlAssociation), associationSon);
	while (associationSon != nullptr){

		// Find out the reference number of this edge
		const XmlAttributeObject *edgeIdAttr = nullptr;
		m_xmlParser->findXmlAttributeObject(*associationSon, static_cast<int>(PredefinedInfoIndex::xmiId), edgeIdAttr);
		int edgeId = edgeIdAttr->m_pAttributeValue->info();

		// Go to <UML:Association.connection>
		const XmlTagObject *connection;
		m_xmlParser->findSonXmlTagObject(*associationSon, static_cast<int>(PredefinedInfoIndex::umlAssociationConnection), connection);

		// We assume binary associations

		// Investigate association ends
		const XmlTagObject *end1;
		m_xmlParser->findSonXmlTagObject(*connection, static_cast<int>(PredefinedInfoIndex::umlAssociationEnd), end1);

		// Something wrong
		if (!end1) {
			GraphIO::logger.lout(Logger::Level::Minor) << "Current association tag does not contain both end tags!" << std::endl;
			// Next association
			m_xmlParser->findBrotherXmlTagObject(*associationSon, static_cast<int>(PredefinedInfoIndex::umlAssociation), associationSon);
			continue;
		}

		const XmlTagObject *end2;
		m_xmlParser->findBrotherXmlTagObject(*end1, static_cast<int>(PredefinedInfoIndex::umlAssociationEnd), end2);

		// Something wrong
		if (!end2) {
			GraphIO::logger.lout(Logger::Level::Minor) << "Current association tag does not contain both end tags!" << std::endl;
			// Next association
			m_xmlParser->findBrotherXmlTagObject(*associationSon, static_cast<int>(PredefinedInfoIndex::umlAssociation), associationSon);
			continue;
		}

		// Use the infoIndex of value of attribute type to find
		// the corresponding nodes
		const XmlAttributeObject *typeAttr1;
		m_xmlParser->findXmlAttributeObject(*end1, static_cast<int>(PredefinedInfoIndex::type), typeAttr1);
		const XmlAttributeObject *typeAttr2;
		m_xmlParser->findXmlAttributeObject(*end2, static_cast<int>(PredefinedInfoIndex::type), typeAttr2);
		int nodeId1 = typeAttr1->m_pAttributeValue->info();
		int nodeId2 = typeAttr2->m_pAttributeValue->info();

		// Create an edge for the graph
		HashElement<int, node> *node1HE = m_idToNode.lookup(nodeId1);
		HashElement<int, node> *node2HE = m_idToNode.lookup(nodeId2);

		// Both nodes were found
		if (node1HE && node2HE){
			node node1 = node1HE->info();
			node node2 = node2HE->info();
			edge modelEdge = modelGraph.newEdge(node1, node2);
			modelGraph.type(modelEdge) = Graph::EdgeType::association;

			// Insert edge id and edge element into hashing table
			m_idToEdge.fastInsert(edgeId, modelEdge);
		}

		// If condition above does not hold: Error!
		// At least one node is not contained in the node hashtable
		// One reason could be that we have an association between at least
		// one element other than class or interface

		// Next association
		m_xmlParser->findBrotherXmlTagObject(*associationSon, static_cast<int>(PredefinedInfoIndex::umlAssociation), associationSon);
	}

	return true;
}

bool UmlToGraphConverter::traversePackagesAndInsertGeneralizationEdges(
	const XmlTagObject &currentRootTag,
	UmlModelGraph &modelGraph)
{
	// TODO: The generalization tags can also occur inside interface classifiers (in Java)
	//       Currently we only consider classes.

	// Identify contained packages (<UML:Package>)
	const XmlTagObject *packageSon;
	m_xmlParser->findSonXmlTagObject(currentRootTag, static_cast<int>(PredefinedInfoIndex::umlPackage), packageSon);
	while (packageSon != nullptr){

		// Find son umlNamespaceOwnedElement
		// if nonexistent then continue
		const XmlTagObject *newRootTag;
		m_xmlParser->findSonXmlTagObject(*packageSon, static_cast<int>(PredefinedInfoIndex::umlNamespaceOwnedElement), newRootTag);
		if (newRootTag != nullptr){

			// Call this function recursively
			if (!traversePackagesAndInsertGeneralizationEdges(*newRootTag, modelGraph))
			{
				return false;
			}
		}

		// Next package
		m_xmlParser->findBrotherXmlTagObject(*packageSon, static_cast<int>(PredefinedInfoIndex::umlPackage), packageSon);
	}

	// Find all classes (<UML:Class>)
	const XmlTagObject *classSon;
	m_xmlParser->findSonXmlTagObject(currentRootTag, static_cast<int>(PredefinedInfoIndex::umlClass), classSon);
	while (classSon != nullptr){

		Array<int> path(2);
		path[0] = static_cast<int>(PredefinedInfoIndex::umlNamespaceOwnedElement);
		path[1] = static_cast<int>(PredefinedInfoIndex::umlGeneralization);
		const XmlTagObject *generalizationTag = nullptr;

		// Found a <UML:Generalization> tag
		if (m_xmlParser->traversePath(*classSon, path, generalizationTag)){

			// Find out the reference number of this edge
			const XmlAttributeObject *edgeIdAttr = nullptr;
			m_xmlParser->findXmlAttributeObject(*generalizationTag, static_cast<int>(PredefinedInfoIndex::xmiId), edgeIdAttr);
			int edgeId = edgeIdAttr->m_pAttributeValue->info();

			// Find child and parent attributes
			const XmlAttributeObject *childAttr = nullptr;
			m_xmlParser->findXmlAttributeObject(*generalizationTag, static_cast<int>(PredefinedInfoIndex::child), childAttr);
			const XmlAttributeObject *parentAttr = nullptr;
			m_xmlParser->findXmlAttributeObject(*generalizationTag, static_cast<int>(PredefinedInfoIndex::parent), parentAttr);

			// Something wrong
			if (!childAttr || !parentAttr){

				GraphIO::logger.lout(Logger::Level::Minor) << "Current dependency tag does not contain both attributes child and parent." << std::endl;

				// Next class
				m_xmlParser->findBrotherXmlTagObject(*classSon, static_cast<int>(PredefinedInfoIndex::umlClass), classSon);
				continue;
			}

			// Get ids and nodes
			int childId = childAttr->m_pAttributeValue->info();
			int parentId = parentAttr->m_pAttributeValue->info();

			// Get hash elements
			HashElement<int, node> *childNodeHE = m_idToNode.lookup(childId);
			HashElement<int, node> *parentNodeHE = m_idToNode.lookup(parentId);

			// Create an edge for the graph
			if (childNodeHE && parentNodeHE){

				node childNode  = childNodeHE->info();
				node parentNode = parentNodeHE->info();

				edge modelEdge = modelGraph.newEdge(childNode, parentNode);
				modelGraph.type(modelEdge) = Graph::EdgeType::generalization;

				// Insert edge id and edge element into hashing table
				m_idToEdge.fastInsert(edgeId, modelEdge);
			}
			// If condition above does not hold: Error!
			// At least one node is not contained in the node hashtable

		}

		// Next class
		m_xmlParser->findBrotherXmlTagObject(*classSon, static_cast<int>(PredefinedInfoIndex::umlClass), classSon);

	}

	return true;

}

bool UmlToGraphConverter::insertDependencyEdges(const XmlTagObject &currentRootTag,
													UmlModelGraph &modelGraph)
{
	// Find first dependency tag (<UML:Dependency>)
	const XmlTagObject *currentDependencyTag = nullptr;
	m_xmlParser->findSonXmlTagObject(currentRootTag, static_cast<int>(PredefinedInfoIndex::umlDependency), currentDependencyTag);

	// Find all dependencys
	while (currentDependencyTag != nullptr){

		// Find out the reference number of this edge
		const XmlAttributeObject *edgeIdAttr = nullptr;
		m_xmlParser->findXmlAttributeObject(*currentDependencyTag, static_cast<int>(PredefinedInfoIndex::xmiId), edgeIdAttr);
		int edgeId = edgeIdAttr->m_pAttributeValue->info();

		// Find client and supplier attributes
		const XmlAttributeObject *clientAttr = nullptr;
		m_xmlParser->findXmlAttributeObject(*currentDependencyTag, static_cast<int>(PredefinedInfoIndex::client), clientAttr);
		const XmlAttributeObject *supplierAttr = nullptr;
		m_xmlParser->findXmlAttributeObject(*currentDependencyTag, static_cast<int>(PredefinedInfoIndex::supplier), supplierAttr);

		// Something wrong
		if (!clientAttr || !supplierAttr){

			GraphIO::logger.lout(Logger::Level::Minor) << "Current dependency tag does not contain both attributes client and supplier." << std::endl;

			// Next dependency
			m_xmlParser->findBrotherXmlTagObject(*currentDependencyTag, static_cast<int>(PredefinedInfoIndex::umlDependency), currentDependencyTag);
			continue;
		}

		// Get ids
		int clientId = clientAttr->m_pAttributeValue->info();
		int supplierId = supplierAttr->m_pAttributeValue->info();

		// Get Hashelements
		HashElement<int, node> *clientNodeHE = m_idToNode.lookup(clientId);
		HashElement<int, node> *supplierNodeHE = m_idToNode.lookup(supplierId);

		// Create an edge for the graph
		if (clientNodeHE && supplierNodeHE){

			node clientNode   = clientNodeHE->info();
			node supplierNode = supplierNodeHE->info();

			edge modelEdge = modelGraph.newEdge(clientNode, supplierNode);
			modelGraph.type(modelEdge) = Graph::EdgeType::dependency;

			// Insert edge id and edge element into hashing table
			m_idToEdge.fastInsert(edgeId, modelEdge);
		}
		// If condition above does not hold: Error!
		// At least one node is not contained in the node hashtable

		// Next dependency
		m_xmlParser->findBrotherXmlTagObject(*currentDependencyTag, static_cast<int>(PredefinedInfoIndex::umlDependency), currentDependencyTag);
	}

	return true;
}

// Extracts the single values of string str with format
// "x, y, width, height," and puts them into doubleArray
static void stringToDoubleArray(const string &str, Array<double> &doubleArray)
{
	size_t strIndex = 0;
	char tempString[20];

	for (int i = 0; i < 4; i++){

		int tempStringIndex = 0;

		// Skip whitespace
		while (isspace(str[strIndex])){
			++strIndex;
		}

		// Copy characters of double value
		// values are separated by comma
		while (str[strIndex] != ','){

			tempString[tempStringIndex] = str[strIndex];
			++tempStringIndex;
			++strIndex;
		}

		// Skip over ','
		++strIndex;

		// Terminate string
		tempString[tempStringIndex] = '\0';

		// Put double value into array
		doubleArray[i] = atof(tempString);
	}
}

bool UmlToGraphConverter::createDiagramGraphs()
{
	// We want to create a diagram graph for each subtree <UML:Diagram> found
	// in the parse tree.
	//
	// Currently we are only interested in class diagrams.

	// Model graph must exist!
	OGDF_ASSERT(m_modelGraph != nullptr);

	// Message
	//std::cout << "Creating diagram graph(s)..." << std::endl;

	// Check root element (must be <XMI>)
	if (m_xmlParser->getRootTag().m_pTagName->info() != static_cast<int>(PredefinedInfoIndex::xmi)){
		GraphIO::logger.lout() << "Root tag is not <XMI>" << std::endl;
		return false;
	}

	// Find the first <UML:Diagram> tag starting at <XMI>
	Array<int> path(2);
	path[0] = static_cast<int>(PredefinedInfoIndex::xmiContent);
	path[1] = static_cast<int>(PredefinedInfoIndex::umlDiagram);
	const XmlTagObject *currentDiagramTag = nullptr;
	m_xmlParser->traversePath(m_xmlParser->getRootTag(), path, currentDiagramTag);

	// Traverse diagrams
	while (currentDiagramTag != nullptr){

		// Find out name of the diagram
		const XmlAttributeObject *nameAttr = nullptr;
		m_xmlParser->findXmlAttributeObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::name), nameAttr);
		string diagramName("");
		if (nameAttr != nullptr){
			diagramName = nameAttr->m_pAttributeValue->key();
		}

		// Find out type of the diagram
		const XmlAttributeObject *diagramTypeAttr = nullptr;
		m_xmlParser->findXmlAttributeObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::diagramType), diagramTypeAttr);

		// No diagramTypeAttribute found --> we continue with the next diagram
		if (diagramTypeAttr == nullptr){

			// Next diagram
			m_xmlParser->findBrotherXmlTagObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::umlDiagram), currentDiagramTag);
			continue;
		}

		// Check which type of diagram we have
		UmlDiagramGraph::UmlDiagramType diagramType;
		switch (diagramTypeAttr->m_pAttributeValue->info()){

		case (static_cast<int>(PredefinedInfoIndex::classDiagram)) :
			diagramType = UmlDiagramGraph::UmlDiagramType::classDiagram;
			break;
		case (static_cast<int>(PredefinedInfoIndex::moduleDiagram)) :
			diagramType = UmlDiagramGraph::UmlDiagramType::moduleDiagram;
			break;
		default:
			diagramType = UmlDiagramGraph::UmlDiagramType::unknownDiagram;
			break;
		}

		// Currently we only allow class diagrams; in all other cases
		// we continue with the next diagram
		if (diagramType != UmlDiagramGraph::UmlDiagramType::classDiagram){

			// Next diagram
			m_xmlParser->findBrotherXmlTagObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::umlDiagram), currentDiagramTag);
			continue;
		}

		// Create a new diagram graph and add it to the list of diagram graphs
		UmlDiagramGraph *diagramGraph =
			new UmlDiagramGraph(*m_modelGraph,
									diagramType,
									diagramName);
		m_diagramGraphs.pushBack(diagramGraph);


		// First pass the <UML:Diagram.element> tag
		const XmlTagObject *rootDiagramElementTag = nullptr;
		m_xmlParser->findSonXmlTagObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::rootUmlDiagramElement), rootDiagramElementTag);

		// No such tag found --> we continue with the next diagram
		if (rootDiagramElementTag == nullptr){

			// Next diagram
			m_xmlParser->findBrotherXmlTagObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::umlDiagram), currentDiagramTag);
			continue;
		}

		// Now investigate the diagram elements
		const XmlTagObject *currentDiagramElementTag = nullptr;
		m_xmlParser->findSonXmlTagObject(*rootDiagramElementTag, static_cast<int>(PredefinedInfoIndex::umlDiagramElement), currentDiagramElementTag);

		// Traverse all diagram elements (<UML:DiagramElement>)
		while (currentDiagramElementTag != nullptr){

			// We have to investigate the subject attribute which contains the
			// reference number of the represented element; then we can check if
			// a node or edge with this reference exists
			const XmlAttributeObject *subjectAttr = nullptr;
			m_xmlParser->findXmlAttributeObject(*currentDiagramElementTag, static_cast<int>(PredefinedInfoIndex::subject), subjectAttr);

			// Not found --> continue with the next diagram element
			if (subjectAttr == nullptr){

				// Next diagram element
				m_xmlParser->findBrotherXmlTagObject(*currentDiagramElementTag, static_cast<int>(PredefinedInfoIndex::umlDiagramElement), currentDiagramElementTag);

				continue;
			}

			// Check wether node or edge with this reference does exist
			int elementId = subjectAttr->m_pAttributeValue->info();

			// Node exists for that reference
			if (m_idToNode.lookup(elementId) != nullptr){

				// Get hash element
				HashElement<int,node> *nodeHashElement = m_idToNode.lookup(elementId);

				// Get node element
				node geometricNode = nodeHashElement->info();

				// Extract geometric information
				const XmlAttributeObject *geometryAttr = nullptr;
				m_xmlParser->findXmlAttributeObject(*currentDiagramElementTag, static_cast<int>(PredefinedInfoIndex::geometry), geometryAttr);

				// Not found
				if (geometryAttr == nullptr){
					break;
				}

				// Get double values of geometry
				Array<double> geometryArray(4);
				stringToDoubleArray(geometryAttr->m_pAttributeValue->key(), geometryArray);

				// Add node to diagram graph
				diagramGraph->addNodeWithGeometry(
					geometricNode,
					geometryArray[0],
					geometryArray[1],
					geometryArray[2],
					geometryArray[3]);
			} else {
				// Edge exists for that reference
				if (m_idToEdge.lookup(elementId) != nullptr){

					// Get hash element
					HashElement<int,edge> *edgeHashElement = m_idToEdge.lookup(elementId);

					// Get node element
					edge geometricEdge = edgeHashElement->info();

					// Add edge to diagram graph
					diagramGraph->addEdge(geometricEdge);
				}
			}

			// Next diagram element
			m_xmlParser->findBrotherXmlTagObject(*currentDiagramElementTag,
			                                     static_cast<int>(PredefinedInfoIndex::umlDiagramElement),
			                                     currentDiagramElementTag);
		}

		// Next diagram
		m_xmlParser->findBrotherXmlTagObject(*currentDiagramTag, static_cast<int>(PredefinedInfoIndex::umlDiagram), currentDiagramTag);
	}

	return true;
}

bool UmlToGraphConverter::createDiagramGraphsInUMLGraphFormat(SList<UMLGraph*> &diagramGraphsInUMLGraphFormat)
{
	// We want to create an instance of UMLGraph for each instance of UmlDiagramGraph
	// contained in the given list. Implicitly we have to create also an instance of class Graph
	// for each UMLGraph.
	// We maintain a hash list for mapping the nodes and edges of the model graph to the
	// new nodes and edges of the graph created for the diagram.
	// We use as key the unique index of the node resp. edge.

	// Message
	//std::cout << "Creating diagram graph(s) in UMLGraph format..." << std::endl;

	// Traverse list of diagram graphs
	for (UmlDiagramGraph *diagramGraph : m_diagramGraphs)
	{
		// Mapping from the index of the existing node to the new nodeElement
		Hashing<int, node> indexToNewNode;

		// Mapping from the index of the existing edge to the new edgeElement
		Hashing<int, edge> indexToNewEdge;

		// Create instance of class graph
		Graph *graph = new Graph();

		// Traverse list of nodes contained in the diagram
		const SList<node> &diagramNodes = diagramGraph->getNodes();
		for (node n : diagramNodes)
		{
			// Create a new "pendant" node for the existing	node
			node newNode = graph->newNode();

			// Insert mapping from index of the existing node to the pendant node
			// into hashtable
			indexToNewNode.fastInsert(n->index(), newNode);
		}

		// Traverse list of edges contained in the diagram
		const SList<edge> &diagramEdges = diagramGraph->getEdges();
		for (edge e : diagramEdges)
		{
			// Find out source and target of the edge
			node source = e->source();
			node target = e->target();

			// Find pendant nodes
			HashElement<int, node> *sourceHashElement =
				indexToNewNode.lookup(source->index());
			HashElement<int, node> *targetHashElement =
				indexToNewNode.lookup(target->index());
			node pendantSource = sourceHashElement->info();
			node pendantTarget = targetHashElement->info();

			// Insert new edge between pendant nodes
			edge newEdge = graph->newEdge(pendantSource, pendantTarget);

			// Insert mapping from index of the existing edgeto the pendant edge
			// into hashtable
			indexToNewEdge.fastInsert(e->index(), newEdge);
		}

		// Create instance of class UMLGraph
		UMLGraph *umlGraph = new UMLGraph(*graph, GraphAttributes::nodeLabel);

		// Now we want to add the geometry information and the node label
		const SList<double> xList = diagramGraph->getX();
		const SList<double> yList = diagramGraph->getY();
		const SList<double> wList = diagramGraph->getWidth();
		const SList<double> hList = diagramGraph->getHeight();
		SListConstIterator<double> xIt, yIt, wIt, hIt;

		// Traverse node list and geometry lists synchronously
		xIt = xList.begin();
		yIt = yList.begin();
		wIt = wList.begin();
		hIt = hList.begin();
		for(node n : diagramNodes)
		{
			// Get pendant node
			HashElement<int, node> *nodeHashElement =
				indexToNewNode.lookup(n->index());
			node pendantNode = nodeHashElement->info();

			// Insert geometry information into umlGraph
			umlGraph->x(pendantNode) = *xIt;
			umlGraph->y(pendantNode) = *yIt;
			umlGraph->width(pendantNode) = *wIt;
			umlGraph->height(pendantNode) = *hIt;

			// Insert label
			string &label = umlGraph->label(pendantNode);
			label = m_modelGraph->getNodeLabel(n);

			// Next iteration
			++xIt;
			++yIt;
			++wIt;
			++hIt;
		}

		// Traverse list of edges contained in the diagram
		for (edge e : diagramEdges)
		{
			// Find pendant edge
			HashElement<int, edge> *edgeHashElement =
				indexToNewEdge.lookup(e->index());
			edge pendantEdge = edgeHashElement->info();

			// Insert type information into umlGraph
			umlGraph->type(pendantEdge) = m_modelGraph->type(e);
		}

		// Add new umlGraph to list
		diagramGraphsInUMLGraphFormat.pushBack(umlGraph);
	}

	return true;
}

}
