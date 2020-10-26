#define CFG_STEPUP_BYTE 0x00
#define CFG_STEPUP_OFF  0x00
#define CFG_STEPUP_ON   0x01

#define CFG_BAT_LOW_BYTE 0x01
#define CFG_BAT_CRITICAL_BYTE 0x02

// define device configuration bytes
#define DEVICE_CONFIG CFG_STEPUP_OFF,22,19

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <ThreeState.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED1_PIN 4
#define LED2_PIN 5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

#define SENS1_PIN 14
#define SENS2_PIN 15
#define SABOTAGE_PIN 16

// activate additional open detection by using a third sensor pins
// #define SENS3_PIN 16
// #define SABOTAGE_PIN 0

// number of available peers per channel
#define PEERS_PER_CHANNEL 10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x09,0x56,0x34},       // Device ID
    "papa222111",           // Device Serial
    {0x00,0xC3},            // Device Model
    0x22,                   // Firmware Version
    as::DeviceType::ThreeStateSensor, // Device Type
    {0x01,0x00}             // Info Bytes
};

typedef NoBattery BattSensType;

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef DualStatusLed<LED2_PIN,LED1_PIN> LedType;
typedef AskSin<LedType,BatterySensor,RadioType> BaseHal;
class Hal : public BaseHal {
public:
  void init (const HMID& id) {
    BaseHal::init(id);
  }
} hal;

DEFREGISTER(Reg0,DREG_INTKEY,DREG_CYCLICINFOMSG,MASTERID_REGS,DREG_TRANSMITTRYMAX,DREG_SABOTAGEMSG)
class RHSList0 : public RegList0<Reg0> {
public:
  RHSList0(uint16_t addr) : RegList0<Reg0>(addr) {}
  void defaults () {
    clear();
    cycleInfoMsg(true);
    transmitDevTryMax(6);
    sabotageMsg(true);
  }
};

DEFREGISTER(Reg1,CREG_AES_ACTIVE,CREG_MSGFORPOS,CREG_EVENTDELAYTIME,CREG_LEDONTIME,CREG_TRANSMITTRYMAX)
class RHSList1 : public RegList1<Reg1> {
public:
  RHSList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
  void defaults () {
    clear();
    msgForPosA(1); // CLOSED
    msgForPosB(2); // OPEN
    msgForPosC(3); // TILTED
    // aesActive(false);
    // eventDelaytime(0);
    ledOntime(100);
    transmitTryMax(6);
  }
};


typedef ThreeStateChannel<Hal,RHSList0,RHSList1,DefList4,PEERS_PER_CHANNEL> ChannelType;

class RHSType : public ThreeStateDevice<Hal,ChannelType,1,RHSList0> {
public:
  typedef ThreeStateDevice<Hal,ChannelType,1,RHSList0> TSDevice;
  RHSType(const DeviceInfo& info,uint16_t addr) : TSDevice(info,addr) {}
  virtual ~RHSType () {}

 // virtual void configChanged () {
 //   TSDevice::configChanged();
 //   // set battery low/critical values
 //   battery().low(getConfigByte(CFG_BAT_LOW_BYTE));
 //   battery().critical(getConfigByte(CFG_BAT_CRITICAL_BYTE));
 //    #ifndef BATTERY_IRQ
 //    // set the battery mode
 //    battery().meter().sensor().mode(getConfigByte(CFG_STEPUP_BYTE));
 //    #endif
 //  }
};

RHSType sdev(devinfo,0x20);
ConfigButton<RHSType> cfgBtn(sdev);

const uint8_t posmap[4] = {Position::State::PosB,Position::State::PosC,Position::State::PosA,Position::State::PosB};

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  sdev.channel(1).init(SENS1_PIN,SENS2_PIN,SABOTAGE_PIN,posmap);
  sdev.initDone();
    
  // measure battery every 1h
  // hal.battery.init(seconds2ticks(60UL*60),sysclock);
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    // if nothing to do - go sleep
    hal.sleep<>();
  }
}

