#include "testwidget.h"
#include "ui_testwidget.h"

#include <CPropertyHeader.h>
#include <CBoolProperty.h>
#include <CIntegerProperty.h>
#include <CDoubleProperty.h>
#include <CStringProperty.h>
#include <CListProperty.h>
#include <CFontProperty.h>
#include <CColorProperty.h>
#include <CDateProperty.h>
#include <CTimeProperty.h>
#include "CDateTimeProperty.h"


TestWidget::TestWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestWidget)
{
    ui->setupUi(this);

    ui->editor->init();

    // Color properties
    CPropertyHeader* colorhead = new CPropertyHeader("ColorHead", "Color Properties");
    ui->editor->add(colorhead);

    CColorProperty* c1 = new CColorProperty(colorhead, "Color1", "Color 1 (user editable)", Qt::red);
    ui->editor->add(c1);

    CColorProperty* c2 = new CColorProperty(colorhead, "Color2", "Color 2 (not user editable)", Qt::green);
    ui->editor->add(c2);
    c2->allowListColorsOnly(true);

    CColorProperty* c3 = new CColorProperty(colorhead, "Color3", "Color 3 (Base colors)", QColor("#345678"));
    ui->editor->add(c3);
    //c3->allowListColorsOnly(true);
    c3->setColorsList(QColorComboBox::baseColors());

    // Boolean properties
    CPropertyHeader* bhead = new CPropertyHeader("BoolHead", "Boolean Properties");
    ui->editor->add(bhead);

    CBoolProperty* b1 = new CBoolProperty(bhead, "SomeBool1", "Boolean 1 On", true);
    ui->editor->add(b1);
    b1->setMarked(Qt::Unchecked);

    CBoolProperty* b2 = new CBoolProperty(b1, "SomeBool2", "Boolean 2 Off", false);
    ui->editor->add(b2);
    b2->setBackground(Qt::yellow);

    CBoolProperty* b3 = new CBoolProperty(bhead, "SomeBool3", "Boolean 3 Disabled", false);
    ui->editor->add(b3);
    b3->setDisabled(true);

    CBoolProperty* b4 = new CBoolProperty(bhead, "SomeBool4", "Boolean 4 On", true);
    ui->editor->add(b4);
    b4->setMarked(Qt::Checked);

    CBoolProperty* b5 = new CBoolProperty(bhead, "SomeBool5", "Boolean 5 Off", false);
    ui->editor->add(b5);
    b5->setMarked(Qt::PartiallyChecked);


    // Integer properties
    CPropertyHeader* inthead = new CPropertyHeader("IntHead", "Integer Properties");
    ui->editor->add(inthead);

    CIntegerProperty* i1 = new CIntegerProperty(inthead, "SomeInt1", "Integer 1", 123, 0);
    ui->editor->add(i1);
    i1->setBackground(Qt::cyan);

    CIntegerProperty* i2 = new CIntegerProperty(i1, "SomeInt2", "Integer 2 (-100..100)", -40, 0, -100, 100);
    ui->editor->add(i2);
    i2->setMarked(Qt::Checked);


    // Double properties
    CPropertyHeader* dblhead = new CPropertyHeader("DblHead", "Double Properties");
    ui->editor->add(dblhead);

    CDoubleProperty* d1 = new CDoubleProperty(dblhead, "SomeDouble1", "Double 1", 45.639, 0, -15.93, 1378789123232.327878273);
    ui->editor->add(d1);


    // Text properties
    CPropertyHeader* txthead = new CPropertyHeader("TxtHead", "Textual Properties");
    ui->editor->add(txthead);

    CStringProperty* s1 = new CStringProperty(txthead, "SomeString1", "A String", "the sun is shining :)");
    ui->editor->add(s1);


    // List properties
    CPropertyHeader* listhead = new CPropertyHeader("ListHead", "List Properties");
    listhead->setMarked(true);
    ui->editor->add(listhead);

    CListData list1;
    list1 << CListDataItem("Item 1", QIcon(":/Info"))
          << CListDataItem("Item 2")
          << CListDataItem("Item 3", QIcon(":/Open"))
          << CListDataItem("Item 4", QIcon(":/Remove"))
          << CListDataItem("Item 5");

    CListProperty* l1 = new CListProperty(listhead, "List1", "List 1", list1, 0);
    ui->editor->add(l1);
    l1->setBackground(Qt::magenta);
    l1->setTextColor(Qt::green);
    l1->setMarked(Qt::Checked);

    CListProperty* l2 = new CListProperty(listhead, "List2", "List 2 (shared List 1)", list1, 5);
    ui->editor->add(l2);
    l2->setMarked(Qt::Unchecked);


    // Font properties
    CPropertyHeader* fonthead = new CPropertyHeader("FontHead", "Font Properties");
    ui->editor->add(fonthead);

    QFont font1("Arial", 10, 100, true);
    CFontProperty* f1 = new CFontProperty(fonthead, "Font1", "Font 1", font1);
    ui->editor->add(f1);

    QFont font2("Courier", 12, 1, false);
    CFontProperty* f2 = new CFontProperty(fonthead, "Font2", "Font 2", font2);
    ui->editor->add(f2);


    // Date & Time properties
    CPropertyHeader* datehead = new CPropertyHeader("DateHead", "Date & Time Properties");
    ui->editor->add(datehead);

    QDate somedate(1980, 7, 15);
    CDateProperty* date1 = new CDateProperty(datehead, "Date1", "Date", somedate);
    ui->editor->add(date1);
    date1->setBackground(Qt::green);
    date1->setTextColor(Qt::black);

    CDateProperty* date2 = new CDateProperty(date1, "Date2", "Date (limited 1985-2005)", somedate);
    date2->setMinimumDate(QDate(1985, 1, 1));
    date2->setMaximumDate(QDate(2005, 12, 31));
    date2->setDisplayFormat("yyyy-MM-dd");
    ui->editor->add(date2);

    QTime sometime(21, 34, 56);
    CTimeProperty* time1 = new CTimeProperty(datehead, "Time1", "Time", sometime);
    ui->editor->add(time1);

    CTimeProperty* time2 = new CTimeProperty(time1, "Time2", "Time (limited)", sometime);
    time2->setTimeRange(QTime(8,0), QTime(20,0));
    time2->setDisplayFormat("h:m:s ap");    // here be Qt bug 49234 under windows...
    ui->editor->add(time2);

    CDateTimeProperty* datetime1 = new CDateTimeProperty(datehead, "DateTime1", "Date & Time", QDateTime(somedate, sometime));
    ui->editor->add(datetime1);


    ui->editor->adjustToContents();
}

TestWidget::~TestWidget()
{
    delete ui;
}

