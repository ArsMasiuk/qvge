/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QtCore/QSettings>

#include "qvge/IFileSerializer.h"


class CNode;

class CFileSerializerXGR : public IFileSerializer
{
public:
	// reimp
	virtual QString description() const {
		return "QVGE graph scene format";
	}

	virtual QString filters() const {
		return "*.xgr";
	}

	virtual QString defaultFileExtension() const {
		return "xgr";
	}

	virtual bool loadSupported() const {
		return true;
	}

	virtual bool load(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

	virtual bool saveSupported() const {
		return true;
	}

	virtual bool save(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;
};

