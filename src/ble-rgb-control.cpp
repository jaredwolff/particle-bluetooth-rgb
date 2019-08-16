/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "application.h"
#line 1 "/Users/jaredwolff/Circuit_Dojo/ble-rgb-control/src/ble-rgb-control.ino"
/*
 * Project ble-rgb-control
 * Description:
 * Author: Jared Wolff
 * Date: 8/9/2019
 */

//SYSTEM_MODE(MANUAL);

// UUIDs for service + characteristics
static void meshHandler(const char *event, const char *data);
static void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
void setup();
void loop();
#line 11 "/Users/jaredwolff/Circuit_Dojo/ble-rgb-control/src/ble-rgb-control.ino"
const char* serviceUuid = "b4250400-fb4b-4746-b2b0-93f0e61122c6"; //service
const char* red         = "b4250401-fb4b-4746-b2b0-93f0e61122c6"; //red char
const char* green       = "b4250402-fb4b-4746-b2b0-93f0e61122c6"; //green char
const char* blue        = "b4250403-fb4b-4746-b2b0-93f0e61122c6"; //blue char

// Set the RGB BLE service
BleUuid rgbService(serviceUuid);

// Variables for keeping state
typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} led_level_t;

// Static level tracking
static led_level_t m_led_level;

// Tracks when to publish to Mesh
static bool m_publish;

// Mesh event handler
static void meshHandler(const char *event, const char *data)
{

  // Convert to String for useful conversion and comparison functions
  String eventString = String(event);
  String dataString = String(data);

  // Determine which event we recieved
  if( eventString.equals("red") ) {
    m_led_level.red = dataString.toInt();
  } else if ( eventString.equals("green") ) {
    m_led_level.green = dataString.toInt();
  } else if ( eventString.equals("blue") ) {
    m_led_level.blue = dataString.toInt();
  } else {
    return;
  }

  // Set RGB color
  RGB.color(m_led_level.red, m_led_level.green, m_led_level.blue);

}

// Static function for handling Bluetooth Low Energy callbacks
static void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {

  // We're only looking for one byte
  if( len != 1 ) {
    return;
  }

  // Sets the global level
  if( context == red ) {
    m_led_level.red = data[0];
  } else if ( context == green ) {
    m_led_level.green = data[0];
  } else if ( context == blue ) {
    m_led_level.blue = data[0];
  }

  // Set RGB color
  RGB.color(m_led_level.red, m_led_level.green, m_led_level.blue);

  // Set to publish
  m_publish = true;

}

// setup() runs once, when the device is first turned on.
void setup() {

  // Enable app control of LED
  RGB.control(true);

  // Init default level
  m_led_level.red = 0;
  m_led_level.green = 0;
  m_led_level.blue = 0;

  // Set to false at first
  m_publish = false;

  // Set the subscription for Mesh updates
  Mesh.subscribe("red",meshHandler);
  Mesh.subscribe("green",meshHandler);
  Mesh.subscribe("blue",meshHandler);

  // Set up characteristics
  BleCharacteristic redCharacteristic("red", BleCharacteristicProperty::WRITE_WO_RSP, red, serviceUuid, onDataReceived, (void*)red);
  BleCharacteristic greenCharacteristic("green", BleCharacteristicProperty::WRITE_WO_RSP, green, serviceUuid, onDataReceived, (void*)green);
  BleCharacteristic blueCharacteristic("blue", BleCharacteristicProperty::WRITE_WO_RSP, blue, serviceUuid, onDataReceived, (void*)blue);

  // Add the characteristics
  BLE.addCharacteristic(redCharacteristic);
  BLE.addCharacteristic(greenCharacteristic);
  BLE.addCharacteristic(blueCharacteristic);

  // Advertising data
  BleAdvertisingData advData;

  // Add the RGB LED service
  advData.appendServiceUUID(rgbService);

  // Start advertising!
  BLE.advertise(&advData);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  // Checks the publish flag,
  // Publishes to a variable called "red" "green" and "blue"
  if( m_publish ) {

    // Reset flag
    m_publish = false;

    // Publish to Mesh
    Mesh.publish("red", String::format("%d", m_led_level.red));
    Mesh.publish("green", String::format("%d", m_led_level.green));
    Mesh.publish("blue", String::format("%d", m_led_level.blue));
  }

}