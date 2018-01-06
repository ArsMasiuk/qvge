#ifndef CBASEPROPERTY_H
#define CBASEPROPERTY_H

#include <QtWidgets/QTreeWidgetItem>

class CBaseProperty: public QTreeWidgetItem
{
public:
    CBaseProperty(const QByteArray& id, const QString &name);
    CBaseProperty(CBaseProperty* top, const QByteArray& id, const QString &name);

    const QByteArray& getId() const { return m_id; }
    const QString& getName() const { return m_name; }

    void setMarked(bool on = true);
    bool isMarked() const;
    bool isMarkable() const;

    void setBackground(const QBrush& bg);
    void setTextColor(const QColor& color);

    // handlers to reimplement
    virtual void onAdded() {}
    virtual void onEnter() {}
    virtual void onLeave() { finishEdit(); }
    virtual void onShowEditor(QWidget* /*editWidget*/) {}
    virtual void onHideEditor(QWidget* /*editWidget*/) {}
    virtual bool onKeyPressed(QKeyEvent* /*event*/, QWidget* /*editWidget*/) { return false; }

    // actions to reimplement
    virtual QVariant getVariantValue() const = 0;
    virtual void setValue();
    virtual void validateValue() {}
    virtual void displayValue() {}

    virtual QWidget* createEditor() const { return NULL; }
    virtual void valueToEditor() {}
    virtual void valueFromEditor() {}
    virtual void startEdit();
    virtual void finishEdit(bool cancel = false);
    virtual bool isEditorWindow(QWidget* editor, QWidget* window) const { return editor == window; }

protected:
    friend class CPropertyEditor;

    QWidget* getActiveEditor();
    void setEditorPrivate();

    void emitValueChanged();

    QByteArray m_id;
    QString m_name;

    bool m_isMarkable;

    bool m_editorIsPrivate;
};

#endif // CBASEPROPERTY_H
