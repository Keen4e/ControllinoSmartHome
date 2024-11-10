#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>
#include <UniversalTimer.h>

const char* reference = "C1";  // Dynamische Referenz, z.B. "C1" oder "C2"
const char* unique_id_new;
// MQTT base topic
const char* base_topic = "aha/C1/";
const char* state_suffix = "stat_t";
const char* command_suffix = "cmd_t";
const char* availability_suffix = "avt_t";
String discoveryMessage;
unsigned long lastReconnectAttempt = 0;
unsigned long lastAvailabilityMessage = 0;
unsigned long previousMillis = 0;
const unsigned long interval = 2 * 60 * 1000;              
const unsigned long reconnectInterval = 10 * 60 * 1000;    
const unsigned long availabilityInterval = 1 * 60 * 1000;  
byte mac[] = { 0xDE, 0xEF, 0xED, 0xEF, 0xFE, 0xEF };       // Hier die MAC-Adresse deines Ethernet-Shields eintragen
IPAddress server(192, 168, 178, 42);                       // IP-Adresse deines MQTT-Brokers
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
  { "aha/C1/C1_D2", CONTROLLINO_D2, digitalWriteAndPublish, "Schrank Fussbodenheizung", "C1/C1_D2", "C1_Schrank", "C1_Schrank" , false },
  { "aha/C1/C1_D3", CONTROLLINO_D3, digitalWriteAndPublish, "Schlafzimmer Deckenlicht", "C1/C1_D3", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D4", CONTROLLINO_D4, digitalWriteAndPublish, "Schlafzimmer Licht Bett Links", "C1/C1_D4", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D5", CONTROLLINO_D5, digitalWriteAndPublish, "Schlafzimmer Licht Bett Rechts", "C1/C1_D5", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D6", CONTROLLINO_D6, digitalWriteAndPublish, "Schlafzimmer Fussbodenheizung", "C1/C1_D6", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D7", CONTROLLINO_D7, digitalWriteAndPublish, "Buero Rolladen Runter", "C1/C1_D7", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D8", CONTROLLINO_D8, digitalWriteAndPublish, "Buero Rolladen Hoch", "C1/C1_D8", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D9", CONTROLLINO_D9, digitalWriteAndPublish, "Schlafzimmer Rollo Tuer hoch", "C1/C1_D9", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D10", CONTROLLINO_D10, digitalWriteAndPublish, "Schlafzimmer Rollo Tuer runter", "C1/C1_D10", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D11", CONTROLLINO_D11, digitalWriteAndPublish, "Buero Stehlampe", "C1/C1_D11", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D12", CONTROLLINO_D12, digitalWriteAndPublish, "Buero Deckenlicht", "C1/C1_D12", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D13", CONTROLLINO_D13, digitalWriteAndPublish, "Buero Arbeitsplatz", "C1/C1_D13", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D14", CONTROLLINO_D14, digitalWriteAndPublish, "Buero Fussbodenheizung", "C1/C1_D14", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D15", CONTROLLINO_D15, digitalWriteAndPublish, "Buero Raumstrom", "C1/C1_D15", "C1_Buero", "C1_Buero" , true },
  { "aha/C1/C1_D16", CONTROLLINO_D16, digitalWriteAndPublish, "Buero Heizung Schreibtisch", "C1/C1_D16", "C1_Buero", "C1_Buero" , false },
  { "aha/C1/C1_D17", CONTROLLINO_D17, digitalWriteAndPublish, "Schlafzimmer Rollo runter", "C1/C1_D17", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D18", CONTROLLINO_D18, digitalWriteAndPublish, "Schlafzimmer Rolle hoch", "C1/C1_D18", "C1_Schlafzimmer", "C1_Schlafzimmer" , false },
  { "aha/C1/C1_D19", CONTROLLINO_D19, digitalWriteAndPublish, "Bad Licht", "C1/C1_D19", "C1_Bad", "C1_Bad", false },
  { "aha/C1/C1_D21", CONTROLLINO_D21, digitalWriteAndPublish, "Bad Dusche Licht", "C1/C1_D21", "C1_Bad", "C1_Bad" , false },
  { "aha/C1/C1_D23", CONTROLLINO_D23, digitalWriteAndPublish, "Bad Fussbodenheizung", "C1/C1_D23", "C1_Bad", "C1_Bad" , false },
  { "aha/C1/C1_R1", CONTROLLINO_R1, digitalWriteAndPublish, "Schrank Deckenlicht", "C1/C1_R1", "C1_Schrank", "C1_Schrank" , false },
  { "aha/C1/C1_R3", CONTROLLINO_R3, digitalWriteAndPublish, "Bad Schublade", "C1/C1_R3", "C1_Bad", "C1_Bad" , false },
  { "aha/C1/C1_R5", CONTROLLINO_R5, digitalWriteAndPublish, "Schrank Pumpe", "C1/C1_R5", "C1_Schrank", "C1_Schrank" , false },
  { "aha/C1/C1_R8", CONTROLLINO_R8, digitalWriteAndPublish, "Flur Licht", "C1/C1_R8", "C1_Flur", "C1_Flur" , false },
  { "aha/C1/C1_R9", CONTROLLINO_R9, digitalWriteAndPublish, "Flur Fussbodenheizung", "C1/C1_R9", "C1_Flur", "C1_Flur" , false },
  { "aha/C1/C1_R11", CONTROLLINO_R11, digitalWriteAndPublish, "Flur Licht", "C1/C1_R11", "C1_Flur", "C1_Flur" , false },
  { "aha/C1/C1_R12", CONTROLLINO_R12, digitalWriteAndPublish, "Bad Raumstrom", "C1/C1_R12", "C1_Bad", "C1_Bad", true },
  { "aha/C1/C1_R14", CONTROLLINO_R14, digitalWriteAndPublish, "Schlafzimmer Raumstrom", "C1/C1_R14", "C1_Schlafzimmer", "C1_Schlafzimmer" , true },
  { "aha/C1/C1_R15", CONTROLLINO_R15, digitalWriteAndPublish, "Schrank Raumstrom", "C1/C1_R15", "C1_Schrank", "C1_Schrank", true },
  //{ "aha/C1/C1_R16", CONTROLLINO_R16, digitalWriteAndPublish, "Schrank Steckdose", "C1/C1_R16", "C1_Schrank", "C1_Schrank" },
  //{ "aha/C1/C1_R17", CONTROLLINO_R17, digitalWriteAndPublish, "Schrank Steckdose", "C1/C1_R17", "C1_Schrank", "C1_Schrank" },

  { "aha/C1/C1_A0", CONTROLLINO_A0, digitalWriteAndPublish, "A0 - Schrank re", "C1/C1_A0", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A1", CONTROLLINO_A1, digitalWriteAndPublish, "A1 - Schrank li", "C1/C1_A1", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A2", CONTROLLINO_A2, digitalWriteAndPublish, "A2 - Bad Spiegel", "C1/C1_A2", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A3", CONTROLLINO_A3, digitalWriteAndPublish, "A3 - Schlafzimmer Deckenlicht", "C1/C1_A3", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A4", CONTROLLINO_A4, digitalWriteAndPublish, "A4 - Schlafzimmer li", "C1/C1_A4", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A5", CONTROLLINO_A5, digitalWriteAndPublish, "A5 - Schlafzimmer re", "C1/C1_A5", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A6", CONTROLLINO_A6, digitalWriteAndPublish, "A6 - Bad", "C1/C1_A6", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A7", CONTROLLINO_A7, digitalWriteAndPublish, "A7 - Rolladen rechts hoch", "C1/C1_A7", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A8", CONTROLLINO_A8, digitalWriteAndPublish, "A8 - Rolladen rechts runter", "C1/C1_A8", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A9", CONTROLLINO_A9, digitalWriteAndPublish, "A9 - Rolladen links hoch", "C1/C1_A9", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A10", CONTROLLINO_A10, digitalWriteAndPublish, "A10 - Rolladen links runter", "C1/C1_A10", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A11", CONTROLLINO_A11, digitalWriteAndPublish, "A11 - Buero Licht", "C1/C1_A11", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A14", CONTROLLINO_A14, digitalWriteAndPublish, "A14 - Buero Rolladen hoch", "C1/C1_A14", "C1_Eingänge", "C1_Eingänge" , false },
  { "aha/C1/C1_A15", CONTROLLINO_A15, digitalWriteAndPublish, "A15 - Buero Rolladen runter", "C1/C1_A15", "C1_Eingänge", "C1_Eingänge" , false }






};
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Controllino1")) {
      Serial.println("connected");
      client.subscribe("aha/C1/#");
      // Serial.println("Sending Discovery...");
      // sendDiscoveryMessages();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void setPosition(int index, int position) {
  if (position < 0) position = 0;
  if (position > 100) position = 100;

  Rolladen& r = rolladen[index];
  int current_position = r.current_position;
  int differenz = position - current_position;

  if (differenz > 0) {
    // Rolladen nach unten fahren
    digitalWrite(r.pin_up, LOW);
    digitalWrite(r.pin_down, HIGH);
    delay(r.run_time * differenz / 100);
    digitalWrite(r.pin_down, LOW);
  } else if (differenz < 0) {
    // Rolladen nach oben fahren
    digitalWrite(r.pin_down, LOW);
    digitalWrite(r.pin_up, HIGH);
    delay(r.run_time * (-differenz) / 100);
    digitalWrite(r.pin_up, LOW);
  }

  r.current_position = position;
}

