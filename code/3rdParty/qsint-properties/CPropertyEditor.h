#ifndef CPROPERTYEDITOR_H
#define CPROPERTYEDITOR_H

#include <QTreeWidget>
#include <QMap>

class CBaseProperty;
class CBoolProperty;
class CIntegerProperty;


class CPropertyEditor : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CPropertyEditor(QWidget *parent = 0);

    void init();
	void clear();
    void adjustToContents();

    bool add(CBaseProperty* prop);
    bool remove(CBaseProperty* prop);

Q_SIGNALS:
	void valueChanged(CBaseProperty* prop, const QVariant &v);
	void stateChanged(CBaseProperty* prop, bool state);

public Q_SLOTS:
    void onWidgetEditorFinished();
    
private Q_SLOTS:
    void onCurrentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous);
    void onItemClicked(QTreeWidgetItem * item, int column);
    void onItemChanged(QTreeWidgetItem * item, int column);

    // internal signals
Q_SIGNALS:
    void emitValueChanged(CBaseProperty* prop, const QVariant &v);
    void emitStateChanged(CBaseProperty* prop, bool state);

protected:
    virtual void keyPressEvent(QKeyEvent * event);

    QMap<QByteArray, CBaseProperty*> m_propertyMap;
    bool m_addingItem;
};

#endif // CPROPERTYEDITOR_H
