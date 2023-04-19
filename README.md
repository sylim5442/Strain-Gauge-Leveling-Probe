# Strain-Gauge-Leveling-Probe
Using strain gauge, HX711 ADC and Arduino/Digispark for homing and automatic bed leveling (ABL) of 3D printer, with improved trigger algorithm.  


<img src="https://user-images.githubusercontent.com/75633795/110341132-fc206b00-8064-11eb-979c-ef66c7f1e925.jpg" width="50%" height="50%"> 

I was amazed by the [Creality CR6-SE ABL](https://www.kickstarter.com/projects/3dprintmill/creality-cr-6-se-leveling-free-diy-3d-printer-kit) which use strain gauge to detect load/pressure on nozzle.
This method of probing has no probe-nozzle offset and shouldn't be affected by temperature of the bed.
Strain gauge probing is used in CNC touch probe before 3D printer was invented. 

CNC Touch Probe

<img src="https://user-images.githubusercontent.com/75633795/110230715-c0df4880-7f4d-11eb-8d78-624114637688.jpg" width="50%" height="50%">

Some makers applied this concept on their design in the past
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
## Demonstration
Youtube! : https://youtu.be/V8OPNfr5NjQ
[![thumbnail](https://user-images.githubusercontent.com/75633795/110341381-3b4ebc00-8065-11eb-9017-79206bed4013.jpg)](https://youtu.be/V8OPNfr5NjQ)

## Working principle
A load cell is placed on the hotend heatsink mount. It picks up tiny warp of the mount is read by HX711 24bits ADC. The Arduino (Digispark) compared the value and change-in-value to threshold values to determine if it should fire signal to printer mainboard. 

In my experience, drifting of readings occurs after the probe is powered for an amount of time. The reading climbs and hit threshold from time to time. I used change-in-value (denoted as de) as second input to see if the reading shoots suddenly (when nozzle touch the bed) or it's just rising slowly. If only readling value hits threshold but change-in-value is small, the probe will not trigger and it will recalibrate threshold after a short amount of time.
After each successful trigger, the probe wait a short amount of time for the nozzle to lift back in the air and it recalibrates/tares itself before next probe. 
To prevent crashing into the bed if the probe is not firing, a panic value is set to trigger the probe in all instance.

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

![wiring](https://user-images.githubusercontent.com/75633795/110344269-46efb200-8068-11eb-8251-2c358f1481de.jpg)

![DSC03501](https://user-images.githubusercontent.com/75633795/110344403-6686da80-8068-11eb-885a-7035a17b84f8.jpg)

## Marlin Configuration
Marlin firmware needed to be adjusted for the probe. Version of marlin I use is 2.0.X bugfix but name of some configurations might change from time to time. 

Define the probe used as NOOZLE_AS_PROBE
![Screenshot 2021-03-08 025822](https://user-images.githubusercontent.com/75633795/110344919-f9c01000-8068-11eb-8dec-f131c877f0fb.png)

![Screenshot 2021-03-08 025954](https://user-images.githubusercontent.com/75633795/110345176-43a8f600-8069-11eb-8c91-02b6d9f12edd.png)

![Screenshot 2021-03-08 030037](https://user-images.githubusercontent.com/75633795/110345200-4a376d80-8069-11eb-8b11-438b6f3a7b3a.png)
The pin number varies for all kind of boards. Use the old z-min pin just to be safe. 

![Screenshot 2021-03-08 030933](https://user-images.githubusercontent.com/75633795/110345509-9c788e80-8069-11eb-899f-6fc289b33000.png)
this part define whether the endstop/probe is active high or active low. Since our probe sends 5v when triggered and 0v when standby, we define ENDSTOPPULLDOWN_ZMIN. 

![Screenshot 2021-03-08 030236](https://user-images.githubusercontent.com/75633795/110346148-3cceb300-806a-11eb-954a-82b23d06804c.png)
make the probe probes twice rather than once (one quick probing and one slower)

the rest are behavior of the probing action, need to experiment on your own.
![Screenshot 2021-03-08 030200](https://user-images.githubusercontent.com/75633795/110346344-6a1b6100-806a-11eb-884b-90946e375427.png)

![Screenshot 2021-03-08 030327](https://user-images.githubusercontent.com/75633795/110346368-6ee01500-806a-11eb-8c33-f93189ac4516.png)

![Screenshot 2021-03-08 031201](https://user-images.githubusercontent.com/75633795/110346404-79021380-806a-11eb-9994-8b3978275ead.png)

## Accuracy and repeatability
I attached the M48 repeatability test and it scored as good as 0.005 deviation. Please note that is after a lot of trials and errors before I can really get it to work properly. 

Since the max sampling rate of HX711 is 80Hz, we can calculate the theoritical resolution of the probe like this:

   Sampling rate: 80Hz
   Homing feedrate: 6mm/s
   Z porbe speed fast: (homing feedrate)/2
   Z probe speed fast: (Z porbe speed fast)/4 = 0.75mm/s
   Resolution of reading: 2* 0.75/80 = 0.01875mm

Therefore, 0.01875mm is the worse case scenario for the deviation of the probe, but most of the time I get around 0.004 to 0.008. If you spot mistake in my calculation please let me know.
Theoritically, by lowering the probing speed we can get better resolution, but this also lower the de value. From my test, with slower probe speed, the probe cannot "feel" the hits and give worst deviatioon value (as high as 0.20).

## Issue
* Spongy bed & gantry

   the Ender 3 I use has only one Z-rod on the left side, the right side is a little bit spongy  
   this causes a dampening effect which lower the de value, therefore probing fails occasionally at the right side of bed during ABL.  
   I suspect this will also happen if a spongy bed with weak spring is used  
   Eventually this's solved by using lower threshold_de
   
* Plastic on the nozzle

   Plastic oozing on the nozzle also lower the de value.  
   Marlin and slicer is configured to perform automatic nozzle brushing before homing and probing. 

   ![Screenshot 2021-03-09 002330](https://user-images.githubusercontent.com/75633795/110349500-bc11b600-806d-11eb-8556-dda79681f347.png)
   
* Filament tugging

   With direct drive extruder and filament spool on top of printer frame, the filament will tug the extruder when extruding, or 
   during z-axis motion. This could triggers the probe as well as messing with the accuracy of the probe. In some case, the probe homes at mid-air.
   
   **Solution**: Disable or 'sleep' the probe except when homing. Use bowden tube.

