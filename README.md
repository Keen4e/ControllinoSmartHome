# ControllinoSmartHome
This is the further enhanced version of my Controllino programming to implement my smarthome. 
The code runns stable in my home now since a year and I consider it as final. 
I offer professional sevices on this project (+the installation/configuration services arround it via a small startup (www.immo-vernetzer.de)
I use 3 controllinos - hence there are 3 folders. They contain the same logic (1,2,3.ino) with different configurations. 

# Introduction:
This project covers the following functions:
- Mapping fron input ports to output ports
- Auto generation of MQTT Autodiscovery for Homeassistant
- Generation of State and availability messages 
- State Handling via MQTT (irrespective of trigger, e.g. Hardware button, MQTT message, homeassistant change)
- Remote triggering of MQTT messages to come back online after connection loss

# Details and configuration:
## File structure
Each Controllino uses 3 files: 
- x.ino (Contains the logic - typically no need for any changes)
- config.hpp (General paramenters like MQTT)
- analog_inputs.hpp (Configuration of the input ports)
- digital_outputs.hpp (Configuration of the output ports) 

## Configuration
The file config.hpp needs to be set to the respective environment (Should be rather self explaining)
```
const SystemConfig SYSTEM_CONFIG = {
    // Network Configuration
    {
        "DE:AD:EB:FE:EF:ED",   // MAC Address
        "192.168.178.111",     // IP Address
        "255.255.255.0",       // Subnet Mask
        "192.168.1.1",         // Gateway
        "192.168.1.1"          // DNS Server
    },
    
    // MQTT Configuration
    {
        "192.168.178.42",      // Broker
        1883,                  // Port
        "Controllino1",        // Client ID
        "aha/C1",              // Base Topic
        "cmd_t",               // Command Topic
        "avt_t",               // Availability Topic
        "stat_t"               // State Suffix
    },
    
    // General Configuration
    {
        "C1",                  // Device Name
        "oben"                // Location
    }
};
```
The analog_inputs.hpp configures inputs like this
```
    {
        "CONTROLLINO_A1", //Pin (needs to follow Controllino logic)
        "A1 - Schrank Deckenlicht", //Name in Text
        "C1/C1_A1", //Technical ID = MQTT Topic
        "C1_Eingänge", // Device in Homeassistant
        "power", //Device Class of home assistant
        {{"CONTROLLINO_R1"}}, //Which output is toggled, multiple entries possible,
        1 // how many outputs are toggelt (Function potentially to be removed) 
    },
```
The digital_outputs.hpp configures outputs like this
```
    {
        "CONTROLLINO_R8", //Pin (needs to follow Controllino logic)
        "Flur Licht", //Name in Text
        "C1/C1_R8", //Technical ID = MQTT Topic
        "C1_Flur", // Device in Homeassistant
        false, // Port is activated during startup of controllino (usefull for power outage)
        {nullptr}, // Blocks corresponding roller shutter 
        0, //how many to block
        "light" //Device Class of home assistant
    },```
