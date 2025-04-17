#ifndef DIGITAL_OUTPUTS_HPP
#define DIGITAL_OUTPUTS_HPP

#include <Arduino.h>

struct DigitalOutput {
    const char* pin;
    const char* name;
    const char* mqtt_topic;
    const char* mqtt_device;
    bool initial_state;
    const char* exclusive_ports[2]; // Maximale Größe 2 für Kompatibilität
    uint8_t exclusive_ports_count;
};

const DigitalOutput DIGITAL_OUTPUTS[] = {
    {
        "CONTROLLINO_D0",
        "Schrank Licht rechte Seite",
        "C1/C1_D0",
        "C1_Schrank",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D1",
        "Schrank Licht linke Seite",
        "C1/C1_D1",
        "C1_Schrank",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D2",
        "Schrank Fussbodenheizung",
        "C1/C1_D2",
        "C1_Schrank",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D3",
        "Schlafzimmer Deckenlicht",
        "C1/C1_D3",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D4",
        "Schlafzimmer Licht Bett Links",
        "C1/C1_D4",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D5",
        "Schlafzimmer Licht Bett Rechts",
        "C1/C1_D5",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D6",
        "Schlafzimmer Fussbodenheizung",
        "C1/C1_D6",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D7",
        "Buero Rolladen Runter",
        "C1/C1_D7",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D8",
        "Buero Rolladen Hoch",
        "C1/C1_D8",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D9",
        "Schlafzimmer Rollo Tuer hoch",
        "C1/C1_D9",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D10",
        "Schlafzimmer Rollo Tuer runter",
        "C1/C1_D10",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D11",
        "Buero Stehlampe",
        "C1/C1_D11",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D12",
        "Buero Deckenlicht",
        "C1/C1_D12",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D13",
        "Buero Arbeitsplatz",
        "C1/C1_D13",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D14",
        "Buero Fussbodenheizung",
        "C1/C1_D14",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D15",
        "Buero Raumstrom",
        "C1/C1_D15",
        "C1_Buero",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D16",
        "Buero Heizung Schreibtisch",
        "C1/C1_D16",
        "C1_Buero",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D17",
        "Schlafzimmer Rollo runter",
        "C1/C1_D17",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D18",
        "Schlafzimmer Rolle hoch",
        "C1/C1_D18",
        "C1_Schlafzimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D19",
        "Bad Licht",
        "C1/C1_D19",
        "C1_Bad",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D21",
        "Bad Dusche Licht",
        "C1/C1_D21",
        "C1_Bad",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D23",
        "Bad Fussbodenheizung",
        "C1/C1_D23",
        "C1_Bad",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R1",
        "Schrank Deckenlicht",
        "C1/C1_R1",
        "C1_Schrank",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R3",
        "Bad Schublade",
        "C1/C1_R3",
        "C1_Bad",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R5",
        "Schrank Pumpe",
        "C1/C1_R5",
        "C1_Schrank",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R8",
        "Flur Licht",
        "C1/C1_R8",
        "C1_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R9",
        "Flur Fussbodenheizung",
        "C1/C1_R9",
        "C1_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R11",
        "Flur Licht",
        "C1/C1_R11",
        "C1_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R12",
        "Bad Raumstrom",
        "C1/C1_R12",
        "C1_Bad",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R14",
        "Schlafzimmer Raumstrom",
        "C1/C1_R14",
        "C1_Schlafzimmer",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R15",
        "Schrank Raumstrom",
        "C1/C1_R15",
        "C1_Schrank",
        true,
        {nullptr},
        0
    }
};

const uint8_t DIGITAL_OUTPUTS_COUNT = sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]);

#endif // DIGITAL_OUTPUTS_HPP