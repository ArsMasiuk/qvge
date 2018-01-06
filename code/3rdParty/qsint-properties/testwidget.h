#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QWidget>

namespace Ui {
class TestWidget;
}

class TestWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit TestWidget(QWidget *parent = 0);
    ~TestWidget();
    
private:
    Ui::TestWidget *ui;
};

#endif // TESTWIDGET_H
