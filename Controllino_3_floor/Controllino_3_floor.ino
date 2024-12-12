#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>
#include <UniversalTimer.h>

// Constants and Variables
const char* clientId = "Controllino3";  //mqtt client name
const char* reference = "C3";           // Dynamic reference, e.g., "C3" or "C2"
String baseTopic_prefix = "aha/";
String topicHash = baseTopic_prefix + reference + "/#";  // Topic for MQTT subscriptions
const char* unique_id_new;
String base_topic = baseTopic_prefix + reference + "/";
const char* state_suffix = "stat_t";        // MQTT topic suffix for state
const char* command_suffix = "cmd_t";       // MQTT topic suffix for commands
const char* availability_suffix = "avt_t";  // MQTT topic suffix for availability
String discoveryMessage;                    // For storing MQTT discovery messages

// Network and Client Configuration
byte mac[] = { 0xDE, 0xEE, 0xEE, 0xEF, 0xFE, 0xEF };  // MAC address for the Ethernet shield
IPAddress server(192, 168, 178, 42);                  // IP address of the MQTT broker
const int mqtt_port = 1883;
EthernetClient ethClient;
PubSubClient client(ethClient);  // MQTT client for publishing and subscribing
// Pin Configuration for CONTROLLINO
const int pins[] = {
  CONTROLLINO_D0, CONTROLLINO_D1, CONTROLLINO_D2, CONTROLLINO_D3,
  CONTROLLINO_D4, CONTROLLINO_D5, CONTROLLINO_D6, CONTROLLINO_D7,
  CONTROLLINO_D8, CONTROLLINO_D9, CONTROLLINO_D10, CONTROLLINO_D11,
  CONTROLLINO_D12, CONTROLLINO_D13, CONTROLLINO_D14, CONTROLLINO_D15,
  CONTROLLINO_D16, CONTROLLINO_D17, CONTROLLINO_D18, CONTROLLINO_D19,
  CONTROLLINO_D20, CONTROLLINO_D21, CONTROLLINO_D22, CONTROLLINO_D23,
  CONTROLLINO_R0, CONTROLLINO_R1, CONTROLLINO_R2, CONTROLLINO_R3,
  CONTROLLINO_R4, CONTROLLINO_R5, CONTROLLINO_R6, CONTROLLINO_R7,
  CONTROLLINO_R8, CONTROLLINO_R9, CONTROLLINO_R10, CONTROLLINO_R11,
  CONTROLLINO_R12, CONTROLLINO_R13, CONTROLLINO_R14, CONTROLLINO_R15
};
const int numPins = sizeof(pins) / sizeof(pins[0]);
bool previousStates[numPins];  // Stores previous states for pins
bool firstRun = true;          // Flag for initial run of certain operations

int previousSensorValue = LOW;
const int numAnalogPins = 10;
int currentActuatorState = LOW;  // Declare currentActuatorState globally
unsigned long time_now = 0;
#define NUM_ROLLADEN 2
int analogStates[numAnalogPins];  // Array zum Speichern der vorherigen Zustände der A-Ports

struct Rolladen {
  int pin_up;                // Pin to move the shutter up
  int pin_down;              // Pin to move the shutter down
  unsigned long run_time;    // Duration for shutter to fully open/close
  int current_position;      // Current position of the shutter
  String topic;              // MQTT topic for this shutter
  String name;               // Display name for this shutter
  unsigned long start_time;  // Start time for movement
  int target_position;       // Target position to reach
  bool moving_up;            // Flag for moving up direction
  bool moving;               // Movement status
};
Rolladen rolladen[NUM_ROLLADEN] = {
  // { CONTROLLINO_D9, CONTROLLINO_D10, 15000, 0, "aha/C3/rolladen/sz_tuer", "Rolladen SZ Tuer", 0, 0, false, false },
  //{ CONTROLLINO_D8, CONTROLLINO_D7, 15000, 0, "aha/C3/rolladen/buero_tuer", "Rolladen Buero Tuer", 0, 0, false, false }
};


typedef struct {
  const char* base_topic;                 // Base MQTT topic
  int pin;                                // Pin number associated with the topic
  void (*action)(int, int, const char*);  // Action to execute on state change
  const char* name;                       // Display name for this mapping
  const char* unique_id;                  // Unique identifier for MQTT discovery
  const char* deviceIdentifier;           // Device identifier for MQTT
  const char* deviceName;                 // Name of the device
  bool defaultState;                      // Default state of the device (on/off)
} TopicActionMapping;

