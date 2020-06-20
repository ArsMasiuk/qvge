/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerXGR.h"
#include "CEditorScene.h"
#include "ISceneItemFactory.h"

#include <QtCore/QFile>
#include <QtCore/QDataStream>


// static reader with DPSE format support

class CDPSERecoder : public ISceneItemFactory
{
public:
    virtual CItem* createItemOfType(const QByteArray& typeId, const CEditorScene& scene) const
    {
        if (typeId == "CBranchNode")
        {
            return scene.createItemOfType("CNode");
        }

        if (typeId == "CFanNode")
        {
            return scene.createItemOfType("CNode");
        }

        if (typeId == "CBranchConnection" || typeId == "CDirectConnection")
        {
            return scene.createItemOfType("CDirectEdge");
        }

        return nullptr;
    }
};

static CDPSERecoder s_dpseRecoder;


// reimp

bool CFileSerializerXGR::load(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	// read file into document
	QFile openFile(fileName);
	if (!openFile.open(QIODevice::ReadOnly))
		return false;

	scene.reset();

    scene.setItemFactoryFilter(&s_dpseRecoder);

	QDataStream ds(&openFile);
#if (QT_VERSION >= 0x050a00)
	ds.setVersion(QDataStream::Qt_5_10);
#endif

	scene.restoreFrom(ds, true);

    scene.setItemFactoryFilter(nullptr);

    scene.addUndoState();

	return true;
}


bool CFileSerializerXGR::save(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	QFile saveFile(fileName);
	if (saveFile.open(QFile::WriteOnly))
	{
		QDataStream ds(&saveFile);
#if (QT_VERSION >= 0x050a00)
		ds.setVersion(QDataStream::Qt_5_10);
#endif

		scene.storeTo(ds, true);

		return true;
	}

	return false;
}
