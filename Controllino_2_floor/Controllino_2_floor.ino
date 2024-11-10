#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>
#include <UniversalTimer.h>
// MQTT base topic
const char* base_topic = "aha/C2/";
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
byte mac[] = { 0xDE, 0xED, 0xED, 0xEF, 0xFE, 0xEF };       
IPAddress server(192, 168, 178, 42);                       
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
  // { CONTROLLINO_D9, CONTROLLINO_D10, 15000, 0, "aha/c2/rolladen/sz_tuer", "Rolladen SZ Tuer", 0, 0, false, false },
  //{ CONTROLLINO_D8, CONTROLLINO_D7, 15000, 0, "aha/c2/rolladen/buero_tuer", "Rolladen Buero Tuer", 0, 0, false, false }
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

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Controllino2")) {
      Serial.println("connected");
      client.subscribe("aha/C2/#");
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

  if (strcmp(topic, "aha/C2/control") == 0) {
    if (strcmp(payloadStr.c_str(), "2") == 0) {
      // Funktion für avt_init aufrufen
      Serial.print("Extern getriggertes senden der state messages");
      sendAvailabilityMessages();
    }
    if (strcmp(payloadStr.c_str(), "3") == 0) {
      // Funktion für stat_init aufrufen
      Serial.print("Extern getriggertes senden der state messages");
      sendStateMessages();
    }
    if (strcmp(payloadStr.c_str(), "1") == 0) {
      // Funktion für stat_init aufrufen
      Serial.print("Extern getriggertes senden der Discovery messages");
      sendDiscoveryMessages();
    }
    if (strcmp(payloadStr.c_str(), "stat_init") == 0) {
      // Funktion für avt_init aufrufen
      Serial.print("Extern getriggertes senden der state messages");
      sendAvailabilityMessages();
    }
    if (strcmp(payloadStr.c_str(), "stat_init") == 0) {
      // Funktion für stat_init aufrufen
      Serial.print("Extern getriggertes senden der state messages");
      sendStateMessages();
    }
    if (strcmp(payloadStr.c_str(), "discovery_init") == 0) {
      // Funktion für stat_init aufrufen
      Serial.print("Extern getriggertes senden der Discovery messages");
      sendDiscoveryMessages();
    }
    return;  // Frühzeitiger Abbruch, da keine weiteren Aktionen notwendig sind
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
             "\"unique_id\": \"C2/%s\", "
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
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/C2_A%d/config", i);

    snprintf(payload, sizeof(payload),
             "{"
             "\"name\": \"Analog Input A%d\", "
             "\"state_topic\": \"%s\", "
             "\"command_topic\": \"%s\", "
             "\"availability_topic\": \"%s\", "
             "\"unique_id\": \"C2_A%d\", "
             "\"device\": {"
             "\"identifiers\": \"C2_A%d\", "
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
    String state = "online";  // Definiere den Status der Verfügbarkeit
    client.publish(availabilityTopic.c_str(), state.c_str(), true);  // Nachricht senden
  }

  // Verfügbarkeitsnachrichten für digitale Pins (z.B. D0 bis D23 und R0 bis R9)
  for (int i = 0; i < numPins; i++) {
    String topic = String(base_topic);
    if (i < 24) {
      topic += "C2_D" + String(i) + "/avt_t";  // z.B. "C2_D0/avt_t"
    } else {
      topic += "C2_R" + String(i - 24) + "/avt_t";  // z.B. "C2_R0/avt_t"
    }
    String message = "online";
    client.publish(topic.c_str(), message.c_str(), true);
  }

  // Verfügbarkeitsnachrichten für analoge Pins (A1 bis A10)
  for (int i = 1; i <= numAnalogPins; i++) {  // Änderung: Startindex auf 1 setzen
    String topic = String(base_topic) + "C2_A" + String(i) + "/avt_t";  // z.B. "C2_A1/avt_t"
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
        topic += "C2_D" + String(i) + "/stat_t";
      } else {  // Übrige Pins sind R0-R9
        topic += "C2_R" + String(i - 24) + "/stat_t";
      }
      String message = currentState ? "ON" : "OFF";
      client.publish(topic.c_str(), message.c_str());
      previousStates[i] = currentState;
    }
  }

  // Statusnachrichten für analoge Pins (A1 bis A10)
  for (int i = 1; i <= numAnalogPins; i++) {  // Ändere den Index auf 1, um A1 bis A10 zu verwenden
    int currentAnalogValue = analogRead(i);  // Liest den analogen Wert von Pin A1 bis A10
    if (currentAnalogValue != analogStates[i - 1] || firstRun) {  // Benutze (i - 1) für Array-Index
      String topic = String(base_topic) + "C2_A" + String(i) + "/stat_t";  // Z.B. "C2_A1/stat_t"
      String message = String(currentAnalogValue);  // Konvertiere den Wert in einen String
      client.publish(topic.c_str(), message.c_str());  // Sende die Nachricht über MQTT

      analogStates[i - 1] = currentAnalogValue;  // Aktualisiere den gespeicherten Zustand
    }
  }
}

