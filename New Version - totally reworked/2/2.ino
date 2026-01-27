//19062025
#define MQTT_MAX_PACKET_SIZE 512
#define MQTT_KEEPALIVE 60

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>

// Ensure proper character encoding
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1

#include "config.hpp"           // Neue Konfiguration
#include "digital_outputs.hpp"  // Digitale Ausgänge
#include "analog_inputs.hpp"    // Analoge Eingänge

#define DEBUG 1

#if DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

EthernetClient ethClient;
PubSubClient client(ethClient);

// Am Anfang der Datei, nach den includes
// Reduzierte und optimierte Buffer-Größen
#define DISCOVERY_TOPIC_SIZE 128    // war 160
#define DISCOVERY_PAYLOAD_SIZE 384  // war 768
#define JSON_DOC_SIZE 256          // war 512
#define BASE_PART_SIZE 32          // war 64
#define FULL_PART_SIZE 64          // war 128

// Statische Buffer statt Stack-Allocation
static char discoveryTopic[DISCOVERY_TOPIC_SIZE];
static char stateTopic[DISCOVERY_TOPIC_SIZE];
static char commandTopic[DISCOVERY_TOPIC_SIZE];
static char availabilityTopic[DISCOVERY_TOPIC_SIZE];
static char basePart[BASE_PART_SIZE];
static char fullPart[FULL_PART_SIZE];
static char discoveryPayloadBuf[DISCOVERY_PAYLOAD_SIZE];
static StaticJsonDocument<JSON_DOC_SIZE> doc;


// Globale Variablen
const char* state_suffix = SYSTEM_CONFIG.mqtt.state_suffix;
const char* command_suffix = SYSTEM_CONFIG.mqtt.command_topic;
const char* availability_suffix = SYSTEM_CONFIG.mqtt.availability_topic;
const char* mqtt_base_topic = SYSTEM_CONFIG.mqtt.base_topic;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;  // 5 Sekunden
bool mqttConnectionStatus = false;

// Statische Arrays (verhindert Heap‑Fragmentierung)
static int previousInputValues[ANALOG_INPUTS_COUNT];
static int previousOutputValues[DIGITAL_OUTPUTS_COUNT];
bool lastInputStates[ANALOG_INPUTS_COUNT] = { false };
bool outputStates[ANALOG_INPUTS_COUNT] = { false };
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms Entprellzeit

// Zusätzliche Array für Entprellung
unsigned long lastDebounceTime[ANALOG_INPUTS_COUNT] = { 0 };
int inputStates[ANALOG_INPUTS_COUNT] = { HIGH };

// Funktionsdeklarationen
void reconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void monitorInputs();
void monitorOutputs();
void mapInputsToOutputs();
void sendStateMessage(const char* mqtt_topic, const char* state_suffix, int value);
void sendInitialStateMessages();
void sendDiscoveryMessages();
int getPinFromString(const char* pinName);

void sendAvailabilityMessages();
bool publishWithRetry(const char* topic, const char* payload, bool retained = false);  // Neue Deklaration
void hexdump(const char* data);  // Neue Deklaration

// Hilfsfunktion zum Konvertieren von Pin-Namen zu Pin-Nummern
// Verbesserte Funktion für das Mapping von Pin-Strings zu Controllino Pins

