// #ifdef SERVER_BUILD
    // #include "labour/platform_layer/server.h"
// #else
    #include "server.h"
    #include "client.h"
// #endif
#include "packet.h"
#include "netTest.h"
#include "networkProtocol.h"




network_context ThreadNetworkContext;

NetworkCommand NetworkToMain[256];
NetworkCommand toNetwork[256];
atomic_uint32 NetworkMainWriteIndex;
atomic_uint32 NetworkMainReadIndex;
atomic_uint32 NetworkMainCount;
atomic_uint32 networkWriteIndex;
atomic_uint32 networkReadIndex;
atomic_uint32 networkCount;
atomic_uint32 NetworkSharedWriteIndex;
atomic_uint32 NetworkSharedReadIndex;

uint32_t        simNetworkCount;
uint32_t        simWriteIndex;
uint32_t        simReadIndex;
NetworkCommand  SimCommands[256];



bool TryReadNextNetworkState(network_thread_state* NetworkThreadState, network_state* OutState) {
    
    
    return false;
}



bool WriteNetworkState(network_thread_state* NetworkThreadState, network_state* InState) {
    InState->timeCopied = GetTimeMS();
    
    // uint32_t performanceTimer = GetTimeMS();
    uint32_t readIndex  = AtomicRead(&NetworkThreadState->ReadIndex) ; //will these wraparound when they overflow?
    uint32_t writeIndex = AtomicRead(&NetworkThreadState->WriteIndex);

    // uint32_t nextSlotIndex      = (writeIndex+1) % 2;
    // uint32_t currentSlotIndex   = (writeIndex)   % 2;
    // //we only write to the slot if its NOT READY TO READ, NOT READY TO READ == 0
    // if(InterlockedCompareExchange(&NetworkThreadState->SlotReady[currentSlotIndex], 0, 0) == 0){ //main thread not caught up, write to next slot and  increment

    //     memcpy(&NetworkThreadState->SharedStates[(currentSlotIndex)  % 2], InState, sizeof(network_state));
    //     InterlockedCompareExchange(&NetworkThreadState->SlotReady[currentSlotIndex], 1, 0);
    //     AtomicIncrement(&NetworkThreadState->WriteIndex);
    //     // printf(">>>>>>> writing to NEXt slot!!! >>>>>> \n");

    // }
    
    //method 2

    if(readIndex == writeIndex){ //main thread not caught up, write to next slot and  increment
        memcpy(&NetworkThreadState->SharedStates[(writeIndex+1)  % 2], InState, sizeof(network_state));
        AtomicIncrement(&NetworkThreadState->WriteIndex);
        // printf(">>>>>>> writing to NEXt slot!!! >>>>>> \n");

    }
    else{//main thread not caught up, write to slot over and over
        memcpy(&NetworkThreadState->SharedStates[writeIndex      % 2], InState, sizeof(network_state));
        // printf("======= writing to SAME slot!!! ======\n");
    }
    
    // printf("memcpy time: %d\n", GetTimeMS() - performanceTimer);
    //takes 0 to 1 ms to copy over to the buffer
    
    
    return true;
}

void NetworkPushToMain(NetworkCommand cmd) {
    if(NetworkMainCount == 256) return;
    cmd.timeQueued = GetTimeMS();
    
    int idx = AtomicIncrement(&NetworkMainWriteIndex) & 255;
    NetworkToMain[idx] = cmd;
    AtomicIncrement(&NetworkMainCount);
}

bool NetworkPopToMain(NetworkCommand* out) {
    if(NetworkMainCount == 0) return false;
    *out = NetworkToMain[AtomicIncrement(&NetworkMainReadIndex) & 255];
    AtomicDecrement(&NetworkMainCount);
    return true;
}

//both operated from the thread, dont need to be atomic, used to simulate the network receiving commands from the game layer
void simPushToNetwork(NetworkCommand cmd){
    if(simNetworkCount == 256){
        printf("too many network commands, returning\n");
        return;
    }
    cmd.timeQueued = GetTimeMS();
    

    simWriteIndex  = (simWriteIndex + 1) & 255;

    SimCommands[simWriteIndex] = cmd;
    simNetworkCount++;
}

bool simPopToNetwork(NetworkCommand* out){
    if(simNetworkCount == 0) return false;
    networkReadIndex = (networkReadIndex + 1) & 255;
    *out = SimCommands[networkReadIndex];
    simNetworkCount--;
    return true;
}


void PushToNetwork(NetworkCommand cmd) {
    // printf("network count: %d\n", networkCount);
    if(networkCount == 256){
        printf("too many network commands, returning\n");
        return;
    }
    cmd.timeQueued = GetTimeMS();
    
    int idx = AtomicIncrement(&networkWriteIndex) & 255;

    toNetwork[idx] = cmd;
    AtomicIncrement(&networkCount);
}

