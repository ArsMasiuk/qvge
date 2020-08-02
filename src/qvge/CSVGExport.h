/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L.Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QString>

#include "qvge/IFileSerializer.h"


class CSVGExport : public IFileSerializer
{
public:
	CSVGExport(bool cutContent = true, int resolution = 0) :
		m_cutContent(cutContent),
		m_resolution(resolution)
	{}

	// reimp
	virtual QString description() const {
		return "Scalable Vector Graphics Format";
	}

	virtual QString filters() const {
		return "Scalable Vector Graphics (*.svg)";
	}

	virtual QString defaultFileExtension() const {
		return "svg";
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
	bool m_cutContent = true;
	int m_resolution = 96;
};