void setup() {
  Serial.begin(115200);
  delay(10);
  

Serial.println("*                   .... 18 12 2025.....                *");
  // Netzwerk und MQTT initialisieren
  byte mac[6];
  if (sscanf(SYSTEM_CONFIG.network.mac_address,
             "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
             &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
      != 6) {
    return;
  }

  IPAddress ip, subnet, gateway, dns;
  ip.fromString(SYSTEM_CONFIG.network.ip_address);
  subnet.fromString(SYSTEM_CONFIG.network.subnet_mask);
  gateway.fromString(SYSTEM_CONFIG.network.gateway);
  dns.fromString(SYSTEM_CONFIG.network.dns_server);

  Ethernet.begin(mac, ip, dns, gateway, subnet);

  // MQTT-Client konfigurieren
  client.setServer(
    SYSTEM_CONFIG.mqtt.broker,
    SYSTEM_CONFIG.mqtt.port);
  client.setCallback(mqttCallback);
  client.setBufferSize(512);

  // Warte auf MQTT-Verbindung
  while (!client.connected()) {
    reconnect();
  }

  // Initialisiere globale Zustandsvariablen
  // previousInputValues / previousOutputValues sind statisch deklariert;
  // keine new[]-Zuweisung nötig (und verursacht Compilerfehler).

  // Initialisiere Ausgänge
  for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
    int pin = getPinFromString(DIGITAL_OUTPUTS[i].pin);
    pinMode(pin, OUTPUT);
    // Explizit alle Ausgänge auf LOW setzen
    digitalWrite(pin, LOW);
    previousOutputValues[i] = LOW;
  }

  // Initialisiere Entprellungs-Arrays und Eingänge
  Serial.println("Initialisiere analoge Eingänge und zugehörige Ausgänge:");
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    int inputPin = getPinFromString(ANALOG_INPUTS[i].pin);
    pinMode(inputPin, INPUT_PULLUP);

    // Initialisiere Zustandsvariablen für Toggle-Logik
    lastInputStates[i] = false;
    outputStates[i] = false;  // Stelle sicher, dass dies false (AUS) ist

    // Lese den Anfangswert, um ein sofortiges Umschalten zu verhindern
    int initialReading = digitalRead(inputPin);
    inputStates[i] = initialReading;
    previousInputValues[i] = initialReading;

    // Stelle sicher, dass alle Ausgänge für diesen Eingang auf LOW initialisiert sind
    if (ANALOG_INPUTS[i].outputs_count > 0) {
      for (uint8_t j = 0; j < ANALOG_INPUTS[i].outputs_count; j++) {
        const char* outputPinStr = ANALOG_INPUTS[i].outputs[j].pin;
        int outputPin = getPinFromString(outputPinStr);

        // Als OUTPUT setzen und sicherstellen, dass er LOW ist
        pinMode(outputPin, OUTPUT);
        digitalWrite(outputPin, LOW);
      }
    }
  }

  // Sende Discovery-Nachrichten
  sendDiscoveryMessages();

  // Sende initiale Statusinformationen
  sendInitialStateMessages();
applyDefaultOutputStates();
  // Debug-Ausgabe
  DEBUG_PRINTLN("Setup abgeschlossen");
}
void loop() {
    // MQTT-Verbindung prüfen ohne Blockierung
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Zustände überwachen
    // monitorInputs(); // Diese Zeile entfernen
    monitorOutputs();
    mapInputsToOutputs(); // Diese Funktion übernimmt jetzt die komplette Eingangsprüfung
}

