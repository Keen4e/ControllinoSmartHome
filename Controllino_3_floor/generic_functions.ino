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

        client.subscribe(topicHash.c_str());  // Konvertiere mit .c_str() zu const char*

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
             "\"unique_id\": \"%s/%s\", "  // Verwendung der ausgelagerten reference
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
             reference,  // Hier wird die kleingeschriebene reference verwendet
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
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/%s_A%d/config", reference, i);

    snprintf(payload, sizeof(payload),
             "{"
             "\"name\": \"Analog Input A%d\", "
             "\"state_topic\": \"%s\", "
             "\"command_topic\": \"%s\", "
             "\"availability_topic\": \"%s\", "
             "\"unique_id\": \"%s_A%d\", "  // Verwendung der ausgelagerten reference
             "\"device\": {"
             "\"identifiers\": \"%s_A%d\", "
             "\"name\": \"Analog Port A%d\", "
             "\"model\": \"Analog Sensor\", "
             "\"manufacturer\": \"DIY\""
             "}"
             "}",
             i, state_topic, command_topic, availability_topic, reference, i, reference, i, i);

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