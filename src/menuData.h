#include "types.h"
#include <menu.h>

ArrayDescriptor<Menu> handleToggleLights(const void *);
ArrayDescriptor<Menu> handleModeSelectionSmooth(const void *);
ArrayDescriptor<Menu> handleModeSelectionStep(const void *);
ArrayDescriptor<Menu> handleModeSelectionRandom(const void *);
ArrayDescriptor<Menu> handleNumOfLedsSelection(const void *);
ArrayDescriptor<Menu> handlePeriodSelection(const void *);
ArrayDescriptor<Menu> handleNumericSelection(const void *); // ToDo: print current value first - depending on path
ArrayDescriptor<Menu> handleGamutEntry(const void *);
const byte printNumOfLeds(const Menu *);
const byte printPeriod(const Menu *);
const byte handleLuminance(const Menu *);
const byte printCurrentLuminance(const Menu *);
const byte printCurrentMode(const Menu *);
const byte printCurrentGamut(const Menu *);
const byte getOptionCase(const Menu *);

const PROGMEM char EMPTY_STRING[] = "";
// control commands
const PROGMEM Menu CMD_RESET_MENU_AR[]{{KEY_RESET_MENU}};
const ArrayDescriptor<Menu> CMD_RESET_MENU DESCRIBE_ARRAY(FAR, CMD_RESET_MENU_AR);

const PROGMEM Menu CMD_POP_MENU_AR[]{{KEY_POP_MENU}};
const ArrayDescriptor<Menu> CMD_POP_MENU DESCRIBE_ARRAY(FAR, CMD_POP_MENU_AR);

const PROGMEM Menu CMD_INPUT_AR[]{{KEY_INT_INPUT}};
const ArrayDescriptor<Menu> CMD_INPUT DESCRIBE_ARRAY(FAR, CMD_INPUT_AR);

// common menu cases
#define DESCRIBE_INPUT_HANDLER_AR(h) {{'\xd', {FAR, EMPTY_STRING}, nullptr, POP_CASES, &h}}

const PROGMEM ArrayDescriptor<Menu> EC_A[]{{}};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> EC_AA[]{DESCRIBE_ARRAY(FAR, EC_A)};
#define EMPTY_CASES DESCRIBE_ARRAY(FAR, EC_AA)

const PROGMEM ArrayDescriptor<Menu> POP_A[]{CMD_POP_MENU};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> POP_AA[]{DESCRIBE_ARRAY(FAR, POP_A)};
#define POP_CASES DESCRIBE_ARRAY(FAR, POP_AA)

const PROGMEM ArrayDescriptor<Menu> INPUT_A[]{CMD_INPUT};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> INPUT_AA[]{DESCRIBE_ARRAY(FAR, INPUT_A)};
#define INPUT_CASES DESCRIBE_ARRAY(FAR, INPUT_AA)

// Modes
const PROGMEM char TITLE_SMOOTH[] = "smooth";
const PROGMEM char TITLE_STEP[] = "step";
const PROGMEM char TITLE_RANDOM[] = "random";
const PROGMEM Menu MODES_AR[]{// align sequence with Mode enum for printCurrentMode
                              {KEY_AUTO, {FAR, TITLE_SMOOTH}, nullptr, EMPTY_CASES, &handleModeSelectionSmooth},
                              {KEY_AUTO, {FAR, TITLE_STEP}, nullptr, EMPTY_CASES, &handleModeSelectionStep},
                              {KEY_AUTO, {FAR, TITLE_RANDOM}, nullptr, EMPTY_CASES, &handleModeSelectionRandom}};
const ArrayDescriptor<Menu> MODES_AD DESCRIBE_ARRAY(FAR, MODES_AR);
const PROGMEM ArrayDescriptor<Menu> MODES_A[]{MODES_AD};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> MODES_AA[]{DESCRIBE_ARRAY(FAR, MODES_A)};

// Options
// O-Luminance
const PROGMEM char TITLE_LUM_STEP_DN[] = "+1";
const PROGMEM char TITLE_LUM_STEP_UP[] = "-1";
const PROGMEM char TITLE_LUM_JUMP_DN[] = "+10";
const PROGMEM char TITLE_LUM_JUMP_UP[] = "-10";
const PROGMEM char TITLE_LUM_VALUE[] = "exact value (0..255)";
const PROGMEM Menu LUMINANCE_CONTROLS_AR[]{
    {'[', {FAR, TITLE_LUM_STEP_DN}, &handleLuminance, POP_CASES, nullptr},
    {']', {FAR, TITLE_LUM_STEP_UP}, &handleLuminance, POP_CASES, nullptr},
    {'{', {FAR, TITLE_LUM_JUMP_DN}, &handleLuminance, POP_CASES, nullptr},
    {'}', {FAR, TITLE_LUM_JUMP_UP}, &handleLuminance, POP_CASES, nullptr},
    {'\xd', {FAR, TITLE_LUM_VALUE}, nullptr, INPUT_CASES, &handleNumericSelection}};