bool PopToNetwork(NetworkCommand* out) {
    if(networkCount == 0) return false;
    *out = toNetwork[AtomicIncrement(&networkReadIndex) & 255];
    AtomicDecrement(&networkCount);
    return true;
}
void append_to_packet(uint16_t clientID, char* buffer, int size){
    NetworkCommand netCmd = {};
    netCmd.type = NetworkThreadCommandTypes::APPEND_TO_PACKET;
    strcpy(netCmd.dataString, buffer);
    netCmd.data.append_to_packet.size = size;
    netCmd.data.append_to_packet.clientID = clientID;
    PushToNetwork(netCmd);
}

void append_to_all_packets(char* buffer, int size){
    NetworkCommand netCmd = {};
    netCmd.type = NetworkThreadCommandTypes::APPEND_TO_ALL_PACKETS;
    strcpy(netCmd.dataString, buffer);
    netCmd.data.append_to_packet.size = size;
    //clientID field is left null since we append to all packets for all clients
    PushToNetwork(netCmd);
}

void send_packet(uint16_t clientID){
    NetworkCommand netCmd = {};
    netCmd.type = NetworkThreadCommandTypes::SEND_PACKET;
    netCmd.data.send_packet.clientID = clientID;
    PushToNetwork(netCmd);

}

void send_all_packets(uint16_t sequence){
    NetworkCommand netCmd = {};
    netCmd.type = NetworkThreadCommandTypes::SEND_ALL_PACKETS;
    netCmd.data.send_packet.sequence = sequence;
    PushToNetwork(netCmd);
}

#if 0 
void push_entity_state_to_network(uint32_t entityID,uint32_t inputTime, uint32_t timeProcessed,fpt_quat rotation, fpt_vec3 position, fpt_vec3 pos_in_chunk, fpt_vec3 forward, fpt_vec3 up, fpt_vec3 right, fpt speed, fpt brushSize, fpt angleH, fpt angleV){
    NetworkCommand netCmd = {};
    netCmd.type = NetworkThreadCommandTypes::SERVER_ENTITY_STATE;

    netCmd.data.entityState.entityID = entityID;
    netCmd.data.entityState.inputTime = inputTime;
    netCmd.data.entityState.timeProcessed = timeProcessed;
    netCmd.data.entityState.rotation = rotation;
    netCmd.data.entityState.position = position;
    netCmd.data.entityState.pos_in_chunk = pos_in_chunk;
    netCmd.data.entityState.forward = forward;
    netCmd.data.entityState.up = up;
    netCmd.data.entityState.right = right;
    netCmd.data.entityState.speed = speed;
    netCmd.data.entityState.brushSize = brushSize;
    netCmd.data.entityState.angleH = angleH;
    netCmd.data.entityState.angleV = angleV;

    PushToNetwork(netCmd);

}
#endif

void network_thread_append_to_packet(network_state* NetworkState, char* buffer, uint16_t size, uint16_t clientID){
    if(!NetworkState->enabled)return;


    PacketManager& pm = NetworkState->packetManager;
    
    uint16_t& bufferSize = pm.current_buffer_size[clientID];
    
    if(bufferSize + size < (uint16_t)MAX_UDP_SIZE){
        memcpy(pm.current_buffer[clientID] + bufferSize, buffer, size);
        bufferSize += size;
    }else{
        printf("STRING TOO LONG, CANT APPEND\n");
    }
    
}

void network_thread_send_packet(network_state* NetworkState, uint16_t clientID){
    // PacketManager& pm = NetworkState->packetManager;

    // int total_sent = 0;
    // packet_header header = {};
    // header.size = strlen(pm.buffer[clientID]);
    // packet_manager_send_binary_udp(NetworkState, NetworkState->serverSockets.udp_socket, (unsigned char*)pm.buffer[clientID], (sockaddr*)&NetworkState->clientSockets.server_addr_udp, sizeof(NetworkState->clientSockets.server_addr_udp), &total_sent, header, clientID);
}

