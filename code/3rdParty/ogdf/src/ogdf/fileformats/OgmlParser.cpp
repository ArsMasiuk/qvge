/** \file
 * \brief Implementation of OGML parser.
 *
 * \author Christian Wolf and Bernd Zey
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

#include <ogdf/fileformats/OgmlParser.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {


// struct definitions for mapping of templates
struct OgmlParser::OgmlNodeTemplate
{
	string  m_id;
	Shape m_shapeType;
	double  m_width;
	double  m_height;
	string  m_color;
	FillPattern m_pattern;
	string  m_patternColor;
	StrokeType m_lineType;
	float  m_lineWidth;
	string  m_lineColor;
	// nodeTemplate stores the graphical type
	// e.g. rectangle, ellipse, hexagon, ...
	string  m_nodeTemplate;

	//Constructor:
	OgmlNodeTemplate(const string &id): m_id(id) { }
};


struct OgmlParser::OgmlEdgeTemplate
{
	string m_id;
	StrokeType m_lineType;
	float m_lineWidth;
	string m_color;
	int m_sourceType; // actually this is only a boolean value 0 or 1
	// ogdf doesn't support source-arrow-color and size
#if 0
	string m_sourceColor;
	double m_sourceSize;
#endif
	int m_targetType; // actually this is only a boolean value 0 or 1
	// ogdf doesn't support target-arrow-color and size
#if 0
	string m_targetColor;
	double m_targetSize;
#endif

	//Constructor:
	OgmlEdgeTemplate(const string &id): m_id(id) { }
};


#if 0
struct  OgmlParser::OgmlLabelTemplate{
	string m_id;
};
#endif


struct OgmlParser::OgmlSegment
{
	DPoint point1, point2;
};


//! Objects of this class represent a value set of an attribute in %Ogml.
class OgmlParser::OgmlAttributeValue
{
	int id; //!< Id of the attribute value; for possible ones see Ogml.h.

protected:
	int checkExpectedTagname(HashElement<string, const XmlTagObject*> *he, Ogml::TagId tag) const
	{
		if (he
		 && he->info()->getName() == Ogml::s_tagNames[tag]) {
			return Ogml::vs_valid;
		}
		return Ogml::vs_idRefErr;
	}

public:
	// Construction
	OgmlAttributeValue() : id(Ogml::av_any) { }

	OgmlAttributeValue(int attributeValueID) {
		if(attributeValueID >= 0 && attributeValueID < Ogml::ATT_VAL_NUM) this->id = attributeValueID;
		else this->id = Ogml::av_any;
	}

	// Destruction
	~OgmlAttributeValue() { }

	// Getter
	const int& getId() const { return id; }
	const string& getValue() const { return Ogml::s_attributeValueNames[id]; }

	// Setter
	void setId(int attributeValueID) {
		if(attributeValueID >= 0 && attributeValueID < Ogml::ATT_VAL_NUM) this->id = attributeValueID;
		else this->id = Ogml::av_any;
	}


	/**
	 * Checks the type of the input given in string
	 * and returns an OgmlAttributeValueId defined in Ogml.h
	 */
	Ogml::AttributeValueId getTypeOfString(const string& input) const
	{
		// |--------------------|
		// | char | ascii-value |
		// |--------------------|
		// | '.'  |     46      |
		// | '-'  |     45      |
		// | '+'  |     43      |
		// | '#'  |     35      |

		// bool values
		bool isInt = true;
		bool isNum = true;
		bool isHex = true;

		// input is a boolean value
		if (input == "true" || input == "false" /*|| input == "0" || input == "1"*/)
			return Ogml::av_bool;

		if (input.length() > 0){
			char actChar = input[0];
			int actCharInt = static_cast<int>(actChar);
			//check the first char
			if (!isalnum(actChar)){

				if (actCharInt == 35){
					// support hex values with starting "#"
					isInt = false;
					isNum = false;
				}
				else
				{

					// (actChar != '-') and (actChar != '+')
					if (actCharInt != 45 && actChar != 43){
						isInt = isNum = false;
					}
					else
					{
						// input[0] == '-' or '+'
						if (input.length() > 1){
							// 2nd char have to be a digit or xdigit
							actChar = input[1];
							if (!isdigit(actChar)){
								isInt = false;
								isNum = false;
								if (!isxdigit(actChar))
									return Ogml::av_string;
							}
						}
						else
							return Ogml::av_string;
					}
				}
			} else {
				if (!isdigit(actChar)){
					isInt = false;
					isNum = false;
				}
				if (!isxdigit(actChar)){
					isHex = false;
				}
			}

			// value for point seperator
			bool numPoint = false;

			// check every input char
			// and set bool value to false if char-type is wrong
			for(size_t it=1; it < input.length() && (isInt || isNum || isHex); ++it)
			{
				actChar = input[it];

				// actChar == '.'
				if (actChar == 46){
					isInt = false;
					isHex = false;
					if (!numPoint){
						numPoint = true;
					}
					else
						isNum = false;
				} else {
					if (!(isdigit(actChar))){
						isInt = false;
						isNum = false;
					}
					if (!(isxdigit(actChar)))
						isHex = false;
				}
			}
		} else {
			// input.length() == 0
			return Ogml::av_none;
		}
		// return correct value
		if (isInt) return Ogml::av_int;
		if (isNum) return Ogml::av_num;
		if (isHex) return Ogml::av_hex;
		// if all bool values are false return av_string
		return Ogml::av_string;
	}

	/**
	 * According to id this method proofs whether s is a valid value
	 * of the value set.
	 * E.g. if id=av_int s should contain an integer value.
	 * It returns the following validity states:
	 * 		vs_idNotUnique    =-10, //id already exhausted
	 * 		vs_idRefErr       = -9, //referenced id wasn't found or wrong type of referenced tag
	 * 		vs_idRefErr       = -8, //referenced id wrong
	 *		vs_attValueErr    = -3, //attribute-value error
	 * 		Ogml::vs_valid		  =  1  //attribute-value is valid
	 *
	 * TODO: Completion of the switch-case statement.
	 */
	int validValue(
		const string &attributeValue,
		const XmlTagObject* xmlTag,		        //owns an attribute with attributeValue
		Hashing<string,
		const XmlTagObject*>& ids) const //hashtable with id-tagName pairs
	{
		// get attribute value type of string
		Ogml::AttributeValueId stringType = getTypeOfString(attributeValue);

		switch (id) {
		case Ogml::av_any:
			return Ogml::vs_valid;
		case Ogml::av_int:
			if (stringType == Ogml::av_int) {
				return Ogml::vs_valid;
			}
			break;
		case Ogml::av_num:
			if (stringType == Ogml::av_num) {
				return Ogml::vs_valid;
			}
			if (stringType == Ogml::av_int) {
				return Ogml::vs_valid;
			}
			break;
		case Ogml::av_bool:
			if (stringType == Ogml::av_bool) {
				return Ogml::vs_valid;
			}
			break;
		case Ogml::av_string:
			return Ogml::vs_valid;
		case Ogml::av_uri:
			return Ogml::vs_valid;  // not yet checked in detail
		case Ogml::av_hex:
			if (stringType == Ogml::av_hex) {
				return Ogml::vs_valid;
			}
			if (stringType == Ogml::av_int) {
				return Ogml::vs_valid;
			}
			break;
		case Ogml::av_oct:
			return Ogml::vs_attValueErr; // not implemented?!
		case Ogml::av_id:
			if (!ids.lookup(attributeValue)) { // id does not exist
				ids.fastInsert(attributeValue, xmlTag);
				return Ogml::vs_valid;
			}
			return Ogml::vs_idNotUnique;
		case Ogml::av_nodeIdRef: // idRef of source, target, nodeRef, nodeStyle
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_node);
		case Ogml::av_edgeIdRef: // idRef of elements edgeRef, edgeStyle
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_edge);
		case Ogml::av_labelIdRef: // idRef of labelRef, labelStyle
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_label);
		case Ogml::av_pointIdRef: // idRef of endpoint point
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_point);
		case Ogml::av_sourceIdRef: // idRef of endpoint source
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_source);
		case Ogml::av_targetIdRef: // idRef of endpoint target
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_target);
		case Ogml::av_nodeStyleTemplateIdRef: // idRef of nodeStyle::template
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_nodeStyleTemplate);
		case Ogml::av_edgeStyleTemplateIdRef: // idRef of edgeStyle::template
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_edgeStyleTemplate);
		case Ogml::av_labelStyleTemplateIdRef: // idRef of labelStyle::template
			return checkExpectedTagname(ids.lookup(attributeValue), Ogml::t_labelStyleTemplate);
		default:
			// Proof string for equality
			if (getValue() == attributeValue) {
				return Ogml::vs_valid;
			}
		}
		return Ogml::vs_attValueErr;
	}

};

//! Objects of this class represent an attribute and its value set in %Ogml.
class OgmlParser::OgmlAttribute
{
	/**
	*/
	int id;  //!< Integer identifier of object; for possible ids see Ogml.h.
	List<OgmlAttributeValue*> values; //!< Represents the value set of this attribute.

public:

	// Construction
	OgmlAttribute() : id(Ogml::a_none), values() { }

	OgmlAttribute(int identifier) : values() {
		if(identifier >= 0 && identifier < Ogml::ATT_NUM) this->id = identifier;
		else this->id = Ogml::a_none;
	}

	// Destruction
	~OgmlAttribute() { }

	// Getter
	const int& getId() const { return id; }
	const string& getName() const { return Ogml::s_attributeNames[id]; }
	const List<OgmlAttributeValue*>& getValueList() const { return values; }

	// Setter
	void setId(int identifier) {
		if(identifier >= 0 && identifier < Ogml::ATT_NUM) this->id = identifier;
		else this->id = Ogml::a_none;
	}

	/**
	 * Pushes pointers to OgmlAttributeValue objects back to list values.
	 * These value objects are looked up in hashtable values.
	 */

	template < size_t n >
	void pushValues(Hashing<int, OgmlAttributeValue> *val, int (&keys)[n]) {
		for(size_t i = 0; i < n; ++i) {
			HashElement<int, OgmlAttributeValue> *he = val->lookup(keys[i]);
			if(he != nullptr)
				values.pushBack( &(he->info()) );
		}
	}

	void pushValue(Hashing<int, OgmlAttributeValue> *val, int key) {
		HashElement<int, OgmlAttributeValue> *he = val->lookup(key);
		if(he != nullptr)
			values.pushBack( &(he->info()) );
	}

	// Prints the value set of the attribute.
	void print(std::ostream &os) const {
		os << "\"" << getName() << "\"={ ";
		for (OgmlAttributeValue *val : values) {
			os << val->getValue() << " ";
		}
		os << "}\n";
	}

	/**This method proofs whether o is a valid attribute in comparison
	* to this object.
	* That means if the name of o and this object are equal and if
	* o has a valid value.
	* It returns a validity state code (see Ogml.h).
	**/
	int validAttribute(const XmlAttributeObject &xmlAttribute,
		const XmlTagObject* xmlTag,
		Hashing<string, const XmlTagObject*>& ids) const
	{
		if (xmlAttribute.getName() != getName()) { // does this ever happen?
			return Ogml::vs_invalid;
		}
		for (OgmlAttributeValue *val : values) {
			int valid;
			if ((valid = val->validValue(xmlAttribute.getValue(), xmlTag, ids)) < 0) {
				return valid;
			}
		}
		return Ogml::vs_valid;
	}
};

//! Objects of this class represent a tag in %Ogml with attributes.
class OgmlParser::OgmlTag
{
	int id; //!< Integer identifier of object; for possible ids see Ogml.h.

	int minOccurs, maxOccurs; // Min. occurs and max. occurs of this tag.

	/**
	 * Flag denotes whether tag content can be ignored.
	 * It is possible to exchange this flag by a list of contents for more
	 * complex purposes ;-)
	 */
	bool ignoreContent;

	List<OgmlParser::OgmlAttribute*> compulsiveAttributes; //!< Represents the compulsive attributes of this object.
	List<OgmlAttribute*> choiceAttributes; //!< Represents the attributes of this object of which at least one needs to exist.
	List<OgmlAttribute*> optionalAttributes; //!< Represents the optional attributes of this object.

	List<OgmlTag*> compulsiveTags;
	List<OgmlTag*> choiceTags;
	List<OgmlTag*> optionalTags;

	void printOwnedTags(std::ostream &os, Mode mode) const
	{
		if(mode < 0 || mode > 2)
			return;  // unsupported mode

		string s;
		const List<OgmlTag*> *list = nullptr;
		switch(mode) {
		case compMode:
			list = &compulsiveTags;
			s += "compulsive";
			break;
		case choiceMode:
			list = &choiceTags;
			s += "selectable";
			break;
		case optMode:
			list = &optionalTags;
			s += "optional";
			break;
		}

		if (list->empty())
			os << "Tag \"<" << getName() <<">\" does not include " << s << " tag(s).\n";
		else {
			os << "Tag \"<" << getName() <<">\" includes the following " << s << " tag(s): \n";
			for (OgmlTag *currTag : *list) {
				os << "\t<" << currTag->getName() << ">\n";
			}
		}
	}

	void printOwnedAttributes(std::ostream &os, Mode mode) const
	{
		if(mode < 0 || mode > 2)
			return;  // unsupported mode

		string s;
		const List<OgmlAttribute*> *list = nullptr;

		switch(mode)
		case compMode: {
			list = &compulsiveAttributes;
			s += "compulsive";
			break;
		case choiceMode:
			list = &choiceAttributes;
			s += "selectable";
			break;
		case optMode:
			list = &optionalAttributes;
			s += "optional";
			break;
		}

		if(list->empty())
			os << "Tag \"<" << getName() <<">\" does not include " << s << " attribute(s).\n";
		else {
			GraphIO::logger.lout(Logger::Level::Minor) << "Tag \"<" << getName() <<">\" includes the following " << s << " attribute(s): \n";
			for (OgmlAttribute *currAtt : *list) {
				os << "\t"  << *currAtt;
			}
		}
	}


public:

