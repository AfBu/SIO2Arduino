SIO2Arduino is an open-source implementation of the Atari SIO device protocol that runs on Arduino hardware. Arduino hardware is open-source and can be assembled by hand or purchased preassembled.

Note: You will need the SdFat library (http://code.google.com/p/sdfatlib/) in your Arduino libraries directory in order to compile.

AfBu's version modified SIO2Arduino in following ways:
- all new features depends heavily on 1602 LCD
- multi-drive mode, tested sucessfuly on Arduino UNO with 2 drives, on MEGA you can go probably much higher than that
- drive-activity led, is blinking when commands are processed on any of drives
- selector button mode is completely re-done:
  - supports sub-directories, yay!
  - requires 2 buttons (1st for file selection, 2nd for drive selection (short press) and image mounting/directory change (long press))
  - image is selected prior mounting, so you can easily prepare image and mount it later
- multi-language support (at compile time)
- display of free sram is available in config
- reset button mode was not tested and may be broken (LCDs are so cheap, buy one, you will not regret it, and use selection mode)
- also teensy support is currently unkown

For more information on SIO2Arduino, see the website at: 
http://www.whizzosoftware.com/sio2arduino

For more information on the Arduino platform, go to:
http://www.arduino.cc