void reconnect() {
  unsigned long currentMillis = millis();

  // Nur alle 5 Sekunden versuchen zu reconnecten
  if (currentMillis - lastReconnectAttempt >= RECONNECT_INTERVAL) {
    lastReconnectAttempt = currentMillis;

    // Client-ID aus der Konfiguration verwenden
    if (client.connect(SYSTEM_CONFIG.mqtt.client_id)) {
      mqttConnectionStatus = true;
      DEBUG_PRINTLN("MQTT-Verbindung wiederhergestellt");

      String subscriptionTopic = String(mqtt_base_topic) + "/#";
      client.subscribe(subscriptionTopic.c_str());
    } else {
      mqttConnectionStatus = false;
      DEBUG_PRINT("MQTT-Verbindung fehlgeschlagen, RC=");
      DEBUG_PRINTLN(client.state());
    }
  }
}
void sendAllStates() {
  sendStateMessage(NULL, state_suffix, -1);
  sendAvailabilityMessages();
}
void applyDefaultOutputStates() {
    for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
        const char* pinName = DIGITAL_OUTPUTS[i].pin;
        int pin = getPinFromString(pinName);
        bool defaultState = DIGITAL_OUTPUTS[i].initial_state;
        // Pin als Ausgang konfigurieren und Zustand setzen
        pinMode(pin, OUTPUT);
        digitalWrite(pin, defaultState ? HIGH : LOW);

        // Überprüfe den tatsächlichen Status
        int actualState = digitalRead(pin);

        // MQTT-Status-Nachricht senden, um UI zu aktualisieren
        String statePayload = defaultState ? "ON" : "OFF";
        String topicStr = String(DIGITAL_OUTPUTS[i].mqtt_topic);
        int slashIndex = topicStr.indexOf('/');
        String baseTopic = "aha/" + topicStr.substring(0, slashIndex);
        String fullTopicPart = topicStr.substring(slashIndex + 1);
        String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;

        client.publish(stateTopic.c_str(), statePayload.c_str());
        // Kurze Pause für stabilere Kommunikation
        delay(5);
    }

}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Konvertiere das Payload-Byte-Array in eine Zeichenkette
  String payloadStr = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  Serial.print("Message: [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(payloadStr);

  // Konstruiere den Control-Topic dynamisch
  String controlTopic = String(mqtt_base_topic) + "/control";

  // Prüfe auf spezifischen Control-Topic
  if (strcmp(topic, controlTopic.c_str()) == 0) {
    if (strcmp(payloadStr.c_str(), "3") == 0) {
      sendStateMessage(NULL, state_suffix, -1);
    }
    if (strcmp(payloadStr.c_str(), "2") == 0) {
      sendAvailabilityMessages();
    }
    if (strcmp(payloadStr.c_str(), "1") == 0) {
      sendDiscoveryMessages();
    }
    return;
  }

  // Prüfen, ob dies ein Command-Topic für einen unserer Ausgänge ist
  String topicStr = String(topic);
  if (topicStr.endsWith(command_suffix)) {
    // Dies ist ein Command-Topic, nun finden wir heraus, für welchen Ausgang
    for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
      String outputTopicBase = String(DIGITAL_OUTPUTS[i].mqtt_topic);
      int slashIndex = outputTopicBase.indexOf('/');
      String baseTopic = "aha/" + outputTopicBase.substring(0, slashIndex);
      String fullTopicPart = outputTopicBase.substring(slashIndex + 1);
      String expectedCommandTopic = baseTopic + "/" + fullTopicPart + "/" + command_suffix;
      
      if (topicStr.equals(expectedCommandTopic)) {
        // Passenden Ausgang gefunden!
        int pin = getPinFromString(DIGITAL_OUTPUTS[i].pin);
        
        // Befehl verarbeiten (ON/OFF)
        bool newState = false;
        if (payloadStr.equals("ON")) {
          newState = true;
        } else if (payloadStr.equals("OFF")) {
          newState = false;
        } else {
          // Unbekannter Befehl

          return;
        }
        
        // Ausgang setzen
        digitalWrite(pin, newState ? HIGH : LOW);
        
        // Status-Update senden
        sendStateMessage(
          DIGITAL_OUTPUTS[i].mqtt_topic,
          state_suffix,
          newState ? HIGH : LOW
        );
        
        Serial.print("Ausgangs-Pin ");
        Serial.print(pin);
        Serial.print(" auf ");
        Serial.println(newState ? "HIGH" : "LOW");
        
        return;
      }
    }
    
    // Wenn wir hier ankommen, wurde kein passender Ausgang gefunden
    Serial.print("Befehl für unbekannten Ausgang empfangen: ");
    Serial.println(topic);
  }
}
int getPinFromString(const char* pinStr) {
  String pinString = String(pinStr);
  // Prüfen, ob der String bereits mit "CONTROLLINO_" beginnt
  if (pinString.startsWith("CONTROLLINO_")) {
    // Direktes Mapping verwenden
  } else {
    // Prefix "CONTROLLINO_" hinzufügen
    pinString = "CONTROLLINO_" + pinString;
  }
  // Mapping für Controllino Digital Pins
  if (pinString == "CONTROLLINO_D0") return CONTROLLINO_D0;
  if (pinString == "CONTROLLINO_D1") return CONTROLLINO_D1;
  if (pinString == "CONTROLLINO_D2") return CONTROLLINO_D2;
  if (pinString == "CONTROLLINO_D3") return CONTROLLINO_D3;
  if (pinString == "CONTROLLINO_D4") return CONTROLLINO_D4;
  if (pinString == "CONTROLLINO_D5") return CONTROLLINO_D5;
  if (pinString == "CONTROLLINO_D6") return CONTROLLINO_D6;
  if (pinString == "CONTROLLINO_D7") return CONTROLLINO_D7;
  if (pinString == "CONTROLLINO_D8") return CONTROLLINO_D8;
  if (pinString == "CONTROLLINO_D9") return CONTROLLINO_D9;
  if (pinString == "CONTROLLINO_D10") return CONTROLLINO_D10;
  if (pinString == "CONTROLLINO_D11") return CONTROLLINO_D11;
  if (pinString == "CONTROLLINO_D12") return CONTROLLINO_D12;
  if (pinString == "CONTROLLINO_D13") return CONTROLLINO_D13;
  if (pinString == "CONTROLLINO_D14") return CONTROLLINO_D14;
  if (pinString == "CONTROLLINO_D15") return CONTROLLINO_D15;
  if (pinString == "CONTROLLINO_D16") return CONTROLLINO_D16;
  if (pinString == "CONTROLLINO_D17") return CONTROLLINO_D17;
  if (pinString == "CONTROLLINO_D18") return CONTROLLINO_D18;
  if (pinString == "CONTROLLINO_D19") return CONTROLLINO_D19;
  if (pinString == "CONTROLLINO_D20") return CONTROLLINO_D20;
  if (pinString == "CONTROLLINO_D21") return CONTROLLINO_D21;
  if (pinString == "CONTROLLINO_D22") return CONTROLLINO_D22;
  if (pinString == "CONTROLLINO_D23") return CONTROLLINO_D23;

  // Mapping für Controllino Analog Pins
  if (pinString == "CONTROLLINO_A0") return CONTROLLINO_A0;
  if (pinString == "CONTROLLINO_A1") return CONTROLLINO_A1;
  if (pinString == "CONTROLLINO_A2") return CONTROLLINO_A2;
  if (pinString == "CONTROLLINO_A3") return CONTROLLINO_A3;
  if (pinString == "CONTROLLINO_A4") return CONTROLLINO_A4;
  if (pinString == "CONTROLLINO_A5") return CONTROLLINO_A5;
  if (pinString == "CONTROLLINO_A6") return CONTROLLINO_A6;
  if (pinString == "CONTROLLINO_A7") return CONTROLLINO_A7;
  if (pinString == "CONTROLLINO_A8") return CONTROLLINO_A8;
  if (pinString == "CONTROLLINO_A9") return CONTROLLINO_A9;
  if (pinString == "CONTROLLINO_A10") return CONTROLLINO_A10;
  if (pinString == "CONTROLLINO_A11") return CONTROLLINO_A11;
  if (pinString == "CONTROLLINO_A12") return CONTROLLINO_A12;
  if (pinString == "CONTROLLINO_A13") return CONTROLLINO_A13;
  if (pinString == "CONTROLLINO_A14") return CONTROLLINO_A14;
  if (pinString == "CONTROLLINO_A15") return CONTROLLINO_A15;

  // Mapping für Relay Pins (falls vorhanden)
  if (pinString == "CONTROLLINO_R0") return CONTROLLINO_R0;
  if (pinString == "CONTROLLINO_R1") return CONTROLLINO_R1;
  if (pinString == "CONTROLLINO_R2") return CONTROLLINO_R2;
  if (pinString == "CONTROLLINO_R3") return CONTROLLINO_R3;
  if (pinString == "CONTROLLINO_R4") return CONTROLLINO_R4;
  if (pinString == "CONTROLLINO_R5") return CONTROLLINO_R5;
  if (pinString == "CONTROLLINO_R6") return CONTROLLINO_R6;
  if (pinString == "CONTROLLINO_R7") return CONTROLLINO_R7;
  if (pinString == "CONTROLLINO_R8") return CONTROLLINO_R8;
  if (pinString == "CONTROLLINO_R9") return CONTROLLINO_R9;
  if (pinString == "CONTROLLINO_R10") return CONTROLLINO_R10;
  if (pinString == "CONTROLLINO_R11") return CONTROLLINO_R11;
  if (pinString == "CONTROLLINO_R12") return CONTROLLINO_R12;
  if (pinString == "CONTROLLINO_R13") return CONTROLLINO_R13;
  if (pinString == "CONTROLLINO_R14") return CONTROLLINO_R14;
  if (pinString == "CONTROLLINO_R15") return CONTROLLINO_R15;
  // Pin nicht gefunden

  return -1;
}