	bool hasChoiceTags() {
		return !choiceTags.empty();
	}

	const List<OgmlTag*>& getCompulsiveTags() const { return compulsiveTags; }

	const List<OgmlTag*>& getChoiceTags() const { return choiceTags; }

	const List<OgmlTag*>& getOptionalTags() const { return optionalTags; }


	const int& getMinOccurs() const { return minOccurs; }

	const int& getMaxOccurs() const { return maxOccurs; }

	const bool& ignoresContent() const { return ignoreContent; }

	void setMinOccurs(int occurs) { minOccurs = occurs; }

	void setMaxOccurs(int occurs) { maxOccurs = occurs; }

	void setIgnoreContent(bool ignore) { ignoreContent = ignore; }

	//Construction
	OgmlTag()
	 : id(Ogml::t_none)
	 , minOccurs(0)
	 , maxOccurs(std::numeric_limits<int>::max())
	 , ignoreContent(0)
	{
	}

	OgmlTag(int identifier)
	 : id(Ogml::t_none)
	 , minOccurs(0)
	 , maxOccurs(std::numeric_limits<int>::max())
	 , ignoreContent(0)
	{
		if (identifier >= 0
		 && identifier < Ogml::TAG_NUM) {
			this->id = identifier;
		} else {
			this->id = Ogml::a_none;
		}
	}

	//Destruction
	~OgmlTag() {}

	//Getter
	const int& getId() const { return id; }
	const string& getName() const { return Ogml::s_tagNames[id]; }

	//Setter
	void setId(int identifier){
		if(identifier >= 0 && identifier < Ogml::TAG_NUM) this->id = identifier;
		else this->id = Ogml::a_none;
	}


	void printOwnedTags(std::ostream& os) const {
		printOwnedTags(os, compMode);
		printOwnedTags(os, choiceMode);
		printOwnedTags(os, optMode);
	}

	void printOwnedAttributes(std::ostream& os) const {
		printOwnedAttributes(os, compMode);
		printOwnedAttributes(os, choiceMode);
		printOwnedAttributes(os, optMode);
	}

	/**Pushes pointers to OgmlAttribute objects back to list reqAttributes.
	* These value objects are looked up in hashtable attrib.
	*/

	template < size_t n >
	void pushAttributes(Mode mode, Hashing<int, OgmlAttribute> *attrib, int (&keys)[n])
	{
		List<OgmlAttribute*> *list = nullptr;
		switch(mode) {
		case compMode:
			list = &compulsiveAttributes;
			break;
		case choiceMode:
			list = &choiceAttributes;
			break;
		case optMode:
			list = &optionalAttributes;
			break;
		}

		for(size_t i = 0; i < n; ++i) {
			HashElement<int, OgmlAttribute>* he = attrib->lookup(keys[i]);
			if(he != nullptr)
				list->pushBack( &(he->info()) );
		}
	}

	void pushAttribute(Mode mode, Hashing<int, OgmlAttribute> *attrib, int key)
	{
		List<OgmlAttribute*> *list = nullptr;
		switch(mode) {
		case compMode:
			list = &compulsiveAttributes;
			break;
		case choiceMode:
			list = &choiceAttributes;
			break;
		case optMode:
			list = &optionalAttributes;
			break;
		}

		HashElement<int, OgmlAttribute>* he = attrib->lookup(key);
		if(he != nullptr)
			list->pushBack( &(he->info()) );
	}


	/**Pushes pointers to OgmlAttribute objects back to list reqAttributes.
	* These value objects are looked up in hashtable tag.
	*/

	template < size_t n >
	void pushTags(Mode mode, Hashing<int, OgmlTag> *tag, int (&keys)[n])
	{
		List<OgmlTag*> *list = nullptr;
		switch(mode) {
		case compMode:
			list = &compulsiveTags;
			break;
		case choiceMode:
			list = &choiceTags;
			break;
		case optMode:
			list = &optionalTags;
			break;
		}

		for(size_t i = 0; i < n; ++i) {
			HashElement<int, OgmlTag>* he = tag->lookup(keys[i]);
			if(he != nullptr)
				list->pushBack( &(he->info()) );
		}
	}

	void pushTag(Mode mode, Hashing<int, OgmlTag> *tag, int key)
	{
		List<OgmlTag*> *list = nullptr;
		switch(mode) {
		case compMode:
			list = &compulsiveTags;
			break;
		case choiceMode:
			list = &choiceTags;
			break;
		case optMode:
			list = &optionalTags;
			break;
		}

		HashElement<int, OgmlTag>* he = tag->lookup(key);
		if(he != nullptr)
			list->pushBack( &(he->info()) );
	}

	/**This method proofs whether o is a valid tag in comparison
	* to this object.
	* That means if the name of o and this object are equal and if
	* the attribute list of o is valid (see also validAttribute(...)
	* in OgmlAttribute.h). Otherwise false.
	*/
	int validTag(const XmlTagObject &o,
		Hashing<string, const XmlTagObject*>& ids) const
	{
		if (o.getName() != getName()) {
			return Ogml::vs_unexpTag;
		}
		if (o.isAttributeLess()) {
			return Ogml::vs_valid;
		}

		// check for compulsive attributes
		for (OgmlAttribute *currAttr : compulsiveAttributes) {
			XmlAttributeObject *att;
			if (!o.findXmlAttributeObjectByName(currAttr->getName(), att)) { // attribute not found
				return Ogml::vs_expAttNotFound;
			}
			int valid;
			if ((valid = currAttr->validAttribute(*att, &o, ids)) < 0) { // attribute invalid
				return valid;
			}
			// attribute is valid
			att->setValid();
		}

		// check for selectable attributes
		bool tookChoice = false;
		for (OgmlAttribute *currAttr : choiceAttributes) {
			XmlAttributeObject *att;
			if (o.findXmlAttributeObjectByName(currAttr->getName(), att)) {
				int valid;
				if ((valid = currAttr->validAttribute(*att, &o, ids)) < 0) {
					return valid;
				}
				tookChoice = true;
				att->setValid();
			}
		}
		if (!choiceAttributes.empty() && !tookChoice)
			return Ogml::vs_expAttNotFound;

		// check for optional attributes
		for (OgmlAttribute *currAttr : optionalAttributes) {
			XmlAttributeObject *att;
			if (o.findXmlAttributeObjectByName(currAttr->getName(), att)) {
				int valid;
				if ((valid = currAttr->validAttribute(*att, &o, ids)) < 0) {
					return valid;
				}
				att->setValid();
			}
		}

		// check for remaining (invalid) attributes
		for (XmlAttributeObject *att = o.m_pFirstAttribute; att; att = att->m_pNextAttribute) {
			if (!att->valid())
				return Ogml::vs_unexpAtt;
		}
		return Ogml::vs_valid;
	}
};

OgmlParser::~OgmlParser()
{
	delete m_tags;
	delete m_attributes;
	delete m_attValues;
}

