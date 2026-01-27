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
        "LED TV", 
        "C3/C3_D0", 
        "C3_Media", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
    {
        "CONTROLLINO_D1", 
        "Fernseher", 
        "C3/C3_D1", 
        "C3_Media", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D2", 
        "Router Server Uhr", 
        "C3/C3_D2", 
        "C3_Media", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D3", 
        "Stereo Anlage", 
        "C3/C3_D3", 
        "C3_Media", 
        false, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D6", 
        "Tuer Rolladen hoch", 
        "C3/C3_D6", 
        "C3_Wohnzimmer", 
        false, 
        {"CONTROLLINO_D7"}, 
        1,
        "switch"
    },
    {
        "CONTROLLINO_D7", 
        "Tuer Rolladen runter", 
        "C3/C3_D7", 
        "C3_Wohnzimmer", 
        false, 
        {"CONTROLLINO_D6"}, 
        1,
        "switch"
    },
    {
        "CONTROLLINO_D8", 
        "Raumstrom Wohnzimmer", 
        "C3/C3_D8", 
        "C3_Wohnzimmer", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D9", 
        "Raumstrom Esszimmer", 
        "C3/C3_D9", 
        "C3_Esszimmer", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D11", 
        "Steckdose Kommode", 
        "C3/C3_D11", 
        "C3_Esszimmer", 
        true, 
        {nullptr}, 
         0,
        "switch"
    },
    {
        "CONTROLLINO_D13", 
        "Steckdose Tisch", 
        "C3/C3_D13", 
        "C3_Esszimmer", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D14", 
        "LED Kommode", 
        "C3/C3_D14", 
        "C3_Esszimmer", 
        false, 
        {nullptr}, 
        0,
        "light"
    },                                                      
    {
        "CONTROLLINO_D15", 
        "Steckdose Fenster", 
        "C3/C3_D15", 
        "C3_Esszimmer", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_D16", 
        "Fenster Seite Rolladen hoch", 
        "C3/C3_D16", 
        "C3_Esszimmer", 
        false, 
        {"CONTROLLINO_D17"}, 
        1,
        "switch"
    },
    {
        "CONTROLLINO_D17", 
        "Fenster Seite Rolladen runter", 
        "C3/C3_D17", 
        "C3_Esszimmer", 
        false, 
        {"CONTROLLINO_D16"}, 
        1,
        "switch"
    },
    {
        "CONTROLLINO_D18", 
        "Aussen Garage/Grill", 
        "C3/C3_D18", 
        "C3_Out", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    
    {
        "CONTROLLINO_D19", 
        "Verteiler Shelly", 
        "C3/C3_D19", 
        "C3_Wohnzimmer", 
        true, 
        {nullptr}, 
           0,
        "switch"
    },
        {
        "CONTROLLINO_D20", 
        "Licht Aussen Rechts", 
        "C3/C3_D20", 
        "C3_Out", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
    {
        "CONTROLLINO_D21", 
        "Licht Aussen Rechts", 
        "C3/C3_D21", 
        "C3_Out", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
        {
        "CONTROLLINO_D22", 
        "LED Schuhschrank", 
        "C3/C3_D22", 
        "C2_Flur", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
            {
        "CONTROLLINO_D23", 
        "LED Kommode Esszimmer", 
        "C3/C3_D23", 
        "C3_Esszimmer", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
    // Neue Einträge für C3-Relais
    {
        "CONTROLLINO_R0", 
        "Fire TV", 
        "C3/C3_R0", 
        "C3_Media", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_R1", 
        "SatUSB", 
        "C3/C3_R1", 
        "C3_Media", 
        true, 
        {nullptr}, 
          0,
        "switch"
    },
     {
        "CONTROLLINO_R4", 
        "LED Netzteil", 
        "C3/C3_R4", 
        "C3_Esszimmer", 
        true, 
        {nullptr}, 
        0,
        "switch"
    },
    {
        "CONTROLLINO_R5", 
        "Wohnzimmer Deckenlicht", 
        "C3/C3_R5", 
        "C3_Wohnzimmer", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
    {
        "CONTROLLINO_R6", 
        "Esszimmer Deckenlicht", 
        "C3/C3_R6", 
        "C3_Esszimmer", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
        {
        "CONTROLLINO_R8", 
        "Aussenlicht", 
        "C3/C3_R8", 
        "C3_Out", 
        false, 
        {nullptr}, 
        0,
        "light"
    },
    {
        "CONTROLLINO_R7", 
        "Aussen Lichterkette", 
        "C3/C3_R7", 
        "C3_Out", 
        false, 
        {nullptr}, 
        0,
        "light"
    }
};

const uint8_t DIGITAL_OUTPUTS_COUNT = sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]);

#endif // DIGITAL_OUTPUTS_HPP