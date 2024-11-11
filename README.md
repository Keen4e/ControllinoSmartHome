# ControllinoSmartHome

This is the initial upload of my Controllino programming to implement my smarthome. 

I use 2 controllinos - hence there are 2 files. They contain the same logic with different configurations. 

There is still a lot of quick and dirty in it - but so far it works ;) Cleanup is planned once time permits

# Introduction:
This project covers the following functions:
- Mapping fron input ports to output ports
- Auto generation of MQTT Autodiscovery for Homeassistant
- Generation of State and availability messages 
- State Handling via MQTT (irrespective of trigger, e.g. Hardware button, MQTT message, homeassistant change)
- Remote triggering of MQTT messages to come back online after connection loss

# Details and configuration:
## File structure
Each Controllino uses 3 files: 
- Controllino_x_floor.ino (Contains the parameters)
- shutter_functions.ino (generic functions for the roller shutter - typically no need to change anything here)
- generic_functions.ino (all other functions- typically no need to change anything here)

## Configuration
The file Controllino_x_floor.ino needs to be set to the respective environment
```
const char* clientId = "Controllino1";  //mqtt client name
const char* reference = "C1";           // Dynamic reference, e.g., "C1" or "C2"
byte mac[] = { 0xDE, 0xEF, 0xED, 0xEF, 0xFE, 0xEF };  // MAC address for the Ethernet shield
IPAddress server(192, 168, 178, 42);                  // IP address of the MQTT broker
```
This strcture takes care that the entities are setup in homeassistant accordingly: 
```
const char* base_topic;                 // Base MQTT topic
  int pin;                                // Pin number associated with the topic
  void (*action)(int, int, const char*);  // Action to execute on state change
  const char* name;                       // Display name for this mapping
  const char* unique_id;                  // Unique identifier for MQTT discovery
  const char* deviceIdentifier;           // Device identifier for MQTT
  const char* deviceName;                 // Name of the device
  bool defaultState;                      // Default state of the device (on/off)
```
Example:
```
TopicActionMapping topicActions[] = {
  { "aha/C1/C1_D0", CONTROLLINO_D0, digitalWriteAndPublish, "Schrank Licht rechte Seite", "C1/C1_D0", "C1_Schrank", "C1_Schrank", false },
```
In this function one maps the inputs (handled via the respective cases) to the outputs of the controllino
```
void readAnalogValues() {
  for (int i = 0; i <= 15; i++) {
    int buttonPin = CONTROLLINO_A0 + i;

    // Analogen Status des aktuellen Pins prüfen und über MQTT senden
    handleDigitalStatus(buttonPin, i);

    // Schalt- und Statusmeldungen für bestimmte digitale Ausgänge senden
    switch (i) {
      case 0:
        handleButtonPress(buttonPin, CONTROLLINO_D0, "aha/C1/C1_D0/stat_t");
        break;
      case 1:

```