OgmlParser::OgmlParser()
{
	m_tags       = new Hashing < int, OgmlParser::OgmlTag >;
	m_attributes = new Hashing < int, OgmlParser::OgmlAttribute >;
	m_attValues  = new Hashing < int, OgmlParser::OgmlAttributeValue >;

	// Create OgmlAttributeValue objects and fill hashtable m_attValues.

	for (int i = 0; i < Ogml::ATT_VAL_NUM; i++)
		m_attValues->fastInsert(i, OgmlAttributeValue(i));

	for (int i = 0; i < Ogml::ATT_NUM; i++)
		m_attributes->fastInsert(i, OgmlAttribute(i));


	// Create OgmlAttribute objects and fill hashtable attributes.

	int textAlignValues[] = { Ogml::av_left, Ogml::av_center, Ogml::av_right, Ogml::av_justify };

	int verticalAlignValues[] = { Ogml::av_top, Ogml::av_middle, Ogml::av_bottom };

	int nLineTypeValues[] = {
		Ogml::av_groove,
		Ogml::av_ridge,
		Ogml::av_inset,
		Ogml::av_outset,
		Ogml::av_none,
		Ogml::av_solid,
		Ogml::av_dash,
		Ogml::av_dot,
		Ogml::av_dashDot,
		Ogml::av_dashDotDot };

	int nShapeTypeValues[] = {
		Ogml::av_rect,
		Ogml::av_roundedRect,
		Ogml::av_ellipse,
		Ogml::av_triangle,
		Ogml::av_invTriangle,
		Ogml::av_pentagon,
		Ogml::av_hexagon,
		Ogml::av_octagon,
		Ogml::av_rhomb,
		Ogml::av_trapeze,
		Ogml::av_invTrapeze,
		Ogml::av_parallelogram,
		Ogml::av_invParallelogram,
		Ogml::av_image };

	int decorationValues[] = {
		Ogml::av_underline,
		Ogml::av_overline,
		Ogml::av_lineThrough,
		Ogml::av_none };

	int endpointIdRefValues[] = { Ogml::av_pointIdRef, Ogml::av_sourceIdRef, Ogml::av_targetIdRef };

	int patternValues[] = {
		Ogml::av_solid,
		Ogml::av_noFill,
		Ogml::av_dense1,
		Ogml::av_dense2,
		Ogml::av_dense3,
		Ogml::av_dense4,
		Ogml::av_dense5,
		Ogml::av_dense6,
		Ogml::av_dense7,
		Ogml::av_hor,
		Ogml::av_ver,
		Ogml::av_cross,
		Ogml::av_bDiag,
		Ogml::av_fDiag,
		Ogml::av_diagCross};

	int stretchValues[] = {
		Ogml::av_ultraCondensed,
		Ogml::av_extraCondensed,
		Ogml::av_condensed,
		Ogml::av_semiCondensed,
		Ogml::av_regular,
		Ogml::av_semiExpanded,
		Ogml::av_expanded,
		Ogml::av_extraExpanded,
		Ogml::av_ultraExpanded };

	int styleValues[] = { Ogml::av_normal, Ogml::av_italic, Ogml::av_oblique };

	int transformValues[] = { Ogml::av_capitalize, Ogml::av_uppercase, Ogml::av_lowercase, Ogml::av_none };

	int typeValues[] = {
		Ogml::av_box,
		Ogml::av_circle,
		Ogml::av_rhomb,
		Ogml::av_triangle,
		Ogml::av_oBox,
		Ogml::av_oCircle,
		Ogml::av_oRhomb,
		Ogml::av_oTriangle,
		Ogml::av_arrow,
		Ogml::av_vee,
		Ogml::av_tee,
		Ogml::av_none };

	int variantValues[] = {Ogml::av_normal, Ogml::av_smallCaps };

	int weightValues[] = {
		Ogml::av_light,
		Ogml::av_normal,
		Ogml::av_demiBold,
		Ogml::av_bold,
		Ogml::av_black,
		Ogml::av_int };

	int constraintTypeValues[] = { Ogml::av_constraintAlignment, Ogml::av_constraintAnchor, Ogml::av_constraintSequence };

	for (int i = 0; i < Ogml::ATT_NUM; i++) {

		OgmlAttribute &att = m_attributes->lookup(i)->info();

		switch (i) {

		case Ogml::a_xmlns:
			att.pushValue(m_attValues, Ogml::av_any);
			break;

		case Ogml::a_textAlign:
			att.pushValues(m_attValues, textAlignValues);
			break;

		case Ogml::a_verticalAlign:
			att.pushValues(m_attValues, verticalAlignValues);
			break;

		case Ogml::a_angle:
			att.pushValue(m_attValues, Ogml::av_int);
			break;

		case Ogml::a_color:
			att.pushValue(m_attValues, Ogml::av_hex);
			break;

		case Ogml::a_decoration:
			att.pushValues(m_attValues, decorationValues);
			break;

		case Ogml::a_defaultEdgeTemplate:
			att.pushValue(m_attValues, Ogml::av_any);
			break;

		case Ogml::a_defaultLabelTemplate:
			att.pushValue(m_attValues, Ogml::av_any);
			break;

		case Ogml::a_defaultNodeTemplate:
			att.pushValue(m_attValues, Ogml::av_any);
			break;

		case Ogml::a_family:
			att.pushValue(m_attValues, Ogml::av_any);
			break;

		case Ogml::a_height:
			att.pushValue(m_attValues, Ogml::av_num);
			break;

		case Ogml::a_id:
			att.pushValue(m_attValues, Ogml::av_id);
			break;

		case Ogml::a_nodeIdRef:
			att.pushValue(m_attValues, Ogml::av_nodeIdRef);
			break;

		case Ogml::a_edgeIdRef:
			att.pushValue(m_attValues, Ogml::av_edgeIdRef);
			break;

		case Ogml::a_labelIdRef:
			att.pushValue(m_attValues, Ogml::av_labelIdRef);
			break;

		case Ogml::a_sourceIdRef:
			att.pushValue(m_attValues, Ogml::av_nodeIdRef);
			break;

		case Ogml::a_targetIdRef:
			att.pushValue(m_attValues, Ogml::av_nodeIdRef);
			break;

		case Ogml::a_nodeStyleTemplateIdRef:
			att.pushValue(m_attValues, Ogml::av_nodeStyleTemplateIdRef);
			break;

		case Ogml::a_edgeStyleTemplateIdRef:
			att.pushValue(m_attValues, Ogml::av_edgeStyleTemplateIdRef);
			break;

		case Ogml::a_labelStyleTemplateIdRef:
			att.pushValue(m_attValues, Ogml::av_labelStyleTemplateIdRef);
			break;

		case Ogml::a_endpointIdRef:
			att.pushValues(m_attValues, endpointIdRefValues);
			break;

		case Ogml::a_name:
			att.pushValue(m_attValues, Ogml::av_any);
			break;

		// attribute type of subelement line of tag nodeStyleTemplate
		case Ogml::a_nLineType:
			att.pushValues(m_attValues, nLineTypeValues);
			break;

		// attribute type of subelement shape of tag nodeStyleTemplate
		case Ogml::a_nShapeType:
			att.pushValues(m_attValues, nShapeTypeValues);
			break;

		case Ogml::a_pattern:
			att.pushValues(m_attValues, patternValues);

			break;

		case Ogml::a_patternColor:
			att.pushValue(m_attValues, Ogml::av_hex);
			break;

		case Ogml::a_rotation:
			att.pushValue(m_attValues, Ogml::av_int);
			break;

		case Ogml::a_size:
			att.pushValue(m_attValues, Ogml::av_int);
			break;

		case Ogml::a_stretch:
			att.pushValues(m_attValues, stretchValues);
			break;

		case Ogml::a_style:
			att.pushValues(m_attValues, styleValues);
			break;

		case Ogml::a_transform:
			att.pushValues(m_attValues, transformValues);
			break;

		// attribute type of subelements source-/targetStyle of tag edgeStyleTemplate
		case Ogml::a_type:
			att.pushValues(m_attValues, typeValues);
			break;

		case Ogml::a_uri:
			att.pushValue(m_attValues, Ogml::av_uri);
			break;

		case Ogml::a_intValue:
			att.pushValue(m_attValues, Ogml::av_int);
			break;

		case Ogml::a_numValue:
			att.pushValue(m_attValues, Ogml::av_num);
			break;

		case Ogml::a_boolValue:
			att.pushValue(m_attValues, Ogml::av_bool);
			break;

		case Ogml::a_variant:
			att.pushValues(m_attValues, variantValues);
			break;

		case Ogml::a_weight:
			att.pushValues(m_attValues, weightValues);
			break;

		case Ogml::a_width:
			att.pushValue(m_attValues, Ogml::av_num);
			break;

		case Ogml::a_x:
			att.pushValue(m_attValues, Ogml::av_num);
			break;

		case Ogml::a_y:
			att.pushValue(m_attValues, Ogml::av_num);
			break;

		case Ogml::a_z:
			att.pushValue(m_attValues, Ogml::av_num);
			break;

		case Ogml::a_constraintType:
			att.pushValues(m_attValues, constraintTypeValues);
			break;

		case Ogml::a_disabled:
			att.pushValue(m_attValues, Ogml::av_bool);
			break;
		}
	}


	// Create OgmlTag objects and fill hashtable tags.

	for (int i = 0; i < Ogml::TAG_NUM; i++)
		m_tags->fastInsert(i, OgmlTag(i));


	// Create tag relations.

	int constraintChoiceAttrs[] = { Ogml::a_id, Ogml::a_name, Ogml::a_disabled };
	int endpointOptAttrs[] = { Ogml::a_type, Ogml::a_color, Ogml::a_size };
	int fillCompAttrs[] = { Ogml::a_color, Ogml::a_pattern, Ogml::a_patternColor };
	int fontOptAttrs[] = { Ogml::a_style, Ogml::a_variant, Ogml::a_weight, Ogml::a_stretch, Ogml::a_size, Ogml::a_color };
	int graphStyleChoiceAttrs[] = { Ogml::a_defaultNodeTemplate, Ogml::a_defaultEdgeTemplate, Ogml::a_defaultLabelTemplate };
	int lineChoiceAttrs[] = { Ogml::a_nLineType, Ogml::a_width, Ogml::a_color };
	int locationCompAttrs[] = { Ogml::a_x, Ogml::a_y };
	int pointCompAttrs[] = { Ogml::a_id, Ogml::a_x, Ogml::a_y };
	int portCompAttrs[] = { Ogml::a_id, Ogml::a_x, Ogml::a_y };
	int shapeChoiceAttrs[] = { Ogml::a_nShapeType, Ogml::a_width, Ogml::a_height, Ogml::a_uri };
	int sourceTargetStyleChoiceAttrs[] = { Ogml::a_type, Ogml::a_color, Ogml::a_size };
	int textChoiceAttrs[] = { Ogml::a_textAlign, Ogml::a_verticalAlign, Ogml::a_decoration, Ogml::a_transform, Ogml::a_rotation };

	int composedChoiceTags[] = {
		Ogml::t_num, Ogml::t_int, Ogml::t_bool, Ogml::t_string, Ogml::t_nodeRef, Ogml::t_edgeRef, Ogml::t_labelRef, Ogml::t_composed };
	int constraintChoiceTags[] = {
		Ogml::t_num, Ogml::t_int, Ogml::t_bool, Ogml::t_string, Ogml::t_nodeRef, Ogml::t_edgeRef, Ogml::t_labelRef, Ogml::t_composed, Ogml::t_constraint };
	int dataChoiceTags[] = { Ogml::t_int, Ogml::t_bool, Ogml::t_num, Ogml::t_string, Ogml::t_data };
	int edgeChoiceTags[] = { Ogml::t_source, Ogml::t_target };
	int edgeOptTags[] = { Ogml::t_data, Ogml::t_label };
	int edgeStyleChoiceTags[] = {
		Ogml::t_edgeStyleTemplateRef, Ogml::t_line, Ogml::t_sourceStyle, Ogml::t_targetStyle, Ogml::t_point, Ogml::t_segment };
	int edgeStyleTemplateChoiceTags[] = { Ogml::t_line, Ogml::t_sourceStyle, Ogml::t_targetStyle };
	int edgeStyleTemplateOptTags[] = { Ogml::t_data, Ogml::t_edgeStyleTemplateRef };
	int graphOptTags[] = { Ogml::t_layout, Ogml::t_data };
	int labelStyleChoiceTags[] = { Ogml::t_labelStyleTemplateRef, Ogml::t_data, Ogml::t_text, Ogml::t_font, Ogml::t_location };
	int labelStyleTemplateCompTags[] = { Ogml::t_text, Ogml::t_font };
	int labelStyleTemplateOptTags[] = { Ogml::t_data, Ogml::t_labelStyleTemplateRef };
	int layoutOptTags[] = { Ogml::t_data, Ogml::t_styleTemplates, Ogml::t_styles, Ogml::t_constraints };
	int nodeOptTags[] = { Ogml::t_data, Ogml::t_label, Ogml::t_node };
	int nodeStyleChoiceTags[] = { Ogml::t_location, Ogml::t_shape, Ogml::t_fill, Ogml::t_line, Ogml::t_image };
	int nodeStyleOptTags[] = { Ogml::t_data, Ogml::t_nodeStyleTemplateRef };
	int nodeStyleTemplateChoiceTags[] = { Ogml::t_shape, Ogml::t_fill, Ogml::t_line };
	int nodeStyleTemplateOptTags[] = { Ogml::t_data, Ogml::t_nodeStyleTemplateRef };
	int segmentOptTags[] = { Ogml::t_data, Ogml::t_line };
	int sourceOptTags[] = { Ogml::t_data, Ogml::t_label };
	int structureOptTags[] = { Ogml::t_node, Ogml::t_edge, Ogml::t_label, Ogml::t_data };
	int stylesChoiceTags[] = { Ogml::t_nodeStyle, Ogml::t_edgeStyle, Ogml::t_labelStyle };
	int stylesOptTags[] = { Ogml::t_graphStyle, Ogml::t_data };
	int styleTemplatesChoiceTags[] = { Ogml::t_nodeStyleTemplate, Ogml::t_edgeStyleTemplate, Ogml::t_labelStyleTemplate };
	int targetOptTags[] = { Ogml::t_data, Ogml::t_label };

	for (int i = 0; i < Ogml::TAG_NUM; ++i) {
		OgmlTag &tag = m_tags->lookup(i)->info();

		switch (i) {
		case Ogml::t_bool:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_boolValue);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			break;

		case Ogml::t_composed:
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			tag.pushTags(choiceMode, m_tags, composedChoiceTags);
			break;

		case Ogml::t_constraint:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_constraintType);
			tag.pushAttributes(choiceMode, m_attributes, constraintChoiceAttrs);
			tag.pushTags(choiceMode, m_tags, constraintChoiceTags);
			break;

		case Ogml::t_constraints:
			tag.setMaxOccurs(1);
			tag.pushTag(compMode, m_tags, Ogml::t_constraint);
			break;

		case Ogml::t_content:
			tag.setMaxOccurs(1);
			tag.setIgnoreContent(true);
			break;

		case Ogml::t_data:
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			tag.pushTags(choiceMode, m_tags, dataChoiceTags);
			break;

		case Ogml::t_default:
			tag.setMaxOccurs(1);
			break;

		case Ogml::t_edge:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_id);
			tag.pushTags(choiceMode, m_tags, edgeChoiceTags);
			tag.pushTags(optMode, m_tags, edgeOptTags);
			break;

		case Ogml::t_edgeRef:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_edgeIdRef);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			break;

		case Ogml::t_edgeStyle:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_edgeIdRef);
			tag.pushTags(choiceMode, m_tags, edgeStyleChoiceTags);
			tag.pushTag(optMode, m_tags, Ogml::t_data);
			break;

		case Ogml::t_edgeStyleTemplate:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_id);
			tag.pushTags(choiceMode, m_tags, edgeStyleTemplateChoiceTags);
			tag.pushTags(optMode, m_tags, edgeStyleTemplateOptTags);
			break;

		case Ogml::t_endpoint:
			tag.setMinOccurs(2);
			tag.setMaxOccurs(2);
			tag.pushAttribute(compMode, m_attributes, Ogml::a_endpointIdRef);
			tag.pushAttributes(optMode, m_attributes, endpointOptAttrs);
			break;

		case Ogml::t_fill:
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, m_attributes, fillCompAttrs);
			break;

		case Ogml::t_font:
			tag.setMaxOccurs(1);
			tag.pushAttribute(compMode, m_attributes, Ogml::a_family);
			tag.pushAttributes(optMode, m_attributes, fontOptAttrs);
			break;

		case Ogml::t_graph:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(1);
			tag.pushTag(compMode, m_tags, Ogml::t_structure);
			tag.pushTags(optMode, m_tags, graphOptTags);
			break;

		case Ogml::t_graphStyle:
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, m_attributes, graphStyleChoiceAttrs);
			break;

		case Ogml::t_int:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_intValue);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			break;

		case Ogml::t_label:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_id);
			tag.pushTag(compMode, m_tags, Ogml::t_content);
			tag.pushTag(optMode, m_tags, Ogml::t_data);
			break;

		case Ogml::t_labelRef:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_labelIdRef);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			break;

		case Ogml::t_labelStyle:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_labelIdRef);
			tag.pushTags(choiceMode, m_tags, labelStyleChoiceTags);
			break;

		case Ogml::t_labelStyleTemplate:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_id);
			tag.pushTags(compMode, m_tags, labelStyleTemplateCompTags);
			tag.pushTags(optMode, m_tags, labelStyleTemplateOptTags);
			break;

		case Ogml::t_layout:
			tag.setMaxOccurs(1);
			tag.pushTags(optMode, m_tags, layoutOptTags);
			break;

		case Ogml::t_line:
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, m_attributes, lineChoiceAttrs);
			break;

		case Ogml::t_location:
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, m_attributes, locationCompAttrs);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_z);
			break;

		case Ogml::t_node:
			tag.setMinOccurs(1);
			tag.pushAttribute(compMode, m_attributes, Ogml::a_id);
			tag.pushTags(optMode, m_tags, nodeOptTags);
			break;

		case Ogml::t_nodeRef:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_nodeIdRef);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			break;

		case Ogml::t_nodeStyle:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_nodeIdRef);
			tag.pushTags(choiceMode, m_tags, nodeStyleChoiceTags);
			tag.pushTags(optMode, m_tags, nodeStyleOptTags);
			break;


		case Ogml::t_nodeStyleTemplate:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_id);
			tag.pushTags(choiceMode, m_tags, nodeStyleTemplateChoiceTags);
			tag.pushTags(optMode, m_tags, nodeStyleTemplateOptTags);
			break;

		case Ogml::t_num:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_numValue);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			break;

		case Ogml::t_ogml:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(1);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_xmlns);
			tag.pushTag(compMode, m_tags, Ogml::t_graph);
			break;

		case Ogml::t_point:
			tag.pushAttributes(compMode, m_attributes, pointCompAttrs);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_z);
			tag.pushTag(optMode, m_tags, Ogml::t_data);
			break;

		case Ogml::t_port:
			tag.pushAttributes(compMode, m_attributes, portCompAttrs);
			break;

		case Ogml::t_segment:
			tag.pushTag(compMode, m_tags, Ogml::t_endpoint);
			tag.pushTags(optMode, m_tags, segmentOptTags);
			break;

		case Ogml::t_shape:
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, m_attributes, shapeChoiceAttrs);
			// comment (BZ): uri is obsolete, images got an own tag
			break;

		case Ogml::t_source:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_sourceIdRef);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_id);
			tag.pushTags(optMode, m_tags, sourceOptTags);
			break;

		case Ogml::t_sourceStyle:
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, m_attributes, sourceTargetStyleChoiceAttrs);
			break;

		case Ogml::t_string:
			tag.pushAttribute(optMode, m_attributes, Ogml::a_name);
			tag.setIgnoreContent(true);
			break;

		case Ogml::t_structure:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(1);
			tag.pushTags(optMode, m_tags, structureOptTags);
			break;

		case Ogml::t_styles:
			tag.setMaxOccurs(1);
			tag.pushTags(choiceMode, m_tags, stylesChoiceTags);
			tag.pushTags(optMode, m_tags, stylesOptTags);
			break;

		case Ogml::t_styleTemplates:
			tag.setMaxOccurs(1);
			tag.pushTags(choiceMode, m_tags, styleTemplatesChoiceTags);
			tag.pushTag(optMode, m_tags, Ogml::t_data);
			break;

		case Ogml::t_target:
			tag.pushAttribute(compMode, m_attributes, Ogml::a_targetIdRef);
			tag.pushAttribute(optMode, m_attributes, Ogml::a_id);
			tag.pushTags(optMode, m_tags, targetOptTags);
			break;

		case Ogml::t_targetStyle:
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, m_attributes, sourceTargetStyleChoiceAttrs);
			break;

		case Ogml::t_labelStyleTemplateRef:
			tag.setMaxOccurs(1);
			tag.pushAttribute(compMode, m_attributes, Ogml::a_labelStyleTemplateIdRef);
			break;

		case Ogml::t_nodeStyleTemplateRef:
			tag.setMaxOccurs(1);
			tag.pushAttribute(compMode, m_attributes, Ogml::a_nodeStyleTemplateIdRef);
			break;

		case Ogml::t_edgeStyleTemplateRef:
			tag.setMaxOccurs(1);
			tag.pushAttribute(compMode, m_attributes, Ogml::a_edgeStyleTemplateIdRef);
			break;

		case Ogml::t_text:
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, m_attributes, textChoiceAttrs);
			break;
		}
	}
}