//entity_state is now a struct in the entity component setup
#if 0
void send_entity_state(network_state* NetworkState, entity_state entityState){

        uint16_t sequence = NetworkState->pingTracker.next_sequence;
        uint16_t magicNumber = 0x5005;//soos
        
        int total_bytes_sent = 0;
        unsigned char buffer[256];


        int packetsize = pack(buffer, "LLLLLLLLLLLLLLLLLLLLLLLLLL",   entityState.entityID, 
                                                                    entityState.inputTime,
                                                                    entityState.timeProcessed, 
                                                                    entityState.rotation.x, 
                                                                    entityState.rotation.y, 
                                                                    entityState.rotation.z, 
                                                                    entityState.rotation.w,

                                                                    entityState.position.x, 
                                                                    entityState.position.y, 
                                                                    entityState.position.z,

                                                                    entityState.pos_in_chunk.x, 
                                                                    entityState.pos_in_chunk.y, 
                                                                    entityState.pos_in_chunk.z,

                                                                    entityState.forward.x,
                                                                    entityState.forward.y,
                                                                    entityState.forward.z,

                                                                    entityState.up.x, 
                                                                    entityState.up.y, 
                                                                    entityState.up.z,

                                                                    entityState.right.x, 
                                                                    entityState.right.y, 
                                                                    entityState.right.z,

                                                                    entityState.speed, 
                                                                    entityState.brushSize,
                                                                    entityState.angleH,
                                                                    entityState.angleV);


        // printf("sending sequence: %d, time: %" UINT64_FORMAT "\n", sequence, now);



        int result = send_binary_udp(NetworkState->serverSockets.udp_socket,
            MessageTypes::ENTITY_STATE, 
            (unsigned char*)buffer,
            packetsize,
            (sockaddr*)&NetworkState->clientManager.udp_addrs[0],
            sizeof(sockaddr_in),
            &total_bytes_sent);
            // printf("sent ping to server, size header: %" UINT64_FORMAT " , size msg: %" UINT64_FORMAT " , total bytes: %" UINT64_FORMAT " , total_sent: %d\n",sizeof(MessageHeader) , strlen(cmd.dataString) + 1, sizeof(MessageHeader) + strlen(cmd.dataString) + 1, total_bytes_sent);
        //pack and send: sequence, NOW, magic number
        // NetworkState->pingTracker.next_sequence++;
        
}


