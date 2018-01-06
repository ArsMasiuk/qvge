#include "CBaseProperty.h"


CBaseProperty::CBaseProperty(const QByteArray &id, const QString &name):
    QTreeWidgetItem(),
    m_id(id),
    m_name(name),
    m_isMarkable(false),
    m_editorIsPrivate(false)
{
    setText(0, m_name);
}


CBaseProperty::CBaseProperty(CBaseProperty *top, const QByteArray &id, const QString &name):
    QTreeWidgetItem(top),
    m_id(id),
    m_name(name),
    m_isMarkable(false),
    m_editorIsPrivate(false)
{
    setText(0, m_name);
}


void CBaseProperty::setMarked(bool on)
{
    m_isMarkable = true;

    setCheckState(0, on ? Qt::Checked : Qt::Unchecked);
}


bool CBaseProperty::isMarked() const
{
    return (checkState(0) == Qt::Checked);
}


bool CBaseProperty::isMarkable() const
{
    return m_isMarkable;
}


void CBaseProperty::setBackground(const QBrush &bg)
{
    QTreeWidgetItem::setBackground(0, bg);
    QTreeWidgetItem::setBackground(1, bg);
}


void CBaseProperty::setTextColor(const QColor &color)
{
    QTreeWidgetItem::setTextColor(0, color);
    QTreeWidgetItem::setTextColor(1, color);
}


void CBaseProperty::setValue()
{
	QSignalBlocker blocker(treeWidget());

    validateValue();

    displayValue();

    valueToEditor();
}


void CBaseProperty::startEdit()
{
    QWidget* editWidget = getActiveEditor();

    // create if needed
    if (editWidget == NULL)
    {
        editWidget = createEditor();
    }

    if (editWidget == NULL)
        return;

    // show it
    if (!m_editorIsPrivate || (getActiveEditor() == NULL))
    {
        treeWidget()->setItemWidget(this, 1, editWidget);
    }

    if (m_editorIsPrivate)
    {
        editWidget->show();
    }

    onShowEditor(editWidget);

    // set value and constraints to the editor
    valueToEditor();

    // focus
    editWidget->setFocus();
}


void CBaseProperty::finishEdit(bool cancel)
{
    QWidget* editWidget = getActiveEditor();
    if (editWidget != NULL)
    {
        if (!cancel)
        {
            valueFromEditor();
        }

        onHideEditor(editWidget);

        if (m_editorIsPrivate)
        {
            editWidget->hide();
        }
        else
        {
            treeWidget()->removeItemWidget(this, 1);
        }
    }

    treeWidget()->setFocus();
}


// internal

QWidget* CBaseProperty::getActiveEditor()
{
    if (treeWidget() != NULL)
    {
        return treeWidget()->itemWidget(this, 1);
    }

    return NULL;
}


void CBaseProperty::setEditorPrivate()
{
    m_editorIsPrivate = true;
}


void CBaseProperty::emitValueChanged()
{
    if (treeWidget() != NULL)
    {
        Q_EMIT treeWidget()->itemChanged(this, 1);
    }
}
