#include <FastLED.h>
#include <Adafruit_NeoPixel.h>

#include "palettes.h"

#define LED_PIN     1
#define CHIPSET     SK6812
#define NUM_LEDS    46
// 46 LEDS 3740 flash, 155 ram
//         3738



#define BRIGHTNESS  255
#define FRAMES_PER_SECOND 100
#define SPEED 40 // noise speed

bool gReverseDirection = true;

//CRGB leds[NUM_LEDS];

//byte noise[NUM_LEDS];  // the noisefield


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

CRGBPalette16 gPal;

void setup() {
  delay(1500); // sanity delay

  gPal = radioactive_slime_gp;
  //gPal = bhw4_018_gp;
  strip.begin();
  strip.setBrightness(BRIGHTNESS);

  // position in palette
  uint8_t colorindex = 0;

  for (int i = 0; i < NUM_LEDS; i++){
    colorindex = lerp8by8(i,BRIGHTNESS,NUM_LEDS);
    CRGB color = ColorFromPalette( gPal, colorindex);
    strip.setPixelColor(i,     color.r, color.g, color.b);
    colorindex +=3;
  }
  strip.show();

  delay(3000);
}

void loop() {

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());


  Fire2012WithPalette(); // run simulation frame, using palette colors
  strip.show();

  delay(1000 / FRAMES_PER_SECOND);

}

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLING  20

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 200

void spacenoise() {
  static byte noise[NUM_LEDS];
  static uint16_t coord_x;
  static uint16_t coord_time;

  coord_time += SPEED;

  // step 1; fill noise with noise
  //void fill_raw_noise8(uint8_t *pData, uint8_t num_points, uint8_t octaves, uint16_t x, int scalex, uint16_t time);
  fill_raw_noise8(noise, NUM_LEDS, 1, coord_x, 57771, coord_time );

  //step 2; map noise to palette color
  for ( int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex =  noise[j] / 2;
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    //strip.setPixelColor(pixelnumber,     color.r, color.g, color.b, scale8(heat[pixelnumber], BRIGHTNESS / 10));
    strip.setPixelColor(pixelnumber,     color.r, color.g, color.b);

  }



}


void Fire2012WithPalette()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    //heat[i] = qsub8( heat[i],  random8(0, (10 * COOLING) / NUM_LEDS));
    heat[i] = qsub8( heat[i],  random8(0, COOLING));

  }

  // Step 2.  Heat from each cell drifts 'out' and diffuses a little

  for ( int k = NUM_LEDS - 1; k >= 1; k--) {
    heat[k] = (heat[k - 1] + heat[k -1] + heat[k - 2] ) / 3;
    //heat[k] = avg8(heat[k - 1], heat[k + 1]) /2;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(NUM_LEDS);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    //byte colorindex = scale8( heat[j], 253);
    byte colorindex = heat[j];
    CRGB color = ColorFromPalette( gPal, colorindex + 1);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    //strip.setPixelColor(pixelnumber,     color.r, color.g, color.b, scale8(heat[pixelnumber], BRIGHTNESS / 10));
    strip.setPixelColor(pixelnumber,     color.r, color.g, color.b, 0);

  }
}
