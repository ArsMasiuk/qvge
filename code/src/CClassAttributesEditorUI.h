/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QWidget>
#include <QList>

class CEditorScene;
class CItem;

class CBaseProperty;

namespace Ui {
class CClassAttributesEditorUI;
}

class CClassAttributesEditorUI : public QWidget
{
    Q_OBJECT

public:
    explicit CClassAttributesEditorUI(QWidget *parent = 0);
    ~CClassAttributesEditorUI();

    //int setupFromItems(CEditorScene& scene, QList<CItem*>& items);

    void setScene(CEditorScene* scene);

protected:
    void connectSignals(CEditorScene* scene);
    void onSceneAttached(CEditorScene* scene);
    void onSceneDetached(CEditorScene* scene);

    void rebuild();

protected Q_SLOTS:
    void onSceneChanged();
    void on_ClassId_currentIndexChanged(int);

private Q_SLOTS:
//	void on_AddButton_clicked();
//	void on_RemoveButton_clicked();
//	void on_Editor_valueChanged(CBaseProperty* prop, const QVariant &v);

private:
    Ui::CClassAttributesEditorUI *ui;

    CEditorScene *m_scene;
};

