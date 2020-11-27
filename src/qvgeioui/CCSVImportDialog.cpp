#include "CCSVImportDialog.h"
#include "ui_CCSVImportDialog.h"

#include <QFile>
#include <QTextStream>


CCSVImportDialog::CCSVImportDialog(QWidget *parent): 
	QDialog(parent),
	ui(new Ui::CCSVImportDialog)
{
	ui->setupUi(this);

	connect(ui->CommaDelim, SIGNAL(toggled(bool)), this, SLOT(OnDelimToggled(bool)));
	connect(ui->PointCommaDelim, SIGNAL(toggled(bool)), this, SLOT(OnDelimToggled(bool)));
	connect(ui->TabDelim, SIGNAL(toggled(bool)), this, SLOT(OnDelimToggled(bool)));
	connect(ui->CustomDelim, SIGNAL(toggled(bool)), this, SLOT(OnDelimToggled(bool)));
	connect(ui->CustomDelimEdit, SIGNAL(textChanged(const QString &)), this, SLOT(OnCustomChanged(const QString &)));
}


CCSVImportDialog::~CCSVImportDialog()
{
}


void CCSVImportDialog::setFileName(const QString & fileName)
{
	m_fileName = fileName;
}


int CCSVImportDialog::exec()
{
	if (!QFile::exists(m_fileName))
	{
		return withError(tr("%1 does not exist").arg(m_fileName));
	}

	QFile f(m_fileName);
	if (!f.open(QFile::ReadOnly))
	{
		return withError(tr("%1 cannot be read").arg(m_fileName));
	}

	QTextStream ts(&f);
	int linesCount = 0;
	while (!ts.atEnd() && linesCount++ < 10)
		m_lines << ts.readLine();

	preview();

	return QDialog::exec();
}


void CCSVImportDialog::OnDelimToggled(bool on)
{
	if (on) preview();
}


void CCSVImportDialog::OnCustomChanged(const QString &text)
{
	if (text.size()) preview();
}


void CCSVImportDialog::preview()
{
	ui->PreviewTable->setUpdatesEnabled(false);
	ui->PreviewTable->reset();

	QString sep;
	if (ui->CommaDelim->isChecked())			sep = ",";
	else if (ui->PointCommaDelim->isChecked())	sep = ";";
	else if (ui->TabDelim->isChecked())			sep = "\t";
	else										sep = ui->CustomDelimEdit->text();

	for (int i = 0; i < m_lines.size(); ++i)
	{
		const QString& line = m_lines.at(i);
		auto refs = line.splitRef(sep);

		for (int x = 0; x < refs.size(); ++x)
		{
			auto item = ui->PreviewTable->setCellText(i, x, refs.at(x));
			if (i)
				item->setFlags(Qt::ItemIsEnabled);
			else
				item->setFlags(Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		}
	}

	ui->PreviewTable->setUpdatesEnabled(true);


	ui->RawPreview->setText(m_lines.join("\n"));
}


void CCSVImportDialog::on_CustomDelim_toggled(bool on)
{
	ui->CustomDelimEdit->setEnabled(on);

	if (on)
		ui->CustomDelimEdit->setFocus();
}


int CCSVImportDialog::withError(const QString &text)
{
	m_lastErrorText = text;
	return Rejected;
}