int previousButtonStates[16] = {0};  // Array zum Speichern der vorherigen Zustände aller analogen Buttons

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
                break;
            case 1:
                handleButtonPress(buttonPin, CONTROLLINO_D5, "aha/C2/C2_D5/stat_t");
                break;
            case 2:
                handleButtonPress(buttonPin, CONTROLLINO_D4, "aha/C2/C2_D4/stat_t");
                break;
            case 3:
                handleButtonPress(buttonPin, CONTROLLINO_D7, "aha/C2/C2_D7/stat_t");
                break;
            case 4:
                handleButtonPress(buttonPin, CONTROLLINO_D8, "aha/C2/C2_D8/stat_t");
                break;
            case 5:
                handleButtonPress(buttonPin, CONTROLLINO_D21, "aha/C2/C2_D21/stat_t");
                handleButtonPress(buttonPin, CONTROLLINO_R1, "aha/C2/C2_R1/stat_t");
                break;
            case 6:
                handleButtonPress(buttonPin, CONTROLLINO_D21, "aha/C2/C2_D21/stat_t");
                handleButtonPress(buttonPin, CONTROLLINO_R1, "aha/C2/C2_R1/stat_t");
                break;
            case 7:
                handleButtonPress(buttonPin, CONTROLLINO_D20, "aha/C2/C2_D20/stat_t");
                break;
            case 8:
                handleButtonPress(buttonPin, CONTROLLINO_D12, "aha/C2/C2_D12/stat_t");
                break;
            case 9:
                handleButtonPress(buttonPin, CONTROLLINO_D11, "aha/C2/C2_D11/stat_t");
                break;
            case 10:
                handleButtonPress(buttonPin, CONTROLLINO_D12, "aha/C2/C2_D12/stat_t");
                break;
            case 11:
                handleButtonPress(buttonPin, CONTROLLINO_D10, "aha/C2/C2_D10/stat_t");
                break;
            case 12:
                handleButtonPress(buttonPin, CONTROLLINO_D1, "aha/C2/C2_D1/stat_t");
                break;
        }
    }
}
void handleDigitalStatus(int buttonPin, int pinIndex) {
    // Array zum Speichern der vorherigen Zustände der digitalen Pins
    static int previousDigitalValues[16] = {LOW};  

    int currentState = digitalRead(buttonPin);  // Aktuellen digitalen Zustand lesen

    // Prüfen, ob sich der digitale Wert geändert hat
    if (currentState != previousDigitalValues[pinIndex]) {
        // MQTT-Topic für den aktuellen Pin erstellen (z.B., "aha/C2/A0/stat_t")
        String topic = "aha/C2/C2_A" + String(pinIndex) + "/stat_t";
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
