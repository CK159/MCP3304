// Read the MCP3304 ADC in single ended mode (12 bit precision)
// Averages each of the 8 chanels over a few hundred samples, print results
// 32,000 samples/second with very little time to spare
// Extension of http://playground.arduino.cc/Code/MCP3208 for Leonardo and MCP3304 in single ended mode.
// Actually does something semi-useful (Reads, averages and prints all chanels) unlike Arduino examples

//Wiring
//Leonardo -> MCP3304
//SELPIN   -> 10 (CS/SHDN)
//MOSI     -> 11 (Din)
//MISO     -> 12 (Dout)
//SCK      -> 13 (CLK)

// for Uno
//#define SELPIN 10    // chip-select
//#define DATAOUT 11   // MOSI 
//#define DATAIN 12    // MISO 
//#define SPICLOCK 13  // Clock 

// for Mega2560 / Max32
//#define SELPIN 53    // chip-select
//#define DATAOUT 51   // MOSI 
//#define DATAIN 50    // MISO 
//#define SPICLOCK 52  // Clock 

#include <SPI.h>

#define SELPIN 7                //chip-select for ADC SPI Communication
#define LEDPIN 13              //ON when processing/reading ADC
unsigned long cycleTime = 200;//milliseconds to spend in each iteration of loop()
long reading[8];             //stores the 8 readings from the 8 ADC chanels
int samples = 800;          //number of samples to take from each chanel in each iteration of loop()
long count = 0;
//Samples per second = (1000 / cycleTime) * num chanels * samples
//This sketch performance:
//Max at 2MHZ:  800 samples, 32000/second
//Max at 1MHZ:  550 samples, 22000/second
//Max at .5MHZ: 350 samples, 14000/second
//See SPI.setClockDivider() to adjust speed

void setup(){ 
  //set pin modes 
  pinMode(SELPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT); 

  // disable device to start with
  digitalWrite(SELPIN, HIGH);
  digitalWrite(LEDPIN, LOW);
  
  //slow the SPI bus down
  //DIV64, .25MHZ (8.5 ksps)
  //DIV32, .5MHZ  (14.6 ksps)
  //DIV16, 1MHZ   (23.3 ksps)
  //DIV8,  2MHZ   (32 ksps)
  SPI.setClockDivider( SPI_CLOCK_DIV8 ); 
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0); //SPI 0,0 as per MCP330x data sheet 
  SPI.begin();

  Serial.begin(115200); 
}

void loop() {
  unsigned long startTime = millis();
  
  //reset accumulated readings
  for (byte i = 0; i < 8; i++)
  {
    reading[i] = 0;
  }
  
  //Read and accumulate the specified number of samples from each of the 8 chanels
  digitalWrite(LEDPIN, HIGH);
  for (int s = 0; s < samples; s++)
  {
    for (byte i = 0; i < 8; i++)
    {
      reading[i] += read_adc(i);
    }
  }
  digitalWrite(LEDPIN, LOW);
  
  count++;
  Serial.println(count);
  //Print out average and raw value of each chanel
  for (byte i = 0; i < 8; i++)
  {
    long avg = reading[i] / samples;
    Serial.print(i);
    Serial.print(": ");
    Serial.println(avg);         //averaged total for this chanel
    Serial.println(reading[i]); //raw accumulated total of this chanel
  }

  //How much of our cycle time is left to sleep?
  //Be sure to check for negative numbers if this loop took longer than the time limit
  unsigned long endTime = startTime + cycleTime;
  unsigned long theTime = millis();
  Serial.print("S: ");
  if (theTime > endTime)
  {
    Serial.print('-');
    Serial.print(theTime - endTime);
    Serial.println('!');
    //No time to sleep
    return;
  }
  unsigned long sleep = endTime - theTime;
  Serial.println(sleep);
  delay(sleep);
}

//From http://playground.arduino.cc/Code/MCP3208 
//0 - 7 to select ADC chanel to read in single ended mode
int read_adc(int channel){
  int adcvalue = 0;
  int b1 = 0, b2 = 0;
  int sign = 0;
  
  //Command bits #1 and #2 format:
  //0000SM12
  //30000000
  //S: Start bit of 1 begins data transfer
  //M: 1 = Single ended mode, 0 = differential mode
  //1,2,3: Chanel bits (D2, D1, D0 in datasheet)
  //Everything after this: Wait for ADC to process sample

  digitalWrite (SELPIN, LOW); // Select adc
  
  byte commandbits = B00001100;  // first byte
  //Use this for differential mode
  //byte commandbits = B00001000;
  
  commandbits |= (channel >> 1);   // high bit of channel
  SPI.transfer(commandbits);       // send out first byte of command bits

  // second byte; Bx0000000; leftmost bit is D0 channel bit
  commandbits = B00000000;
  commandbits |= (channel << 7);        // if D0 is set it will be the leftmost bit now
  b1 = SPI.transfer(commandbits);       // send out second byte of command bits

  // hi byte will have XX0SBBBB
  // set the top 3 don't care bits so it will format nicely
  b1 |= B11100000;
  sign = b1 & B00010000;
  int hi = b1 & B00001111;

  // read low byte
  b2 = SPI.transfer(b2);              // don't care what we send
  int lo = b2;
  digitalWrite(SELPIN, HIGH); // turn off device

  int reading = hi * 256 + lo;

  if (sign) {
    reading = (4096 - reading) * -1;
  }

  return (reading);
}