int OgmlParser::validate(const XmlTagObject * xmlTag, int ogmlTagId)
{
	// Perhaps xmlTag is already valid
	if (xmlTag->valid())
		return Ogml::vs_valid;

	OgmlTag *ogmlTag = &m_tags->lookup(ogmlTagId)->info();
	if (!ogmlTag) {
		std::cerr << "Did not find tag with id \"" << ogmlTagId << "\" in hashtable in OgmlParser::validate! Aborting.\n";
		return Ogml::vs_unexpTag;
	}

	// abort if tag is not valid
	int valid;
	if ((valid = ogmlTag->validTag(*xmlTag, m_ids)) < 0) {
#ifdef OGDF_DEBUG
		this->printValidityInfo(*ogmlTag, *xmlTag, valid, __LINE__);
#endif
		return valid;
	}

	// if tag ignores its content simply return
	if (ogmlTag->ignoresContent()) {
		xmlTag->setValid();
#ifdef OGDF_DEBUG
		this->printValidityInfo(*ogmlTag, *xmlTag, Ogml::vs_valid, __LINE__);
#endif
		return Ogml::vs_valid;
	}

	// check if all required son tags exist
	for (OgmlTag *currTag : ogmlTag->getCompulsiveTags()) {
		int cnt = 0;

		// search for untested sons
		for (XmlTagObject *sonTag = xmlTag->m_pFirstSon; sonTag; sonTag = sonTag->m_pBrother) {
			if (sonTag->getName() == currTag->getName()) {
				++cnt;
				if ((valid = validate(sonTag, currTag->getId())) < 0) {
					return valid;
				}
			}
		}

		// Exp. son not found
		if (cnt == 0) {
#ifdef OGDF_DEBUG
			this->printValidityInfo(*ogmlTag, *xmlTag, Ogml::vs_expTagNotFound, __LINE__);
#endif
			return Ogml::vs_expTagNotFound;
		}

		// Check cardinality
		if (cnt < currTag->getMinOccurs()
		 || cnt > currTag->getMaxOccurs()) {
#ifdef OGDF_DEBUG
			this->printValidityInfo(*currTag, *xmlTag, Ogml::vs_cardErr, __LINE__);
#endif
			return Ogml::vs_cardErr;
		}
	}

	// Check if choice son tags exist
	if (ogmlTag->hasChoiceTags()) {
		bool tookChoice = false;

		// find all obligatoric sons: all obligatoric sons
		for (OgmlTag *currTag : ogmlTag->getChoiceTags())
		{
			int cnt = 0;

			// search for untested sons
			for (XmlTagObject *sonTag = xmlTag->m_pFirstSon; sonTag; sonTag = sonTag->m_pBrother) {
				if (sonTag->getName() == currTag->getName()) {
					tookChoice = true;
					++cnt;
					if ((valid = validate(sonTag, currTag->getId())) < 0) {
						return valid;
					}
				}
			}

			// Check cardinality
			if (cnt > 0
			 && (cnt < currTag->getMinOccurs()
			  || cnt > currTag->getMaxOccurs())) {
#ifdef OGDF_DEBUG
				this->printValidityInfo(*currTag, *xmlTag, Ogml::vs_cardErr, __LINE__);
#endif
				return Ogml::vs_cardErr;
			}
		}
		if ((!tookChoice)
		 && (xmlTag->m_pFirstSon)) {
#ifdef OGDF_DEBUG
			this->printValidityInfo(*ogmlTag, *xmlTag, Ogml::vs_tagEmptIncl, __LINE__);
#endif
			return Ogml::vs_tagEmptIncl;
		}
#if 0
		// this code is not valid, as iterator it (in previous code) must be invalid at this point!
		if ((!tookChoice) && (xmlTag->m_pFirstSon)) {
			this->printValidityInfo((**it), *xmlTag, valid = Ogml::vs_tagEmptIncl, __LINE__);
			return valid;
		}
#endif

	}	//Check choice son tags

	// find all optional sons: all optional sons
	for (OgmlTag *currTag : ogmlTag->getOptionalTags()) {
		int cnt = 0;

		// search for untested sons
		for (XmlTagObject *sonTag = xmlTag->m_pFirstSon; sonTag; sonTag = sonTag->m_pBrother) {
			if (sonTag->getName() == currTag->getName()) {
				++cnt;
				if ((valid = validate(sonTag, currTag->getId())) < 0) {
					return valid;
				}
			}
		}

		// Check cardinality
#if 0
		if( (cnt<currTag->getMinOccurs() || cnt>currTag->getMaxOccurs()) )
#else
		if (cnt > currTag->getMaxOccurs()) {
#endif
#ifdef OGDF_DEBUG
			this->printValidityInfo(*currTag, *xmlTag, Ogml::vs_cardErr, __LINE__);
#endif
			return Ogml::vs_cardErr;
		}
	}

	// Are there invalid son tags left?
	for (XmlTagObject *sonTag = xmlTag->m_pFirstSon; sonTag; sonTag = sonTag->m_pBrother) {
		if (!sonTag->valid()) {
#ifdef OGDF_DEBUG
			this->printValidityInfo(*ogmlTag, *xmlTag, Ogml::vs_unexpTag, __LINE__);
#endif
			return Ogml::vs_unexpTag;
		}
	}

	// Finally xmlTag is valid :-)
	xmlTag->setValid();

#ifdef OGDF_DEBUG
	this->printValidityInfo(*ogmlTag, *xmlTag, Ogml::vs_valid, __LINE__);
#endif
	return Ogml::vs_valid;
}

std::ostream& operator<<(std::ostream& os, const OgmlParser::OgmlAttribute& oa)
{
	oa.print(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, const OgmlParser::OgmlTag& ot)
{
	ot.printOwnedTags(os);
	ot.printOwnedAttributes(os);
	return os;
}


void OgmlParser::printValidityInfo(const OgmlTag & ot, const XmlTagObject & xto, int valStatus, int line)
{
	const string &ogmlTagName = ot.getName();

	switch (valStatus) {

	case Ogml::vs_tagEmptIncl:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" expects tag(s) to include! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		ot.printOwnedTags(GraphIO::logger.lout());
		break;

	case Ogml::vs_idNotUnique:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" owns already assigned id! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		break;

	case Ogml::vs_idRefErr:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" references unknown or wrong id! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		break;

	case Ogml::vs_unexpTag:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" owns unexpected tag! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		ot.printOwnedTags(GraphIO::logger.lout());
		break;

	case Ogml::vs_unexpAtt:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" owns unexpected attribute(s)! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		ot.printOwnedAttributes(GraphIO::logger.lout());
		break;

	case Ogml::vs_expTagNotFound:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" does not own compulsive tag(s)! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		ot.printOwnedTags(GraphIO::logger.lout());
		break;

	case Ogml::vs_expAttNotFound:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" does not own compulsive attribute(s)! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		ot.printOwnedAttributes(GraphIO::logger.lout());
		break;

	case Ogml::vs_attValueErr:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" owns attribute with wrong value! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		ot.printOwnedAttributes(GraphIO::logger.lout());
		break;

	case Ogml::vs_cardErr:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" occurence exceeds the number of min. ("
		  << ot.getMinOccurs() << ") or max. (" << ot.getMaxOccurs() << ") occurences in its context! "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		break;

	case Ogml::vs_invalid:
		GraphIO::logger.lout() << "Tag \"<" << ogmlTagName << ">\" is invalid! No further information available. "
		  "(Input source line: " << xto.getLine() << ", recursion depth: " << xto.getDepth() << ")" << std::endl;
		GraphIO::logger.lout() << ot;
		break;

	case Ogml::vs_valid:
		//std::cout << "INFO: tag \"<" << ogmlTagName << ">\" is valid :-) ";
		//std::cout << "(Input source line: " << xto.
		//	getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		break;
	}

#ifdef OGDF_DEBUG
	if(valStatus != Ogml::vs_valid)
		GraphIO::logger.lout() << "(Line OgmlParser::validate: " << line << ")\n";
#endif
}

bool OgmlParser::isGraphHierarchical(const XmlTagObject *xmlTag) const
{
	if(xmlTag->getName() == Ogml::s_tagNames[Ogml::t_node] && isNodeHierarchical(xmlTag))
		return true;

	// Depth-Search only if ret!=true
	if(xmlTag->m_pFirstSon && isGraphHierarchical(xmlTag->m_pFirstSon))
		return true;

	// Breadth-Search only if ret!=true
	return xmlTag->m_pBrother && isGraphHierarchical(xmlTag->m_pBrother);
}

bool OgmlParser::isNodeHierarchical(const XmlTagObject *xmlTag) const
{
	bool ret = false;
	if(xmlTag->getName() == Ogml::s_tagNames[Ogml::t_node]) {

		XmlTagObject* dum;
		// check if an ancestor is a node
		ret = xmlTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_node], dum);
	}

	return ret;
}

bool OgmlParser::checkGraphType(const XmlTagObject *xmlTag) const
{
	if(xmlTag->getName() != Ogml::s_tagNames[Ogml::t_ogml]) {
		GraphIO::logger.lout() << "Expecting root tag \"" << Ogml::s_tagNames[Ogml::t_ogml] << "\" in OgmlParser::checkGraphType!" << std::endl;
		return false;
	}

	// Normal graph present
	if(!isGraphHierarchical(xmlTag)) {
		m_graphType = Ogml::graph;
		return true;
	}

	// Cluster-/Compound graph present
	m_graphType = Ogml::clusterGraph;

	// Traverse the parse tree and collect all edge tags
	List<const XmlTagObject*> edges;
	if(xmlTag->getName() == Ogml::s_tagNames[Ogml::t_edge]) edges.pushBack(xmlTag);
	XmlTagObject* son = xmlTag->m_pFirstSon;
	while(son) {
		if(son->getName() == Ogml::s_tagNames[Ogml::t_edge]) edges.pushBack(son);
		son = son->m_pBrother;
	}

	// Cluster graph already present
	if(edges.empty()) return true;

	// Traverse edges
	for (ListConstIterator<const XmlTagObject*> edgeIt = edges.begin(); edgeIt.valid() && m_graphType != Ogml::compoundGraph; ++edgeIt) {
		// Traverse the sources/targets.
		// Parse tree is valid so one edge contains at least one source/target
		// with idRef attribute
		for (son = (*edgeIt)->m_pFirstSon; son; son = son->m_pBrother) {
			XmlAttributeObject* att;
			if (son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att)) {
				const XmlTagObject *refTag = m_ids.lookup(att->getValue())->info();
				if(isNodeHierarchical(refTag)) {
					m_graphType = Ogml::compoundGraph;
					return true;
				}
			}
		}
	}

	return true;
}

// Mapping Fill Pattern
FillPattern OgmlParser::getFillPattern(string s)
{
	if (s == Ogml::s_attributeValueNames[Ogml::av_noFill])
		return FillPattern::None;
	if (s == Ogml::s_attributeValueNames[Ogml::av_solid])
		return FillPattern::Solid;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense1])
		return FillPattern::Dense1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense2])
		return FillPattern::Dense2;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense3])
		return FillPattern::Dense3;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense4])
		return FillPattern::Dense4;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense5])
		return FillPattern::Dense5;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense6])
		return FillPattern::Dense6;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dense7])
		return FillPattern::Dense7;
	if (s == Ogml::s_attributeValueNames[Ogml::av_hor])
		return FillPattern::Horizontal;
	if (s == Ogml::s_attributeValueNames[Ogml::av_ver])
		return FillPattern::Vertical;
	if (s == Ogml::s_attributeValueNames[Ogml::av_cross])
		return FillPattern::Cross;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bDiag])
		return FillPattern::BackwardDiagonal;
	if (s == Ogml::s_attributeValueNames[Ogml::av_fDiag])
		return FillPattern::ForwardDiagonal;
	if (s == Ogml::s_attributeValueNames[Ogml::av_diagCross])
		return FillPattern::DiagonalCross;
	// default return solid
	return FillPattern::Solid;
}


// Mapping Shape to Integer
Shape OgmlParser::getShape(string s)
{
	if (s == "roundedRect")
		return Shape::RoundedRect;
	else if(s == "ellipse")
		return Shape::Ellipse;
	else if(s == "triangle")
		return Shape::Triangle;
	else if(s == "pentagon")
		return Shape::Pentagon;
	else if(s == "hexagon")
		return Shape::Hexagon;
	else if(s == "octagon")
		return Shape::Octagon;
	else if(s == "rhomb")
		return Shape::Rhomb;
	else if(s == "trapeze")
		return Shape::Trapeze;
	else if(s == "parallelogram")
		return Shape::Parallelogram;
	else if(s == "invTriangle")
		return Shape::InvTriangle;
	else if(s == "invTrapeze")
		return Shape::InvTrapeze;
	else if(s == "invParallelogram")
		return Shape::InvParallelogram;
	else if(s == "image")
		return Shape::Image;

	// default return rectangle
	return Shape::Rect;
}