// Extrahiert den Pin direkt aus einem MQTT-Topic basierend auf bekannten Mustern
// Extrahiert den Pin direkt aus einem MQTT-Topic für Controllino mit ausführlichem Debug

void sendInitialStateMessages() {
  //Sende Statusinachrichten für alle digitalen Ausgänge
  for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
      const DigitalOutput& digitalOutput = DIGITAL_OUTPUTS[i];
      bool state = digitalRead(getPinFromString(digitalOutput.pin));

      sendStateMessage(
          digitalOutput.mqtt_topic,
          state_suffix,
          state
      );
  }

  // Sende Statusinachrichten für alle Eingänge
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
      const AnalogInput& analogInput = ANALOG_INPUTS[i];
      bool state = digitalRead(getPinFromString(analogInput.pin));

      sendStateMessage(
          analogInput.mqtt_topic,
          state_suffix,
          state
      );
  }

  // Optional: Sende Verfügbarkeitsnachrichten
  sendAvailabilityMessages();
}
void monitorInputs() {
    for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
        int pin = getPinFromString(ANALOG_INPUTS[i].pin);
        int currentValue = digitalRead(pin);

        // Wenn sich der Wert geändert hat, sende eine Status-MQTT-Nachricht
        if (currentValue != previousInputValues[i]) {
            Serial.print(F("Eingang "));
            Serial.print(ANALOG_INPUTS[i].name);
            Serial.print(F(" (Pin "));
            Serial.print(pin);
            Serial.print(F(") änderte sich von "));
            Serial.print(previousInputValues[i] ? "HIGH" : "LOW");
            Serial.print(F(" zu "));
            Serial.println(currentValue ? "HIGH" : "LOW");

            previousInputValues[i] = currentValue;
            sendStateMessage(
                ANALOG_INPUTS[i].mqtt_topic,
                state_suffix,
                currentValue
            );
        }
    }
}

