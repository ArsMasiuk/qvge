/** \file
 * \brief Implementation of XML parser (class XmlParser)
 * (used for parsing and reading XML files)
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

#include <ogdf/fileformats/XmlParser.h>

namespace ogdf {

void XmlParser::reportError(
	const char *functionName,
	int sourceLine,
	const char *message,
	int inputFileLine)
{
	m_parseError = true;

	Logger::slout()
		<< "Error reported!\n"
		<< "\tFunction: " << functionName
		<< "(), Source line: " << sourceLine
		<< "\n\tMessage: " << message << "\n";
	if (inputFileLine != -1) {
		Logger::slout() << "\tCurrent line of input file: " << inputFileLine;
	}
}

bool XmlTagObject::isLeaf() const {
	if(this->m_pFirstSon)	return false;
	else return true;
}

bool XmlTagObject::findSonXmlTagObjectByName(
	const string &sonsName,
	XmlTagObject *&son) const
{
	XmlTagObject *currentSon = this->m_pFirstSon;
	while(currentSon && currentSon->m_pTagName->key() != sonsName)
	{
		currentSon = currentSon->m_pBrother;
	}

	if(currentSon) {
		son = currentSon;
		return true;
	}

	son = nullptr;
	return false;
}

bool XmlTagObject::findSonXmlTagObjectByName(
	const string &sonsName,
	List<XmlTagObject*> &sons) const
{
	bool found = false;
	XmlTagObject *currentSon = this->m_pFirstSon;
	while(currentSon)
	{
		if(currentSon->m_pTagName->key() == sonsName) {
			found = true;
			sons.pushBack(currentSon);
		}
		currentSon = currentSon->m_pBrother;
	}

	return found;
}

bool XmlTagObject::hasMoreSonXmlTagObject(const List<string> &sonNamesToIgnore) const {
	const XmlTagObject *currentSon = this->m_pFirstSon;
	while(currentSon)
	{

		//Proof if name of currentSon is inequal to all in sonsName
		bool found = false;
		for(const string &str : sonNamesToIgnore) {
			if (str == currentSon->m_pTagName->key()) {
				found = true;
				break;
			}
		}
		if(!found) return true;
		currentSon = currentSon->m_pBrother;
	}

	return false;
}

bool XmlTagObject::findXmlAttributeObjectByName(
	const string &attName,
	XmlAttributeObject*& attribute) const
{
	XmlAttributeObject *currentAttribute = this->m_pFirstAttribute;
	while ((currentAttribute != nullptr) &&
		(currentAttribute->m_pAttributeName->key() != attName))
	{
		currentAttribute = currentAttribute->m_pNextAttribute;
	}

	// Attribute found
	if (currentAttribute != nullptr){
		attribute = currentAttribute;
		return true;
	}

	// Not found
	attribute = nullptr;
	return false;
}

bool XmlTagObject::isAttributeLess() const {
	if(this->m_pFirstAttribute) return false;
	else return true;
}


XmlParser::XmlParser(std::istream &is) :
	m_pRootTag(nullptr),
	m_hashTableInfoIndex(0),
	m_recursionDepth(0),
	m_parseError(false)
{
	// Create scanner
	m_pScanner = new XmlScanner(is);
}

XmlParser::~XmlParser()
{
	// Delete parse tree
	if (m_pRootTag)
		destroyParseTree(m_pRootTag);

	// Delete scanner
	delete m_pScanner;

}

bool XmlParser::createParseTree()
{
	// create parse tree
	m_parseError = false;
	m_pRootTag = parse();

	// recursion depth not correct
	if (m_recursionDepth != 0) {
		reportError("XmlParser::createParseTree", __LINE__, "Recursion depth not equal to zero after parsing!");
		return false;
	}

	return !m_parseError;
}

void XmlParser::destroyParseTree(XmlTagObject *root)
{
	// Destroy all attributes of root
	XmlAttributeObject *currentAttribute = root->m_pFirstAttribute;
	while (currentAttribute != nullptr){
		XmlAttributeObject *nextAttribute = currentAttribute->m_pNextAttribute;
		delete currentAttribute;
		currentAttribute = nextAttribute;
	}

	// Traverse children of root and destroy them
	XmlTagObject *currentChild = root->m_pFirstSon;
	while (currentChild != nullptr){
		XmlTagObject *nextChild = currentChild->m_pBrother;
		destroyParseTree(currentChild);
		currentChild = nextChild;
	}

	// Destroy root itself
	delete root;
}

// Take a look at the state machine of parse() to understand
// what is going on here.
//
// TODO: It seems to be useful that this function throws an exception
//       if something goes wrong.
XmlTagObject *XmlParser::parse()
{
	// Increment recursion depth
	++m_recursionDepth;

	// currentTagObject is the tag object we want to create
	// in this invocation of parse()
	XmlTagObject *currentTagObject = nullptr;

	// Now we are in the start state of the state machine
	for( ; ; )
	{
		XmlToken token = m_pScanner->getNextToken();

		// Expect "<", otherwise failure
		if (token != XmlToken::openingBracket){
			reportError("XmlParser::parse",
						__LINE__,
						"Opening Bracket expected!",
						getInputFileLineCounter());
			return currentTagObject;
		}

		// Let's look what comes after "<"
		token = m_pScanner->getNextToken();

		// Read "?", i.e. we have the XML header line <? ... ?>
		if (token == XmlToken::questionMark){

			// Skip until we reach the matching question mark
			if (!m_pScanner->skipUntil('?')){
				reportError("XmlParser::parse",
							__LINE__,
							"Could not found the matching '?'",
							getInputFileLineCounter());
				return currentTagObject;
			}

			// Consume ">", otherwise failure
			token = m_pScanner->getNextToken();
			if (token != XmlToken::closingBracket){
				reportError("XmlParser::parse",
							__LINE__,
							"Closing Bracket expected!",
							getInputFileLineCounter());
				return currentTagObject;
			}

			// Go to start state of the state machine
			continue;
		}

		// Read "!", i.e. we have a XML comment <!-- bla -->
		if (token == XmlToken::exclamationMark){

			// A preambel comment <!lala > which could be also nested
			if ((m_pScanner->getNextToken() != XmlToken::minus) ||
				(m_pScanner->getNextToken() != XmlToken::minus))
			{
				if (!m_pScanner->skipUntilMatchingClosingBracket()){

					reportError("XmlParser::parse",
						__LINE__,
						"Could not find closing comment bracket!",
						getInputFileLineCounter());
					return currentTagObject;
				}

				continue;
			}

			// Find end of comment
			bool endOfCommentFound = false;
			while (!endOfCommentFound){

				// Skip until we find a - (and skip over it)
				if (!m_pScanner->skipUntil('-', true)){
					reportError("XmlParser::parse",
						__LINE__,
						"Closing --> of comment not found!",
						getInputFileLineCounter());
					return currentTagObject;
				}

				// The next characters must be -> (note that one minus is already consumed)
				if ((m_pScanner->getNextToken() == XmlToken::minus) &&
					(m_pScanner->getNextToken() == XmlToken::closingBracket))
				{
					endOfCommentFound = true;
				}
			}

			// Go to start state of the state machine
			continue;
		}

		// We have found an identifier, i.e. a tag name
		if (token == XmlToken::identifier){

			// Get hash element of token string
			HashedString *tagName =
				hashString(m_pScanner->getCurrentToken());

			// Create new tag object
			currentTagObject = new XmlTagObject(tagName);
			if (currentTagObject == nullptr){
				OGDF_THROW(InsufficientMemoryException);
			}

			//push (opening) tagName to stack
			m_tagObserver.push(tagName->key());
			// set depth of current tag object
			currentTagObject->setDepth(m_recursionDepth);

			// set line of the tag object in the parsed xml document
			currentTagObject->setLine(getInputFileLineCounter());

			// Next token
			token = m_pScanner->getNextToken();

			// Again we found an identifier, so it must be an attribute
			if (token == XmlToken::identifier){

				// Read list of attributes
				do {
					// Save the attribute name
					HashedString *attributeName =
						hashString(m_pScanner->getCurrentToken());

					// Consume "=", otherwise failure
					token = m_pScanner->getNextToken();
					if (token != XmlToken::equalSign)
					{
						reportError("XmlParser::parse",
									__LINE__,
									"Equal Sign expected!",
									getInputFileLineCounter());
						return currentTagObject;
					}

					// Read value
					token = m_pScanner->getNextToken();
					if ((token != XmlToken::quotedValue) &&
						(token != XmlToken::identifier) &&
						(token != XmlToken::attributeValue))
					{
						reportError("XmlParser::parse",
									__LINE__,
									"No valid attribute value!",
									getInputFileLineCounter());
						return currentTagObject;
					}

					// Create a new XmlAttributeObject
					XmlAttributeObject *currentAttributeObject =
						new XmlAttributeObject(attributeName, hashString(m_pScanner->getCurrentToken()));
					if (currentAttributeObject == nullptr){
						OGDF_THROW(InsufficientMemoryException);
					}

					// Append attribute to attribute list of the current tag object
					appendAttributeObject(currentTagObject, currentAttributeObject);

					// Get next token
					token = m_pScanner->getNextToken();
				}
				while (token == XmlToken::identifier);
			}

			// Read "/", i.e. the tag is ended immeadiately, e.g.
			// <A ... /> without a closing tag </A>
			if (token == XmlToken::slash){
				// Consume ">", otherwise failure
				token = m_pScanner->getNextToken();
				if (token != XmlToken::closingBracket)
				{
					reportError("XmlParser::parse",
								__LINE__,
								"Closing Bracket expected!",
								getInputFileLineCounter());
					return currentTagObject;
				}

				// The tag is closed and ended so we return
				string s = m_tagObserver.popRet();
				--m_recursionDepth;
				return currentTagObject;
			}

			// Read ">", i.e. the tag is closed and we
			// expect some content
			if (token == XmlToken::closingBracket){
				// We read something different from "<", so we have to
				// deal with a tag value now, i.e. a string inbetween the
				// opening and the closing tag, e.g. <A ...> lalala </A>
				if (m_pScanner->testNextToken() != XmlToken::openingBracket){

					// Read the characters until "<" is reached and put them into
					// currentTagObject
					m_pScanner->readStringUntil('<');
					currentTagObject->m_pTagValue = hashString(m_pScanner->getCurrentToken());

					// We expect a closing tag now, i.e. </id>
					token = m_pScanner->getNextToken();
					if (token != XmlToken::openingBracket)
					{
						reportError("XmlParser::parse",
							__LINE__,
							"Opening Bracket expected!",
							getInputFileLineCounter());
						return currentTagObject;
					}

					token = m_pScanner->getNextToken();
					if (token != XmlToken::slash)
					{
						reportError("XmlParser::parse",
									__LINE__,
									"Slash expected!",
									getInputFileLineCounter());
						return currentTagObject;
					}

					token = m_pScanner->getNextToken();
					if (token != XmlToken::identifier)
					{
						reportError("XmlParser::parse",
									__LINE__,
									"Identifier expected!",
									getInputFileLineCounter());
						return currentTagObject;
					}

					// next token is the closing tag
					string nextTag(m_pScanner->getCurrentToken());
					// pop corresponding tag from stack
					string s = m_tagObserver.popRet();
					// compare the two tags
					if (s != nextTag)
					{
						// the closing tag doesn't correspond to the opening tag:
						reportError("XmlParser::parse",
									__LINE__,
									"wrong closing tag!",
									getInputFileLineCounter());
						return currentTagObject;
					}

					token = m_pScanner->getNextToken();
					if (token != XmlToken::closingBracket)
					{
						reportError("XmlParser::parse",
									__LINE__,
									"Closing Bracket expected!",
									getInputFileLineCounter());
						return currentTagObject;
					}

					// The tag is closed so we return
					--m_recursionDepth;
					return currentTagObject;
				}

				// Found "<", so a (series of) new tag begins and we have to perform
				// recursive invocation of parse()
				//
				// There are two exceptions:
				// - a slash follows afer <, i.e. we have a closing tag
				// - an exclamation mark follows after <, i.e. we have a comment
				while (m_pScanner->testNextToken() == XmlToken::openingBracket){

					// Leave the while loop if a closing tag occurs
					if (m_pScanner->testNextNextToken() == XmlToken::slash){
						break;
					}

					// Ignore comments
					if (m_pScanner->testNextNextToken() == XmlToken::exclamationMark){

						// Comment must start with <!--
						if ((m_pScanner->getNextToken() != XmlToken::openingBracket) ||
							(m_pScanner->getNextToken() != XmlToken::exclamationMark) ||
							(m_pScanner->getNextToken() != XmlToken::minus) ||
							(m_pScanner->getNextToken() != XmlToken::minus))
						{
							reportError("XmlParser::parse",
								__LINE__,
								"Comment must start with <!--",
								getInputFileLineCounter());
							return currentTagObject;
						}

						// Find end of comment
						bool endOfCommentFound = false;
						while (!endOfCommentFound){

							// Skip until we find a - (and skip over it)
							if (!m_pScanner->skipUntil('-', true)){
								reportError("XmlParser::parse",
									__LINE__,
									"Closing --> of comment not found!",
									getInputFileLineCounter());
								return currentTagObject;
							}

							// The next characters must be -> (note that one minus is already consumed)
							if ((m_pScanner->getNextToken() == XmlToken::minus) &&
								(m_pScanner->getNextToken() == XmlToken::closingBracket))
							{
								endOfCommentFound = true;
							}
						}

						// Proceed with outer while loop
						continue;
					}

					// The new tag object is a son of the current tag object
					XmlTagObject *sonTagObject = parse();
					appendSonTagObject(currentTagObject, sonTagObject);
				}

				// Now we have found all tags.
				// We expect a closing tag now, i.e. </id>
				token = m_pScanner->getNextToken();
				if (token != XmlToken::openingBracket)
				{
					reportError("XmlParser::parse",
								__LINE__,
								"Opening Bracket expected!",
								getInputFileLineCounter());
					return currentTagObject;
				}

				token = m_pScanner->getNextToken();
				if (token != XmlToken::slash)
				{
					reportError("XmlParser::parse",
								__LINE__,
								"Slash expected!",
								getInputFileLineCounter());
					return currentTagObject;
				}

				token = m_pScanner->getNextToken();
				if (token != XmlToken::identifier)
				{
					reportError("XmlParser::parse",
								__LINE__,
								"Identifier expected!",
								getInputFileLineCounter());
					return currentTagObject;
				}

				// next token is the closing tag
				string nextTag(m_pScanner->getCurrentToken());
				// pop corresponding tag from stack
				string s = m_tagObserver.popRet();
				// compare the two tags
				if (s != nextTag)
				{
					// the closing tag doesn't correspond to the opening tag:
					reportError("XmlParser::parse",
								__LINE__,
								"wrong closing tag!",
								getInputFileLineCounter());
					return currentTagObject;
				}

				token = m_pScanner->getNextToken();
				if (token != XmlToken::closingBracket)
				{
					reportError("XmlParser::parse",
								__LINE__,
								"Closing Bracket expected!",
								getInputFileLineCounter());
					return currentTagObject;
				}

				--m_recursionDepth;

				// check if Document contains code after the last closing bracket
				if (m_recursionDepth == 0){
					token = m_pScanner->getNextToken();
					if (token != XmlToken::endOfFile){
						reportError("XmlParser::parse",
								__LINE__,
								"Document contains code after the last closing bracket!",
								getInputFileLineCounter());
						return currentTagObject;
					}
				}

				return currentTagObject;
			}

			OGDF_ASSERT(false);
#if 0
			continue;
#endif
		}

		OGDF_ASSERT(false);
	}
}

void XmlParser::appendAttributeObject(
	XmlTagObject *tagObject,
	XmlAttributeObject *attributeObject)
{

	// No attribute exists yet
	if (tagObject->m_pFirstAttribute == nullptr) {
		tagObject->m_pFirstAttribute = attributeObject;
	}
	// At least one attribute exists
	else{

		XmlAttributeObject *currentAttribute = tagObject->m_pFirstAttribute;

		// Find the last attribute
		while (currentAttribute->m_pNextAttribute != nullptr){
			currentAttribute = currentAttribute->m_pNextAttribute;
		}

		// Append given attribute
		currentAttribute->m_pNextAttribute = attributeObject;

	}
}

void XmlParser::appendSonTagObject(
	XmlTagObject *currentTagObject,
	XmlTagObject *sonTagObject)
{
	// No Son exists yet
	if (currentTagObject->m_pFirstSon == nullptr) {
		currentTagObject->m_pFirstSon = sonTagObject;
	}
	// At least one son exists
	else{

		XmlTagObject *currentSon = currentTagObject->m_pFirstSon;

		// Find the last son
		while (currentSon->m_pBrother != nullptr){
			currentSon = currentSon->m_pBrother;
		}

		// Append given son
		currentSon->m_pBrother = sonTagObject;
	}
}

HashedString *XmlParser::hashString(const string &str)
{
	// insertByNeed inserts a new element (str, -1) into the
	// table if no element with key str exists;
	// otherwise nothing is done
	HashedString *key = m_hashTable.insertByNeed(str,-1);

	// String str was not contained in the table
	// --> assign a new info index to the new string
	if(key->info() == -1){
		key->info() = m_hashTableInfoIndex++;
	}

	return key;
}

bool XmlParser::traversePath(
	const XmlTagObject &startTag,
	const Array<int> &infoIndexPath,
	const XmlTagObject *&targetTag) const
{
	// Traverse array
	const XmlTagObject *currentTag = &startTag;
	for (auto &elem : infoIndexPath){
		const XmlTagObject *sonTag;

		// Not found
		if (!findSonXmlTagObject(*currentTag, elem, sonTag)){
			return false;
		}

		// Found
		currentTag = sonTag;

	}

	targetTag = currentTag;
	return true;
}

bool XmlParser::findSonXmlTagObject(const XmlTagObject &father,
										int sonInfoIndex,
										const XmlTagObject *&son) const
{
	// Traverse sons
	const XmlTagObject *currentSon = father.m_pFirstSon;
	while ((currentSon != nullptr) &&
		(currentSon->m_pTagName->info() != sonInfoIndex))
	{
		currentSon = currentSon->m_pBrother;
	}

	// Son found
	if (currentSon != nullptr){
		son = currentSon;
		return true;
	}

	// Not found
	son = nullptr;
	return false;
}

bool XmlParser::findBrotherXmlTagObject(const XmlTagObject &currentTag,
											int brotherInfoIndex,
											const XmlTagObject *&brother) const
{

	const XmlTagObject *currentBrother = currentTag.m_pBrother;
	while ((currentBrother != nullptr) &&
		(currentBrother->m_pTagName->info() != brotherInfoIndex))
	{
		currentBrother = currentBrother->m_pBrother;
	}

	// brother found
	if (currentBrother != nullptr){
		brother = currentBrother;
		return true;
	}

	// Not found
	brother = nullptr;
	return false;
}

bool XmlParser::findXmlAttributeObject(
	const XmlTagObject &currentTag,
	int attributeInfoIndex,
	const XmlAttributeObject *&attribute) const
{
	const XmlAttributeObject *currentAttribute = currentTag.m_pFirstAttribute;
	while ((currentAttribute != nullptr) &&
		(currentAttribute->m_pAttributeName->info() != attributeInfoIndex))
	{
		currentAttribute = currentAttribute->m_pNextAttribute;
	}

	// Attribute found
	if (currentAttribute != nullptr){
		attribute = currentAttribute;
		return true;
	}

	// Not found
	attribute = nullptr;
	return false;
}

void XmlParser::printHashTable(std::ostream &os)
{
	// Header
	os << "\n--- Content of Hash table: m_hashTable ---\n" << std::endl;

	// Get iterator
	HashConstIterator<string, int> it;

	// Traverse table
	for( it = m_hashTable.begin(); it.valid(); ++it){
		os << "\"" << it.key() << "\" has index " << it.info() << std::endl;
	}
}

void XmlParser::printXmlTagObjectTree(
	std::ostream &outs,
	const XmlTagObject &rootObject,
	int indent) const
{
	printSpaces(outs, indent);

	// Opening tag (bracket and Tag name)
	outs << "<" << rootObject.m_pTagName->key();

	// Attributes
	XmlAttributeObject *currentAttribute = rootObject.m_pFirstAttribute;
	while (currentAttribute != nullptr){

		outs << " "
			 << currentAttribute->m_pAttributeName->key()
			 << " = \""
			 << currentAttribute->m_pAttributeValue->key()
			 << "\"";

		// Next attribute
		currentAttribute = currentAttribute->m_pNextAttribute;
	}

	// Closing bracket
	outs << ">" << std::endl;

	// Children
	const XmlTagObject *currentChild = rootObject.m_pFirstSon;
	while (currentChild != nullptr){
		// Proceed recursively
		printXmlTagObjectTree(outs, *currentChild, indent + 2);

		// Next child
		currentChild = currentChild->m_pBrother;
	}

	// Content
	if (rootObject.m_pTagValue != nullptr){
		printSpaces(outs, indent + 2);

		outs << rootObject.m_pTagValue->key() << std::endl;
	}

	// Closing tag
	printSpaces(outs, indent);
	outs << "</" << rootObject.m_pTagName->key() << ">" << std::endl;
}

void XmlParser::printSpaces(std::ostream &outs, int nOfSpaces) const
{
	for (int i = 0; i < nOfSpaces; i++){
		outs << " ";
	}
}

std::ostream &operator<<(std::ostream &os, const XmlParser &parser)
{
	parser.printXmlTagObjectTree(os, parser.getRootTag(), 0);
	return os;
}
}