// Mapping OgmlNodeShape to OGDF::NodeTemplate
string OgmlParser::getNodeTemplateFromOgmlValue(string s)
{
	// Mapping OGML-Values to ogdf
	if (s == Ogml::s_attributeValueNames[Ogml::av_rect])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_roundedRect])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_triangle])
		s = "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_invTriangle])
		s = "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_ellipse])
		return "ogdf:std:ellipse";
	if (s == Ogml::s_attributeValueNames[Ogml::av_hexagon])
		return "ogdf:std:hexagon";
	if (s == Ogml::s_attributeValueNames[Ogml::av_rhomb])
		return "ogdf:std:rhombus";
	if (s == Ogml::s_attributeValueNames[Ogml::av_trapeze])
		return "ogdf:std:trapeze";
	if (s == Ogml::s_attributeValueNames[Ogml::av_invTrapeze])
		return "ogdf:std:trapeze";
	if (s == Ogml::s_attributeValueNames[Ogml::av_parallelogram])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_invParallelogram])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_pentagon])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_octagon])
		return"ogdf:std:rect";
	// default
	return "ogdf:std:rect";
}


// Mapping Line type to Integer
StrokeType OgmlParser::getStrokeType(string s)
{
	if (s == Ogml::s_attributeValueNames[Ogml::av_none])
		return StrokeType::None;
	if (s == Ogml::s_attributeValueNames[Ogml::av_solid])
		return StrokeType::Solid;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dash])
		return StrokeType::Dash;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dot])
		return StrokeType::Dot;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dashDot])
		return StrokeType::Dashdot;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dashDotDot])
		return StrokeType::Dashdotdot;

#if 0
	// Mapping OGML-Values to ogdf
	// solid | dotted | dashed | double | triple
	//		 | groove | ridge | inset | outset | none
	if (s == Ogml::s_attributeValueNames[Ogml::av_groove])
		return 5;
	if (s == Ogml::s_attributeValueNames[Ogml::av_ridge])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_inset])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_outset])
		return 1;
	default return bpSolid
#endif
	return StrokeType::Solid;
}


// Mapping ArrowStyles to Integer
int OgmlParser::getArrowStyleAsInt(string s)
{
	// TODO: Complete, if new arrow styles are implemented in ogdf
	if (s == "none")
		return 0;
	else
		return 1;
	// default return 0
	return 0;
}


// Mapping ArrowStyles to EdgeArrow
EdgeArrow OgmlParser::getArrowStyle(int i)
{
	switch (i){
	case 0:
		return EdgeArrow::None;
		break;
	case 1:
		return EdgeArrow::Last;
		break;
	case 2:
		return EdgeArrow::First;
		break;
	case 3:
		return EdgeArrow::Both;
		break;
	default:
		return EdgeArrow::Last;
	}
}



// returns the string with "<" substituted for "&lt;"
//  and ">" substituted for "&gt;"
string OgmlParser::getLabelCaptionFromString(string str)
{
	string output;
	size_t i=0;
	while (i<str.length())
	{
		if (str[i] == '&')
		{
			if (i+3 < str.length())
			{
				if ((str[i+1] == 'l') && (str[i+2] == 't') && (str[i+3] == ';')){
					// found char sequence "&lt;"
					output += "<";
				} else {
					if ((str[i+1] == 'g') && (str[i+2] == 't') && (str[i+3] == ';')){
						// found char sequence "&gt;"
						// \n newline is required!!!
						output += ">\n";
					}
				}
				i = i + 4;
			}
		} else {
			char c = str[i];
			output += c;
			i++;
		}
	}
	str += "\n";
	return output;
}


// returns the integer value of the id at the end of the string - if existent
// the return value is 'id', the boolean return value is for checking existance of an integer value
//
// why do we need such a function?
// in OGML every id is globally unique, so we write a char-prefix
// to the ogdf-id's ('n' for node, 'e' for edge, ...)
bool OgmlParser::getIdFromString(string str, int &id)
{
	if (str.length() == 0)
		return false;

	string strId;
	size_t i=0;
	while (i<str.length()) {
		// if act char is a digit append it to the strId
		if (isdigit(str[i]))
			strId += str[i];
		i++;
	}

	if (strId.length() == 0)
		return false;

	// transform str to int
	id = std::stoi(strId);
	return true;
}


bool OgmlParser::addAttributes(
	Graph &G,
	GraphAttributes &GA,
	ClusterGraphAttributes *pCGA,
	const XmlTagObject *root)
{
	HashConstIterator<string, const XmlTagObject*> it;

	if(!root) {
		GraphIO::logger.lout(Logger::Level::Minor) << "Cannot determine layout information, no parse tree available!" << std::endl;
	} else {
		// root tag isn't a nullptr pointer... let's start...
		XmlTagObject* son = root->m_pFirstSon;
		// first traverse to the structure- and the layout block
		if (son->getName() != Ogml::s_tagNames[Ogml::t_graph]){
			while (son->getName() != Ogml::s_tagNames[Ogml::t_graph]){
				son = son->m_pFirstSon;
				if (!son){
					// wrong rootTag given or graph tag wasn't found
					return false;
				}
			}
		}

		// now son is the graph tag which first child is structure
		XmlTagObject* structure = son->m_pFirstSon;
		if (structure->getName() != Ogml::s_tagNames[Ogml::t_structure]){
			return false;
		}
		// now structure is what it is meant to be
		// traverse the children of structure
		// and set the labels
		son = structure->m_pFirstSon;
		while(son)
		{
			//Set labels of nodes
			if ((son->getName() == Ogml::s_tagNames[Ogml::t_node]) && GA.has(GraphAttributes::nodeLabel))
			{
				if (!isNodeHierarchical(son))
				{
					// get the id of the actual node
					XmlAttributeObject *att;
					if(son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
					{
						// lookup for node
						node actNode = (m_nodes.lookup(att->getValue()))->info();
						// find label tag
						XmlTagObject* label;
						if (son->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label))
						{
							// get content tag
							XmlTagObject* content = label->m_pFirstSon;
							// get the content as string
							if (content->m_pTagValue) {
								string str = content->getValue();
								string labelStr = getLabelCaptionFromString(str);
								// now set the label of the node
								GA.label(actNode) = labelStr;
							}
						}
					}
				}
				else
				{
					// get the id of the actual cluster
					XmlAttributeObject *att;
					if(pCGA != nullptr && son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
					{
						// lookup for cluster
						cluster actCluster = (m_clusters.lookup(att->getValue()))->info();
						// find label tag
						XmlTagObject* label;
						if (son->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label))
						{
							// get content tag
							XmlTagObject* content = label->m_pFirstSon;
							// get the content as string
							if (content->m_pTagValue) {
								string str = content->getValue();
								string labelStr = getLabelCaptionFromString(str);
								// now set the label of the node
								pCGA->label(actCluster) = labelStr;
							}
						}
					}
					// hierSon = hierarchical Son
					XmlTagObject *hierSon;
					if (son->m_pFirstSon)
					{
						hierSon = son->m_pFirstSon;
						while(hierSon) {
							// recursive call for setting labels of child nodes
							if (!setLabelsRecursive(G, GA, pCGA, hierSon))
								return false;
							hierSon = hierSon->m_pBrother;
						}
					}
				}
			}

			//Set labels of edges
			if ((son->getName() == Ogml::s_tagNames[Ogml::t_edge]) && GA.has(GraphAttributes::edgeLabel))
			{
				// get the id of the actual edge
				XmlAttributeObject *att;
				if (son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
				{
					// lookup for edge
					//  0, if (hyper)edge not read from file
					if(m_edges.lookup(att->getValue())){
						edge actEdge = (m_edges.lookup(att->getValue()))->info();
						// find label tag
						XmlTagObject* label;
						if(son->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label))
						{
							// get content tag
							XmlTagObject* content = label->m_pFirstSon;
							// get the content as string
							if (content->m_pTagValue) {
								string str = content->getValue();
								string labelStr = getLabelCaptionFromString(str);
								// now set the label of the node
								GA.label(actEdge) = labelStr;
							}
						}
					}
				}
			}

			// Labels
#if 0
			// ACTUALLY NOT IMPLEMENTED IN OGDF
			if (son->getName() == Ogml::s_tagNames[t_label]) {
				// get the id of the actual edge
				XmlAttributeObject *att;
				if (son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att)){
					// lookup for label
					label actLabel = (labels.lookup(att->getValue()))->info();
					// get content tag
					XmlTagObject* content = son->m_pFirstSon;
					// get the content as string
					if (content->m_pTagValue){
						string str = content->getValue();
						string labelStr = getLabelCaptionFromString(str);
						// now set the label of the node
						GA.labelLabel(actLabel) = labelStr;
					}
				}
			}
#endif

			// go to the next brother
			son = son->m_pBrother;
		}

		// get the layout tag
		XmlTagObject* layout = nullptr;
		if (structure->m_pBrother != nullptr) {
			layout = structure->m_pBrother;
		}

		if ((layout) && (layout->getName() == Ogml::s_tagNames[Ogml::t_layout]))
		{
			// layout exists

			// first get the styleTemplates
			XmlTagObject *layoutSon;
			if (layout->m_pFirstSon)
			{
				// layout has at least one child-tag
				layoutSon = layout->m_pFirstSon;
				// ->loop through all of them
				while (layoutSon)
				{
					// style templates
					if (layoutSon->getName() == Ogml::s_tagNames[Ogml::t_styleTemplates])
					{
						// has children data, nodeStyleTemplate, edgeStyleTemplate, labelStyleTemplate
						XmlTagObject *styleTemplatesSon;
						if (layoutSon->m_pFirstSon)
						{
							styleTemplatesSon = layoutSon->m_pFirstSon;

							while (styleTemplatesSon)
							{
								// nodeStyleTemplate
								if (styleTemplatesSon->getName() == Ogml::s_tagNames[Ogml::t_nodeStyleTemplate])
								{
									XmlAttributeObject *actAtt;
									if (styleTemplatesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt))
									{
										const string &actKey = actAtt->getValue();
										OgmlNodeTemplate *actTemplate = new OgmlNodeTemplate(actKey); // when will this be deleted?

										XmlTagObject *actTag;

										// template inheritance
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_nodeStyleTemplateRef], actTag))
										{
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeStyleTemplateIdRef], actAtt)) {
												// actual template references another
												// get it from the hash table
												OgmlNodeTemplate *refTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
												if (refTemplate) {
													// the referenced template was inserted into the hash table
													// so copy the values
													string actId = actTemplate->m_id;
													*actTemplate = *refTemplate;
													actTemplate->m_id = actId;
												}
											}
										}

#if 0
										// data
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
											// found data for nodeStyleTemplate
											// no implementation required for ogdf
										}
#endif

										// shape tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_shape], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nShapeType], actAtt)) {
												// TODO: change, if shapes are expanded
												// actually shape and template are calculated from the same value!!!
												actTemplate->m_nodeTemplate = getNodeTemplateFromOgmlValue(actAtt->getValue());
												actTemplate->m_shapeType = getShape(actAtt->getValue());
											}
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												actTemplate->m_width = std::stod(actAtt->getValue());
											// height
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_height], actAtt))
												actTemplate->m_height = std::stod(actAtt->getValue());
											// uri
											//ACTUALLY NOT SUPPORTED
#if 0
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_uri], actAtt))
												GA.uri(actNode) = actAtt->getValue();
#endif
										}

										// fill tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_fill], actTag))
										{
											// fill color
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												actTemplate->m_color = actAtt->getValue();
											// fill pattern
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_pattern], actAtt))
												actTemplate->m_pattern = getFillPattern(actAtt->getValue());
											// fill patternColor
											//TODO: check if pattern color exists
#if 0
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_patternColor], actAtt));
												actTemplate->m_patternColor = actAtt->getValue());
#endif
										}

										// line tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
												actTemplate->m_lineType = getStrokeType(actAtt->getValue());
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												actTemplate->m_lineWidth = std::stof(actAtt->getValue());
											// color
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												actTemplate->m_lineColor = actAtt->getValue();
										}

										//insert actual template into hash table
										m_ogmlNodeTemplates.fastInsert(actKey, actTemplate);
									}
								}

								// edgeStyleTemplate
								if (styleTemplatesSon->getName() == Ogml::s_tagNames[Ogml::t_edgeStyleTemplate])
								{
									XmlAttributeObject *actAtt;
									if (styleTemplatesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt))
									{
										const string &actKey = actAtt->getValue();
										OgmlEdgeTemplate *actTemplate = new OgmlEdgeTemplate(actKey); // when will this be deleted?

										XmlTagObject *actTag;

										// template inheritance
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_edgeStyleTemplateRef], actTag)){
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_edgeStyleTemplateIdRef], actAtt)){
												// actual template references another
												// get it from the hash table
												OgmlEdgeTemplate *refTemplate = m_ogmlEdgeTemplates.lookup(actAtt->getValue())->info();
												if (refTemplate){
													// the referenced template was inserted into the hash table
													// so copy the values
													string actId = actTemplate->m_id;
													*actTemplate = *refTemplate;
													actTemplate->m_id = actId;
												}
											}
										}

#if 0
										// data
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
											// found data for edgeStyleTemplate
											// no implementation required for ogdf
										}
#endif

										// line tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
												actTemplate->m_lineType = getStrokeType(actAtt->getValue());
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												actTemplate->m_lineWidth = std::stof(actAtt->getValue());
											// color
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												actTemplate->m_color = actAtt->getValue();
										}

										// sourceStyle tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_sourceStyle], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
												actTemplate->m_sourceType = getArrowStyleAsInt(actAtt->getValue());
											// color
#if 0
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
												actTemplate->m_sourceColor = actAtt->getValue();
#endif
											// size
#if 0
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
												actTemplate->m_sourceSize = atof(actAtt->getValue());
#endif
										}

										// targetStyle tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_targetStyle], actTag)){
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
												actTemplate->m_targetType = getArrowStyleAsInt(actAtt->getValue());
											// color
#if 0
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
												actTemplate->m_targetColor = actAtt->getValue();
