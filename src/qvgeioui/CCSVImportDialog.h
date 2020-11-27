#pragma once

#include <QDialog>


namespace Ui {
	class CCSVImportDialog;
}


class CCSVImportDialog : public QDialog
{
	Q_OBJECT

public:
	CCSVImportDialog(QWidget *parent = 0);
	~CCSVImportDialog();

	void setFileName(const QString &fileName);

	const QString& getLastErrorText() const {
		return m_lastErrorText;
	}

	virtual int exec();

private Q_SLOTS:
	void on_CustomDelim_toggled(bool on);
	void OnDelimToggled(bool on);
	void OnCustomChanged(const QString &text);

private:
	int withError(const QString &text);
	void preview();

	Ui::CCSVImportDialog *ui;

	QString m_fileName;
	QStringList m_lines;
	QString m_lastErrorText;
};

