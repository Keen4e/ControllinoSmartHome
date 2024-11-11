#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>
#include <UniversalTimer.h>

// Constants and Variables
const char* clientId = "Controllino1";  //mqtt client name
const char* reference = "C1";           // Dynamic reference, e.g., "C1" or "C2"
String baseTopic_prefix = "aha/";
String topicHash = baseTopic_prefix + reference + "/#";  // Topic for MQTT subscriptions
const char* unique_id_new;
String base_topic = baseTopic_prefix + reference + "/";
const char* state_suffix = "stat_t";        // MQTT topic suffix for state
const char* command_suffix = "cmd_t";       // MQTT topic suffix for commands
const char* availability_suffix = "avt_t";  // MQTT topic suffix for availability
String discoveryMessage;                    // For storing MQTT discovery messages

// Network and Client Configuration
byte mac[] = { 0xDE, 0xEF, 0xED, 0xEF, 0xFE, 0xEF };  // MAC address for the Ethernet shield
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
  // { CONTROLLINO_D9, CONTROLLINO_D10, 15000, 0, "aha/C1/rolladen/sz_tuer", "Rolladen SZ Tuer", 0, 0, false, false },
  //{ CONTROLLINO_D8, CONTROLLINO_D7, 15000, 0, "aha/C1/rolladen/buero_tuer", "Rolladen Buero Tuer", 0, 0, false, false }
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
  { "aha/C1/C1_D0", CONTROLLINO_D0, digitalWriteAndPublish, "Schrank Licht rechte Seite", "C1/C1_D0", "C1_Schrank", "C1_Schrank", false },
  { "aha/C1/C1_D1", CONTROLLINO_D1, digitalWriteAndPublish, "Schrank Licht linke Seite", "C1/C1_D1", "C1_Schrank", "C1_Schrank", false },
  { "aha/C1/C1_D2", CONTROLLINO_D2, digitalWriteAndPublish, "Schrank Fussbodenheizung", "C1/C1_D2", "C1_Schrank", "C1_Schrank", false },
  { "aha/C1/C1_D3", CONTROLLINO_D3, digitalWriteAndPublish, "Schlafzimmer Deckenlicht", "C1/C1_D3", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D4", CONTROLLINO_D4, digitalWriteAndPublish, "Schlafzimmer Licht Bett Links", "C1/C1_D4", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D5", CONTROLLINO_D5, digitalWriteAndPublish, "Schlafzimmer Licht Bett Rechts", "C1/C1_D5", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D6", CONTROLLINO_D6, digitalWriteAndPublish, "Schlafzimmer Fussbodenheizung", "C1/C1_D6", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D7", CONTROLLINO_D7, digitalWriteAndPublish, "Buero Rolladen Runter", "C1/C1_D7", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D8", CONTROLLINO_D8, digitalWriteAndPublish, "Buero Rolladen Hoch", "C1/C1_D8", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D9", CONTROLLINO_D9, digitalWriteAndPublish, "Schlafzimmer Rollo Tuer hoch", "C1/C1_D9", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D10", CONTROLLINO_D10, digitalWriteAndPublish, "Schlafzimmer Rollo Tuer runter", "C1/C1_D10", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D11", CONTROLLINO_D11, digitalWriteAndPublish, "Buero Stehlampe", "C1/C1_D11", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D12", CONTROLLINO_D12, digitalWriteAndPublish, "Buero Deckenlicht", "C1/C1_D12", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D13", CONTROLLINO_D13, digitalWriteAndPublish, "Buero Arbeitsplatz", "C1/C1_D13", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D14", CONTROLLINO_D14, digitalWriteAndPublish, "Buero Fussbodenheizung", "C1/C1_D14", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D15", CONTROLLINO_D15, digitalWriteAndPublish, "Buero Raumstrom", "C1/C1_D15", "C1_Buero", "C1_Buero", true },
  { "aha/C1/C1_D16", CONTROLLINO_D16, digitalWriteAndPublish, "Buero Heizung Schreibtisch", "C1/C1_D16", "C1_Buero", "C1_Buero", false },
  { "aha/C1/C1_D17", CONTROLLINO_D17, digitalWriteAndPublish, "Schlafzimmer Rollo runter", "C1/C1_D17", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D18", CONTROLLINO_D18, digitalWriteAndPublish, "Schlafzimmer Rolle hoch", "C1/C1_D18", "C1_Schlafzimmer", "C1_Schlafzimmer", false },
  { "aha/C1/C1_D19", CONTROLLINO_D19, digitalWriteAndPublish, "Bad Licht", "C1/C1_D19", "C1_Bad", "C1_Bad", false },
  { "aha/C1/C1_D21", CONTROLLINO_D21, digitalWriteAndPublish, "Bad Dusche Licht", "C1/C1_D21", "C1_Bad", "C1_Bad", false },
  { "aha/C1/C1_D23", CONTROLLINO_D23, digitalWriteAndPublish, "Bad Fussbodenheizung", "C1/C1_D23", "C1_Bad", "C1_Bad", false },
  { "aha/C1/C1_R1", CONTROLLINO_R1, digitalWriteAndPublish, "Schrank Deckenlicht", "C1/C1_R1", "C1_Schrank", "C1_Schrank", false },
  { "aha/C1/C1_R3", CONTROLLINO_R3, digitalWriteAndPublish, "Bad Schublade", "C1/C1_R3", "C1_Bad", "C1_Bad", false },
  { "aha/C1/C1_R5", CONTROLLINO_R5, digitalWriteAndPublish, "Schrank Pumpe", "C1/C1_R5", "C1_Schrank", "C1_Schrank", false },
  { "aha/C1/C1_R8", CONTROLLINO_R8, digitalWriteAndPublish, "Flur Licht", "C1/C1_R8", "C1_Flur", "C1_Flur", false },
  { "aha/C1/C1_R9", CONTROLLINO_R9, digitalWriteAndPublish, "Flur Fussbodenheizung", "C1/C1_R9", "C1_Flur", "C1_Flur", false },
  { "aha/C1/C1_R11", CONTROLLINO_R11, digitalWriteAndPublish, "Flur Licht", "C1/C1_R11", "C1_Flur", "C1_Flur", false },
  { "aha/C1/C1_R12", CONTROLLINO_R12, digitalWriteAndPublish, "Bad Raumstrom", "C1/C1_R12", "C1_Bad", "C1_Bad", true },
  { "aha/C1/C1_R14", CONTROLLINO_R14, digitalWriteAndPublish, "Schlafzimmer Raumstrom", "C1/C1_R14", "C1_Schlafzimmer", "C1_Schlafzimmer", true },
  { "aha/C1/C1_R15", CONTROLLINO_R15, digitalWriteAndPublish, "Schrank Raumstrom", "C1/C1_R15", "C1_Schrank", "C1_Schrank", true },
  //{ "aha/C1/C1_R16", CONTROLLINO_R16, digitalWriteAndPublish, "Schrank Steckdose", "C1/C1_R16", "C1_Schrank", "C1_Schrank" },
  //{ "aha/C1/C1_R17", CONTROLLINO_R17, digitalWriteAndPublish, "Schrank Steckdose", "C1/C1_R17", "C1_Schrank", "C1_Schrank" },

  { "aha/C1/C1_A0", CONTROLLINO_A0, digitalWriteAndPublish, "A0 - Schrank re", "C1/C1_A0", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A1", CONTROLLINO_A1, digitalWriteAndPublish, "A1 - Schrank li", "C1/C1_A1", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A2", CONTROLLINO_A2, digitalWriteAndPublish, "A2 - Bad Spiegel", "C1/C1_A2", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A3", CONTROLLINO_A3, digitalWriteAndPublish, "A3 - Schlafzimmer Deckenlicht", "C1/C1_A3", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A4", CONTROLLINO_A4, digitalWriteAndPublish, "A4 - Schlafzimmer li", "C1/C1_A4", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A5", CONTROLLINO_A5, digitalWriteAndPublish, "A5 - Schlafzimmer re", "C1/C1_A5", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A6", CONTROLLINO_A6, digitalWriteAndPublish, "A6 - Bad", "C1/C1_A6", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A7", CONTROLLINO_A7, digitalWriteAndPublish, "A7 - Rolladen rechts hoch", "C1/C1_A7", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A8", CONTROLLINO_A8, digitalWriteAndPublish, "A8 - Rolladen rechts runter", "C1/C1_A8", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A9", CONTROLLINO_A9, digitalWriteAndPublish, "A9 - Rolladen links hoch", "C1/C1_A9", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A10", CONTROLLINO_A10, digitalWriteAndPublish, "A10 - Rolladen links runter", "C1/C1_A10", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A11", CONTROLLINO_A11, digitalWriteAndPublish, "A11 - Buero Licht", "C1/C1_A11", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A14", CONTROLLINO_A14, digitalWriteAndPublish, "A14 - Buero Rolladen hoch", "C1/C1_A14", "C1_Eingänge", "C1_Eingänge", false },
  { "aha/C1/C1_A15", CONTROLLINO_A15, digitalWriteAndPublish, "A15 - Buero Rolladen runter", "C1/C1_A15", "C1_Eingänge", "C1_Eingänge", false }






};