void callback(char* topic, byte* payload, unsigned int length) {
  time_now = millis();
  Serial.print(topic);
  String payloadStr = "";
  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  Serial.print(" -> ");
  Serial.print(payloadStr);
  Serial.println();

  String controlTopic = String("aha/") + reference + "/control";
    if (String(topic) == controlTopic) {
        if (payloadStr == "2") {
            Serial.print("Extern getriggertes Senden der Availability Messages");
            sendAvailabilityMessages();
        }
        if (payloadStr == "3") {
            Serial.print("Extern getriggertes Senden der State Messages");
            sendStateMessages();
        }
        if (payloadStr == "1") {
            Serial.print("Extern getriggertes Senden der Discovery Messages");
            sendDiscoveryMessages();
        }
        if (payloadStr == "stat_init") {
            Serial.print("Extern getriggertes Senden der State Messages");
            sendAvailabilityMessages();
            sendStateMessages();
        }
        if (payloadStr == "discovery_init") {
            Serial.print("Extern getriggertes Senden der Discovery Messages");
            sendDiscoveryMessages();
        }
        return;  // Exit early as no further actions are needed
    }

  // Überprüfe die anderen Topics
  for (int i = 0; i < sizeof(topicActions) / sizeof(topicActions[0]); i++) {
    // Verwende die Arduino String-Klasse für die Verkettung
    String fullTopic = String(topicActions[i].base_topic) + "/cmd_t";
    if (String(topic) == fullTopic) {
      if (strcmp(payloadStr.c_str(), "ON") == 0) {
        topicActions[i].action(topicActions[i].pin, true, topicActions[i].base_topic);

      } else if (strcmp(payloadStr.c_str(), "OFF") == 0) {
        topicActions[i].action(topicActions[i].pin, false, topicActions[i].base_topic);
      }
      break;
    }
  }
  // Convert payload to a String
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  for (int i = 0; i < NUM_ROLLADEN; i++) {
    if (String(topic) == rolladen[i].topic + "/command") {
      int target_position = message.toInt();
      if (target_position >= 0 && target_position <= 100) {
        setPosition(i, target_position);
      }
      break;
    }
  }
}
void updateRolladen(int index) {
  if (rolladen[index].moving) {
    unsigned long elapsed_time = millis() - rolladen[index].start_time;
    unsigned long move_duration = rolladen[index].run_time * abs(rolladen[index].target_position - rolladen[index].current_position) / 100;

    if (elapsed_time >= move_duration) {
      // Bewegung abgeschlossen
      digitalWrite(rolladen[index].pin_up, LOW);
      digitalWrite(rolladen[index].pin_down, LOW);
      rolladen[index].current_position = rolladen[index].target_position;
      rolladen[index].moving = false;
    } else {
      // Bewegung fortsetzen
      if (rolladen[index].moving_up) {
        digitalWrite(rolladen[index].pin_down, LOW);
        digitalWrite(rolladen[index].pin_up, HIGH);
      } else {
        digitalWrite(rolladen[index].pin_up, LOW);
        digitalWrite(rolladen[index].pin_down, HIGH);
      }
    }
  }
}

