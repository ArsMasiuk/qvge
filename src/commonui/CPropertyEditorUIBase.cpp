#include "CPropertyEditorUIBase.h"


CPropertyEditorUIBase::CPropertyEditorUIBase(QWidget * parent) : QtTreePropertyBrowser(parent) 
{
	setResizeMode(Interactive);
}

CPropertyEditorUIBase::~CPropertyEditorUIBase() 
{
	
}


QtBrowserItem* CPropertyEditorUIBase::selectItemByName(const QString& name)
{
	QList<QtBrowserItem*> items = topLevelItems();
	for (auto item : items)
	{
		if (item->property()->propertyName() == name)
		{
			setCurrentItem(item);
			return item;
		}
	}

	return NULL;
}


QtProperty* CPropertyEditorUIBase::getCurrentTopProperty() const
{
	auto item = currentItem();
	if (!item)
		return NULL;

	while (item->parent())
		item = item->parent();

	return item->property();
}


QString CPropertyEditorUIBase::getCurrentTopPropertyName() const
{
	QtProperty *prop = getCurrentTopProperty();
	if (prop)
		return prop->propertyName();
	else
		return "";
}



QVariant CPropertyEditorUIBase::getCurrentTopPropertyValue() const
{
	QtVariantProperty* vprop = dynamic_cast<QtVariantProperty*>(getCurrentTopProperty());
	if (vprop)
		return vprop->value();
	else
		return QVariant();
}


int CPropertyEditorUIBase::getCurrentTopPropertyValueType() const
{
	QtVariantProperty* vprop = dynamic_cast<QtVariantProperty*>(getCurrentTopProperty());
	if (vprop)
		return vprop->valueType();
	else
		return -1;
}


QVariant CPropertyEditorUIBase::getCurrentPropertyValue() const
{
	auto item = currentItem();
	if (item)
	{
		QtVariantProperty* vprop = dynamic_cast<QtVariantProperty*>(item->property());
		if (vprop)
			return vprop->value();
	}

	return QVariant();
}


int CPropertyEditorUIBase::getCurrentPropertyValueType() const
{
	auto item = currentItem();
	if (item)
	{
		QtVariantProperty* vprop = dynamic_cast<QtVariantProperty*>(item->property());
		if (vprop)
			return vprop->valueType();
	}

	return QVariant().type();
}


void CPropertyEditorUIBase::updateTooltip(QtVariantProperty* prop)
{
	if (prop)
		prop->setToolTip(QString("%2<br><i>[%1]</i>").arg(prop->value().typeName()).arg(prop->valueText()));
}
