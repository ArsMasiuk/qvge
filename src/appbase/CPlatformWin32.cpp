// ————————————————————————————————-
/**
 * @file
 * @brief
 * @author Gerolf Reinwardt
 * @date 30. march 2011
 *
 * Copyright © 2011, Gerolf Reinwardt. All rights reserved.
 *
 * Simplified BSD License
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Gerolf Reinwardt.
 */
 // ————————————————————————————————-

#include "CPlatformWin32.h"

#include <windows.h>

#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>

void CPlatformWin32::registerFileType(const QString& documentId,
	const QString& fileTypeName,
	const QString& fileExtension,
	qint32 appIconIndex,
	DdeCommands commands)
{
	// first register the type ID of our server
	if (!SetHkcrUserRegKey(documentId, fileTypeName))
		return;

	QString exePath = QFileInfo(qApp->applicationFilePath()).absoluteFilePath();
	QString nativePath = QDir::toNativeSeparators(exePath);

	if (!SetHkcrUserRegKey(QString("%1\\DefaultIcon").arg(documentId),
		QString("\"%1\",%2").arg(nativePath).arg(appIconIndex)))
		return;

	if (commands & DDEOpen)
		registerCommand("Open", documentId, "\"%1\"", "[open(\"%1\")]");
	if (commands & DDENew)
		registerCommand("New", documentId, "-new \"%1\"", "[new(\"%1\")]");
	if (commands & DDEPrint)
		registerCommand("Print", documentId, "-print \"%1\"", "[print(\"%1\")]");

	LONG lSize = _MAX_PATH * 2;
	wchar_t szTempBuffer[_MAX_PATH * 2];
	LONG lResult = ::RegQueryValue(HKEY_CLASSES_ROOT,
		(const wchar_t*)fileExtension.utf16(),
		szTempBuffer,
		&lSize);

	QString temp = QString::fromUtf16((unsigned short*)szTempBuffer);

	if (lResult != ERROR_SUCCESS || temp.isEmpty() || temp == documentId)
	{
		// no association for that suffix
		if (!SetHkcrUserRegKey(fileExtension, documentId))
			return;

		SetHkcrUserRegKey(QString("%1\\Shell\\New").arg(fileExtension), QLatin1String(""), QLatin1String("NullFile"));
	}
}


void CPlatformWin32::registerCommand(const QString& command,
	const QString& documentId,
	const QString cmdLineArg,
	const QString ddeCommand)
{
	QString exePath = QFileInfo(qApp->applicationFilePath()).absoluteFilePath();
	QString commandLine = QDir::toNativeSeparators(exePath);
	commandLine.prepend(QLatin1String("\""));
	commandLine.append(QLatin1String("\""));

	if (!cmdLineArg.isEmpty())
	{
		commandLine.append(QChar(' '));
		commandLine.append(cmdLineArg);
	}

	if (!SetHkcrUserRegKey(QString("%1\\Shell\\%2\\Command").arg(documentId).arg(command), commandLine))
		return; // just skip it

	if (!SetHkcrUserRegKey(QString("%1\\Shell\\%2\\ddeexec").arg(documentId).arg(command), ddeCommand))
		return;
}


bool CPlatformWin32::SetHkcrUserRegKey(QString key,
	const QString& value,
	const QString& valueName)
{
	HKEY hKey;

	key.prepend(QString::fromUtf8("Software\\Classes\\"));

	LONG lRetVal = RegCreateKeyW(HKEY_CURRENT_USER,
		(const wchar_t*)key.utf16(),
		&hKey);

	if (ERROR_SUCCESS == lRetVal) {
		LONG lResult = ::RegSetValueExW(hKey,
			valueName.isEmpty() ? 0 : (const wchar_t*)valueName.utf16(),
			0,
			REG_SZ,
			(CONST BYTE*)value.utf16(),
			(value.length() + 1) * sizeof(wchar_t));

		if ((::RegCloseKey(hKey) == ERROR_SUCCESS) && (lResult == ERROR_SUCCESS)) {
			return true;
		}

		QMessageBox::warning(0, QString::fromUtf8("Error in setting Registry values"),
			QString::fromUtf8("registration database update failed for key '%s'.").arg(key));
	}
	else {
		wchar_t buffer[4096];
		::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, lRetVal, 0, buffer, 4096, 0);
		QString szText = QString::fromUtf16((const ushort*)buffer);
		QMessageBox::warning(0, QString::fromUtf8("Error in setting Registry values"), szText);
	}

	return false;
}

