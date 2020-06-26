/** \file
 * \brief Declaration of basic types for graphics.
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

#include <ogdf/basic/basic.h>
#include <map>
#include <typeinfo>
#include <ogdf/basic/Logger.h>


namespace ogdf {

//! Line types of strokes.
/**
 * @ingroup graph-drawing
 */
enum class StrokeType : unsigned char {
	None,      //!< no line
	Solid,     //!< solid line
	Dash,      //!< dashed line
	Dot,       //!< dotted line
	Dashdot,   //!< line style "dash dot dash dot ..."
	Dashdotdot //!< line style "dash dot dot dash dot dot ..."
};

//! Output operator
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const StrokeType &st);

//! Converts integer \p i to stroke type.
/**
 * @ingroup graph-drawing
 */
StrokeType intToStrokeType(int i);

//! Converts string \p s to stroke type.
/**
 * @ingroup graph-drawing
 */
OGDF_EXPORT string strokeTypeToString(StrokeType st);

//! Converts string \p s to stroke type.
/**
 * @ingroup graph-drawing
 */
OGDF_EXPORT StrokeType stringToStrokeType(string s);


//! Line cap types of strokes.
/**
 * @ingroup graph-drawing
 */
enum class StrokeLineCap : unsigned char{
	Butt,
	Round,
	Square
};


//! Line join types of strokes.
/**
 * @ingroup graph-drawing
 */
enum class StrokeLineJoin : unsigned char {
	Miter,
	Round,
	Bevel
};


//! Fill patterns.
/**
 * @ingroup graph-drawing
 */
enum class FillPattern {
	None,
	Solid,
	Dense1,
	Dense2,
	Dense3,
	Dense4,
	Dense5,
	Dense6,
	Dense7,
	Horizontal,
	Vertical,
	Cross,
	BackwardDiagonal,
	ForwardDiagonal,
	DiagonalCross
};

void initFillPatternHashing();

//! Converts fillpattern \p fp to string.
/**
 * @ingroup graph-drawing
 */
OGDF_EXPORT string fillPatternToString(FillPattern fp);

//! Converts string \p s to fill pattern.
/**
 * @ingroup graph-drawing
 */
OGDF_EXPORT FillPattern stringToFillPattern(string s);

//! Output operator
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const FillPattern &fp);

//! Converts integer \p i to fill pattern.
/**
 * @ingroup graph-drawing
 */
FillPattern intToFillPattern(int i);


//! Types for node shapes.
/**
 * @ingroup graph-drawing
 */
enum class Shape {
	Rect,               //!< rectangle
	RoundedRect,        //!< rectangle with rounded corners
	Ellipse,            //!< ellipse
	Triangle,           //!< isosceles triangle (base side down)
	Pentagon,           //!< pentagon
	Hexagon,            //!< hexagon
	Octagon,            //!< octagon
	Rhomb,              //!< rhomb (=diamond)
	Trapeze,            //!< trapeze (upper side shorter)
	Parallelogram,      //!< parallelogram (slanted to the right)
	InvTriangle,        //!< isosceles triangle (base side up)
	InvTrapeze,         //!< inverted trapeze  (upper side longer)
	InvParallelogram,   //!< inverted parallelogram (slanted to the left)
	Image
};

//! Converts shape \p s to string.
/**
* @ingroup graph-drawing
*/
OGDF_EXPORT string shapeToString(Shape s);

//! Converts string \p s to shape.
/**
* @ingroup graph-drawing
*/
OGDF_EXPORT Shape stringToShape(string s);

//! Output operator
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const FillPattern &fp);


//! Types for edge arrows.
/**
 * @ingroup graph-drawing
 */
enum class EdgeArrow {
	None, //!< no edge arrows
	Last, //!< edge arrow at target node of the edge
	First, //!< edge arrow at source node of the edge
	Both, //!< edge arrow at target and source node of the edge
	Undefined
};

//! Output operator
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const EdgeArrow &ea);

//! Colors represented as RGBA values.
/**
 * @ingroup graph-drawing
 *
 * The Color class represents colors with four components: R (red), G (green), B (blue), and A (alpha channel).
 * Each component has a value between and 255. The alpha channel controls tranparency, where an opaque color
 * has an alpha channel of 255.
 */
