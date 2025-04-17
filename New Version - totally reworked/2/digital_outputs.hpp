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
        "NSPanel Kueche",
        "C2/C2_D0",
        "C2_Kueche",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D1",
        "Licht Kueche",
        "C2/C2_D1",
        "C2_Kueche",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D2",
        "Kuehlschrank",
        "C2/C2_D2",
        "C2_Kueche",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D3",
        "Raumstrom Kueche",
        "C2/C2_D3",
        "C2_Kueche",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D4",
        "Esszimmer Rolladen hoch",
        "C2/C2_D4",
        "C2_Esszimmer",
        false,
       // {nullptr},
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D5",
        "Esszimmer Rolladen runter",
        "C2/C2_D5",
        "C2_Esszimmer",
        false,
       // {nullptr},
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D6",
        "Kueche Steckdose bei Rolladen (todo)",
        "C2/C2_D6",
        "C2_Kueche",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D7",
        "Kueche Rolladen hoch",
        "C2/C2_D7",
        "C2_Kueche",
        false,
      //  {nullptr},
        {"CONTROLLINO_D8"},
        1
    },
    {
        "CONTROLLINO_D8",
        "Kueche Rolladen runter",
        "C2/C2_D8",
        "C2_Kueche",
        false,
        //{nullptr},
        {"CONTROLLINO_D7"},
        1
    },
    {
        "CONTROLLINO_D9",
        "Esszimmer Steckdose Rolladen",
        "C2/C2_D9",
        "C2_Esszimmer",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D10",
        "Klo Licht",
        "C2/C2_D10",
        "C2_Klo",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D11",
        "Esszimmer Deckenlampe",
        "C2/C2_D11",
        "C2_Esszimmer",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D12",
        "D12 tbd",
        "C2/C2_D12",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D13",
        "Klingel und Flur",
        "C2/C2_D13",
        "C2_out",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D14",
        "Flur Licht",
        "C2/C2_D14",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D15",
        "Flur Garderobe",
        "C2/C2_D15",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D16",
        "Fire Tablet",
        "C2/C2_D16",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D17",
        "Flur Treppe oben 1",
        "C2/C2_D17",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D18",
        "Flur Treppe oben 2",
        "C2/C2_D18",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D19",
        "Flur Treppe oben 3",
        "C2/C2_D19",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D20",
        "Flur Licht Spiegel",
        "C2/C2_D20",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D21",
        "Flur Treppe ganz unten",
        "C2/C2_D21",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D22",
        "Flur Treppe mitte 2",
        "C2/C2_D22",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_D23",
        "Flur Treppe mitte 3",
        "C2/C2_D23",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R0",
        "Weinregal",
        "C2/C2_R0",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R1",
        "Flur Treppe Mitte Unten",
        "C2/C2_R1",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R2",
        "Flur Treppe unten 3",
        "C2/C2_R2",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R3",
        "Flur Treppenhaus",
        "C2/C2_R3",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R4",
        "R4 tbd",
        "C2/C2_R4",
        "C2_tbd",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R5",
        "Eszzimmer Steckdose altes Licht",
        "C2/C2_R5",
        "C2_Esszimmer",
        true,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R6",
        "R6 tbd",
        "C2/C2_R6",
        "C2_tbd",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R7",
        "R7 tbd",
        "C2/C2_R7",
        "C2_tbd",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R8",
        "Werkzeugkeller",
        "C2/C2_R8",
        "C2_Werkzeugkeller",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R9",
        "Treppe ganz oben 3",
        "C2/C2_R9",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R10",
        "Treppe ganz oben 2",
        "C2/C2_R10",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R11",
        "Treppe ganz oben 1",
        "C2/C2_R11",
        "C2_Flur",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R12",
        "Raumstrom Hobbyraum",
        "C2/C2_R12",
        "C2_Hobbyraum",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R13",
        "R13 tbd",
        "C2/C2_R13",
        "C2_tbd",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R14",
        "R14 tbd",
        "C2/C2_R14",
        "C2_tbd",
        false,
        {nullptr},
        0
    },
    {
        "CONTROLLINO_R15",
        "R15 tbd",
        "C2/C2_R15",
        "C2_tbd",
        false,
        {nullptr},
        0
    }
};

const uint8_t DIGITAL_OUTPUTS_COUNT = sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]);

#endif // DIGITAL_OUTPUTS_HPP