void monitorOutputs() {
  for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
    int pin = getPinFromString(DIGITAL_OUTPUTS[i].pin);
    int currentValue = digitalRead(pin);



    // Wenn sich der Wert geändert hat, sende eine Status-MQTT-Nachricht
    if (currentValue != previousOutputValues[i]) {
      Serial.print(pin);
      Serial.print(" from ");
      Serial.print(previousOutputValues[i]);
      Serial.print(" to ");
      Serial.println(currentValue);
      previousOutputValues[i] = currentValue;
      sendStateMessage(
        DIGITAL_OUTPUTS[i].mqtt_topic,
        state_suffix,
        currentValue);
    }
  }
}

void mapInputsToOutputs() {
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    int reading = digitalRead(getPinFromString(ANALOG_INPUTS[i].pin));

    // Wenn eine Zustandsänderung erkannt wird (Prellen beginnt), starte den Timer.
    if (reading != lastInputStates[i]) {
      lastDebounceTime[i] = millis();
    }

    // Wenn der Timer abgelaufen ist, hat sich der Zustand stabilisiert.
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      // Prüfe, ob der neue, stabile Zustand sich vom alten stabilen Zustand unterscheidet.
      if (reading != inputStates[i]) {
        // Aktualisiere den stabilen Zustand.
        inputStates[i] = reading;

        // Sende immer den neuen Zustand des Eingangs per MQTT.
        sendStateMessage(
          ANALOG_INPUTS[i].mqtt_topic,
          state_suffix,
          reading
        );

        // Reagiere nur auf die fallende Flanke (Tastendruck) für die lokale Aktion.
        if (reading == LOW) {
          // Führe die lokale Verknüpfung nur aus, wenn MQTT offline ist.

            if (ANALOG_INPUTS[i].outputs_count > 0) {
              for (uint8_t j = 0; j < ANALOG_INPUTS[i].outputs_count; j++) {
                const char* outputPinStr = ANALOG_INPUTS[i].outputs[j].pin;
                int outputPin = getPinFromString(outputPinStr);

                // Toggle Ausgangszustand
                bool currentState = digitalRead(outputPin);
                digitalWrite(outputPin, !currentState);

                // Finde den passenden Ausgang und sende seinen neuen Zustand
                for (uint8_t k = 0; k < DIGITAL_OUTPUTS_COUNT; k++) {
                  if (strcmp(DIGITAL_OUTPUTS[k].pin, outputPinStr) == 0) {
                     Serial.print(F("Eingang "));
                     Serial.print(ANALOG_INPUTS[i].name);
                     Serial.print(F(" (Pin "));
                     Serial.print(outputPinStr);
                     Serial.print(F(") änderte sich von "));
                     Serial.print(previousInputValues[i] ? "HIGH" : "LOW");
                     Serial.print(F(" zu "));
                     Serial.println(currentState ? "HIGH" : "LOW");  // Geändert: currentValue -> currentState
                    sendStateMessage(
                      DIGITAL_OUTPUTS[k].mqtt_topic,
                      state_suffix,
                      !currentState
                    );
                    break;
                  }
                }
              }
            }
          //}
        }
      }
    }
    // Aktualisiere den Zustand des letzten Lesevorgangs.
    lastInputStates[i] = reading;
  }
}


