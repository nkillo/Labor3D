#include "packet.h"
#include "binarySerialization.h"
#include "networkProtocol.h"
#include "server.h"
#include "client.h"
#include "netTest.h"


    int AdjustBandwidthBudget(network_state* NetworkState, uint32_t& bandwidth_accumulator, uint32_t bytes){
        // LOG_INFO(NetworkState, "adjust: bandwidth is: %u", cs.bandwidth_accumulator);

        uint32_t bits = bytes * 8; //bytes * 8 = bits
        
        if(bandwidth_accumulator < bits){ //budget exhausted
            return -1;
        }

        bandwidth_accumulator -= bits;
        
        return 0;
    }

    void BandwidthBudget(network_state* NetworkState, uint32_t& local_time_ms, uint32_t& bandwidth_accumulator){
        //adjust bandwidth budget
        // LOG_INFO(NetworkState, "budget: bandwidth is: %u", cs.bandwidth_accumulator);

        uint32_t timeNow = GetTimeMS()/*REPLACE*/;
            
        uint32_t difference = (timeNow - local_time_ms) & UINT32_MAX; //determine the offset/wraparound

        uint32_t amount = 0; 

        if(difference > 0){
            local_time_ms = timeNow;
            amount = ((TARGET_BANDWIDTH / 1000) * difference);
        }

        if(bandwidth_accumulator + amount < TARGET_BANDWIDTH){
            bandwidth_accumulator += amount; //calculating in milliseconds, hence 1000
        }
    }


    void prepare_to_receive_chunk(network_state* NetworkState, ConnectionState* cs){
        LOG_DEBUG(NetworkState, "prepare_to_receive_chunk() CLEARING OLD CHUNK DATA FOR NEW RECEIVE");
        memset(cs->receive_chunk_bitfield, 0, sizeof(uint64_t) * 4);
        memset(cs->receive_chunk, 0, MAX_SLICES * MAX_UDP_SIZE);
    }

    void prepare_to_send_chunk(network_state* NetworkState, ConnectionState* cs){
        LOG_DEBUG(NetworkState, "prepare_to_send_chunk() CLEARING OLD CHUNK DATA FOR NEW SEND");
        memset(cs->send_chunk_bitfield, 0, sizeof(uint64_t) * 4);
        memset(cs->times_slice_sent, 0, sizeof(int) * MAX_SLICES);
        cs->any_slices_sent = false;
    }


    int receive_chunk_slice(network_state* NetworkState, ConnectionState* cs, packet_header header, char* buffer){
        
        const char* connection = SERVER_STR;
        if(cs->is_client){
            connection = CLIENT_STR;
        }

        
        int slice = header.current_fragment;
        int array_index = slice / 64;
        int bit_index = slice & 63;       // Which bit within that uint64_t (same as i % 64)
        
        if ((cs->receive_chunk_bitfield[array_index] & (1ULL << bit_index)) == 0) {
            cs->receive_chunk_bitfield[array_index] |= (1ULL << bit_index);
            LOG_DEBUG(NetworkState, "%s RECEIVED CHUNK SLICE: %d", connection, slice);
            //copy data over into its slot
            memcpy(cs->receive_chunk + (MAX_UDP_SIZE * slice), buffer, MAX_UDP_SIZE);
        }
        else {
            LOG_DEBUG(NetworkState, "%s ALREADY RECEIVED CHUNK SLICE: %d", connection, slice);
        }

    

        cs->received_slice_this_tick = true;



        return 0;
    }


    bool handle_ack(network_state* NetworkState, packet_header& header, ConnectionState* cs, int clientID, int ack){

        // labour::performance_timer::Timer ackTimer;
        // ackTimer.reset();

        const char* connection = SERVER_STR;
        if(cs->is_client){
            connection = CLIENT_STR;
        }

        bool handled = false;

        // This bit is set (1)
        // Do something with i
        // uint16_t bit_ack = (ack - (i + 1));
        uint16_t tick = ack & (SNAPSHOT_BUFFER_SIZE - 1);
        // LOG_TRACE(NetworkState, "%s INSIDE BITFIELD LOOP, i: %d", connection, i);
        
        if(cs->in_flight_sequences[tick] != ack){
            //bit_ack is the index into the array where we store the sequences
            //we also use it to directly compare against the sequences stored there. is that a problem
            // LOG_TRACE(NetworkState, "%s in_flight_sequence %d DOESNT MATCH compared sequence: %d, tick: %d",connection, cs->in_flight_sequences[tick], ack, tick);
            return false;
        }

        // if(header.id != cs->packet_resend_IDs[tick]){
        //     log_to_console(NetworkState, LogTypes::Trace, "%s, header.id %d NOT EQUAL to  stored id: %d",connection, header.id, cs->packet_resend_IDs[tick]);
        //     return false;
        // }


        //REMOVE, it has been acknowledged, clear this tick slot, decrement count
        log_to_console(NetworkState, LogTypes::Debug, "%s sequence: %d %sACKNOWLEDGED%s",connection, cs->in_flight_sequences[tick], BGREEN, RESET);
        
        //CLEAR IN FLIGHT BITFIELD
        uint16_t acked_sequence = cs->in_flight_sequences[tick];
        uint16_t diff = (cs->latest_in_flight_sequence - acked_sequence) & 65535;
        if (diff > 63){
            LOG_ERROR(NetworkState, "%s latest sent sequence %d is GREATER than acked sequence: %d by over 63 sequences???", connection, cs->latest_in_flight_sequence, acked_sequence);
            assert(diff <= 63 && "latest sent sequence is GREATER than acked sequence by over 63 sequences ERROR");
        }
        //EXAMPLE: latest is 8, acked sequence is 6, so diff is 2
        //EXAMPLE: latest is 8, so bitfield starts at 7, second bit is 6
        if(diff > 0){
            cs->in_flight_bitfield &= ~(1ULL << (diff - 1));
        }
        else{
            LOG_DEBUG(NetworkState, "%s latest sent sequence %d %sACKNOWLEDGED%s", connection, cs->latest_in_flight_sequence, BGREEN, RESET);
            cs->latest_in_flight_sequence_acked = true;
        }
        //END CLEAR IN FLIGHT BITFIELD

        if(acked_sequence > cs->latest_acked_in_flight_sequence || ( cs->latest_acked_in_flight_sequence - acked_sequence) > UINT16_MAX/2){
            cs->latest_acked_in_flight_sequence = acked_sequence;
        }
        
        if(acked_sequence == cs->oldest_in_flight_sequence){
            int oldest_sequence = -1; // Start with -1, indicating no valid sequence found.

            for (int j = 0; j < cs->in_flight_packet_count; j++) {
                uint16_t current_sequence = cs->packed_sequences[j];

                // If oldest_sequence is uninitialized (-1) or the current sequence is older
                if (oldest_sequence == -1 || 
                    (int16_t)(current_sequence - (uint16_t)oldest_sequence) < 0) {
                    oldest_sequence = current_sequence;
                }
            }
            cs->oldest_in_flight_sequence = oldest_sequence;
        }
        

        clear_sequence(NetworkState, cs, tick, false);

    
        // LOGLINE_TRACE(NetworkState, "ack took: %f seconds\n",ackTimer.elapsed());
        // ackTimer.stop();
        //0.004871 seconds

        handled = true;
        return handled;
    }

    int receive_packet(network_state* NetworkState, bool sim_client, uint8_t sim_clientID){

        const char* connection = SERVER_STR;
        if(sim_client)connection = CLIENT_STR;

        char udp_buffer[MAX_UDP_SIZE] = {0};
        sockaddr_in sender_addr;
        socklen_t addr_len = sizeof(sender_addr);
        packet_header header;
        packet_header debug_header;
        int bytes = 0;

        if(NetworkState->netTest.enabled || NetworkState->serverSockets.active){
            bytes = packet_manager_receive_binary_udp(NetworkState, NetworkState->serverSockets.udp_socket, header, debug_header, (unsigned char*)udp_buffer, sizeof(udp_buffer),  (sockaddr*)&sender_addr, &addr_len, sim_clientID, sim_client);
            if(bytes < 0){
                // LOG_ERROR(NetworkState, "%s bytes < 0, bytes = %d", connection, bytes);
                return -1;
            }
        }
        else if(NetworkState->clientSockets.active){
            bytes = packet_manager_receive_binary_udp(NetworkState, NetworkState->clientSockets.udp_socket, header, debug_header, (unsigned char*)udp_buffer, sizeof(udp_buffer),  (sockaddr*)&sender_addr, &addr_len);
        }


        if (bytes < 0) {
            #ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK)
            #else
                if (errno == EAGAIN || errno == EWOULDBLOCK)
            #endif
                    // No more packets available right now
                    // LOG_ERROR(NetworkState, "%s no more packets available right now", connection);

                    return -1;
            
            // Handle actual errors
            printf("Error receiving UDP packet: %d\n", 
                #ifdef _WIN32
                    WSAGetLastError()
                #else
                    errno
                #endif
            );
            return -1;
        }

        if (bytes > 0) {
            ConnectionState* cs = nullptr; 

            // uint32_t now = GetTimeMS();
            uint32_t now = GetTimeMS()/*REPLACE*/;
            
            uint8_t clientID = 0;

            NetTest& nt =       NetworkState->netTest;
            
            packet_entry entry = {};
            int sender_index = UINT16_MAX;

            if(sim_client){//a simulated client is calling this function
                cs = &nt.client_connection_state[sim_clientID]; //the simulated client accesses its connection state with the server
                clientID = sim_clientID;
                connection = CLIENT_STR;
                sender_index = 0; //client is active, it only communicates with the server

            }else{
                // Find which client sent this
                sender_index = NetworkState->clientManager.tokenToID[header.token];

                if(sender_index == UINT16_MAX){
                    sender_index = -1;

                    //we NEED to init the sender here, read the message, if its not a connection request, drop it immediately.
                    unpack((unsigned char*)udp_buffer, "CH", &entry.type, &entry.size);

                    // if(entry.size <= 0){
                    //     LOG_ERROR(NetworkState, "%s entry size is %d, breakpoint activated", connection, entry.size);
                    //     LOG_ERROR(NetworkState, "%s entry size is %d, breakpoint activated", connection, entry.size);
                    // }
                    if(entry.size <= 0 ){
                        LOG_ERROR(NetworkState, "receive_packet() ENTRY SIZE IS 0!");
                        return 0;
                    }


                    if(entry.type == MessageTypes::MSG_CONNECT_REQUEST){
                        //doesnt have a reference to it since we need to put this in the server specific code
                        LOG_DEBUG(NetworkState, "%s SERVER HANDLING CLIENT CONNECTION HANDSHAKE", connection);
                        int result = assignClient(NetworkState, sender_addr, clientID);
                        if(result == 0){
                            handle_connection_handshake(NetworkState, udp_buffer, clientID);
                            sender_index = clientID;
                        }
                        else if(result == 1){
                            //client already added
                            sender_index = clientID;
                        }
                        else{
                            LOG_DEBUG(NetworkState, "%s FAILED TO ADD CLIENT TO SERVER, IGNORING!", connection);
                        }

                        //need to handle acknowledgement, dont return yet
                        // return 0;
                    }
                    if(sender_index == -1){
                        LOG_ERROR(NetworkState, "%s sender_index is -1, UNKNOWN SENDER, RETURNING!", connection);
                        return 0;
                        // assert(sender_index != -1 && "SERVER, sender_index is -1! ERROR");

                    }
                }else{//client successfully sent the token
                    cs = &NetworkState->connectionState[sender_index]; //the server accesses its connection state with the client/real client accesses real connection state

                    if(!cs->connection_handshake_completed){
                        //client sent the correct token
                        cs->connection_handshake_completed = true;
                    }
                }
                //it happens because the sender_index is -1

                cs = &NetworkState->connectionState[sender_index]; //the server accesses its connection state with the client/real client accesses real connection state


            }


            
    




            if(!cs){
                // spdlog::error("connection state is null??");
                LOG_ERROR(NetworkState, "connection state is null?? RETURNING");
                return 0;
            }
            
            LogTypes logLevel = LogTypes::Debug;

            if(sim_client || NetworkState->clientSockets.active){
                connection = CLIENT_STR;
            }


            log_to_console(NetworkState, logLevel, "%s %sreceive_packet()%s, %d : %d , in flight packets: %d, incoming latent (travelling) packets: %d, tick: %d, now: %u", connection, MAGENTA, RESET, header.sequence, header.id, cs->in_flight_packet_count, cs->socket_message_count, (header.sequence & (SNAPSHOT_BUFFER_SIZE - 1)), now);
            // log_to_console(NetworkState, logLevel, "UDP bytes received: header: %d, payload: %d, total: %d",PACKED_MESSAGE_HEADER_SIZE, bytes, header.size + PACKED_MESSAGE_HEADER_SIZE);

            log_to_console(NetworkState, logLevel, "RECEIVED  %d:%d, ack: %d, size: %d, total_fragments: %d, fragment_number: %d", header.sequence, header.id , header.ack, header.size, header.total_fragments, header.current_fragment);
            // log_to_console(NetworkState, logLevel, "RECEIVED ack: %d, bitfield: ", header.ack);


            if (header.current_fragment < 0 || header.current_fragment > header.total_fragments) {
                LOG_ERROR(NetworkState, "Invalid fragment index: %d total fragments: %d", header.current_fragment, header.total_fragments);
                return 0; // or handle the error appropriately
            }

            if(header.sequence == UINT16_MAX && header.total_fragments > 7){
                //chunks must always have a max sequence and more than 8 fragments
                LOG_DEBUG(NetworkState, "%s RECEIVING CHUNK SLICE!!!!", connection);
                LOG_DEBUG(NetworkState, "%s RECEIVING CHUNK SLICE!!!!", connection);
                int result = receive_chunk_slice(NetworkState, cs, header, udp_buffer);
                assert(result == 0 && "ERROR RECEIVING CHUNK SLICE");
                return 0;//the chunk receive function ostensibly handles everything it needs to so we can return from here, we handled this specific packet
            }


            //regular packets processed here


            // assert(header.total_fragments  < 8 && "TOTAL FRAGMENTS EXCEED 8, NOT YET IMPLEMENTED CHUNKS OR SLICES, ERROR!");
            // assert(header.current_fragment < 8 && "CURRENT FRAGNMENT EXCEEDS 8, NOT YET IMPLEMENTED CHUNKS OR SLICES, ERROR!");
            

            uint16_t curr_ack = header.ack;
            uint64_t ack_bitfield = header.ack_bitfield;
            uint16_t current_remote_sequence = header.sequence;
            uint16_t& last_remote_sequence = cs->remote_sequence;
            
            bool& client_synced = cs->client_synced;

            uint16_t received_tick = (current_remote_sequence & (SNAPSHOT_BUFFER_SIZE - 1));
            



            char char_bitfield[65] = {0}; // Enough for 64 bits + null terminator
            for (int i = 63; i >= 0; i--) {
                char_bitfield[63 - i] = ((header.ack_bitfield >> i) & 1) + '0'; // Convert bit to char
            }
            log_to_console(NetworkState, logLevel, "%s", char_bitfield);


            uint64_t& local_ack_bitfield = cs->ack_bitfield;
            // uint64_t& ordered_bitfield = cs->ordered_bitfield;
            // uint8_t& buffer_count = cs->ordered_buffer_count;

            //TODO: client first connect bitfield init. could be better. currently have it with a bool that we need to be careful with
            //client hasnt connected yet, we check again when populating the bitfield and then set it to true
            //there is no previous sequence or packet to acknowledge, last remote sequence can be reset
            // if(!client_synced){
            //     log_to_console(NetworkState, logLevel, "receive_packet() clientID %d first connect/need to sync", clientID);
            //     last_remote_sequence = current_remote_sequence - 1;
            // }
            // LOG_DEBUG(NetworkState, "%s received sequence: %d\n",connection, header.sequence);

            bool valid_entry = true;
            /*When we receive a packet, we check the sequence number of the packet against the remote sequence number. 
            If the packet sequence is more recent, we update the remote sequence number.*/
            if(((cs->received_sequences             [received_tick]    != current_remote_sequence  )   && //sequence AND id is not the same as received
                (cs->received_ids                   [header.id & (ID_BUFFER_SIZE - 1)]    != header.id                ))  ||//id buffer is larger, index into it based on id tick
                (now - cs->received_sequence_times  [received_tick]    >  120000                   )) //if one of those is not true, check received time, 
                                                                                                        //if the last match was received over 2 minutes ago, its new
            {

                bool all_fragments_received = false;
  
  
                //if the slot has fragments, we should clear them out if its been over 64 ticks since it was last populated
                //if this slot was populated over 64 ticks ago, the data has most likely grown stale.
                // if((now - cs->received_sequence_times  [received_tick]) > NetworkState->netTest.tick_timestep_ms * SNAPSHOT_BUFFER_SIZE){
                //instead, if the received sequence is not the same as the previous slot, it is probably new and we should wipe the old data
                if(cs->received_sequences      [received_tick] != current_remote_sequence){
                    
                    // LOG_DEBUG(NetworkState, "%s sequence: %d tick: %d last received OVER 64 ticks ago, %d ms ago, greater than max timeout of: %d, data is stale. resetting", connection, current_remote_sequence, received_tick, now - cs->received_sequence_times  [received_tick], NetworkState->netTest.tick_timestep_ms * SNAPSHOT_BUFFER_SIZE);
                    //clear out the potentially stale fragment data in case it was dropped
                    memset(cs->remote_received_history[received_tick], 0, MAX_PACKET_SIZE);
                    cs->received_history_size[received_tick] = 0;
                    cs->fragment_bitfield[received_tick] = 0;
                    //need to clear the oth
                    cs->received_sequences             [received_tick] = -1;
                    cs->received_ids                   [header.id & (ID_BUFFER_SIZE - 1)] = -1; //id buffer is larger, index into it based on id tick
                    cs->received_sequence_times         [received_tick] = now; 

                }

                //check if fragment has already been received
                if(cs->fragment_bitfield[received_tick] & (1ULL << header.current_fragment)){
                    LOG_DEBUG(NetworkState, "%s ALREADY RECEIVED FRAGMENT: %d FROM SEQUENCE: %d, time dif: %d RETURNING", connection, header.current_fragment, header.sequence, now - cs->received_sequence_times  [received_tick]);
                    return 0;
                }

                uint8_t testType = 0;
                uint16_t testSize = 0;
                unpack((unsigned char*)udp_buffer, "CH", &testType, &testSize);
                // LOG_DEBUG(NetworkState, "%s received buffer entry type: %d, size: %d", connection, testType, testSize);

                memcpy(cs->remote_received_history[received_tick] + (header.current_fragment * MAX_UDP_SIZE), udp_buffer, bytes);
                cs->received_history_size[received_tick] += bytes;
                cs->fragment_bitfield[received_tick] |= (1ULL << header.current_fragment);

                //just be stupid and loop over all the bits
                int count = 0;
                for(int i = 0; i < 8; i++){
                    if(cs->fragment_bitfield[received_tick] & (1ULL << i)){
                        count++;
                    }
                    else{
                        break;
                    }
                }

                if(header.total_fragments + 1 == count){//all fragments received
                    all_fragments_received = true;
                }else{
                    LOG_DEBUG(NetworkState, "%s total_fragments: %d for sequence: %d NOT RECEIVED, returning", connection, header.total_fragments, header.sequence);
                    return 0;
                }



                uint16_t difference = (current_remote_sequence - last_remote_sequence) & 65535; //determine the offset
                
                if(sequence_greater_than(current_remote_sequence + 1, last_remote_sequence)){

                    //handle a received sequence that skipped 1 or more sequences between it and our remote_sequence
                    if (difference > 1) {

                        //clear entries between latest sequence and received
                        for(int i = 0; i < difference; i++){
                            int index = (i + last_remote_sequence) & (SNAPSHOT_BUFFER_SIZE - 1);
                            cs->received_sequences  [index] = -1;
                        }

                        // log_to_console(NetworkState, LogTypes::Debug, "out of order, current: %d, last remote: %d, diff: %d", current_remote_sequence, last_remote_sequence, difference);

                        // if(difference >= 64){ //probably not a problem, going to ignore it for now, if its greater, even if we miss a large portion, we should take it
                        //     LOG_ERROR(NetworkState, "out of order sequence a whole second ahead?? current: %d, last remote: %d", current_remote_sequence, last_remote_sequence);
                        //     valid_entry = false;
                        // }

                        // log_to_console(NetworkState, LogTypes::Debug, "MISSING A SEQUENCE: CURRENT: %d, LAST: %d", current_remote_sequence, last_remote_sequence);

                    }

                    if(valid_entry){
                        last_remote_sequence = current_remote_sequence;

                        //if the client has not yet connected yet, dont push back the bitfield
                        if(!client_synced){
                            //there is no previous sequence or packet to acknowledge, don't push a bit back
                            client_synced = true;
                        }
                        else{
                            // local_ack_bitfield = (local_ack_bitfield << 1) | 1; // Shift left and set the new sequence bit
                            local_ack_bitfield <<= (difference); // Shift left and set the new sequence bit
                            if(difference > 0)local_ack_bitfield  |= (1ULL << (difference - 1));

                        }
                    }
                 
                    
                }
                else{//received sequence is less than our current remote sequence
                    // log_to_console(NetworkState, LogTypes::Debug, "received sequence: %d, less than current tracked sequence: %d", current_remote_sequence, last_remote_sequence);
                    //check if this sequence has been received yet, if not populate it and add it to the ack bitfield
                    difference = (last_remote_sequence - current_remote_sequence) & 65535;
                    if(difference < SNAPSHOT_BUFFER_SIZE){
                        //set bit in bitfield here
                                          //if the client has not yet connected yet, dont push back the bitfield
                        if(!client_synced){
                            //there is no previous sequence or packet to acknowledge, don't push a bit back
                            client_synced = true;
                        }
                        else{
                            // local_ack_bitfield = (local_ack_bitfield << 1) | 1; // Shift left and set the new sequence bit
                            if(difference > 0)local_ack_bitfield  |= (1ULL << (difference - 1));
                        }
                    }
                }

                if(valid_entry){
                    

                    cs->received_sequences      [received_tick] = current_remote_sequence;
                    cs->received_ids            [header.id & (ID_BUFFER_SIZE - 1)] = header.id; //id buffer is larger, index into it based on id tick
                    cs->received_sequence_times [received_tick] = now;
                    // LOG_DEBUG(NetworkState, "%s RECEIVED SEQUENCE: %d TICK: %d", connection, current_remote_sequence, received_tick);
                    cs->remote_header_history   [received_tick][header.current_fragment] = header;

                    //TODO:
                    //memcpy the buffer into our own received buffer/recent buffer history
                }else{
                    LOG_DEBUG(NetworkState, "%s RECEIVED INVALID SEQUENCE: %d TICK: %d ", connection, current_remote_sequence, received_tick);
                }

            }
            else{//the received sequence has already been received
                LOG_DEBUG(NetworkState, "%s sequence: %d, id: %d ALREADY RECEIVED, IGNORING", connection, current_remote_sequence, header.id);
                return 0;
            }
            cs->ack = last_remote_sequence;

            char* full_buffer = cs->remote_received_history[received_tick];
            int remaining =     cs->received_history_size[received_tick];
            int processed = 0;
            int entry_count = 0;

            while(remaining > 0){//process all entries in the packet
                unpack((unsigned char*)full_buffer + processed, "CH", &entry.type, &entry.size);

                // if(entry.size <= 0){
                //     LOG_ERROR(NetworkState, "%s entry size is %d, breakpoint activated", connection, entry.size);
                //     LOG_ERROR(NetworkState, "%s entry size is %d, breakpoint activated", connection, entry.size);
                // }
                // assert(entry.size > 0 && "receive_packet() ENTRY SIZE IS 0!");
                if(entry.size <= 0){
                    LOG_DEBUG(NetworkState, "%s ENTRY SIZE 0! END OF RECEIVED BUFFER REACHED, entry type: %d, size: %d, BREAK OUT OF READ LOOP", connection, entry.type, entry.size);
                    break;
                }
                // LOG_DEBUG(NetworkState, "%s entry.type: %d, entry.size: %d", connection, entry.type, entry.size);



                if(!sim_client && NetworkState->serverSockets.active){
                    if(sender_index >= 0) {
                        // log_to_console(NetworkState, logLevel, "%s UDP from client %d", connection, sender_index);
                        // log_to_console(NetworkState, logLevel, "client token: %d corresponds to clientID: %d", header.token, NetworkState->clientManager.tokenToID[header.token]);
                    }
                    else{
                        LOG_ERROR(NetworkState, "poll_network_messages() MESSAGE RECEIVED %d clientID?? RETURNING", header.token);
                        LOG_ERROR(NetworkState, "poll_network_messages() MESSAGE RECEIVED %d clientID?? RETURNING", header.token);
                        continue;
                    }
                }
                else{

                }
            
            
                
                PacketManager& pm = NetworkState->packetManager;


                // clientID = sender_index;




                // #ifndef SERVER_BUILD
                if(entry.type == MessageTypes::MSG_CONNECTED_HANDSHAKE){
                    if(!cs->connection_handshake_completed){
                        cs->connection_handshake_completed = true;
                        handle_connected_handshake_response(NetworkState, full_buffer, *cs);
                    }
                }
                else if(entry.type == MessageTypes::RESYNC_DUMMY){
                    // LOG_WARN(NetworkState, "%s RECEIVED RESYNC DUMMY PACKET!", connection);
                    // LOG_WARN(NetworkState, "%s RECEIVED RESYNC DUMMY PACKET!", connection);
                }

                else if(entry.type == MessageTypes::CHUNK_SEND_REQUEST){
                    LOG_DEBUG(NetworkState, "%s CHUNK SEND REQUEST RECEIVED", connection);
                    LOG_DEBUG(NetworkState, "%s CHUNK SEND REQUEST RECEIVED", connection);
                    cs->ready_to_receive_chunk = true;
                    cs->receiving_chunk = true;
                    prepare_to_receive_chunk(NetworkState, cs);
                    memset(cs->scratch_buffer, 0, MAX_PACKET_SIZE);
                    int entrysize = pack((unsigned char*)cs->scratch_buffer, "CH", MessageTypes::CHUNK_RECEIVE_READY, 3);
                    // int result = sim_append_to_packet(NetworkState, *cs, cs->scratch_buffer, entrysize);
                    int result = sim_append_network_command(NetworkState, *cs, cs->scratch_buffer, entrysize);
                    
                    if(result != 0){
                        LOG_ERROR(NetworkState, "%s COULD NOT APPEND CHUNK RECEIVE READY ENTRY TO PACKET", connection);
                    }
                }

                else if(entry.type == MessageTypes::CHUNK_RECEIVE_READY){
                    LOG_DEBUG(NetworkState, "%s CHUNK SEND CAN NOW PROCEED", connection );
                    LOG_DEBUG(NetworkState, "%s CHUNK SEND CAN NOW PROCEED", connection );
                    prepare_to_send_chunk(NetworkState, cs);
                    cs->waiting_to_send_chunk = false; //we know we can send the chunk over now
                    cs->sending_chunk = true;
                }

                else if(entry.type == MessageTypes::END_NET_CMDS){//just to test to see if we can distinguish between end of strictly net layer commands and game info
                    // LOG_DEBUG(NetworkState, "%s END OF NET COMMANDS, BREAK OUT OF LOOP", connection);
                }
                // if(entry.type == MessageTypes::SIMULATION){//just to make sure we can see the sim data, and we can as of splitting into net commands and game commands
                //     LOG_DEBUG(NetworkState, "%s SIMULATION DATA RECEIVED", connection);
                //     LOG_DEBUG(NetworkState, "%s SIMULATION DATA RECEIVED", connection);
                //     LOG_DEBUG(NetworkState, "%s SIMULATION DATA RECEIVED", connection);
                // }
                else if(entry.type == MessageTypes::CHUNK_ACK_BITFIELD){
                    LOG_DEBUG(NetworkState, "%s CHUNK ACK BITFIELD RECEIVED", connection);

                    unpack((unsigned char*)full_buffer + processed + 3, "QQQQ", &cs->send_chunk_bitfield[0], &cs->send_chunk_bitfield[1], &cs->send_chunk_bitfield[2], &cs->send_chunk_bitfield[3]);

                    print_send_chunk_bitfield(NetworkState, *cs);

                    //just be stupid and loop over all the bits
                    int count = 0;
                    for(int i = 0; i < cs->num_slices; i++){
                        int array_index = i / 64;
                        int bit_index = i & 63;
                        if(cs->send_chunk_bitfield[array_index] & (1ULL << bit_index)){
                            count++;
                        }
                    }
                    if(count == cs->num_slices){
                        LOG_DEBUG(NetworkState, "%s SUCCESSFULLY SENT ENTIRE CHUNK!, all %d slices sent, total sent: %d!", connection, count, cs->sent_slices);
                        cs->sending_chunk = false;

                        memset(cs->scratch_entry_buffer, 0, MAX_UDP_SIZE);
                        int entrysize = pack((unsigned char*)cs->scratch_entry_buffer, "CH", MessageTypes::CHUNK_SEND_FINISHED, 3);
                        int result = sim_append_network_command(NetworkState, *cs, cs->scratch_entry_buffer, entrysize);
                        memset(cs->scratch_entry_buffer, 0, MAX_UDP_SIZE);

                    }
                    else{
                        LOG_DEBUG(NetworkState, "%s SENT %d OUT OF %d SLICES!",connection, count, cs->num_slices);
                        LOG_DEBUG(NetworkState, "%s SENT %d OUT OF %d SLICES!",connection, count, cs->num_slices);
                    }


                }

                else if(entry.type == MessageTypes::CHUNK_SEND_FINISHED){
                    LOG_DEBUG(NetworkState, "%s SUCCESSFULLY FINISHED SENDING CHUNK!", connection);
                    cs->receiving_chunk = false;

                }


                LOG_TRACE(NetworkState, "%s remaining bytes: %d",connection, remaining);
                remaining -= entry.size;
                
                processed += entry.size;
                entry_count++;
                
            }
            if(sender_index < 0){
                LOG_ERROR(NetworkState, "%s sender_index: %d", connection, sender_index);
                assert(sender_index >= 0 && "receive_packet() SENDER INDEX IS NULL");
            }

            //TODO:
            //need to save received buffer and header for the time being in some temp buffer for the game layer to retrieve

            // #endif


            //if packet is not reliable, push it directly to the buffer
            //if is reliable, handle all this shit

            // log_to_console(NetworkState, logLevel, "BEFORE:count: %d,  ordered_bitfield:  ", buffer_count);
            // for (int i = 63; i >= 0; i--) {
            //     char_bitfield[63 - i] = ((ordered_bitfield >> i) & 1) + '0'; // Convert bit to char
            // }
            // log_to_console(NetworkState, logLevel, "%s", char_bitfield);



            /*
            //this from when I thought I needed to sort received packets, but I only need to do this for fragments
            //example: if header type = fragment, insert to ordered buffer etc, same with chunks/slices
            //process ordered_buffers
            int processed_count = 0;
            bool first = true;
            for(uint8_t i = 0; i < buffer_count; i++){
                if ((ordered_bitfield >> i) & 1) {
                    processed_count++;

                    //how exactly do we process the newly received packets?
                    packet_header current_header = cs->ordered_header[i];
                    char current_buffer[MAX_UDP_SIZE];
                    memset(current_buffer, 0, MAX_UDP_SIZE);
                    memcpy(current_buffer, cs->ordered_buffer[i], current_header.size);

                    // /*
                    // we have the current header/buffer. we need to determine the tick/slot to insert them into the REMOTE buffers
                    uint16_t remote_tick = (header.sequence & (SNAPSHOT_BUFFER_SIZE - 1));

                    cs->latest_unprocessed_packet = remote_tick;
                    
                    //increment per packet we need to process for the game layer to know
                    cs->unprocessed_packet_count++;
                    
                    if(first){//need to track the first packet we move over and set the current packet to it so we can access it in the game layer
                        cs->current_processed_packet  = remote_tick;

                        //oldest packet needs better methods of tracking
                        // pm.oldest_processed_packet  [clientID] = remote_tick;
                        first = false;
                    }
                    //
                    
                    //when we process and add the buffer to the history, we also shift the ack bitfield
                    log_to_console(NetworkState, logLevel, "client: %d, ack =  %d, setting to: %d", clientID, cs->ack, header.sequence);
                    cs->ack = header.sequence; //set the ack to the latest sequence


                    //don't want to push a non acked sequence into the bitfield in the case of the client first connecting
                    if(!client_synced){
                        //there is no previous sequence or packet to acknowledge, don't push a bit back
                        client_synced = true;
                    }
                    else{
                        local_ack_bitfield = (local_ack_bitfield << 1) | 1; // Shift left and set the new sequence bit
                    }
    




                    //clear that entry in ordered_buffer
                    packet_header empty_header = {};
                    memset(cs->ordered_buffer[i], 0, MAX_UDP_SIZE);
                    cs->ordered_header[i] = empty_header; //need to store the header seperately



                }
                else{
                    //end of processable packets
                    break;
                }
            }*/


            // if(processed_count > 0){
            //     ordered_bitfield = (ordered_bitfield >> processed_count);
            //     buffer_count -= processed_count;
            // }

            // log_to_console(NetworkState, logLevel, "after ordered buffers processing:");
            
            // log_to_console(NetworkState, logLevel, "AFTER: count: %d,  ordered_bitfield:  ", buffer_count);


            // for (int i = 63; i >= 0; i--) {
            //     char_bitfield[63 - i] = ((ordered_bitfield >> i) & 1) + '0'; // Convert bit to char
            // }
            // log_to_console(NetworkState, logLevel, "%s", char_bitfield);


            // if(!NetworkState || ShouldThreadsExit){
            //     log_to_console(NetworkState, logLevel, "receive_packet() after select() timeout, THREAD SHOULD EXIT");
            //     return -1;
            // }

            // log_to_console(NetworkState, logLevel, "current ack : %d, local_ack_bitfield: ", cs->ack);

            // for (int i = 63; i >= 0; i--) {
            //     char_bitfield[63 - i] = ((local_ack_bitfield >> i) & 1) + '0'; // Convert bit to char
            // }
            // log_to_console(NetworkState, logLevel, "%s", char_bitfield);


            //adjust ack bitfield
            //start at last_remote_sequence. bitfield counts back from there
            //the bitfield is basically an array. each next remote sequence we receive
            // log_to_console(NetworkState, logLevel, "%s latest acked sequence on our side: %s%d%s",connection, MAGENTA, last_remote_sequence, RESET);

            /*
            Additionally, when a packet is received, 
            ack bitfield is scanned and if bit n is set, 
            then we acknowledge sequence number packet sequence - n, if it has not been acked already.
            */


            uint16_t num_packets_in_flight = cs->in_flight_packet_count;
            
            uint16_t tick = (header.ack & (SNAPSHOT_BUFFER_SIZE - 1)); //send one sequence per tick



            bool acked = handle_ack(NetworkState, header, cs, clientID, header.ack);


            bool ack_bitfield_populated = ack_bitfield > 0;
            bool in_flight_bitfield_populated = cs->in_flight_bitfield > 0;
            bool in_flight_packet_acked = false;


            for(int i = 0; i < 64; i++) {
                if(ack_bitfield & (1ULL << i)) {
                    // This bit is set (1)
                    // Do something with i
                    uint16_t bit_ack = (curr_ack - (i + 1));
                    in_flight_packet_acked = handle_ack(NetworkState, header, cs, clientID, bit_ack);

                }
                else{ //BIT IS 0, THIS PACKET HAS NOT BEEN ACKNOWLEDGED
                    continue;//do nothing, they will be scanned next tick and resent if needed
                }
            
            }

            // if(!in_flight_packet_acked && ack_bitfield_populated && in_flight_bitfield_populated){
            //     LOG_DEBUG(NetworkState, "%s no sequences were acked from the bitfield?", connection);
            //     LOG_DEBUG(NetworkState, "%s latest sent sequence: %d, in_flight_bitfield", connection, cs->latest_in_flight_sequence);
                
            //     for (int i = 63; i >= 0; i--) {
            //         char_bitfield[63 - i] = ((cs->in_flight_bitfield >> i) & 1) + '0'; // Convert bit to char
            //     }
            //     log_to_console(NetworkState, logLevel, "%s", char_bitfield);

            //     LOG_DEBUG(NetworkState, "%s         received ack: %d, received ack bitfield:", connection, header.ack);
                
            //     for (int i = 63; i >= 0; i--) {
            //         char_bitfield[63 - i] = ((header.ack_bitfield >> i) & 1) + '0'; // Convert bit to char
            //     }
            //     log_to_console(NetworkState, logLevel, "%s", char_bitfield);

            // }
            // else{
            //     LOG_DEBUG(NetworkState, "%s latest sent sequence: %d, in_flight_bitfield", connection, cs->latest_in_flight_sequence);
            //     for (int i = 63; i >= 0; i--) {
            //         char_bitfield[63 - i] = ((cs->in_flight_bitfield >> i) & 1) + '0'; // Convert bit to char
            //     }
            //     log_to_console(NetworkState, logLevel, "%s", char_bitfield);
            // }

        
        }
        return 0;

    }




    int packet_manager_receive_binary_udp(network_state* NetworkState, int socket, packet_header& header, packet_header& debug_header, unsigned char* buffer, int buffer_size, sockaddr* src_addr, socklen_t* addr_len, uint8_t clientID, bool sim_client){
        
        const char* connection = SERVER_STR;
        ConnectionState* cs = nullptr;
        if(sim_client){
            connection = CLIENT_STR;
            //simulated client is receiving
            cs = &NetworkState->netTest.client_connection_state[clientID];
        }else{
            cs = &NetworkState->connectionState[clientID];
        }

        char recv_buffer[SINGLE_FRAGMENT_SIZE] = {0};

        int bytes = 0;
        //determines if we should use the real or the simulated function
        if(NetworkState->netTest.enabled){
            bytes = sim_recvfrom(NetworkState, recv_buffer, src_addr, debug_header, clientID, sim_client, cs);
            if(bytes < PACKED_MESSAGE_HEADER_SIZE) {
                // LOG_ERROR(NetworkState, "%s received UDP message too small! bytes: %d\n",connection, bytes);
                return -1;
            }
        }
        else{
            //return the length of the message on successful completion
            bytes = recvfrom(socket, recv_buffer, sizeof(recv_buffer), 0, src_addr, addr_len);
        }
        if(bytes < 0) {
            // Consider adding error handling here - could be EAGAIN, EWOULDBLOCK, etc.
            // LOG_ERROR(NetworkState, "%s receive_message_udp() bytes less than 0? bytes: %d\n",connection, bytes);
            return -1;
        }
        
        if(bytes < PACKED_MESSAGE_HEADER_SIZE) {
            // LOG_ERROR(NetworkState, "%s received UDP message too small! bytes: %d",connection, bytes);
            return -1;
        }
        unpack((unsigned char*)recv_buffer, "HCCHHHHQ", &header.token, &header.total_fragments, 
                &header.current_fragment, &header.id, &header.sequence, &header.ack, &header.size, &header.ack_bitfield);
        

        if(header.size > buffer_size){
            // LOG_ERROR(NetworkState, "%s received UDP message buffer too small! header->size: %d, buffer_size: %d",connection, header.size, buffer_size);
            return -2;
        }

        if(AdjustBandwidthBudget(NetworkState, cs->bandwidth_accumulator, bytes) < 0){
            uint32_t bits = bytes * 8;
            LOG_ERROR(NetworkState, "%s (RECEIVE) OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, cs->bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (RECEIVE) OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, cs->bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (RECEIVE) OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, cs->bandwidth_accumulator, bits);
        }

        if(cs->is_server && AdjustBandwidthBudget(NetworkState, NetworkState->total_bandwidth_accumulator, bytes) < 0){
            uint32_t bits = bytes * 8;
            LOG_ERROR(NetworkState, "%s (RECEIVE) SERVER OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, NetworkState->total_bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (RECEIVE) SERVER OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, NetworkState->total_bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (RECEIVE) SERVER OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, NetworkState->total_bandwidth_accumulator, bits);
        }

        //copy payload to users buffer
        if(header.size > 0){
            memcpy(buffer, recv_buffer + PACKED_MESSAGE_HEADER_SIZE, header.size);
        }


        // pm.packet_resend_timeouts[]
        

        return bytes - PACKED_MESSAGE_HEADER_SIZE;
    }
    

    int check_in_flight_packets(network_state* NetworkState, ConnectionState& cs, int clientID){


        NetTest& nt = NetworkState->netTest;
        //print out number of in flight packets
        // spdlog::info("in flight packets: {}", cs.in_flight_packet_count);
        const char* connection = CLIENT_STR;
        sockaddr* addr = nullptr;
        if(cs.is_client){
            // addr = (sockaddr*)&nt.client_udp_addrs[clientID];
        }
        else{
            // addr = (sockaddr*)&nt.server_udp_addr;
            connection = SERVER_STR;
        }
        
        addr = (sockaddr*)&cs.socket_udp_addr;
        
        // LOG_INFO(NetworkState, "%s in flight packets: %d, incoming latent (travelling) packets: %d, latest send sequence: %d", connection, cs.in_flight_packet_count, cs.socket_message_count, cs.latest_in_flight_sequence);
        // LOG_DEBUG(NetworkState, "%s in flight packets: %d, incoming latent (travelling) packets: %d, latest send sequence: %d", connection, cs.in_flight_packet_count, cs.socket_message_count, cs.latest_in_flight_sequence);
        assert(cs.in_flight_packet_count < 64 && "IN FLIGHT PACKET COUNT IS GREATER THAN 64! ERROR");
        //loop through packets, updating the times, and determine if a packet needs resending based on RTT + RTT*.1
        
        //determine which packets should be resent
        int i = 0;
        while(i < cs.in_flight_packet_count){
            // uint32_t now = GetTimeMS();
            uint32_t now = GetTimeMS()/*REPLACE*/;

            int packed_sequence = cs.packed_sequences[i];
            if(packed_sequence == -1){
                LOG_ERROR(NetworkState, "%s packed sequence at index: %d is -1??", connection, i);
            }

            int tick = packed_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
            uint32_t time_sent = cs.packet_resend_timeouts[tick];

            uint32_t diff;
            if (now >= time_sent) {
                diff = now - time_sent;
            } else {
                // Handle the case where the timer might have wrapped around
                // or there's some other timing issue
                //sometimes if we just resent it, 
                diff = 0;
            }

            uint16_t sequence = cs.in_flight_sequences[tick];
            uint16_t id = cs.in_flight_IDs[tick];

            if(cs.in_flight_sequences[tick] == -1){
                LOG_ERROR(NetworkState, "%s in flight sequence at tick: %d is %d, should be: %d", connection, tick, cs.in_flight_sequences[tick], packed_sequence);
                print_arrays(NetworkState, &cs, LogTypes::Error);
                assert(0 && "in flight sequence is -1");
            }
            if(cs.in_flight_IDs[tick] == -1){      
                LOG_ERROR(NetworkState, "%s in flight id at tick: %d is %d, should be: %d", connection, tick, cs.in_flight_IDs[tick], id);
                print_arrays(NetworkState, &cs, LogTypes::Error);

                assert(0 && "in flight ID is -1");
            }

            uint16_t id_tick = id & (ID_BUFFER_SIZE - 1);
            int times_resent = cs.id_send_attempts[id_tick] - 1;

            if(times_resent == -1){
                print_arrays(NetworkState, &cs, LogTypes::Error);
                // printf("");
            }



            uint32_t exponent = 1 << (times_resent > 0 ? times_resent : 0);

            uint32_t RTO = (cs.SRTT * 1.5f) * (exponent);
            if(RTO == 0){
                // printf("");
                assert(0 && "RTO is 0??");

            }

            // LOG_DEBUG(NetworkState, "%s packed sequence %d sequence: %d, sent %d ms ago, RTO: %u, times resent: %d, in flight count: %d", connection, packed_sequence, sequence, diff, RTO, times_resent, cs.in_flight_packet_count);
            if(packed_sequence != sequence){
                LOG_ERROR(NetworkState, "%s packed sequence: %d, sequence: %d", connection, packed_sequence, sequence);
                print_arrays(NetworkState, &cs);
                LOG_ERROR(NetworkState, "%s packed sequence: %d, sequence: %d", connection, packed_sequence, sequence);

            }
            assert(packed_sequence == sequence && "sequence stored in packed array is not equal to sequence pulled from in flight array! ERROR");
            
            if((diff) > RTO){
                // LOG_DEBUG(NetworkState, "%s diff: %u, RTO: %u now: %u, time_sent: %u", connection, diff, RTO, now, time_sent);


                //what do we need to reset when we clear this slot after too many resend attempts?
                if(times_resent >= 3){
                    LOG_DEBUG(NetworkState, "%s tried sending id %d too many times: %d, dropping and initiating desync warning", connection, id, times_resent);

                    print_id_sequences(NetworkState, &cs, id, LogTypes::Debug);
                        
                    if(cs.is_server)cs.need_resync = true;
                    if(cs.is_server)cs.times_desynced++;
                    clear_sequence(NetworkState, &cs, tick, true);
                    
                    cs.lost_count++;
                    //continue without incrementing since we cleared a sequence from packed sequences
                    continue;
                }
                
                LOG_DEBUG(NetworkState, "%s %ssequence %d id: %d sent %d ago, RESENDING%s, latency: %d, times_resent: %d, in flight count: %d", connection, BRED, sequence, id, diff, RESET, cs.latency, times_resent, cs.in_flight_packet_count);



                char buffer[MAX_PACKET_SIZE];
                packet_header header = cs.local_header_history[tick][0];
                
                if(header.total_fragments > 0){
                    cs.lost_fragments += header.total_fragments + 1;
                    LOG_DEBUG(NetworkState, "%s LOST PACKET HAD %d FRAGMENTS, TOTAL LOST FRAGMENTS: %d", connection, header.total_fragments + 1, cs.lost_fragments);
                }

                
                memcpy(buffer, cs.local_buffer_history[tick], cs.local_history_size[tick]);

                bool resending = true;


                // uint8_t testType  = 0;
                // uint16_t testSize = 0;
                // unpack((unsigned char*) buffer, "CH", &testType, &testSize);
                // if(testType == 10){
                //     LOG_TRACE(NetworkState, "resend debug! type is client connect request! type: %d, size: %d", testType, testSize);
                //     LOG_TRACE(NetworkState, "resend debug! type is client connect request! type: %d, size: %d", testType, testSize);
                //     if(resending){
                //         LOG_DEBUG(NetworkState, "%s RESENDING SERVER CONNECTION HANDSHAKE, sequence: %d", connection, cs.local_sequence);
                //     }
                //     else if(cs.local_sequence != 0){
                //         LOG_DEBUG(NetworkState, "%s NOT RESENDING SERVER CONNECTION HANDSHAKE, sequence: %d", connection, cs.local_sequence);
                //         assert(0 && "server should not be sending the client connection handshake again!");
                //     }
                // }


                int total_sent = 0;
                int socket = 0;//dont need to worry about this for the simulation

                socklen_t addr_len = sizeof(sockaddr_in);
                uint16_t token = 0;

                if(cs.local_history_size[tick] == 0){
                    LOG_TRACE(NetworkState, "local history size is 0 for tick: %d\n", tick);
                }


                if(cs.local_history_size[tick] == 0){
                    // printf("");
                }
                assert(cs.local_history_size[tick] != 0 && "buffer size is 0???");


                int result = sim_send_binary_udp(NetworkState, socket, addr, addr_len, 
                                                    &total_sent, clientID, token, cs, 
                                                    cs.local_buffer_history[tick], cs.local_history_size[tick], !cs.is_client, &header, resending, tick);
                LOG_DEBUG(NetworkState, "%s local history buffer and size at tick: %d CLEARED", connection, tick);
                if(result == 1){
                    //resent AND overwrote, continue the loop without incrementing
                    continue;
                }
                if(result != 0){
                    LOG_ERROR(NetworkState, "%s COULDN'T RESEND LOST PACKET SEQUENCE: %d!", connection, sequence);
                }
                //ideally we would fit all this into the connectionState, as part of a simulation section...

                // char buffer[16];
                // packet_header header = {};
                // packet_entry entry = {};
                // entry.type = MessageTypes::SIMULATION;
                // entry.size = 0;
                // int packetsize = pack((unsigned char*) buffer, "CH", entry.type, entry.size);
                // header.size = entry.size;

                // ConnectionState& cs = nt.client_connection_state[clientID];


                // uint8_t& write          = nt.client_queue_write_index[clientID];
                // uint8_t& read           = nt.client_queue_read_index[clientID];
                // uint8_t& size           = nt.client_queue_count[clientID];
                // //write == read on start.. what do we do?
                // // if(size < SEND_QUEUE_SIZE){
                // //only append if the client is connected, will probably break later but good enough for now
                // if(size == 0 && cs.connected){ //for now only append if there is nothing else in the queue in case some logic elsewhere (like handle connected client) is in the queue
                //     char* append_slot       = nt.client_send_queue[clientID][write];

                //     uint16_t& buffer_size   = nt.client_send_queue_size[clientID][write]; //actual chars taken up in the current slot

                //     uint16_t& total_count   = nt.client_queue_total_count;
                    
                //     int result = sim_network_thread_append_to_packet(NetworkState, append_slot + buffer_size, buffer_size, write, read, buffer, packetsize, clientID);
                //     if(result == 0){
                //         //successfully appended to queue
                //         size++;
                //     }
                // }





            }
            i++;

        }


        return 0;
    }

    
    int packet_manager_send_binary_udp(network_state* NetworkState, int socket, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token){
    // int packet_manager_send_binary_udp(network_state* NetworkState, int socket, packet_header header, char* payload, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token){
        PacketManager& pm = NetworkState->packetManager;
        packet_header header = {};

        header.size = pm.current_buffer_size[clientID];
        if(header.size <= 0){
            //nothing to send, return
            return -1;
        }


        unsigned char* payload = (unsigned char*)pm.current_buffer[clientID];

        header.ack = pm.remote_sequence[clientID];
        header.ack_bitfield = pm.ack_bitfield[clientID];
        header.token = token;
        header.id = 0;
        // header.time_sent = GetTimeMS();
        // header.sequence = pm.local_sequence[clientID];
        header.sequence = NetworkState->local_sequence;
        
        char send_buffer[MAX_UDP_SIZE];
        int headerSize = pack((unsigned char*)send_buffer, "HCCHHHHQ", header.token, header.total_fragments, header.current_fragment, header.id, header.sequence, header.ack, header.size, header.ack_bitfield);

        if(headerSize + header.size > MAX_UDP_SIZE){
            LOG_TRACE(NetworkState, "message too big for UDP\n");
            return -3;
        }

        //copy payload right after header
        if(header.size > 0){
            memcpy(send_buffer + headerSize, payload, header.size);
        }

        int total_size = headerSize + header.size;
        if(NetworkState->netTest.enabled){
            sim_sendto(NetworkState, send_buffer, clientID, total_size, dest_addr);
        }
        else{
            if(sendto(socket, send_buffer, total_size, 0, dest_addr, addr_len) != total_size){
                LOG_TRACE(NetworkState, "UDP message sendto() FAILED\n");
                return -1;
            }
        }
        *total_sent = total_size;


        //add to field in networkState
        //tick is based off of the local sequence
        uint16_t tick = (header.sequence & (SNAPSHOT_BUFFER_SIZE - 1)); //send one sequence per tick, sequence 0 will be tick 0
        
        uint16_t index = pm.in_flight_packet_count[clientID] & (SNAPSHOT_BUFFER_SIZE - 1);

        if(index >= SNAPSHOT_BUFFER_SIZE){
            LOG_TRACE(NetworkState, "INDEX SHOULD NOT BE THIS HIGH: %d\n", index);
        }
        pm.tick_to_index[clientID][tick]  = index;
        pm.index_to_tick[clientID][index] = tick;
        
        //need to use sequence to compare against the tick, the sequence can be the same as the tick, which will give us the index into where its stored
        // pm.packet_resend_timeouts   [clientID][tick] = GetTimeMS();     //time we sent it
        pm.packet_resend_timeouts   [clientID][tick] = GetTimeMS()/*REPLACE*/;     //time we sent it
        pm.packet_resend_IDs        [clientID][tick] = header.id;            //id of the message we need to resent
        pm.in_flight_sequences      [clientID][tick] = header.sequence;

        pm.local_header_history    [clientID][tick] = header;
        memset(pm.local_buffer_history[clientID][tick], 0, MAX_UDP_SIZE); //clear slot
        memcpy(pm.local_buffer_history[clientID][tick], send_buffer, headerSize + header.size);
        
        //clear buffer for next packet
        memset(&pm.current_buffer[clientID], 0, MAX_UDP_SIZE); //clear slot
        pm.current_buffer_size[clientID] = 0;
        
        // pm.local_sequence[clientID]++;
        pm.in_flight_packet_count[clientID]++;

        return 0;
    }




    
    void init_packet_manager(network_state* NetworkState){
        // PacketManager& pm = NetworkState->packetManager;
        PacketManager* pm = &NetworkState->packetManager;
        memset(pm, 0, sizeof(PacketManager));       
        // pm->local_time_ms = GetTimeMS();
        pm->local_time_ms = GetTimeMS()/*REPLACE*/;

        pm->bandwidth_accumulator = TARGET_BANDWIDTH;
        // memset(pm.buffer, 0,  sizeof(uint8_t) * MAX_CLIENTS * MAX_UDP_SIZE);

        // memset(pm.buffer_history, 0,    sizeof(uint8_t) * MAX_CLIENTS * SNAPSHOT_BUFFER_SIZE * MAX_UDP_SIZE);
        // memset(pm.received_history, 0,  sizeof(uint8_t) * MAX_CLIENTS * SNAPSHOT_BUFFER_SIZE * MAX_UDP_SIZE);

        // memset(pm.packet_resend_timeouts, 0,  sizeof(uint16_t) * MAX_CLIENTS * SNAPSHOT_BUFFER_SIZE);
        // memset(pm.packet_resend_IDs, 0,  sizeof(uint16_t) * MAX_CLIENTS * SNAPSHOT_BUFFER_SIZE);

        // memset(pm.packet_resend_timeout_count, 0,  sizeof(uint16_t) * MAX_CLIENTS);

        // memset(pm.rtt, 0,  sizeof(uint32_t) * MAX_CLIENTS);

        // memset(pm.client_tick, 0,  sizeof(uint8_t) * MAX_CLIENTS);
        // memset(pm.tick_offset, 0,  sizeof(uint8_t) * MAX_CLIENTS);


        // memset(pm.local_sequence, 0,  sizeof(uint16_t) * MAX_CLIENTS);
        // memset(pm.remote_sequence, 0,  sizeof(uint16_t) * MAX_CLIENTS);

        // memset(pm.sequences,    0,      sizeof(uint16_t) * MAX_CLIENTS);
        // memset(pm.ack,          0,      sizeof(uint16_t) * MAX_CLIENTS);

        // memset(pm.ack_bitfield, 0,  sizeof(uint64_t) * MAX_CLIENTS);

        // pm.local_tick = 0;
        
        // pm.resent_count = 0;
        // pm.lost_count = 0;




    }





