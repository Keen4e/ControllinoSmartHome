
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

#include <PubSubClient.h>
#include <Controllino.h>
#include <ArduinoJson.h>

#include "config.hpp"             // Neue Konfiguration
#include "digital_outputs.hpp"    // Digitale Ausgänge
#include "analog_inputs.hpp"      // Analoge Eingänge

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

// Globale Variablen
const char* state_suffix = SYSTEM_CONFIG.mqtt.state_suffix;
const char* command_suffix = SYSTEM_CONFIG.mqtt.command_topic;
const char* availability_suffix = SYSTEM_CONFIG.mqtt.availability_topic;
const char* mqtt_base_topic = SYSTEM_CONFIG.mqtt.base_topic;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5 Sekunden
bool mqttConnectionStatus = false;

// Dynamische Arrays zur Überwachung der Zustände
int* previousInputValues = nullptr;
int* previousOutputValues = nullptr;
bool lastInputStates[ANALOG_INPUTS_COUNT] = {false};
bool outputStates[ANALOG_INPUTS_COUNT] = {false};
const unsigned long DEBOUNCE_DELAY = 25;  // 50ms Entprellzeit
unsigned long pressStartTime[ANALOG_INPUTS_COUNT] = { 0 };  // Speichert den Zeitpunkt, an dem ein Tastendruck beginnt
bool buttonPressInProgress[ANALOG_INPUTS_COUNT] = { false }; // Zeigt an, ob ein Tastendruck gerade läuft
const unsigned long LONG_PRESS_THRESHOLD = 3000; // 3 Sekunden in Millisekunden

// Zusätzliche Array für Entprellung
unsigned long lastDebounceTime[ANALOG_INPUTS_COUNT] = {0};
int inputStates[ANALOG_INPUTS_COUNT] = {HIGH};

// Funktionsdeklarationen
void reconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void monitorInputs();
void monitorOutputs();
void mapInputsToOutputs();
void sendStateMessage(const char* mqtt_topic, const char* state_suffix, int value);

void sendDiscoveryMessages();
int getPinFromString(const char* pinName);
int extractPinFromTopic(const char* topic);