#endif
											// size
#if 0
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
												actTemplate->m_targetSize = atof(actAtt->getValue());
#endif
										}

										//insert actual template into hash table
										m_ogmlEdgeTemplates.fastInsert(actKey, actTemplate);
									}

								}

								// labelStyleTemplate
								if (styleTemplatesSon->getName() == Ogml::s_tagNames[Ogml::t_labelStyleTemplate]){
									// ACTUALLY NOT SUPPORTED
								}

								styleTemplatesSon = styleTemplatesSon->m_pBrother;
							}
						}
					}

					//STYLES
					if (layoutSon->getName() == Ogml::s_tagNames[Ogml::t_styles])
					{
						// has children graphStyle, nodeStyle, edgeStyle, labelStyle
						XmlTagObject *stylesSon;
						if (layoutSon->m_pFirstSon)
						{
							stylesSon = layoutSon->m_pFirstSon;

							while (stylesSon)
							{
								// GRAPHSTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[Ogml::t_graphStyle])
								{
									XmlAttributeObject *actAtt;
									// defaultNodeTemplate
									if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_defaultNodeTemplate], actAtt))
									{
										OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();

#if 0
										XmlTagObject *actTag;
										// data
										if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
											// found data for graphStyle
											// no implementation required for ogdf
										}
#endif

										// set values for ALL nodes
										for(node v : G.nodes){

											if (GA.has(GraphAttributes::nodeType)) {
												GA.templateNode(v) = actTemplate->m_nodeTemplate;
												GA.shape(v) = actTemplate->m_shapeType;
											}
											if (GA.has(GraphAttributes::nodeGraphics)) {
												GA.width(v) = actTemplate->m_width;
												GA.height(v) = actTemplate->m_height;
											}
											if (GA.has(GraphAttributes::nodeStyle)) {
												GA.fillColor(v) = actTemplate->m_color;
												GA.fillPattern(v) = actTemplate->m_pattern;
												//GA.nodePatternColor(v) = actTemplate->m_patternColor;
												GA.strokeType(v) = actTemplate->m_lineType;
												GA.strokeWidth(v) = actTemplate->m_lineWidth;
												GA.strokeColor(v) = actTemplate->m_lineColor;
											}
										}
									}

#if 0
									// defaultClusterTemplate
									if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_defaultCompoundTemplate], actAtt)){
										//										OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
										//										// set values for ALL Cluster
										for(cluster c : G.clusters){
											if (CGA.has(GraphAttributes::nodeType){
												CGA.templateCluster(c) = actTemplate->m_nodeTemplate;
												// no shape definition for clusters
												//CGA.shapeNode(c) = actTemplate->m_shapeType;
											}
											if (CGA.has(GraphAttributes::nodeGraphics){
												CGA.width(c) = actTemplate->m_width;
												CGA.height(c) = actTemplate->m_height;
											}
											if (CGA.has(GraphAttributes::nodeColor)
												CGA.clusterFillColor(c) = actTemplate->m_color;
											if (CGA.has(GraphAttributes::nodeStyle){
												CGA.clusterFillPattern(c) = actTemplate->m_pattern;
												CGA.clusterBackColor(c) = actTemplate->m_patternColor;
												CGA.clusterLineStyle(c) = actTemplate->m_lineType;
												CGA.clusterLineWidth(c) = actTemplate->m_lineWidth;
												CGA.clusterColor(c) = actTemplate->m_lineColor;
											}
										}
									}
#endif

									// defaultEdgeTemplate
									if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_defaultEdgeTemplate], actAtt))
									{
										OgmlEdgeTemplate* actTemplate = m_ogmlEdgeTemplates.lookup(actAtt->getValue())->info();

										// set values for ALL edges
										for(edge e : G.edges)
										{
											if (GA.has(GraphAttributes::edgeStyle)) {
												GA.strokeType(e) = actTemplate->m_lineType;
												GA.strokeWidth(e) = actTemplate->m_lineWidth;
												GA.strokeColor(e) = actTemplate->m_color;
											}

											//edgeArrow
											if ((GA.attributes()) & (GraphAttributes::edgeArrow))
											{
												if (actTemplate->m_sourceType == 0) {
													if (actTemplate->m_targetType == 0) {
														// source = no_arrow, target = no_arrow // =>none
														GA.arrowType(e) = EdgeArrow::None;
													}
													else {
														// source = no_arrow, target = arrow // =>last
														GA.arrowType(e) = EdgeArrow::Last;
													}
												}
												else {
													if (actTemplate->m_targetType == 0){
														// source = arrow, target = no_arrow // =>first
														GA.arrowType(e) = EdgeArrow::First;
													}
													else {
														// source = arrow, target = arrow // =>both
														GA.arrowType(e) = EdgeArrow::Both;
													}
												}
											}
										}
									}

									// defaultLabelTemplate
#if 0
									if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_defaultLabelTemplate], actAtt)){
										// set values for ALL labels
										// ACTUALLY NOT IMPLEMENTED
										label l;
										forall_labels(l, G){
											// ...
										}
									}
#endif
								}

								// NODESTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[Ogml::t_nodeStyle])
								{
									// get the id of the actual node
									XmlAttributeObject *att;
									if(stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att))
									{
										// check if referenced id is a node or a cluster/compound
										if (m_nodes.lookup(att->getValue()))
										{
											// lookup for node
											node actNode = (m_nodes.lookup(att->getValue()))->info();

											// actTag is the actual tag that is considered
											XmlTagObject* actTag;
											XmlAttributeObject *actAtt;

#if 0
											// data
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
												// found data for nodeStyle
												// no implementation required for ogdf
											}
#endif

											// check if actual nodeStyle references a template
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_nodeStyleTemplateRef], actTag))
											{
												// get referenced template id
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeStyleTemplateIdRef], actAtt))
												{
													// actual nodeStyle references a template
													OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
													if (GA.has(GraphAttributes::nodeType)) {
														GA.templateNode(actNode) = actTemplate->m_nodeTemplate;
														GA.shape(actNode) = actTemplate->m_shapeType;
													}
													if (GA.has(GraphAttributes::nodeGraphics)) {
														GA.width(actNode) = actTemplate->m_width;
														GA.height(actNode) = actTemplate->m_height;
													}
													if (GA.has(GraphAttributes::nodeStyle)) {
														GA.fillColor(actNode) = actTemplate->m_color;
														GA.fillPattern(actNode) = actTemplate->m_pattern;
														//GA.nodePatternColor(actNode) = actTemplate->m_patternColor;
														GA.strokeType(actNode) = actTemplate->m_lineType;
														GA.strokeWidth(actNode) = actTemplate->m_lineWidth;
														GA.strokeColor(actNode) = actTemplate->m_lineColor;
													}
												}
											}

											// Graph::nodeType
											//TODO: COMPLETE, IF NECESSARY
											if(GA.has(GraphAttributes::nodeType))
												GA.type(actNode) = Graph::NodeType::vertex;

											// location tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_location], actTag))
												&& GA.has(GraphAttributes::nodeGraphics))
											{
												// set location of node
												// x
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_x], actAtt))
													GA.x(actNode) = std::stod(actAtt->getValue());
												// y
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_y], actAtt))
													GA.y(actNode) = std::stod(actAtt->getValue());
												// z
#if 0
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_x], actAtt))
													GA.z(actNode) = atof(actAtt->getValue());
#endif
											}

											// shape tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_shape], actTag))
												&& GA.has(GraphAttributes::nodeType))
											{
												// set shape of node
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nShapeType], actAtt)) {
													GA.templateNode(actNode) = getNodeTemplateFromOgmlValue(actAtt->getValue());
													// TODO: change, if shapes are expanded
													// actually shape and template are calculated from the same value!!!
													GA.shape(actNode) = getShape(actAtt->getValue());
												}
												// width
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt)) {
													GA.width(actNode) = std::stod(actAtt->getValue());
													GA.x(actNode) += 0.5 * GA.width(actNode);
												}
												// height
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_height], actAtt)) {
													GA.height(actNode) = std::stod(actAtt->getValue());
													GA.y(actNode) += 0.5 * GA.height(actNode);
												}
												// uri
#if 0
												//ACTUALLY NOT SUPPORTED
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_uri], actAtt))
													GA.uri(actNode) = actAtt->getValue();
#endif
											}

											// fill tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_fill], actTag))
												&& GA.has(GraphAttributes::nodeStyle))
											{
												// fill color
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
													GA.fillColor(actNode) = actAtt->getValue();
												// fill pattern
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_pattern], actAtt))
													GA.fillPattern(actNode) = getFillPattern(actAtt->getValue());
												// fill patternColor
												//TODO: check if pattern color exists
#if 0
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_patternColor], actAtt))
													GA.nodePatternColor(actNode) = actAtt->getValue());
#endif
											}

											// line tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
												&& GA.has(GraphAttributes::nodeStyle))
											{
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
													GA.strokeType(actNode) = getStrokeType(actAtt->getValue());
												// width
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
													GA.strokeWidth(actNode) = std::stof(actAtt->getValue());
												// color
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
													GA.strokeColor(actNode) = actAtt->getValue();
											}
#if 0
											// ports
											// go through all ports with dummy tagObject port
											XmlTagObject* port = stylesSon->m_pFirstSon;
											while(port){
												if (port->getName() == ogmlTagObjects[t_port]){
													// TODO: COMPLETE
													// ACTUALLY NOT IMPLEMENTED IN OGDF
												}
												// go to next tag
												port = port->m_pBrother;
											}
#endif
										}
										else
										// CLUSTER NODE STYLE
										{
											// get the id of the cluster/compound
											XmlAttributeObject *attr;
											if(pCGA != nullptr && stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], attr))
											{
												// lookup for node
												cluster actCluster = (m_clusters.lookup(attr->getValue()))->info();
												// actTag is the actual tag that is considered
												XmlTagObject* actTag;
												XmlAttributeObject *actAtt;

#if 0
												// data
												if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
													// found data for nodeStyle (CLuster/Compound)
													// no implementation required for ogdf
												}
#endif

												// check if actual nodeStyle (equal to cluster) references a template
												if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_nodeStyleTemplateRef], actTag))
												{
													// get referenced template id
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeStyleTemplateIdRef], actAtt))
													{
														// actual nodeStyle references a template
														OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
														if (pCGA->has(GraphAttributes::nodeType)) {
															pCGA->templateCluster(actCluster) = actTemplate->m_nodeTemplate;
															// no shape definition for clusters
															//pCGA->shapeNode(actCluster) = actTemplate->m_shapeType;
														}
														if (pCGA->has(GraphAttributes::nodeGraphics)) {
															pCGA->width(actCluster) = actTemplate->m_width;
															pCGA->height(actCluster) = actTemplate->m_height;
														}
														if (pCGA->has(GraphAttributes::nodeStyle)) {
															pCGA->fillColor(actCluster) = actTemplate->m_color;
															pCGA->setFillPattern(actCluster, actTemplate->m_pattern);
															pCGA->fillBgColor(actCluster) = actTemplate->m_patternColor;
															pCGA->setStrokeType(actCluster, actTemplate->m_lineType);
															pCGA->strokeWidth(actCluster) = actTemplate->m_lineWidth;
															pCGA->strokeColor(actCluster) = actTemplate->m_lineColor;
														}
													}
												}

												// Graph::nodeType
												//TODO: COMPLETE, IF NECESSARY
												// not supported for clusters!!!
												//CGA.type(actCluster) = Graph::vertex;

												// location tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_location], actTag))
													&& pCGA->has(GraphAttributes::nodeGraphics))
												{
													// set location of node
													// x
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_x], actAtt))
														pCGA->x(actCluster) = std::stod(actAtt->getValue());
													// y
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_y], actAtt))
														pCGA->y(actCluster) = std::stod(actAtt->getValue());
													// z
#if 0
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_x], actAtt))
														CGA.clusterZPos(actCluster) = atof(actAtt->getValue());
#endif
												}

												// shape tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_shape], actTag))
													&& pCGA->has(GraphAttributes::nodeType))
												{
													// set shape of node
													// type
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nShapeType], actAtt)) {
														pCGA->templateCluster(actCluster) = getNodeTemplateFromOgmlValue(actAtt->getValue());
														// no shape definition for clusters
														//CGA.shapeNode(actCluster) = getShape(actAtt->getValue());
													}
													// width
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
														pCGA->width(actCluster) = std::stod(actAtt->getValue());
													// height
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_height], actAtt))
														pCGA->height(actCluster) = std::stod(actAtt->getValue());
													// uri
#if 0
													// ACTUALLY NOT SUPPORTED
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_uri], actAtt))
														CGA.uriCluster(actCluster) = actAtt->getValue();
#endif
												}

												// fill tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_fill], actTag))
													&& pCGA->has(GraphAttributes::nodeStyle))
												{
													// fill color
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
														pCGA->fillColor(actCluster) = actAtt->getValue();
													// fill pattern
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_pattern], actAtt))
														pCGA->setFillPattern(actCluster, getFillPattern(actAtt->getValue()));
													// fill patternColor
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_patternColor], actAtt))
														pCGA->fillBgColor(actCluster) = actAtt->getValue();
												}

												// line tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
													&& pCGA->has(GraphAttributes::nodeStyle))
												{
													// type
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
														pCGA->setStrokeType(actCluster, getStrokeType(actAtt->getValue()));
													// width
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
														pCGA->strokeWidth(actCluster) = std::stof(actAtt->getValue());
													// color
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
														pCGA->strokeColor(actCluster) = actAtt->getValue();
												}

#if 0
												// ports
												// go through all ports with dummy tagObject port
												XmlTagObject* port = stylesSon->m_pFirstSon;
												while(port){
													if (port->getName() == ogmlTagObjects[t_port]){
														// TODO: COMPLETE
														// no implementation required for ogdf
													}

													// go to next tag
													port = port->m_pBrother;
												}