const PROGMEM ArrayDescriptor<Menu> LUMINANCE_CONTROLS_A[] DESCRIBE_ARRAY(FAR, LUMINANCE_CONTROLS_AR);
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> LUMINANCE_CONTROLS_AA[] DESCRIBE_ARRAY(FAR, LUMINANCE_CONTROLS_A);

//ToDo: define DESCRIBE_INPUT_HANDLER_AA(NAME, h) macro
// O-NumOfLeds
const PROGMEM Menu LEDS_INPUT_AR[] DESCRIBE_INPUT_HANDLER_AR(handleNumOfLedsSelection);
const PROGMEM ArrayDescriptor<Menu> LEDS_INPUT_A[] {CMD_INPUT, DESCRIBE_ARRAY(FAR, LEDS_INPUT_AR)};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> LEDS_INPUT_AA[]{DESCRIBE_ARRAY(FAR, LEDS_INPUT_A)};

// O-Period
const PROGMEM Menu PERIOD_INPUT_AR[] DESCRIBE_INPUT_HANDLER_AR(handlePeriodSelection);
const PROGMEM ArrayDescriptor<Menu> PERIOD_INPUT_A[] {CMD_INPUT, DESCRIBE_ARRAY(FAR, PERIOD_INPUT_AR)};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> PERIOD_INPUT_AA[]{DESCRIBE_ARRAY(FAR, PERIOD_INPUT_A)};

const PROGMEM char TITLE_NUM_OF_LEDS[] = "number of LEDs";
const PROGMEM char TITLE_LUMINANCE[] = "luminance";
const PROGMEM char TITLE_PERIOD[] = "period (milliseconds)";
const PROGMEM Menu OPTIONS[]{
    {KEY_AUTO, {FAR, TITLE_NUM_OF_LEDS}, &printNumOfLeds, DESCRIBE_ARRAY(FAR, LEDS_INPUT_AA), nullptr},
    {KEY_AUTO, {FAR, TITLE_LUMINANCE}, &printCurrentLuminance, DESCRIBE_ARRAY(FAR, LUMINANCE_CONTROLS_AA), nullptr},
    {KEY_AUTO, {FAR, TITLE_PERIOD}, &printPeriod, DESCRIBE_ARRAY(FAR, PERIOD_INPUT_AA), nullptr}};
#define OPTIONS_COMMON DESCRIBE_ARRAY(FAR, OPTIONS)

const PROGMEM char TITLE_GAMUT[] = "gamut";
const PROGMEM Menu OPTIONS_CASE0_MENUS[]{{KEY_AUTO, {FAR, TITLE_GAMUT}, &printCurrentGamut, EMPTY_CASES, &handleGamutEntry}};
const PROGMEM ArrayDescriptor<Menu> OPTIONS_CASE0_AR[]{
    OPTIONS_COMMON,
    DESCRIBE_ARRAY(FAR, OPTIONS_CASE0_MENUS)};

const PROGMEM char TITLE_GAIN_TIME[] = "gain duration (milliseconds)";
const PROGMEM char TITLE_DIM_TIME[] = "dimming duration (milliseconds)";
const PROGMEM Menu OPTIONS_CASE1_MENUS[]{
    {KEY_AUTO, {FAR, TITLE_GAIN_TIME}, nullptr, INPUT_CASES, &handleNumericSelection},
    {KEY_AUTO, {FAR, TITLE_DIM_TIME}, nullptr, INPUT_CASES, &handleNumericSelection},
};
const PROGMEM ArrayDescriptor<Menu> OPTIONS_CASE1_AR[]{
    OPTIONS_COMMON,
    DESCRIBE_ARRAY(FAR, OPTIONS_CASE1_MENUS)};

const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> OPTIONS_CASES_AR[]{DESCRIBE_ARRAY(FAR, OPTIONS_CASE0_AR), DESCRIBE_ARRAY(FAR, OPTIONS_CASE1_AR)};

// Main Menu
const PROGMEM char TITLE_TOGGLE_LIGHTS[] = "toggle lights";
const PROGMEM char TITLE_MODE[] = "mode";
const PROGMEM char TITLE_OPTIONS[] = "options";
const PROGMEM Menu MENU_AR[]{
    {' ', {FAR, TITLE_TOGGLE_LIGHTS}, nullptr, POP_CASES, &handleToggleLights},
    {KEY_AUTO, {FAR, TITLE_MODE}, &printCurrentMode, DESCRIBE_ARRAY(FAR, MODES_AA), nullptr},
    {KEY_AUTO, {FAR, TITLE_OPTIONS}, &getOptionCase, DESCRIBE_ARRAY(FAR, OPTIONS_CASES_AR), nullptr}};
const PROGMEM ArrayDescriptor<Menu> MENU_A[]{DESCRIBE_ARRAY(FAR, MENU_AR)};
const PROGMEM ArrayDescriptor<ArrayDescriptor<Menu>> MENU_AA[]{DESCRIBE_ARRAY(FAR, MENU_A)};

const PROGMEM Menu menu{'m', {}, nullptr, DESCRIBE_ARRAY(FAR, MENU_AA), nullptr};