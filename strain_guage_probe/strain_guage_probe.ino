/**
   Leveling system with load cell, HX711 and Digispark
   by ShaoYang Lim a.k.a. Yonggor
   last update March 7, 2021

   This sketch utilise HX711 library for Arduino by Bodge
   https://github.com/bogde/HX711

   Also check out:
   David Pilling's Z probe
   https://www.davidpilling.com/wiki/index.php/Zprobe
   Z probe using SMD resistor 2512
   https://github.com/IvDm/Z-probe-on-smd-resistors-2512
**/

/*
    Attention:
    this sketch sets endstop pin high when the probe detects a touchdown,
    which is the opposite of many printers including Creality Ender 3 I am working with.
    Reconfiguration of printer firmware must be done for Marlin, RRF, Klipper or other.

    this sketch decides whether to trigger the probe_out pin based on two factors:
    1. load cell readings : val0
    2. peaking of reading : de
    de is change of gradient of load cell readings, which indicates the sudden change of readings,
    instead of drifting (consistant increasing readings)
*/
#include "HX711.h"

//  uncomment the board use
#define Digispark
//#define Arduino_Nano

//  choose working mode
//  only uncomment one mode
//  use mode0 for regular use
#define mode0
//#define mode1
//#define mode2

/*
    HX711 circuit wiring
   If using Digispark Board, pls follow the pin here to avoid trouble.
   If using Arduno Nano, you can change pin numbers to your own liking.
*/
#ifdef Digispark
#define LOADCELL_DOUT_PIN 0
#define LOADCELL_SCK_PIN 2
#define probe_out 4 //Pin to send printer
#define LED_out 1
#endif

#ifdef Arduino_Nano
#define LOADCELL_DOUT_PIN 2
#define LOADCELL_SCK_PIN 3
#define probe_out 5
#define LED_out 4
#endif

// variables
long val0 = 0; //latest reading
long val1 = 0; //latest-1 reading
long val2 = 0; //latest-2 reading
long de = 0;   //
//  define threshold values
long threshold_Val = 50;
long const threshold_gap = threshold_Val; // value to add to threshold_Val in each recalibration of probe trigger
int const threshold_De = 5;
#define panic_val 3000 //  force HIGH output if val0 exceed this value, disregard de value.
//  The probe recalibrates its threshold value after each tigger.
//  Must avoid second probing before recalibration is finished.
//  delay_threshold defines time before recalibrate the trigger threshold, allow the nozzle to lift out of bed
#define delay_threshold 700
//  enable serial reporting of probe value, doesn't work with Digispark
//#define EN_serial
int i = 0;

HX711 scale;

void setup()
{

#ifdef EN_serial
  Serial.begin(57600);
#endif

  pinMode(probe_out, OUTPUT);
  pinMode(LED_out, OUTPUT);
  digitalWrite(probe_out, LOW);
  digitalWrite(LED_out, LOW);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  //check if HX711 is working properly, blink rapidly if not.
  if (!scale.wait_ready_retry(10))
  {
    while (1)
    {
      digitalWrite(LED_out, HIGH);
      delay(50);
      digitalWrite(LED_out, LOW);
      delay(50);
    }
  }

  scale.set_scale(570.f); // hx711 calibration value, not important in our probe
  delay(2000);            // pre-warm up strain gauge, reduce drifting
  scale.tare();           // reset the scale to 0

  // "End of setup loop" indicator
  // indicate the probe is ready for probing.
  digitalWrite(LED_out, HIGH);
  delay(500);
  digitalWrite(LED_out, LOW);
  delay(200);
  for (int j = 0; j <= 5; j++)
  {
    digitalWrite(LED_out, HIGH);
    delay(50);
    digitalWrite(LED_out, LOW);
    delay(50);
  }
}