class OGDF_EXPORT Color {
	uint8_t m_red, m_green, m_blue, m_alpha;

public:
	//! Named colors (same as SVG color keywords).
	enum class Name {
		Aliceblue,
		Antiquewhite,
		Aqua,
		Aquamarine,
		Azure,
		Beige,
		Bisque,
		Black,
		Blanchedalmond,
		Blue,
		Blueviolet,
		Brown,
		Burlywood,
		Cadetblue,
		Chartreuse,
		Chocolate,
		Coral,
		Cornflowerblue,
		Cornsilk,
		Crimson,
		Cyan,
		Darkblue,
		Darkcyan,
		Darkgoldenrod,
		Darkgray,
		Darkgreen,
		Darkgrey,
		Darkkhaki,
		Darkmagenta,
		Darkolivegreen,
		Darkorange,
		Darkorchid,
		Darkred,
		Darksalmon,
		Darkseagreen,
		Darkslateblue,
		Darkslategray,
		Darkslategrey,
		Darkturquoise,
		Darkviolet,
		Deeppink,
		Deepskyblue,
		Dimgray,
		Dimgrey,
		Dodgerblue,
		Firebrick,
		Floralwhite,
		Forestgreen,
		Fuchsia,
		Gainsboro,
		Ghostwhite,
		Gold,
		Goldenrod,
		Gray,
		Green,
		Greenyellow,
		Grey,
		Honeydew,
		Hotpink,
		Indianred,
		Indigo,
		Ivory,
		Khaki,
		Lavender,
		Lavenderblush,
		Lawngreen,
		Lemonchiffon,
		Lightblue,
		Lightcoral,
		Lightcyan,
		Lightgoldenrodyellow,
		Lightgray,
		Lightgreen,
		Lightgrey,
		Lightpink,
		Lightsalmon,
		Lightseagreen,
		Lightskyblue,
		Lightslategray,
		Lightslategrey,
		Lightsteelblue,
		Lightyellow,
		Lime,
		Limegreen,
		Linen,
		Magenta,
		Maroon,
		Mediumaquamarine,
		Mediumblue,
		Mediumorchid,
		Mediumpurple,
		Mediumseagreen,
		Mediumslateblue,
		Mediumspringgreen,
		Mediumturquoise,
		Mediumvioletred,
		Midnightblue,
		Mintcream,
		Mistyrose,
		Moccasin,
		Navajowhite,
		Navy,
		Oldlace,
		Olive,
		Olivedrab,
		Orange,
		Orangered,
		Orchid,
		Palegoldenrod,
		Palegreen,
		Paleturquoise,
		Palevioletred,
		Papayawhip,
		Peachpuff,
		Peru,
		Pink,
		Plum,
		Powderblue,
		Purple,
		Red,
		Rosybrown,
		Royalblue,
		Saddlebrown,
		Salmon,
		Sandybrown,
		Seagreen,
		Seashell,
		Sienna,
		Silver,
		Skyblue,
		Slateblue,
		Slategray,
		Slategrey,
		Snow,
		Springgreen,
		Steelblue,
		Tan,
		Teal,
		Thistle,
		Tomato,
		Turquoise,
		Violet,
		Wheat,
		White,
		Whitesmoke,
		Yellow,
		Yellowgreen
	};

	//! Creates an opaque black color.
	Color() : m_red(0), m_green(0), m_blue(0), m_alpha(255) { }

	//! Creates a color from given RGBA-values.
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : m_red(r), m_green(g), m_blue(b), m_alpha(a) { }

	//! Creates a color from given RGBA-values.
	Color(int r, int g, int b, int a = 255) : m_red((uint8_t)r), m_green((uint8_t)g), m_blue((uint8_t)b), m_alpha((uint8_t)a) { }

	//! Creates a color from given color name \p name.
	Color(Color::Name name);

	//! Crates a color from string \p str.
	Color(const string &str) { fromString(str); }

	//! Crates a color from string \p str.
	Color(const char *str) { fromString(string(str)); }

	//! Returns the red component.
	uint8_t red() const { return m_red; }

	//! Returns the green component.
	uint8_t green() const { return m_green; }

	//! Returns the blue component.
	uint8_t blue() const { return m_blue; }

	//! Returns the alpha channel.
	uint8_t alpha() const { return m_alpha; }

	//! Sets the red component to \p r.
	void red(uint8_t r) { m_red = r; }

	//! Sets the green component to \p g.
	void green(uint8_t g) { m_green = g; }

