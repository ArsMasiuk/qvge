/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "IFileSerializer.h"

class CNode;


class CFileSerializerCSV : public IFileSerializer
{
public:
	// reimp
	virtual QString description() const {
        return "Comma Separated Values";
	}

	virtual QString filters() const {
        return "*.csv";
	}

	virtual QString defaultFileExtension() const {
        return "csv";
	}

	virtual bool loadSupported() const {
		return true;
	}

	virtual bool load(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

	virtual bool saveSupported() const {
		return false;
	}

	virtual bool save(const QString& /*fileName*/, CEditorScene& /*scene*/, QString* lastError = nullptr) const {
		return false;
	}

    // setup
    void setDelimiter(const char d) {
        m_delimiter = d;
    }

private:
    char m_delimiter = ';';
};