void loop()
{
  val2 = val1;
  val1 = val0;
  getValue();
  getDe();

  /*
    connection checking,
    open or short connection load cell return highest/lowest value of reading
    2^24 = 16 777 216
      min reading: -8388607
      min reading:  8388607
    value of lowest end and highest end are treated as faulty connection
    LED blinks slowly
  */

  if (scale.read() <= -7000000 || scale.read() >= 7000000)
  {
    digitalWrite(LED_out, HIGH);
    delay(500);
    digitalWrite(LED_out, LOW);
    delay(750);
  }

  /*
   * ************** PANIC MODE ***********************************
      If the probe is not triggered when the nozzle is already pressing the bed
      blast HIGH when val0 >= panic_val to prevent damage
      set val0 to a reasonable value, which drifting couldn't reach normally
      e.g.: 100*threshlod_Val = 5000
     *********************************************************
  */
  else if (val0 >= panic_val)
  {
    digitalWrite(probe_out, HIGH);
    digitalWrite(LED_out, HIGH);
    delay(500);
    digitalWrite(probe_out, LOW);
    digitalWrite(LED_out, LOW);
    delay(500);
    recalibrate(3);
  }

  /*
   * ************** MODE 0 ***********************************
     regular working mode impliments pressure and speed of pressure
     rising to detect homing
     *********************************************************
  */
#ifdef mode0

  else if (val0 >= threshold_Val && val0 < panic_val)
  {
    // sucessful trigger
    if (de >= threshold_De)
    {
      digitalWrite(probe_out, HIGH);
      digitalWrite(LED_out, HIGH);
      delay(100);
      digitalWrite(probe_out, LOW);
      digitalWrite(LED_out, LOW);
      delay(delay_threshold);
      recalibrate(3);
    }
    
    //  unsucessful trigger (de smaller than threshlod)
    //  recalibrate trigger threshlod after certain amount of time (i value)
    else if (de < threshold_De)
    {
      i++;
      if (i >= 200)
      {
        recalibrate(5);
        i = 0;
      }
    }
  }

  else
  {
    digitalWrite(probe_out, LOW);
    digitalWrite(LED_out, LOW);
  }
#endif

  /*
   * ********** MODE 1 **********************************************
     Output HIGH when threshold_Val is reached
     useful for calibrating threshold_Val
     not suitable for actual probing because drifting cause
     unexpected trigger
     ****************************************************************
  */
#ifdef mode1
  if (val0 >= threshold_Val)
  {
    digitalWrite(probe_out, HIGH);
    digitalWrite(LED_out, HIGH);
    delay(200);
    digitalWrite(probe_out, LOW);
    digitalWrite(LED_out, LOW);
    delay(delay_threshold);
    recalibrate(3);
  }

  else
  {
    digitalWrite(probe_out, LOW);
    digitalWrite(LED_out, LOW);
  }
#endif

  /*
   * ************ MODE 2 *************************************
     Output HIGH when threshold_De is reached
     useful for calibrating threshold_De
     not suitable for actual probing because shaking/vibration causes
     unexpected trigger
     *********************************************************
  */
#ifdef mode2
  if (de >= threshold_De)
  {
    digitalWrite(probe_out, HIGH);
    digitalWrite(LED_out, HIGH);
    delay(200);
    digitalWrite(probe_out, LOW);
    digitalWrite(LED_out, LOW);
  }

  else
  {
    digitalWrite(probe_out, LOW);
    digitalWrite(LED_out, LOW);
  }
#endif

  //serial communication
#ifdef EN_serial
  Serial.print("reading: ");
  Serial.println(scale.read());
  Serial.print("Value: ");
  Serial.println(val0);
  Serial.print("De: ");
  Serial.println(de);
#endif
}

int getValue()
{
  val0 = scale.get_units(1);
}

int getDe()
{
  de = val0 + val2 - (2 * val1);
  abs(de);
}

int recalibrate(int num_sample)
{
  threshold_Val = scale.get_units(num_sample) + threshold_gap;
  for (int k = 0; k < 3; k++)
  {
    digitalWrite(LED_out, HIGH);
    delay(20);
    digitalWrite(LED_out, LOW);
    delay(50);
  }
}
