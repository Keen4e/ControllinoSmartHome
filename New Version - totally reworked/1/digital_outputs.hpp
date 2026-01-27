#ifndef DIGITAL_OUTPUTS_HPP
#define DIGITAL_OUTPUTS_HPP

#include <Arduino.h>

struct DigitalOutput {
    const char* pin;
    const char* name;
    const char* mqtt_topic;
    const char* mqtt_device;
    bool initial_state;
    const char* outputs[5];
    uint8_t outputs_count;
    const char* device_class;  // Neuer Parameter: "light" oder "switch"
};

const DigitalOutput DIGITAL_OUTPUTS[] = {
    {
        "CONTROLLINO_D0",
        "Schrank Licht rechte Seite",
        "C1/C1_D0",
        "C1_Schrank",
        false,
        {nullptr},
        0,
        "light"  // Neuer Parameter
    },
    {
        "CONTROLLINO_D1",
        "Schrank Licht linke Seite",
        "C1/C1_D1",
        "C1_Schrank",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D2",
        "Schrank Bodenheizung",
        "C1/C1_D2",
        "C1_Schrank",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D3",
        "Schlafzimmer Deckenlicht",
        "C1/C1_D3",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D4",
        "Schlafzimmer Licht Bett Links",
        "C1/C1_D4",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D5",
        "Schlafzimmer Licht Bett Rechts",
        "C1/C1_D5",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D6",
        "Schlafzimmer Bodenheizung",
        "C1/C1_D6",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D7",
        "Buero Rolladen Runter",
        "C1/C1_D7",
        "C1_Buero",
        false,
        {"CONTROLLINO_D8"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D8",
        "Buero Rolladen Hoch",
        "C1/C1_D8",
        "C1_Buero",
        false,
        {"CONTROLLINO_D7"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D9",
        "Schlafzimmer Rollo Tuer hoch",
        "C1/C1_D9",
        "C1_Schlafzimmer",
        false,
        {"CONTROLLINO_D10"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D10",
        "Schlafzimmer Rollo Tuer runter",
        "C1/C1_D10",
        "C1_Schlafzimmer",
        false,
        {"CONTROLLINO_D9"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D11",
        "Buero Stehlampe",
        "C1/C1_D11",
        "C1_Buero",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D12",
        "Buero Deckenlicht",
        "C1/C1_D12",
        "C1_Buero",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D13",
        "Buero Arbeitsplatz",
        "C1/C1_D13",
        "C1_Buero",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D14",
        "Buero Bodenheizung",
        "C1/C1_D14",
        "C1_Buero",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D15",
        "Buero Raumstrom",
        "C1/C1_D15",
        "C1_Buero",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D16",
        "Buero Heizung Schreibtisch",
        "C1/C1_D16",
        "C1_Buero",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D17",
        "Schlafzimmer Rollo runter",
        "C1/C1_D17",
        "C1_Schlafzimmer",
        false,
        {"CONTROLLINO_D18"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D18",
        "Schlafzimmer Rollo hoch",
        "C1/C1_D18",
        "C1_Schlafzimmer",
        false,
        {"CONTROLLINO_D17"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D19",
        "Bad Licht",
        "C1/C1_D19",
        "C1_Bad",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D21",
        "Bad Dusche Licht",
        "C1/C1_D21",
        "C1_Bad",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D23",
        "Bad Bodenheizung",
        "C1/C1_D23",
        "C1_Bad",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_R1",
        "Schrank Deckenlicht",
        "C1/C1_R1",
        "C1_Schrank",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R3",
        "Bad Schublade",
        "C1/C1_R3",
        "C1_Bad",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_R5",
        "Schrank Pumpe",
        "C1/C1_R5",
        "C1_Schrank",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_R8",
        "Flur Licht",
        "C1/C1_R8",
        "C1_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R9",
        "Flur Bodenheizung",
        "C1/C1_R9",
        "C1_Flur",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_R11",
        "Flur Licht",
        "C1/C1_R11",
        "C1_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R12",
        "Bad Raumstrom",
        "C1/C1_R12",
        "C1_Bad",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_R14",
        "Schlafzimmer Raumstrom",
        "C1/C1_R14",
        "C1_Schlafzimmer",
        true,
        {nullptr},
        0,
        "switch"
    }
};

const uint8_t DIGITAL_OUTPUTS_COUNT = sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]);

#endif // DIGITAL_OUTPUTS_HPP