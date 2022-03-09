#include <Arduino.h>
#include <math.h>
#include "types.h"
#include "menu.h"
#include "menuData.h"

//#define DEBUG
#define STRIP_ENABLED
#define NUMLEDS 300
#define STRIP_PIN 9

#ifdef STRIP_ENABLED
#include "microLED.h"
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB, CLI_AVER> strip;
#endif
MenuNavigator<10000> menuNavigator(Serial, menu);

Mode mode = SMOOTH;
const Gamut::Key KEYS_RAINBOW[]{{0, 0xff0000}, {0xffU, 0xffff00}, {0x1ff, 0x00ff00}, {0x2ff, 0x00ffff}, {0x3ff, 0x0000ff}, {0x4ff, 0xff00ff}, {0x5ff, 0xff0000}};
const Gamut::Key KEYS_TRICOLOR[]{{0, 0x00ffffff}, {1, 0x0000ff}, {2, 0x00ff0000}, {3, 0x00ffffff}};

const PROGMEM char TITLE_RAINBOW[] = "rainbow";
const PROGMEM char TITLE_TRICOLOR[] = "tricolor";
const Gamut gamuts_a[]{
    {{FAR, TITLE_RAINBOW}, DESCRIBE_ARRAY(NEAR, KEYS_RAINBOW)},
    {{FAR, TITLE_TRICOLOR}, DESCRIBE_ARRAY(NEAR, KEYS_TRICOLOR)}};
ArrayDescriptor<Gamut> gamuts DESCRIBE_ARRAY(NEAR, gamuts_a);
const Gamut *gamut = &gamuts_a[0];
Menu *gamutsMenu_a = nullptr;

uint16_t numLeds = NUMLEDS;
byte luminance = 255;
uint16_t period = 4000;

bool enabled = true;
float factor;
float speed;

void calcFactor(void)
{
  factor = (float)numLeds / gamut->keys[gamut->keys.len - 1].ledIndex;
}
void calcSpeed(void)
{
  speed = (float)numLeds / period;
}

void setup()
{
  Serial.begin(9600);
#ifdef STRIP_ENABLED
  strip.clear();
  strip.show(); // вывод изменений на ленту
  delay(1);     // между вызовами show должна быть пауза минимум 40 мкс !!!!
#endif
#ifdef DEBUG
  Serial.print(F("Mode="));
  Serial.print(mode);
  Serial.print(F(", Gamut="));
  Serial.println(gamut->name.getPrintable());
#endif
  calcFactor();
  calcSpeed();
}

void loop()
{
  menuNavigator.processMenu();

#ifdef DEBUG
  static bool logged = false;
  if (!logged)
  {
    Serial.print(" keyCount:");
    Serial.print(gamut->keys.len);
    Serial.print(" factor:");
    Serial.println(factor);
    logged = true;
  }
#endif

  if (!enabled)
    return;

#ifdef STRIP_ENABLED
  float ref = (float)numLeds - speed * (millis() % period);

#ifdef DEBUG
  Serial.println();
  Serial.print(F("ref="));
  Serial.println(ref);
#endif

  for (uint16_t i = 0; i < numLeds; i++)
  {
    float idx = ref - floor(ref) + (uint16_t)(floor(ref) + i) % numLeds;

    uint16_t keyRange;
    for (keyRange = 0; keyRange < gamut->keys.len - 1; keyRange++)
    {
      if (factor * gamut->keys[keyRange + 1].ledIndex > idx)
        break;
    }

    mData val;
    switch (mode)
    {
    case SMOOTH:
      val = getBlend(idx - factor * gamut->keys[keyRange].ledIndex,
                     factor * (gamut->keys[keyRange + 1].ledIndex - gamut->keys[keyRange].ledIndex),
                     mHEX(gamut->keys[keyRange].rgb),
                     mHEX(gamut->keys[keyRange + 1].rgb));
      break;
    case STEPPED:
    {
      float phase = idx - factor * gamut->keys[keyRange + 1].ledIndex + 1;
      if (phase < 0)
      {
        phase = 0; // blend last one-LED step only
      }

      val = getBlend(phase,
                     1, // one-LED step transition
                     mHEX(gamut->keys[keyRange].rgb),
                     mHEX(gamut->keys[keyRange + 1].rgb));
      break;
    }

    default: // random
      val = mWhite;
      break;
    }

#ifdef DEBUG
    Serial.print(((uint32_t)val.r << 16) + ((uint32_t)val.g << 8) + val.b, HEX);
    Serial.print(F(" "));
#endif
    strip.set(i, /**val/*/ getFade(val, 255 - luminance) /**/);
  }

#ifdef DEBUG
  Serial.println();
#endif

  strip.show();
  delay(1);
#endif
}