void receive_entity_state(network_state* NetworkState, char* buffer){
    entity_state entityState = {};

    unpack((unsigned char*) buffer, "LLLLLLLLLLLLLLLLLLLLLLLLLL",   &entityState.entityID, 
                                                                &entityState.inputTime,
                                                                &entityState.timeProcessed,
                                                                &entityState.rotation.x, 
                                                                &entityState.rotation.y, 
                                                                &entityState.rotation.z, 
                                                                &entityState.rotation.w,

                                                                &entityState.position.x, 
                                                                &entityState.position.y, 
                                                                &entityState.position.z,

                                                                &entityState.pos_in_chunk.x, 
                                                                &entityState.pos_in_chunk.y, 
                                                                &entityState.pos_in_chunk.z,

                                                                &entityState.forward.x,
                                                                &entityState.forward.y,
                                                                &entityState.forward.z,

                                                                &entityState.up.x, 
                                                                &entityState.up.y, 
                                                                &entityState.up.z,

                                                                &entityState.right.x, 
                                                                &entityState.right.y, 
                                                                &entityState.right.z,

                                                                &entityState.speed, 
                                                                &entityState.brushSize,
                                                                &entityState.angleH,
                                                                &entityState.angleV);

        entityState.received = true;
        // entityState.timeReceived = GetTimeMS();
        entityState.timeReceived = GetTimeMS()/*REPLACE*/;

        NetworkState->entityState = entityState;
        
}
#endif



        void hostProcessNetworkCommand(network_state* NetworkState, NetworkCommand cmd, bool& commandProcessed){
            int total_bytes_sent = 0;
            int result;
            char state_msg[1024];
            const char* msgType = " -INFO- ";
            switch(cmd.type) {
                case NETWORK_CMD_BROADCAST:
                    printf("BROADCAST NOT SETUP\n");
                    //all of these were commented out due to possibly overflowing te state_msg buffer
                    // sprintf(state_msg, "%s Server Broadcast: %s",msgType, cmd.dataString);
                    //printf("%s\n", state_msg);
                    // broadcast_tcp_message(NetworkState, MessageTypes::MSG_BROADCAST, state_msg, strlen(state_msg) + 1);
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_PING:
                    printf("ping not yet setup...\n");
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_UDP_BROADCAST:
                    printf("BROADCAST NOT SETUP\n");

                    // sprintf(state_msg, "%s Server UDP Broadcast: %s",msgType, cmd.dataString);
                    //printf("%s\n", state_msg);
                    // broadcast_udp_message(NetworkState, state_msg);

                    commandProcessed = true;
                    break;

                case NETWORK_CMD_UDP_WHISPER:
                    printf("WHISPER NOT SETUP\n");
                    // sprintf(state_msg, "%s Server UDP Whisper: %s",msgType, cmd.dataString);
                    //printf("%s\n", state_msg);
                    // total_bytes_sent = 0;
                    // result = send_message_udp(NetworkState->serverSockets.udp_socket,
                    // MessageTypes::MSG_UDP_WHISPER, 
                    // state_msg,
                    // strlen(state_msg) + 1,
                    // (sockaddr*)&NetworkState->clientManager.udp_addrs[cmd.data.udp_whisper.clientID],
                    // sizeof(sockaddr_in),
                    // &total_bytes_sent);

                    // if(result != 0){
                    //     printf("error sending udp whisper\n");
                    // }

                    commandProcessed = true;
                    break;

                case NETWORK_CMD_INIT:
                    setup_sockets(NetworkState);
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_DISCONNECT:{

                    printf("DISCONNECTING FROM SOCKET, RESTARTING THREAD\n");
                    #ifndef SERVER_BUILD
                    disconnect_client(NetworkState);
                    #endif
                    cleanup_server_sockets(NetworkState);
                    
                    NetworkCommand netcmd = {};
                    netcmd.type = NetworkThreadCommandTypes::NETWORK_CMD_DISCONNECTED;
                    NetworkPushToMain(netcmd);

                    commandProcessed = true;
                    break;
                }
                    
                case NETWORK_CMD_START_HOST:{
                    #ifndef SERVER_BUILD
                    disconnect_client(NetworkState);
                    #endif
                    cleanup_server_sockets(NetworkState);
                    setup_sockets(NetworkState);
                    commandProcessed = true;
                    break;
                }
                
                case SERVER_ENTITY_STATE:
                    // send_entity_state(NetworkState, cmd.data.entityState);
                    printf("REMOVED PLAYER STATE FROM NETWORK, SEND PLAYER INPUT FAILED!\n");

                    break;

                    
                case APPEND_TO_ALL_PACKETS:{
                    if(NetworkState->netTest.enabled)break;

                    for(int clientID = 0; clientID < NetworkState->clientManager.numClients; clientID++){
                        network_thread_append_to_packet(NetworkState, cmd.dataString, cmd.data.append_to_packet.size, clientID);
                    }
                }
                    break;
                
                case APPEND_TO_PACKET:{
                    network_thread_append_to_packet(NetworkState, cmd.dataString, cmd.data.append_to_packet.size, cmd.data.append_to_packet.clientID);
                    
                }
                    break;
                
                case SEND_PACKET:{
                    // network_thread_send_packet(NetworkState, cmd.data.send_packet.clientID);
                    PacketManager& pm = NetworkState->packetManager;
                    uint16_t clientID = cmd.data.send_packet.clientID;
                    int total_sent = 0;
                    // packet_manager_send_binary_udp(NetworkState, NetworkState->serverSockets.udp_socket,(sockaddr*)&NetworkState->clientManager.udp_addrs[clientID], sizeof(sockaddr_in), &total_sent, clientID);
                        
                }
                    break;

                case SEND_ALL_PACKETS:{
                    if(NetworkState->netTest.enabled)break;
                    NetworkState->local_sequence = cmd.data.send_packet.sequence;
                    PacketManager& pm = NetworkState->packetManager;


                    check_in_flight_packets(NetworkState, NetworkState->connectionState[0]);

                    //send everything in the send buffers or try to until we hit congestion limit
                    // int clientID = 0;
                    // while(pm.queue_total_count && pm.bandwidth_accumulator > 0){
                        
                    //     // pm.current_buffer[clientID];

                    //     int total_sent = 0;
                    //     uint8_t& writeIndex =   pm.queue_write_index[clientID]; 
                    //     uint8_t& readIndex =    pm.queue_read_index[clientID];
                    //     uint8_t& count =        pm.queue_count[clientID];

                    //     if(count > 0 && readIndex != writeIndex){
                    //         char* payload = pm.send_queue[clientID][readIndex];
                    //         packet_header header = pm.header_queue[clientID][readIndex];
                    //         packet_manager_send_binary_udp(NetworkState, NetworkState->serverSockets.udp_socket, header, payload, (sockaddr*)&NetworkState->clientManager.udp_addrs[clientID], sizeof(sockaddr_in), &total_sent, clientID);

                    //         //move to the next client and send a packet for them
                    //         count--;
                    //         pm.queue_total_count--;
                    //         readIndex = (readIndex + 1) & (SEND_QUEUE_SIZE - 1);
                    //     }
        
                    //     clientID = (clientID + 1) & (MAX_CLIENTS - 1);
                    // }

                 
                    for(int clientID = 0; clientID < NetworkState->clientManager.numClients; clientID++){



                        // network_thread_send_packet(NetworkState, cmd.data.send_packet.clientID);
                        PacketManager& pm = NetworkState->packetManager;
                        int total_sent = 0;
                        packet_manager_send_binary_udp(NetworkState, NetworkState->serverSockets.udp_socket, (sockaddr*)&NetworkState->clientManager.udp_addrs[clientID], sizeof(sockaddr_in), &total_sent, clientID);
                    }
                }
                    break;
                case NETWORK_LOG_DEBUG:
                    NetworkState->logger.lowest_printed_type = LogTypes::Debug;
                    break;
                
                case NETWORK_LOG_INFO:
                    NetworkState->logger.lowest_printed_type = LogTypes::Info;
                    break;
            }
        }