// Hilfsfunktion zum Konvertieren von Pin-Namen zu Pin-Nummern
// Verbesserte Funktion für das Mapping von Pin-Strings zu Controllino Pins
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
    Serial.println("WARNING: Pin not found in mapping table:");
    Serial.print(pinString);
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
void setup() {
    Serial.begin(115200);

    // Netzwerk und MQTT initialisieren
    byte mac[6];
    if (sscanf(SYSTEM_CONFIG.network.mac_address, 
               "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
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
        SYSTEM_CONFIG.mqtt.port
    );
    client.setCallback(mqttCallback);
    client.setBufferSize(750);

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
        
        Serial.print("Initialisiere Pin ");
        Serial.print(DIGITAL_OUTPUTS[i].pin);
        Serial.print(" (");
        Serial.print(pin);
        Serial.print(") mit initialem Zustand: ");
        Serial.println(DIGITAL_OUTPUTS[i].initial_state ? "HIGH (EIN)" : "LOW (AUS)");
        
        digitalWrite(pin, DIGITAL_OUTPUTS[i].initial_state ? HIGH : LOW);
        
        // Überprüfe, ob der Pin tatsächlich den gewünschten Zustand angenommen hat
        int actualState = digitalRead(pin);
        Serial.print("Tatsächlicher Pin-Zustand nach Initialisierung: ");
        Serial.println(actualState == HIGH ? "HIGH" : "LOW");
    }
    

    // Initialisiere Entprellungs-Arrays und Eingänge
    Serial.println("Initialisiere analoge Eingänge und zugehörige Ausgänge:");
    for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
        // Eingänge als INPUT_PULLUP konfigurieren
        int inputPin = getPinFromString(ANALOG_INPUTS[i].pin);
        pinMode(inputPin, INPUT_PULLUP);
        
        Serial.print("Konfiguriere Eingang ");
        Serial.print(ANALOG_INPUTS[i].pin);
        Serial.print(" (");
        Serial.print(inputPin);
        Serial.println(") als INPUT_PULLUP");
        
        // Vorherigen Eingangswert speichern
        previousInputValues[i] = HIGH;
        
        // Initialisiere Zustandsvariablen für Toggle-Logik
        lastInputStates[i] = false;
        outputStates[i] = false;
        
        // Ausgänge für diesen Eingang konfigurieren
        if (ANALOG_INPUTS[i].outputs_count > 0) {
            Serial.print("Eingang ");
            Serial.print(ANALOG_INPUTS[i].pin);
            Serial.print(" hat ");
            Serial.print(ANALOG_INPUTS[i].outputs_count);
            Serial.println(" zugeordnete Ausgänge:");
            
            for (uint8_t j = 0; j < ANALOG_INPUTS[i].outputs_count; j++) {
                const char* outputPinStr = ANALOG_INPUTS[i].outputs[j].pin;
                int outputPin = getPinFromString(outputPinStr);
                
                // Ausgänge als OUTPUT konfigurieren
                pinMode(outputPin, OUTPUT);
                
                // NICHT einfach auf LOW setzen, sondern den konfigurierten Default-Wert verwenden
                // Suche den konfigurierten Default-Wert für diesen Ausgang
                bool defaultState = false; // Standard-Fallback
                for (uint8_t k = 0; k < DIGITAL_OUTPUTS_COUNT; k++) {
                    if (strcmp(DIGITAL_OUTPUTS[k].pin, outputPinStr) == 0) {
                        defaultState = DIGITAL_OUTPUTS[k].initial_state;
                        Serial.print("  Initialisiere Ausgang ");
                        Serial.print(outputPinStr);
                        Serial.print(" (");
                        Serial.print(outputPin);
                        Serial.print(") mit Default-Zustand: ");
                        Serial.println(defaultState ? "HIGH" : "LOW");
                        
                        digitalWrite(outputPin, defaultState ? HIGH : LOW);
                        
                        // Überprüfe, ob der Pin tatsächlich den gewünschten Zustand angenommen hat
                        int actualState = digitalRead(outputPin);
                        Serial.print("  Tatsächlicher Pin-Zustand nach Initialisierung: ");
                        Serial.println(actualState == HIGH ? "HIGH" : "LOW");
                        
                        break;
                    }
                }
            }
        }
    }
    
    // Anfangszustände der Ausgänge speichern
    for (uint8_t i = 0; i < DIGITAL_OUTPUTS_COUNT; i++) {
        previousOutputValues[i] = digitalRead(getPinFromString(DIGITAL_OUTPUTS[i].pin));
    }

    // Sende Discovery-Nachrichten
   //  applyDefaultOutputStates();
    sendDiscoveryMessages();
    
    // Sende initiale Statusinformationen
    sendInitialStateMessages();
   
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
    monitorInputs();
    monitorOutputs();
    mapInputsToOutputs();
    
    delay(100);  // Kurze Verzögerung
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
            
            // Optional: Sende alle Zustände nach Reconnect
           // sendAllStates();
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

    // Prüfe, ob es ein Steuerbefehl ist
    if (strstr(topic, command_suffix) != NULL) {

        // Extrahiere den Pin aus dem Thema mit ausführlichem Debug
        int pin = extractPinFromTopic(topic);
        
        // Wenn kein gültiger Pin gefunden wurde
        if (pin == -1) {
            Serial.println("ERROR: Could not extract a valid pin from topic!");
            return;
        }
        
        // Pin als Ausgang konfigurieren
        pinMode(pin, OUTPUT);
        Serial.print("Set pin mode to OUTPUT for pin ");
        Serial.println(pin);

        // Status setzen basierend auf der Nachricht
        if (payloadStr == "ON") {
            digitalWrite(pin, HIGH);
            Serial.print("Set pin ");
            Serial.print(pin);
            Serial.println(" to HIGH (ON)");
            
            // Direkte Überprüfung des Pin-Status nach dem Setzen
            Serial.print("Pin status after setting to HIGH: ");
            Serial.println(digitalRead(pin));
            
        } else if (payloadStr == "OFF") {
            digitalWrite(pin, LOW);
            Serial.print("Set pin ");
            Serial.print(pin);
            Serial.println(" to LOW (OFF)");
            
            // Direkte Überprüfung des Pin-Status nach dem Setzen
            Serial.print("Pin status after setting to LOW: ");
            Serial.println(digitalRead(pin));
            
        } else {
            Serial.print("Unknown command: ");
            Serial.println(payloadStr);
            return;
        }
        
        // Status-Update zurücksenden
        String topicStr = String(topic);
        int cmdSuffixPos = topicStr.lastIndexOf(command_suffix);
        if (cmdSuffixPos != -1) {
            String baseTopic = topicStr.substring(0, cmdSuffixPos);
            String stateTopic = baseTopic + state_suffix;
            client.publish(stateTopic.c_str(), payloadStr.c_str());
        }
    } else {
       // Serial.println("Not a command topic, ignoring.");
    }
}