void moveRolladenUp(int index, bool initial = false) {
  if (!rolladen[index].moving) {
    rolladen[index].moving_up = true;
    rolladen[index].moving = true;
    rolladen[index].start_time = millis();
    rolladen[index].target_position = initial ? 0 : rolladen[index].current_position;
  }
}

void moveRolladenDown(int index) {
  if (!rolladen[index].moving) {
    rolladen[index].moving_up = false;
    rolladen[index].moving = true;
    rolladen[index].start_time = millis();
    rolladen[index].target_position = 100;
  }
}

void moveRolladenTo(int index, int target_position) {
  if (!rolladen[index].moving && target_position != rolladen[index].current_position) {
    rolladen[index].moving_up = target_position > rolladen[index].current_position;
    rolladen[index].moving = true;
    rolladen[index].start_time = millis();
    rolladen[index].target_position = target_position;
  }
}
void setup() {
  Serial.begin(9600);
  Serial.println("Ready for setup");
  Ethernet.begin(mac);
  client.setServer(server, 1883);
  client.setCallback(callback);
  client.setBufferSize(500);
  // Hardware-Pins initialisieren
  DDRD |= B01110000;  // Setze die Ports PD4, PD5, PD6 als Ausgänge
  DDRJ |= B00010000;  // Setze den Port PJ4 als Ausgang
  for (int i = 0; i <= 23; i++) {
    pinMode(CONTROLLINO_D0 + i, OUTPUT);
  }
  for (int i = 0; i <= 15; i++) {
    pinMode(CONTROLLINO_R0 + i, OUTPUT);
  }
  // Analogpins als INPUT_PULLUP setzen
  for (int i = 0; i <= 15; i++) {
    pinMode(CONTROLLINO_A0 + i, INPUT_PULLUP);
  }
  Serial.println("Ports initiated");
  // Setze Standardwerte für einige Ports
  delay(150);

   for (int i = 0; i < sizeof(topicActions) / sizeof(topicActions[0]); i++) {
    const TopicActionMapping* action = &topicActions[i];
    pinMode(action->pin, OUTPUT); 
    digitalWrite(action->pin, action->defaultState ? HIGH : LOW); // Aktivieren oder Deaktivieren basierend auf defaultState
    Serial.print("Setting pin ");
    Serial.print(action->pin);
    Serial.println(action->defaultState ? " HIGH" : " LOW");
  }

  Serial.println("Default ports started");
  // Rolläden initial hochfahren
  for (int i = 0; i < NUM_ROLLADEN; i++) {
    //  moveRolladenUp(i, true);
  }
  Serial.println("Cover initiated");
  // MQTT-Verbindung versuchen
}
void sendDiscoveryMessages() {
  Serial.println("...sending discovery messages...");
  // MQTT-Topic-Basis
  
 String base_topic = String("aha/") + reference;
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
             "\"unique_id\": \"+ reference + /%s\", "
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

  

  // Geräte Discovery-Nachrichten für andere Geräte
  for (int i = 0; i < sizeof(topicActions) / sizeof(TopicActionMapping); i++) {
    const TopicActionMapping* action = &topicActions[i];
    // Ableitung der Topics
    char state_topic[200];
    char command_topic[200];
    char availability_topic[200];
    char discovery_topic[250];
    char payload[511];
 String unique_id_new = reference + action->unique_id;
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

bool publishWithRetry(const char* topic, const char* payload, int maxRetries = 5) {
  int retryCount = 0;
  while (retryCount < maxRetries) {
    if (client.publish(topic, payload)) {
      return true;  // Erfolg
    }
    retryCount++;
    Serial.print("Failed to publish to topic: ");
    Serial.println(topic);
    Serial.print("MQTT Error: ");
    Serial.println(client.state());  // Ausgabe des MQTT-Fehlercodes
    delay(500);                      // Wartezeit zwischen den Versuchen
  }
  return false;  // Fehler nach maxRetries
}
void publishState(int index) {
  String stateTopic = rolladen[index].topic + "/stat_t";
  String stateMessage = String(rolladen[index].current_position);
  client.publish(stateTopic.c_str(), stateMessage.c_str(), true);
}
void sendAvailabilityMessages() {
  // Verfügbarkeitsnachrichten für Rolläden
  for (int i = 0; i < NUM_ROLLADEN; i++) {
    String availabilityTopic = rolladen[i].topic + "/avt_t";
    String state = "online";                                         // Definiere den Status der Verfügbarkeit
    client.publish(availabilityTopic.c_str(), state.c_str(), true);  // Nachricht senden
  }

  // Verfügbarkeitsnachrichten für digitale Pins (z.B. D0 bis D23 und R0 bis R9)
  for (int i = 0; i < numPins; i++) {
    String topic = String(base_topic);
    if (i < 24) {
      topic += "C1_D" + String(i) + "/avt_t";  // z.B. "C1_D0/avt_t"
    } else {
      topic += "C1_R" + String(i - 24) + "/avt_t";  // z.B. "C1_R0/avt_t"
    }
    String message = "online";
    client.publish(topic.c_str(), message.c_str(), true);
  }

  // Verfügbarkeitsnachrichten für analoge Pins (A1 bis A10)
  for (int i = 1; i <= numAnalogPins; i++) {                            // Änderung: Startindex auf 1 setzen
    String topic = String(base_topic) + "C1_A" + String(i) + "/avt_t";  // z.B. "C1_A1/avt_t"
    String message = "online";
    client.publish(topic.c_str(), message.c_str(), true);  // Nachricht senden
  }
}
void sendStateMessages() {
  // Rolladen-Statusnachrichten
  for (int i = 0; i < NUM_ROLLADEN; i++) {
    Rolladen& r = rolladen[i];  // Referenz auf das aktuelle Rolladen-Objekt

    // Bestimme den aktuellen Zustand des Rolladens
    String message;
    if (r.moving) {
      message = r.moving_up ? "MOVING_UP" : "MOVING_DOWN";
    } else {
      if (r.current_position == 0) {
        message = "CLOSED";
      } else if (r.current_position == 100) {
        message = "OPEN";
      } else {
        message = "PARTIALLY_OPEN";
      }
    }

    // Veröffentliche den Status des Rolladens
    String statTopic = r.topic + "/stat_t";
    client.publish(statTopic.c_str(), message.c_str());
  }

  // Statusnachrichten für digitale Pins (z.B. D0 bis D23 und R0 bis R9)
  for (int i = 0; i < numPins; i++) {
    bool currentState = digitalRead(pins[i]);
    if (currentState != previousStates[i] || firstRun) {
      String topic = String(base_topic);
      if (i < 24) {  // Erste 24 Pins sind D0-D23
        topic += "C1_D" + String(i) + "/stat_t";
      } else {  // Übrige Pins sind R0-R9
        topic += "C1_R" + String(i - 24) + "/stat_t";
      }
      String message = currentState ? "ON" : "OFF";
      client.publish(topic.c_str(), message.c_str());
      previousStates[i] = currentState;
    }
  }

  // Statusnachrichten für analoge Pins (A1 bis A10)
  for (int i = 1; i <= numAnalogPins; i++) {                               // Ändere den Index auf 1, um A1 bis A10 zu verwenden
    int currentAnalogValue = digitalRead(i);                                // Liest den analogen Wert von Pin A1 bis A10
    if (currentAnalogValue != analogStates[i - 1] || firstRun) {           // Benutze (i - 1) für Array-Index
      String topic = String(base_topic) + "C1_A" + String(i) + "/stat_t";  // Z.B. "C1_A1/stat_t"
      String message = String(currentAnalogValue);                         // Konvertiere den Wert in einen String
      client.publish(topic.c_str(), message.c_str());                      // Sende die Nachricht über MQTT

      analogStates[i - 1] = currentAnalogValue;  // Aktualisiere den gespeicherten Zustand
    }
  }
}

int previousButtonStates[16] = { 0 };  // Array zum Speichern der vorherigen Zustände aller analogen Buttons

void handleButtonPress(int buttonPin, int actuatorPin, const char* topic) {
  int sensorValue = digitalRead(buttonPin);

  // Prüfen, ob sich der Status geändert hat
  if (sensorValue != previousSensorValue) {
    Serial.println();
    Serial.print("Input received A");
    Serial.print(sensorValue);

    // Wenn der Knopf gedrückt wird
    if (sensorValue == HIGH) {
      // Betätige den Aktor und sende Statusnachricht
      handlePress(actuatorPin, topic);

      // Warten, bis der Knopf losgelassen wird
      while (digitalRead(buttonPin) == HIGH) {
        delay(50);
      }
    }
  }

  // Update des vorherigen Sensorwerts
  previousSensorValue = sensorValue;
}

void handlePress(int actuatorPin, const char* topic) {
  currentActuatorState = digitalRead(actuatorPin);  // Aktuellen Zustand des Aktors lesen

  // MQTT Nachricht senden mit "ON" oder "OFF" basierend auf aktuellem Zustand
  client.publish(topic, (currentActuatorState == LOW) ? "ON" : "OFF");

  // Aktor umschalten (LOW -> HIGH oder HIGH -> LOW)
  digitalWrite(actuatorPin, (currentActuatorState == HIGH) ? LOW : HIGH);

  // Konsolenausgabe für Debugging
  Serial.print("Button Pressed -> ");
  Serial.print(actuatorPin);
  Serial.println((currentActuatorState == LOW) ? " LOW" : " HIGH");
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
      case 2:
        handleButtonPress(buttonPin, CONTROLLINO_D19, "aha/C1/C1_D19/stat_t");
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
      case 6:
        handleButtonPress(buttonPin, CONTROLLINO_R8, "aha/C1/C1_R8/stat_t");
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
void handleDigitalStatus(int buttonPin, int pinIndex) {
  // Array zum Speichern der vorherigen Zustände der digitalen Pins
  static int previousDigitalValues[16] = { LOW };

  int currentState = digitalRead(buttonPin);  // Aktuellen digitalen Zustand lesen

  // Prüfen, ob sich der digitale Wert geändert hat
  if (currentState != previousDigitalValues[pinIndex]) {
    // MQTT-Topic für den aktuellen Pin erstellen (z.B., "aha/C1/A0/stat_t")
    String topic = "aha/C1/C1_A" + String(pinIndex) + "/stat_t";
    String message = (currentState == HIGH) ? "ON" : "OFF";  // Zustand in String umwandeln

    // MQTT-Nachricht veröffentlichen
    client.publish(topic.c_str(), message.c_str());
    Serial.print("Digital status change detected on A");
    Serial.print(pinIndex);
    Serial.print(" -> ");
    Serial.println(message);

    // Aktuellen Zustand als vorherigen speichern
    previousDigitalValues[pinIndex] = currentState;
  }
}

void checkPinStates() {

  for (int i = 0; i < NUM_ROLLADEN; i++) {
    Rolladen& r = rolladen[i];  // Referenz auf das aktuelle Rolladen-Objekt
    // Bestimme den aktuellen Zustand des Rolladens
    String message;
    if (r.moving) {
      message = r.moving_up ? "MOVING_UP" : "MOVING_DOWN";
    } else {
      if (r.current_position == 0) {
        message = "CLOSED";
      } else if (r.current_position == 100) {
        message = "OPEN";
      } else {
        message = "PARTIALLY_OPEN";
      }
    }

    // Veröffentliche den Status des Rolladens
    String statTopic = r.topic + "/stat_t";
    client.publish(statTopic.c_str(), message.c_str());
  }

  for (int i = 0; i < numPins; i++) {
    bool currentState = digitalRead(pins[i]);
    if (currentState != previousStates[i] || firstRun) {
      String topic = String(base_topic);
      if (i < 24) {  // First 24 pins are D0-D23
        topic += "CONTROLLINO_D" + String(i) + "/stat_t";
      } else {  // Remaining pins are R0-R17
        topic += "CONTROLLINO_R" + String(i - 24) + "/stat_t";
      }
      String message = currentState ? "ON" : "OFF";
      client.publish(topic.c_str(), message.c_str());
      previousStates[i] = currentState;
    }
  }
  if (firstRun) {
    firstRun = false;
  }
}


void loop() {
  Ethernet.maintain();
  client.loop();
  readAnalogValues();
  if (!client.connected()) {
    Serial.print("...lost connections... ");
    reconnect();
  }
}