// #ifndef SERVER_BUILD




            
        void clientProcessNetworkCommand(network_state* NetworkState, NetworkCommand cmd, bool& commandProcessed){
            int result;
            char commandStr[256];
            int total_bytes_sent = 0;
            char handshakeStr[256] = "TEST HANDSHAKE";
            switch(cmd.type) {
                case NETWORK_CMD_BROADCAST:
                    printf("Network Broadcast: %s\n", cmd.dataString);
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_CONNECT:
                    NetworkState->clientSockets.udp_port = cmd.data.connect_to_server.udp_portNum;
                    setup_server_connection(NetworkState, cmd.dataString, cmd.data.connect_to_server.udp_portNum);
                    connect_to_server(NetworkState);
                    commandProcessed = true;
                    
                    break;
                case NETWORK_CMD_CONNECTED_HANDSHAKE:
                    //send handshake immediately after connecting instead
                    // if(send_message_tcp(NetworkState->clientSockets.tcp_socket, MessageTypes::MSG_CONNECTED_HANDSHAKE, (const char*)NetworkState->clientSockets.udp_port, sizeof(NetworkState->clientSockets.udp_port), &total_bytes_sent) == 0){
                    //     printf("sent connected handshake to server, size header: %" UINT64_FORMAT " size msg: %" UINT64_FORMAT " , total bytes: %" UINT64_FORMAT " , total_sent: %d\n",sizeof(MessageHeader) , sizeof(NetworkState->clientSockets.udp_port), sizeof(MessageHeader) + sizeof(NetworkState->clientSockets.udp_port), total_bytes_sent);
                    // }
                    printf("CHAT TCP DISABLED\n");
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_CHAT:
                    // if(send_message_tcp(NetworkState->clientSockets.tcp_socket, MessageTypes::MSG_CHAT, cmd.dataString, strlen(cmd.dataString) + 1, &total_bytes_sent) == 0){
                    // printf("sent chat message to server, size header: %" UINT64_FORMAT " , size msg: %" UINT64_FORMAT " , total bytes: %" UINT64_FORMAT " , total_sent: %d\n",sizeof(MessageHeader) , strlen(cmd.dataString) + 1, sizeof(MessageHeader) + strlen(cmd.dataString) + 1, total_bytes_sent);
                    // }
                    printf("CHAT TCP DISABLED\n");
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_CHAT_UDP:
                    if(send_message_udp(NetworkState->clientSockets.udp_socket, MessageTypes::MSG_CHAT_UDP, cmd.dataString, strlen(cmd.dataString) + 1, (sockaddr*)&NetworkState->clientSockets.server_addr_udp, sizeof(NetworkState->clientSockets.server_addr_udp), &total_bytes_sent) == 0){
                    printf("sent chat message to server, size header: %" UINT64_FORMAT " , size msg: %" UINT64_FORMAT " , total bytes: %" UINT64_FORMAT " , total_sent: %d\n",sizeof(MessageHeader) , strlen(cmd.dataString) + 1, sizeof(MessageHeader) + strlen(cmd.dataString) + 1, total_bytes_sent);
                    }
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_DISCONNECT:{
                    send_disconnect(NetworkState);
                    printf("disconnecting from server\n");
                    disconnect_client(NetworkState);
                    cleanup_server_sockets(NetworkState);

                    NetworkCommand netcmd = {};
                    netcmd.type = NetworkThreadCommandTypes::NETWORK_CMD_DISCONNECTED;
                    NetworkPushToMain(netcmd);

                    commandProcessed = true;
                    break;
                }
                
                case NETWORK_CMD_INIT:
                    init_client_sockets(NetworkState);
                    commandProcessed = true;
                    break;

                case NETWORK_CMD_START_CLIENT:
                    disconnect_client(NetworkState);
                    cleanup_server_sockets(NetworkState);
                    init_client_sockets(NetworkState);
                    commandProcessed = true;
                    break;
                case CLIENT_INPUT:
                    if(NetworkState->clientSockets.isConnected){
                        printf("REMOVED PLAYER STATE FROM NETWORK, SEND PLAYER INPUT FAILED!\n");
                        // send_player_input(NetworkState, &cmd.data.playerInput);
                    }
                    break;
                
                case SERVER_ENTITY_STATE:
                    if(NetworkState->clientSockets.isConnected){
                        printf("REMOVED PLAYER STATE FROM NETWORK, SEND PLAYER INPUT FAILED!\n");
                        // send_entity_state(NetworkState, cmd.data.entityState);
                    }        
                    break;
                case APPEND_TO_PACKET:{
                    if(NetworkState->clientSockets.isConnected){
                        network_thread_append_to_packet(NetworkState, cmd.dataString, cmd.data.append_to_packet.size, cmd.data.append_to_packet.clientID);
                    }
                }
                    break;
                
                case SEND_PACKET:{
                    if(NetworkState->clientSockets.isConnected){
                        // network_thread_send_packet(NetworkState, cmd.data.send_packet.clientID);
                         PacketManager& pm = NetworkState->packetManager;
                        uint16_t clientID = 0;//just use 0 since the client only ever communicates with the server
                        int total_sent = 0;
                        // packet_manager_send_binary_udp(NetworkState, NetworkState->clientSockets.udp_socket, (sockaddr*)&NetworkState->clientSockets.server_addr_udp, sizeof(NetworkState->clientSockets.server_addr_udp), &total_sent, clientID, NetworkState->clientSockets.token);
                    }
                }
                    break;
                case SEND_ALL_PACKETS:{
                    //loop through all clientIDs and check if we can send their buffer
                    NetworkState->local_sequence = cmd.data.send_packet.sequence;
                    //then check all in flight packets

                    check_in_flight_packets(NetworkState, NetworkState->connectionState[0]);


                    // network_thread_send_packet(NetworkState, cmd.data.send_packet.clientID);
                    PacketManager& pm = NetworkState->packetManager;
                    uint16_t clientID = 0;//just use 0 since the client only ever communicates with the server
                    int total_sent = 0;
                    packet_manager_send_binary_udp(NetworkState, NetworkState->clientSockets.udp_socket, (sockaddr*)&NetworkState->clientSockets.server_addr_udp, sizeof(NetworkState->clientSockets.server_addr_udp), &total_sent, clientID, NetworkState->clientSockets.token);




                }
                    break;

            } 
        }
      

