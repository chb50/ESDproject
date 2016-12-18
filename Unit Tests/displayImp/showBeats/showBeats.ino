//audio visualizer display using 16x16 led matrix of WS2812 leds
#include <Adafruit_NeoPixel.h>
#include "WS2812_Definitions.h"
#include <ffft.h>
#include <math.h>

#define LED_MATRIX_PIN 4 //this is the port that the led matrix will be connected to
#define LED_COUNT 16 //note this is for a single row

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_MATRIX_PIN, NEO_GRB + NEO_KHZ800);

void setup() {

  leds.begin();
  clearLEDs();
  leds.show(); //no led change happens until this function is called, this includes clearing the leds

}

void loop() {
  int amp = 5;
  for(int columnId = 0; columnId < LED_COUNT; columnId++) {
    lightCol(columnId, amp, 0);
  }

}

//this function controls the color of each column of the display
//run this function for each column to get entire spectrum

//columnId designates the column that we want to turn on
//amp is how high the column should produce a color
void lightCol(int columnId, int amp, int palletType) {
   
   clearLEDs();
   //NOTE: i think matrix is hooked up in snake pattern
   //so will need to address every other led column in reverse order
   
   if (amp >= LED_COUNT) { //failsafe
      return;
   }
    //columnId*LED_COUNT is the column offset
   int columnOffset = columnId*LED_COUNT;
   unsigned long int* colorBuff = columnColorPallet(palletType);
   
   if (columnId % 2 == 0) { //then assign as normal
     for (int i = 0; i < amp; ++i) {
        leds.setPixelColor(columnOffset + i, colorBuff[i]);
     }
   } else { //then assign backwards
      for (int i = 0; i < amp; ++i) {
        leds.setPixelColor(columnOffset + (LED_COUNT - 1 - i), colorBuff[i]);
     }
   }

   leds.show(); //display led
   
   delete colorBuff;
}

unsigned long int* columnColorPallet(int palletType){
  unsigned long int* colorBuff = new unsigned long int[LED_COUNT];
  switch(palletType) {
    case 0:
      //green-yello-red color scheme
      colorBuff[0] = GREEN; colorBuff[1] = GREEN; colorBuff[2] = GREEN; colorBuff[3] = GREEN; 
      colorBuff[4] = YELLOW; colorBuff[5] = YELLOW; colorBuff[6] = YELLOW; colorBuff[7] = YELLOW; 
      colorBuff[8] = ORANGE; colorBuff[9] = ORANGE; colorBuff[10] = ORANGE; colorBuff[11] = ORANGE; 
      colorBuff[12] = RED; colorBuff[13] = RED; colorBuff[14] = RED; colorBuff[15] = RED;
    case 1:
      //cool colors: Indigo-purple-blue-skyblue
      colorBuff[0] = INDIGO; colorBuff[1] = INDIGO; colorBuff[2] = INDIGO; colorBuff[3] = INDIGO; 
      colorBuff[4] = PURPLE; colorBuff[5] = PURPLE; colorBuff[6] = PURPLE; colorBuff[7] = PURPLE; 
      colorBuff[8] = BLUE; colorBuff[9] = BLUE; colorBuff[10] = BLUE; colorBuff[11] = BLUE; 
      colorBuff[12] = TURQUOISE; colorBuff[13] = TURQUOISE; colorBuff[14] = TURQUOISE; colorBuff[15] = TURQUOISE;
  }
  return colorBuff;

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