void sendInitialStateMessages() {
    // Sende Statusinachrichten für alle digitalen Ausgänge
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
                currentValue
            );
        }
    }
}

void mapInputsToOutputs() {
  for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
    int reading = digitalRead(getPinFromString(ANALOG_INPUTS[i].pin));

    // Entprellungslogik
    if (reading != previousInputValues[i]) {
      lastDebounceTime[i] = millis();
    }

    // Prüfe, ob Entprellungszeit überschritten
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      // Zustandsänderung prüfen
      if (reading != inputStates[i]) {
        inputStates[i] = reading;

        // Wenn Taste gedrückt wird (LOW)
        if (inputStates[i] == LOW) {
          // Startzeit des Tastendrucks speichern und Tastendruck als aktiv markieren
          pressStartTime[i] = millis();
          buttonPressInProgress[i] = true;
          
          Serial.print("Button press started on input ");
          Serial.println(i);
        }
        // Wenn Taste losgelassen wird (HIGH) und ein Tastendruck aktiv war
        else if (inputStates[i] == HIGH && buttonPressInProgress[i]) {
          // Dauer des Tastendrucks berechnen
          unsigned long pressDuration = millis() - pressStartTime[i];
          buttonPressInProgress[i] = false;
          
          Serial.print("Button released on input ");
          Serial.print(i);
          Serial.print(", press duration: ");
          Serial.print(pressDuration);
          Serial.println(" ms");

          // Überprüfen, ob es ein kurzer Tastendruck war (unter 3 Sekunden)
          if (pressDuration < LONG_PRESS_THRESHOLD) {
            Serial.println("Short press detected - processing action");
            
            // Ausgänge für diesen Eingang durchgehen
            if (ANALOG_INPUTS[i].outputs_count > 0) {
              for (uint8_t j = 0; j < ANALOG_INPUTS[i].outputs_count; j++) {
                const char* outputPinStr = ANALOG_INPUTS[i].outputs[j].pin;
                int outputPin = getPinFromString(outputPinStr);

                // Toggle Ausgangszustand
                outputStates[i] = !outputStates[i];

                // Pin schalten
                digitalWrite(outputPin, outputStates[i] ? HIGH : LOW);

                // MQTT-Nachricht senden
                String topicStr = String(ANALOG_INPUTS[i].mqtt_topic);
                int slashIndex = topicStr.indexOf('/');
                String baseTopic = "aha/" + topicStr.substring(0, slashIndex);
                String fullTopicPart = topicStr.substring(slashIndex + 1);
                String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;

                const char* statePayload = outputStates[i] ? "ON" : "OFF";
                client.publish(stateTopic.c_str(), statePayload);
              }
            }
          } else {
            Serial.println("Long press detected - ignoring action");
          }
        }
      }
    }

    // Letzten Zustand aktualisieren
    previousInputValues[i] = reading;
  }
}


