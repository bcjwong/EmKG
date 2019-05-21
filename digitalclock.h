
#ifndef DIGITALCLOCK_H
#define DIGITALCLOCK_H

#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QPainter>
#include <QtGui>
#include <QLine>
#include <QPen>
#include <QPaintEvent>


class DigitalClock : public QWidget
{
	Q_OBJECT

	public:
		DigitalClock();
		QLabel* label;
		int value[100];	//initialize a array of 100 values to store the voltage data			

	protected:
		void paintEvent( QPaintEvent* event );

	private:
		int x1;
		int y1;
		int x2;
		int y2;

};


#endif
