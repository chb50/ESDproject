//audio visualizer display using 16x16 led matrix of WS2812 leds
#include <Adafruit_NeoPixel.h>
#include "WS2812_Definitions.h"
#include <avr/pgmspace.h>
#include <ffft.h>
#include <math.h>

//FFT stuff -- for now we will get an 8x8 display running
//when all goes well (hopefully), then we will upgrade to a 16x16

// Microphone connects to Analog Pin 0.  Corresponding ADC channel number
// varies among boards...it's ADC0 on Uno and Mega, ADC7 on Leonardo.
// Other boards may require different settings; refer to datasheet.
#define ADC_CHANNEL 0 //will be connecting to ADC channel 0 for this project

int16_t       capture[FFT_N];    // Audio capture buffer
complex_t     bfly_buff[FFT_N];  // FFT "butterfly" buffer
uint16_t      spectrum[FFT_N/2]; // Spectrum output buffer
volatile byte samplePos = 0;     // Buffer position counter

byte
  peak[8],      // Peak level of each column; used for falling dots
  dotCount = 0, // Frame counter for delaying dot-falling speed
  colCount = 0; // Frame counter for storing past column data
int
  col[8][10],   // Column levels for the prior 10 frames
  minLvlAvg[8], // For dynamic adjustment of low & high ends of graph,
  maxLvlAvg[8], // pseudo rolling averages for the prior few frames.
  colDiv[8];    // Used when filtering FFT output to 8 columns

/* AS PER PICCOLO.PDE...
These tables were arrived at through testing, modeling and trial and error,
exposing the unit to assorted music and sounds.  But there's no One Perfect
EQ Setting to Rule Them All, and the graph may respond better to some
inputs than others.  The software works at making the graph interesting,
but some columns will always be less lively than others, especially
comparing live speech against ambient music of varying genres.
*/
static const uint8_t PROGMEM
  // This is low-level noise that's subtracted from each FFT output column:
  noise[64]={ 8,6,6,5,3,4,4,4,3,4,4,3,2,3,3,4,
              2,1,2,1,3,2,3,2,1,2,3,1,2,3,4,4,
              3,2,2,2,2,2,2,1,3,2,2,2,2,2,2,2,
              2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,4 },
  // These are scaling quotients for each FFT output column, sort of a
  // graphic EQ in reverse.  Most music is pretty heavy at the bass end.
  eq[64]={
    255, 175,218,225,220,198,147, 99, 68, 47, 33, 22, 14,  8,  4,  2,
      0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  // When filtering down to 8 columns, these tables contain indexes
  // and weightings of the FFT spectrum output values to use.  Not all
  // buckets are used -- the bottom-most and several at the top are
  // either noisy or out of range or generally not good for a graph.
  col0data[] = {  2,  1,  // # of spectrum bins to merge, index of first
    111,   8 },           // Weights for each bin
  col1data[] = {  4,  1,  // 4 bins, starting at index 1
     19, 186,  38,   2 }, // Weights for 4 bins.  Got it now?
  col2data[] = {  5,  2,
     11, 156, 118,  16,   1 },
  col3data[] = {  8,  3,
      5,  55, 165, 164,  71,  18,   4,   1 },
  col4data[] = { 11,  5,
      3,  24,  89, 169, 178, 118,  54,  20,   6,   2,   1 },
  col5data[] = { 17,  7,
      2,   9,  29,  70, 125, 172, 185, 162, 118, 74,
     41,  21,  10,   5,   2,   1,   1 },
  col6data[] = { 25, 11,
      1,   4,  11,  25,  49,  83, 121, 156, 180, 185,
    174, 149, 118,  87,  60,  40,  25,  16,  10,   6,
      4,   2,   1,   1,   1 },
  col7data[] = { 37, 16,
      1,   2,   5,  10,  18,  30,  46,  67,  92, 118,
    143, 164, 179, 185, 184, 174, 158, 139, 118,  97,
     77,  60,  45,  34,  25,  18,  13,   9,   7,   5,
      3,   2,   2,   1,   1,   1,   1 },
  // And then this points to the start of the data for each of the columns:
  * const colData[]  = {
    col0data, col1data, col2data, col3data,
    col4data, col5data, col6data, col7data };

//LED stuff
#define LED_MATRIX_PIN 4 //this is the port that the led matrix will be connected to
#define LED_COUNT 256

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_MATRIX_PIN, NEO_GRB + NEO_KHZ800);

