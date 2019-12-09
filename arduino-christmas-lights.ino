/**
 * Arduino Christmas Lights.
 * By Peter Bloomfield: peter.bloomfield.online
 * 
 * This sketch controls a chain of RGB LEDs and makes them twinkle in various colour schemes.
 * The chain must be compatible with Adafruit Neopixel.
 * 
 * Set g_dataPin to the pin you are using to control the LEDs.
 * Set g_ledCount to the number of LEDs in your chain.
 * 
 * This is designed to be controlled remotely, e.g. from a Bluetooth module on Serial1 by default.
 * Send the following ASCII characters to the Arduino to have the specified effect:
 * 
 * 1-9 = Set duration of one cycle of twinkling in seconds (lower number means faster flashing). NOTE: This must be the ASCII characters 1 to 9, not the binary values 1-9.
 * n = New random twinkling pattern.
 * r = Use red colour scheme.
 * y = Use yellow colour scheme.
 * g = Use green colour scheme.
 * c = Use cyan colour scheme.
 * b = Use blue colour scheme.
 * m = Use magenta colour scheme.
 * p = Use purple colour scheme.
 * w = Use bright white colour scheme.
 * o = Use off-white colour scheme.
 * x = Use xmas colour scheme (red and green).
 */

#include <Adafruit_NeoPixel.h>

auto & MySerial{ Serial1 };
const int g_indicatorPin = 13;
const int g_dataPin = 8;
const int g_ledCount = 100;

// The duration in seconds of one standard cycle of the lights.
float g_cycleDuration = 5.0f;

Adafruit_NeoPixel g_leds{ g_ledCount, g_dataPin, NEO_RGB + NEO_KHZ800 };

enum class ColourScheme
{
  red,
  yellow,
  green,
  cyan,
  blue,
  magenta,
  purple,
  white,
  offWhite,
  xmas // red and green
};

auto g_colourScheme{ ColourScheme::xmas };

// Seed the random number generator by sampling an analog pin.
// The pin should be floating.
void randomSeedFromAdc(const int analogPin)
{
  uint16_t seed = 0;
  for (int i = 0; i < 16; ++i)
  {
    if ((analogRead(analogPin) % 2) == 0)
    {
      seed |= (1 << i);
    }
    delay(11);
  }
  randomSeed(seed);
}

// Returns a random number in the range 0 to 1.
float getRandomFraction()
{
  return static_cast<float>(random(1000)) / 1000.0f;
}

// Convert a floating point value in the range 0 to 1 to a byte value in the range 0 to 255.
byte toByte(const float fraction)
{
  if (fraction <= 0.0f)
    return 0;

  if (fraction >= 1.0f)
    return 255;

  return static_cast<byte>(fraction * 255);
}

// Describes the cyclic behaviour of one light component's brightness.
struct LightParams
{
  LightParams() = default;
  LightParams(const float maxBrightness, const float phase) : maxBrightness{ maxBrightness }, phase{ phase } {}

  // Generate a random set of light parameters.
  // The light's maximum brightness will be a random number between b1 and b2.
  // NOTE: b1 and b2 must be in the range 0 to 1 inclusive, and b1 must be less than b2.
  // The phase will always be a random number between 0 and 1.
  static LightParams makeRandom(float b1, float b2)
  {
    if (b1 < 0.0f)
      b1 = 0.0f;
    else if (b1 > 1.0f)
      b1 = 1.0f;

    if (b2 < 0.0f)
      b2 = 0.0f;
    else if (b2 > 1.0f)
      b2 = 1.0f;

    if (b2 < b1)
      b2 = b1;
    
    return LightParams{
      b1 + (getRandomFraction() * (b2 - b1)),
      // Phase will be in range 0 to 1
      getRandomFraction()
    };
  }

  // Calculate how bright the light should currently be.
  // Returns a value between 0 (off) and 1 (fully on).
  // TODO: Make this take a single value containing the proportion of time through the cycle.
  // TODO: Do all the math with integers instead of floats, and use a restricted lookup table instead of sin().
  float calculateCurrentBrightness(const float elapsedSeconds, const float cycleDuration)
  {
    // TODO: Adjust sine wave so that it dips below zero; the light should stay off while in negative brightness.
    //       This will mean more lights are off giving a more distinct twinkle effect.
    const float timeScaling{ (2 * PI) / cycleDuration };
    const float x{ timeScaling * (elapsedSeconds - (phase * cycleDuration)) };
    //const float y{ (0.5f * maxBrightness * sin(x)) + (0.5f * maxBrightness) };
    const float y{ maxBrightness * sin(x) };
    
    if (y < 0.0f)
      return 0.0f;

    if (y > 1.0f)
      return 1.0f;

    return y;
  }
  
