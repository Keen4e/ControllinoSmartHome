#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>

#include "config.hpp"           // Neue Konfiguration
#include "digital_outputs.hpp"  // Digitale Ausgänge
#include "analog_inputs.hpp"    // Analoge Eingänge

#define DEBUG 1

#if DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_TIMESTAMP() do { \
  unsigned long ms = millis(); \
  Serial.print(ms/1000); \
  Serial.print('.'); \
  Serial.print(ms%1000); \
  Serial.print(": "); \
} while (0)
#define DEBUG_MSG(msg) do { DEBUG_TIMESTAMP(); Serial.println(F(msg)); } while (0)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_TIMESTAMP()
#define DEBUG_MSG(msg)
#endif

EthernetClient ethClient;
PubSubClient client(ethClient);

// Globale Variablen
const char* state_suffix = SYSTEM_CONFIG.mqtt.state_suffix;
const char* command_suffix = SYSTEM_CONFIG.mqtt.command_topic;
const char* availability_suffix = SYSTEM_CONFIG.mqtt.availability_topic;
const char* mqtt_base_topic = SYSTEM_CONFIG.mqtt.base_topic;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;  // 5 Sekunden
bool mqttConnectionStatus = false;

// Dynamische Arrays zur Überwachung der Zustände
int* previousInputValues = nullptr;
int* previousOutputValues = nullptr;
bool lastInputStates[ANALOG_INPUTS_COUNT] = { false };
bool outputStates[ANALOG_INPUTS_COUNT] = { false };
const unsigned long DEBOUNCE_DELAY = 25;  // 50ms Entprellzeit
unsigned long pressStartTime[ANALOG_INPUTS_COUNT] = { 0 };  // Speichert den Zeitpunkt, an dem ein Tastendruck beginnt
bool buttonPressInProgress[ANALOG_INPUTS_COUNT] = { false }; // Zeigt an, ob ein Tastendruck gerade läuft
const unsigned long LONG_PRESS_THRESHOLD = 3000; // 3 Sekunden in Millisekunden

// Zusätzliche Array für Entprellung
unsigned long lastDebounceTime[ANALOG_INPUTS_COUNT] = { 0 };
int inputStates[ANALOG_INPUTS_COUNT] = { HIGH };

// Neue globale Variable für den letzten erkannten langen Tastendruck
bool wasLongPress[ANALOG_INPUTS_COUNT] = { false };

struct InputState {
  bool isPressed;
  bool wasLongPress;
  unsigned long pressStartTime;
  unsigned long lastToggleTime;
  unsigned long lastDebounceTime;
};


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
int extractPinFromTopic(const char* topic);
void sendAvailabilityMessages();
bool publishWithRetry(const char* topic, const char* payload, int retries = 3) {
  while (retries > 0) {
    if (client.publish(topic, payload)) return true;
    retries--;
    delay(20);
  }
  if (DEBUG) {
    Serial.print(F("Failed to publish to "));
    Serial.println(topic);
  }
  return false;
}

// Hilfsfunktion zum Konvertieren von Pin-Namen zu Pin-Nummern
// Verbesserte Funktion für das Mapping von Pin-Strings zu Controllino Pins