byte pallet;


int val = 0;


void setup() {

  //FFT stuff provided by Piccolo.pde
  uint8_t i, j, nBins, binNum, *data;

  memset(peak, 0, sizeof(peak));
  memset(col , 0, sizeof(col));

  for(i=0; i<8; i++) {
    minLvlAvg[i] = 0;
    maxLvlAvg[i] = 512;
    data         = (uint8_t *)pgm_read_word(&colData[i]);
    nBins        = pgm_read_byte(&data[0]) + 2;
    binNum       = pgm_read_byte(&data[1]);
    for(colDiv[i]=0, j=2; j<nBins; j++)
      colDiv[i] += pgm_read_byte(&data[j]);
  }

  // Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion
  ADMUX  = ADC_CHANNEL; // Channel sel, right-adj, use AREF pin
  ADCSRA = _BV(ADEN)  | // ADC enable
           _BV(ADSC)  | // ADC start
           _BV(ADATE) | // Auto trigger
           _BV(ADIE)  | // Interrupt enable
           _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
  ADCSRB = 0;                // Free run mode, no high MUX bit
  DIDR0  = 1 << ADC_CHANNEL; // Turn off digital input for ADC pin
  TIMSK0 = 0;                // Timer0 off

  sei(); // Enable interrupts

  //LED stuff
  leds.begin();
  clearLEDs();
  leds.show(); //no led change happens until this function is called, this includes clearing the leds

  Serial.begin(9600);

}

//make sure uart is disconnected before uploading to board
void loop() {
  pallet = Serial.read();
  Serial.println(pallet);
//   clearLEDs();
//   leds.show();

//  FFT stuff provided by Piccolo.pde
  uint8_t  i, x, L, *data, nBins, binNum, weighting, c;
  uint16_t minLvl, maxLvl;
  int      level, y, sum;

  while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish

  fft_input(capture, bfly_buff);   // Samples -> complex #s
  samplePos = 0;                   // Reset sample counter
  ADCSRA |= _BV(ADIE);             // Resume sampling interrupt
  fft_execute(bfly_buff);          // Process complex data
  fft_output(bfly_buff, spectrum); // Complex -> spectrum

  // Remove noise and apply EQ levels
  for(x=0; x<FFT_N/2; x++) {
    L = pgm_read_byte(&noise[x]);
    spectrum[x] = (spectrum[x] <= L) ? 0 :
      (((spectrum[x] - L) * (256L - pgm_read_byte(&eq[x]))) >> 8);
  }

  //for threading: each thread requires ~36 bytes. 288 bytes taken from ram
  // Downsample spectrum output to 8 columns:
  for(x=0; x<8; x++) {
    data   = (uint8_t *)pgm_read_word(&colData[x]);
    nBins  = pgm_read_byte(&data[0]) + 2;
    binNum = pgm_read_byte(&data[1]);
    for(sum=0, i=2; i<nBins; i++)
      sum += spectrum[binNum++] * pgm_read_byte(&data[i]); // Weighted
    col[x][colCount] = sum / colDiv[x];                    // Average
    minLvl = maxLvl = col[x][0];
    for(i=1; i<10; i++) { // Get range of prior 10 frames
      if(col[x][i] < minLvl)      minLvl = col[x][i];
      else if(col[x][i] > maxLvl) maxLvl = col[x][i];
    }
    // minLvl and maxLvl indicate the extents of the FFT output, used
    // for vertically scaling the output graph (so it looks interesting
    // regardless of volume level).  If they're too close together though
    // (e.g. at very low volume levels) the graph becomes super coarse
    // and 'jumpy'...so keep some minimum distance between them (this
    // also lets the graph go to zero when no sound is playing):
    if((maxLvl - minLvl) < 8) maxLvl = minLvl + 8;
    minLvlAvg[x] = (minLvlAvg[x] * 7 + minLvl) >> 3; // Dampen min/max levels
    maxLvlAvg[x] = (maxLvlAvg[x] * 7 + maxLvl) >> 3; // (fake rolling average)

    // Second fixed-point scale based on dynamic min/max levels:
    level = 10L * (col[x][colCount] - minLvlAvg[x]) /
      (long)(maxLvlAvg[x] - minLvlAvg[x]);

    // Clip output and convert to byte:
    if(level < 0L)      c = 0;
    else if(level > 10) c = 10; // Allow dot to go a couple pixels off top
    else                c = (uint8_t)level;

    if(c > peak[x]) peak[x] = c; // Keep dot on top

    // The 'peak' dot color varies, but doesn't necessarily match
    // the three screen regions...yellow has a little extra influence.
    y = 8 - peak[x]; //this is the amplitude for the column at a particlar time

    //LED stuff
    //x = column
    //y = row (amp)
//    lightCol8(x, c, pallet);
//    Serial.print(peak[x]); //TODO: peak[x] may be amplitude, idk yet
//    Serial.print(" ");

    setLEDSon(x, c, pallet);

  }


//  leds.show(); //display led

  //val = ADC;
//  delay(500);

}


