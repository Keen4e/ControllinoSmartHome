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
        "A0 - Schrank re",
        "C1/C1_A0",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_R1"}},
        1
    },
    {
        "CONTROLLINO_A1",
        "A1 - Schrank Deckenlicht",
        "C1/C1_A1",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_R1"}},
        1
    },
    {
        "CONTROLLINO_A2",
        "A2 - Bad Spiegel",
        "C1/C1_A2",
        "C1_Eingänge",
        "power",
      //  {{"CONTROLLINO_D19"},{"CONTROLLINO_D21"} },
      //  2
        {{"CONTROLLINO_R8"}},
        1
    },
    {
        "CONTROLLINO_A3",
        "A3 - Schlafzimmer Deckenlicht",
        "C1/C1_A3",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_D3"}},
        1
    },
    {
        "CONTROLLINO_A4",
        "A4 - Schlafzimmer li",
        "C1/C1_A4",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_D4"}},
        1
    },
    {
        "CONTROLLINO_A5",
        "A5 - Schlafzimmer Rolladen Tuere hoch",
        "C1/C1_A5",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A6",
        "A6 - Flur",
        "C1/C1_A6",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_D19"},{"CONTROLLINO_D21"} },
        2
        //{{"CONTROLLINO_R8"}},
        //1
    },
    {
        "CONTROLLINO_A7",
        "A7 - Schlafzimmer Rolladen Tuere runter",
        "C1/C1_A7",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A8",
        "A8 - Rolladen Tuer runter",
        "C1/C1_A8",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A9",
        "A9 - Rolladen Seite runter",
        "C1/C1_A9",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A10",
        "A10 - Rolladen Seite hoch",
        "C1/C1_A10",
        "C1_Eingänge",
        "power",
         {{"CONTROLLINO_D12"}},
        1
    },
    {
        "CONTROLLINO_A11",
        "A11 - Buero Licht",
        "C1/C1_A11",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A12",
        "A12 - Buero Stehlampe",
        "C1/C1_A12",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_D11"}},
        1
    },
        {
        "CONTROLLINO_A13",
        "A13 - Schlafzimmer re",
        "C1/C1_A13",
        "C1_Eingänge",
        "power",
        {{"CONTROLLINO_D5"}},
        1
    },
    {
        "CONTROLLINO_A14",
        "A14 - Buero Rolladen runter",
        "C1/C1_A14",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A15",
        "A15 - Buero Rolladen hoch",
        "C1/C1_A15",
        "C1_Eingänge",
        "power",
        {{""}},
        1
    }
};

const uint8_t ANALOG_INPUTS_COUNT = sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]);

#endif // ANALOG_INPUTS_HPP