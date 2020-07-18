#pragma once

#include <QString>

class CPlatformWin32
{
public:
	enum DdeCommand {
		DDEOpen = 0x0001, /* open a file via explorer*/
		DDENew = 0x0002, /* create a new file via explorer*/
		DDEPrint = 0x0004, /* print a file via explorer*/
	};
	Q_DECLARE_FLAGS(DdeCommands, DdeCommand)

	static void registerFileType(const QString& documentId,
		const QString& fileTypeName,
		const QString& fileExtension,
		qint32 appIconIndex = 0,
		DdeCommands commands = DDEOpen);

	static void registerCommand(const QString& command,
		const QString& documentId,
		const QString cmdLineArg = QString::null,
		const QString ddeCommand = QString::null);

private:
	static bool SetHkcrUserRegKey(QString key,
		const QString& value,
		const QString& valueName = QString::null);
};
