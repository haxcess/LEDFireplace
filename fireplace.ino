#include <FastLED.h>
#include <Adafruit_NeoPixel.h>

#include "palettes.h"

#define LED_PIN 1
#define CHIPSET SK6812
#define NUM_LEDS 46
// 46 LEDS 3740 flash, 155 ram
//         3738

#define BRIGHTNESS 64
#define FRAMES_PER_SECOND 100
#define SPEED 1 // noise speed

bool gReverseDirection = true;

//CRGB leds[NUM_LEDS];

//byte noise[NUM_LEDS];  // the noisefield

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

CRGBPalette16 gPal;

void setup()
{
  delay(1500); // sanity delay

  gPal = PartyColors_p;
  //gPal = bhw4_018_gp;
  strip.begin();
  strip.setBrightness(BRIGHTNESS);

  for (int i = 0; i < NUM_LEDS; i++)
  {
    // position in palette
    uint8_t hue = i * (255 / NUM_LEDS);
    CRGB color = ColorFromPalette(gPal, hue, BRIGHTNESS, LINEARBLEND);
    strip.setPixelColor(i, color.r, color.g, color.b);
  }
  strip.show();

  delay(3000);
}

void loop()
{
  static uint8_t startIndex = 0;
  startIndex += SPEED; /* motion speed */

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random());

  Fire2012WithPalette(); // run simulation frame, using palette colors
  //spacenoise(startIndex);

  strip.show();

  delay(1000 / FRAMES_PER_SECOND);
  ChangePalettePeriodically();
}

void ChangePalettePeriodically()
{
  uint8_t secondHand = (millis() / 1000) % 60;
  static uint8_t lastSecond = 60;

  if (lastSecond != secondHand)
  {
    lastSecond = secondHand;
    switch (secondHand)
    {
    case 0:
    {
      gPal = PartyColors_p;
      break;
    }
    case 15:
    {
      gPal = ForestColors_p;
      break;
    }
    case 30:
    {
      gPal = LavaColors_p;
      break;
    }
    case 45:
    {
      gPal = green_crystal_gp;

      break;
    }
    }
  }
}

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLING 20

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 200

void spacenoise(uint8_t colorIndex)
{

  for (int i = 0; i < NUM_LEDS; i++)
  {
    CRGB color = ColorFromPalette(gPal, sin8(colorIndex++), BRIGHTNESS, LINEARBLEND);

    strip.setPixelColor(i, color.r, color.g, color.b);
  }
}

void Fire2012WithPalette()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; i++)
  {
    //heat[i] = qsub8( heat[i],  random8(0, (10 * COOLING) / NUM_LEDS));
    heat[i] = qsub8(heat[i], random8(0, COOLING));
  }

  // Step 2.  Heat from each cell drifts 'out' and diffuses a little

  for (int k = NUM_LEDS - 1; k >= 1; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 1] + heat[k - 2]) / 3;
    //heat[k] = avg8(heat[k - 1], heat[k + 1]) /2;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING)
  {
    int y = random8(NUM_LEDS);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < NUM_LEDS; j++)
  {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    //byte colorindex = scale8( heat[j], 253);
    byte colorindex = heat[j];
    CRGB color = ColorFromPalette(gPal, colorindex + 1);
    int pixelnumber;
    if (gReverseDirection)
    {
      pixelnumber = (NUM_LEDS - 1) - j;
    }
    else
    {
      pixelnumber = j;
    }
    //strip.setPixelColor(pixelnumber,     color.r, color.g, color.b, scale8(heat[pixelnumber], BRIGHTNESS / 10));
    strip.setPixelColor(pixelnumber, color.r, color.g, color.b, 0);
  }
}