	//! Sets the blue component to \p b.
	void blue(uint8_t b) { m_blue = b; }

	//! Sets the alpha channel to \p a.
	void alpha(uint8_t a) { m_alpha = a; }

	//! Converts the color to a string and returns it.
	/**
	 * Colors as represented as strings using the \#RRGGBB hex notation.
	 * Please note that in this notation the alpha channel is not represented and
	 * is assumed to be 255 (an opaque color).
	 */
	string toString() const;

	//! Sets the color the the color defined by \p str.
	bool fromString(const string &str);

	//! Returns true iff \p c and this color are equal in every component.
	bool operator==(const Color &c) const {
		return m_red == c.m_red && m_green == c.m_green && m_blue == c.m_blue && m_alpha == c.m_alpha;
	}

	//! Returns true iff \p c and this color differ in any component.
	bool operator!=(const Color &c) const {
		return !operator==(c);
	}

	//! Writes the string representation of color \p c to output stream \p os.
	friend std::ostream &operator<<(std::ostream &os, const Color &c) {
		return os << c.toString();
	}
};


//! Properties of strokes.
/**
 * @ingroup graph-drawing
 */
struct OGDF_EXPORT Stroke {
	Color          m_color;    //!< stroke color
	float          m_width;    //!< stroke width
	StrokeType     m_type; //!< stroke type (e.g. solid or dashed)
	StrokeLineCap  m_cap; //!< line-cap of the stroke
	StrokeLineJoin m_join; //!< line-join of the stroke

	Stroke() : m_color(Color::Name::Black), m_width(1.0f), m_type(StrokeType::Solid), m_cap(StrokeLineCap::Butt), m_join(StrokeLineJoin::Miter) { }
	Stroke(Color c) : m_color(c), m_width(1.0f), m_type(StrokeType::Solid), m_cap(StrokeLineCap::Butt), m_join(StrokeLineJoin::Miter) { }
};


//! Properties of fills.
/**
 * @ingroup graph-drawing
 */
struct OGDF_EXPORT Fill {
	Color       m_color;   //!< fill color
	Color       m_bgColor; //!< background color of fill pattern
	FillPattern m_pattern; //!< fill pattern

	Fill() : m_color(Color::Name::White), m_bgColor(Color::Name::Black), m_pattern(FillPattern::Solid) { }
	Fill(Color c) : m_color(c), m_bgColor(Color::Name::Black), m_pattern(FillPattern::Solid) { }
	Fill(Color c, FillPattern pattern) : m_color(c), m_bgColor(Color::Name::Black), m_pattern(pattern) { }
	Fill(Color c, Color bgColor, FillPattern pattern) : m_color(c), m_bgColor(bgColor), m_pattern(pattern) { }
};

namespace graphics {
	extern OGDF_EXPORT std::map<Shape, string> fromShape;
	extern OGDF_EXPORT std::map<string, Shape> toShape;

	extern OGDF_EXPORT std::map<StrokeType, string> fromStrokeType;
	extern OGDF_EXPORT std::map<string, StrokeType> toStrokeType;

	extern OGDF_EXPORT std::map<FillPattern, string> fromFillPattern;
	extern OGDF_EXPORT std::map<string, FillPattern> toFillPattern;

	template<class Enum>
	inline void init() {};

	template<class Enum>
	inline void initSecondMap(std::map<Enum, string> &fromMap, std::map<string, Enum> &toMap) {
		for(auto it : fromMap) {
			toMap.emplace(it.second, it.first);
		}
	}


	template<>
	inline void init<StrokeType>() {
		fromStrokeType.emplace(StrokeType::None,          "None");
		fromStrokeType.emplace(StrokeType::Solid,         "Solid");
		fromStrokeType.emplace(StrokeType::Dash,          "Dash");
		fromStrokeType.emplace(StrokeType::Dot,           "Dot");
		fromStrokeType.emplace(StrokeType::Dashdot,       "Dashdot");
		fromStrokeType.emplace(StrokeType::Dashdotdot,    "Dashdotdot");

		initSecondMap<StrokeType>(fromStrokeType, toStrokeType);
	}

