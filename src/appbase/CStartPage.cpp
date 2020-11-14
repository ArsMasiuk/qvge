#include "CStartPage.h"
#include "CMainWindow.h"

#include <QCommandLinkButton>
#include <QSpacerItem>
#include <QFileInfo>
#include <QDateTime>
#include <QMessageBox>


CStartPage::CStartPage(CMainWindow *parent) : QWidget(parent)
{
	ui.setupUi(this);

	m_parent = parent;

	createActions();
	createRecentDocs();
}

CStartPage::~CStartPage() 
{
}


void CStartPage::createActions()
{
    const auto &docTypes = m_parent->getRegisteredDocumentTypes();

	// create document actions
	for (auto &doc : docTypes)
	{
		if (doc.canCreate)
		{
			QCommandLinkButton *newFileButton = new QCommandLinkButton(
				tr("Create") + " " + doc.name, doc.description, this
			);

			newFileButton->setIcon(QIcon(":/Icons/New"));
			newFileButton->setMinimumHeight(64);
			
			QAction *newFile = new QAction(newFileButton);
			newFile->setData(doc.type);

			connect(newFileButton, &QCommandLinkButton::clicked, newFile, &QAction::triggered);
			connect(newFile, &QAction::triggered, this, &CStartPage::onCreateDocument);

			ui.LeftWidget->layout()->addWidget(newFileButton);
		}
	}

	// spacer
	ui.LeftWidget->layout()->addItem(new QSpacerItem(1, 50));


	// open document actions
	QCommandLinkButton *openFileButton = new QCommandLinkButton(
		tr("Open..."), tr("Open existing document(s)"), this
	);

	openFileButton->setIcon(QIcon(":/Icons/Open"));
	openFileButton->setMinimumHeight(64);

	connect(openFileButton, &QCommandLinkButton::clicked, m_parent, &CMainWindow::selectAndOpenDocument);

	ui.LeftWidget->layout()->addWidget(openFileButton);
}


void CStartPage::onCreateDocument()
{
	QAction *act = dynamic_cast<QAction*>(sender());
	if (act)
		m_parent->createNewDocument(act->data().toByteArray());
}


void CStartPage::createRecentDocs()
{
    const auto &recentList = m_parent->getRecentFilesList();
	int i = 0;

	ui.CleanRecentButton->setVisible(recentList.size());

	for (const QString &fileName : recentList)
	{
		QWidget *host = new QWidget(this);
		host->setLayout(new QHBoxLayout);

		QFileInfo fi(fileName);

		QToolButton *deleteButton = new QToolButton(host);
		deleteButton->setAutoRaise(true);
		deleteButton->setIcon(QIcon(":/Icons/Delete"));
		deleteButton->setToolTip(tr("Remove this file from the list"));

		QAction *deleteAction = new QAction(fileName, deleteButton);
		deleteAction->setData(i);

		connect(deleteButton, &QCommandLinkButton::clicked, deleteAction, &QAction::triggered);
		connect(deleteAction, &QAction::triggered, this, &CStartPage::onRemoveDocument);

		QCommandLinkButton *fileButton = new QCommandLinkButton(
			fi.baseName(),
			fi.lastModified().toString() + " | " + fileName,
			host
		);

		fileButton->setMinimumHeight(64);

		QAction *recentAction = new QAction(fileName, fileButton);
		recentAction->setData(i);

		connect(fileButton, &QCommandLinkButton::clicked, recentAction, &QAction::triggered);
		connect(recentAction, &QAction::triggered, this, &CStartPage::onRecentDocument);

		host->layout()->addWidget(fileButton);
		host->layout()->addWidget(deleteButton);

        ui.RightWidget->layout()->addWidget(host);
		m_buttons[i++] = host;
	}


	// spacer
	QWidget *temp = new QWidget(this);
	temp->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	ui.RightWidget->layout()->addWidget(temp);
}


void CStartPage::onRecentDocument()
{
	QAction *act = dynamic_cast<QAction*>(sender());
	if (act)
		m_parent->openDocument(act->text());
}


void CStartPage::onRemoveDocument()
{
	QAction *act = dynamic_cast<QAction*>(sender());
	if (act)
	{
		int r = QMessageBox::question(this,
			tr("Remove Document"),
			tr("Are you sure to remove the document from the list?\n\n(File itself will not be removed!)"),
			QMessageBox::Yes, QMessageBox::Cancel);

		if (r == QMessageBox::Cancel)
			return;

		if (m_parent->removeRecentDocument(act->text()))
		{
			int i = act->data().toInt();
			QWidget *w = m_buttons[i];
			if (w)
			{
				delete w;
				m_buttons.remove(i);
				ui.CleanRecentButton->setVisible(m_buttons.size());
			}
		}
	}
}


void CStartPage::on_CleanRecentButton_clicked()
{
	int r = QMessageBox::question(this,
		tr("Clean Recent Documents"),
		tr("Are you sure to clean the recent documents list?\n\n(Files will not be removed!)"),
		QMessageBox::Yes, QMessageBox::Cancel);

	if (r == QMessageBox::Yes)
	{
		m_parent->cleanRecentFilesList();

		for (QWidget *w : m_buttons)
		{
			ui.RightWidget->layout()->removeWidget(w);
		}

		m_buttons.clear();

		ui.CleanRecentButton->hide();
	}
}