// #endif

  #ifdef _WIN32
            DWORD WINAPI NetworkThreadProc(LPVOID lpParameter)
        #else
        void* NetworkThreadProc(void* lpParameter)
        #endif
        {

            int sim_second = 50;
            int ticker = 0;
            // performance_timer::Timer serverTimer;
            // performance_timer::Timer clientTimer;

            network_state* threadState = (network_state*)malloc(sizeof(network_state));
            memset(threadState, 0, sizeof(network_state));

            network_thread_state* ThreadState = (network_thread_state*)lpParameter;
            // network_state* NetworkState = ThreadState->NetworkState;
            network_state* NetworkState = threadState;
            
            // NetworkState->packetManager.bandwidth_accumulator = TARGET_BANDWIDTH;
            NetworkState->total_bandwidth_accumulator = TARGET_BANDWIDTH;
            NetworkState->local_time_ms = GetTimeMS()/*REPLACE*/;
            
            // NetworkState->packetManager.local_time_ms = GetTimeMS();

            NetworkState->packetManager.local_time_ms = GetTimeMS()/*REPLACE*/;
            
            // NetworkState->netTest.local_time_ms = GetTimeMS();
            NetworkState->netTest.local_time_ms = GetTimeMS()/*REPLACE*/;
            
            //simulates network/actives test suite
            NetworkState->netTest.enabled = true;
            NetworkState->netTest.tick_timestep_ms = 16;
            // NetworkState->netTest.tick_timestep_ms = 16;
            NetworkState->netTest.packet_loss_percentage = 10; //out of 100
            bool increment_drop_rate = true;
            debug_logger_open_file(&NetworkState->logger, "network_log.txt", true, LogTypes::Info);
            log_to_console(NetworkState, LogTypes::Info, "NETWORK LOG START");


            // int x = 0;
            // LOG_INFO(&NetworkState->logger, "var test %d", x);
            // LOG_TRACE(&NetworkState->logger, "test");

            // basic test that creates a simulated entity and sends a simulated connection message
            sim_test(NetworkState);

                while(!ShouldThreadsExit) {
                    if(!NetworkState){
                        printf("CLIENT THREAD NETWORK STATE IS NULL! BREAK OUT\n");
                        break;
                    }

                    BandwidthBudget(NetworkState, NetworkState->local_time_ms, NetworkState->total_bandwidth_accumulator);

                    // entity_state entityState = {};
                    // NetworkState->entityState = entityState;


                    NetworkState->currentTick = NetworkState->tick;
                    NetworkCommand cmd = {};
                    // NetworkState->currentTime = GetTimeMS();
                    NetworkState->currentTime = GetTimeMS()/*REPLACE*/;
                    bool stopProcessing = false;
                    while((PopToNetwork(&cmd) || simPopToNetwork(&cmd)) && !stopProcessing) {
                        bool commandProcessed = false;
                        if(NetworkState->serverSockets.active){
                            hostProcessNetworkCommand(NetworkState, cmd, commandProcessed);
                        } 
                        else if(NetworkState->clientSockets.active){
                            clientProcessNetworkCommand(NetworkState, cmd, commandProcessed);
                        }
                        if(!commandProcessed){
                            switch(cmd.type) {
                                case NETWORK_CMD_START_HOST:
                                    cleanup_server_sockets(NetworkState);
                                    disconnect_client(NetworkState);
                                    setup_sockets(NetworkState);
                                    stopProcessing = true;
                                    break;
                                case NETWORK_CMD_START_CLIENT:
                                    cleanup_server_sockets(NetworkState);
                                    disconnect_client(NetworkState);
                                    init_client_sockets(NetworkState);
                                    stopProcessing = true;
                                    break;
                            }
                        }

                    }
                
                    if(NetworkState->clientSockets.active){
                        //if we are trying to connect to the server, send a connection request periodically
                        // if(!NetworkState->clientSockets.isConnected && NetworkState->clientSockets.connecting){
                        //     if(!NetworkState){
                        //         printf("NETWORKS STATE IS NULL???\n");
                        //     }
                        //     else{
                        //         connect_to_server(NetworkState);
                        //         if(ShouldThreadsExit){
                        //             printf("client tried to connect to server, but thread must exit, sloppy but it should work!\n");
                        //             return 0;
                        //         }
                        //     }
                        // }

                        // uint64_t now = GetTimeMS();
                        uint64_t now = GetTimeMS()/*REPLACE*/;
                        
                        bool stopPingTesting = false;
                        if( stopPingTesting && NetworkState->clientSockets.isConnected){
                            // Send ping every 500ms
                            PingTracker& pingTracker = NetworkState->pingTracker; 
                            if (now - pingTracker.last_ping_time >= 500) {
                                send_ping(NetworkState);
                                pingTracker.last_ping_time = now;
                            }
                            // Print stats every second
                            if (now - pingTracker.last_stats_time >= 1000) {
                                printf("RTT: %.2fms, Lost: %d\n", 
                                pingTracker.latest_rtt,
                                pingTracker.lost_packets);
                                pingTracker.last_stats_time = now;
                            }
                        }
    
                        if(poll_client_messages(NetworkState) == -1){  // Check for responses
                            break;
                        }
                        

                        NetworkState->clientSockets.tick++;
                    }

                    else if(NetworkState->serverSockets.active){
                        if(ShouldThreadsExit){
                            printf("SERVER THREAD SHOULD EXIT! RETURNING\n");
                            break;
                        }

                        player_input playerInput = {};
                        for(int i = 0; i < NetworkState->clientManager.numClients; i++){
                            NetworkState->playerInputs[i][0][(NetworkState->currentTick) & (SNAPSHOT_BUFFER_SIZE - 1)] = playerInput;
                        }


                        if(poll_network_messages(NetworkState) == -1){
                            break;
                        }
                    
          
                        NetworkState->clientManager.tick++;
                    }

                    // if(NetworkState->enabled){ //maybe the memcpy was slowing us down?
                    if(NetworkState->enabled && !NetworkState->netTest.enabled){
                        WriteNetworkState(ThreadState, NetworkState);
                        NetworkState->tick++;
                    }
                    // else if (!NetworkState->netTest.enabled){
                    else{
                        //inactive, sleep for 16ms
                        Sleep(16);
                    }


                    if(NetworkState->enabled && NetworkState->netTest.enabled){

                        // serverTimer.reset();

                        NetTest& nt = NetworkState->netTest;
                        // uint32_t now = GetTimeMS();
                        uint32_t now = GetTimeMS()/*REPLACE*/;

                        uint32_t diff = now - nt.local_time_ms;
                        // if(ticker > sim_second){
                        
                        if(diff > 0){

                            nt.local_time_ms = now;

                            nt.tick_accumulator += diff;
                            //uncomment this, was experimenting with artificial timing
                            while(nt.tick_accumulator > nt.tick_timestep_ms){ //simulates game tick

                                //slop to vary the drop rate
                                // if(increment_drop_rate && NetworkState->netTest.packet_loss_percentage < 100)NetworkState->netTest.packet_loss_percentage++;
                                // if(NetworkState->netTest.packet_loss_percentage >= 100)increment_drop_rate = false;
                                // if(!increment_drop_rate && NetworkState->netTest.packet_loss_percentage > 0)NetworkState->netTest.packet_loss_percentage--;
                                // if(NetworkState->netTest.packet_loss_percentage <= 0)increment_drop_rate = true;


                                //the server should check for incoming messages right here, just to update its sequence/acks reponses to clients
                                while(nt.server_waiting_messages_total > 0 && receive_packet(NetworkState, false) == 0){

                                }
                                            


                                int total_bytes_sent = 0;
                                char buffer[16];
                                packet_header header = {};
                                packet_entry entry = {};
                                entry.type = MessageTypes::SIMULATION;
                                entry.size = 3;
                                int packetsize = pack((unsigned char*) buffer, "CH", entry.type, entry.size);

                                header.size = entry.size;

            
                                for(int clientID = 0; clientID < NetworkState->clientManager.numClients; clientID++){

                                    ConnectionState& cs = NetworkState->connectionState[clientID];
                                    BandwidthBudget(NetworkState, cs.local_time_ms, cs.bandwidth_accumulator);

                                    log_connection_state(NetworkState, cs, clientID);

                                    sim_append_to_packet(NetworkState, cs, buffer, packetsize);



                                    if(!cs.connection_handshake_completed){//need to put this in every message until we know the client has gotten it and knows its connected
                                        entry.type = MessageTypes::MSG_CONNECTED_HANDSHAKE;
                                        entry.size = 5;
                                        packetsize = pack((unsigned char*) buffer, "CHH", entry.type, entry.size, NetworkState->clientManager.IDToToken[clientID]);
                                        sim_append_network_command(NetworkState, cs, buffer, packetsize);
                                    }


                                    int total_sent = 0;
                                    int socket = cs.socket;//dont need to worry about this for the simulation
                                    sockaddr* addr = (sockaddr*)&cs.socket_udp_addr;
                                    socklen_t addr_len = sizeof(sockaddr_in);
                                    uint16_t token = 0;
                                    // ConnectionState& cs = nt.server_connection_state[clientID];

                                    //then check all in flight packets
                                    check_in_flight_packets(NetworkState, cs, clientID);
                                    bool is_server = true;

                                    if(cs.need_resync){
                                        // LOG_ERROR(NetworkState, "%s NEED TO RESYNC WITH CLIENT!", SERVER_STR);
                                        // LOG_ERROR(NetworkState, "%s NEED TO RESYNC WITH CLIENT!", SERVER_STR);
                                        // LOG_ERROR(NetworkState, "%s NEED TO RESYNC WITH CLIENT!", SERVER_STR);
                                        entry.type = MessageTypes::RESYNC_DUMMY;
                                        entry.size = 3;
                                        packetsize = pack((unsigned char*) buffer, "CHH", entry.type, entry.size);
                                        sim_append_network_command(NetworkState, cs, buffer, packetsize);
                                        cs.need_resync = false; //is sending once and resetting the bool good enough? I suppose if we overwrite again it will desync again
                                        cs.times_resynced++; //like we try resending sync info, and if it gets through, great, if not, we just set the bool and try again
                                    }


                                    if(cs.received_slice_this_tick){

                                        print_receive_chunk_bitfield(NetworkState, cs);



                                        cs.received_slice_this_tick = false;
                                        memset(cs.scratch_entry_buffer, 0, MAX_UDP_SIZE);
                                        size_t size_in_bytes = sizeof(uint8_t) + sizeof(uint16_t) + (sizeof(uint64_t) * 4);
                                        assert(size_in_bytes == 35 && "INCORRECT NUMBER OF BYTES IN ENTRY"); 
                                        int entrysize = pack((unsigned char*)cs.scratch_entry_buffer, "CHQQQQ", MessageTypes::CHUNK_ACK_BITFIELD, size_in_bytes, cs.receive_chunk_bitfield[0], cs.receive_chunk_bitfield[1], cs.receive_chunk_bitfield[2], cs.receive_chunk_bitfield[3]);
                                        int result = sim_append_network_command(NetworkState, cs, cs.scratch_entry_buffer, entrysize);
                                        memset(cs.scratch_entry_buffer, 0, MAX_UDP_SIZE);
                                    }



                                    char end_buffer[16];
                                    int entrysize = pack((unsigned char*)end_buffer, "CH", MessageTypes::END_NET_CMDS, 3);
                                    // int result = sim_append_to_packet(NetworkState, cs, cs.scratch_buffer, entrysize);
                                    int result = sim_append_network_command(NetworkState, cs, end_buffer, entrysize);

                                    send_all_queued_data(NetworkState, socket, addr, addr_len, &total_sent, clientID, token, cs, is_server);


                                }

                                // clientTimer.reset();

                                //update the clients
                                sim_test_update(NetworkState);

                                // printf("client took: %f seconds\n",clientTimer.elapsed());
                                // clientTimer.stop();
                                
                                // nt.tick_accumulator -= nt.tick_timestep_ms;
                                nt.tick_accumulator = 0 ;//reset to 0 so we dont accumulate tons of ticks if we stop in the debugger
                                ticker = 0;
                            }
                            // printf("server took: %f seconds\n",serverTimer.elapsed());
                            // serverTimer.stop();
                                


                        }
                     

                        #ifdef _WIN32
                            //dont sleep at all during verbose logging
                            Sleep(1);  // Windows sleep takes milliseconds, minimum is typically 1ms
                        #else
                            usleep(600);  // POSIX usleep takes microseconds
                        #endif
                    }
                    ticker++;
                    NetworkState->sim_time++;
                }
                debug_logger_close_file(&NetworkState->logger);
                printf("cleaning up thread state\n");
                free(threadState);
                return 0;  

        }




