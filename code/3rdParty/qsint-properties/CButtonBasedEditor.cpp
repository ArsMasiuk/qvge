#include "CButtonBasedEditor.h"

#include <QHBoxLayout>
#include <QEvent>


CButtonBasedEditor::CButtonBasedEditor(QWidget *hostedEditor, QWidget *parent) :
    QWidget(parent),
    m_hostedEditor(hostedEditor)
{
    QHBoxLayout* hbl = new QHBoxLayout();
    hbl->setContentsMargins(0,0,0,0);
    hbl->setSpacing(0);
    setLayout(hbl);

    hbl->addWidget(m_hostedEditor);

    m_button = new QToolButton(this);
    m_button->setText("...");
    hbl->addWidget(m_button);

    connect(m_button, SIGNAL(clicked()), this, SLOT(onEditButtonActivated()));
}


CButtonBasedEditor::~CButtonBasedEditor()
{
    layout()->removeWidget(m_hostedEditor);
    m_hostedEditor->setParent(NULL);
    m_hostedEditor->hide();
}


void CButtonBasedEditor::enableButton(bool on)
{
   m_button->setVisible(on);
}


bool CButtonBasedEditor::event(QEvent *e)
{
    if (e->type() == QEvent::FocusIn)
    {
        m_hostedEditor->setFocus();
        e->accept();
        return true;
    }

    return QWidget::event(e);
}


void CButtonBasedEditor::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    m_hostedEditor->show();
}