	template<>
	inline void init<FillPattern>() {
		fromFillPattern.emplace(FillPattern::None,               "None");
		fromFillPattern.emplace(FillPattern::Solid,              "Solid");
		fromFillPattern.emplace(FillPattern::Dense1,             "Dense1");
		fromFillPattern.emplace(FillPattern::Dense2,             "Dense2");
		fromFillPattern.emplace(FillPattern::Dense3,             "Dense3");
		fromFillPattern.emplace(FillPattern::Dense4,             "Dense4");
		fromFillPattern.emplace(FillPattern::Dense5,             "Dense5");
		fromFillPattern.emplace(FillPattern::Dense6,             "Dense6");
		fromFillPattern.emplace(FillPattern::Dense7,             "Dense7");
		fromFillPattern.emplace(FillPattern::Horizontal,         "Horizontal");
		fromFillPattern.emplace(FillPattern::Vertical,           "Vertical");
		fromFillPattern.emplace(FillPattern::Cross,              "Cross");
		fromFillPattern.emplace(FillPattern::BackwardDiagonal,   "BackwardDiagonal");
		fromFillPattern.emplace(FillPattern::ForwardDiagonal,    "ForwardDiagonal");
		fromFillPattern.emplace(FillPattern::DiagonalCross,      "DiagonalCross");

		initSecondMap<FillPattern>(fromFillPattern, toFillPattern);
	}

	template<>
	inline void init<Shape>() {
		fromShape.emplace(Shape::Rect,                 "Rect");
		fromShape.emplace(Shape::RoundedRect,          "RoundedRect");
		fromShape.emplace(Shape::Ellipse,              "Ellipse");
		fromShape.emplace(Shape::Triangle,             "Triangle");
		fromShape.emplace(Shape::Pentagon,             "Pentagon");
		fromShape.emplace(Shape::Hexagon,              "Hexagon");
		fromShape.emplace(Shape::Octagon,              "Octagon");
		fromShape.emplace(Shape::Rhomb,                "Rhomb");
		fromShape.emplace(Shape::Trapeze,              "Trapeze");
		fromShape.emplace(Shape::Parallelogram,        "Parallelogram");
		fromShape.emplace(Shape::InvTriangle,          "InvTriangle");
		fromShape.emplace(Shape::InvTrapeze,           "InvTrapeze");
		fromShape.emplace(Shape::InvParallelogram,     "InvParallelogram");
		fromShape.emplace(Shape::Image,                "Image");

		initSecondMap<Shape>(fromShape, toShape);
		toShape.emplace("rectangle",                    Shape::Rect);
		toShape.emplace("box",                          Shape::Image);
	}

	template<class ToClass>
	inline std::map<string, ToClass>* getMapToEnum() {
		std::cout << "getMapToEnum was wrongly called\n";
		OGDF_ASSERT(false);
		return nullptr;
	};

	template<class FromClass>
	inline std::map<FromClass, string>* getMapToString() {
		FromClass fc;
		std::cout << "getMapToString was wrongly called " << typeid(fc).name() << "\n";
		OGDF_ASSERT(false);
		return nullptr;
	};

	template<>
	inline std::map<string, FillPattern> *getMapToEnum() {
		return &toFillPattern;
	};

	template<>
	inline std::map<string, Shape> *getMapToEnum() {
		return &toShape;
	};

	template<>
	inline std::map<string, StrokeType> *getMapToEnum() {
		return &toStrokeType;
	};

	template<>
	inline std::map<FillPattern, string> *getMapToString() {
		return &fromFillPattern;
	};

	template<>
	inline std::map<Shape, string> *getMapToString() {
		return &fromShape;
	};

	template<>
	inline std::map<StrokeType, string> *getMapToString() {
		return &fromStrokeType;
	};
}

template<class FromClass>
inline string toString(FromClass key) {
	auto *map = graphics::getMapToString<FromClass>();
	if(map->empty()) {
		graphics::init<FromClass>();
	}
	auto it = map->find(key);
	OGDF_ASSERT(it != map->end());
	return (*it).second;
};

template<class ToClass>
inline ToClass fromString(string key) {
	auto map = graphics::getMapToEnum<ToClass>();
	if(map->empty()) {
		graphics::init<ToClass>();
	}
	auto it = map->find(key);
	if(it != map->end()) {
		return (*it).second;
	} else {
		Logger::slout() << "Encountered invalid " << typeid((*map->begin()).second).name()
						<< ": " << key << " " << map->size() << " " << map->empty() << std::endl;
		return static_cast<ToClass>(std::numeric_limits<int>::min());
	}
};
}
