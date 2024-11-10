#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>
#include <UniversalTimer.h>

const char* clientId = "Controllino1";
const char* reference = "C1";  // Dynamische Referenz, z.B. "C1" oder "C2"
String baseTopic_prefix = "aha/";
String topicPrefix = baseTopic_prefix + reference + "/#";
const char* unique_id_new;
String base_topic = baseTopic_prefix + reference + "/";
const char* state_suffix = "stat_t";
const char* command_suffix = "cmd_t";
const char* availability_suffix = "avt_t";
String discoveryMessage;

byte mac[] = { 0xDE, 0xEF, 0xED, 0xEF, 0xFE, 0xEF };  // Hier die MAC-Adresse deines Ethernet-Shields eintragen
IPAddress server(192, 168, 178, 42);                  // IP-Adresse deines MQTT-Brokers
const int mqtt_port = 1883;
EthernetClient ethClient;
PubSubClient client(ethClient);
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
bool previousStates[numPins];
bool firstRun = true;
int result = 1;   //Result for strength   0 = ok, 1 =NOK
int result1 = 1;  //Result for angel error  0 for OK 1 for NOK
int previousSensorValue = LOW;
const int numAnalogPins = 10;
int currentActuatorState = LOW;  // Declare currentActuatorState globally
unsigned long time_now = 0;
#define NUM_ROLLADEN 2
int analogStates[numAnalogPins];  // Array zum Speichern der vorherigen Zustände der A-Ports

struct Rolladen {
  int pin_up;
  int pin_down;
  unsigned long run_time;
  int current_position;
  String topic;
  String name;
  unsigned long start_time;  // Startzeit für die Bewegung
  int target_position;       // Zielposition für die Bewegung
  bool moving_up;            // Flag, ob der Rolladen nach oben bewegt wird
  bool moving;               // Flag, ob der Rolladen gerade bewegt wird
};
Rolladen rolladen[NUM_ROLLADEN] = {
  // { CONTROLLINO_D9, CONTROLLINO_D10, 15000, 0, "aha/C1/rolladen/sz_tuer", "Rolladen SZ Tuer", 0, 0, false, false },
  //{ CONTROLLINO_D8, CONTROLLINO_D7, 15000, 0, "aha/C1/rolladen/buero_tuer", "Rolladen Buero Tuer", 0, 0, false, false }
};


