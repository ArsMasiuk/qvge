#ifndef CDOTPREVIEWPAGE_H
#define CDOTPREVIEWPAGE_H

#include <QWidget>

namespace Ui {
class CDOTPreviewPage;
}

class CDOTPreviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit CDOTPreviewPage(QWidget *parent = nullptr);
    ~CDOTPreviewPage();

    bool load(const QString &fileName, QString *lastError = nullptr);

private:
    Ui::CDOTPreviewPage *ui;
};

#endif // CDOTPREVIEWPAGE_H
