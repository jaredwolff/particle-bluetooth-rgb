/*
 * Project ble-rgb-control
 * Description: Control the onboard RGB led using BLE
 * Author: Jared Wolff
 * Date: 8/9/2019
 */

SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler(115200, LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_ALL }, // enable all app messages
});

#define MAX_BATT_V 4.1
#define MIN_BATT_V 3.1
#define MEASUREMENT_INTERVAL_MS 10000

// UUID for battery service
BleUuid batteryServiceUUID = BleUuid(0x180F);
BleUuid batteryCharUUID = BleUuid(0x2A19);

// Timer for batt measurement
system_tick_t lastMeasurementMs = 0;

// Batt char
BleCharacteristic batteryLevelCharacteristic;

// UUIDs for service + characteristics
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

void batteryProcess() {
 // Reset if overflow
  if( millis() < lastMeasurementMs ) {
    lastMeasurementMs = millis();
  }

  // Check if it's time to make a measurement
  if( millis() > (lastMeasurementMs + MEASUREMENT_INTERVAL_MS) ) {
    lastMeasurementMs = millis();

    float voltage = analogRead(BATT) * 0.0011224;
    float normalized = (voltage-MIN_BATT_V)/(MAX_BATT_V-MIN_BATT_V) * 100;

    // If normalized goes above or below the min/max, set to the min/max
    if( normalized > 100 ) {
      normalized = 100;
    } else if( normalized < 0 ) {
      normalized = 0;
    }

    // Set the battery value
    batteryLevelCharacteristic.setValue((uint8_t)normalized);

    // Print the results
    Log.info("Batt level: %d", (uint8_t)normalized);

  }
}

// setup() runs once, when the device is first turned on.
void setup() {

  (void)logHandler;

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
  batteryLevelCharacteristic = BleCharacteristic ("bat", BleCharacteristicProperty::NOTIFY, batteryCharUUID, batteryServiceUUID);

  // Add the characteristics
  BLE.addCharacteristic(redCharacteristic);
  BLE.addCharacteristic(greenCharacteristic);
  BLE.addCharacteristic(blueCharacteristic);
  BLE.addCharacteristic(batteryLevelCharacteristic);

  // Advertising data
  BleAdvertisingData advData;

  // Add the RGB LED service
  advData.appendServiceUUID(batteryServiceUUID);
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

  // Check battery
  batteryProcess();

}