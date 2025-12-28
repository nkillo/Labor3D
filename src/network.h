#pragma once


// #include "threadPool.h"

/////////////////////////////////// NETWORKING THREAD COMMAND QUEUE ///////////////////////////////////

struct network_thread_state{
    network_state* NetworkState;
    network_state* SharedStates;
    atomic_uint32  WriteIndex;
    atomic_uint32  ReadIndex;
    atomic_uint32  SlotReady[2];
    atomic_uint32  SharedStateCount;
    
};

enum NetworkThreadCommandTypes {
    NETWORK_CMD_EXIT = 0,
    NETWORK_CMD_INIT,
    NETWORK_CMD_SAVE,
    NETWORK_CMD_KICK,
    NETWORK_CMD_BROADCAST,
    NETWORK_CMD_CHAT,
    NETWORK_CMD_CHAT_UDP,
    NETWORK_CMD_PING,
    NETWORK_CMD_STATUS,
    NETWORK_CMD_HELP,
    NETWORK_CMD_CONNECT,
    NETWORK_CMD_CONNECTING,
    NETWORK_CMD_CONNECT_FAILED,
    NETWORK_CMD_CONNECTED,
    NETWORK_CMD_CONNECTED_HANDSHAKE,
    NETWORK_CMD_DISCONNECT,
    NETWORK_CMD_DISCONNECTED,
    NETWORK_CMD_UDP_BROADCAST,
    NETWORK_CMD_UDP_WHISPER,
    NETWORK_CMD_START_HOST,
    NETWORK_CMD_START_CLIENT,
    CLIENT_CONNECTED,
    CLIENT_DISCONNECTED,
    CLIENT_UPDATE,
    CLIENT_INPUT,
    SERVER_ENTITY_STATE,
    SERVER_UPDATE,
    NETWORK_STATE_WRITE, //the network thread copies state to a shared memory location
    NETWORK_STATE_READ, //the main thread notifies the network thread that it has read from that location
    APPEND_TO_PACKET,
    APPEND_TO_ALL_PACKETS,
    SEND_PACKET,
    SEND_ALL_PACKETS,
    NETWORK_LOG_DEBUG,
    NETWORK_LOG_INFO,
    // Add more as needed
};



struct NetworkCommand {
    int type;
    char dataString[MAX_UDP_SIZE];
    uint32_t timeQueued;
    union{

        struct {uint16_t clientID;} udp_whisper;
        struct {uint16_t clientID; uint16_t size;} append_to_packet;
        struct {uint16_t clientID; uint16_t sequence;} send_packet;
        struct {uint16_t client_udp_port;} post_connection_handshake;
        struct {int x,y,z;} test;
        struct {int tcp_portNum, udp_portNum;} connect_to_server;
        struct {player_input playerInput;};
        // struct {entity_state entityState;};
    } data;
};

uint32_t xorshift32(uint32_t& state) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

struct network_context{
    u32 seed;

};

extern atomic_uint32 NetworkSharedWriteIndex;
extern atomic_uint32 NetworkSharedReadIndex;

extern network_context ThreadNetworkContext;
extern NetworkCommand NetworkToMain[256];
extern NetworkCommand toNetwork[256];
extern atomic_uint32 NetworkMainWriteIndex;
extern atomic_uint32 NetworkMainReadIndex;
extern atomic_uint32 NetworkMainCount;
extern atomic_uint32 networkWriteIndex;
extern atomic_uint32 networkReadIndex;
extern atomic_uint32 networkCount;



bool WriteNetworkState(network_thread_state* NetworkThreadState, network_state* InState);
bool TryReadNextNetworkState(network_thread_state* NetworkThreadState, network_state* OutState);
void NetworkPushToMain(NetworkCommand cmd);
bool NetworkPopToMain(NetworkCommand* out);
void PushToNetwork(NetworkCommand cmd);

bool PopToNetwork(NetworkCommand* out);
#if 0
void push_entity_state_to_network(uint32_t entityID,uint32_t inputTime,uint32_t timeProcessed, fpt_quat rotation, fpt_vec3 position, fpt_vec3 pos_in_chunk, fpt_vec3 forward, fpt_vec3 up, fpt_vec3 right, fpt speed, fpt brushSize, fpt angleH, fpt angleV);
void send_entity_state(network_state* NetworkState, entity_state entityState);
void receive_entity_state(network_state* NetworkState, char* buffer);
#endif
/////////////////////////////////// END NETWORKING THREAD QUEUE END ///////////////////////////////////
void append_to_packet(uint16_t clientID, char* buffer, int size);
void append_to_all_packets(char* buffer, int size);
void send_packet(uint16_t clientID);
void send_all_packets(uint16_t sequence);

void network_thread_append_to_packet(network_state* NetworkState, char* buffer, uint16_t size, uint16_t clientID);
void network_thread_send_packet(network_state* NetworkState, uint16_t clientID);





    #ifdef _WIN32
    DWORD WINAPI NetworkThreadProc(LPVOID lpParameter);
    #else
    void* NetworkThreadProc(void* lpParameter);
    #endif
   

