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
        "NSPanel Kueche",
        "C2/C2_D0",
        "C2_Kueche",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D1",
        "Licht Kueche",
        "C2/C2_D1",
        "C2_Kueche",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D2",
        "Kuehlschrank",
        "C2/C2_D2",
        "C2_Kueche",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D3",
        "Raumstrom Kueche",
        "C2/C2_D3",
        "C2_Kueche",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D4",
        "Esszimmer Rolladen runter",
        "C2/C2_D4",
        "C2_Esszimmer",
        false,
       // {nullptr},
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D5",
        "Esszimmer Rolladen hoch",
        "C2/C2_D5",
        "C2_Esszimmer",
        false,
       // {nullptr},
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D6",
        "Esszimmer Steckdose bei Rolladen",
        "C2/C2_D6",
        "C2_Kueche",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D7",
        "Kueche Rolladen hoch",
        "C2/C2_D7",
        "C2_Kueche",
        false,
      //  {nullptr},
        {"CONTROLLINO_D8"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D8",
        "Kueche Rolladen runter",
        "C2/C2_D8",
        "C2_Kueche",
        false,
        //{nullptr},
        {"CONTROLLINO_D7"},
        1,
        "switch"
    },
    {
        "CONTROLLINO_D9",
        "Kueche Steckdose Rolladen",
        "C2/C2_D9",
        "C2_Esszimmer",
        true,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D10",
        "Klo Licht",
        "C2/C2_D10",
        "C2_Klo",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D11",
        "Esszimmer Deckenlampe",
        "C2/C2_D11",
        "C2_Esszimmer",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D12",
        "Flur Deckenlicht",
        "C2/C2_D12",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D13",
        "Klingel und Flur",
        "C2/C2_D13",
        "C2_out",
        true,
        {nullptr},
        0,
        "switch"
    },
   

    {
        "CONTROLLINO_D16",
        "Fire Tablet",
        "C2/C2_D16",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_D17",
        "Flur Treppe Mitte 1",
        "C2/C2_D17",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D18",
        "Flur Treppe Mitte 2",
        "C2/C2_D18",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D19",
        "Flur Treppe Mitte 3",
        "C2/C2_D19",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D20",
        "Flur Licht Spiegel",
        "C2/C2_D20",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_D21",
        "Flur Treppe ganz unten",
        "C2/C2_D21",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },


    {
        "CONTROLLINO_R0",
        "Weinregal",
        "C2/C2_R0",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R1",
        "Flur Treppe Mitte Unten",
        "C2/C2_R1",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },

    {
        "CONTROLLINO_R3",
        "Flur Treppenhaus",
        "C2/C2_R3",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },

    {
        "CONTROLLINO_R5",
        "Eszzimmer Steckdose altes Licht",
        "C2/C2_R5",
        "C2_Esszimmer",
        true,
        {nullptr},
        0,
        "switch"
    },
  
    {
        "CONTROLLINO_R8",
        "Werkzeugkeller",
        "C2/C2_R8",
        "C2_Werkzeugkeller",
        false,
        {nullptr},
        0,
        "switch"
    },
    {
        "CONTROLLINO_R9",
        "Treppe ganz oben 3",
        "C2/C2_R9",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R10",
        "Treppe ganz oben 2",
        "C2/C2_R10",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R11",
        "Treppe ganz oben 1",
        "C2/C2_R11",
        "C2_Flur",
        false,
        {nullptr},
        0,
        "light"
    },
    {
        "CONTROLLINO_R12",
        "Raumstrom Hobbyraum",
        "C2/C2_R12",
        "C2_Hobbyraum",
        false,
        {nullptr},
        0,
        "switch"
    }
};

const uint8_t DIGITAL_OUTPUTS_COUNT = sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]);

#endif // DIGITAL_OUTPUTS_HPP