// Update the setup function to properly initialize outputStates to match physical state
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("***********************************************************");
  Serial.println("                   CONTROLLINO                            ");
  Serial.println("              ....starting up.....");
  Serial.println("***********************************************************");
  // Netzwerk und MQTT initialisieren
  byte mac[6];
  if (sscanf(SYSTEM_CONFIG.network.mac_address,
             "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
             &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
      != 6) {
    DEBUG_PRINTLN("Ungültige MAC-Adresse");
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
  previousInputValues = new int[ANALOG_INPUTS_COUNT];
  previousOutputValues = new int[DIGITAL_OUTPUTS_COUNT];

  // Initialisiere Ausgänge
  Serial.println("Initialisiere digitale Ausgänge:");
  for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
    int pin = getPinFromString(DIGITAL_OUTPUTS[i].pin);
    pinMode(pin, OUTPUT);
    
    // Setze den Ausgang auf den vordefinierten Initialzustand
    bool initialState = DIGITAL_OUTPUTS[i].initial_state;
    digitalWrite(pin, initialState ? HIGH : LOW);
    previousOutputValues[i] = initialState ? HIGH : LOW;
  }

  // Initialisiere Entprellungs-Arrays und Eingänge
  Serial.println("Initialisiere analoge Eingänge und zugehörige Ausgänge:");
 for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
  int inputPin = getPinFromString(ANALOG_INPUTS[i].pin);
  
  // Initialize with consistent state across all tracking variables
  int initialReading = digitalRead(inputPin);
  inputStates[i] = initialReading;
  previousInputValues[i] = initialReading;
  lastDebounceTime[i] = 0;
  buttonPressInProgress[i] = false;
  pressStartTime[i] = 0;
  
  // Log the initial state
  Serial.print("Input ");
  Serial.print(i);
  Serial.print(" (");
  Serial.print(ANALOG_INPUTS[i].name);
  Serial.print(") initialized with state: ");
  Serial.println(initialReading == LOW ? "PRESSED (LOW)" : "RELEASED (HIGH)");
}
  

  // Sende Discovery-Nachrichten
  sendDiscoveryMessages();

  // Sende initiale Statusinformationen
  sendInitialStateMessages();
  applyDefaultOutputStates();
   synchronizeOutputStates();
  // Nach Anwendung der Default-Zustände, aktualisiere die outputStates
  updateOutputStatesFromPins();

  // Debug-Ausgabe
  DEBUG_PRINTLN("Setup abgeschlossen");
}
void synchronizeOutputStates() {
  Serial.println("\n*** Synchronisiere outputStates mit tatsächlichen Pin-Zuständen ***");
  
  // Zuordnung zwischen Eingängen und Ausgängen finden und synchronisieren
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    if (ANALOG_INPUTS[i].outputs_count > 0) {
      // Nimm den ersten Ausgang als Referenz für diesen Eingang
      const char* outputPinStr = ANALOG_INPUTS[i].outputs[0].pin;
      int outputPin = getPinFromString(outputPinStr);
      
      // Lese tatsächlichen Pin-Zustand
      bool actualState = (digitalRead(outputPin) == HIGH);
      
      // Setze den Logik-Zustand auf den physischen Zustand
      outputStates[i] = actualState;
      
      Serial.print("Input ");
      Serial.print(i);
      Serial.print(" (");
      Serial.print(ANALOG_INPUTS[i].name);
      Serial.print(") outputState synchronisiert zu ");
      Serial.println(actualState ? "HIGH (EIN)" : "LOW (AUS)");
    }
  }
  
  Serial.println("*** Synchronisierung abgeschlossen ***\n");
}
void updateOutputStatesFromPins() {
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    if (ANALOG_INPUTS[i].outputs_count > 0) {
      // Nimm den ersten Ausgang als Referenz
      const char* outputPinStr = ANALOG_INPUTS[i].outputs[0].pin;
      int outputPin = getPinFromString(outputPinStr);
      
      // Aktualisiere den Status basierend auf dem tatsächlichen Pin-Zustand
      outputStates[i] = (digitalRead(outputPin) == HIGH);
      
      Serial.print("Input ");
      Serial.print(i);
      Serial.print(" outputState synchronisiert zu ");
      Serial.println(outputStates[i] ? "HIGH" : "LOW");
    }
  }
}


void loop() {
  // MQTT-Verbindung prüfen ohne Blockierung
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Zustände überwachen
  monitorInputs();
  monitorOutputs();
  mapInputsToOutputs();

  delay(50);  // Kurze Verzögerung
}

