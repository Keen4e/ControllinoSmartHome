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
        "LED TV", 
        "C4/C4_D0", 
        "C4_Media", 
        false, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D1", 
        "Fernseher", 
        "C4/C4_D1", 
        "C4_Media", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D2", 
        "Router Server Uhr", 
        "C4/C4_D2", 
        "C4_Media", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D3", 
        "Stereo Anlage", 
        "C4/C4_D3", 
        "C4_Media", 
        false, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D6", 
        "Tuer Rolladen hoch", 
        "C4/C4_D6", 
        "C4_Wohnzimmer", 
        false, 
        {"CONTROLLINO_D7"}, 
        1
    },
    {
        "CONTROLLINO_D7", 
        "Tuer Rolladen runter", 
        "C4/C4_D7", 
        "C4_Wohnzimmer", 
        false, 
        {"CONTROLLINO_D6"}, 
        1
    },
    {
        "CONTROLLINO_D8", 
        "Raumstrom Wohnzimmer", 
        "C4/C4_D8", 
        "C4_Wohnzimmer", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D9", 
        "Raumstrom Esszimmer", 
        "C4/C4_D9", 
        "C4_Esszimmer", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D11", 
        "Steckdose Kommode", 
        "C4/C4_D11", 
        "C4_Esszimmer", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D13", 
        "Steckdose Tisch", 
        "C4/C4_D13", 
        "C4_Esszimmer", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D14", 
        "LED Kommode", 
        "C4/C4_D14", 
        "C4_Esszimmer", 
        false, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D15", 
        "Steckdose Fenster", 
        "C4/C4_D15", 
        "C4_Esszimmer", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D16", 
        "Fenster Seite Rolladen hoch", 
        "C4/C4_D16", 
        "C4_Esszimmer", 
        false, 
        {"CONTROLLINO_D17"}, 
        1
    },
    {
        "CONTROLLINO_D17", 
        "Fenster Seite Rolladen runter", 
        "C4/C4_D17", 
        "C4_Esszimmer", 
        false, 
        {"CONTROLLINO_D16"}, 
        1
    },
    {
        "CONTROLLINO_D18", 
        "Aussen Garage/Grill", 
        "C4/C4_D18", 
        "C4_Out", 
        true, 
        {nullptr}, 
        0
    },
    
    {
        "CONTROLLINO_D19", 
        "Verteiler Shelly", 
        "C4/C4_D19", 
        "C4_Wohnzimmer", 
        true, 
        {nullptr}, 
        0
    },
        {
        "CONTROLLINO_D20", 
        "Licht Aussen Rechts", 
        "C4/C4_D20", 
        "C4_Out", 
        false, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_D21", 
        "Licht Aussen Rechts", 
        "C4/C4_D21", 
        "C4_Out", 
        false, 
        {nullptr}, 
        0
    },
    
    // Neue Einträge für C3-Relais
    {
        "CONTROLLINO_R0", 
        "Fire TV", 
        "C3/C3_R0", 
        "C3_Media", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_R1", 
        "SatUSB", 
        "C3/C3_R1", 
        "C3_Media", 
        true, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_R5", 
        "Wohnzimmer Deckenlicht", 
        "C3/C3_R5", 
        "C3_Wohnzimmer", 
        false, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_R6", 
        "Esszimmer Deckenlicht", 
        "C3/C3_R6", 
        "C3_Esszimmer", 
        false, 
        {nullptr}, 
        0
    },
        {
        "CONTROLLINO_R8", 
        "Aussenlicht", 
        "C3/C3_R8", 
        "C3_Out", 
        false, 
        {nullptr}, 
        0
    },
    {
        "CONTROLLINO_R7", 
        "Aussen tbd", 
        "C3/C3_R7", 
        "C3_Out", 
        false, 
        {nullptr}, 
        0
    }
};

const uint8_t DIGITAL_OUTPUTS_COUNT = sizeof(DIGITAL_OUTPUTS) / sizeof(DIGITAL_OUTPUTS[0]);

#endif // DIGITAL_OUTPUTS_HPP