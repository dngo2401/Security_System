#include <ArduinoBLE.h>
#include <HardwareSerial.h>

BLEService security_service("8964B976-F323-E27D-3030-4998C7B3711D");
BLEStringCharacteristic sensor_data("8964B977-F323-E27D-3030-4998C7B3711D", BLERead | BLENotify, 128);
BLEStringCharacteristic arm_command("8964B978-F323-E27D-3030-4998C7B3711D", BLERead | BLEWrite, 1);

HardwareSerial ardu(2);

void command_handler(BLEDevice central, BLECharacteristic characteristic) {
  String command = arm_command.value();
  if (command == "1") {
    ardu.write('1');
  } else {
    ardu.write('0');
  }
}

void connect_handler(BLEDevice central) {
  digitalWrite(2, HIGH);
}

void disconnect_handler(BLEDevice central) {
  digitalWrite(2, LOW);
}

void setup() {
  pinMode(2, OUTPUT);
  BLE.begin();
  ardu.begin(9600, SERIAL_8N1, 16, 17); 

  BLE.setAdvertisedService(security_service);
  BLE.setEventHandler(BLEConnected, connect_handler);
  BLE.setEventHandler(BLEDisconnected, disconnect_handler);

  security_service.addCharacteristic(sensor_data);
  security_service.addCharacteristic(arm_command);
  
  arm_command.setEventHandler(BLEWritten, command_handler);

  BLE.addService(security_service);
  BLE.advertise();
}

void loop() {
  BLE.poll();
  if (ardu.available())
    sensor_data.setValue(ardu.readString());
}

