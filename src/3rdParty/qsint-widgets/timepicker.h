#ifndef TIMEPICKER_H
#define TIMEPICKER_H

#include <QWidget>
#include <QTime>
#include <QButtonGroup>
#include <QToolButton>
#include <QTimeEdit>


namespace QSint
{

/**
 * @brief The TimePicker class allows to choose a time moment in a hour:minute form via simple UI.
 * \since 0.4
 */
class TimePicker : public QWidget
{
    Q_OBJECT

public:
    /** Constructor.
      */
	explicit TimePicker(QWidget *parent = 0);

	/** Returns current time.
	*/
	QTime time() const
		{ return m_time; }

public Q_SLOTS:
	/** Sets current time.
	*/
	void setTime(const QTime& time);

Q_SIGNALS:
	void timePicked(const QTime& time);

private Q_SLOTS:
	void hourClicked(int hour);
	void minuteClicked(int minute);

protected:
	QTime m_time;
	QButtonGroup m_hourButtons;
	QButtonGroup m_minuteButtons;
};


// button with popup dialog

class TimePickerButton : public QToolButton
{
	Q_OBJECT

public:
	TimePickerButton(QWidget *parent = 0);

	void setTimeEdit(QTimeEdit *timeEdit) 	{ m_editor = timeEdit; }

    TimePicker*	pickerWidget()			{ return &m_timePicker; }
	QDialog*		dialogWidget()			{ return m_dialog; }

private Q_SLOTS:
	void OnButtonClicked();

protected:
	QDialog* m_dialog;
	QTimeEdit* m_editor;
	TimePicker m_timePicker;
};


}


#endif // TIMEPICKER_H
