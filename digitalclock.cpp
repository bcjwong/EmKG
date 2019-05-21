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
#include <QTimer>
#include <fcntl.h>
#include <string>
#include <QPainter>
#include <QLine>
#include <QPen>
#include <QPaintEvent>

#include "digitalclock.h"

DigitalClock::DigitalClock()
	:
	x1(0), // initializing the coordinates
	y1(0),
	x2(0),
	y2(0)
	{
		for(int i = 0; i < 100; i++)
		{
			this->value[i] = 30; 
		}

		QTimer* t = new QTimer( this );
		connect( t, SIGNAL( timeout() ), this, SLOT(paintEvent(QPaintEvent * e))); // start up the QpaintEvent when the timer runs out
		t->start( 10 ); // start up the timer every 0.01 seconds 

	}	// The purpose of the timer is to continuously update the screen so the pulse graph appears to be rolling.


void DigitalClock::paintEvent(QPaintEvent * e)
{
	int x1 = 0; // Initialize the x-coordinates 
	int x2 = 5; // Each data are plotted 5 units apart

	int y1,y2;
	int temp;
	int max = this->value[0]; // Determine the highest input voltage from this time frame
	int min = this->value[0]; // Determine the lowest input voltage from this time frame
	int avg = this->value[0]; // Determine the average input voltage from this time frame

	// The purpose of finding highest and lowest is so the screen can always resize based on the signal
	// Resizing will ensure that the signal does not appear too small or too big on the screen. 

	for (int i = 1; i < 100; i++)
	{
		if (value[i] > max)
		{
			max = this->value[i]; // Find the maximum value
		}
		if (value[i] < min)
		{
			min = this->value[i]; // Find the minimum value
		}
		avg += value[i];
	}

	avg = avg/100; // Find the average value


	Q_UNUSED( e );
	QPainter painter(this);
	QString line; // Initilize a string 
	

	QFile file("/proc/mygraph"); // All new data are stored in this procfile
	
	QPen myPen(Qt::black, 2, Qt::SolidLine); // Set a solid black line pen
	painter.setPen(myPen); // Use this pen as the Signal on the screen

	if (file.open(QIODevice::ReadOnly)) // Open the proc file
	{
		
		for(int i = 0; i < 100; i++)
		{
			this->value[i] = this->value[i+1]; // Push all the old data to foward in the array
		}



		QTextStream stream(&file);
		line = stream.readLine(); // Read the value in the proc file as a string
		temp = line.toInt(); // Convert string into interger

		this->value[99] = temp; // Place the newest data at the end of the array
		

		if (max == min)
		{
			max = min+1; // Prevent floating point exception
		}
		

		for(int i = 0; i < 100; i++) // plot out the data onto the screen
		{
			//auto allign max and min in the list for the screen
			y1 = 250 - (this->value[i] - min + 1)*250/(max-min); 
			// This equation flips, and plot the signal on the screen in proportion to the actual size of the signal
			y1 = y1 + 5; // Move the entire signal graph up 5 pixels so there's space to display BPM

			y2 = 250 - (this->value[i+1]-min + 1)*250/(max-min);
			y2 = y2 + 5;

			
			painter.drawLine(x1,y1,x2,y2);
			x1 = x1 + 5; 
			x2 = x2 + 5;
		}

		// Create axis for the signal so the user knows the actual value of each data points on the Signal
		QPen axis(Qt::lightGray, 1, Qt::DashLine);
		painter.setPen(axis);
		y1 = (200 - min + 1)*260/(max-min);
		painter.drawLine(0,y1,500,y1);

		y1 = (400 - min + 1)*260/(max-min);
		painter.drawLine(0,y1,500,y1);

		y1 = (600 - min + 1)*260/(max-min);
		painter.drawLine(0,y1,500,y1);

		y1 = (800 - min + 1)*260/(max-min);
		painter.drawLine(0,y1,500,y1);
	}


	QString text = "PBM: ";
	QFile file2("/dev/mypulse"); // Beats per minute (BPM) is store in the dev file
	if (file2.open(QIODevice::ReadOnly)) // Read from dev file
	{
		QTextStream stream(&file2);
		line = stream.readLine();
		text.append(line);
		temp = line.toInt();

		if(temp == 999999) // Detect if the device is calibrating
		{
			text = "Calibrating";
			painter.setPen(myPen);
		}

		else if (temp > 90) // If BPM is too high the BPM appears read to indicate dangerous hearbeat
		{
			QPen redpen(Qt::red, 1, Qt::SolidLine);
			painter.setPen(redpen);
		}

		else // Set up regular black pen for regular BPM
		{
			painter.setPen(myPen);
		}		

		painter.drawText(200,270, text); // Display BPM onto the bottom of the screen

	}
	
	
	update();
	

}	