  // The maximum brightness that can be reached by this light.
  // It will fade in and out on a sine wave.
  float maxBrightness{ 1.0f };

  // The time offset of the sine wave.
  // This is measured in proportions of a whole cycle.
  // For example, if the cycle duration is 3 seconds, then a phase of 0.5
  //  will offset this light's cycle by 1.5 seconds.
  float phase{ 1.0f };
};

LightParams g_params[g_ledCount];

void updateIndicator()
{
  // Flash the indicator LED on and off every second.
  if (micros() % 1000000 < 500000)
    digitalWrite(g_indicatorPin, HIGH);
  else
    digitalWrite(g_indicatorPin, LOW);
}

void randomiseLightParams()
{
  for (int i = 0; i < g_ledCount; ++i)
  {
    g_params[i] = LightParams::makeRandom(0.5f, 1.0f);
  }
}

void handleCommand(char command)
{
  if (command >= '1' && command <= '9')
  {
    g_cycleDuration = static_cast<float>(command - '1' + 1);
    return;
  }
  
  switch(command)
  {
  case 'n': case 'N':
    randomiseLightParams();
    break;
    
  case 'r': case 'R':
    g_colourScheme = ColourScheme::red;
    break;

  case 'y': case 'Y':
    g_colourScheme = ColourScheme::yellow;
    break;

  case 'g': case 'G':
    g_colourScheme = ColourScheme::green;
    break;

  case 'c': case 'C':
    g_colourScheme = ColourScheme::cyan;
    break;
  
  case 'b': case 'B':
    g_colourScheme = ColourScheme::blue;
    break;

  case 'm': case 'M':
    g_colourScheme = ColourScheme::magenta;
    break;

  case 'p': case 'P':
    g_colourScheme = ColourScheme::purple;
    break;

  case 'w': case 'W':
    g_colourScheme = ColourScheme::white;
    break;

  case 'o': case 'O':
    g_colourScheme = ColourScheme::offWhite;
    break;

  case 'x': case 'X':
    g_colourScheme = ColourScheme::xmas;
    break;
  }
}

void setup()
{
  pinMode(g_indicatorPin, OUTPUT);
  pinMode(g_dataPin, OUTPUT);
  
  digitalWrite(g_indicatorPin, LOW);

  randomiseLightParams();

  MySerial.begin(9600);
}

void loop()
{
  if (MySerial.available())
    handleCommand(static_cast<char>(MySerial.read()));
  
  const auto curMicros{ micros() };
  const float elapsedSeconds{ (float)curMicros / 1000000.0f };

  // Calculate and display the lights for the current time point.
  for (int i{ 0 }; i < g_ledCount; ++i)
  {
    const float brightness{ g_params[i].calculateCurrentBrightness(elapsedSeconds, g_cycleDuration) };
    const byte brightnessByte{ toByte(brightness) };

    byte red{ 0 };
    byte green{ 0 };
    byte blue{ 0 };

    switch (g_colourScheme)
    {
      case ColourScheme::red:
        red = brightnessByte;
        break;

      case ColourScheme::yellow:
        red = brightnessByte;
        green = brightnessByte;
        break;

      case ColourScheme::green:
        green = brightnessByte;
        break;

      case ColourScheme::cyan:
        green = brightnessByte;
        blue = brightnessByte;
        break;

      case ColourScheme::blue:
        blue = brightnessByte;
        break;

      case ColourScheme::magenta:
        red = brightnessByte;
        blue = brightnessByte;
        break;

      case ColourScheme::purple:
        red = toByte(brightness * 0.5f);
        blue = brightnessByte;
        break;

      case ColourScheme::white:
        red = brightnessByte;
        green = brightnessByte;
        blue = brightnessByte;
        break;

      case ColourScheme::offWhite:
        red = brightnessByte;
        green = toByte(brightness * 0.7f);
        blue = toByte(brightness * 0.35f);
        break;

      case ColourScheme::xmas:
        if ((i % 2) == 0)
          red = brightnessByte;
        else
          green = brightnessByte;
        break;
    }
  
    g_leds.setPixelColor(i, red, green, blue);
  }

  g_leds.show();
  
  updateIndicator();
}
