#include <FastLED.h>           // Only being used for fast math functions
#include <Adafruit_NeoPixel.h> // Actual LED output

#include "palettes.h" // Contains palette definitions

#define LED_PIN 1
#define CHIPSET SK6812
#define NUM_LEDS 46

#define BRIGHTNESS 224       //
#define FRAMES_PER_SECOND 64 //
uint8_t SPEED = 5;           // noise speed

// I use adafruit neopixel because the strip is RGBW and expects 32 bit colors
// FastLED has fast math functions, but is only RGB (24 bit colors) so it doesn't work with my LEDs

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

CRGBPalette16 gPal;

// Plan on using Galois Linear Feedback Shift Register (gLFSR)
// Provides predictable random numbers very cheaply.
uint16_t random_number = 0x9CE7U; // must be nonzero seed

// Galois LFSR taps for random number generator
#define LFSRTAPS 0xB400U // Determines sequence of random numbers

// The 16 bit version of our coordinates for noise
static uint16_t z;

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t SCALE = 900; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
uint8_t noise[NUM_LEDS];

void setup()
{
  delay(500); // sanity delay

  gPal = lava_gp;
  strip.begin();

  // Randomize starting location
  z = grandom();

  // This is a simple POST, light all the LEDs for a few seconds
  //
  do
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      // position in palette
      uint8_t hue = i * (255 / NUM_LEDS);
      CRGB color = GreenHeat(hue + (millis() / 10));
      // CRGB color = ColorFromPalette(gPal, hue + seconds16(), BRIGHTNESS);
      strip.setPixelColor(i, color.r, color.g, color.b);
    }
    strip.setBrightness(25);
    strip.show();
    delay(10);

  } while (seconds16() <= 5);

  //  delay(4000);
}

void loop()
{
  static uint8_t startIndex = 0;
  startIndex += SPEED; /* motion speed */

  fillnoise8();
  //Fire2012WithPalette(); // run simulation frame, using palette colors
  PaletteDraw(startIndex);
  ChangePalettePeriodically();
  strip.show();

  delay(1000 / FRAMES_PER_SECOND);

  random_number = lfsr16_next(random_number); // cycle the LFSR
  random16_add_entropy(random_number);
}

CRGB GreenHeat(uint8_t temperature)
{
  // TODO: Turn this into a function that returns packed uint32_t color
  CRGB heatcolor;

  // Scale 'heat' down from 0-255 to 0-191,
  // which can then be easily divided into three
  // equal 'thirds' of 64 units each.
  uint8_t t192 = scale8_video(temperature, 191);

  // calculate a value that ramps up from
  // zero to 255 in each 'third' of the scale.
  uint8_t heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2;                 // scale up to 0..252

  // now figure out which third of the spectrum we're in:
  if (t192 & 0x80)
  {
    // we're in the hottest third
    heatcolor.r = qadd8(0xC0U, random8(0x3FU)); // shimmer a bit
    heatcolor.g = qadd8(0xC0U, random8(0x3FU)); // shimmer a bit
    heatcolor.b = heatramp;                     // scale blue
  }
  else if (t192 & 0x40)
  {
    // we're in the middle third
    heatcolor.r = heatramp;                     //
    heatcolor.g = qadd8(0xC0U, random8(0x3FU)); // shimmer max green
    heatcolor.b = 0;                            //
  }
  else
  {
    // we're in the coolest third
    heatcolor.r = 0;                               //
    heatcolor.g = heatramp;                        //
    heatcolor.b = qsub8(random8(0x3FU), heatramp); // shimmer blue at the bottom
  }
  return heatcolor;
}

void ChangePalettePeriodically()
{
  /* TODO 
 *    Turn this into a function that returns a palette
 *    want to see if something like this is smaller
 * switch(secondHand){  // would only run once per second? could remove the if; shaving 10 bytes
 * case  0: return gPal = LavaColors_p;
 * case 15: return gPal = HeatColors_p;
 * 
 * } 
Could also turn this into a playlist style function that moves
Through an array of palette objects
Could tune this to a playlist :
Move through 6 palettes in sequence then repeat the last two forever .
With blending... 
 * 
  */

  uint8_t secondHand = seconds16() % 60;
  static uint8_t lastSecond = 59;

  if (lastSecond != secondHand) // the effect of this is
  {                             // to only run the following
    lastSecond = secondHand;    // once per second. I think
                                // it could be cleaner/faster
    switch (secondHand)
    {
    case 0:
    {
      //  gPal = honeycomb_gp;
      break;
    }
    case 15:
    {
      gPal = LavaColors_p;
      break;
    }
    case 30:
    {
      // gPal = HeatColors_p;
      break;
    }
    case 45:
    {
      // gPal = lava_gp;
      break;
    }
    default:
      return;
      /*
    the effect looks like dripping/scrolling/sweeping when SCALE < 200
    and shimmering/rippling when SCALE > 200
    noted function size is compared to not setting scale at all here.
    uncomment the ones you want
*/
      // SCALE WARPING
      // SCALE = secondHand * 17; // 28 bytes sweep scale from 0 to 1024 over 60 secs
      // SCALE = 200 + (6 * triwave8(secondHand && 255)); // 42 bytes variable ripple; not sure if visible
      // SCALE = triwave8(secondHand && 255);             // 32 bytes looks like dripping
      // SPEED WARPING
      //SPEED = quadwave8(mul8(secondHand, 5)); // 42 bytes warp the speed of the effect
    }
  }
}

//
void PaletteDraw(uint8_t colorIndex)
{

  for (int i = 0; i < NUM_LEDS; i++)
  {
    CRGB color = ColorFromPalette(gPal, noise[i], BRIGHTNESS, LINEARBLEND);
    //CRGB color = GreenHeat(noise[i]);
    strip.setPixelColor(i, color.r, color.g, color.b);
  }
  strip.setBrightness(BRIGHTNESS);
}

uint16_t lfsr16_next(uint16_t n)
{
  return (n >> 0x01U) ^ (-(n & 0x01U) & LFSRTAPS);
}
uint16_t grandom(void)
{
  return (random_number = lfsr16_next(random_number));
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8()
{
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if (SPEED < 50)
  {
    dataSmoothing = 200 - (SPEED * 4);
  }

  for (int j = 0; j < NUM_LEDS; j++)
  {
    uint8_t data = inoise8(z + (j * SCALE));

    // The range of the inoise8 function is roughly 16-238.
    // These two operations expand those values out to roughly 0..255
    // You can comment them out if you want the raw noise data.
    data = qsub8(data, 16);
    data = qadd8(data, scale8(data, 39));

    if (dataSmoothing)
    {
      uint8_t olddata = noise[j];
      uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
      data = newdata;
    }

    noise[j] = data;
  }

  z += SPEED;

  // apply slow drift to X and Y, just for visual variation.
}
