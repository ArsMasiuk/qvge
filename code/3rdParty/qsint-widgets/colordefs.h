#ifndef COLORDEFS_H
#define COLORDEFS_H


#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtGui/QColor>


namespace QSint
{


/** This type defines a list of QColor to be used in \a setScheme() functions.
*/
typedef QList<QColor> ColorList;

/** Returns default color scheme.
  \sa setScheme()
  */
ColorList* defaultColors();
/** Returns second default color scheme.
  \sa setScheme()
  */
ColorList* defaultColors2();
/** Returns default color scheme based on 16 base colors.
  \sa setScheme()
  */
ColorList* baseColors();
/** Returns default color scheme based on QColor::colorNames() list.
  \sa setScheme()
  */
ColorList* namedColors();

ColorList* openOfficeColors();



struct NamedColorsScheme
{
	ColorList colors;
    QMap<QRgb, QString> colorNames;
	int gridWidth;

	void addNamedColor(const QColor& color, const QString& name);
	QString colorName(const QColor& color) const;
};

const NamedColorsScheme& BaseColors();

const NamedColorsScheme& SvgColors();

const NamedColorsScheme& OpenOfficeColors();

}


#endif // COLORDEFS_H
