//led test script
#include <Adafruit_NeoPixel.h>
#include "WS2812_Definitions.h"

#define LED_PIN 4
#define LED_COUNT 32
#define COLUMN 2

uint8_t iter = 0;
uint8_t backiter = LED_COUNT;
bool backtrack = false;

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // put your setup code here, to run once:
  leds.begin();
  clearLEDs();
  leds.show();
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly
  if(!backtrack) {
    leds.setPixelColor(iter, YELLOW);
    displayBound(iter+1, iter+3);
    leds.setPixelColor(iter + 4, BLUE);
    Serial.println("test");
    iter++;
  } else {
    leds.setPixelColor(iter, BLACK);
    leds.setPixelColor(iter + 1, BLACK);
    leds.setPixelColor(iter + 2, BLUE);
    iter--;
  }

  if(iter == LED_COUNT or iter == 0) {
    backtrack = not backtrack;
  }
  leds.show();

  delay(500);
}

void displayBound(uint8_t from, uint8_t to) {
  for(int i = from; i < to; ++i) {
    leds.setPixelColor(i, BLACK);
  }
}

void clearLEDs()
{
  for (int i=0; i<LED_COUNT; i++)
  {
    leds.setPixelColor(i, 0);
  }
}