typedef struct {
  const char* base_topic;
  int pin;
  void (*action)(int, int, const char*);
  const char* name;
  const char* unique_id;
  const char* deviceIdentifier;
  const char* deviceName;
  bool defaultState;
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


void sendDiscoveryMessages() {
  Serial.println("...sending discovery messages...");
  // MQTT-Topic-Basis
  const char* base_topic = "";
  // Rolläden Discovery-Nachrichten
  for (int i = 0; i < NUM_ROLLADEN; i++) {
    // Erstellen der String-Variablen für die Topics
    char state_topic[200];
    char command_topic[200];
    char availability_topic[200];
    char discovery_topic[511];  // Größerer Puffer für Discovery-Topic
    snprintf(state_topic, sizeof(state_topic), "%s%s/stat_t", base_topic, rolladen[i].topic.c_str());
    snprintf(command_topic, sizeof(command_topic), "%s%s/cmd_t", base_topic, rolladen[i].topic.c_str());
    snprintf(availability_topic, sizeof(availability_topic), "%s%s/avt_t", base_topic, rolladen[i].topic.c_str());
    snprintf(discovery_topic, sizeof(discovery_topic), "%s%s/config", base_topic, rolladen[i].topic.c_str());

    // JSON-Nachricht für Rolladen
    char message[511];
    snprintf(message, sizeof(message),
             "{"
             "\"name\": \"%s\", "
             "\"unique_id\": \"C1/%s\", "
             "\"state_topic\": \"%s\", "
             "\"command_topic\": \"%s\", "
             "\"availability_topic\": \"%s\", "
             "\"payload_open\": \"OPEN\", "
             "\"payload_close\": \"CLOSE\", "
             "\"payload_stop\": \"STOP\", "
             "\"position_open\": 100, "
             "\"position_closed\": 0, "
             "\"device\": {"
             "\"identifiers\": \"%s\", "
             "\"name\": \"%s\", "
             "\"model\": \"Rolladen\", "
             "\"manufacturer\": \"DIY\""
             "}"
             "}",
             rolladen[i].name.c_str(),
             rolladen[i].topic.c_str(),
             state_topic,
             command_topic,
             availability_topic,
             rolladen[i].topic.c_str(),
             rolladen[i].name.c_str());

    Serial.println(discovery_topic);
    Serial.println(message);
    bool result = client.publish(discovery_topic, message, true);

    if (result) {
      Serial.println("Message published successfully.");
    } else {
      Serial.println("Failed to publish message.");
    }
  }

  base_topic = "aha/";

  // Geräte Discovery-Nachrichten für andere Geräte
  for (int i = 0; i < sizeof(topicActions) / sizeof(TopicActionMapping); i++) {
    const TopicActionMapping* action = &topicActions[i];
    // Ableitung der Topics
    char state_topic[200];
    char command_topic[200];
    char availability_topic[200];
    char discovery_topic[250];
    char payload[511];

    snprintf(state_topic, sizeof(state_topic), "%s%s/%s", base_topic, action->unique_id, state_suffix);
    snprintf(command_topic, sizeof(command_topic), "%s%s/%s", base_topic, action->unique_id, command_suffix);
    snprintf(availability_topic, sizeof(availability_topic), "%s%s/%s", base_topic, action->unique_id, availability_suffix);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/switch/%s/config", action->unique_id);

    snprintf(payload, sizeof(payload),
             "{"
             "\"name\": \"%s\", "
             "\"state_topic\": \"%s\", "
             "\"command_topic\": \"%s\", "
             "\"availability_topic\": \"%s\", "
             "\"payload_on\": \"ON\", "
             "\"payload_off\": \"OFF\", "
             "\"device_class\": \"switch\", "
             "\"unique_id\": \"%s\", "
             "\"device\": {"
             "\"identifiers\": \"%s\", "
             "\"name\": \"%s\""
             "}"
             "}",
             action->name, state_topic, command_topic, availability_topic, action->unique_id,
             action->deviceIdentifier, action->deviceName);

    Serial.println(discovery_topic);
    Serial.println(payload);
    bool result = client.publish(discovery_topic, payload, true);
    if (result) {
      Serial.println("Message published successfully.");
    } else {
      Serial.println("Failed to publish message.");
    }
  }

  // Geräte Discovery-Nachrichten für A1 bis A10
  for (int i = 1; i <= 10; i++) {
    char state_topic[200];
    char command_topic[200];
    char availability_topic[200];
    char discovery_topic[250];
    char payload[511];

    snprintf(state_topic, sizeof(state_topic), "%sA%d/stat_t", base_topic, i);
    snprintf(command_topic, sizeof(command_topic), "%sA%d/cmd_t", base_topic, i);
    snprintf(availability_topic, sizeof(availability_topic), "%sA%d/avt_t", base_topic, i);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/C1_A%d/config", i);

    snprintf(payload, sizeof(payload),
             "{"
             "\"name\": \"Analog Input A%d\", "
             "\"state_topic\": \"%s\", "
             "\"command_topic\": \"%s\", "
             "\"availability_topic\": \"%s\", "
             "\"unique_id\": \"C1_A%d\", "
             "\"device\": {"
             "\"identifiers\": \"C1_A%d\", "
             "\"name\": \"Analog Port A%d\", "
             "\"model\": \"Analog Sensor\", "
             "\"manufacturer\": \"DIY\""
             "}"
             "}",
             i, state_topic, command_topic, availability_topic, i, i, i);

    Serial.println(discovery_topic);
    Serial.println(payload);
    bool result = client.publish(discovery_topic, payload, true);
    if (result) {
      Serial.println("Message published successfully for A Port.");
    } else {
      Serial.println("Failed to publish message for A Port.");
    }
  }
}

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