void sendStateMessage(const char* mqtt_topic = NULL, const char* state_suffix = NULL, int value = -1) {
    // Wenn keine Parameter gegeben sind, sende alle Status
    if (mqtt_topic == NULL) {
        // State-Nachrichten für digitale Ausgänge senden
        for (uint8_t i = 0; i < sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]); i++) {
            const DigitalOutput& digitalOutput = DIGITAL_OUTPUTS[i];
            bool state = digitalRead(digitalOutput.pin);
            sendStateMessage(digitalOutput.mqtt_topic, state_suffix, state);
        }

        // State-Nachrichten für digitale Eingänge senden
        for (uint8_t i = 0; i < ANALOG_INPUTS_COUNT; i++) {
            const AnalogInput& analogInput = ANALOG_INPUTS[i];
            bool state = digitalRead(getPinFromString(analogInput.pin));
            sendStateMessage(analogInput.mqtt_topic, state_suffix, state);
        }
        return;
    }
    
    // Einzelne Status-Nachricht senden
    // Topic zusammenbauen
    String topicParts = String(mqtt_topic);
    int slashIndex = topicParts.indexOf('/');
    String baseTopic = "aha/" + topicParts.substring(0, slashIndex);
    String fullTopicPart = topicParts.substring(slashIndex + 1);
    String stateTopic = baseTopic + "/" + fullTopicPart + "/" + state_suffix;
    
    // Digitale Werte als ON/OFF formatieren
    String statePayload = (value == HIGH) ? "ON" : "OFF";
    
    // Senden der MQTT-Nachricht
    bool publishResult = client.publish(stateTopic.c_str(), statePayload.c_str());
    
    // Debug-Ausgabe
    Serial.print("Sent state message to topic ");
    Serial.print(stateTopic);
    Serial.print(": ");
    Serial.println(statePayload);
    
    // Optional: Fehlerbehandlung
    if (!publishResult) {
        Serial.println("Failed to publish MQTT message!");
    }
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
        
        Serial.print("Verarbeite digitalen Ausgang: ");
        Serial.println(digitalOutput.name);
        
        String topicParts = digitalOutput.mqtt_topic;
        int slashIndex = topicParts.indexOf('/');
        String baseTopic = "aha/" + topicParts.substring(0, slashIndex);
        String fullTopicPart = topicParts.substring(slashIndex + 1);
        
        String discoveryTopic = "homeassistant/switch/" + baseTopic.substring(4) + "/" + fullTopicPart + "/config";
        
        Serial.print("Discovery-Topic: ");
        Serial.println(discoveryTopic);
        
        StaticJsonDocument<512> discoveryDoc;
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
            Serial.print("Discovery-Nachricht erfolgreich gesendet für: ");
            Serial.println(digitalOutput.name);
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
        
        // Initiale Verfügbarkeitsnachricht senden
        bool availabilityPublishResult = client.publish(availabilityTopic.c_str(), "online");
        
        client.loop(); // MQTT-Nachrichten verarbeiten
        delay(100);    // Kurze Verzögerung zwischen Nachrichten
    }

    // Discovery-Nachrichten für digitale Eingänge (als AnalogInput deklariert) senden
    Serial.println("Sende Discovery-Nachrichten für digitale Eingänge...");
    Serial.print("Anzahl digitaler Eingänge: ");
    Serial.println(sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]));

    for (uint8_t i = 0; i < sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]); i++) {
        const AnalogInput& analogInput = ANALOG_INPUTS[i];
        
        Serial.print("Verarbeite digitalen Eingang: ");
        Serial.println(analogInput.name);
        
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
            Serial.print("Discovery-Nachricht erfolgreich gesendet für: ");
            Serial.println(analogInput.name);
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
        
        // Initiale Verfügbarkeitsnachricht senden
        bool availabilityPublishResult = client.publish(availabilityTopic.c_str(), "online");
        
        client.loop(); // MQTT-Nachrichten verarbeiten
        delay(100);    // Kurze Verzögerung zwischen Nachrichten
    }

    Serial.println("Senden der Discovery-Nachrichten abgeschlossen.");
}