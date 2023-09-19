#include <math.h>

//********************************************************************************************************************
//***********************************************Version 1.1**********************************************************
//*******Please refer to the Document titled: Watermark sensor reading circuit prior to next step*********************
//************Documentation available at : www.irrometer.com/200ss.html ******************************************
// Version 1.0 created 5/13/2021 by Jeremy Sullivan, Irrometer Co Inc.*****************************************************
// Version 1.1 updated 7/21/2023 by Jeremy Sullivan, Irrometer Co Inc.*****************************************************
// Code tested on Arduino UNO R3
// Purpose of this code is to demonstrate valid WM reading code, circuitry and excitation using a voltage divider and "psuedo-ac" method
// This program uses a modified form of Dr. Clint Shock's 1998 calibration equation.

// Sensor to be energized by digital pin 12 or digital pin 8, alternating between HIGH and LOW states

//This is a simplified version of the MEGA_MUX code in the same ZIP. The MEGA_MUX code is an example which uses multiplexers, a temp sensor, and a calibration resistor to read multiple sensors as accurately as possible.
//As a simplified example, this version reads one sensor only and assumes a default temperature of 24C.

//NOTE: the 0.09 excitation time may not be sufficient depending on circuit design, cable lengths, voltage, etc. Increase if necessary to get accurate readings, do not exceed 0.2
//NOTE: this code assumes a 10 bit ADC. If using 12 bit, replace the 1024 in the voltage conversions to 4096

#define num_of_read 1 // number of iterations, each is actually two reads of the sensor (both directions)
const int Rx = 10000;  //fixed resistor attached in series to the sensor and ground...the same value repeated for all WM and Temp Sensor.
const long default_TempC = 24;
const long open_resistance = 35000; //check the open resistance value by replacing sensor with an open and replace the value here...this value might vary slightly with circuit components
const long short_resistance = 200; // similarly check short resistance by shorting the sensor terminals and replace the value here.
const long short_CB = 240, open_CB = 255 ;
const int SupplyV = 5; // Assuming 5V output for SupplyV, this can be measured and replaced with an exact value if required
const float cFactor = 1.1; //correction factor optional for adjusting curve, 1.1 recommended to match IRROMETER devices as well as CS CR1000
int i, j = 0, WM1_CB = 0;
float SenV10K = 0, SenVWM1 = 0, SenVWM2 = 0, ARead_A1 = 0, ARead_A2 = 0, WM_Resistance = 0, WM1_Resistance = 0 ;

void setup()
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  // initialize the pins, 8 and 12 randomly chosen. In the voltage divider circuit example in figure 1(www.irrometer.com/200ss.html), pin 12 is the "Output Pin" and pin 8 is the "GND".
  // if the direction is reversed, the WM1_Resistance A and B formulas would have to be swapped.
  pinMode(8, OUTPUT);
  pinMode(12, OUTPUT);
  //set both low
  digitalWrite(8, LOW);
  digitalWrite(12, LOW);

  delay(100);   // time in milliseconds, wait 0.1 minute to make sure the OUTPUT is assigned
}

void loop()
{
  while (j == 0)
  {

    //Read the first Watermark sensor

    WM1_Resistance = readWMsensor();
    WM1_CB = myCBvalue(WM1_Resistance, default_TempC, cFactor);

    //*****************output************************************

    Serial.print("WM1 Resistance(Ohms)= ");
    Serial.print(WM1_Resistance);
    Serial.print("\n");
    Serial.print("WM1(cb/kPa)= ");
    Serial.print(abs(WM1_CB));
    Serial.print("\n");

    delay(10000);
    //j=1;
  }
}

//conversion of ohms to CB
int myCBvalue(int res, float TC, float cF) {
  int WM_CB;
  res = res * cF;
  if (res > 550.00) {
    if (res > 8000.00) {
      WM_CB = (-2.246 - 5.239 * (res / 1000.00) * (1 + .018 * (TC - 24.00)) - .06756 * (res / 1000.00) * (res / 1000.00) * ((1.00 + 0.018 * (TC - 24.00)) * (1.00 + 0.018 * (TC - 24.00))));
    } else if (res > 1000.00) {
      WM_CB = (-3.213 * (res / 1000.00) - 4.093) / (1 - 0.009733 * (res / 1000.00) - 0.01205 * (TC)) ;
    } else {
      WM_CB = ((res / 1000.00) * 23.156 - 12.736) * (1.00 + 0.018 * (TC - 24.00));
    }
  } else {
    if (res > 300.00)  {
      WM_CB = 0.00;
    }
    if (res < 300.00 && res >= short_resistance) {
      WM_CB = short_CB; //240 is a fault code for sensor terminal short
      Serial.print("Sensor Short WM \n");
    }
  }
  if (res >= open_resistance || res == 0) {

    WM_CB = open_CB; //255 is a fault code for open circuit or sensor not present
    Serial.print("Open or Fault for WM \n");
  }
  return WM_CB;
}

//read ADC and get resistance of sensor
float readWMsensor() {

  ARead_A1 = 0;
  ARead_A2 = 0;

  for (i = 0; i < num_of_read; i++)
  {
    digitalWrite(12, HIGH);
    delay(0.09);
    ARead_A1 += analogRead(A1);
    Serial.print("Reading A1: "); // New Log
    Serial.println(ARead_A1);
    digitalWrite(12, LOW);
    delay(100);

    digitalWrite(8, HIGH);
    delay(0.09);
    ARead_A2 += analogRead(A1);
    Serial.print("Reading A2: "); // New Log
    Serial.println(ARead_A2);
    digitalWrite(8, LOW);
  }
  
  SenVWM1 = ((ARead_A1 / 1024) * SupplyV) / num_of_read;
  Serial.print("Sensor Voltage A: "); // New Log
  Serial.println(SenVWM1, 2); // Print with 2 decimal points
  SenVWM2 = ((ARead_A2 / 1024) * SupplyV) / num_of_read;
  Serial.print("Sensor Voltage B: "); // New Log
  Serial.println(SenVWM2, 2); // Print with 2 decimal points
  
  double WM_ResistanceA = (Rx * (SupplyV - SenVWM1) / SenVWM1);
  Serial.print("WM_ResistanceA (calculated): "); // New Log
  Serial.println(WM_ResistanceA);

  double WM_ResistanceB = Rx * SenVWM2 / (SupplyV - SenVWM2);
  Serial.print("WM_ResistanceB (calculated): "); // New Log
  Serial.println(WM_ResistanceB);

  double WM_Resistance = ((WM_ResistanceA + WM_ResistanceB) / 2);

  return WM_Resistance;
}
