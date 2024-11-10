unsigned long lastReconnectAttempt = 0;
unsigned long lastAvailabilityMessage = 0;
unsigned long previousMillis = 0;
const unsigned long interval = 2 * 60 * 1000;
const unsigned long reconnectInterval = 10 * 60 * 1000;
const unsigned long availabilityInterval = 1 * 60 * 1000;

int previousButtonStates[16] = { 0 };  // Array zum Speichern der vorherigen Zustände aller analogen Buttons
void publishState(int index) {
  String stateTopic = rolladen[index].topic + "/stat_t";
  String stateMessage = String(rolladen[index].current_position);
  client.publish(stateTopic.c_str(), stateMessage.c_str(), true);
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
    digitalWrite(action->pin, action->defaultState ? HIGH : LOW);  // Aktivieren oder Deaktivieren basierend auf defaultState
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
void handleDigitalStatus(int buttonPin, int pinIndex) {
  // Array zum Speichern der vorherigen Zustände der digitalen Pins
  static int previousDigitalValues[16] = { LOW };

  int currentState = digitalRead(buttonPin);  // Aktuellen digitalen Zustand lesen

  // Prüfen, ob sich der digitale Wert geändert hat
  if (currentState != previousDigitalValues[pinIndex]) {
    String topic = String("aha/") + reference + "/" + reference + "_A" + String(pinIndex) + "/stat_t";
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
void sendAvailabilityMessages() {
  // Verfügbarkeitsnachrichten für Rolläden
  for (int i = 0; i < NUM_ROLLADEN; i++) {
    String availabilityTopic = rolladen[i].topic + "/avt_t";
    String state = "online";                                         // Definiere den Status der Verfügbarkeit
    client.publish(availabilityTopic.c_str(), state.c_str(), true);  // Nachricht senden
  }


  for (int i = 0; i < numPins; i++) {
    String topic = String(base_topic) + reference + "_";

    if (i < 24) {
      topic += "D" + String(i) + "/avt_t";
    } else {
      topic += "R" + String(i - 24) + "/avt_t";
    }
    String message = "online";
    client.publish(topic.c_str(), message.c_str(), true);
  }

  for (int i = 1; i <= numAnalogPins; i++) {
    String topic = String(base_topic) + reference + "_A" + String(i) + "/avt_t";
    String message = "online";
    client.publish(topic.c_str(), message.c_str(), true);  // Nachricht senden
  }}
  void reconnect() {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(clientId)) {  // Verwendung der clientId-Variable
        Serial.println("connected");

        client.subscribe(topicPrefix.c_str());  // Konvertiere mit .c_str() zu const char*

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
  void loop() {
    Ethernet.maintain();
    client.loop();
    readAnalogValues();
    if (!client.connected()) {
      Serial.print("...lost connections... ");
      reconnect();
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


  for (int i = 0; i < numPins; i++) {
    bool currentState = digitalRead(pins[i]);

    if (currentState != previousStates[i] || firstRun) {
      String topic = String(base_topic) + reference + "_";

      if (i < 24) {
        topic += "D" + String(i) + "/stat_t";
      } else {
        topic += "R" + String(i - 24) + "/stat_t";
      }

      String message = currentState ? "ON" : "OFF";
      client.publish(topic.c_str(), message.c_str());
      previousStates[i] = currentState;
    }
  }

  // Statusnachrichten für analoge Pins (A1 bis A10)
  for (int i = 1; i <= numAnalogPins; i++) {                               // Ändere den Index auf 1, um A1 bis A10 zu verwenden
    int currentAnalogValue = digitalRead(i);                               // Liest den analogen Wert von Pin A1 bis A10
    if (currentAnalogValue != analogStates[i - 1] || firstRun) {           // Benutze (i - 1) für Array-Index
      String topic = String(base_topic) + reference + "_A" + String(i) + "/stat_t";  
      String message = String(currentAnalogValue);                         // Konvertiere den Wert in einen String
    
      client.publish(topic.c_str(), message.c_str());                      // Sende die Nachricht über MQTT

      analogStates[i - 1] = currentAnalogValue;  // Aktualisiere den gespeicherten Zustand
    }
  }


}