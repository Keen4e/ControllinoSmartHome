#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>
#include <UniversalTimer.h>

// Constants and Variables
const char* clientId = "Controllino2";  //mqtt client name
const char* reference = "C2";           // Dynamic reference, e.g., "C2" or "C2"
String baseTopic_prefix = "aha/";
String topicHash = baseTopic_prefix + reference + "/#";  // Topic for MQTT subscriptions
const char* unique_id_new;
String base_topic = baseTopic_prefix + reference + "/";
const char* state_suffix = "stat_t";        // MQTT topic suffix for state
const char* command_suffix = "cmd_t";       // MQTT topic suffix for commands
const char* availability_suffix = "avt_t";  // MQTT topic suffix for availability
String discoveryMessage;                    // For storing MQTT discovery messages

// Network and Client Configuration
byte mac[] = { 0xDE, 0xEF, 0xED, 0xEE, 0xFE, 0xEF };  // MAC address for the Ethernet shield
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
  // { CONTROLLINO_D9, CONTROLLINO_D10, 15000, 0, "aha/C2/rolladen/sz_tuer", "Rolladen SZ Tuer", 0, 0, false, false },
  //{ CONTROLLINO_D8, CONTROLLINO_D7, 15000, 0, "aha/C2/rolladen/buero_tuer", "Rolladen Buero Tuer", 0, 0, false, false }
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
  { "aha/C2/C2_D0", CONTROLLINO_D0, digitalWriteAndPublish, "NSPanel Kueche", "C2/C2_D0", "C2_Kueche", "C2_Kueche", true },
  { "aha/C2/C2_D1", CONTROLLINO_D1, digitalWriteAndPublish, "Licht Kueche", "C2/C2_D1", "C2_Kueche", "C2_Kueche", false },
  { "aha/C2/C2_D2", CONTROLLINO_D2, digitalWriteAndPublish, "Kuehlschrank", "C2/C2_D2", "C2_Kueche", "C2_Kueche", true },
  { "aha/C2/C2_D3", CONTROLLINO_D3, digitalWriteAndPublish, "Raumstrom Kueche", "C2/C2_D3", "C2_Kueche", "C2_Kueche", true },
  { "aha/C2/C2_D4", CONTROLLINO_D4, digitalWriteAndPublish, "Esszimmer Rolladen hoch", "C2/C2_D4", "C2_Esszimmer", "C2_Esszimmer", false },
  { "aha/C2/C2_D5", CONTROLLINO_D5, digitalWriteAndPublish, "Esszimmer Rolladen runter", "C2/C2_D5", "C2_Esszimmer", "C2_Esszimmer", false },
  { "aha/C2/C2_D6", CONTROLLINO_D6, digitalWriteAndPublish, "Kueche Steckdose bei Rolladen (todo)", "C2/C2_D6", "C2_Kueche", "C2_Kueche", true },
  { "aha/C2/C2_D7", CONTROLLINO_D7, digitalWriteAndPublish, "Kueche Rolladen hoch", "C2/C2_D7", "C2_Kueche", "C2_Kueche", false },
  { "aha/C2/C2_D8", CONTROLLINO_D8, digitalWriteAndPublish, "Kueche Rolladen runter", "C2/C2_D8", "C2_Kueche", "C2_Kueche", false },
  { "aha/C2/C2_D9", CONTROLLINO_D9, digitalWriteAndPublish, "Esszimmer Steckdose Rolladen", "C2/C2_D9", "C2_Esszimmer", "C2_Esszimmer", false },
  { "aha/C2/C2_D10", CONTROLLINO_D10, digitalWriteAndPublish, "Klo Licht", "C2/C2_D10", "C2_Klo", "C2_Klo", false },
  { "aha/C2/C2_D11", CONTROLLINO_D11, digitalWriteAndPublish, "Esszimmer Deckenlampe", "C2/C2_D11", "C2_Esszimmer", "C2_Esszimmer", false },
  { "aha/C2/C2_D12", CONTROLLINO_D12, digitalWriteAndPublish, "D12 tbd", "C2/C2_D12", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D13", CONTROLLINO_D13, digitalWriteAndPublish, "Klingel und Flur", "C2/C2_D13", "C2_out", "C2_out", true },
  { "aha/C2/C2_D14", CONTROLLINO_D14, digitalWriteAndPublish, "Flur Licht", "C2/C2_D14", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D15", CONTROLLINO_D15, digitalWriteAndPublish, "Flur Garderobe", "C2/C2_D15", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D16", CONTROLLINO_D16, digitalWriteAndPublish, "Fire Tablet", "C2/C2_D16", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D17", CONTROLLINO_D17, digitalWriteAndPublish, "Flur Treppe oben 1", "C2/C2_D17", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D18", CONTROLLINO_D18, digitalWriteAndPublish, "Flur Treppe oben 2", "C2/C2_D18", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D19", CONTROLLINO_D19, digitalWriteAndPublish, "Flur Treppe oben 3", "C2/C2_D19", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D20", CONTROLLINO_D20, digitalWriteAndPublish, "Flur Licht Spiegel", "C2/C2_D20", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D21", CONTROLLINO_D21, digitalWriteAndPublish, "Flur Treppen unten", "C2/C2_D21", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D22", CONTROLLINO_D22, digitalWriteAndPublish, "Flur Treppe mitte 2", "C2/C2_D22", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_D23", CONTROLLINO_D23, digitalWriteAndPublish, "Flur Treppe mitte 3", "C2/C2_D23", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R0", CONTROLLINO_R0, digitalWriteAndPublish, "Flur Treppe unten 1", "C2/C2_R0", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R1", CONTROLLINO_R1, digitalWriteAndPublish, "Flur Treppe unten 12", "C2/C2_R1", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R2", CONTROLLINO_R2, digitalWriteAndPublish, "Flur Treppe unten 3", "C2/C2_R2", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R3", CONTROLLINO_R3, digitalWriteAndPublish, "Flur Treppenhaus", "C2/C2_R3", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R4", CONTROLLINO_R4, digitalWriteAndPublish, "R4 tbd", "C2/C2_R4", "C2_tbd", "C2_tbd", false },
  { "aha/C2/C2_R5", CONTROLLINO_R5, digitalWriteAndPublish, "Eszzimmer Steckdose altes Licht", "C2/C2_R5", "C2_Esszimmer", "C2_Esszimmer", false },
  { "aha/C2/C2_R6", CONTROLLINO_R6, digitalWriteAndPublish, "R6 tbd", "C2/C2_R6", "C2_tbd", "C2_tbd", false },
  { "aha/C2/C2_R7", CONTROLLINO_R7, digitalWriteAndPublish, "R7 tbd", "C2/C2_R7", "C2_tbd", "C2_tbd", false },
  { "aha/C2/C2_R8", CONTROLLINO_R8, digitalWriteAndPublish, "Werkzeugkeller", "C2/C2_R8", "C2_Werkzeugkeller", "C2_Werkzeugkeller", false },
  { "aha/C2/C2_R9", CONTROLLINO_R9, digitalWriteAndPublish, "Treppe ganz oben 3", "C2/C2_R9", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R10", CONTROLLINO_R10, digitalWriteAndPublish, "Treppe ganz oben 2", "C2/C2_R10", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R11", CONTROLLINO_R11, digitalWriteAndPublish, "Treppe ganz oben 1", "C2/C2_R11", "C2_Flur", "C2_Flur", false },
  { "aha/C2/C2_R12", CONTROLLINO_R12, digitalWriteAndPublish, "Raumstrom Hobbyraum", "C2/C2_R12", "C2_Hobbyraum", "C2_Hobbyaum", false },
  { "aha/C2/C2_R13", CONTROLLINO_R13, digitalWriteAndPublish, "R13 tbd", "C2/C2_R13", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_R14", CONTROLLINO_R14, digitalWriteAndPublish, "R14 tbd", "C2/C2_R14", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_R15", CONTROLLINO_R15, digitalWriteAndPublish, "R15 tbd", "C2/C2_R15", "C2_Eingänge", "C2_Eingänge", false },
 
  { "aha/C2/C2_A1", CONTROLLINO_A1, digitalWriteAndPublish, "A1 - Rollo Kueche hoch", "C2/C2_A1", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A2", CONTROLLINO_A2, digitalWriteAndPublish, "A2 - Rollo Kueche runter", "C2/C2_A2", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A3", CONTROLLINO_A3, digitalWriteAndPublish, "A3 - Roll Esszimmer runter", "C2/C2_A3", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A4", CONTROLLINO_A4, digitalWriteAndPublish, "A4 - Rollo Esszimmer hoch", "C2/C2_A4", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A5", CONTROLLINO_A5, digitalWriteAndPublish, "A5 - Schalter Waeschekeller", "C2/C2_A5", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A6", CONTROLLINO_A6, digitalWriteAndPublish, "A6 - Schalter Hobbyraum", "C2/C2_A6", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A7", CONTROLLINO_A7, digitalWriteAndPublish, "A7 - Schalter Wohnzimmer", "C2/C2_A7", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A8", CONTROLLINO_A8, digitalWriteAndPublish, "A8 - Flur alter 3er re", "C2/C2_A8", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A9", CONTROLLINO_A9, digitalWriteAndPublish, "A9 - Flur alter 3er li", "C2/C2_A9", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A10", CONTROLLINO_A10, digitalWriteAndPublish, "A10 - Schalter Haustuer", "C2/C2_A10", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A11", CONTROLLINO_A11, digitalWriteAndPublish, "A11 - Schalter WC", "C2/C2_A11", "C2_Eingänge", "C2_Eingänge", false },
  { "aha/C2/C2_A12", CONTROLLINO_A12, digitalWriteAndPublish, "A12 - Schalter Kueche", "C2/C2_A11", "C2_Eingänge", "C2_Eingänge", false }

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
                break;
            case 1://A1 - Rollo Kueche hoch
                handleButtonPress(buttonPin, CONTROLLINO_D5, "aha/C2/C2_D5/stat_t");
                break;
            case 2://A2 - Rollo Kueche runter
                handleButtonPress(buttonPin, CONTROLLINO_D4, "aha/C2/C2_D4/stat_t");
                break;
            case 3://A3 - Roll Esszimmer runter
                handleButtonPress(buttonPin, CONTROLLINO_D7, "aha/C2/C2_D7/stat_t");
                break;
            case 4://A4 - Rollo Esszimmer hoch
                handleButtonPress(buttonPin, CONTROLLINO_D8, "aha/C2/C2_D8/stat_t");
                break;
            case 5://A5 - Schalter Waeschekeller
                handleButtonPress(buttonPin, CONTROLLINO_D21, "aha/C2/C2_D21/stat_t");
                handleButtonPress(buttonPin, CONTROLLINO_R1, "aha/C2/C2_R1/stat_t");
                break;
            case 6://A6 - Schalter Hobbyraum
                handleButtonPress(buttonPin, CONTROLLINO_D21, "aha/C2/C2_D21/stat_t");
                handleButtonPress(buttonPin, CONTROLLINO_R1, "aha/C2/C2_R1/stat_t");
                break;
            case 7://A7 - Schalter Wohnzimmer
                handleButtonPress(buttonPin, CONTROLLINO_D20, "aha/C2/C2_D20/stat_t");
                break;
            case 8://A8 - Flur alter 3er re
                handleButtonPress(buttonPin, CONTROLLINO_D11, "aha/C2/C2_D11/stat_t");
                break;
            case 9://A9 - Flur alter 3er li
                handleButtonPress(buttonPin, CONTROLLINO_D12, "aha/C2/C2_D12/stat_t");
                break;
            case 10://A10 - Schalter Haustuer
                handleButtonPress(buttonPin, CONTROLLINO_D12, "aha/C2/C2_D12/stat_t");
                break;
            case 11://A11 - Schalter WC
                handleButtonPress(buttonPin, CONTROLLINO_D10, "aha/C2/C2_D10/stat_t");
                break;
            case 12://A12 - Schalter Kueche"
                handleButtonPress(buttonPin, CONTROLLINO_D1, "aha/C2/C2_D1/stat_t");
                break;
        }
    }
}
