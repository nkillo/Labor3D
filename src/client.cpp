// #include "client.h"
// #include "packet.h"


MainClientSocketState mainClientSocketState;



    ReadBuffer readBuffer;
    

    bool init_client_sockets(network_state* NetworkState){
        memset(readBuffer.buffer, 0, MAX_TCP_SIZE * sizeof(char));
        readBuffer.bytes_stored = 0;

        #ifdef _WIN32
            // Initialize Winsock
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                printf("WSAStartup failed\n");
                return false;
            }
        #endif
        init_packet_manager(NetworkState);
        NetworkState->clientSockets.tcp_socket = INVALID_SOCKET;
        NetworkState->clientSockets.udp_socket = INVALID_SOCKET;
        NetworkState->clientSockets.isConnected = false;
        NetworkState->clientSockets.active = true;
        NetworkState->enabled = true;

        //ConnectionState Init
        NetworkState->connectionState[0].is_client = true;
        NetworkState->connectionState[0].is_server = false;
        init_connection_state(NetworkState, NetworkState->connectionState[0]);

        return true;
    }

    void client_process_client_command(char* cmdString){
        NetworkCommand netCmd = {};
        char originalMessage[256];
        strcpy(originalMessage, cmdString);

        char* remainder;
        char* command = get_token(cmdString, ' ', &remainder);
        printf("command: %s, remainder: %s\n", command, remainder);

        if (command == NULL) return;
        
        if (strcmp(command, "/connect") == 0) {
            // Get the address:port portion
            char* port;
            char* addressPort = get_token(remainder, ':', &port);
            int portNum;
            printf("addressPort: %s\n", addressPort);
            if (addressPort == NULL) {
                printf("Error: Missing address:port\n");
                return;
            }
            if(!port){
                portNum = 12345;
                printf("Port not specified! Using default: %d\n", portNum);
            }
            else{
                *(port-1) = '\0';
                printf("address: %s, port: %s\n", addressPort, port);        
                portNum = atoi(port);

            }
            // Null terminate the address string at the ':'
            // Move port pointer past the ':'
            char* address = addressPort;
            
            // Now we can call connect function
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECT;
            strcpy(netCmd.dataString, address);
            netCmd.data.connect_to_server.tcp_portNum = portNum;
            netCmd.data.connect_to_server.udp_portNum = portNum + 1;
            PushToNetwork(netCmd);
        }
        else if(strcmp(command, "/c") == 0){ //short hand for connecting to localhost
            char address[256] = "127.0.0.1";
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECT;
            strcpy(netCmd.dataString, address);
            netCmd.data.connect_to_server.tcp_portNum = 12345;
            netCmd.data.connect_to_server.udp_portNum = 12346;
            PushToNetwork(netCmd);
        }
        else if(strcmp(command, "/dc") == 0){ //shorthand for disconnect from server
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_DISCONNECT;
            PushToNetwork(netCmd);
        }
        else if (strcmp(command, "/udp_chat") == 0) {
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CHAT_UDP;
            strcpy(netCmd.dataString, remainder);
            PushToNetwork(netCmd);
        }
        else if (strcmp(command, "/startHost") == 0){
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_START_HOST;
            PushToNetwork(netCmd);
        }
        else if (strcmp(command, "/startClient") == 0){
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_START_CLIENT;
            PushToNetwork(netCmd);
        }
        else{
            netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CHAT;
            strcpy(netCmd.dataString, originalMessage);
            PushToNetwork(netCmd);
        }
    }

    
    void tell_network_thread_to_disconnect(){//disconnects client from server
        NetworkCommand netCmd = {};
        netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_DISCONNECT;
        PushToNetwork(netCmd);

    }


    void push_player_input_to_network(player_input* playerInput){
        NetworkCommand netCmd = {};
        netCmd.type = NetworkThreadCommandTypes::CLIENT_INPUT;

        netCmd.data.playerInput = *playerInput;

        PushToNetwork(netCmd);

    }

    void send_player_input(network_state* NetworkState, player_input* playerInput){

        // labour::performance_timer::Timer inputSerializeTimer;
        // labour::performance_timer::Timer sendTimer;
        // inputSerializeTimer.reset();
        // sendTimer.reset();

        // uint64_t now = GetTimeMS();
        // NetworkState->pingTracker.sent_times[NetworkState->pingTracker.next_sequence] = now;

        uint16_t sequence = NetworkState->pingTracker.next_sequence;
        uint16_t magicNumber = 0x5005;//soos
        
        int total_bytes_sent = 0;
        char buffer[256];


        int packetsize = pack((unsigned char*)buffer, "CCcccChhHHlllllLQHC",   playerInput->buttons, 
                                                            playerInput->mouseButtons, 
                                                            playerInput->mouse_wheel, 
                                                            playerInput->look_x, 
                                                            playerInput->look_y,
                                                            playerInput->action, 
                                                            playerInput->mouse_dx, 
                                                            playerInput->mouse_dy, 
                                                            playerInput->mouse_x, 
                                                            playerInput->mouse_y,
                                                            playerInput->fptAngleH, 
                                                            playerInput->fptAngleV,
                                                            playerInput->rayDir.x,
                                                            playerInput->rayDir.y,
                                                            playerInput->rayDir.z,
                                                            playerInput->tick,
                                                            playerInput->time,
                                                            playerInput->sequence,
                                                            playerInput->debugFlags);


        // printf("sending sequence: %d, time: %" UINT64_FORMAT "\n", sequence, now);
            // printf("serialize took: %f seconds\n",inputSerializeTimer.elapsed());
            // inputSerializeTimer.stop();
        // if(send_binary_udp(NetworkState->clientSockets.udp_socket, 
        // MessageTypes::PLAYER_INPUT, 
        // buffer, 
        // packetsize, 
        // (sockaddr*)&NetworkState->clientSockets.server_addr_udp, 
        // sizeof(NetworkState->clientSockets.server_addr_udp), 
        // &total_bytes_sent, NetworkState->clientSockets.token) == 0){
        network_thread_append_to_packet(NetworkState, buffer, packetsize, 0);

            // printf("sent ping to server, size header: %zu, size msg: %zu, total bytes: %zu, total_sent: %d\n",sizeof(MessageHeader) , strlen(cmd.dataString) + 1, sizeof(MessageHeader) + strlen(cmd.dataString) + 1, total_bytes_sent);
        // }
        //pack and send: sequence, NOW, magic number
        // NetworkState->pingTracker.next_sequence++;
            //         printf("send took: %f seconds\n",sendTimer.elapsed());
            // sendTimer.stop();


    }


    
    void setup_server_connection(network_state* NetworkState, const char* ip, int udp_port){
        //setup socket UDP creation/binding
        if(!NetworkState->clientSockets.connecting){

                // NetworkState->clientSockets.tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
                NetworkState->clientSockets.udp_socket = socket(AF_INET, SOCK_DGRAM,  0);


                // int buffer_size = 262144; // 256KB buffer
                int buffer_size = 10485760; // 10MB buffer

                #ifdef _WIN32
                    const char buffer_size_val = (const char)buffer_size;
                #else
                    int buffer_size_val = buffer_size;

                #endif

                if (setsockopt(NetworkState->clientSockets.udp_socket, SOL_SOCKET, SO_SNDBUF, &buffer_size_val, sizeof(buffer_size)) < 0) {
                    perror("Failed to set send buffer size");
                    return;
                }
                if (setsockopt(NetworkState->clientSockets.udp_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size_val, sizeof(buffer_size)) < 0) {
                    perror("Failed to set receive buffer size");
                    return;
                }



                // if(NetworkState->clientSockets.tcp_socket == INVALID_SOCKET) {
                //     perror("TCP socket creation failed");
                //     return false;
                // }

                if(NetworkState->clientSockets.udp_socket == INVALID_SOCKET) {
                    // CLOSE_SOCKET(NetworkState->clientSockets.tcp_socket);  // Cleanup TCP if UDP fails
                    perror("UDP socket creation failed");
                    return;
                }

                NetworkState->clientSockets.client_addr = {0};
                NetworkState->clientSockets.client_addr.sin_family = AF_INET;
                NetworkState->clientSockets.client_addr.sin_addr.s_addr = INADDR_ANY;
                NetworkState->clientSockets.client_addr.sin_port = 0; //let os choose port

                if(bind(NetworkState->clientSockets.udp_socket, (sockaddr*)&NetworkState->clientSockets.client_addr, sizeof(NetworkState->clientSockets.client_addr)) < 0){
                    perror("UDP bind failed");
                    printf("UDP BIND FAILED\n");
                    return;
                }

                //get the port OS assigned us
                sockaddr_in bound_addr;
                NetworkState->clientSockets.udp_port_len = sizeof(bound_addr);
                getsockname(NetworkState->clientSockets.udp_socket, (sockaddr*)&bound_addr, &NetworkState->clientSockets.udp_port_len);
                NetworkState->clientSockets.udp_port = ntohs(bound_addr.sin_port);

                printf("Client UDP bound to port: %d\n", NetworkState->clientSockets.udp_port);

                #ifdef _WIN32
                    u_long mode = 1;
                    // ioctlsocket(NetworkState->clientSockets.tcp_socket, FIONBIO, &mode);
                    ioctlsocket(NetworkState->clientSockets.udp_socket, FIONBIO, &mode);
                #else
                    // fcntl(NetworkState->clientSockets.tcp_socket, F_SETFL, O_NONBLOCK);
                    fcntl(NetworkState->clientSockets.udp_socket, F_SETFL, O_NONBLOCK);
                #endif
                
                //setup udp address
                if(udp_port == -1)udp_port = 12346;
                memset(&NetworkState->clientSockets.server_addr_udp, 0, sizeof(NetworkState->clientSockets.server_addr_udp));
                NetworkState->clientSockets.server_addr_udp.sin_family = AF_INET;
                NetworkState->clientSockets.server_addr_udp.sin_port = htons(udp_port);
                inet_pton(AF_INET, ip, &NetworkState->clientSockets.server_addr_udp.sin_addr);

                NetworkState->clientSockets.isConnected = false;
                NetworkState->clientSockets.connecting = true;

                // NetworkState->clientSockets.connectStart = GetTimeMS();
                NetworkState->clientSockets.connectStart = GetTimeMS()/*REPLACE*/;


                //handle it if we cant connect like this
                // NetworkState->clientSockets.udp_socket = INVALID_SOCKET;
                // NetworkState->clientSockets.isConnected = false;
                // cmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECT_FAILED;
                // NetworkPushToMain(cmd);

                // or if we successfully connected
                // printf("Connection established immediately.\n");
                // NetworkState->clientSockets.isConnected = true;
                // cmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECTED;
                // NetworkPushToMain(cmd);
                
        }
    }

    void connect_to_server(network_state* NetworkState){

     

        // uint32_t currentTime = GetTimeMS();
        uint32_t currentTime = GetTimeMS()/*REPLACE*/;
        if (currentTime - NetworkState->clientSockets.connectStart > 5000) { // 5 seconds in ms
            // Connection timed out
            printf("CONNECTION ATTEMPT TIMED OUT\n");
            printf("CONNECTION ATTEMPT TIMED OUT\n");
            printf("CONNECTION ATTEMPT TIMED OUT\n");
            printf("CONNECTION ATTEMPT TIMED OUT\n");
            NetworkState->clientSockets.isConnected = false;
            NetworkState->clientSockets.connecting = false;
            return;
        }

        //sleep for 16 ms in the main loop and 500ms when connecting to reduce bandwidth consumption
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(ShouldThreadsExit){
            printf("connect_to_server() THREAD SHOULD EXIT, RETURNING\n");
            return;
        }
        if(!NetworkState){
            printf("connect_to_server() NETWORK STATE IS NULL, RETURNING\n");
            return;
        }

        NetworkCommand cmd = {};

        cmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECTING;
        NetworkPushToMain(cmd);

        printf("thread connect to server...\n");

        // Attempt to connect
        send_connected_handshake_request(NetworkState);

    }


    void disconnect_client(network_state* NetworkState, bool disconnectedFromServer){
        if(NetworkState->clientSockets.tcp_socket != INVALID_SOCKET){
            CLOSE_SOCKET(NetworkState->clientSockets.tcp_socket);
            NetworkState->clientSockets.tcp_socket = INVALID_SOCKET;
        }
        if(NetworkState->clientSockets.udp_socket != INVALID_SOCKET){
            CLOSE_SOCKET(NetworkState->clientSockets.udp_socket);
            NetworkState->clientSockets.udp_socket = INVALID_SOCKET;
        }
        NetworkState->clientSockets.isConnected = false;
        NetworkState->clientSockets.connecting = false;
        NetworkState->clientSockets.active = false;
        NetworkState->enabled = false;

        if(disconnectedFromServer){
            init_client_sockets(NetworkState);
        }
    }



    // Poll for any incoming messages (similar to server's poll)
    int poll_client_messages(network_state* NetworkState) {

            if(NetworkState->netTest.enabled){
                while(receive_packet(NetworkState) == 0){}
                return 0;
            }
            
            if(!NetworkState){
                printf("poll_client_messages() NetworkState IS NULL! RETURNING!\n");
                return -1;
            }

           fd_set read_fds;
    
            FD_ZERO(&read_fds);
            FD_SET(NetworkState->clientSockets.udp_socket, &read_fds);
            
            timeval tv = {0, 100};
            
            #ifdef _WIN32
                int activity = select(0, &read_fds, NULL, NULL, &tv);
            #else
                int activity = select(NetworkState->clientSockets.udp_socket + 1, &read_fds, NULL, NULL, &tv);
            #endif
            if(!NetworkState){
                printf("poll_client_messages() after select() timeout, NETWORKS STATE IS NULL???\n");
                return -1;
            }
            if(ShouldThreadsExit){
                printf("poll_client_messages() after select() timeout, THREAD SHOULD EXIT\n");
                return -1;
            }

        if (activity > 0) {




            // // Check TCP messages
            // if (FD_ISSET(NetworkState->clientSockets.tcp_socket, &read_fds)) {
            //     int bytes_available;
            //     #ifdef _WIN32
            //     u_long bytes;
            //     if (ioctlsocket(NetworkState->clientSockets.tcp_socket, FIONREAD, &bytes) == 0) {
            //         bytes_available = (int)bytes;
            //     }
            //     #else
            //         if (ioctl(NetworkState->clientSockets.tcp_socket, FIONREAD, &bytes_available) >= 0) {}
            //     #endif

            //     if (bytes_available == 0) {  // Socket closed
            //         printf("SERVER CLOSED??\n");
            //         bool disconnectedFromServer = true;
            //         disconnect_client(NetworkState, disconnectedFromServer);
            //         return;
            //     }
            //     MessageHeader header = {};
            //     char buffer[MAX_TCP_SIZE];
            //     int result = receive_message_tcp(NetworkState->clientSockets.tcp_socket, &header, buffer, MAX_TCP_SIZE, &readBuffer);
            //     if (result > 0) {
            //         printf("Received TCP Message size: %d, type: %d\n", header.size, header.type);
            //         if(header.type == MessageTypes::MSG_CHAT){
            //             printf("Recieved Message: %s\n", buffer);
            //         }
 
            //         else if(header.type == MessageTypes::MSG_BROADCAST){
            //             printf("Recieved Broadcast: %s\n", buffer);
            //         }

            //     }
            //     else if (result < 0) {
            //         printf("error reading TCP message!\n");
            //         printf("ERROR: DISCONNECTING FROM SERVER\n");
            //         printf("ERROR: DISCONNECTING FROM SERVER\n");
            //         printf("ERROR: DISCONNECTING FROM SERVER\n");
            //         printf("ERROR: DISCONNECTING FROM SERVER\n");
            //         printf("ERROR: DISCONNECTING FROM SERVER\n");
            //         bool disconnectedFromServer = true;
            //         disconnect_client(NetworkState, disconnectedFromServer);
            //     }
            // }
            
            // Check UDP messages
            if (FD_ISSET(NetworkState->clientSockets.udp_socket, &read_fds)) {

                while(NetworkState && !ShouldThreadsExit && receive_packet(NetworkState) == 0){

                //     char udp_buffer[MAX_UDP_SIZE];
                //     sockaddr_in server_addr;
                //     socklen_t addr_len = sizeof(server_addr);
                //     MessageHeader header;
                //     int bytes;
                //     bytes = receive_message_udp(NetworkState->clientSockets.udp_socket, &header, udp_buffer, sizeof(udp_buffer), (sockaddr*)&server_addr, &addr_len);

                    
                //     // If no more packets, break the loop
                //     if (bytes < 0) {
                //         #ifdef _WIN32
                //             if (WSAGetLastError() == WSAEWOULDBLOCK)
                //         #else
                //             if (errno == EAGAIN || errno == EWOULDBLOCK)
                //         #endif
                //                 break;  // No more packets available right now
                        
                //         // Handle actual errors
                //         printf("Error receiving UDP packet: %d\n", 
                //             #ifdef _WIN32
                //                 WSAGetLastError()
                //             #else
                //                 errno
                //             #endif
                //         );
                //         break;
                //     }
                        

                //     if (bytes > 0) {
                //         printf("UDP bytes received: header: %d, payload: %d, total: %d\n",PACKED_MESSAGE_HEADER_SIZE, bytes, header.size + PACKED_MESSAGE_HEADER_SIZE);
                //         if(header.type == MessageTypes::MSG_PONG){
                //             handle_ping_response(NetworkState, udp_buffer);

                //         }

                //         else if(header.type == MessageTypes::MSG_CHAT_UDP){
                //             printf("Chat UDP Recieved: %s\n", udp_buffer);
                //         }
                //         else if(header.type == MessageTypes::MSG_UDP_BROADCAST){
                //             printf("Chat UDP Recieved: %s\n", udp_buffer);
                //         }
                //         else if(header.type == MessageTypes::MSG_UDP_WHISPER){
                //             printf("Chat UDP Recieved: %s\n", udp_buffer);
                //         }
                //         else if(header.type == MessageTypes::ENTITY_STATE){
                //             receive_entity_state(NetworkState, udp_buffer);
                //         }
                //     }
                }
                if(!NetworkState || ShouldThreadsExit){
                    return -1;
                }
            }
        }
        return 0;
    }



    void send_disconnect(network_state* NetworkState){
        int total_bytes_sent = 0;
        unsigned char buffer[256];

        uint16_t magicNumber = 0x5005;//soos

        int packetsize = pack(buffer, "H", magicNumber);


        if(send_binary_udp(NetworkState->clientSockets.udp_socket, 
        MessageTypes::MSG_DISCONNECT, 
        buffer, 
        packetsize, 
        (sockaddr*)&NetworkState->clientSockets.server_addr_udp, 
        sizeof(NetworkState->clientSockets.server_addr_udp), 
        &total_bytes_sent, NetworkState->clientSockets.token) == 0){
            printf("sending disconnect request\n");
        }

    }

    void send_ping(network_state* NetworkState){
        // uint64_t now = GetTimeMS();
        uint64_t now = GetTimeMS()/*REPLACE*/;
        NetworkState->pingTracker.sent_times[NetworkState->pingTracker.next_sequence] = now;

        uint16_t sequence = NetworkState->pingTracker.next_sequence;
        uint16_t magicNumber = 0x5005;//soos
        
        int total_bytes_sent = 0;
        unsigned char buffer[256];


        int packetsize = pack(buffer, "HQH", sequence, now, magicNumber);
        // int offset = 0;

        // packi16(buffer + offset, sequence);
        // offset += 2;
        
        // packi64(buffer + offset, now);
        // offset += 8;

        // packi16(buffer + offset, magicNumber);
        // offset += 2;

        printf("sending sequence: %d, time: %" UINT64_FORMAT "\n", sequence, now);

        if(send_binary_udp(NetworkState->clientSockets.udp_socket, 
        MessageTypes::MSG_PING, 
        buffer, 
        packetsize, 
        (sockaddr*)&NetworkState->clientSockets.server_addr_udp, 
        sizeof(NetworkState->clientSockets.server_addr_udp), 
        &total_bytes_sent, NetworkState->clientSockets.token) == 0){
            // printf("sent ping to server, size header: %zu, size msg: %zu, total bytes: %zu, total_sent: %d\n",sizeof(MessageHeader) , strlen(cmd.dataString) + 1, sizeof(MessageHeader) + strlen(cmd.dataString) + 1, total_bytes_sent);
        }
        //pack and send: sequence, NOW, magic number
        NetworkState->pingTracker.next_sequence++;
    }

    

    void handle_ping_response(network_state* NetworkState, char* buffer){
        uint16_t sequence = 0;
        uint64_t ping_time = 0;
        uint64_t server_receive_time = 0;
        uint64_t server_send_time = 0;
        uint16_t magicNumber = 0;
        
        unpack((unsigned char*)buffer, "HQHQQ", &sequence, &ping_time, &magicNumber, &server_receive_time, &server_send_time);
        
        printf("sequence: %d, ping_time: %" UINT64_FORMAT " server receive time: %" UINT64_FORMAT ", server send time: %" UINT64_FORMAT "\n", sequence, ping_time, server_receive_time, server_send_time);
        if(magicNumber == 0x5005){
            //soos
            // printf("correct magic number 5005!\n");
        }
        else{
            printf("INCORRECT MAGIC NUMBER?? DISCONNECTING FROM SERVER\n");
            bool disconnectedFromServer = true;
            disconnect_client(NetworkState, disconnectedFromServer);
            return;
        }
        

        //handle ping response
        // uint64_t now = GetTimeMS();
        uint64_t now = GetTimeMS()/*REPLACE*/;


        uint64_t original_time = NetworkState->pingTracker.sent_times[sequence];
        if (NetworkState->pingTracker.sent_times[sequence]) {
            NetworkState->pingTracker.latest_rtt = now - original_time;  // RTT in ms
            NetworkState->pingTracker.sent_times[sequence] = 0;  // Clear tracked sequence
            
            // Readable time metrics
            float latency = NetworkState->pingTracker.latest_rtt / 2.0f;  // One-way latency
            NetworkState->clientSockets.clientTimeOffset = server_receive_time - (ping_time + latency);
            printf("RTT: %fms, Latency: %fms, offset: %" UINT64_FORMAT "\n", NetworkState->pingTracker.latest_rtt, latency, NetworkState->clientSockets.clientTimeOffset);
        }
    }
    void send_connected_handshake_request(network_state* NetworkState){
        if(!NetworkState){
            printf("send_connected_handshake_request() NETWORK STATE IS NULL, RETURNING\n");
            return;
        }
        int total_bytes_sent = 0;
        char buffer[16];
        packet_header header = {};
        packet_entry entry = {};
        entry.type = MessageTypes::MSG_CONNECT_REQUEST;
        entry.size = 5;
        int packetsize = pack((unsigned char*) buffer, "CHH", entry.type, entry.size, NetworkState->clientSockets.udp_port);
        
        PacketManager& pm = NetworkState->packetManager;
        int total_sent = 0;

        uint32_t clientID = 0;//0 for server
        header.size = entry.size;
        

        network_thread_append_to_packet(NetworkState, buffer, packetsize, clientID);

        // packet_manager_send_binary_udp(NetworkState, NetworkState->clientSockets.udp_socket, (unsigned char*)buffer, (sockaddr*)&NetworkState->clientSockets.server_addr_udp, sizeof(sockaddr_in), &total_sent, header, clientID);
    }

    void handle_connected_handshake_response(network_state* NetworkState, char* buffer, ConnectionState& cs){
        uint16_t token = 0;
        packet_entry entry = {};
        int offset = sizeof(entry.type) + sizeof(entry.size);
        unpack((unsigned char*) buffer + offset, "H", &token);
        NetworkState->clientSockets.token = token;
        printf("CLIENT RECIEVED HANDSHAKE FROM SERVER, token: %d\n", NetworkState->clientSockets.token);
        if(NetworkState->clientSockets.isConnected){
            LOG_ERROR(NetworkState, "%s handle_connected_handshake_response() ALREADY CONNECTED???", CLIENT_STR);
            assert(0 && "client is already connected???");
        }
        NetworkState->clientSockets.isConnected = true;
        NetworkState->clientSockets.connecting = false;
        
        //for the sim client, eventually the client sockets once I figure out how to bring it all together
        cs.connected = true;
        cs.connecting = false;
        cs.originalToken = token;
        cs.token = token;
        // cs.last_update = GetTimeMS();
        cs.last_update = GetTimeMS()/*REPLACE*/;

        cs.connection_duration = 0;
        NetworkCommand cmd = {};
        cmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECTED;
        NetworkPushToMain(cmd);

    }
    void send_time_sync(network_state* NetworkState){}
    void handle_time_sync_response(network_state* NetworkState, char* buffer){}

