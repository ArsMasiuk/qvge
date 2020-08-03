#include "timepicker.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QStyle>
#include <QToolButton>
#include <QGroupBox>
#include <QDialog>


namespace QSint
{


TimePicker::TimePicker(QWidget *parent) :
	QWidget(parent)
{
    QHBoxLayout *hbl = new QHBoxLayout();
    setLayout(hbl);

	QGroupBox *hourBox = new QGroupBox(this);
	hourBox->setTitle(tr("Hours"));
	QGridLayout *hourGrid = new QGridLayout();
	hourGrid->setSpacing(0);
	hourGrid->setMargin(1);
	hourBox->setLayout(hourGrid);
	hbl->addWidget(hourBox);

	int hour = 0;
	for (int r = 0; r < 6; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			QToolButton *hourButton = new QToolButton(this);
			hourButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
			hourButton->setText(QString::number(hour));
			hourGrid->addWidget(hourButton, r, c);
			m_hourButtons.addButton(hourButton, hour);

			hour++;
		}
	}

	QGroupBox *minuteBox = new QGroupBox(this);
	minuteBox->setTitle(tr("Minutes"));
	QGridLayout *minuteGrid = new QGridLayout();
	minuteGrid->setSpacing(0);
	minuteGrid->setMargin(1);
	minuteBox->setLayout(minuteGrid);
	hbl->addWidget(minuteBox);

	int minute = 0;
	for (int r = 0; r < 6; r++)
	{
		for (int c = 0; c < 10; c++)
		{
			QToolButton *minuteButton = new QToolButton(this);
			minuteButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
			minuteButton->setText(QString::number(minute));
			minuteGrid->addWidget(minuteButton, r, c);
			m_minuteButtons.addButton(minuteButton, minute);

			minute++;
		}
	}

	m_time = QTime::currentTime();

	connect(&m_hourButtons, SIGNAL(buttonClicked(int)), this, SLOT(hourClicked(int)));
	connect(&m_minuteButtons, SIGNAL(buttonClicked(int)), this, SLOT(minuteClicked(int)));
}


void TimePicker::setTime(const QTime& time)
{
	m_time = time;
}


void TimePicker::hourClicked(int hour)
{
	m_time.setHMS(hour, m_time.minute(), 0, 0);

	Q_EMIT timePicked(m_time);
}


void TimePicker::minuteClicked(int minute)
{
	m_time.setHMS(m_time.hour(), minute, 0, 0);

	Q_EMIT timePicked(m_time);
}


// picker button

TimePickerButton::TimePickerButton(QWidget *parent) : QToolButton(parent), m_editor(NULL)
{
	QFrame *frame = new QFrame();
	frame->setLayout(new QVBoxLayout());
	frame->layout()->addWidget(&m_timePicker);
	frame->layout()->setMargin(0);
	frame->setFrameShape(QFrame::Box);

	m_dialog = new QDialog(this, Qt::Popup);
	m_dialog->setLayout(new QVBoxLayout());
	m_dialog->layout()->addWidget(frame);
	m_dialog->layout()->setMargin(0);

	connect(this, SIGNAL(clicked()), this, SLOT(OnButtonClicked()));
}


void TimePickerButton::OnButtonClicked()
{
	if (m_editor)
	{
		m_timePicker.setTime(m_editor->time());
		connect(&m_timePicker, SIGNAL(timePicked(QTime)), m_editor, SLOT(setTime(QTime)));
	}

	m_dialog->move(parentWidget()->mapToGlobal(pos() + QPoint(-m_dialog->width() / 2, height())));
	m_dialog->exec();

	if (m_editor)
	{
		m_timePicker.disconnect(m_editor);
	}
}


}

