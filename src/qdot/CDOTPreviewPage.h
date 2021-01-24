#ifndef CDOTPREVIEWPAGE_H
#define CDOTPREVIEWPAGE_H

#include <QWidget>
#include <QString>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>

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

Q_SIGNALS:
	void documentChanged(bool changed);

private Q_SLOTS:
	void on_LayoutButton_activated(QVariant data);
	void on_DotEditor_undoAvailable(bool available);

private:
	bool runPreview(const QString &engine, const QString &dotFilePath, QString &svgFilePath, QString* lastError = nullptr) const;
	QString errorNotWritable(const QString &path) const;
	QString errorCannotRun(const QString &path) const;
	QString errorCannotFinish(const QString &path) const;

    Ui::CDOTPreviewPage *ui;
	QGraphicsScene m_previewScene;
	QGraphicsSvgItem m_previewItem;

	QString m_dotFileName;
};

#endif // CDOTPREVIEWPAGE_H