void sendStateMessage(const char* mqtt_topic, const char* state_suffix, int value) {
    if (!mqtt_topic) {
        // ... bestehender Code für NULL mqtt_topic ...
        return;
    }

    static char stateTopic[128];  // Statischer Buffer
    const char* slash = strchr(mqtt_topic, '/');
    if (!slash) return;

    size_t baseLen = slash - mqtt_topic;
    if (baseLen >= sizeof(basePart)) baseLen = sizeof(basePart) - 1;

    memcpy(basePart, mqtt_topic, baseLen);
    basePart[baseLen] = '\0';
    
    snprintf(stateTopic, sizeof(stateTopic), 
            "aha/%s/%s/%s", 
            basePart, 
            slash + 1, 
            state_suffix);

    publishWithRetry(stateTopic, value == HIGH ? "ON" : "OFF");
}
void sendAvailabilityMessages() {
  char availabilityTopic[128];
  for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
    const char* topicParts = DIGITAL_OUTPUTS[i].mqtt_topic;
    const char* slash = strchr(topicParts, '/');
    if (!slash) continue;
    int baseLen = slash - topicParts;
    char base[48];
    char full[64];
    if (baseLen >= (int)sizeof(base)) baseLen = sizeof(base)-1;
    memcpy(base, topicParts, baseLen); base[baseLen] = '\0';
    strncpy(full, slash + 1, sizeof(full)-1); full[sizeof(full)-1] = '\0';
    snprintf(availabilityTopic, sizeof(availabilityTopic), "aha/%s/%s/%s", base, full, availability_suffix);
    client.publish(availabilityTopic, "online");
  }

  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    const char* topicParts = ANALOG_INPUTS[i].mqtt_topic;
    const char* slash = strchr(topicParts, '/');
    if (!slash) continue;
    int baseLen = slash - topicParts;
    char base[48];
    char full[64];
    if (baseLen >= (int)sizeof(base)) baseLen = sizeof(base)-1;
    memcpy(base, topicParts, baseLen); base[baseLen] = '\0';
    strncpy(full, slash + 1, sizeof(full)-1); full[sizeof(full)-1] = '\0';
    snprintf(availabilityTopic, sizeof(availabilityTopic), "aha/%s/%s/%s", base, full, availability_suffix);
    client.publish(availabilityTopic, "online");
  }
}
void sendDiscoveryMessages() {
    Serial.println(F("Sende Discovery-Nachrichten..."));
    
    // Discovery für digitale Ausgänge
    for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
        const DigitalOutput& digitalOutput = DIGITAL_OUTPUTS[i];
        const char* topicParts = digitalOutput.mqtt_topic;
        
        // Validiere Topic
        if (!topicParts || !strchr(topicParts, '/')) continue;
        
        // Parse und sanitize Topic-Teile
        size_t baseLen = strchr(topicParts, '/') - topicParts;
        if (baseLen >= BASE_PART_SIZE) baseLen = BASE_PART_SIZE - 1;
        
        memset(basePart, 0, BASE_PART_SIZE);
        memset(fullPart, 0, FULL_PART_SIZE);
        
        memcpy(basePart, topicParts, baseLen);
        strncpy(fullPart, strchr(topicParts, '/') + 1, FULL_PART_SIZE - 1);
        
        // Sanitize strings
        sanitizeString(basePart, strlen(basePart));
        sanitizeString(fullPart, strlen(fullPart));
        
        // Bestimme device_class basierend auf device_class Parameter
        const char* discoveryType = "switch";  // Default
        const char* deviceClass = "switch";    // Default
        
        if (strcmp(digitalOutput.device_class, "light") == 0) {
            discoveryType = "light";
            deviceClass = "light";
        } else if (strcmp(digitalOutput.device_class, "switch") == 0) {
            discoveryType = "switch";
            deviceClass = "switch";
        }
        
        // Build discovery topic mit dynamischem Typ
        snprintf(discoveryTopic, DISCOVERY_TOPIC_SIZE,
                "homeassistant/%s/%s/%s/config", discoveryType, basePart, fullPart);
                
        snprintf(stateTopic, sizeof(stateTopic),
                 "aha/%s/%s/%s", basePart, fullPart, state_suffix);
        snprintf(commandTopic, sizeof(commandTopic),
                 "aha/%s/%s/%s", basePart, fullPart, command_suffix);
        snprintf(availabilityTopic, sizeof(availabilityTopic),
                 "aha/%s/%s/%s", basePart, fullPart, availability_suffix);

        // Reuse static doc
        doc.clear();
        doc["name"] = digitalOutput.name;
        doc["stat_t"] = stateTopic;
        doc["cmd_t"] = commandTopic;
        doc["avty_t"] = availabilityTopic;
        doc["pl_on"] = "ON";
        doc["pl_off"] = "OFF";
        doc["dev_cla"] = deviceClass;  // Verwende dynamischen device_class
        doc["uniq_id"] = fullPart;

        JsonObject device = doc.createNestedObject("device");
        device["ids"] = digitalOutput.mqtt_device;
        device["name"] = digitalOutput.mqtt_device;

        size_t len = serializeJson(doc, discoveryPayloadBuf, sizeof(discoveryPayloadBuf));
        if (len == 0) {
          Serial.println(F("Fehler: Discovery-Payload zu groß oder serializeJson fehlgeschlagen"));
          continue;
        }

        Serial.print(F("Discovery-Topic: "));
        Serial.println(discoveryTopic);
        Serial.print(F("Discovery-Payload: "));
        Serial.println(discoveryPayloadBuf);

        if (!client.connected()) reconnect();
        bool published = client.publish(discoveryTopic, discoveryPayloadBuf, true);
        if (!published) {
          Serial.print(F("FEHLER: Konnte Discovery nicht senden für: "));
          Serial.println(digitalOutput.name);
          Serial.print(F("Client-State: "));
          Serial.println(client.state());
        }

        bool state = digitalRead(getPinFromString(digitalOutput.pin));
        client.publish(stateTopic, state ? "ON" : "OFF", true);
        client.publish(availabilityTopic, "online", true);

        client.loop();
        delay(8);
    }

    // Discovery für Eingänge (binary_sensor)
    for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
        const AnalogInput& analogInput = ANALOG_INPUTS[i];
        const char* topicParts = analogInput.mqtt_topic;
        printTopicDebug(topicParts);
        const char* slashPos = strchr(topicParts, '/');
        if (!slashPos) continue;
        int baseLen = slashPos - topicParts;
        if (baseLen >= (int)sizeof(basePart)) baseLen = sizeof(basePart) - 1;
        memcpy(basePart, topicParts, baseLen);
        basePart[baseLen] = '\0';

        // Debug
        Serial.print(F("mqtt_topic addr: "));
        Serial.println((uintptr_t)topicParts, HEX);
        Serial.print(F("mqtt_topic: "));
        Serial.println(topicParts);
        hexdump(topicParts);

        char fullPart[128];
        size_t fullLen = strlen(slashPos + 1);
        if (fullLen >= sizeof(fullPart)) fullLen = sizeof(fullPart) - 1;
        memcpy(fullPart, slashPos + 1, fullLen);
        fullPart[fullLen] = '\0';
        for (size_t k = 0; k < fullLen; ++k) {
          unsigned char c = (unsigned char)fullPart[k];
          if (!((c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '_' || c == '-')) {
            fullPart[k] = 'A';
          }
        }

        snprintf(discoveryTopic, sizeof(discoveryTopic),
                 "homeassistant/binary_sensor/%s/%s/config", basePart, fullPart);
        snprintf(stateTopic, sizeof(stateTopic),
                 "aha/%s/%s/%s", basePart, fullPart, state_suffix);
        snprintf(availabilityTopic, sizeof(availabilityTopic),
                 "aha/%s/%s/%s", basePart, fullPart, availability_suffix);

        doc.clear();
        doc["name"] = encodeUTF8(analogInput.name);  // Encode special characters
        doc["stat_t"] = stateTopic;
        doc["avty_t"] = availabilityTopic;
        doc["pl_on"] = "ON";
        doc["pl_off"] = "OFF";
        doc["dev_cla"] = analogInput.device_class;
        doc["uniq_id"] = fullPart;

        JsonObject device = doc.createNestedObject("device");
        device["ids"] = encodeUTF8(analogInput.mqtt_device);
        device["name"] = encodeUTF8(analogInput.mqtt_device);

        size_t len = serializeJson(doc, discoveryPayloadBuf, sizeof(discoveryPayloadBuf));
        if (len == 0) {
          Serial.println(F("Fehler: Discovery-Payload zu groß oder serializeJson fehlgeschlagen"));
          continue;
        }

        Serial.print(F("Discovery-Topic: "));
        Serial.println(discoveryTopic);
        Serial.print(F("Discovery-Payload: "));
        Serial.println(discoveryPayloadBuf);

        if (!client.connected()) reconnect();
        bool published = client.publish(discoveryTopic, discoveryPayloadBuf, true);
        if (!published) {
          Serial.print(F("FEHLER: Konnte Discovery nicht senden für: "));
          Serial.println(analogInput.name);
          Serial.print(F("Client-State: "));
          Serial.println(client.state());
        }

        bool state = digitalRead(getPinFromString(analogInput.pin));
        client.publish(stateTopic, state ? "ON" : "OFF", true);
        client.publish(availabilityTopic, "online", true);

        client.loop();
        delay(8);
    }

    Serial.println(F("Senden der Discovery-Nachrichten abgeschlossen."));
}