#endif
											}
										}
									}
								}

								// EDGESTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[Ogml::t_edgeStyle])
								{
									// get the id of the actual edge
									XmlAttributeObject *att;
									if(stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_edgeIdRef], att))
									{
										// lookup for edge
										edge actEdge = (m_edges.lookup(att->getValue()))->info();

										// actTag is the actual tag that is considered
										XmlTagObject* actTag;
										XmlAttributeObject *actAtt;

										// data
#if 0
										if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
											// found data for edgeStyle
											// no implementation required for ogdf
										}
#endif

										// check if actual edgeStyle references a template
										if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_edgeStyleTemplateRef], actTag))
										{
											// get referenced template id
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_edgeStyleTemplateIdRef], actAtt))
											{
												// actual edgeStyle references a template
												OgmlEdgeTemplate* actTemplate = m_ogmlEdgeTemplates.lookup(actAtt->getValue())->info();
												if (GA.has(GraphAttributes::edgeStyle)) {
													GA.strokeType(actEdge) = actTemplate->m_lineType;
													GA.strokeWidth(actEdge) = actTemplate->m_lineWidth;
													GA.strokeColor(actEdge) = actTemplate->m_color;
												}

												//edgeArrow
												if ((GA.attributes()) & (GraphAttributes::edgeArrow))
												{
													if (actTemplate->m_sourceType == 0) {
														if (actTemplate->m_targetType == 0) {
															// source = no_arrow, target = no_arrow // =>none
															GA.arrowType(actEdge) = EdgeArrow::None;
														}
														else {
															// source = no_arrow, target = arrow // =>last
															GA.arrowType(actEdge) = EdgeArrow::Last;
														}
													}
													else {
														if (actTemplate->m_targetType == 0) {
															// source = arrow, target = no_arrow // =>first
															GA.arrowType(actEdge) = EdgeArrow::First;
														}
														else {
															// source = arrow, target = arrow // =>both
															GA.arrowType(actEdge) = EdgeArrow::Both;
														}
													}
												}
											}
										}

										// Graph::edgeType
										//TODO: COMPLETE, IF NECESSARY
										if(GA.has(GraphAttributes::edgeType))
											GA.type(actEdge) = Graph::EdgeType::association;

										// line tag
										if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
											&& GA.has(GraphAttributes::edgeType))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
												GA.strokeType(actEdge) = getStrokeType(actAtt->getValue());
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												GA.strokeWidth(actEdge) = std::stof(actAtt->getValue());
											// color
											if ((actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												&& GA.has(GraphAttributes::edgeType))
												GA.strokeColor(actEdge) = actAtt->getValue();
										}

										// mapping of arrows
										if (GA.has(GraphAttributes::edgeArrow))
										{
											// values for mapping edge arrows to GDE
											// init to -1 for a simple check
											int sourceInt = -1;
											int targetInt = -1;

											// sourceStyle tag
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_sourceStyle], actTag))
											{
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
													sourceInt = getArrowStyleAsInt((actAtt->getValue()));
												// color
#if 0
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
													;
#endif
												// size
#if 0
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
													;
#endif
											}

											// targetStyle tag
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_targetStyle], actTag))
											{
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
													targetInt = getArrowStyleAsInt((actAtt->getValue()));
												// color
#if 0
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
													;
#endif
												// size
#if 0
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
													;
#endif
											}

											// map edge arrows
											if ((sourceInt != -1) || (targetInt != -1))
											{
												if (sourceInt <= 0) {
													if (targetInt <= 0) {
														//source=no arrow, target=no arrow // => none
														GA.arrowType(actEdge) = EdgeArrow::None;
													}
													else {
														// source=no arrow, target=arrow // => last
														GA.arrowType(actEdge) = EdgeArrow::Last;
													}
												}
												else {
													if (targetInt <= 0) {
														//source=arrow, target=no arrow // => first
														GA.arrowType(actEdge) = EdgeArrow::First;
													}
													else {
														//source=target=arrow // => both
														GA.arrowType(actEdge) = EdgeArrow::Both;
													}
												}
											}
										}

										// points & segments
										// bool value for checking if segments exist
										bool segmentsExist = stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_segment], actTag);
										if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_point], actTag))
											&& GA.has(GraphAttributes::edgeGraphics))
										{
											// at least one point exists
											XmlTagObject *pointTag = stylesSon->m_pFirstSon;
											DPolyline dpl;
											dpl.clear();
											// traverse all points in the order given in the ogml file
											while (pointTag)
											{
												if (pointTag->getName() == Ogml::s_tagNames[Ogml::t_point])
												{

													if (pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt)) {
														DPoint dp;
														// here we have a point
														if (pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_x], actAtt)) {
															dp.m_x = std::stod(actAtt->getValue());
														}
														if (pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_y], actAtt)) {
															dp.m_y = std::stod(actAtt->getValue());
														}
#if 0
														if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_z], actAtt))
															dp.m_z = atof(actAtt->getValue());
#endif
														// insert point into hash table
														pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt);
														m_points.fastInsert(actAtt->getValue(), dp);
														//insert point into polyline
														if (!segmentsExist)
															dpl.pushBack(dp);
													}
												}
												// go to next tag
												pointTag = pointTag->m_pBrother;
											}
											//concatenate polyline
											if (!segmentsExist) {
												GA.bends(actEdge).conc(dpl);
											}
											else{
												// work with segments
												// one error can occur:
												// if a segments is going to be inserted,
												// which doesn't match with any other,
												// the order can be not correct at the end
												// then the edge is relly corrupted!!

												// TODO: this implementation doesn't work with hyperedges
												//       cause hyperedges have more than one source/target

												// segmentsUnsorted stores all found segments
												List<OgmlSegment> segmentsUnsorted;
												XmlTagObject *segmentTag = stylesSon->m_pFirstSon;
												while (segmentTag)
												{
													if (segmentTag->getName() == Ogml::s_tagNames[Ogml::t_segment])
													{
														XmlTagObject *endpointTag = segmentTag->m_pFirstSon;
														OgmlSegment actSeg;
														int endpointsSet = 0;
														while ((endpointTag) && (endpointsSet <2)) {
															if (endpointTag->getName() == Ogml::s_tagNames[Ogml::t_endpoint]) {
																// get the referenced point
																endpointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_endpointIdRef], actAtt);
																DPoint dp = (m_points.lookup(actAtt->getValue()))->info();

																if (endpointsSet == 0)
																	actSeg.point1 = dp;
																else
																	actSeg.point2 = dp;
																endpointsSet++;
															}
															endpointTag = endpointTag->m_pBrother;
														}
														// now we created a segment
														// we can insert this easily into in segmentsUnsorted
														if (actSeg.point1 != actSeg.point2) {
															segmentsUnsorted.pushBack(actSeg);
														}
													}
													// go to next tag
													segmentTag = segmentTag->m_pBrother;
												}
												// now are the segments stored in the segmentsUnsorted list
												//  but we have to sort it in segments list while inserting
												List<OgmlSegment> segments;
												ListIterator<OgmlSegment> segIt;
												// check the number of re-insertions
												int checkNumOfSegReInserts = segmentsUnsorted.size()+2;
												while ((segmentsUnsorted.size() > 0) && (checkNumOfSegReInserts > 0))
												{
													OgmlSegment actSeg = segmentsUnsorted.front();
													segmentsUnsorted.popFront();
													// actSeg has to be inserted in correct order
													//  and then being deleted
													//  OR waiting in list until it can be inserted
													// size == 0 => insert
													if (segments.size() == 0) {
														segments.pushFront(actSeg);
													}
													else {
														// segments contains >1 segment
														segIt = segments.begin();
														bool inserted = false;
														while (segIt.valid() && !inserted)
														{
															if ((actSeg.point1 == (*segIt).point1) ||
																(actSeg.point1 == (*segIt).point2) ||
																(actSeg.point2 == (*segIt).point1) ||
																(actSeg.point2 == (*segIt).point2))
															{
																// found two matching segments
																// now we can insert
																// there are some cases to check
																if (actSeg.point1 == (*segIt).point1) {
																	DPoint dumP = actSeg.point1;
																	actSeg.point1 = actSeg.point2;
																	actSeg.point2 = dumP;
																	segments.insertBefore(actSeg, segIt);
																}
																else {
																	if (actSeg.point2 == (*segIt).point1) {
																		segments.insertBefore(actSeg, segIt);
																	}
																	else {
																		if (actSeg.point2 == (*segIt).point2) {
																			DPoint dumP = actSeg.point1;
																			actSeg.point1 = actSeg.point2;
																			actSeg.point2 = dumP;
																			segments.insertAfter(actSeg, segIt);
																		}
																		else {
																			segments.insertAfter(actSeg, segIt);
																		}
																	}
																}
																inserted = true;
															}
															++segIt;
														}
														if (!inserted) {
															// segment doesn't found matching segment,
															//  so insert it again into unsorted segments list
															//  so it will be inserted later
															segmentsUnsorted.pushBack(actSeg);
															checkNumOfSegReInserts--;
														}
													}
												}


												if (checkNumOfSegReInserts==0) {
													GraphIO::logger.lout(Logger::Level::Minor) << "Segment definition is not correct!" << std::endl
													  << "  Not able to work with #" << segmentsUnsorted.size() << " segments" << std::endl
													  << "  Please check connection and sorting of segments!" << std::endl;
#if 0
													// inserting the bends although there might be an error
													// I commented this, because in this case in ogdf the edge will
													//   be a straight edge and there will not be any artefacts
													// TODO: uncomment if desired
													for (segIt = segments.begin(); segIt.valid(); segIt++){
														dpl.pushBack((*segIt).point1);
														dpl.pushBack((*segIt).point2);
													}
#endif
												}
												else {
													// the segments are now ordered (perhaps in wrong way)...
													// so we have to check if the first and last point
													//  are graphically laying in the source- and target- node
													bool invertSegments = false;
													segIt = segments.begin();
													node target = actEdge->target();
													node source = actEdge->source();
													// check if source is a normal node or a cluster
													//if (...){

													//}
													//else{
													// big if-check: if (first point is in target
													//                   and not in source)
													//                   AND
													//                   (last point is in source
													//                   and not in target)
													if (( ( (GA.x(target) + GA.width(target))>= (*segIt).point1.m_x )
														&&   (GA.x(target)                      <= (*segIt).point1.m_x )
														&& ( (GA.y(target) + GA.height(target))>= (*segIt).point1.m_y )
														&&   (GA.y(target)                      <= (*segIt).point1.m_y ) )
														&&
														(!( ( (GA.x(source) + GA.width(source))>= (*segIt).point1.m_x )
														&&   (GA.x(source)                      <= (*segIt).point1.m_x )
														&& ( (GA.y(source) + GA.height(source))>= (*segIt).point1.m_y )
														&&   (GA.y(source)                      <= (*segIt).point1.m_y ) )))
													{
														segIt = segments.rbegin();
														if (( ( (GA.x(source) + GA.width(source))>= (*segIt).point2.m_x )
															&&   (GA.x(source)                      <= (*segIt).point2.m_x )
															&& ( (GA.y(source) + GA.height(source))>= (*segIt).point2.m_y )
															&&   (GA.y(source)                      <= (*segIt).point2.m_y ) )
															&&
															(!( ( (GA.x(target) + GA.width(source))>= (*segIt).point2.m_x )
															&&   (GA.x(target)                      <= (*segIt).point2.m_x )
															&& ( (GA.y(target) + GA.height(source))>= (*segIt).point2.m_y )
															&&   (GA.y(target)                      <= (*segIt).point2.m_y ) ))) {
																// invert the segment-line
																invertSegments = true;
														}
													}
													//}
													if (!invertSegments){
														for (segIt = segments.begin(); segIt.valid(); ++segIt) {
															dpl.pushBack((*segIt).point1);
															dpl.pushBack((*segIt).point2);
														}
													}
													else {
														for (segIt = segments.rbegin(); segIt.valid(); --segIt) {
															dpl.pushBack((*segIt).point2);
															dpl.pushBack((*segIt).point1);
														}
													}
													// unify bends = delete superfluous points
													dpl.unify();
													// finally concatenate/set the bends
													GA.bends(actEdge).conc(dpl);
												}
											}
										}

									}

								}
#if 0
								// LABELSTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[t_labelStyle]){
									// labelStyle
									// ACTUALLY NOT SUPPORTED
								}
#endif

								stylesSon = stylesSon->m_pBrother;
							}

						}
					}

					// CONSTRAINTS
					if (layoutSon->getName() == Ogml::s_tagNames[Ogml::t_constraints]) {

						// this code is encapsulated in the method
						// OgmlParser::buildConstraints
						// has to be called by read methods after building

						// here we only set the pointer,
						//  so we don't have to traverse the parse tree
						//  to the constraints tag later
						m_constraintsTag = layoutSon;

					}


					// go to next brother
					layoutSon = layoutSon->m_pBrother;
				}
			}
		}
	}

