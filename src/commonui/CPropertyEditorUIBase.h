#pragma once

#include <QtTreePropertyBrowser>
#include <QtVariantProperty>

class CPropertyEditorUIBase : public QtTreePropertyBrowser
{
	Q_OBJECT

public:
	CPropertyEditorUIBase(QWidget *parent = Q_NULLPTR);
	virtual ~CPropertyEditorUIBase();

	QtBrowserItem* selectItemByName(const QString& name);

	QtProperty* getCurrentTopProperty() const;
	QString getCurrentTopPropertyName() const;
	QVariant getCurrentTopPropertyValue() const;
	int getCurrentTopPropertyValueType() const;

	QVariant getCurrentPropertyValue() const;
	int getCurrentPropertyValueType() const;

	virtual void updateTooltip(QtVariantProperty* prop);
};