const byte printCurrentGamut(const Menu *selection)
{
  Serial.print(F("selected gamut: "));
  Serial.println(gamut->name.getPrintable());
  return 0;
}
const byte printCurrentLuminance(const Menu *selection)
{
  Serial.print(F("selected luminance: "));
  Serial.println(luminance);
  return 0;
}
const byte printCurrentMode(const Menu *selection)
{
  Serial.print(F("selected mode: "));
  Serial.print(mode + 1);
  Serial.print(F(" - "));
  Serial.println(MODES_AD[mode].title.getPrintable());
  return 0;
}

const byte getOptionCase(const Menu *selection)
{
  return mode == RANDOM ? 1 : 0;
}

ArrayDescriptor<Menu> handleToggleLights(const Menu *selection)
{
  enabled = !enabled;
#ifdef STRIP_ENABLED
  strip.clear();
  strip.show(); // вывод изменений на ленту
#endif
  Serial.print(F("enabled = "));
  Serial.println(enabled);
  return {};
}

void setMode(Mode selection)
{
  mode = selection;
  printCurrentMode(nullptr);
}

ArrayDescriptor<Menu> handleModeSelection(Mode mode)
{
  setMode(mode);
  return CMD_RESET_MENU;
}
ArrayDescriptor<Menu> handleModeSelectionSmooth(const Menu *selection)
{
  return handleModeSelection(SMOOTH);
}
ArrayDescriptor<Menu> handleModeSelectionStep(const Menu *selection)
{
  return handleModeSelection(STEPPED);
}
ArrayDescriptor<Menu> handleModeSelectionRandom(const Menu *selection)
{
  return handleModeSelection(RANDOM);
}

ArrayDescriptor<Menu> handleNumOfLedsSelection(const Menu *selection)
{
  Serial.print(F("current LED count: "));
  Serial.println(numLeds);

  // ToDo: calcSpeed() & calcFactor()
}
ArrayDescriptor<Menu> handlePeriodSelection(const Menu *selection)
{
  Serial.print(F("current loop period: "));
  Serial.println(period);
  
  // ToDo: calcSpeed()
}
ArrayDescriptor<Menu> handleNumericSelection(const Menu *selection)
{
  return {};
}
const byte handleLuminance(const Menu *selection)
{
  switch (selection->key)
  {
  case '[':
    luminance--;
    break;
  case ']':
    luminance++;
    break;
  case '{':
    luminance -= 10;
    break;
  case '}':
    luminance += 10;
    break;
  }
  return 0;
}
const byte processGamutEntry(const Menu *selection)
{
  gamut = &gamuts_a[selection->key - '1'];
  calcFactor();
  return 0;
}
ArrayDescriptor<Menu> handleGamutEntry(const Menu *selection)
{
  gamutsMenu_a = (Menu *)realloc(gamutsMenu_a, sizeof(Menu[gamuts.len]));
  // Menu *gamutsMenu_a = new Menu[gamuts.len]; // FixMe: where is it deleted?
  for (byte i = 0; i < gamuts.len; i++)
  {
    gamutsMenu_a[i] = {(char)('1' + i),
                       gamuts[i].name,
                       &processGamutEntry,
                       POP_CASES,
                       nullptr};
  }

  /**
    <m>. <presets>
      <preset values>
      Enter. use selected preset
      e. edit preset
        @edit
    'n'. new
      #edit:
      > name:
      > sequence (LED1:RRGGBB1;LED2:RRGGBB2;...;LEDn:RRGGBBn):
  */
  return {gamuts.len, NEAR, gamutsMenu_a};
}