void reconnect() {
  static uint8_t reconnectCount = 0;
  const uint8_t MAX_RETRIES = 3;
  
  if (!client.connected() && millis() - lastReconnectAttempt >= RECONNECT_INTERVAL) {
    lastReconnectAttempt = millis();
    
    if (++reconnectCount > MAX_RETRIES) {
      // Hardware-Reset nach zu vielen Versuchen
      DEBUG_MSG("Too many reconnection attempts, resetting...");
      delay(1000);
      // Implement watchdog reset here
      return;
    }
    
    if (client.connect(SYSTEM_CONFIG.mqtt.client_id)) {
      reconnectCount = 0;
      mqttConnectionStatus = true;
      client.subscribe((String(mqtt_base_topic) + "/#").c_str());
      sendAvailabilityMessages();
    }
  }
}
void sendAllStates() {
  sendStateMessage(NULL, state_suffix, -1);
  sendAvailabilityMessages();
}
void applyDefaultOutputStates() {
    Serial.println("\n*** Anwenden der konfigurierten Default-Zustände für alle Ausgänge ***");

    for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
        const char* pinName = DIGITAL_OUTPUTS[i].pin;
        int pin = getPinFromString(pinName);
        bool defaultState = DIGITAL_OUTPUTS[i].initial_state;

        Serial.print("Setze Pin ");
        Serial.print(pinName);
        Serial.print(" (");
        Serial.print(pin);
        Serial.print(") auf Default-Zustand: ");
        Serial.println(defaultState ? "HIGH (EIN)" : "LOW (AUS)");

        // Pin als Ausgang konfigurieren und Zustand setzen
        pinMode(pin, OUTPUT);
        digitalWrite(pin, defaultState ? HIGH : LOW);

        // Überprüfe den tatsächlichen Status
        int actualState = digitalRead(pin);
        Serial.print("Tatsächlicher Zustand nach Setzen: ");
        Serial.println(actualState == HIGH ? "HIGH" : "LOW");

        // MQTT-Status-Nachricht senden, um UI zu aktualisieren
        String statePayload = defaultState ? "ON" : "OFF";
        String topicStr = String(DIGITAL_OUTPUTS[i].mqtt_topic);
        int slashIndex = topicStr.indexOf('/');
        String baseTopic = "aha/" + topicStr.substring(0, slashIndex);
        String fullTopicPart = topicStr.substring(slashIndex + 1);
        String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;

        client.publish(stateTopic.c_str(), statePayload.c_str());
        Serial.print("Status-Nachricht gesendet an: ");
        Serial.print(stateTopic);
        Serial.print(" mit Inhalt: ");
        Serial.println(statePayload);

        // Kurze Pause für stabilere Kommunikation
        delay(20);
    }

    // Synchronisiere auch alle outputStates nach dem Setzen der Default-Zustände
    synchronizeOutputStates();

    Serial.println("*** Default-Zustände für alle Ausgänge angewendet ***\n");
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Konvertiere das Payload-Byte-Array in eine Zeichenkette
  String payloadStr = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  Serial.print("\n*** Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(payloadStr);

  // Konstruiere den Control-Topic dynamisch
  String controlTopic = String(mqtt_base_topic) + "/control";

  // Prüfe auf spezifischen Control-Topic
  if (strcmp(topic, controlTopic.c_str()) == 0) {
    if (strcmp(payloadStr.c_str(), "3") == 0) {
      Serial.println("Extern getriggertes Senden aller Statusinformationen");
      sendStateMessage(NULL, state_suffix, -1);
    }
    if (strcmp(payloadStr.c_str(), "2") == 0) {
      Serial.println("Extern getriggertes Senden der Availability Messages");
      sendAvailabilityMessages();
    }
    if (strcmp(payloadStr.c_str(), "1") == 0) {
      Serial.println("Extern getriggertes Senden der Discovery Messages");
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
          Serial.print("Unbekannter Befehl: ");
          Serial.println(payloadStr);
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
struct PinMapping {
  const char* name;
  uint8_t pin;
};

const PinMapping PIN_MAP[] PROGMEM = {
  {"D0", CONTROLLINO_D0},
  {"D1", CONTROLLINO_D1},
  // ... weitere Pins
};

int getPinFromString(const char* pinStr) {
  String pinName = pinStr;
  if (!pinName.startsWith("CONTROLLINO_")) {
    pinName = "CONTROLLINO_" + pinName;
  }
  
  for (const auto& mapping : PIN_MAP) {
    if (pinName.endsWith(mapping.name)) {
      return mapping.pin;
    }
  }
  return -1;
}

// Extrahiert den Pin direkt aus einem MQTT-Topic basierend auf bekannten Mustern
// Extrahiert den Pin direkt aus einem MQTT-Topic für Controllino mit ausführlichem Debug
int extractPinFromTopic(const char* topic) {
  String topicStr = String(topic);
  Serial.print("Extrahiere Pin aus Topic: ");
  Serial.println(topicStr);

  // Topic-Teile in Segmente aufteilen
  int startPos = topicStr.lastIndexOf('/');
  if (startPos == -1) {
    Serial.println("Ungültiges Topic-Format: Kein '/' gefunden");
    return -1;
  }

  // Das vorletzte Segment enthält die Pin-Information (z.B. C2_D11)
  String segment = "";
  int prevSlashPos = topicStr.lastIndexOf('/', startPos - 1);
  if (prevSlashPos != -1) {
    segment = topicStr.substring(prevSlashPos + 1, startPos);
  } else {
    segment = topicStr.substring(0, startPos);
  }

  Serial.print("Extrahiertes Segment: ");
  Serial.println(segment);

  // Nun suchen wir präzise nach 'D1', 'D11', usw. mit Unterstrichen oder Schrägstrichen davor
  for (int d = 0; d <= 23; d++) {
    // Für einstellige Zahlen
    String dPatternSingle = "D" + String(d) + "_";
    String dPatternSingleEnd = "D" + String(d) + "/";
    String dPatternSingleFinal = "D" + String(d);

    // Für zweistellige Zahlen (10-23)
    String dPatternDouble = "D" + String(d) + "_";
    String dPatternDoubleEnd = "D" + String(d) + "/";
    String dPatternDoubleFinal = "D" + String(d);

    // Genaue Muster-Suche
    bool foundPattern = false;

    // Prüfe, ob das Muster am Ende steht
    if (segment.endsWith(dPatternSingleFinal) || segment.endsWith(dPatternDoubleFinal)) {
      foundPattern = true;
    }

    // Prüfe, ob das Muster mit einem Unterstrich oder Schrägstrich gefolgt wird
    int patternPos = segment.indexOf(dPatternSingle);
    if (patternPos != -1 || segment.indexOf(dPatternSingleEnd) != -1) {
      foundPattern = true;
    }

    if (foundPattern) {
      String pinStr = "CONTROLLINO_D" + String(d);
      Serial.print("Pin gefunden: ");
      Serial.println(pinStr);
      return getPinFromString(pinStr.c_str());
    }
  }

  // Ähnliche Prüfung für A-Pins
  for (int a = 0; a <= 15; a++) {
    String aPattern = "_A" + String(a);
    String aPatternEnd = "A" + String(a) + "/";
    String aPatternFinal = "A" + String(a);

    if (segment.endsWith(aPatternFinal) || segment.indexOf(aPattern) != -1 || segment.indexOf(aPatternEnd) != -1) {
      String pinStr = "CONTROLLINO_A" + String(a);
      Serial.print("Pin gefunden: ");
      Serial.println(pinStr);
      return getPinFromString(pinStr.c_str());
    }
  }

  // Ähnliche Prüfung für R-Pins
  for (int r = 0; r <= 15; r++) {
    String rPattern = "_R" + String(r);
    String rPatternEnd = "R" + String(r) + "/";
    String rPatternFinal = "R" + String(r);

    if (segment.endsWith(rPatternFinal) || segment.indexOf(rPattern) != -1 || segment.indexOf(rPatternEnd) != -1) {
      String pinStr = "CONTROLLINO_R" + String(r);
      Serial.print("Pin gefunden: ");
      Serial.println(pinStr);
      return getPinFromString(pinStr.c_str());
    }
  }

  Serial.println("Kein Pin im Topic gefunden");
  return -1;
}
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
      if (currentValue != previousInputValues[i]) {  // Bei digitalen Werten keine Hysterese notwendig
          Serial.print("Value changed for digital input on pin ");
          Serial.print(pin);
          Serial.print(" from ");
          Serial.print(previousInputValues[i]);
          Serial.print(" to ");
          Serial.println(currentValue);
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
      Serial.print("Value changed for digital output on pin ");
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
  static unsigned long lastToggleTime[ANALOG_INPUTS_COUNT] = { 0 }; // Verhindert Prellen beim Umschalten
  const unsigned long TOGGLE_DELAY = 50; // Minimaler Abstand zwischen Schaltvorgängen
  
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    int inputPin = getPinFromString(ANALOG_INPUTS[i].pin);
    int reading = digitalRead(inputPin);

    // Nur verarbeiten wenn sich der Wert geändert hat
    if (reading != previousInputValues[i]) {
      lastDebounceTime[i] = millis();
      
      // Debug output
      if (DEBUG) {
        Serial.print(F("Input ")); // F() Makro spart RAM
        Serial.print(i);
        Serial.print(F(" changed from "));
        Serial.print(previousInputValues[i] == LOW ? F("PRESSED") : F("RELEASED"));
        Serial.print(F(" to "));
        Serial.println(reading == LOW ? F("PRESSED") : F("RELEASED"));
      }
    }

    // Entprellte Zustandsänderung verarbeiten
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      if (reading != inputStates[i]) {
        inputStates[i] = reading;
        
        // Taste wurde gedrückt
        if (reading == LOW) {
          pressStartTime[i] = millis();
          buttonPressInProgress[i] = true;
          wasLongPress[i] = false;
          
          if (DEBUG) {
            Serial.print(F("Button press started: "));
            Serial.println(ANALOG_INPUTS[i].name);
          }
        }
        // Taste wurde losgelassen
        else if (buttonPressInProgress[i]) {
          unsigned long pressDuration = millis() - pressStartTime[i];
          buttonPressInProgress[i] = false;
          
          // Langer Tastendruck erkannt
          if (pressDuration >= LONG_PRESS_THRESHOLD) {
            wasLongPress[i] = true;
            if (DEBUG) Serial.println(F("Long press detected - no action"));
          }
          // Kurzer Tastendruck und kein vorheriger langer Tastendruck
          else if (!wasLongPress[i] && (millis() - lastToggleTime[i] > TOGGLE_DELAY)) {
            outputStates[i] = !outputStates[i];
            lastToggleTime[i] = millis();
            
            if (ANALOG_INPUTS[i].outputs_count > 0) {
              updateOutputsForInput(i);
            }
            if (DEBUG) Serial.println(F("Short press - toggling state"));
          }
        }
      }
    }
    previousInputValues[i] = reading;
  }
}

// Neue Hilfsfunktion zum Aktualisieren der Ausgänge
void updateOutputsForInput(uint8_t inputIndex) {
  for (uint8_t j = 0; j < ANALOG_INPUTS[inputIndex].outputs_count; j++) {
    const char* outputPinStr = ANALOG_INPUTS[inputIndex].outputs[j].pin;
    int outputPin = getPinFromString(outputPinStr);
    
    digitalWrite(outputPin, outputStates[inputIndex] ? HIGH : LOW);
    
    // MQTT Update senden
    for (uint8_t k = 0; k < DIGITAL_OUTPUTS_COUNT; k++) {
      if (strcmp(DIGITAL_OUTPUTS[k].pin, outputPinStr) == 0) {
        String topicStr = String(DIGITAL_OUTPUTS[k].mqtt_topic);
        int slashIndex = topicStr.indexOf('/');
        String baseTopic = "aha/" + topicStr.substring(0, slashIndex);
        String fullTopicPart = topicStr.substring(slashIndex + 1);
        String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;
        
        const char* statePayload = outputStates[inputIndex] ? "ON" : "OFF";
        client.publish(stateTopic.c_str(), statePayload);
        break;
      }
    }
  }
}
void sendStateMessage(const char* mqtt_topic, const char* state_suffix, int value) {
  if (!client.connected()) return;
  
  static char topicBuffer[64];
  snprintf(topicBuffer, sizeof(topicBuffer), "aha/%s/%s", mqtt_topic, state_suffix);
  const char* payload = (value == HIGH) ? "ON" : "OFF";
  
  publishWithRetry(topicBuffer, payload);
}
void sendAvailabilityMessages() {
  // Availability-Nachrichten für digitale Ausgänge senden
  for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
    const DigitalOutput& digitalOutput = DIGITAL_OUTPUTS[i];
    String topicParts = digitalOutput.mqtt_topic;
    int slashIndex = topicParts.indexOf('/');
    String baseTopic = "aha/" + topicParts.substring(0, slashIndex);
    String fullTopicPart = topicParts.substring(slashIndex + 1);
    String availabilityTopic = baseTopic + "/" + fullTopicPart + "/" + availability_suffix;

    bool publishResult = client.publish(availabilityTopic.c_str(), "online");
    // Hier kannst du noch eine Fehlerbehandlung hinzufügen, wenn das Senden fehlschlägt
  }

  // Availability-Nachrichten für analoge Eingänge senden
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    const AnalogInput& analogInput = ANALOG_INPUTS[i];
    String topicParts = analogInput.mqtt_topic;
    int slashIndex = topicParts.indexOf('/');
    String baseTopic = "aha/" + topicParts.substring(0, slashIndex);
    String fullTopicPart = topicParts.substring(slashIndex + 1);
    String availabilityTopic = baseTopic + "/" + fullTopicPart + "/" + availability_suffix;

    bool publishResult = client.publish(availabilityTopic.c_str(), "online");
    // Hier kannst du noch eine Fehlerbehandlung hinzufügen, wenn das Senden fehlschlägt
  }
}
void sendDiscoveryMessages() {
  Serial.println("Sende Discovery-Nachrichten...");
  Serial.print("Anzahl digitaler Ausgänge: ");
  Serial.println(sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]));

  // Discovery-Nachrichten für digitale Ausgänge senden
  for (uint8_t i = 0; i < sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]); i++) {
    const DigitalOutput& digitalOutput = DIGITAL_OUTPUTS[i];

    // Serial.print("Verarbeite digitalen Ausgang: ");
    // Serial.println(digitalOutput.name);

    String topicParts = digitalOutput.mqtt_topic;
    int slashIndex = topicParts.indexOf('/');
    String baseTopic = "aha/" + topicParts.substring(0, slashIndex);
    String fullTopicPart = topicParts.substring(slashIndex + 1);

    String discoveryTopic = "homeassistant/switch/" + baseTopic.substring(4) + "/" + fullTopicPart + "/config";

    Serial.print("Discovery-Topic: ");
     Serial.println(discoveryTopic);

    StaticJsonDocument<712> discoveryDoc;
    discoveryDoc["name"] = digitalOutput.name;

    String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;
    String commandTopic = baseTopic + "/" + fullTopicPart + "/" + command_suffix;
    String availabilityTopic = baseTopic + "/" + fullTopicPart + "/" + availability_suffix;

    discoveryDoc["state_topic"] = stateTopic;
    discoveryDoc["command_topic"] = commandTopic;
    discoveryDoc["availability_topic"] = availabilityTopic;

    discoveryDoc["payload_on"] = "ON";
    discoveryDoc["payload_off"] = "OFF";
    discoveryDoc["device_class"] = "switch";

    discoveryDoc["unique_id"] = fullTopicPart;

    JsonObject device = discoveryDoc.createNestedObject("device");
    device["identifiers"] = digitalOutput.mqtt_device;
    device["name"] = digitalOutput.mqtt_device;

    String discoveryPayload;
    serializeJson(discoveryDoc, discoveryPayload);

     Serial.print("Discovery-Payload: ");
     Serial.println(discoveryPayload);

    if (!client.connected()) {
      Serial.println("MQTT-Client nicht verbunden! Versuche erneut zu verbinden...");
      reconnect();
    }

    bool publishResult = client.publish(discoveryTopic.c_str(), discoveryPayload.c_str());

    if (publishResult) {
      // Serial.print("Discovery-Nachricht erfolgreich gesendet für: ");
      // Serial.println(digitalOutput.name);
    } else {
      Serial.print("FEHLER: Konnte Discovery-Nachricht NICHT senden für: ");
      Serial.println(digitalOutput.name);
      Serial.print("Client-State: ");
      Serial.println(client.state());
    }

    // Initiale Status-Nachricht senden
    bool state = digitalRead(digitalOutput.pin);
    String statePayload = state ? "ON" : "OFF";
    bool statePublishResult = client.publish(stateTopic.c_str(), statePayload.c_str());

    // // Initiale Verfügbarkeitsnachricht senden
    bool availabilityPublishResult = client.publish(availabilityTopic.c_str(), "online");

    client.loop();  // MQTT-Nachrichten verarbeiten
    delay(50);     // Kurze Verzögerung zwischen Nachrichten
  }

  // Discovery-Nachrichten für digitale Eingänge (als AnalogInput deklariert) senden
  Serial.println("Sende Discovery-Nachrichten für digitale Eingänge...");
  Serial.print("Anzahl digitaler Eingänge: ");
  Serial.println(sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]));

  for (uint8_t i = 0; i < sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]); i++) {
    const AnalogInput& analogInput = ANALOG_INPUTS[i];

    // Serial.print("Verarbeite digitalen Eingang: ");
    // Serial.println(analogInput.name);

    String topicParts = analogInput.mqtt_topic;
    int slashIndex = topicParts.indexOf('/');
    String baseTopic = "aha/" + topicParts.substring(0, slashIndex);
    String fullTopicPart = topicParts.substring(slashIndex + 1);

    String discoveryTopic = "homeassistant/binary_sensor/" + baseTopic.substring(4) + "/" + fullTopicPart + "/config";

     Serial.print("Discovery-Topic: ");
     Serial.println(discoveryTopic);

    StaticJsonDocument<512> discoveryDoc;
    discoveryDoc["name"] = analogInput.name;

    String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;
    String availabilityTopic = baseTopic + "/" + fullTopicPart + "/" + availability_suffix;

    discoveryDoc["state_topic"] = stateTopic;
    discoveryDoc["availability_topic"] = availabilityTopic;

    discoveryDoc["payload_on"] = "ON";
    discoveryDoc["payload_off"] = "OFF";
    discoveryDoc["device_class"] = analogInput.device_class;

    discoveryDoc["unique_id"] = fullTopicPart;

    JsonObject device = discoveryDoc.createNestedObject("device");
    device["identifiers"] = analogInput.mqtt_device;
    device["name"] = analogInput.mqtt_device;

    String discoveryPayload;
    serializeJson(discoveryDoc, discoveryPayload);

     Serial.print("Discovery-Payload: ");
     Serial.println(discoveryPayload);

    if (!client.connected()) {
      Serial.println("MQTT-Client nicht verbunden! Versuche erneut zu verbinden...");
      reconnect();
    }

    bool publishResult = client.publish(discoveryTopic.c_str(), discoveryPayload.c_str());

    if (publishResult) {
      // Serial.print("Discovery-Nachricht erfolgreich gesendet für: ");
      // Serial.println(analogInput.name);
    } else {
      Serial.print("FEHLER: Konnte Discovery-Nachricht NICHT senden für: ");
      Serial.println(analogInput.name);
      Serial.print("Client-State: ");
      Serial.println(client.state());
    }

    // Initiale Status-Nachricht senden
    bool state = digitalRead(analogInput.pin);
    String statePayload = state ? "ON" : "OFF";
    bool statePublishResult = client.publish(stateTopic.c_str(), statePayload.c_str());

    // // Initiale Verfügbarkeitsnachricht senden
    bool availabilityPublishResult = client.publish(availabilityTopic.c_str(), "online");

    client.loop();  // MQTT-Nachrichten verarbeiten
    delay(50);     // Kurze Verzögerung zwischen Nachrichten
  }

  Serial.println("Senden der Discovery-Nachrichten abgeschlossen.");
}