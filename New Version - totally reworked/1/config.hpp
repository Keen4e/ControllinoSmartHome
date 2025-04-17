#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>

struct NetworkConfig {
    const char* mac_address;
    const char* ip_address;
    const char* subnet_mask;
    const char* gateway;
    const char* dns_server;
};

struct MqttConfig {
    const char* broker;
    uint16_t port;
    const char* client_id;
    const char* base_topic;
    const char* command_topic;
    const char* availability_topic;
    const char* state_suffix;
};

struct GeneralConfig {
    const char* device_name;
    const char* location;
};

struct SystemConfig {
    NetworkConfig network;
    MqttConfig mqtt;
    GeneralConfig general;
};

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


#endif // CONFIG_HPP