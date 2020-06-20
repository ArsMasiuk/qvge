#include "pathpicker.h"

#include <QHBoxLayout>
#include <QFileDialog>
#include <QStyle>


namespace QSint
{


PathPicker::PathPicker(QWidget *parent) :
    QWidget(parent),
    m_editorEnabled(true),
    m_pickMode(PF_EXISTING_FILE),
    m_dialogMode(DF_DEFAULT)
{
    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->setSpacing(0);
    hbl->setMargin(0);
    setLayout(hbl);

    m_editor = new QLineEdit(this);

    m_button = new QToolButton(this);
    m_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_button->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));

    hbl->addWidget(m_editor);
    hbl->addWidget(m_button);

    connect(m_button, SIGNAL(clicked()), this, SLOT(showPickDialog()));
	connect(m_editor, SIGNAL(editingFinished()), this, SIGNAL(changed()));
}


void PathPicker::setEditorEnabled(bool set)
{
    if (m_editorEnabled != set)
    {
        m_editorEnabled = set;
        m_editor->setEnabled(set);
    }
}


void PathPicker::setObjectsToPick(int flags, bool updateIcon)
{
    if (m_pickMode != flags)
    {
        m_pickMode = flags;

        if (updateIcon)
        {
            QStyle::StandardPixmap iconStyle = QStyle::SP_DialogOpenButton;

            switch (m_pickMode)
            {
            case PF_EXISTING_FILES:
                iconStyle = QStyle::SP_FileDialogEnd;
                break;

            case PF_EXISTING_DIR:
                iconStyle = QStyle::SP_DirIcon;
                break;

            case PF_SAVE_FILE:
                iconStyle = QStyle::SP_DialogSaveButton;
                break;
            }

            m_button->setIcon(style()->standardIcon(iconStyle));
        }
    }
}


void PathPicker::showPickDialog()
{
    Q_EMIT beforePicked();

	QString openDir = m_editor->text().trimmed();
	if (openDir.isEmpty()) {
		openDir = m_dir;
	}

    QString result;
    QString caption(m_caption);

    // default caption if empty
    if (caption.isEmpty()){
        switch (m_pickMode){
        case PF_EXISTING_FILE:
            caption = tr("Choose a file to open");
            break;

        case PF_EXISTING_FILES:
            caption = tr("Choose files to open");
            break;

        case PF_EXISTING_DIR:
            caption = tr("Choose a directory");
            break;

        case PF_SAVE_FILE:
            caption = tr("Choose a file to save");
            break;
        }
    }

    bool isSystem = (m_dialogMode == DF_SYSTEM);

    // use native dialogs?
    if (isSystem) {
        switch (m_pickMode){
        case PF_EXISTING_DIR:
            result = QFileDialog::getExistingDirectory(NULL,
                        caption,
						openDir);
            break;

        case PF_EXISTING_FILE:
            result = QFileDialog::getOpenFileName(NULL,
                        caption,
						openDir,
                        m_filter);
            break;

        case PF_SAVE_FILE:
            result = QFileDialog::getSaveFileName(NULL,
                        caption,
						openDir,
                        m_filter);
            break;

        case PF_EXISTING_FILES:
            {
            QStringList list = QFileDialog::getOpenFileNames(NULL,
                        caption,
						openDir,
                        m_filter);

            if (!list.isEmpty())
                result = list.join(";");

            break;
            }

        default:
            return;
        }
    }

    // use Qt dialogs instead
    if (!isSystem) {
		QFileDialog dialog(NULL, caption, openDir, m_filter);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);

        switch (m_pickMode){
            case PF_EXISTING_FILE:
                dialog.setAcceptMode(QFileDialog::AcceptOpen);
                dialog.setFileMode(QFileDialog::ExistingFile);
                break;

            case PF_EXISTING_FILES:
                dialog.setAcceptMode(QFileDialog::AcceptOpen);
                dialog.setFileMode(QFileDialog::ExistingFiles);
                break;

            case PF_EXISTING_DIR:
                dialog.setAcceptMode(QFileDialog::AcceptOpen);
                dialog.setFileMode(QFileDialog::Directory);
                dialog.setOption(QFileDialog::ShowDirsOnly);
                break;

            case PF_SAVE_FILE:
                dialog.setAcceptMode(QFileDialog::AcceptOpen);
                dialog.setFileMode(QFileDialog::AnyFile);
                break;

            default:
                return;
        }

        if (dialog.exec()) {
            QStringList list = dialog.selectedFiles();
            if (!list.isEmpty())
                result = list.join(";");
        }
    }

    if (result.isEmpty())
        return;

    m_editor->setText(result);

    Q_EMIT picked(result);

	Q_EMIT changed();
}


}

