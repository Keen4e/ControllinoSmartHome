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
        "DE:AD:BE:EF:FE:ED",   // MAC Address
        "192.168.178.113",     // IP Address
        "255.255.255.0",       // Subnet Mask
        "192.168.1.1",         // Gateway
        "192.168.1.1"          // DNS Server
    },
    
    // MQTT Configuration
    {
        "192.168.178.42",      // Broker
        1883,                  // Port
        "Controllino3",        // Client ID
        "aha/C3",              // Base Topic
        "cmd_t",               // Command Topic
        "avt_t",               // Availability Topic
        "stat_t"               // State Suffix
    },
    
    // General Configuration
    {
        "C3",                  // Device Name
        "Mitte"                // Location
    }
};

// Compatibility function for legacy code
const char* getConfigJson() {
    return R"json({
  "network": {
    "mac_address": "DE:AD:BE:EF:FE:ED",
    "ip_address": "192.168.178.113",
    "subnet_mask": "255.255.255.0",
    "gateway": "192.168.1.1",
    "dns_server": "192.168.1.1"
  },
  "mqtt": {
    "broker": "192.168.178.42",
    "port": 1883,
    "client_id": "Controllino3",
    "base_topic": "aha/C3",
    "command_topic": "cmd_t",
    "availability_topic": "avt_t",
    "state_suffix": "stat_t"
  },
  "general": {
    "device_name": "C3",
    "location": "Wozi"
  }
})json";
}

#endif // CONFIG_HPP