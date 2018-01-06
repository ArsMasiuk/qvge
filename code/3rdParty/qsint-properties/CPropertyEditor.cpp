#include "CPropertyEditor.h"
#include "CBoolProperty.h"
#include "CIntegerProperty.h"

#include <QKeyEvent>
#include <QHeaderView>
#include <QDebug>
#include <QApplication>


CPropertyEditor::CPropertyEditor(QWidget *parent) :
    QTreeWidget(parent),
    m_addingItem(false)
{
	init();
}


void CPropertyEditor::init()
{
    setColumnCount(2);

    QStringList labels;
    labels << tr("Parameter") << tr("Value");
    setHeaderLabels(labels);

    header()->setSectionsMovable(false);
//    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
//    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    setUniformRowHeights(true);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);


    connect(this,
            SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this,
            SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*))
    );

    connect(this,
            SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this,
            SLOT(onItemClicked(QTreeWidgetItem*,int))
    );

    connect(this,
            SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this,
            SLOT(onItemChanged(QTreeWidgetItem*,int))
    );


	// these connections must delay signal emitting to the very end of the update chain
	connect(this,
			SIGNAL(emitValueChanged(CBaseProperty*, const QVariant &)),
			this,
			SIGNAL(valueChanged(CBaseProperty*, const QVariant &)),
			Qt::QueuedConnection
	);

	connect(this,
            SIGNAL(emitStateChanged(CBaseProperty*, bool)),
			this,
            SIGNAL(stateChanged(CBaseProperty*, bool)),
			Qt::QueuedConnection
	);
}


void CPropertyEditor::adjustToContents()
{
    header()->resizeSections(QHeaderView::ResizeToContents);
}


void CPropertyEditor::clear()
{
	m_propertyMap.clear();

	QTreeWidget::clear();
}


// properties

bool CPropertyEditor::add(CBaseProperty *prop)
{
    if (prop == NULL)
        return false;

    if (m_propertyMap.contains(prop->getId()))
    {
        qDebug() << "Property with id=" << prop->getId() << " already assigned";
        return false;    // exists
    }

    m_addingItem = true;

    prop->setSizeHint(1, QSize(100,24));

    m_propertyMap[prop->getId()] = prop;
    addTopLevelItem(prop);

    prop->onAdded();

    expandItem(prop);

    m_addingItem = false;

    return true;
}


bool CPropertyEditor::remove(CBaseProperty *prop)
{
    if (prop == NULL)
        return false;

    if (!m_propertyMap.contains(prop->getId()))
    {
        qDebug() << "Property with id=" << prop->getId() << " has not been assigned";
        return false;    // exists
    }

    m_propertyMap.remove(prop->getId());

    int idx = indexOfTopLevelItem(prop);
    takeTopLevelItem(idx);
    delete prop;

    return true;
}


// this slot is called from item widget editor to signal that editing is over

void CPropertyEditor::onWidgetEditorFinished()
{
    CBaseProperty* prop = dynamic_cast<CBaseProperty*>(currentItem());
    if (prop != NULL)
        prop->finishEdit();
}


// slots

void CPropertyEditor::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current == previous)
        return;

    CBaseProperty* oldProp = dynamic_cast<CBaseProperty*>(previous);
    if (oldProp != NULL)
        oldProp->onLeave();

    CBaseProperty* newProp = dynamic_cast<CBaseProperty*>(current);
    if (newProp != NULL)
        newProp->onEnter();
}


void CPropertyEditor::onItemClicked(QTreeWidgetItem *item, int column)
{
    CBaseProperty* prop = dynamic_cast<CBaseProperty*>(item);
    if (prop != NULL && !prop->isDisabled())
    {
        if (column == 1)
            prop->startEdit();
        else
            prop->finishEdit();
    }
}


void CPropertyEditor::onItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_addingItem)
        return;

    CBaseProperty* prop = dynamic_cast<CBaseProperty*>(item);
    if (prop != NULL)
    {
        if (column == 1)
        {
            qDebug() << "Value state of property [" << prop->getId() << "] changed to: " << prop->getVariantValue();

			Q_EMIT emitValueChanged(prop, prop->getVariantValue());
        }
        else 
		if (column == 0)
        {
			if (prop->isMarkable())
			{
				qDebug() << "Marked state of property [" << prop->getId() << "] changed to: " << prop->isMarked();

				Q_EMIT emitStateChanged(prop, prop->isMarked());
			}
        }
    }
}


// keys event

void CPropertyEditor::keyPressEvent(QKeyEvent *event)
{

//    qDebug() << event->key();

    CBaseProperty* prop = dynamic_cast<CBaseProperty*>(currentItem());
    if (prop != NULL)
    {
        QWidget* editWidget = prop->getActiveEditor();

        if (prop->onKeyPressed(event, editWidget))
            return;

        switch (event->key())
        {
            case Qt::Key_Return:

                if (editWidget == NULL)
                {
                    prop->startEdit();
                }
                else
                {
                    if (editWidget->isVisible())
                        prop->finishEdit();
                    else
                        prop->startEdit();
                }

                break;

            case Qt::Key_Escape:

                if (editWidget != NULL && editWidget->isVisible())
                    prop->finishEdit(true);

                break;

            case Qt::Key_Space:

                if (prop->isMarkable())
                    prop->setMarked(!prop->isMarked());

                return;

            default:
                break;
        }
    }

    QTreeWidget::keyPressEvent(event);
}

