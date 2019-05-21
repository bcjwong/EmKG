#include <QApplication>
#include <QWidget>
#include <QPalette>
#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QIODevice>
#include <QPainter>
#include <QtGui>
#include <QLine>
#include <QPen>
#include <QPaintEvent>
#include "digitalclock.h"



int main(int argc, char **argv){
	QApplication app(argc, argv);
	

	DigitalClock *clock = new DigitalClock();
   	clock->showFullScreen();
	

	return app.exec();
}
