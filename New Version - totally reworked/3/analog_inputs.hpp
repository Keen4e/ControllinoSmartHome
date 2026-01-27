#ifndef ANALOG_INPUTS_HPP
#define ANALOG_INPUTS_HPP

#include <Arduino.h>

struct OutputPin {
  const char* pin;
};

struct AnalogInput {
  const char* pin;
  const char* name;
  const char* mqtt_topic;
  const char* mqtt_device;
  const char* device_class;
  OutputPin outputs[4];  // Maximal 4 Ausgänge pro Eingang
  uint8_t outputs_count;
};

const AnalogInput ANALOG_INPUTS[] = {
  { 
    "CONTROLLINO_A0",
    "A0 - Wohnzimmer Rollo Hauptfenster hoch",
    "C3/C3_A0",
    "C3_Eingaenge",
    "power",
    {},
    0 
    },
  { 
    "CONTROLLINO_A1",
    "A1 - Terasse Rollo Tuer runter",
    "C3/C3_A1",
    "C3_Eingaenge",
    "power",
    { { "" } },
    1 
    },
  { 
    "CONTROLLINO_A2",
    "A2 - Terasse Rollo Tuer hoch",
    "C3/C3_A2",
    "C3_Eingaenge",
    "power",
    { { "" } },
    1 
    },
  { 
    "CONTROLLINO_A3",
    "A3 - Terasse Rollo Haupt runter",
    "C3/C3_A3",
    "C3_Eingaenge",
    "power",
    {},
    0 
    },
  { 
    "CONTROLLINO_A4",
    "A4 - Kamin links",
    "C3/C3_A4",
    "C3_Eingaenge",
    "power",
    { { "CONTROLLINO_R5" } },
    1 
    },
  { 
    "CONTROLLINO_A5",
    "A5 - Kamin rechtes",
    "C3/C3_A5",
    "C3_Eingaenge",
    "power",
    { { "CONTROLLINO_R6" } },
    1 
    },
  { 
    "CONTROLLINO_A6",
    "A6 - Esszimmerschalter",
    "C3/C3_A6",
    "C3_Eingaenge",
    "power",
    { { "CONTROLLINO_R6" } },
    1 
    },
  
  { 
    "CONTROLLINO_A8",
    "A8 - Rolladen Esszimmer hoch ",
    "C3/C3_A8",
    "C3_Eingaenge",
    "power",
    { { "" } },
    1 
    },
  { 
    "CONTROLLINO_A9",
    "A9 - Rolladen Esszimmer runter ",
    "C3/C3_A9",
    "C3_Eingaenge",
    "power",
    { { "" } },
    1 
    },
  { 
    "CONTROLLINO_A10",
    "A10 - Sofa 1 ",
    "C3/C3_A10",
    "C3_Eingaenge",
    "power",
    {"CONTROLLINO_R5"},
    1 
    },
  { 
    "CONTROLLINO_A11",
    "A11 - Sofa 2 ",
    "C3/C3_A11",
    "C3_Eingaenge",
    "power",
    {"CONTROLLINO_R6"},
    1 
    }
};

const uint8_t ANALOG_INPUTS_COUNT = sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]);

#endif  // ANALOG_INPUTS_HPP