//  Variables
int PulseSensorPurplePin = 0;         // Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0

int LED13 = 13;         //The on-board Arduion LED

int Signal;                // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 550;            // Determine which Signal to "count as a beat", and which to ingore.


// The SetUp Function:
void setup() {
  pinMode(LED13,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(11, OUTPUT);          // pins 11-4 represent the binary digits of the incoming analog signal
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  //pinMode(3, OUTPUT);         // pins 0, 1 are unused because because TX->1 and 0->RX
  //pinMode(2, OUTPUT);         // pins 2 and 3 are unused in order to free up GPIO pins on the gumstix for the button and LED.  
  //pinMode(1, OUTPUT);
  //pinMode(0, OUTPUT);
   Serial.begin(9600);         // Set's up Serial Communication at certain speed.

}   

// The Main Loop Function
void loop() {

  Signal = analogRead(PulseSensorPurplePin);      // Reads the PulseSensor's value and ssigns this value to the "Signal" variable.

  Serial.println(Signal);                    // Send the Signal value to Serial Plotter.

  int bit;
  
  //Reads the bit value of the signal, and assigns each pin with 0 or 1. 
  //Signal value is between 0-1000. This means we only need 10 bits to represent the number.
  for(int i = 11; i >= 4; i--)          // Sets output value to pins. 
  {
    bit = bitRead(Signal, i-2 );           //bitRead converts signal to bits by deciding which bit to read (9 to 0) and returns 0 or 1. 
    if (bit == 1)
    {
      digitalWrite(i, HIGH);           // Sets output pin to 1
    }
    else
    {
      digitalWrite(i, LOW);            // Sets output pin to 0
    }
  }
  Serial.println();

}