int freeMemory() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

static void printTopicDebug(const char* topic) {
  Serial.print(F("mqtt_topic addr: "));
  Serial.println((uintptr_t)topic, HEX);
  if (!topic) { Serial.println(F("NULL mqtt_topic")); return; }
  size_t len = strlen(topic);
  Serial.print(F("mqtt_topic: "));
  Serial.println(topic);
}

// Neue Hilfsfunktion für String-Sanitization
static void sanitizeString(char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) {
            str[i] = 'D';  // Ersetzt ungültige Zeichen durch 'D'
        }
    }
}

bool publishWithRetry(const char* topic, const char* payload, bool retained) {
    if (!client.connected()) {
        reconnect();  // Stellt sicher, dass wir verbunden sind
    }
    
    for (uint8_t retry = 0; retry < 3; retry++) {
        if (client.publish(topic, payload, retained)) {
            return true;
        }
        delay(50);
        client.loop();
        
        // Bei Verbindungsverlust erneut verbinden
        if (!client.connected()) {
            reconnect();
        }
    }
    return false;
}

// Implementierung der hexdump Funktion
void hexdump(const char* data) {
    if (!data) {
        Serial.println(F("NULL data pointer"));
        return;
    }
    
    size_t len = strlen(data);
    Serial.print(F("Length: ")); 
    Serial.println(len);
    Serial.print(F("Hex: "));
    
    for (size_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        
        // Konvertiere Byte zu Hex
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", (unsigned char)data[i]);
        Serial.print(hex);
    }
    Serial.println();
}

// Nach den anderen Hilfsfunktionen wie sanitizeString, vor sendDiscoveryMessages
static String encodeUTF8(const char* str) {
    if (!str) return String();
    
    String result;
    while (*str) {
        unsigned char c = (unsigned char)*str++;
        
        switch(c) {
            case 0xC4: // Ä
                result += "Ae";
                break;
            case 0xE4: // ä
                result += "ae";
                break;
            case 0xD6: // Ö
                result += "Oe";
                break;
            case 0xF6: // ö
                result += "oe";
                break;
            case 0xDC: // Ü
                result += "Ue";
                break;
            case 0xFC: // ü
                result += "ue";
                break;
            case 0xDF: // ß
                result += "ss";
                break;
            default:
                if (c < 0x80) { // ASCII-Zeichen
                    result += (char)c;
                } else {
                    result += '_'; // Ersetze unbekannte UTF-8 Zeichen
                }
        }
    }
    return result;
}