#if 0
	std::cout << "buildAttributedClusterGraph COMPLETE. Check... " << std::endl << std::flush;
	for(edge e : G.edges) {
		//std::cout << "CGA.label    " << e << " = " << CGA.label(e) << std::endl << std::flush;
		std::cout << "CGA.arrowType" << e << "   = " << CGA.arrowType(e) << std::endl << std::flush;
		std::cout << "CGA.styleEdge" << e << "   = " << CGA.styleEdge(e) << std::endl << std::flush;
		std::cout << "CGA.edgeWidth" << e << "   = " << CGA.strokeWidth(e) << std::endl << std::flush;
		std::cout << "CGA.strokeColor" << e << " = " << CGA.strokeColor(e) << std::endl << std::flush;
		std::cout << "CGA.type     " << e << "   = " << CGA.type(e) << std::endl << std::flush;
		ListConstIterator<DPoint> it;
		for(it = CGA.bends(e).begin(); it!=CGA.bends(e).end(); ++it) {
			std::cout << "point " << " x=" << (*it).m_x << " y=" << (*it).m_y << std::endl << std::flush;
		}

	}

	for(node n : G.nodes) {
		std::cout << "CGA.label(" << n << ")         = " << CGA.label(n) << std::endl << std::flush;
		std::cout << "CGA.templateNode(" << n << ")  = " << CGA.templateNode(n) << std::endl << std::flush;
		std::cout << "CGA.shapeNode(" << n << ")     = " << CGA.shapeNode(n) << std::endl << std::flush;
		std::cout << "CGA.width(" << n << ")         = " << CGA.width(n) << std::endl << std::flush;
		std::cout << "CGA.height(" << n << ")        = " << CGA.height(n) << std::endl << std::flush;
		std::cout << "CGA.fillColor(" << n << ")     = " << CGA.fillColor(n) << std::endl << std::flush;
		std::cout << "CGA.nodePattern(" << n << ")   = " << CGA.nodePattern(n) << std::endl << std::flush;
		std::cout << "CGA.styleNode(" << n << ")     = " << CGA.styleNode(n) << std::endl << std::flush;
		std::cout << "CGA.strokeWidth(" << n << ")   = " << CGA.strokeWidth(n) << std::endl << std::flush;
		std::cout << "CGA.strokeColor(" << n << ")   = " << CGA.strokeColor(n) << std::endl << std::flush;
		std::cout << "CGA.x(" << n << ")             = " << CGA.x(n) << std::endl << std::flush;
		std::cout << "CGA.y(" << n << ")             = " << CGA.y(n) << std::endl << std::flush;
		std::cout << "CGA.type(" << n << ")          = " << CGA.type(n) << std::endl << std::flush;
	}

	for(cluster c : CGA.constClusterGraph().clusters){
		std::cout << "CGA.templateCluster(" << c << ")    = " << CGA.templateCluster(c) << std::endl << std::flush;
		std::cout << "CGA.width(" << c << ")       = " << CGA.width(c) << std::endl << std::flush;
		std::cout << "CGA.height(" << c << ")      = " << CGA.height(c) << std::endl << std::flush;
		std::cout << "CGA.clusterFillColor(" << c << ")   = " << CGA.clusterFillColor(c) << std::endl << std::flush;
		std::cout << "CGA.clusterFillPattern(" << c << ") = " << CGA.clusterFillPattern(c) << std::endl << std::flush;
		std::cout << "CGA.clusterBackColor(" << c << ")   = " << CGA.clusterBackColor(c) << std::endl << std::flush;
		std::cout << "CGA.clusterLineStyle(" << c << ")   = " << CGA.clusterLineStyle(c) << std::endl << std::flush;
		std::cout << "CGA.clusterLineWidth(" << c << ")   = " << CGA.clusterLineWidth(c) << std::endl << std::flush;
		std::cout << "CGA.clusterColor(" << c << ")       = " << CGA.clusterColor(c) << std::endl << std::flush;
		std::cout << "CGA.x(" << c << ")        = " << CGA.x(c) << std::endl << std::flush;
		std::cout << "CGA.y(" << c << ")        = " << CGA.y(c) << std::endl << std::flush;
	}

	std::cout << "buildAttributedClusterGraph COMPLETE... Check COMPLETE... Let's have fun in GDE ;) " << std::endl << std::flush;
#endif

	// building terminated, so return true
	return true;

}

// sets the labels of hierarchical nodes => cluster
bool OgmlParser::setLabelsRecursive(Graph &G, GraphAttributes &GA, ClusterGraphAttributes *pCGA, XmlTagObject *root)
{
	if ((root->getName() == Ogml::s_tagNames[Ogml::t_node]) && GA.has(GraphAttributes::nodeLabel))
	{
		if (!isNodeHierarchical(root))
		{
			// get the id of the actual node
			XmlAttributeObject *att;
			if(root->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
			{
				// lookup for node
				node actNode = (m_nodes.lookup(att->getValue()))->info();
				// find label tag
				XmlTagObject* label;
				if (root->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label)) {
					// get content tag
					XmlTagObject* content = label->m_pFirstSon;
					// get the content as string
					if (content->m_pTagValue){
						string str = content->getValue();
						string labelStr = getLabelCaptionFromString(str);
						// now set the label of the node
						GA.label(actNode) = labelStr;
					}
				}
			}
		} else {
			// get the id of the actual cluster
			XmlAttributeObject *att;
			if(pCGA != nullptr && root->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
			{
				// lookup for cluster
				cluster actCluster = (m_clusters.lookup(att->getValue()))->info();
				// find label tag
				XmlTagObject* label;
				if (root->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label)) {
					// get content tag
					XmlTagObject* content = label->m_pFirstSon;
					// get the content as string
					if (content->m_pTagValue) {
						string str = content->getValue();
						string labelStr = getLabelCaptionFromString(str);
						// now set the label of the node
						pCGA->label(actCluster) = labelStr;
					}
				}
			}
			// hierSon = hierarchical Son
			XmlTagObject *hierSon;
			if (root->m_pFirstSon)
			{
				hierSon = root->m_pFirstSon;
				while(hierSon) {
					// recursive call for setting labels of child nodes
					if (!setLabelsRecursive(G, GA, pCGA, hierSon))
						return false;
					hierSon = hierSon->m_pBrother;
				}
			}

		}
	}
	return true;
}

bool OgmlParser::buildGraph(Graph &G)
{
	G.clear();

	int id = 0;

	//Build nodes first
	HashConstIterator<string, const XmlTagObject*> it;

	for(it = m_ids.begin(); it.valid(); ++it)
	{
		if( it.info()->getName() == Ogml::s_tagNames[Ogml::t_node] && !isNodeHierarchical(it.info()))
		{
			// get id string from xmlTag
			XmlAttributeObject *idAtt;
			if ( (it.info())->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], idAtt)
				&& (getIdFromString(idAtt->getValue(), id)) )
			{
				// now we got an id from the id-string
				// we have to check, if this id was assigned
				if (m_nodeIds.lookup(id)) {
					// new id was assigned to another node
					id = G.maxNodeIndex() + 1;
				}
			}
			else {
				// default id setting
				id = G.maxNodeIndex() + 1;
			}
			m_nodes.fastInsert(it.key(), G.newNode(id));
			m_nodeIds.fastInsert(id, idAtt->getValue());
		}
	}

	id = 0;

	//Build edges second
	for(it = m_ids.begin(); it.valid(); ++it)
	{
		if( it.info()->getName() == Ogml::s_tagNames[Ogml::t_edge] )
		{
			//Check sources/targets
			ArrayBuffer<node> srcTgt;
			const XmlTagObject* son = it.info()->m_pFirstSon;
			while(son) {
				if( son->getName() == Ogml::s_tagNames[Ogml::t_source] ||
					son->getName() == Ogml::s_tagNames[Ogml::t_target] )
				{
					XmlAttributeObject *att;
					son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att);
					//Validate if source/target is really a node
					if (m_ids.lookup(att->getValue())->info()->getName() != Ogml::s_tagNames[Ogml::t_node]) {
						GraphIO::logger.lout(Logger::Level::Minor) << "Edge relation between graph elements of type not node are temporarily not supported!" << std::endl;
					}
					else {
						srcTgt.push(m_nodes.lookup(att->getValue())->info());
					}
				}
				son = son->m_pBrother;
			}
			if (srcTgt.size() != 2) {
				GraphIO::logger.lout(Logger::Level::Minor) << "Hyperedges are temporarily not supported! Discarding edge." << std::endl;
			}
			else {
				// create edge

				// get id string from xmlTag
				XmlAttributeObject *idAtt;
				if ( (it.info())->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], idAtt)
					&& (getIdFromString(idAtt->getValue(), id)) )
				{
					if (m_edgeIds.lookup(id)) {
						// new id was assigned to another edge
						id = G.maxEdgeIndex() + 1;
					}
				}
				else {
					// default id setting
					id = G.maxEdgeIndex() + 1;
				}
				m_edges.fastInsert(it.key(), G.newEdge(srcTgt.popRet(), srcTgt.popRet(), id));
				m_edgeIds.fastInsert(id, idAtt->getValue());
			}
		}
	}

	//Structure data determined, so building the graph was successfull.
	return true;
}

bool OgmlParser::buildClusterRecursive(
	const XmlTagObject *xmlTag,
	cluster parent,
	Graph &G,
	ClusterGraph &CG)
{
	// create new cluster

	// first get the id
	int id = -1;

	XmlAttributeObject *idAtt;
	if (  (xmlTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], idAtt))
		&& (getIdFromString(idAtt->getValue(), id)) )
	{
		if (m_clusterIds.lookup(id)) {
			// id was assigned to another cluster
			id = CG.maxClusterIndex() + 1;
		}
	}
	else {
		// default id setting
		id = CG.maxClusterIndex() + 1;
	}
	// create cluster and insert into hash tables
	cluster actCluster = CG.newCluster(parent, id);
	m_clusters.fastInsert(idAtt->getValue(), actCluster);
	m_clusterIds.fastInsert(id, idAtt->getValue());

	// check children of cluster tag
	XmlTagObject *son = xmlTag->m_pFirstSon;

	while(son)
	{
		if (son->getName() == Ogml::s_tagNames[Ogml::t_node]) {
			if (isNodeHierarchical(son))
				// recursive call
				buildClusterRecursive(son, actCluster, G, CG);
			else {
				// the actual node tag is a child of the cluster
				XmlAttributeObject *att;
				//parse tree is valid so tag owns id attribute
				son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att);
				// get node from lookup table with the id in att
				node v = m_nodes.lookup(att->getValue())->info();
				// assign node to actual cluster
				CG.reassignNode(v, actCluster);
			}
		}

		son = son->m_pBrother;
	}

	return true;
}



bool OgmlParser::buildCluster(
	const XmlTagObject *rootTag,
	Graph &G,
	ClusterGraph &CG)
{
	CG.init(G);

	if (rootTag->getName() != Ogml::s_tagNames[Ogml::t_ogml]) {
		GraphIO::logger.lout() << "Expecting root tag \"" << Ogml::s_tagNames[Ogml::t_ogml] << "\" in OgmlParser::buildCluster!" << std::endl;
		return false;
	}

	//Search for first node tag
	XmlTagObject *nodeTag;
	rootTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_graph], nodeTag);
	nodeTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_structure], nodeTag);
	nodeTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_node], nodeTag);

	while (nodeTag)
	{
		if(nodeTag->getName() == Ogml::s_tagNames[Ogml::t_node] && isNodeHierarchical(nodeTag)) {
			if (!buildClusterRecursive(nodeTag, CG.rootCluster(), G, CG))
				return false;
		}

		nodeTag = nodeTag->m_pBrother;
	}

	return true;
}

//Commented out due to missing graphconstraints in OGDF
#if 0
bool OgmlParser::buildConstraints(Graph& G, GraphConstraints &GC) {

	// constraints-tag was already set
	// if not, then return... job's done
	if (!m_constraintsTag)
		return true;

	if (m_constraintsTag->getName() != Ogml::s_tagNames[t_constraints]){
		std::cerr << "Error: constraints tag is not the required tag!" << std::endl;
		return false;
	}

	XmlTagObject* constraintTag;
	if(! m_constraintsTag->findSonXmlTagObjectByName(Ogml::s_tagNames[t_constraint], constraintTag) ) {
		std::cerr << "Error: no constraint block in constraints block of valid parse tree found!" << std::endl;
		return false;
	}


	while(constraintTag) {

//		// found data
//		if (constraintTag->getName() == Ogml::s_tagNames[t_data]){
//			// found data for constraints in general
//			// no implementation required for ogdf
//		}

		if(constraintTag->getName() == Ogml::s_tagNames[t_constraint]) {

			XmlAttributeObject* actAtt;
			string cId;
			string cType;

			if (constraintTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt))
				// set id of the constraint
				cId = actAtt->getValue();

			if (constraintTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_type], actAtt))
				cType = actAtt->getValue();
			else {
				std::cerr << "Error: constraint doesn't own compulsive attribute \'type\' in valid parse tree!" << std::endl;
				return false;
			}
			// now we need a constraint manager to create a constraint
			//  with the type of the name stored in cType
			// create the constraint
			Constraint* c = ConstraintManager::createConstraintByName(G, &cType);
			// check if the constraintManager doesn't return a null pointer
			//  that occurs if cM doesn't know the constraint name
			if (c) {
				// let the constraint load itself
				if (c->buildFromOgml(constraintTag, &m_nodes)){
					// add constraint if true is returned
					GC.addConstraint(c);
				}
				else
					std::cerr << "Error while building constraint with name \""<<cType<<"\"!" << std::endl;
			}
			else
				std::cerr << "Error: constraint type \""<<cType<<"\" is unknown!" << std::endl;

		}

		// go to next constraint tag
		constraintTag = constraintTag->m_pBrother;
	}

	// terminated, so return true
	return true;

}
#endif

bool OgmlParser::doRead(
	std::istream &is,
	Graph &G,
	ClusterGraph *pCG,
	GraphAttributes *pGA,
	ClusterGraphAttributes *pCGA)
{
	try {
		// XmlParser for parsing the ogml file
		XmlParser p(is);
		if(!p.createParseTree())
			return false;

		// get root object of the parse tree
		const XmlTagObject *root = &p.getRootTag();

		// validate the document
		if (validate(root, Ogml::t_ogml) != Ogml::vs_valid)
			return false;

		checkGraphType(root);

		// build graph
		if (!buildGraph(G))
			return false;

		// build cluster structure (if required)
		Ogml::GraphType gt = getGraphType();
		if(pCG != nullptr && gt != Ogml::graph) {
			if (!buildCluster(root, G, *pCG))
				return false;
		}

		// add attributes
		if(pGA != nullptr) {
			if (!addAttributes(G, *pGA, pCGA, root))
				return false;
		}

	} catch(const char *error) {
		Logger::slout() << error << std::endl << std::flush;
		return false;
	}

	return true;
}

}
