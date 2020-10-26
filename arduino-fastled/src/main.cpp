#include <arduino.h>
#include <FastLED.h>

// LED STRIPE PIN
#define DATA_PIN 6
#define NUM_LEDS 49
#define MAX_BRIGHTNESS 255 // 0-255

int randColor;

CRGB leds[NUM_LEDS];
CRGB incomes[NUM_LEDS];

#define EI_NOTEXTERNAL
//#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Dimmer.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED_PIN 4

// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

// number of available peers per channel
#define PEERS_PER_CHANNEL 4

// all library classes are placed in the namespace 'as'
using namespace as;

const struct DeviceInfo PROGMEM devinfo = {
    {0x11,0x12,0x22},       // Device ID
    "DIMX111111",           // Device Serial
//    {0x00,0x67},            // Device Model
    {0x00,0xF4},            // Device Model
    0x25,                   // Firmware Version
    as::DeviceType::Dimmer, // Device Type
    {0x01,0x00}             // Info Bytes
};


// ---

template <class HalType,class DimmerType>
class I2CDimmerControl {
private:
  DimmerType& dimmer;
  uint8_t physical[DimmerType::channelCount/DimmerType::virtualCount];
  uint8_t factor[DimmerType::channelCount/DimmerType::virtualCount];
  uint8_t counter;
  uint8_t color;
  uint8_t overloadcounter;

  class ChannelCombiner : public Alarm {
    I2CDimmerControl<HalType,DimmerType>& control;
  public:
    ChannelCombiner (I2CDimmerControl<HalType,DimmerType>& d) : Alarm(0), control(d) {}
    virtual ~ChannelCombiner () {}
    virtual void trigger (AlarmClock& clock) {
      control.updatePhysical();
      set(millis2ticks(10));
      clock.add(*this);
    }
  } cb;

public:
  I2CDimmerControl (DimmerType& dim) : dimmer(dim), cb(*this) {}
  ~I2CDimmerControl () {}

  uint8_t channelCount  () { return DimmerType::channelCount; }
  uint8_t virtualCount  () { return DimmerType::virtualCount; }
  uint8_t physicalCount () { return DimmerType::channelCount/DimmerType::virtualCount; }

  void firstinit () {
    for( uint8_t i=1; i<=channelCount(); ++i ) {
      if( i <= physicalCount() ){
        dimmer.dimmerChannel(i).getList1().logicCombination(LOGIC_OR);
      }
      else {
        dimmer.dimmerChannel(i).getList1().logicCombination(LOGIC_INACTIVE);
      }
    }

  }

  bool init (HalType& hal) {
    bool first = dimmer.init(hal);
    if( first == true ) {
      firstinit();
    }
//    Wire.begin( );
//    set(0,0);
    for( uint8_t i=0; i<physicalCount(); ++i ) {
        physical[i] = 0;
        factor[i] = 200; // 100%
    }
    initChannels();
    cb.trigger(sysclock);
    return first;
  }

  void initChannels () {
    for( uint8_t i=1; i<=physicalCount(); ++i ) {
      for( uint8_t j=i; j<=channelCount(); j+=physicalCount() ) {
        dimmer.dimmerChannel(j).setPhysical(physical[i-1]);
        bool powerup = dimmer.dimmerChannel(j).getList1().powerUpAction();
        dimmer.dimmerChannel(j).setLevel(powerup == true ? 200 : 0,0,0xffff);
      }
    }
  }

  void hsvToRgb(double h, double s, double v, byte rgb[]) {
    double r, g, b;

    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    rgb[0] = r * 255;
    rgb[1] = g * 255;
    rgb[2] = b * 255;
  }

  void set (uint8_t value, uint8_t color) {
    DPRINT("Value "); DDEC(value); DPRINT(" -- "); DDEC(color);DPRINT("\n");
    byte rgb[3];
    if (value < 1) {
//      leds.off();
      LEDS.showColor(CRGB(0, 0,0));
    } else if (color < 190) {
      hsvToRgb(color / 200.0, 1,value / 200.0, rgb);
      DPRINT("R: "); DDEC(rgb[0]); DPRINT("G: "); DDEC(rgb[1]); DPRINT("B: "); DDEC(rgb[2]); DPRINT("\n");
      LEDS.showColor(CRGB(rgb[0], rgb[1], rgb[2]));
//      leds.set_brightness(4, rgb[0]); /* red PIN = 11 */
//      leds.set_brightness(5, rgb[1]); /* green PIN = 12 */
//      leds.set_brightness(6, rgb[2]); /* blue  = PIN 13 */
//      leds.set_brightness(3, 0); /* ww PIN = 9 */
//      leds.set_brightness(2, 0); /* cw PIN = 8 */
    } else if (color < 195) {
//      leds.set_brightness(4, 100); /* red PIN = 11 */
//      leds.set_brightness(5, 100); /* green PIN = 12 */
//      leds.set_brightness(6, 10); /* blue  = PIN 13 */ 
//      leds.set_brightness(3, value); /* ww PIN = 9 */
//      leds.set_brightness(2, 0); /* cw PIN = 8 */
    } else {
//      leds.set_brightness(4, 100); /* red PIN = 11 */
//      leds.set_brightness(5, 100); /* green PIN = 12 */
//      leds.set_brightness(6, 100); /* blue  = PIN 13 */ 
//      leds.set_brightness(3, 0); /* ww PIN = 9 */
//      leds.set_brightness(2, value); /* cw PIN = 8 */
    }
  }
  
