/* 
This is a simple bluetooth joystic control using nrf52.
 */

#include <Arduino.h>
#include <bluefruit.h>

BLEDis bledis;
BLEHidAdafruit blehid;

// Array of pins where buttons and joystick are wired, along with keycode array to follow.
uint8_t pins[]    = { 11, A0, A1, A2, A3, A4, A5, 7, 15, 16, 31, 30};
uint8_t hidcode[] = { HID_KEY_SHIFT_LEFT, HID_KEY_ARROW_UP, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_LEFT ,HID_KEY_ARROW_RIGHT, HID_KEY_Q, HID_KEY_W, HID_KEY_RETURN, HID_KEY_Z, HID_KEY_X, HID_KEY_A, HID_KEY_S };
uint8_t pincount = sizeof(pins)/sizeof(pins[0]);

bool keyPressedPreviously = false;

bool flashing = false;
uint8_t connectedLed = 26;
bool connectedLedState = false;
long ledRate = 1000;
long lastLedSync = 0;
bool connected=false;


static void connect_callback(uint16_t conn_handle) {
  Serial.println("Connected");  
  connectedLedState=true;
  flashing=false;
  connected=true;
  digitalWrite(connectedLed, connectedLedState);
}


static void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("Disonnected");  
  connectedLedState=false;
  flashing=true;
  connected=false;
  digitalWrite(connectedLed, connectedLedState);
}


void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  
  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  // Start Advertising 
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}



void flash_connected_led() 
{
  if (connected) {
    return;
  }
  if (millis() - lastLedSync >=1000) 
  {
    lastLedSync = millis();    
    connectedLedState = !connectedLedState;
    digitalWrite(connectedLed, connectedLedState);
  }
}


void set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap)
{
  (void) conn_handle;
  
  // light up Red Led if any bits is set
  if ( led_bitmap )
  {
    ledOn( LED_RED );
  }
  else
  {
    ledOff( LED_RED );
  }
}

void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 HID Keyscan Example");

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values -40, -20, -16, -12, -8, -4, 0, 3, 4
  Bluefruit.setName("BLE Joystick");

  // Configure and Start Device Information Service
  bledis.setManufacturer("Psimax Aerospace");
  bledis.setModel("Joy 7");
  bledis.begin();

  // set up pin as input
  for (uint8_t i=0; i<pincount; i++)
  {
    pinMode(pins[i], INPUT_PULLUP);
  }

  // leds
  pinMode(connectedLed, OUTPUT);

  blehid.begin();
  startAdv();
}



void loop()
{
  /*-------------- Scan Pin Array and send report ---------------------*/
  bool anyKeyPressed = false;

  uint8_t modifier = 0;
  uint8_t count=0;
  uint8_t keycode[6] = { 0 };

  // scan normal key and send report
  for(uint8_t i=0; i < pincount; i++)
  {

    if ( 0 == digitalRead(pins[i]) )
    {
      // if pin is active (low), add its hid code to key report
      keycode[count++] = hidcode[i];

      // 6 is max keycode per report
      if ( count == 6)
      {
        blehid.keyboardReport(modifier, keycode);

        // reset report
        count = 0;
        memset(keycode, 0, 6);
      }

      // used later
      anyKeyPressed = true;
      keyPressedPreviously = true;
    }    
  }

  // Send any remaining keys (not accumulated up to 6)
  if ( count )
  {
    blehid.keyboardReport(modifier, keycode);
  }

  // Send All-zero report to indicate there is no keys pressed
  // Most of the time, it is, though we don't need to send zero report
  // every loop(), only a key is pressed in previous loop() 
  if ( !anyKeyPressed && keyPressedPreviously )
  {
    keyPressedPreviously = false;
    blehid.keyRelease();
  }  
  
  
  if (!connected) {
    flash_connected_led();
  }

  // Poll interval
  delay(1);
}