//ISR provided by Piccolo.pde
ISR(ADC_vect) { // Audio-sampling interrupt
  static const int16_t noiseThreshold = 4;
  int16_t              sample         = ADC; // 0-1023

  capture[samplePos] =
    ((sample > (512-noiseThreshold)) &&
     (sample < (512+noiseThreshold))) ? 0 :
    sample - 512; // Sign-convert for FFT; -512 to +511

  if(++samplePos >= FFT_N) ADCSRA &= ~_BV(ADIE); // Buffer full, interrupt off
}

//this function controls the color of each column of the display
//run this function for each column to get entire spectrum


//TODO: addressing the matrix is actually the worst thing in the world
//gunna need to fix the inexing to acount for its snake-like pattern
uint8_t* matrixAddr(int columnId) {
  //so the addressing on this matrix sucks cause ins really an led strip that was arranged into an array
  //so addressing is backwards for every other column if you want to use it on a column by column basis

  static uint8_t rangeBuff[2]; //declared static to return pointer from func
  //NOTE: inclusive range (includes end value)
  //first value specifies columnId parity
  //if columnId is even, then we address as normal
  //if columnId is odd, then we address in reverse order (begin value is higher than end value)

  switch(columnId) {
     case 0:
      rangeBuff[0] = 0;
      rangeBuff[1] = 15;
      break;
     case 1:
      rangeBuff[0] = 31;
      rangeBuff[1] = 16;
      break;
     case 2:
      rangeBuff[0] = 32;
      rangeBuff[1] = 47;
      break;
     case 3:
      rangeBuff[0] = 63;
      rangeBuff[1] = 48;
      break;
     case 4:
      //Serial.print("test\n");
      rangeBuff[0] = 64;
      rangeBuff[1] = 79;
      break;
     case 5:
      rangeBuff[0] = 95;
      rangeBuff[1] = 80;
      break;
     case 6:
      rangeBuff[0] = 96;
      rangeBuff[1] = 111;
      break;
     case 7:
      rangeBuff[0] = 127;
      rangeBuff[1] = 112;
      break;
     case 8:
      rangeBuff[0] = 128;
      rangeBuff[1] = 143;
      break;
     case 9:
      rangeBuff[0] = 159;
      rangeBuff[1] = 144;
      break;
     case 10:
      rangeBuff[0] = 160;
      rangeBuff[1] = 175;
      break;
     case 11:
      rangeBuff[0] = 191;
      rangeBuff[1] = 176;
      break;
     case 12:
      rangeBuff[0] = 192;
      rangeBuff[1] = 207;
      break;
     case 13:
      rangeBuff[0] = 223;
      rangeBuff[1] = 208;
      break;
     case 14:
      rangeBuff[0] = 224;
      rangeBuff[1] = 239;
      break;
     case 15:
      rangeBuff[0] = 255;
      rangeBuff[1] = 240;
      break;
  }

  return rangeBuff;

}

