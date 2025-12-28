#pragma once
#include "networkProtocol.h"



#include "network.h"


enum ConsoleThreadCommandTypes {
    CONSOLE_CMD_EXIT = 0,
    CONSOLE_CMD_SAVE,
    CONSOLE_CMD_KICK,
    CONSOLE_CMD_BROADCAST,
    CONSOLE_CMD_UDP_BROADCAST,
    CONSOLE_CMD_UDP_WHISPER,
    CONSOLE_CMD_PING,
    CONSOLE_CMD_STATUS,
    CONSOLE_CMD_HELP,
    CONSOLE_CMD_DEBUG,
    CONSOLE_CMD_INFO,
    // Add more as needed
};


struct ConsoleCommand {
    int type;
    char dataString[256];
    union{
        struct {uint16_t clientID;} udp_whisper;
        struct {uint16_t client_udp_port;} post_connection_handshake;
        struct {int x,y,z;} test;
        struct {int tcp_portNum, udp_portNum;} connect_to_server;
    } data;
};


struct MainServerSocketState {

};


    extern MainServerSocketState mainServerSocketState;
 
    extern ConsoleCommand ConsoleToMain[256];
    extern ConsoleCommand toConsole[256];
    extern atomic_uint32 ConsoleMainWriteIndex;
    extern atomic_uint32 ConsoleMainReadIndex;
    extern atomic_uint32 ConsoleMainCount;
    extern atomic_uint32 consoleWriteIndex;
    extern atomic_uint32 consoleReadIndex;
    extern atomic_uint32 consoleCount;
 
    void init();

    void setup_sockets(network_state* NetworkState);
    void cleanup_server_sockets(network_state* NetworkState);


    int poll_network_messages(network_state* NetworkState);

    void broadcast_tcp_message(network_state* NetworkState, uint16_t type, const char* payload, uint16_t payload_size);

    void broadcast_udp_message(network_state* NetworkState, const char* message);
    int removeClient(network_state* NetworkState, uint8_t socket);
    void handle_connection_handshake(network_state* NetworkState, char* buffer, uint8_t clientID);
    
    void handle_player_input(network_state* NetworkState, char* buffer, int clientID);

    void handle_ping_request(network_state* NetworkState, char* buffer, int clientID);
    int assignClient(network_state* NetworkState, sockaddr_in client_addr, uint8_t& clientID);

  #ifdef _WIN32
    DWORD WINAPI ServerNetworkThreadProc(LPVOID lpParameter);
    #else
    void* ServerNetworkThreadProc(void* lpParameter);
    #endif
        



        
/////////////////////////////////// SERVER CONSOLE COMMAND QUEUE ///////////////////////////////////



void ConsolePushToMain(ConsoleCommand cmd);
bool ConsolePopToMain(ConsoleCommand* out);
void PushToConsole(ConsoleCommand cmd);
bool PopToConsole(ConsoleCommand* out);
#ifdef _WIN32
DWORD WINAPI ConsoleThreadProc(LPVOID lpParameter);
#else
void* ConsoleThreadProc(void* lpParameter);
#endif



/////////////////////////////////// END SERVER CONSOLE THREAD COMMAND QUEUE END ///////////////////////////////////


