/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CATTRIBUTESEDITORUI_H
#define CATTRIBUTESEDITORUI_H

#include <QWidget>
#include <QList>

#include <QtVariantPropertyManager>
#include <QtVariantEditorFactory>

class CEditorScene;
class CItem;

class CPropertyEditorUIBase;


namespace Ui {
class CAttributesEditorUI;
}

class CAttributesEditorUI : public QWidget
{
    Q_OBJECT

public:
    explicit CAttributesEditorUI(QWidget *parent = 0);
    ~CAttributesEditorUI();

    int setupFromItems(CEditorScene& scene, QList<CItem*>& items);

	CPropertyEditorUIBase* getEditor();

private Q_SLOTS:
	void on_AddButton_clicked();
	void on_ChangeButton_clicked();
	void on_RemoveButton_clicked();
	void on_Editor_currentItemChanged(QtBrowserItem*);
    void onValueChanged(QtProperty *property, const QVariant &val);

private:
    Ui::CAttributesEditorUI *ui;

    CEditorScene *m_scene;
	QList<CItem*> m_items;

    QtVariantPropertyManager m_manager;
    QtVariantEditorFactory m_factory;
};

#endif // CATTRIBUTESEDITORUI_H
