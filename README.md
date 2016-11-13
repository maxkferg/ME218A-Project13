# ME21A-Project13
Microcontroller code for team 13 group project


## Services

### LifecycleService (Josh)
LifeCycleService controls the following:
1. Welcome performance
2. Passage of time
3. Final performance	
The Lifecycle service does not interface with any hardware=it only post messages to the hardware services (Water tube and LED service)
  
### MicrophoneService (Max)
* Handle the input from the Microphone 
* Perform a FFT on the signal
* Post to the LED and water tube services

### SensitivityService (Max)
* Control the input on the sensitivity knob
* Store the current sensitivity as a module variable
* Post any sensitivy chnages to ALL services
* Fire haptic feedback on the sensitivity knob

### WaterTubeService (Gregory)
* This service controls the height of the water level
* It is a hardware service, so it interfaces directly with the 
* Tiva/PWM and servo motors. Note this module does not know anything
about the state of the performance. It just does what it is told.

### LEDService (Chi)
* Control the LED Strips
* This is a hardware only service. It just obeys the messages it gets from other services
* Other services can post LED_CHANGE_X to change the color of each LED strip




