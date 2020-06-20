#ifndef PATHPICKER_H
#define PATHPICKER_H

#include <QLineEdit>
#include <QToolButton>


namespace QSint
{


/**
    \brief Widget combining a line edit and a tool button used for
    selection of file system objects.
    \since 0.2.1

    \image html PathPicker.png An example of PathPicker

    Click on the tool button invokes a modal dialog where you can choose a file,
    a directory or a number of files to open (or a file to save). The selected
    path(es) will be then transferred to the line edit along with emitting the
    picked() signal.

    Using setObjectsToPick() and setDialogType(), you can specify the style of the
    pick dialog and the file system objects (see PickFlags and DialogFlags enums).

    Use setCaption() to modify the default caption of the pick dialog.

    Method setDefaultDirectory() lets you set the initial directory where to point the
    pick dialog.

    Via setFilters() you can specify additional file name wildcards (See Qt documentation
    on the QFileDialog class).

    To customize the look of the tool button (i.e. icon, text, etc.), retrieve the
    pointer to the tool button via button() method. Use editor() method to get the
    line edit widget.
*/
class PathPicker : public QWidget
{
    Q_OBJECT

public:
    /// \brief Defines file system objects to pick.
    enum PickFlags {
        /// single existing file can be picked (default)
        PF_EXISTING_FILE,
        /// one or more existing files can be picked
        PF_EXISTING_FILES,
        /// single existing directory can be picked
        PF_EXISTING_DIR,
        /// a file to save can be picked (non-existing as well)
        PF_SAVE_FILE
    };

    /// \brief Defines pick dialog type
    enum DialogFlags {
        /// use system dialog
        DF_SYSTEM = 0,
        /// use default Qt dialog
        DF_DEFAULT
    };

    /** Constructor.
      */
    explicit PathPicker(QWidget *parent = 0);

	/** Returns current path text.
	*/
	QString currentPath() const { return m_editor->text(); }

	/** Sets current path text.
	*/
	void setCurrentPath(const QString& path) { m_editor->setText(path); }

    /** Enables (default) or disables manual path editor according to \a set parameter.
      */
    void setEditorEnabled(bool set);

    /** Returns \a true if manual path editor is enabled, \a false otherwise.
      */
    inline bool isEditorEnabled() const { return m_editorEnabled; }

    /** Defines objects being picked as \a PickFlags.
     * If \a updateIcon is \a true then style default icon will be assigned to the pick button.
      */
    void setObjectsToPick(int flags, bool updateIcon = true);

    /** Returns \a PickFlags defining objects to pick.
      */
    inline int objectsToPick() const { return m_pickMode; }

    /** Defines pick dialog type as \a DialogFlags.
      */
    void setDialogType(int flags) { m_dialogMode = flags; }

    /** Returns \a DialogFlags defining dialog type.
      */
    inline int dialogType() const { return m_dialogMode; }

    /** Defines the default directory to open when showing the dialog.
      */
    void setDefaultDirectory(const QString& dirName) { m_dir = dirName; }

    /** Returns the default directory which is open when showing the dialog.
      */
	inline QString defaultDirectory() const { return m_dir; }

	/** Returns the currently chosen directory.
	 * \since 0.3
	  */
	inline QString currentDirectory() const { return m_editor->text(); }

    /** Defines the file name filters applied to the filenames.
      */
    void setFilters(const QString& filters) { m_filter = filters; }

    /** Returns the active file name filters applied to the filenames.
      */
    inline QString filters() const { return m_filter; }

    /** Defines the caption of the pick dialog. If no caption has been
        specified, then the default one will be shown.
      */
    void setCaption(const QString& text) { m_caption = text; }

    /** Returns the caption of the pick dialog, or empty string if the
        default one should be used.
      */
    inline QString caption() const { return m_caption; }

    /** Returns the tool button widget.
      */
    QToolButton* button() { return m_button; }

    /** Returns the line edit widget.
      */
    QLineEdit* editor() { return m_editor; }

Q_SIGNALS:
    /** Emitted when user is about to pick an object from the dialog or editor.
      */
    void beforePicked();

    /** Emitted when user picks an object from the dialog or editor.
        \a path
      */
    void picked(const QString& path);

    /** Emitted when path is changed by some way.
      */
	void changed();

protected Q_SLOTS:
    /** Invokes file object pick dialog. When a valid object(s) has been picked
        from the filesystem, the implementation should set the content of the editor
        and emit \a picked() signal.
      */
    virtual void showPickDialog();

protected:
    QLineEdit *m_editor;
    bool m_editorEnabled;

    QToolButton *m_button;

    int m_pickMode;
    int m_dialogMode;

    QString m_filter;
    QString m_dir;
    QString m_caption;
};


}


#endif // PATHPICKER_H