  void updatePhysical () {
    // DPRINT("update phys ");
    // DPRINT("Pin ");DHEX(pin);DPRINT("  Val ");DHEXLN(calcPwm());
    uint8_t c = dimmer.dimmerChannel(2).status();
    for( uint8_t i=0; i<physicalCount(); ++i ) {
      uint8_t value = (uint8_t)combineChannels(i+1);
      value = (((uint16_t)factor[i] * (uint16_t)value) / 200);
      if( physical[i] != value || c != color) {
        // DPRINT("Ch: ");DDEC(i+1);DPRINT(" Phy: ");DDECLN(value);
        physical[i]  = value;
        color = c;
        set(physical[i], color);
      }
    }
  }

  uint16_t combineChannels (uint8_t start) {
    uint16_t value = 0;
    for( uint8_t i=start; i<=channelCount(); i+=physicalCount() ) {
      uint8_t level = dimmer.dimmerChannel(i).status();
      switch( dimmer.dimmerChannel(i).getList1().logicCombination() ) {
      default:
      case LOGIC_INACTIVE:
        break;
      case LOGIC_OR:
        value = value > level ? value : level;
        break;
      case LOGIC_AND:
        value = value < level ? value : level;
        break;
      case LOGIC_XOR:
        value = value==0 ? level : (level==0 ? value : 0);
        break;
      case LOGIC_NOR:
        value = 200 - (value > level ? value : level);
        break;
      case LOGIC_NAND:
        value = 200 - (value < level ? value : level);
        break;
      case LOGIC_ORINVERS:
        level = 200 - level;
        value = value > level ? value : level;
        break;
      case LOGIC_ANDINVERS:
        level = 200 - level;
        value = value < level ? value : level;
        break;
      case LOGIC_PLUS:
        value += level;
        if( value > 200 ) value = 200;
        break;
      case LOGIC_MINUS:
        if( level > value ) value = 0;
        else value -= level;
        break;
      case LOGIC_MUL:
        value = value * level / 200;
        break;
      case LOGIC_PLUSINVERS:
        level = 200 - level;
        value += level;
        if( value > 200 ) value = 200;
        break;
        break;
      case LOGIC_MINUSINVERS:
        level = 200 - level;
        if( level > value ) value = 0;
        else value -= level;
        break;
      case LOGIC_MULINVERS:
        level = 200 - level;
        value = value * level / 200;
        break;
      case LOGIC_INVERSPLUS:
        value += level;
        if( value > 200 ) value = 200;
        value = 200 - value;
        break;
      case LOGIC_INVERSMINUS:
        if( level > value ) value = 0;
        else value -= level;
        value = 200 - value;
        break;
      case LOGIC_INVERSMUL:
        value = value * level / 200;
        value = 200 - value;
        break;
      }
    }
    // DHEXLN(value);
    return value;
  }
   
  void setOverload (bool overload=false) {
      counter++;
      if ( overload ){
          overloadcounter++;
          
      }
      for( uint8_t i=1; i<=physicalCount(); ++i ) {
        typename DimmerType::DimmerChannelType& c = dimmer.dimmerChannel(i);
        if ( counter > 5 ){
            if((counter - overloadcounter) <= 2 ){
              factor[i-1] = 0;
              c.overload(true);
          }
          else{
             counter = 0;
             overloadcounter = 0;
          }
          
        }
        else if ( c.getoverload()) {
              c.overload(false);
              factor[i-1] = 200;
        }
     }
  }

  void setTemperature (uint16_t temp) {
    uint8_t t = temp/10;
    for( uint8_t i=1; i<=physicalCount(); ++i ) {
      typename DimmerType::DimmerChannelType& c = dimmer.dimmerChannel(i);
      if( c.getList1().overTempLevel() <= t ) {
        factor[i-1] = 0; // overtemp -> switch off
        c.overheat(true);
        c.reduced(false);
      }
      else if( c.getList1().reduceTempLevel() <= t ) {
        factor[i-1] = c.getList1().reduceLevel();
        c.overheat(false);
        c.reduced(true);
      }
      else {
        factor[i-1] = 200; // 100%
        c.overheat(false);
        c.reduced(false);
      }
    }
  }
};


// ---

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType,NoBattery,RadioType> HalType;
typedef DimmerChannel<HalType,PEERS_PER_CHANNEL> ChannelType;
typedef DimmerDevice<HalType,ChannelType,3,3> DimmerType;

HalType hal;
DimmerType sdev(devinfo,0x20);

I2CDimmerControl<HalType,DimmerType> control(sdev);
ConfigToggleButton<DimmerType> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  if( control.init(hal) ) {
    // first init - setup connection between config button and first channel
    sdev.channel(1).peer(cfgBtn.peer());
  }
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  sdev.initDone();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setTemperature( CRGB(255, 180, 150) );
  FastLED.setBrightness( MAX_BRIGHTNESS );
  LEDS.showColor(CRGB(150, 100, 100));
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    hal.activity.savePower<Idle<true> >(hal);
  }
}
