#include "server.h"
#include "networkProtocol.h"
#include "packet.h"
#include "netTest.h"

    struct ClientMap {
        uint32_t clientId;
        int arrayIndex;
    };    
    
    struct PingPacket {
        uint32_t timestamp;     // Time sent
        uint32_t clientId;      // To track multiple clients
    };

    struct ClientPingData {
        bool awaitingResponse;
        uint64_t lastPingTime;
        uint32_t latency;
    };

    struct ServerPingTracker{
        ClientPingData clients[MAX_CLIENTS];
        uint32_t lastPingBroadcast;
    };



    ConsoleCommand ConsoleToMain[256];
    ConsoleCommand toConsole[256];
    atomic_uint32 ConsoleMainWriteIndex;
    atomic_uint32 ConsoleMainReadIndex;
    atomic_uint32 ConsoleMainCount;
    atomic_uint32 consoleWriteIndex;
    atomic_uint32 consoleReadIndex;
    atomic_uint32 consoleCount;


    ClientMap clientIdToIndex[MAX_CLIENTS];

    void initClientManager(network_state* NetworkState){

        // Reset TCP and UDP connection info
        memset(NetworkState->clientManager.udp_addrs,         0, sizeof(sockaddr_in)  * MAX_CLIENTS);
        memset(NetworkState->clientManager.client_udp_ports,  0, sizeof(uint16_t)     * MAX_CLIENTS);
        
        // Reset read buffers
        ReliableUDP emptyReliableUDP = {};
        for(int i = 0; i < MAX_CLIENTS; i++) {
            NetworkState->clientManager.readBuffers[i] = {}; // Zero-initialize the read buffer struct
            // NetworkState->reliableUDP[i] = {}; //does this work as well as memset?
            // memcpy((void*)&NetworkState->reliableUDP[i], (const void*)&emptyReliableUDP, sizeof(ReliableUDP));
        }
        
        // Reset networking state
        memset(NetworkState->clientManager.last_ping_times,   0,          sizeof(uint64_t) * MAX_CLIENTS);
        memset(NetworkState->clientManager.freeClientIDs,     0,          sizeof(uint8_t) * MAX_CLIENTS);
        memset(NetworkState->clientManager.last_sequences,    0,          sizeof(uint16_t) * MAX_CLIENTS);
        memset(NetworkState->clientManager.tokenToID,         0xFF, sizeof(uint16_t) * (UINT16_MAX+1) );
        memset(NetworkState->clientManager.IDToToken,         0xFF,     sizeof(uint16_t) * MAX_CLIENTS);  // Set to invalid token (65535)

        
        // Reset timing information
        memset(NetworkState->clientManager.last_client_times,    0, sizeof(uint64_t) * MAX_CLIENTS);
        memset(NetworkState->clientManager.server_receive_times, 0, sizeof(uint64_t) * MAX_CLIENTS);
        memset(NetworkState->clientManager.client_clock_offsets, 0, sizeof(uint64_t) * MAX_CLIENTS);
        NetworkState->clientManager.numClients = 0;
        // std::random_device rd;
        // ThreadNetworkContext.rng.seed(rd());

        NetworkState->clientManager.freeClientIDCount = 0;
    }

    int removeClient(network_state* NetworkState, uint8_t clientID){

        if(NetworkState->clientManager.numClients <= 0){
            printf("NETWORK ERROR: min clients %d reached!, can't remove client??\n", NetworkState->clientManager.numClients);
            return -1;
        }

        uint32_t lastIndex = NetworkState->clientManager.numClients - 1;
        uint16_t token  = NetworkState->clientManager.IDToToken[clientID];
   
        NetworkState->clientManager.IDToToken[clientID] = UINT16_MAX;
        NetworkState->clientManager.tokenToID[token] = UINT16_MAX;

        if(lastIndex != clientID && lastIndex != 0){
            uint32_t removedIndex = clientID;
            
            NetworkState->clientManager.udp_addrs[removedIndex] =  NetworkState->clientManager.udp_addrs[lastIndex];
            NetworkState->clientManager.IDToToken[removedIndex] = NetworkState->clientManager.IDToToken[lastIndex];
            // NetworkState->reliableUDP[removedIndex] = NetworkState->reliableUDP[lastIndex];
            
            uint16_t lastToken = NetworkState->clientManager.IDToToken[lastIndex];
            NetworkState->clientManager.IDToToken[removedIndex] = lastToken;
            NetworkState->clientManager.tokenToID[lastToken] = removedIndex;
            NetworkState->clientManager.tokenToID[lastIndex] = UINT16_MAX;
        }

        NetworkState->clientManager.freeClientIDs[NetworkState->clientManager.freeClientIDCount] = clientID;
        NetworkState->clientManager.freeClientIDCount++;
        memset(&NetworkState->clientManager.udp_addrs[clientID], 0,sizeof(NetworkState->clientManager.udp_addrs[clientID]));
        NetworkState->clientManager.IDToToken[lastIndex] = UINT16_MAX;
        // NetworkState->reliableUDP[lastIndex] = {};
        NetworkState->clientManager.numClients--;
        NetworkState->netTest.num_clients--;
        

        return 0;
    }

    uint16_t generate_unique_token(network_state* NetworkState) {
        uint16_t token;
        bool is_unique;
        // std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);
        //check that token is unique by comparing against all other client tokens
        do {
            
            printf("NEED TO RANDOMLY GENERATE TOKEN!!!\n");
            token = 0;
            is_unique = true;
            
            for(int i = 0; i < NetworkState->clientManager.numClients; i++) {
                if(NetworkState->clientManager.IDToToken[i] == token) {
                    is_unique = false;
                    break;
                }
            }
        } while(!is_unique);

        return token;
    }

    bool compareAddr(sockaddr_in a, sockaddr_in b) {
        return (a.sin_addr.s_addr == b.sin_addr.s_addr && 
                a.sin_port == b.sin_port);
    }


    int assignClient(network_state* NetworkState, sockaddr_in client_addr, uint8_t& clientID){
        
        bool found = false;
        for(int i = 0; i < NetworkState->clientManager.numClients; i++){
            if(compareAddr(NetworkState->clientManager.udp_addrs[i], client_addr)) {
                found = true;
                printf("ALREADY ADDED CLIENT AT SLOT %d, RETURNING\n", i);
                clientID = i;
                return 1;
            }
        }

        if(NetworkState->clientManager.numClients >= MAX_CLIENTS){
            printf("NETWORK ERROR: max clients %d reached!, rejecting new client\n", NetworkState->clientManager.numClients);
            return -1;
        }

        if(NetworkState->clientManager.freeClientIDCount > 0){
            clientID = NetworkState->clientManager.freeClientIDs[0];
            NetworkState->clientManager.freeClientIDCount--;
            NetworkState->clientManager.freeClientIDs[NetworkState->clientManager.freeClientIDCount] = 0;
        }else{
            clientID = NetworkState->clientManager.numClients;
        }
        


        if(clientID >= MAX_SOCKET_ID){
            printf("NETWORK ERROR: clientID: %d LARGER THAN MAX_SOCKET_ID: %d\n", clientID, MAX_SOCKET_ID);
            return -2;//totally arbitrary error return value
        }
        uint16_t unique_token = generate_unique_token(NetworkState);
        int slot = clientID;
        NetworkState->clientManager.udp_addrs[slot] = client_addr;
        NetworkState->clientManager.tokenToID[unique_token] = slot;
        NetworkState->clientManager.IDToToken[slot] = unique_token;
        NetworkState->connectionState[slot].token = unique_token;
        memset(NetworkState->clientManager.readBuffers[slot].buffer, 0, MAX_TCP_SIZE * sizeof(char));
        NetworkState->clientManager.readBuffers[slot].bytes_stored = 0;

        printf("added clientID: %d to slot: %d, token: %d\n", clientID, slot, unique_token);
        NetworkState->clientManager.numClients++;
        return 0;

    }

   void setup_sockets(network_state* NetworkState) {
        initClientManager(NetworkState);
        init_packet_manager(NetworkState);
        sockaddr_in udp_addr;
        
        #ifdef _WIN32
            // Initialize Winsock
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                printf("WSAStartup failed\n");
                return;
            }
        #endif

             

        // Create UDP socket
        NetworkState->serverSockets.udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (NetworkState->serverSockets.udp_socket == INVALID_SOCKET) {
            perror("UDP socket creation failed");
            return;
        }

        // int buffer_size = 262144; // 256KB buffer
        int buffer_size = 10485760; // 10MB buffer

        #ifdef _WIN32
            const char buffer_size_val = (const char)buffer_size;
        #else
            int buffer_size_val = buffer_size;

        #endif

        if (setsockopt(NetworkState->serverSockets.udp_socket, SOL_SOCKET, SO_SNDBUF, &buffer_size_val, sizeof(buffer_size)) < 0) {
            perror("Failed to set send buffer size");
            return;
        }
        if (setsockopt(NetworkState->serverSockets.udp_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size_val, sizeof(buffer_size)) < 0) {
            perror("Failed to set receive buffer size");
            return;
        }


        // Set UDP socket options
        BOOL opt = TRUE;
        if (setsockopt(NetworkState->serverSockets.udp_socket, SOL_SOCKET, SO_REUSEADDR, 
                    (char*)&opt, sizeof(opt)) < 0) {
            perror("UDP setsockopt failed");
            return;
        }

        // Configure UDP address
        memset(&udp_addr, 0, sizeof(udp_addr));
        udp_addr.sin_family = AF_INET;
        udp_addr.sin_addr.s_addr = INADDR_ANY;
        udp_addr.sin_port = htons(UDP_PORT);

        // Bind UDP socket
        if (bind(NetworkState->serverSockets.udp_socket, ( sockaddr*)&udp_addr, 
                sizeof(udp_addr)) < 0) {
            perror("UDP bind failed");
            return;
        }

        // Set non-blocking mode for UDP
        #ifdef _WIN32
            u_long mode = 1;  // 1 = non-blocking
            ioctlsocket(NetworkState->serverSockets.udp_socket, FIONBIO, &mode);
        #else
            fcntl(NetworkState->serverSockets.udp_socket, F_SETFL, O_NONBLOCK);
        #endif
        

        NetworkState->serverSockets.active = true;
        NetworkState->enabled = true;

        for(int i = 0; i < MAX_CLIENTS; i++){
            //ConnectionState Init
            //CONNECTION STATE INIT
            NetworkState->connectionState[i].is_client = false;        
            NetworkState->connectionState[i].is_server = true;        
            init_connection_state(NetworkState, NetworkState->connectionState[i]);

        }


        return;
    }

    void cleanup_server_sockets(network_state* NetworkState) {
        #ifdef _WIN32
            closesocket(NetworkState->serverSockets.udp_socket);
            WSACleanup();
        #else
            close(NetworkState->serverSockets.udp_socket);
        #endif
        
        NetworkState->serverSockets.udp_socket = INVALID_SOCKET;
        NetworkState->serverSockets.active = false;
        NetworkState->enabled = false;

        for(int i = 0; i < MAX_CLIENTS; i++){
            //ConnectionState Init
            //CONNECTION STATE INIT
            NetworkState->connectionState[i].is_client = false;        
            NetworkState->connectionState[i].is_server = false;        
            init_connection_state(NetworkState, NetworkState->connectionState[i]);

        }

    }

    int poll_network_messages(network_state* NetworkState) {
        if(NetworkState->netTest.enabled){
            while(receive_packet(NetworkState) == 0){}
            return 0;
        }

        if(!NetworkState){
            printf("poll_network_messages() NETWORK STATE NULL, RETURNING!\n");
            return -1;  
        }
        fd_set read_fds;
      
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        // Set up the fd_set for select()
        FD_ZERO(&read_fds);
        FD_SET(NetworkState->serverSockets.udp_socket, &read_fds);


        
        timeval tv = {0, 100};
        // Check for messages
        #ifdef _WIN32
            int activity = select(0, &read_fds, NULL, NULL, &tv);
        #else
            int activity = select(NetworkState->serverSockets.udp_socket + 1, &read_fds, NULL, NULL, &tv);
        #endif
        if(!NetworkState){
            printf("poll_network_messages() after select() timeout, NETWORKS STATE IS NULL???\n");
            return -1;
        }
        if(ShouldThreadsExit){
            printf("poll_network_messages() after select() timeout, THREAD SHOULD EXIT\n");
            return -1;
        }
        
        if (activity > 0) {
            
            // Check UDP socket
            if (FD_ISSET(NetworkState->serverSockets.udp_socket, &read_fds)) {

 
                while(NetworkState && !ShouldThreadsExit && receive_packet(NetworkState) == 0){
            
                }
                if(!NetworkState || ShouldThreadsExit){
                    return -1;
                }
            }
            
           
        }
        return 0;

    }

    void handle_player_input(network_state* NetworkState, char* buffer, int clientID){
        player_input playerInput = {};

        unpack((unsigned char*)buffer, "CCcccChhHHlllllLQHC",   &playerInput.buttons, 
                                                                &playerInput.mouseButtons, 
                                                                &playerInput.mouse_wheel, 
                                                                &playerInput.look_x, 
                                                                &playerInput.look_y,
                                                                &playerInput.action, 
                                                                &playerInput.mouse_dx, 
                                                                &playerInput.mouse_dy, 
                                                                &playerInput.mouse_x, 
                                                                &playerInput.mouse_y,
                                                                &playerInput.fptAngleH, 
                                                                &playerInput.fptAngleV,
                                                                &playerInput.rayDir.x,
                                                                &playerInput.rayDir.y,
                                                                &playerInput.rayDir.z,
                                                                &playerInput.tick,
                                                                &playerInput.time,
                                                                &playerInput.sequence,
                                                                &playerInput.debugFlags);

        // printf("server received player input, time delay: %d\n", GetTimeMS() - playerInput.time);
        playerInput.flags.received = 1; //how necessary is this? we are just using it for debugging
        NetworkState->playerInputs[clientID][0][(NetworkState->currentTick) & (SNAPSHOT_BUFFER_SIZE - 1)] = playerInput;
    }

    void handle_ping_request(network_state* NetworkState, char* buffer, int clientID){
        printf("PING MESSAGE RECIEVED\n");
        // uint64_t receive_time = GetTimeMS();
        uint64_t receive_time = GetTimeMS()/*REPLACE*/;
        
        uint16_t sequence = 0;
        uint64_t ping_time = 0;
        uint16_t magicNumber = 0;
        
        unpack((unsigned char*)buffer, "HQH", &sequence, &ping_time, &magicNumber);
        
        
        NetworkState->clientManager.last_ping_times[clientID] =  ping_time;
        NetworkState->clientManager.last_sequences[clientID] =  sequence;

        NetworkState->clientManager.last_client_times[clientID] = ping_time;
        NetworkState->clientManager.server_receive_times[clientID] = receive_time;
        NetworkState->clientManager.client_clock_offsets[clientID] = receive_time - ping_time;
        

        if(magicNumber == 0x5005){
            //soos
            printf("correct magic number 5005!\n");
        }
        else{
            printf("incorrect magic number recieved??? DISCONNECTING CLIENT\n");
            printf("incorrect magic number recieved??? DISCONNECTING CLIENT\n");
            printf("incorrect magic number recieved??? DISCONNECTING CLIENT\n");
            printf("incorrect magic number recieved??? DISCONNECTING CLIENT\n");
            printf("incorrect magic number recieved??? DISCONNECTING CLIENT\n");
            removeClient(NetworkState, clientID);
        }

        char response[256];
        // uint64_t send_time = GetTimeMS();
        uint64_t send_time = GetTimeMS()/*REPLACE*/;

        int size = pack((unsigned char*)response, "HQHQQ", sequence, ping_time, magicNumber, receive_time, send_time);
        printf("clientID: %d, sequence: %d, ping_time: %" UINT64_FORMAT "\n",clientID, sequence, ping_time);

        int total_bytes_sent = 0;

        int result = send_binary_udp(NetworkState->serverSockets.udp_socket,
        MessageTypes::MSG_PONG, 
        (unsigned char*)response,
        size,
        (sockaddr*)&NetworkState->clientManager.udp_addrs[clientID],
        sizeof(sockaddr_in),
        &total_bytes_sent);
    }

    void handle_connection_handshake(network_state* NetworkState, char* buffer, uint8_t clientID){
        uint16_t client_udp_port;
        packet_entry entry = {};
        int offset = sizeof(entry.type) + sizeof(entry.size);
        unpack((unsigned char*) buffer + offset, "H", &client_udp_port);
        printf("Recieved handshake, client: %d udp_port: %d\n", clientID, client_udp_port);
        NetworkState->clientManager.udp_addrs[clientID].sin_port = htons(client_udp_port); //set client udp port
        int total_bytes_sent = 0;
        char send_buffer[16];
        //send client the ID the server stored for it
        
        entry.type = MessageTypes::MSG_CONNECTED_HANDSHAKE;
        entry.size = 5;
        int packetsize = pack((unsigned char*) send_buffer, "CHH", entry.type, entry.size, NetworkState->clientManager.IDToToken[clientID]);
        
        PacketManager& pm = NetworkState->packetManager;
        ConnectionState& cs = NetworkState->connectionState[clientID];

        if(!NetworkState->netTest.enabled){
            network_thread_append_to_packet(NetworkState, send_buffer, packetsize, clientID);

        } else{
            sim_append_network_command(NetworkState, cs, send_buffer, packetsize);
            // sim_append_to_packet(NetworkState, cs, send_buffer, packetsize);

        }
    
    }

    void broadcast_tcp_message(network_state* NetworkState, uint16_t type, const char* payload, uint16_t payload_size) {
        // for (int i = 0; i < NetworkState->clientManager.numClients; i++) {
        //     int total_bytes_sent = 0;
        //     int result = send_message_tcp(NetworkState->clientManager.tcpClients[i], type, payload, payload_size, &total_bytes_sent);
        //     if(result != 0){
        //         printf("error sending tcp broadcast\n");
        //     }
        // }
    }

    void broadcast_udp_message(network_state* NetworkState, const char* message) {
        for (int i = 0; i < NetworkState->clientManager.numClients; i++) {
            int total_bytes_sent = 0;
            int result = send_message_udp(NetworkState->serverSockets.udp_socket,
             MessageTypes::MSG_UDP_BROADCAST, 
             message,
             strlen(message) + 1,
            (sockaddr*)&NetworkState->clientManager.udp_addrs[i],
            sizeof(sockaddr_in),
            &total_bytes_sent);

            if(result != 0){
                printf("error sending udp broadcast\n");
            }
        }
    }






    void ConsolePushToMain(ConsoleCommand cmd) {
        if(ConsoleMainCount == 256) return;
        int idx = AtomicIncrement(&ConsoleMainWriteIndex) & 255;
        ConsoleToMain[idx] = cmd;
        AtomicIncrement(&ConsoleMainCount);
    }

    bool ConsolePopToMain(ConsoleCommand* out) {
        if(ConsoleMainCount == 0) return false;
        *out = ConsoleToMain[AtomicIncrement(&ConsoleMainReadIndex) & 255];
        AtomicDecrement(&ConsoleMainCount);
        return true;
    }

    void PushToConsole(ConsoleCommand cmd) {
        if(consoleCount == 256) return;
        int idx = AtomicIncrement(&consoleWriteIndex) & 255;
        toConsole[idx] = cmd;
        AtomicIncrement(&consoleCount);
    }

    bool PopToConsole(ConsoleCommand* out) {
        if(consoleCount == 0) return false;
        *out = toConsole[AtomicIncrement(&consoleReadIndex) & 255];
        AtomicDecrement(&consoleCount);
        return true;
    }




    #ifdef _WIN32
    DWORD WINAPI ConsoleThreadProc(LPVOID lpParameter)
    #else
    void* ConsoleThreadProc(void* lpParameter)
    #endif
    {
        char commandStr[256];
        ConsoleCommand cmd = {};
        while(!ShouldThreadsExit) {

            if(PopToConsole(&cmd)) {
                switch(cmd.type) {
                    case CONSOLE_CMD_BROADCAST:

                        printf("Console Broadcast: %s \n", cmd.dataString);

                        break;
                }
            } 

            printf("Server> ");
            HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

            char commandStr[256];
            DWORD charsRead;

            ReadConsoleA(hInput, commandStr, sizeof(commandStr), &charsRead, NULL);

            // Strip newline manually if needed
            if (charsRead > 0 && commandStr[charsRead - 2] == '\r' && commandStr[charsRead - 1] == '\n') {
                commandStr[charsRead - 2] = 0;
            } else {
                commandStr[charsRead] = 0;
            }

            char originalMsg[256];
            strcpy(originalMsg, commandStr);
            char* remainder;
            char* command = get_token(commandStr, ' ', &remainder);
            if(strcmp(command, "exit") == 0) {
                printf("\n");
                cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_EXIT;
                strcpy(cmd.dataString, commandStr);
                ConsolePushToMain(cmd);
                printf("CONSOLE THREAD EXITING\n");
                break;
            }
            else if(strcmp(command, "ping") == 0) {
                cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_PING;
                strcpy(cmd.dataString, commandStr);
                ConsolePushToMain(cmd);
            }
            else if(strcmp(command, "/udp_broadcast") == 0){
                char* leftover;
                // char* message = get_token(remainder, ' ', &leftover);
                // printf("udp broadcast, message: %s\n", message);
                cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_UDP_BROADCAST;
                strcpy(cmd.dataString, remainder);
                ConsolePushToMain(cmd);
            }
            else if(strcmp(command, "/udp_whisper") == 0){
                char* message;
                char* clientID = get_token(remainder, ' ', &message);
                printf("udp whisper, clientID %s, message: %s\n",clientID, message);
                if(!message){
                    printf("WHISPER MESSAGE SYNTAX ERROR, NEED CLIENTID: /udp_whisper clientID message\n");
                }
                else{
                    cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_UDP_WHISPER;
                    cmd.data.udp_whisper.clientID = atoi(clientID);
                    strcpy(cmd.dataString, message);
                    ConsolePushToMain(cmd);
                }

            }
            else if(strcmp(command, "logdebug") == 0) {
                printf("\n");
                cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_DEBUG;
                ConsolePushToMain(cmd);
            }
            else if(strcmp(command, "loginfo") == 0) {
                printf("\n");
                cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_INFO;
                ConsolePushToMain(cmd);
            }


            else{
                cmd.type = ConsoleThreadCommandTypes::CONSOLE_CMD_BROADCAST;
                strcpy(cmd.dataString, originalMsg);
                ConsolePushToMain(cmd);
            }
        }
        return 0;
    }







