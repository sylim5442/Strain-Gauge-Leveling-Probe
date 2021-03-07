# Still under construction 
## Message Me If you need info ASAP

# Strain-Gauge-Leveling-Probe
Using strain gauge, HX711 ADC and Arduino/Digispark for automatic bed leveling of the 3D printer, with improved trigger algorithm.  

I was amazed by the [Creality CR6-SE ABL](https://www.kickstarter.com/projects/3dprintmill/creality-cr-6-se-leveling-free-diy-3d-printer-kit) which use strain gauge to detect load/pressure on nozzle.
This method of probing has no probe-nozzle offset and shouldn't be affected by temperature of the bed.
Strain gauge probing is used in CNC touch probe before 3D printer was invented. 

CNC Touch Probe
![cnc_touch_probe](https://user-images.githubusercontent.com/75633795/110230715-c0df4880-7f4d-11eb-8d78-624114637688.jpg)

Some maker applied this concept on their design in the past
* Palmerr23 - https://www.instructables.com/Reprap-Load-Cell-Z-Probe/
* David Pilling - https://www.davidpilling.com/wiki/index.php/Zprobe#a12
* IvDm - https://github.com/IvDm/Z-probe-on-smd-resistors-2512

While I made my own strain gauge/load cell contraption, I refered to [IvDm's Arduino sketch](https://github.com/IvDm/Z-probe-on-smd-resistors-2512/blob/master/strain_gauge_switch_ATtiny85_V_1.1.ino) a for trigger algorithm. But soon, I realised couple of weakness with the sketch. 
* Drifting
  * The strain gauge resistance rises when temperature increase, this will change the strain gauge readings and is known as drifting. Normally strain gauge is not affected much by the hotend or bed temperature since they are isolated from them & [half bridge/full bridge wheatstone](https://www.ni.com/en-my/innovations/white-papers/06/how-is-temperature-affecting-your-strain-measurement-accuracy-.html) compensates the effect. 
However, when the strain cell is constantly getting excited for readings, it will emit heat significant enough to cause drifting. If the reading is done with long interval and cut off power between interval (such as [power_down](https://github.com/bogde/HX711/blob/master/keywords.txt) in bogde's HX711 library) then this effect is very minimal.

  * But for our application the probe will read value constantly at 80Hz, drifting will cause the probe to false trigger, giving me failed homing and mesh.  

* Crushing bed
  * In some cases drifting eventually make the probe not responding to bed touch, the hotend will crush onto build plate and damaging the nozzle, heatbreak, z rod and motor. 

## My Improvement
* Trigger algorithm

   *load cell reading + change in gradients*   
   This sketch will also read change in gradient of readings, denoted as de.   
   > if de >= threshold_de, probe is touching the bed   
   >  if de < threshold_de, probe not touching the bed or drifting occurs

* Panic mode

   When a threshold value is reached, Panic mode forces trigger the output to HIGH, preventing damage to the printer.

* Blinks

   Use led to indicate the working state of the probe
  * Start working (finished setup loop)
  
        digitalWrite(LED_out, HIGH);
        delay(50);
        digitalWrite(LED_out, LOW);
        delay(50);
          for (int j = 0; j <= 5; j++)
          {
            digitalWrite(LED_out, HIGH);
            delay(50);
            digitalWrite(LED_out, LOW);
            delay(50);
           }
  * Trigger

           digitalWrite(probe_out, HIGH);
           digitalWrite(LED_out, HIGH);
           delay(100);
           digitalWrite(probe_out, LOW);
           digitalWrite(LED_out, LOW);
  * recalibrating

          for (int k = 0; k < 3; k++)
          {
            digitalWrite(LED_out, HIGH);
            delay(20);
            digitalWrite(LED_out, LOW);
            delay(50);
          }
## Working principle
To be added

## Wiring
Typically HX711 board is shipped with 10Hz sampling rate, some modification is needed to use 80Hz sampling rate.
In my case, it's resoldering the 0ohm resistor on XFW-HX711 board. 

Wiring for Digispark board.

   |Endstop pins | Digispark | HX711 | Remark
   |--- | --------- | ----- | --
   |5V/V |  5V | Vcc | 
   |GND/G | GND | GND |  
   | | 0 | DT/DOUT | serial data
   | | 1 |  | LED pin
   | | 2 | SCK | serial clock
   | | 3 |  | *used in USB comm*
   | IN/S | 4 |  | *used in USB comm*
   | | 5 |  | *low voltage, don't use*
   
Check [Digistump documentation](http://digistump.com/wiki/digispark/quickref) for more info
  
If you are using other Arduino boards such as Nano or Pro Mini, or using atmel chip for custom pcb, you will need to decide the wiring on your own.

## Marlin Configuration
To be added

## Issue
* Spongy bed & gantry

   the Ender 3 I use has only one Z-rod on the left side, the right side is a little bit spongy  
   this causes a dampening effect which lower the de value, therefore probing fails occasionally at the right side of bed during ABL.  
   I suspect this will also happen if a spongy bed with weak spring is used  
   Eventually this's solved by using lower threshold_de
   
* Plastic on the nozzle
   
   **picture**

   Plastic oozing on the nozzle also lower the de value.  
   Marlin and slicer is configured to perform automatic nozzle brushing before homing and probing. 

   **Picture**
