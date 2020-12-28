/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2017 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QString>
#include <QPrinter>
#include <QSettings>
#include <QPageSetupDialog>

#include "qvgelib/IFileSerializer.h"


class CPDFExport : public IFileSerializer
{
public:
	CPDFExport();
	virtual ~CPDFExport();

	// setup interface
	void readSettings(QSettings& settings);
	void writeSettings(QSettings& settings);
	bool setupDialog(CEditorScene& scene);

	// reimp: IFileSerializer
	virtual QString description() const {
		return "Adobe Portable Document Format";
	}

	virtual QString filters() const {
		return "Adobe Portable Document Format (*.pdf)";
	}

	virtual QString defaultFileExtension() const {
		return "pdf";
	}

	virtual bool loadSupported() const {
		return false;
	}

	virtual bool load(const QString& /*fileName*/, CEditorScene& /*scene*/, QString* /*lastError = nullptr*/) const {
		return false;
	}

	virtual bool saveSupported() const {
		return true;
	}

	virtual bool save(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

private:
	mutable QPrinter *m_printer = nullptr;
	QPageSetupDialog m_pageDialog;
};