//for 8x8 led display
unsigned long int* columnColorPallet8(byte palletType){
  static unsigned long int* colorBuff = new unsigned long int[10];
  switch(palletType) {
//    case 0: // test Case
//      //green-yello-red color scheme
//      colorBuff[0] = GREEN; colorBuff[1] = GREEN;
//      colorBuff[2] = YELLOW; colorBuff[3] = YELLOW;
//      colorBuff[4] = ORANGE; colorBuff[5] = ORANGE;
//      colorBuff[6] = RED; colorBuff[7] = RED;
//      colorBuff[8] = NAVY; colorBuff[9] = NAVY;
//      break;
//    case 1: //test case
//      //cool colors: Indigo-purple-blue-skyblue
//      colorBuff[0] = INDIGO; colorBuff[1] = INDIGO;
//      colorBuff[2] = PURPLE; colorBuff[3] = PURPLE;
//      colorBuff[4] = BLUE; colorBuff[5] = BLUE;
//      colorBuff[6] = TURQUOISE; colorBuff[7] = TURQUOISE;
//      break;

     case 253:
      //green-yello-red color scheme
      colorBuff[0] = GREEN; colorBuff[1] = GREEN;
      colorBuff[2] = YELLOW; colorBuff[3] = YELLOW;
      colorBuff[4] = ORANGE; colorBuff[5] = ORANGE;
      colorBuff[6] = ORANGE; colorBuff[7] = ORANGE;
      colorBuff[8] = RED; colorBuff[9] = RED;
      break;
     case 251:
      //cool colors: Indigo-purple-blue-skyblue
      colorBuff[0] = INDIGO; colorBuff[1] = INDIGO;
      colorBuff[2] = PURPLE; colorBuff[3] = PURPLE;
      colorBuff[4] = BLUE; colorBuff[5] = BLUE;
      colorBuff[6] = TURQUOISE; colorBuff[7] = TURQUOISE;
      break;
     case 247: //white - pink - red
      colorBuff[0] = WHITE; colorBuff[1] = WHITE;
      colorBuff[2] = DEEPPINK; colorBuff[3] = DEEPPINK;
      colorBuff[4] = RED; colorBuff[5] = RED;
      colorBuff[6] = DARKRED; colorBuff[7] = DARKRED;
      break;
  }
  return colorBuff;
}

//have to assign every led, even if they should be off
void blackOut(uint8_t from, uint8_t to) {
  for(int i = from; i < to; ++i) {
    leds.setPixelColor(i, BLACK);
  }
}

// Sets all LEDs to off, but DOES NOT update the display;
// call leds.show() to actually turn them off after this.
void clearLEDs()
{
  for (int i=0; i<LED_COUNT; i++)
  {
    leds.setPixelColor(i, 0);
  }
}


void setLEDSon(int col, int amp, byte palletType) {

  unsigned long int* colorBuff = columnColorPallet8(palletType);
  
  int count = 0;
  for (int i=col*32-16; i<col*32+32; i++){
//    Serial.print("i: ");
//    Serial.print(i);
//    Serial.print("num: ");
//    Serial.println(num);
    if (amp > count) {
      if (palletType == 255) {
        switch(col) {
          case 1:
            leds.setPixelColor(i, RED);
            break;
          case 2:
            leds.setPixelColor(i, ORANGE);
            break;
          case 3:
            leds.setPixelColor(i, YELLOW);
            break;
          case 4:
            leds.setPixelColor(i, GREEN);
            break;
          case 5:
            leds.setPixelColor(i, BLUE);
            break;
          case 6:
            leds.setPixelColor(i, INDIGO);
            break;
          case 7:
            leds.setPixelColor(i, VIOLET);
            break;
          case 8:
            leds.setPixelColor(i, WHITE);
            break;
          default:
            leds.setPixelColor(i, WHITE);
            break;
        }
      }
      else {
        leds.setPixelColor(i, colorBuff[count]);
      }
    }
    else {
      leds.setPixelColor(i, 0);
    }
    count++;
//    Serial.println("test\n");
  }
//  Serial.println("set color");
  leds.show();
}

