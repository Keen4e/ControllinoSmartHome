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