// Function to read and print analog values for pins A0 to A15
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
        handleButtonPress(buttonPin, CONTROLLINO_D1, "aha/C1/C1_D0/stat_t");
        break;
      case 2:  //Flur Licht
        handleButtonPress(buttonPin, CONTROLLINO_R8, "aha/C1/C1_R8/stat_t");
        break;
      case 3:
        handleButtonPress(buttonPin, CONTROLLINO_D3, "aha/C1/C1_D3/stat_t");
        break;
      case 4:
        handleButtonPress(buttonPin, CONTROLLINO_D4, "aha/C1/C1_D4/stat_t");
        break;
      case 5:
        handleButtonPress(buttonPin, CONTROLLINO_D9, "aha/C1/C1_D5/stat_t");
        handleButtonPress(buttonPin, CONTROLLINO_D18, "aha/C1/C1_D10/stat_t");
        break;
      case 6:  //Bad Licht
        handleButtonPress(buttonPin, CONTROLLINO_D19, "aha/C1/C1_D19/stat_t");
        break;
      case 7:
        handleButtonPress(buttonPin, CONTROLLINO_D10, "aha/C1/C1_D7/stat_t");
        handleButtonPress(buttonPin, CONTROLLINO_D17, "aha/C1/C1_D9/stat_t");
        break;
      case 8:
        handleButtonPress(buttonPin, CONTROLLINO_D10, "aha/C1/C1_D8/stat_t");
        break;
      case 9:
        //  handleButtonPress(buttonPin, CONTROLLINO_D17, CONTROLLINO_D17, "aha/C1/C1_D9/stat_t", "aha/C1/C1_D9/stat_t");
        break;
      case 10:
        // handleButtonPress(buttonPin, CONTROLLINO_D18, CONTROLLINO_D18, "aha/C1/C1_D10/stat_t", "aha/C1/C1_D10/stat_t");
        break;
      case 11:
        handleButtonPress(buttonPin, CONTROLLINO_D12, "aha/C1/C1_D12/stat_t");
        break;
      case 12:
        handleButtonPress(buttonPin, CONTROLLINO_D11, "aha/C1/C1_D11/stat_t");
        break;
      case 14:
        handleButtonPress(buttonPin, CONTROLLINO_D17, "aha/C1/C1_D17/stat_t");
        break;
      case 15:
        handleButtonPress(buttonPin, CONTROLLINO_D18, "aha/C1/C1_D18/stat_t");
        break;
        // Add more cases as needed
    }
  }
}
