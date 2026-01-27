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
        "CONTROLLINO_A1",
        "A1 - Rollo Esszimmer hoch",
        "C2/C2_A1",
        "C2_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A2",
        "A2 - Rollo Esszimmer runter",
        "C2/C2_A2",
        "C2_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A3",
        "A3 - Rollo Kueche hoch",
        "C2/C2_A3",
        "C2_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A4",
        "A4 - Rollo Kueche runter",
        "C2/C2_A4",
        "C2_Eingänge",
        "power",
        {{""}},
        1
    },
    {
        "CONTROLLINO_A5",
        "A5 - Schalter Waeschekeller",
        "C2/C2_A5",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_D21"},{"CONTROLLINO_R1"}},
        2
    },
    {
        "CONTROLLINO_A6",
        "A6 - Schalter Hobbyraum",
        "C2/C2_A6",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_R0"}},
        1
    },
    {
        "CONTROLLINO_A7",
        "A7 - Schalter Wohnzimmer Flur Unten",
        "C2/C2_A7",
        "C2_Eingänge",
        "power",
         {{"CONTROLLINO_R1"}},
        1
    },
    {
        "CONTROLLINO_A8",
        "A8 - Flur alter 3er re",
        "C2/C2_A8",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_D12"}},
        1
    },
    {
        "CONTROLLINO_A9",
        "A9 - Flur alter 3er li",
        "C2/C2_A9",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_D11"}},
        1
    },
    {
        "CONTROLLINO_A10",
        "A10 - Schalter Haustuer",
        "C2/C2_A10",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_D12"}},
        1
    },
    {
        "CONTROLLINO_A11",
        "A11 - Schalter WC",
        "C2/C2_A11",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_D10"}},
        1
    },
    {
        "CONTROLLINO_A12",
        "A12 - Schalter Kueche",
        "C2/C2_A12",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_D1"}},
        1
    },
        {
        "CONTROLLINO_A13",
        "A13 - Schalter Keller Wohnzimmer oben",
        "C2/C2_A13",
        "C2_Eingänge",
        "power",
        {{"CONTROLLINO_R3"}},
        1
    }
};

const uint8_t ANALOG_INPUTS_COUNT = sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]);

#endif // ANALOG_INPUTS_HPP