void digitalWriteAndPublish(int pin, bool state, const char* statusTopic) {
  digitalWrite(pin, state ? HIGH : LOW);
  String FullStatusTopic = String(statusTopic) + "/stat_t";
  client.publish(FullStatusTopic.c_str(), state ? "ON" : "OFF");
}

TopicActionMapping topicActions[] = {
  { "aha/C3/C3_D0", CONTROLLINO_D0, digitalWriteAndPublish, "Stereo Anlage", "C3/C3_D0", "C3_Media", "C3_Media", false },
  { "aha/C3/C3_D1", CONTROLLINO_D1, digitalWriteAndPublish, "LED TV", "C3/C3_D1", "C3_Media", "C3_Media", false },
  { "aha/C3/C3_D2", CONTROLLINO_D2, digitalWriteAndPublish, "Router Server Uhr", "C3/C3_D2", "C3_Media", "C3_Media", true },
  { "aha/C3/C3_D3", CONTROLLINO_D3, digitalWriteAndPublish, "Fernseher", "C3/C3_D3", "C3_Media", "C3_Media", false },
  { "aha/C3/C3_D4", CONTROLLINO_D4, digitalWriteAndPublish, "Markise Hoch", "C3/C3_D4", "C3_Markise", "C3_Markise", false },
  { "aha/C3/C3_D5", CONTROLLINO_D5, digitalWriteAndPublish, "Markise Runter", "C3/C3_D5", "C3_Markise", "C3_Markise", false },
  { "aha/C3/C3_D6", CONTROLLINO_D6, digitalWriteAndPublish, "Tuer Rolladen Runter", "C3/C3_D6", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_D7", CONTROLLINO_D7, digitalWriteAndPublish, "Tuer Rolladen Hoch", "C3/C3_D7", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_D8", CONTROLLINO_D8, digitalWriteAndPublish, "Raumstrom Wohnzimmmer", "C3/C3_D8", "C3_Wohnzimmer", "C3_Wohnzimmer", true },
  { "aha/C3/C3_D9", CONTROLLINO_D9, digitalWriteAndPublish, "Raumstrom Esszimmer", "C3/C3_D9", "C3_Esszimmer", "C3_Esszimmer", true },
  { "aha/C3/C3_D10", CONTROLLINO_D10, digitalWriteAndPublish, "Steckdose Komode 1", "C3/C3_D10", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D11", CONTROLLINO_D11, digitalWriteAndPublish, "Steckdose Komode 2", "C3/C3_D11", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D12", CONTROLLINO_D12, digitalWriteAndPublish, "Steckdose Komode 3", "C3/C3_D12", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D13", CONTROLLINO_D13, digitalWriteAndPublish, "Steckdose Tisch", "C3/C3_D13", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D14", CONTROLLINO_D14, digitalWriteAndPublish, "LED Kommode", "C3/C3_D14", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D15", CONTROLLINO_D15, digitalWriteAndPublish, "Steckdose Fenster", "C3/C3_D15", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D16", CONTROLLINO_D16, digitalWriteAndPublish, "Fenster Seite Rolladen hoch", "C3/C3_D16", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D17", CONTROLLINO_D17, digitalWriteAndPublish, "Fenster Seite Rolladen runter", "C3/C3_D17", "C3_Esszimmer", "C3_Esszimmer", false },
  { "aha/C3/C3_D18", CONTROLLINO_D18, digitalWriteAndPublish, "Hauptfenster Rolladen hoch", "C3/C3_D18", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_D19", CONTROLLINO_D19, digitalWriteAndPublish, "Hauptfenster Rolladen runter", "C3/C3_D19", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_D20", CONTROLLINO_D20, digitalWriteAndPublish, "Strom Aussen", "C3/C3_D20", "C3_Out", "C3_Out", true },
  { "aha/C3/C3_D21", CONTROLLINO_D21, digitalWriteAndPublish, "Licht Aussen Links", "C3/C3_D21", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_D22", CONTROLLINO_D22, digitalWriteAndPublish, "Licht Aussen Rechts", "C3/C3_D22", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_D23", CONTROLLINO_D23, digitalWriteAndPublish, "Licht Hintereingang", "C3/C3_D23", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_R0", CONTROLLINO_R0, digitalWriteAndPublish, "Fire TV", "C3/C3_R0", "C3_Media", "C3_Media", false },
  { "aha/C3/C3_R1", CONTROLLINO_R1, digitalWriteAndPublish, "SatUSB", "C3/C3_R1", "C3_Media", "C3_Media", false },
  { "aha/C3/C3_R2", CONTROLLINO_R2, digitalWriteAndPublish, "Wohnzimmer Deckenlicht", "C3/C3_R2", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_R3", CONTROLLINO_R3, digitalWriteAndPublish, "Aussenlicht Markise", "C3/C3_R3", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_R4", CONTROLLINO_R4, digitalWriteAndPublish, "Esszimmer Deckenlicht", "C3/C3_R4", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_R5", CONTROLLINO_R5, digitalWriteAndPublish, "Steckdose Hecke Oben", "C3/C3_R5", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_R6", CONTROLLINO_R6, digitalWriteAndPublish, "Licht Hecke oben", "C3/C3_R6", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_R7", CONTROLLINO_R7, digitalWriteAndPublish, "Treppenlicht Eingang", "C3/C3_R7", "C3_Out", "C3_Out", false },
  { "aha/C3/C3_R8", CONTROLLINO_R8, digitalWriteAndPublish, "Treppenlicht Weg", "C3/C3_R8", "C3_Our", "C3_Out", false },
  { "aha/C3/C3_R9", CONTROLLINO_R9, digitalWriteAndPublish, "Stehlampe", "C3/C3_R9", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_R10", CONTROLLINO_R10, digitalWriteAndPublish, "Couch links", "C3/C3_R10", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_R11", CONTROLLINO_R11, digitalWriteAndPublish, "Couch rechts", "C3/C3_R11", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_R12", CONTROLLINO_R12, digitalWriteAndPublish, "Couch links 2", "C3/C3_R12", "C3_Wohnzimmer", "C3_Wohnzimmer", false },
  { "aha/C3/C3_R13", CONTROLLINO_R12, digitalWriteAndPublish, "LED", "C3/C3_R13", "C3_Wohnzimmer", "C3_Wohnzimmer", false },

  { "aha/C3/C3_A0", CONTROLLINO_A0, digitalWriteAndPublish, "A0 - Terasse Rollo Tuer hoch", "C3/C3_A0", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A1", CONTROLLINO_A1, digitalWriteAndPublish, "A1 - Terasse Rollo Tuer runter", "C3/C3_A1", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A2", CONTROLLINO_A2, digitalWriteAndPublish, "A2 - Terasse Rollo Haupt runter", "C3/C3_A2", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A3", CONTROLLINO_A3, digitalWriteAndPublish, "A3 - Terasse Rollo Haupt runter", "C3/C3_A3", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A4", CONTROLLINO_A4, digitalWriteAndPublish, "A4 - Sofa 1", "C3/C3_A4", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A5", CONTROLLINO_A5, digitalWriteAndPublish, "A5 - Sofa 2", "C3/C3_A5", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A6", CONTROLLINO_A6, digitalWriteAndPublish, "A6 - Wohnzimmer", "C3/C3_A6", "C3_Eingänge", "C3_Eingänge", false },
  { "aha/C3/C3_A7", CONTROLLINO_A7, digitalWriteAndPublish, "A7 - Esszimmer", "C3/C3_A7", "C3_Eingänge", "C3_Eingänge", false },
 
};

// Function to read and print analog values for pins A0 to A15
void readAnalogValues() {
  for (int i = 0; i <= 15; i++) {
    int buttonPin = CONTROLLINO_A0 + i;

    // Analogen Status des aktuellen Pins prüfen und über MQTT senden
    handleDigitalStatus(buttonPin, i);

    // Schalt- und Statusmeldungen für bestimmte digitale Ausgänge senden
    switch (i) {
      case 0: // Terasse Rollo Hoch
        handleButtonPress(buttonPin, CONTROLLINO_D6, "aha/C3/C3_D6/stat_t");
        break;
      case 1:// Terasse Rollo Runter
        handleButtonPress(buttonPin, CONTROLLINO_D7, "aha/C3/C3_D7/stat_t");
        break;
      case 2:  //Roll Haupt runter
        handleButtonPress(buttonPin, CONTROLLINO_D18, "aha/C3/C3_D18/stat_t");
        break;
      case 3://Roll Haupt hoch
        handleButtonPress(buttonPin, CONTROLLINO_D19, "aha/C3/C3_D19/stat_t");
        break;
      case 4: // Sofa 1 Wohnzimmer
        handleButtonPress(buttonPin, CONTROLLINO_D4, "aha/C3/C3_D4/stat_t");
        break;
      case 5: // Sofa 2 Esszimmer
        handleButtonPress(buttonPin, CONTROLLINO_R4, "aha/C3/C3_R4/stat_t");
        break;
      case 6:  //Wohnzimmer
        handleButtonPress(buttonPin, CONTROLLINO_D4, "aha/C3/C3_D4/stat_t");
        break;
      case 7: //Esszimmer
        handleButtonPress(buttonPin, CONTROLLINO_R4, "aha/C3/C3_R4/stat_t");
              break;
     
    }
  }
}
