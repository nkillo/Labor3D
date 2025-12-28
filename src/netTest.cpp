#include "netTest.h"
#include "packet.h"



    void print_id_sequences(network_state* NetworkState, ConnectionState* cs, int id, LogTypes logtype){
        // return;
        if(logtype < NetworkState->logger.lowest_printed_type) return;

        int id_tick = id & (ID_BUFFER_SIZE - 1);
        char id_buffer[64];
        int buffer_offset = sprintf(id_buffer, "id: %d: sequences:", id);
        for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){
            buffer_offset += sprintf(id_buffer + buffer_offset, " %d,", cs->id_to_sequence[id_tick][i]);
        }
        sprintf(id_buffer + buffer_offset, "\n");
        log_inline(NetworkState, logtype, id_buffer);

    }

    
    void validate_packed_sequences(network_state* NetworkState, ConnectionState* cs){
        const char* connection = CLIENT_STR;
        if(cs->is_server)connection = SERVER_STR;

        for(int i = 0; i < cs->in_flight_packet_count; i++){

            int packed_sequence = cs->packed_sequences[i];
            if(packed_sequence == -1){
                LOG_ERROR(NetworkState, "%s packed sequence at index: %d is -1??", connection, i);
            }

            int tick = packed_sequence & (SNAPSHOT_BUFFER_SIZE - 1);


            uint16_t sequence = cs->in_flight_sequences[tick];
            uint16_t id = cs->in_flight_IDs[tick];

            if(cs->in_flight_sequences[tick] == -1){
                LOG_ERROR(NetworkState, "%s in flight sequence at tick: %d is %d, should be: %d", connection, tick, cs->in_flight_sequences[tick], packed_sequence);
                print_arrays(NetworkState, cs, LogTypes::Error);
                assert(0 && "in flight sequence is -1");
            }
            if(cs->in_flight_IDs[tick] == -1){      
                LOG_ERROR(NetworkState, "%s in flight id at tick: %d is %d, should be: %d", connection, tick, cs->in_flight_IDs[tick], id);
                print_arrays(NetworkState, cs, LogTypes::Error);

                assert(0 && "in flight ID is -1");
            }

            if(cs->packed_sequences[i] == -1){
                LOG_ERROR(NetworkState, "%spacked sequences ERROR: -1 at slot %d%s",BRED, i, RESET);
                assert(0 && "PACKED SEQUENCES ERROR");
                return;
            }
        }
        // printf("NOTHING WRONG??? count: %d\n", cs->in_flight_packet_count);
    }
    
    void clear_sequence(network_state* NetworkState, ConnectionState* cs, int tick, bool timeout){
        const char* connection = SERVER_STR;
        if(!cs->is_server)connection = CLIENT_STR;

        LOG_DEBUG(NetworkState, "%s CLEARING %d : %d , tick: %d, timeout: %d",connection, cs->in_flight_sequences[tick], cs->in_flight_IDs[tick], tick, timeout );
        // LOGLINE_TRACE(NetworkState, "sequence [%d]: %d set to -1\n", tick, cs->in_flight_sequences[tick]);


        print_arrays(NetworkState, cs, LogTypes::Debug);

        
        int id = cs->in_flight_IDs[tick];
        int id_tick = id & (ID_BUFFER_SIZE - 1);

        bool resent = false;
        //TODO:
        //once it works we need adjust this so if its been acked we compare against the same sequence 
        if(cs->id_send_attempts[id_tick] > 1)resent = true;
        
        print_id_sequences(NetworkState, cs, id, LogTypes::Debug);


        for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){
            if(cs->id_to_sequence[id_tick][i] != -1){
                uint16_t previous_sequence = cs->id_to_sequence[id_tick][i];
                uint16_t previous_tick = previous_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
                uint16_t prev_in_flight = cs->in_flight_sequences[previous_tick];
                if(prev_in_flight == previous_sequence && (prev_in_flight != cs->in_flight_sequences[tick])){
                    LOG_DEBUG(NetworkState, "FOUND PREVIOUS IN FLIGHT SEQUENCE: %d at slot: %d in in_flight_sequences, acked tick: %d CLEARING! id_tick: %d\n", previous_sequence, previous_tick, tick, id_tick);
                    cs->in_flight_sequences[previous_tick] = -1;
                    cs->in_flight_IDs[previous_tick] = -1;

                    //clear the previous slots in the array that were occupied
                    uint16_t previous_index = cs->tick_to_packed[previous_tick]; 
                    cs->tick_to_packed[previous_tick] = -1;
                    // cs->packed_to_tick[previous_index] = 0; //this is the shared slot in the packed array that points to the current sequence, which sets it to 0
                    //and we dont want that

                }

            }

        }
        for (int i = 0; i < MAX_SEND_ATTEMPTS; i++)
        {
            cs->id_to_sequence[id_tick][i] = -1;
        }
        
        cs->id_in_flight_count[id_tick] = 0;
        cs->id_send_attempts[id_tick] = 0;



      

        int index = cs->tick_to_packed[tick];
        int last_index = cs->in_flight_packet_count - 1 & (SNAPSHOT_BUFFER_SIZE - 1);
        int swapped_tick = cs->packed_to_tick[last_index];

        LOG_DEBUG(NetworkState, "%s index: %d, last_index: %d, swapped_tick: %d, tick_to_packed[%d] is %d,",connection, index, last_index, swapped_tick, swapped_tick, cs->tick_to_packed[swapped_tick]);
    
        cs->tick_to_packed[tick] = -1;
        cs->tick_to_packed[swapped_tick] = index;

        int swapped_id = cs->in_flight_IDs[swapped_tick];
        int swapped_id_tick = swapped_id & (ID_BUFFER_SIZE - 1);

        if(swapped_id != -1){
            
            print_id_sequences(NetworkState, cs, id, LogTypes::Debug);


            for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){//set all the previous associated slots to the new index
                if(cs->id_to_sequence[swapped_id_tick][i] != -1){
                    uint16_t previous_sequence = cs->id_to_sequence[swapped_id_tick][i];
                    uint16_t previous_tick = previous_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
                    uint16_t previous_index = cs->tick_to_packed[previous_tick];


                    if(cs->in_flight_sequences[previous_tick] == previous_sequence){ //sometimes newer sequences can overwrite older ones, in which case we need to check so we don't overwrite the newer sequences
                        LOG_DEBUG(NetworkState, "%s swapping old tick_to_packed[%d]:%d to %d, swapped_tick: %d, swapped_id: %d, previous_sequence: %d, previous_index: %d",connection, previous_tick, cs->tick_to_packed[previous_tick], index,  swapped_tick, swapped_id, previous_sequence, previous_index);
                        cs->tick_to_packed[previous_tick] = index;
                    }
                    else{//don't want to overwrite this slot, since it contains NEW information that has overwritten this outdated piece of information
                        LOG_DEBUG(NetworkState, "%s, tick: %d contains sequence:id: %d:%d which must have overwritten the previously stored sequence:id of %d:%d", connection, previous_tick, cs->in_flight_sequences[previous_tick], cs->in_flight_IDs[previous_tick], previous_sequence, cs->in_flight_IDs[tick]);
                    }
                }
                else{
                    LOG_DEBUG(NetworkState, "%s id_to_sequence[%d][%d] = %d, , swapped_tick: %d, swapped_id: %d, swapped_id_tick: %d", connection, swapped_id_tick, i, cs->id_to_sequence[swapped_id_tick][i], swapped_tick, swapped_id, swapped_id_tick );
                }
            }
        }


        cs->in_flight_sequences[tick] = -1;
        cs->in_flight_IDs[tick] = -1;

        cs->packed_to_tick[index] = swapped_tick;
    
    

        // uint32_t now = GetTimeMS();
        uint32_t now = GetTimeMS()/*REPLACE*/;

        //determine round trip time here    
        if(!resent && !timeout){
            cs->RTT = now - cs->send_times[tick];
            if(cs->RTT > 1000){
                LOG_WARN(NetworkState, "%s probably a timing error here, add explicit check, RTT = %d",connection, cs->RTT);
            }
            // LOG_WARN(NetworkState, "%s SRTT: %u, RTT: %u, tick: %d, now: %u, send time: %u", connection, cs->SRTT, cs->RTT, tick, now , cs->send_times[tick]);
            cs->SRTT = (1 - .125f) * cs->SRTT + (.125f * cs->RTT); //alpha of 1/8

        }


        LOG_DEBUG(NetworkState, "%s setting packed_sequences[%d]:%d to cs->packed_sequences[last_index]: %d, last index: %d",connection, index, cs->packed_sequences[index], cs->packed_sequences[last_index], last_index);

        cs->packed_sequences[index] = cs->packed_sequences[last_index];
        
        cs->packed_to_tick[last_index] = -1;
        cs->packed_sequences[last_index] = -1;

        
        cs->in_flight_packet_count--;
        // printf("%sDECREMENTING IN FLIGHT PACKET COUNT TO: %d%s\n", BRED, cs->in_flight_packet_count, RESET);

        print_arrays(NetworkState, cs, LogTypes::Debug);
        validate_packed_sequences(NetworkState, cs);

    
    }

    void print_arrays(network_state* NetworkState, ConnectionState* cs, LogTypes logtype){
        // return;
        if(logtype < NetworkState->logger.lowest_printed_type) return;

        
        // labour::performance_timer::Timer printTimer;
        // printTimer.reset();


        char sequence_buffer[512];
        char id_buffer[512];
        char tick_to_packed_buffer[512];
        char packed_to_tick_buffer[512];
        char packed_buffer[512];

        int sequence_offset =               sprintf(sequence_buffer,        "in_flight_sequences: ");
        int id_offset =                     sprintf(id_buffer,              "in_flight_IDs:       ");
        int tick_to_packed_offset =         sprintf(tick_to_packed_buffer,  "tick to packed:      ");
        int packed_to_tick_offset =         sprintf(packed_to_tick_buffer,  "packed to tick:      ");
        int packed_buffer_offset =          sprintf(packed_buffer,          "packed sequences:    ");
 

        //single loop for faster processing
        for(int l = 0; l < SNAPSHOT_BUFFER_SIZE; l++) {
            sequence_offset         += sprintf(sequence_buffer          + sequence_offset,          "%4d ", cs->in_flight_sequences[l]);
            id_offset               += sprintf(id_buffer                + id_offset,                "%4d ", cs->in_flight_IDs[l]);
            tick_to_packed_offset   += sprintf(tick_to_packed_buffer    + tick_to_packed_offset,    "%4d ", cs->tick_to_packed[l]);
            packed_to_tick_offset   += sprintf(packed_to_tick_buffer    + packed_to_tick_offset,    "%4d ", cs->packed_to_tick[l]);
            packed_buffer_offset    += sprintf(packed_buffer            + packed_buffer_offset,     "%4d ", cs->packed_sequences[l]);
        }
        sprintf(sequence_buffer + sequence_offset, "\n");
        sprintf(id_buffer + id_offset, "\n");
        sprintf(tick_to_packed_buffer + tick_to_packed_offset, "\n");
        sprintf(packed_to_tick_buffer + packed_to_tick_offset, "\n");
        sprintf(packed_buffer + packed_buffer_offset, "\n");

        // log_inline(NetworkState, logtype, sequence_buffer);
        // log_inline(NetworkState, logtype, id_buffer);
        // log_inline(NetworkState, logtype, tick_to_packed_buffer);
        // log_inline(NetworkState, logtype, packed_to_tick_buffer);
        // log_inline(NetworkState, logtype, packed_buffer);

        char combined_buffer[3072]; //512 * 6 = 3072
        // sprintf(combined_buffer, "TICKS              :   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63\n");
        sprintf(combined_buffer, "TICKS              :    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48   49   50   51   52   53   54   55   56   57   58   59   60   61   62   63\n");
        strcat(combined_buffer, sequence_buffer);
        strcat(combined_buffer, id_buffer);
        strcat(combined_buffer, tick_to_packed_buffer);
        strcat(combined_buffer, packed_to_tick_buffer);
        strcat(combined_buffer, packed_buffer);
        
        log_inline(NetworkState, logtype, combined_buffer);

       
        // log_inline(NetworkState, logtype,  "TICKS              : ");
        // for(int l = 0; l < SNAPSHOT_BUFFER_SIZE; l++){
        //     log_inline(NetworkState, logtype, "%4d ", l);
        // }
        // log_inline(NetworkState, logtype, "\n");

        // log_inline(NetworkState, logtype, "in_flight_sequences: ");
        // for(int l = 0; l < SNAPSHOT_BUFFER_SIZE; l++){
        //     log_inline(NetworkState, logtype, "%3d ", cs->in_flight_sequences[l]);
        // }
        // log_inline(NetworkState, logtype, "\n");

        // log_inline(NetworkState, logtype, "in_flight_IDs:       ");
        // for(int l = 0; l < SNAPSHOT_BUFFER_SIZE; l++){
        //     log_inline(NetworkState, logtype, "%3d ", cs->in_flight_IDs[l]);
        // }
        // log_inline(NetworkState, logtype, "\n");

        // log_inline(NetworkState, logtype, "tick to packed:      ");
        // for(int m = 0; m < SNAPSHOT_BUFFER_SIZE; m++){
        //     log_inline(NetworkState, logtype, "%3d ", cs->tick_to_packed[m]);
        // }
        // log_inline(NetworkState, logtype, "\n");

        // log_inline(NetworkState, logtype, "packed to tick:      ");

        // for(int m = 0; m < SNAPSHOT_BUFFER_SIZE; m++){
        //     log_inline(NetworkState, logtype, "%3d ", cs->packed_to_tick[m]);
        // }
        // log_inline(NetworkState, logtype, "\n");

        // log_inline(NetworkState, logtype, "packed sequences:    ");
        // for(int m = 0; m < SNAPSHOT_BUFFER_SIZE; m++){
        //     log_inline(NetworkState, logtype, "%3d ", cs->packed_sequences[m]);
        // }
        // log_inline(NetworkState, logtype, "\n");

        
        // printf("print took: %f seconds\n",printTimer.elapsed());
        // printTimer.stop();
    }


    void print_receive_chunk_bitfield(network_state* NetworkState, ConnectionState& cs){
        char char_bitfield[65] = {0}; // Enough for 64 bits + null terminator
        for (size_t j = 0; j < 4; j++)
        {
            for (int i = 63; i >= 0; i--) {
                char_bitfield[63 - i] = ((cs.receive_chunk_bitfield[j] >> i) & 1) + '0'; // Convert bit to char
            }
            log_to_console(NetworkState, LogTypes::Debug, "cs.receive_chunk_bitfield[%d]: %s", j, char_bitfield);
        }
    }


    void print_send_chunk_bitfield(network_state* NetworkState, ConnectionState& cs){
        char char_bitfield[65] = {0}; // Enough for 64 bits + null terminator
        for (size_t j = 0; j < 4; j++)
        {
            for (int i = 63; i >= 0; i--) {
                char_bitfield[63 - i] = ((cs.send_chunk_bitfield[j] >> i) & 1) + '0'; // Convert bit to char
            }
            log_to_console(NetworkState, LogTypes::Debug, "cs.send_chunk_bitfield[%d]: %s", j, char_bitfield);
        }
    }


    void sim_test(network_state* NetworkState){
        //this will be called each loop from the main thread
        
        uint8_t clientID = 0;
        setup_sim_client(NetworkState, clientID);
        //hopefully that will setup a simple client so we can try to send a message to the server with it. lets see if it works
        sim_send_connected_handshake_request(NetworkState, clientID);
        //handled in update
        // sim_send_to_server(NetworkState, clientID);



    }

    uint32_t simulate_jitter(uint32_t base_time_ms) {
        int32_t jitter = (rand() % 21) - 10;  // -10 to +10
        return base_time_ms + jitter;
    }
//  int send_all_queued_data(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, bool is_server){

//         // uint8_t& read           = cs.send_queue_read_index;
//         uint8_t& size           = cs.send_queue_count;
//         int i = 0;
//         //TODO: BANDWIDTH MANAGEMENT 
//         //send a message, cycle to next client, send message, repeat until all messages are sent or bandwidth budget is exceeded
//         while(size > 0 ){

//             char* buffer       = cs.send_queue[i];
//             uint16_t& buffer_size   = cs.send_queue_size[i]; //actual chars taken up in the current slot

//             int result = -1;
//             result = sim_send_binary_udp(NetworkState, socket, addr, addr_len, total_sent, clientID, token, cs, buffer, buffer_size, is_server);

            
//             if(result == 0){
//                 //otherwise we will subtract and get an overflow to 255 and assert because the while loop will keep going
//                 if(size > 0)size--;
//             }
//             else{
//                 LOG_DEBUG(NetworkState, "sim_send_binary_udp() failed? error code: %d", result);
//                 assert(0 && "ERROR SENDING MESSAGE");
//                 return -1;
//             }

//             i++;
//         }
//         return 0;
//     }

    //sloppified this function, this puts the network commands (stuff like connect/send chunk) at the front for the endpoint to read first
    //if we don't care about order, if we end up wanting to deserialize in that loop, remove all refernces to network commands here
    //the original function is right above us in case we want to use that instead
    //putting the network commands at the front is maybe a minor optimization so we don't need to read all the net commands if we dont need to in the receive function. 
    //premature optimization :(
    //also if we want to revert to the original, replace all instances of sim_network_command_append_to_packet() with sim_append_network_command()
    int send_all_queued_data(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, bool is_server){

        // uint8_t& read           = cs.send_queue_read_index;
        uint8_t& size           = cs.send_queue_count;
        uint16_t& net_cmd_size   = cs.network_command_size;
        int i = 0;
        //TODO: BANDWIDTH MANAGEMENT 
        //send a message, cycle to next client, send message, repeat until all messages are sent or bandwidth budget is exceeded
        while(size > 0 || net_cmd_size > 0){

            char* buffer       = cs.send_queue[i];
            uint16_t& buffer_size   = cs.send_queue_size[i]; //actual chars taken up in the current slot

            int result = -1;

            if(cs.network_command_size > 0){//slop to check if there are any network commands we want to stick at the top of the message
                if(buffer_size + cs.network_command_size <= MAX_PACKET_SIZE){
                    memset(cs.scratch_buffer, 0, MAX_PACKET_SIZE);
                    memcpy(cs.scratch_buffer, cs.network_command, cs.network_command_size);
                    memcpy(cs.scratch_buffer + cs.network_command_size, buffer, buffer_size);
                    uint16_t total_size = buffer_size + cs.network_command_size;
                    // LOG_INFO(NetworkState, "sending message with net command!");
                    result = sim_send_binary_udp(NetworkState, socket, addr, addr_len, total_sent, clientID, token, cs, cs.scratch_buffer, total_size, is_server);
                    //need to clear the stuff after we send, the function would do if we didn't send it the non send queue info
                    memset(cs.network_command, 0, MAX_PACKET_SIZE);
                    memset(buffer, 0, buffer_size);
                    buffer_size = 0;
                    cs.network_command_size = 0;
     

                }else{
                    LOG_ERROR(NetworkState, "CANT FIT NET COMMANDS INTO THE BUFFER!");
                    LOG_ERROR(NetworkState, "CANT FIT NET COMMANDS INTO THE BUFFER!");
                    LOG_ERROR(NetworkState, "CANT FIT NET COMMANDS INTO THE BUFFER!");
                    assert(0 && "COULDNT FIT THE NET COMMANDS IN WITH THE GAME COMMANDS BUFFER TOO FULL ERROR");//not really an error but this shouldnt happen
                }

            }
            else{
                // LOG_INFO(NetworkState, "sending message withOUT net command!");
                result = sim_send_binary_udp(NetworkState, socket, addr, addr_len, total_sent, clientID, token, cs, buffer, buffer_size, is_server);

            }
            
            if(result == 0){
                //otherwise we will subtract and get an overflow to 255 and assert because the while loop will keep going
                if(size > 0)size--;
            }
            else{
                LOG_DEBUG(NetworkState, "sim_send_binary_udp() failed? error code: %d", result);
                assert(0 && "ERROR SENDING MESSAGE");
                return -1;
            }

            i++;
        }
        return 0;
    }

    int sim_network_command_append_to_packet(network_state* NetworkState, char* append_slot, uint16_t& append_buffer_size, char* buffer, uint16_t size){
        // if(!NetworkState->enabled)return -1; //dont need to worry if its enabled during simulation
        if(append_buffer_size + size < (uint16_t)MAX_PACKET_SIZE){
            memcpy(append_slot, buffer, size);
            append_buffer_size += size;
            return 0;
        }
        LOG_ERROR(NetworkState, "sim_network_command_append_to_packet() STRING TOO LONG, CANT APPEND");
        return -1;
    }

    int sim_append_network_command(network_state* NetworkState, ConnectionState& cs, char* buffer, uint16_t packetsize){
            char* append_slot       = cs.network_command;

            uint16_t& buffer_size   = cs.network_command_size; //actual chars taken up in the current slot
            
            int result = sim_network_command_append_to_packet(NetworkState, append_slot + buffer_size, buffer_size, buffer, packetsize);
            if(result == 0){
                return 0;
            }

            assert(0 && "sim_append_network_command() failed?");
            return -1;
    }

    int sim_append_to_packet(network_state* NetworkState, ConnectionState& cs, char* buffer, uint16_t packetsize){
            uint8_t& size           = cs.send_queue_count;
            //write == read on start.. what do we do?
            // if(size < SEND_QUEUE_SIZE){
            //only append if the client is connected, will probably break later but good enough for now
            char* append_slot       = cs.send_queue[size];

            uint16_t& buffer_size   = cs.send_queue_size[size]; //actual chars taken up in the current slot

            
            int result = sim_network_thread_append_to_packet(NetworkState, append_slot + buffer_size, buffer_size, buffer, packetsize);
            if(result == 0){
                //successfully appended to queue
                if(size == 0)size++; //only increment size if its 0, otherwise we can stay in the current slot
                return 0;
            }else if(result == 1){
                size++;
                return 0;
            }else{
                return -1;
            }
            return -1;
    }


    void sim_test_update(network_state* NetworkState){
        NetTest& nt = NetworkState->netTest;
        
        char buffer[16] = {0};

        for(uint8_t clientID = 0; clientID < nt.num_clients; clientID++){
            
            // uint32_t now = GetTimeMS();
            uint32_t now = GetTimeMS()/*REPLACE*/;

            ConnectionState& cs = nt.client_connection_state[clientID];
            
            BandwidthBudget(NetworkState, cs.local_time_ms, cs.bandwidth_accumulator);

            
            log_connection_state(NetworkState, cs, clientID);


            //poll for messages
            while(cs.socket_message_count > 0 && receive_packet(NetworkState, true, clientID) == 0){

            }
                        


            packet_entry entry = {};
            entry.type = MessageTypes::SIMULATION;
            
            bool send_net_command = false;
            //check if we are ready to start sending a chunk 
            // if(cs.waiting_to_send_chunk){
            //     //check if enough time has passed to resend the send request
            //     if(now - cs.chunk_send_request_timeout > NetworkState->netTest.tick_timestep_ms * SNAPSHOT_BUFFER_SIZE){
            //         LOG_DEBUG(NetworkState, "%s resend chunk send request, last send was %d ms ago", CLIENT_STR, now - now - cs.chunk_send_request_timeout);
            //         //resend the chunk send request
            //         entry.type = MessageTypes::CHUNK_SEND_REQUEST;
            //         cs.chunk_send_request_timeout = now;
            //         send_net_command = true;
            //     }
            // }


            // if(!cs.is_server && cs.local_sequence == 2){ //basic chunk send 
            //     //just for the simulation, realistically the game layer will send us the chunk info
            //     cs.num_slices = 256;
            //     cs.send_chunk_size = MAX_UDP_SIZE * cs.num_slices;
                
            //     int entrysize = pack((unsigned char*)cs.send_chunk, "CH", MessageTypes::CHUNK_INFO, MAX_UDP_SIZE);

            //     LOG_DEBUG(NetworkState, "%s INITIATING CHUNK SEND REQUEST", CLIENT_STR);
            //     cs.waiting_to_send_chunk = true;
            //     cs.ready_to_receive_chunk = false;
            //     entry.type = MessageTypes::CHUNK_SEND_REQUEST;
            //     cs.chunk_send_request_timeout = now;
            //     send_net_command = true;
            // }

            // //x5.5 to give the receive enough time to spam out all the receive messages and respond without us needing to slam them all out again
            // if(cs.sending_chunk && (GetTimeMS() - cs.oldest_slice_send_time > cs.latency * 5.5)){
            //     if(!cs.any_slices_sent){
            //         cs.chunk_send_timeout = GetTimeMS();
            //     }
            //     if(GetTimeMS() - cs.chunk_send_timeout > (cs.SRTT * SNAPSHOT_BUFFER_SIZE)){
            //         LOG_ERROR(NetworkState, "%s chunk send time exceeded max allowed time limit! lost slices: %d, acked slices: %d", CLIENT_STR, cs.lost_slices, cs.acked_slices);
            //         LOG_ERROR(NetworkState, "%s chunk send time exceeded max allowed time limit! lost slices: %d, acked slices: %d", CLIENT_STR, cs.lost_slices, cs.acked_slices);
            //         LOG_ERROR(NetworkState, "%s chunk send time exceeded max allowed time limit! lost slices: %d, acked slices: %d", CLIENT_STR, cs.lost_slices, cs.acked_slices);
            //         assert(0 && "chunk send time exceeded max allowed time limit!");
            //     }
            //     bool is_server = false;
            //     int total_sent = 0;
            //     //all chunk send info will go here
            //     int num_slices = ((cs.send_chunk_size) / MAX_UDP_SIZE); 
            //     //determine which slices need to be sent based on acks
            //     sockaddr* addr = (sockaddr*)&cs.socket_udp_addr;
            //     socklen_t addr_len = sizeof(sockaddr_in);
            //     //if we are sending a chunk, pass over the function to see if something needs to be resent
            //     sim_send_binary_chunk(NetworkState, cs.socket, addr, addr_len, 
            //                 &total_sent, clientID, cs.token, cs, cs.send_chunk, 
            //                 cs.send_chunk_size, is_server);
            
                                

            //     assert(num_slices == cs.num_slices && "calculated num slices is not equal to the connection state number of slices, programmer error");

            // }





            entry.size = 3;
            int packetsize = pack((unsigned char*) buffer, "CH", entry.type, entry.size);
            // packetsize += 1025; //if we want to sloppily test packet fragments

            if(!cs.is_server && cs.connected){
            
                if(send_net_command)sim_append_network_command(NetworkState, cs, buffer, packetsize);
                
                else sim_append_to_packet(NetworkState, cs, buffer, packetsize);
                // sim_append_to_packet(NetworkState, cs, buffer, packetsize);

                //only append one slice update entry per packet, otherwise we will fill up the entire packet with chunk slice acks
                
                
                //test to see if we can distinguish between end of net commands and beginning of game logic
                // char end_buffer[16];
                // int entrysize = pack((unsigned char*)end_buffer, "CH", MessageTypes::END_NET_CMDS, 3);
                // // int result = sim_append_to_packet(NetworkState, *cs, buffer, entrysize);
                // int result = sim_append_network_command(NetworkState, cs, end_buffer, entrysize);
            //SLOP
            }else{sim_send_connected_handshake_request(NetworkState, clientID);}//keep sending connection requests until we get through



            int total_sent = 0;
            int socket = cs.socket;//dont need to worry about this for the simulation
            // sockaddr* addr = (sockaddr*)&nt.client_udp_addrs[clientID];
            sockaddr* addr = (sockaddr*)&cs.socket_udp_addr;
            socklen_t addr_len = sizeof(sockaddr_in);
            uint16_t token = cs.token;
            bool is_server = false;
            if(cs.token != cs.originalToken){
                LOG_ERROR(NetworkState, "clientID %d token set to: %d, original: %d", clientID, token, cs.originalToken);
            }
            
            //then check all in flight packets
            check_in_flight_packets(NetworkState, cs, clientID);




                                    
            send_all_queued_data(NetworkState, socket, addr, addr_len, &total_sent, clientID, token, cs, is_server);


        }
    }



    void sim_connect_to_server(network_state* NetworkState, uint8_t clientID){
        // uint32_t now = GetTimeMS();
        uint32_t now = GetTimeMS()/*REPLACE*/;

        // if (currentTime - NetworkState->clientSockets.connectStart > 5000) { // 5 seconds in ms
        //     // Connection timed out
        //     printf("CONNECTION ATTEMPT TIMED OUT\n");
        //     printf("CONNECTION ATTEMPT TIMED OUT\n");
        //     printf("CONNECTION ATTEMPT TIMED OUT\n");
        //     printf("CONNECTION ATTEMPT TIMED OUT\n");
        //     NetworkState->clientSockets.isConnected = false;
        //     NetworkState->clientSockets.connecting = false;
        //     return;
        // }



        LOG_DEBUG(NetworkState, "thread connect to server...");

        // Attempt to connect
        sim_send_connected_handshake_request(NetworkState, clientID);

    }

    void sim_send_connected_handshake_request(network_state* NetworkState, uint8_t clientID){
        NetTest& nt = NetworkState->netTest;
        ConnectionState& cs = nt.client_connection_state[clientID];

        int total_bytes_sent = 0;
        char buffer[16];
        packet_header header = {};
        packet_entry entry = {};
        entry.type = MessageTypes::MSG_CONNECT_REQUEST;
        entry.size = 5;
        int packetsize = pack((unsigned char*) buffer, "CHH", entry.type, entry.size, cs.socket_udp_addr.sin_port);
        
        PacketManager& pm = NetworkState->packetManager;
        int total_sent = 0;

        header.size = entry.size;
        

        sim_append_network_command(NetworkState, cs, buffer, packetsize);
        // sim_append_to_packet(NetworkState, cs, buffer, packetsize);
        // packet_manager_send_binary_udp(NetworkState, NetworkState->clientSockets.udp_socket, (unsigned char*)buffer, (sockaddr*)&NetworkState->clientSockets.server_addr_udp, sizeof(sockaddr_in), &total_sent, header, clientID);
    }
    int sim_network_thread_append_to_packet(network_state* NetworkState, char* append_slot, uint16_t& append_buffer_size, char* buffer, uint16_t size){
        // if(!NetworkState->enabled)return -1; //dont need to worry if its enabled during simulation
        if(append_buffer_size + size  /*+ ( cs.net_send_queue_size[cs.send_queue_write_index] )*/ <= (uint16_t)MAX_PACKET_SIZE){ //need to check that the current bytes in the net command also dont exceed
            memcpy(append_slot, buffer, size);
            append_buffer_size += size;
            if(append_buffer_size + size == (uint16_t)MAX_PACKET_SIZE){
                return 1; //need to tell the above layer to move to the next packet to append to
            }
            return 0;
        }else{
            // LOG_ERROR(NetworkState, "STRING TOO LONG, CANT APPEND, net command bytes: %d, game command bytes: %d",/* cs.net_send_queue_size[cs.send_queue_write_index], */ size);
            assert(0 && "NETWORK SEND BUFFER TOO LARGE, CANT APPEND MORE INFO");
            return -1;
        }
        return -1;
    }


    // static int num_times = 0;    
    int sim_send_binary_udp(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, 
                            int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, char* buffer, 
                            uint16_t& buffer_size, bool is_server, packet_header* header, bool resending, int resent_tick){

    // int packet_manager_send_binary_udp(network_state* NetworkState, int socket, packet_header header, char* payload, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token){
        // assert(cs.local_sequence < 64 && "end of full second, read the log and see if it looks right");
        PacketManager& pm = NetworkState->packetManager;
        // uint32_t now = GetTimeMS();
        uint32_t now = GetTimeMS()/*REPLACE*/;

        //to help myself log who's sending what
        // const char* connection = BBLUE "[SERVER]" RESET;
        const char* connection = SERVER_STR;
        ConnectionState* receivingCS = nullptr;
        if(is_server){
            receivingCS = &NetworkState->netTest.client_connection_state[clientID]; //server is sending to client
        }
        else{
            receivingCS = &NetworkState->connectionState[clientID];//client is sending to server
            // const char* connection = BGREEN "[CLIENT]" RESET;
            connection = CLIENT_STR;
        }

        unsigned char* payload = (unsigned char*)buffer;

        int num_fragments = ((buffer_size + MAX_UDP_SIZE) / MAX_UDP_SIZE);
        int curr_fragment = 0;




        if(is_server){
            LOG_TRACE(NetworkState, "%s server sim_send_binary_udp(), remote_sequence: %d", SERVER_STR, cs.remote_sequence);
        }
        if(!header){
            packet_header temp_header = {};
            header = &temp_header;
            header->size = buffer_size;

            header->token = token;
            header->id = cs.local_sequence;
            // header->sequence = pm.local_sequence[clientID];
            //end packet manager shit
        }

        int old_sequence = -1;
        if(resending)old_sequence = header->sequence;

        header->sequence = cs.local_sequence;

        header->ack = cs.remote_sequence;
        header->ack_bitfield = cs.ack_bitfield;
        // header->time_sent = GetTimeMS();
        header->total_fragments = num_fragments - 1;//because +1 to it on receive, since we only have 255 vals and we want to be able to send 256 slices for chunks
        header->current_fragment = curr_fragment;

        if(header->size <= 0){
            //nothing to send, return
            return -1;
        }

        //this becomes the send queue logic


        //tick is based off of the local sequence
        uint16_t tick = (header->sequence & (SNAPSHOT_BUFFER_SIZE - 1)); //send one sequence per tick, sequence 0 will be tick 0
        int id_tick = header->id & (ID_BUFFER_SIZE - 1);
        

      
        LOG_DEBUG(NetworkState, "%s %sSENDING%s %d : %d, TICK: %d, id_tick: %d, in_flight_count: %d, now: %u", connection, MAGENTA, RESET, header->sequence, header->id, tick, id_tick, cs.id_in_flight_count[id_tick], now);

        // char char_bitfield[65] = {0}; // Enough for 64 bits + null terminator
        // for (int i = 63; i >= 0; i--) {
        //     char_bitfield[63 - i] = ((header->ack_bitfield >> i) & 1) + '0'; // Convert bit to char
        // }
        // num_times++;
        // printf("are we being called twice?? num times called: %d\n", num_times);
        // LOG_DEBUG(NetworkState, "%s SENDING sequence: %d, ID: %d, REMOTE SEQUENCE: %d"/* , bitfield: %s */,connection, header->sequence, header->id, cs.remote_sequence /*,  char_bitfield */);
        if(cs.latest_in_flight_sequence == cs.local_sequence){
            LOG_ERROR(NetworkState, "%s Resending the same sequence???", connection);
            assert(0 && "resending the same sequence?");
        }
        // print_arrays(NetworkState, &cs);

        if(buffer_size == 0){
            printf("%sBUFFER SIZE IS 0%s",BRED, RESET);
            assert(buffer_size != 0 && "buffer size is 0???");
        }

        uint16_t remaining_size = buffer_size;

        int send_count = 0;
        for(uint8_t curr_frag = 0; curr_frag <= header->total_fragments; curr_frag++){
            //setup the header info, paste in the binary data, and send
            assert(remaining_size != 0 && "send packet, remaining size is 0?? ERROR");

            if(remaining_size > MAX_UDP_SIZE){
                header->size = MAX_UDP_SIZE;
            }else{
                header->size = remaining_size;
            }


            char send_buffer[SINGLE_FRAGMENT_SIZE];

            // uint8_t testType  = 0;
            // uint16_t testSize = 0;
            // unpack((unsigned char*) buffer, "CH", &testType, &testSize);
            // if(testType == 10){
            //     printf("send debug! type is client connect request! type: %d, size: %d\n", testType, testSize);
            //     printf("send debug! type is client connect request! type: %d, size: %d\n", testType, testSize);
            //     if(resending){
            //         LOG_DEBUG(NetworkState, "%s RESENDING SERVER CONNECTION HANDSHAKE, sequence: %d", connection, cs.local_sequence);
            //     }
            //     else if(cs.local_sequence != 0){
            //         LOG_DEBUG(NetworkState, "%s NOT RESENDING SERVER CONNECTION HANDSHAKE, sequence: %d", connection, cs.local_sequence);
            //         assert(0 && "server should not be sending the client connection handshake again!");
            //     }
            // }

            int headerSize = pack((unsigned char*)send_buffer, "HCCHHHHQ", header->token, header->total_fragments, curr_frag, 
                                    header->id, header->sequence, header->ack, header->size, header->ack_bitfield);

            if(headerSize + header->size > SINGLE_FRAGMENT_SIZE){
                LOG_ERROR(NetworkState, "message too big for UDP");
                return -3;
            }

            //copy payload right after header
            if(header->size > 0){
                memcpy(send_buffer + headerSize, payload + (curr_frag * MAX_UDP_SIZE), header->size);
            }

            int total_size = headerSize + header->size;
            if(NetworkState->netTest.enabled){
                int random_val = rand() % 100;
                // Drop packet if random number is less than the loss percentage
                bool dropped = random_val < NetworkState->netTest.packet_loss_percentage;

                if(!cs.first_message_sent){dropped = true;} //always drop the first packet just as a test

                if(!dropped){
                    sim_sendto(NetworkState, send_buffer, clientID, total_size, addr, is_server, receivingCS, header);
                    send_count++;
                }else{
                    cs.dropped_packets++;
                    LOG_DEBUG(NetworkState, "%s %sPACKET SEQUENCE: %d FRAGMENT: %d, DROPPED!%s ", connection, RED, header->sequence, curr_frag, RESET);
                }
            }
            else{
                if(sendto(socket, send_buffer, total_size, 0, addr, addr_len) != total_size){
                    LOG_ERROR(NetworkState, "UDP message sendto() FAILED");
                    return -1;
                }
                send_count++;
            }
            *total_sent += total_size;

            remaining_size -= header->size;
            
            cs.local_header_history     [tick][curr_frag] = *header;
            
            cs.sent_fragments++;
        }

        if(AdjustBandwidthBudget(NetworkState, cs.bandwidth_accumulator, *total_sent) < 0){
            uint32_t bits = *total_sent * 8;
            LOG_ERROR(NetworkState, "%s (SEND) OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, cs.bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (SEND) OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, cs.bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (SEND) OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, cs.bandwidth_accumulator, bits);
        }
        if(cs.is_server && AdjustBandwidthBudget(NetworkState, NetworkState->total_bandwidth_accumulator, *total_sent) < 0){
            uint32_t bits = *total_sent * 8;
            LOG_ERROR(NetworkState, "%s (SEND) SERVER OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, NetworkState->total_bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (SEND) SERVER OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, NetworkState->total_bandwidth_accumulator, bits);
            LOG_ERROR(NetworkState, "%s (SEND) SERVER OVER BANDWIDTH BUDGET!! available bits: %u , sent bits: %u", connection, NetworkState->total_bandwidth_accumulator, bits);
        }

        
        print_arrays(NetworkState, &cs, LogTypes::Debug);

        //need to simulate packet manager per client in netTest
        uint16_t index = cs.in_flight_packet_count & (SNAPSHOT_BUFFER_SIZE - 1);

        //total_sent is the number of bytes in the packet + the header size
        assert((*total_sent - (PACKED_MESSAGE_HEADER_SIZE* num_fragments))  == buffer_size && "didnt send all bytes in the packet buffer?? ERROR");
        

        // if(header->sequence == 6065 && header->id == 6044){
        //     printf("debug case");
        // }

        bool overwritten = false;            
        bool overwritten_id = false;
        if((cs.id_send_attempts[id_tick] >= MAX_SEND_ATTEMPTS) || (header->sequence == header->id && cs.id_send_attempts[id_tick] > 0)){
            //there is a case where we do not exceed the max resend attempts, and thus do not try to clear the previous ID. when we send a new ID, we need to check against
            //the second check makes sure that this slot is clear, if it isnt then we have overwritten another active ID
            
            //another active ID in that slot
            // LOG_ERROR(NetworkState, "%s id: %d, id_tick: %d, id_sent_attempts[%d] = %d", connection, header->id, id_tick, id_tick, cs.id_send_attempts[id_tick]);
            // LOG_ERROR(NetworkState, "%s id: %d, id_tick: %d, id_sent_attempts[%d] = %d", connection, header->id, id_tick, id_tick, cs.id_send_attempts[id_tick]);
            // //this means that this is another edge case where we need to overwrite these slots since we've wrapped around 128 ids
            // log_inline(NetworkState, LogTypes::Warn, "sequences associated with id: %d: ", header->id);
            // for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){
            //     log_inline(NetworkState, LogTypes::Warn, " %d,", cs.id_to_sequence[id_tick][i]);
            // }
            LOG_DEBUG(NetworkState, "%s id: %d, id_tick: %d, id_sent_attempts[%d] = %d, tick: %d", connection, header->id, id_tick, id_tick, cs.id_send_attempts[id_tick], tick);


            print_id_sequences(NetworkState, &cs, header->id, LogTypes::Debug);


            
            //we've wrapped around enough times to overwrite the old id slot, so we need to clear it
            cs.overwritten_id_count++;


            //we aren't overwriting anything, we aren't resending, so we can clear the timed out previous ID info before overwriting it

            if(cs.is_server)cs.need_resync = true;
            if(cs.is_server)cs.times_desynced++;
            
            int count = 0;
            for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){
                int temp_sequence = cs.id_to_sequence[id_tick][i];
                int temp_tick = temp_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
                if(temp_sequence == cs.in_flight_sequences[temp_tick]){
                    count++;
                    clear_sequence(NetworkState, &cs, temp_tick, true);
                    break;
                }
            }
            if(count == 0){//we couldnt cl
                LOG_ERROR(NetworkState, "%s Couldn't find and clear the outdated ID??", connection);
            }
            index = cs.in_flight_packet_count & (SNAPSHOT_BUFFER_SIZE - 1); //append to the end of the packed sequence array
        
            print_arrays(NetworkState, &cs, LogTypes::Debug);


        }



        if(index >= SNAPSHOT_BUFFER_SIZE){
            LOG_ERROR(NetworkState, "%s INDEX SHOULD NOT BE THIS HIGH: %d",connection, index);
        }
        // cs.packet_resend_timeouts   [tick] = GetTimeMS();     //time we sent it
        cs.packet_resend_timeouts   [tick] = GetTimeMS()/*REPLACE*/;     //time we sent it
        // LOG_DEBUG(NetworkState, "%s sent packets: %d", connection, send_count);
        // if(!resending){
        
            if(header->total_fragments > 0){
                cs.packet_is_fragmented[tick] = true;
            }

            //need to use sequence to compare against the tick, the sequence can be the same as the tick, which will give us the index into where its stored
            int sequence_tick = -1;
            int prev_id_tick = -1;
            if(cs.in_flight_sequences[tick] != -1){
                sequence_tick = cs.in_flight_sequences[tick] & (SNAPSHOT_BUFFER_SIZE - 1);
                prev_id_tick = cs.in_flight_IDs[sequence_tick] & (ID_BUFFER_SIZE - 1);
                if(cs.id_in_flight_count[prev_id_tick] > 1 || cs.in_flight_IDs[tick] == header->id){//not overwritten, there are other sequences in flight
                    LOG_DEBUG(NetworkState, "%s IN FLIGHT SEQUENCES IS NOT -1: OVERWRITING OLD %d:%d at TICK/SLOT: %d WITH %d:%d, sequence_tick: %d, prev_id_tick: %d, in_flight_count: %d", connection, cs.in_flight_sequences[tick], cs.in_flight_IDs[tick],tick, header->sequence, header->id, sequence_tick, prev_id_tick, cs.id_in_flight_count[prev_id_tick]);
                    cs.id_in_flight_count[prev_id_tick]--;
                    
                    
                    // print_arrays(NetworkState, &cs, LogTypes::Warn);
                
                }
                else{
                    LOG_DEBUG(NetworkState, "%s OVERWRITING IN FLIGHT SEQUENCE cs.in_flight_sequences[%d] = %d, tick: %d", connection, tick, cs.in_flight_sequences[tick], tick);
                    overwritten = true;
                }
            }
            else if( cs.packed_sequences[index] != -1){
                sequence_tick = cs.packed_sequences[index] & (SNAPSHOT_BUFFER_SIZE - 1);
                prev_id_tick = cs.in_flight_IDs[sequence_tick] & (ID_BUFFER_SIZE - 1);
                if(cs.id_in_flight_count[prev_id_tick] > 1){//not overwritten, there are other sequences in flight
                    LOG_DEBUG(NetworkState, "%s PACKED SEQUENCES IS NOT -1: OVERWRITING OLD %d:%d at TICK/SLOT: %d WITH %d:%d, sequence_tick: %d, prev_id_tick: %d, in_flight_count: %d", connection, cs.in_flight_sequences[tick], cs.in_flight_IDs[tick],tick, header->sequence, header->id, sequence_tick, prev_id_tick, cs.id_in_flight_count[prev_id_tick]);
                    cs.id_in_flight_count[prev_id_tick]--;

                    // print_arrays(NetworkState, &cs, LogTypes::Warn);
                
                }
                else{
                    LOG_DEBUG(NetworkState, "%s OVERWRITING PACKED SEQUENCE cs.packed_sequences[%d] = %d, index: %d", connection, index, cs.packed_sequences[index], index);
                    overwritten = true;
                }
            }
            if(overwritten){
                // if(cs.in_flight_IDs[tick] == header->id){
                //     printf("debug case");
                // }
                cs.overwritten_packets++;
                // cs.need_resync = true;
                if(cs.in_flight_sequences[tick] == -1 || cs.in_flight_IDs[tick] == -1){
                    // printf("");
                    assert(0 && "in flight sequence AND ID is -1!");
                }
                // if(cs.in_flight_sequences[tick] == 65535 || cs.in_flight_IDs[tick] == 65535){
                //     printf("");
                // }
                LOG_DEBUG(NetworkState, "%s sequence: %d id: %d LOST, overwritten count: %d", connection, cs.in_flight_sequences[tick], cs.in_flight_IDs[tick], cs.overwritten_packets);
                LOG_DEBUG(NetworkState, "%s id: %d in flight count: %d, send attempts: %d", connection, cs.in_flight_IDs[tick], cs.id_in_flight_count[prev_id_tick], cs.id_send_attempts[prev_id_tick]);
                LOG_DEBUG(NetworkState, "%s replaced by: sequence: %d id: %d", connection, header->sequence, header->id);

                LOG_DEBUG(NetworkState, "%s OVERWITTEN IN FLIGHT PACKET! PACKET LOST! NEED TO RESYNC!", connection);
                // LOG_ERROR(NetworkState, "%s OVERWITTEN IN FLIGHT PACKET! PACKET LOST! NEED TO RESYNC!", connection);
                if(cs.is_server)cs.need_resync = true;
                if(cs.is_server)cs.times_desynced++;
                // print_arrays(NetworkState, &cs, LogTypes::Warn);

                int id = cs.in_flight_IDs[tick];
                int times_resent = cs.id_send_attempts[prev_id_tick] - 1;

                if(times_resent == -1){
                    // printf("");
                    assert(0 && "TIMES RESENT IS -1?!");
                }

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

                uint32_t exponent = 1 << (times_resent > 0 ? times_resent : 0);

                uint32_t RTO = (cs.SRTT * 1.5f) * (exponent);
                if(RTO == 0){
                    // printf("");
                    assert(0 && "RTO IS 0?!"); //somehow the sequence ??! is a trigraph? stumbled on warnings for it in the linux compiler

                }
           
                // log_inline(NetworkState, LogTypes::Warn, "sequences associated with id: %d: ", id);
                // for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){
                    // log_inline(NetworkState, LogTypes::Warn, " %d,", cs.id_to_sequence[prev_id_tick][i]);
                // }

                print_id_sequences(NetworkState, &cs, id, LogTypes::Debug);


                // int packed_sequence = cs.packed_sequences[index];
                // LOG_INFO(NetworkState, "%s packed sequence %d sequence: %d, sent %d ms ago, RTO: %u, times resent: %d, in flight count: %d", connection, packed_sequence, sequence, diff, RTO, times_resent, cs.in_flight_packet_count);
                
                if(id_tick != prev_id_tick){
    
                    for (int i = 0; i < MAX_SEND_ATTEMPTS; i++)
                    {
                        cs.id_to_sequence[prev_id_tick][i] = -1;
                    }
                    cs.id_in_flight_count[prev_id_tick] = 0;
                    cs.id_send_attempts[prev_id_tick] = 0;

                }
                

                index = cs.tick_to_packed[tick];
                LOG_DEBUG(NetworkState, "index to write overwritten sequence to: %d\n", index);

            }
            //if we are overwriting an ID AND its the same slot that we need to write to
            if(index == 65535){
                LOG_ERROR(NetworkState, "%s INDEX IS 65535", connection);
            }
            cs.packet_num_fragments[tick] = header->total_fragments;
            cs.latest_tick = tick;

            cs.send_times[tick] = now;
            if(!resending){

                
                cs.tick_to_packed[tick]  = index;
                cs.packed_to_tick[index] = tick;
            }

            cs.packet_resend_IDs        [tick] = header->id;            //id of the message we need to resen

            // assert(cs.in_flight_sequences[tick] == -1 && "OVERWRITTEN OLD IN FLIGHT PACKET, PACKET LOST!");

            
            
            cs.in_flight_sequences      [tick] = header->sequence;
            cs.in_flight_IDs      [tick] = header->id;
            cs.latest_in_flight_sequence = header->sequence;
            //we know the latest sequence, the bitfield counts back from it
            //need to check if this is the first message we've sent, if thats the case dont shift the bitfield
            if(cs.first_message_sent && !cs.latest_in_flight_sequence_acked){
                cs.in_flight_bitfield = (cs.in_flight_bitfield << 1) | 1; // Shift left and set the new sequence bit
            }else{
                cs.first_message_sent = true;
            }
            cs.latest_in_flight_sequence_acked = false;

            // for (int i = 63; i >= 0; i--) {
            //     char_bitfield[63 - i] = ((cs.in_flight_bitfield >> i) & 1) + '0'; // Convert bit to char
            // }
            // log_to_console(NetworkState, LogTypes::Debug, "%s sim_send_binary_udp() latest sequence: %d"/* , in_flight_bitfield: %s" */,connection, cs.latest_in_flight_sequence/* , char_bitfield */);

            if(resending && resent_tick == tick){
                
                //we don't want to clear these since it would immediately delete what we sent in because of our resend code slop


            }else{ //any other case is fine
                memset(cs.local_buffer_history[tick], 0, MAX_PACKET_SIZE); //clear slot
                memcpy(cs.local_buffer_history[tick], buffer, buffer_size);
                
                assert(buffer_size != 0 && "buffer size is 0???");
                
                cs.local_history_size[tick] = buffer_size;
                //clear buffer for next packet
                memset(buffer, 0, MAX_PACKET_SIZE); //clear slot
                buffer_size = 0;
                
            }

            // pm.local_sequence[clientID]++;
            cs.local_sequence++;
        // }
        if(resending){

            uint16_t old_tick = old_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
            int old_index = cs.tick_to_packed[old_tick];

            int new_index = old_index;
            LOG_DEBUG(NetworkState, "%s resending, old_index: %d, new_index: %d", connection, old_index, new_index);

            if(overwritten){
                new_index = index; //new index is the slot that we want to overwrite
                LOG_DEBUG(NetworkState, "%s resending and overwriting, old_index: %d, new_index: %d", connection, old_index, new_index);
            }
            //resending packet
            cs.resend_count++;
            

            cs.packed_sequences[new_index] = header->sequence;
            cs.packed_to_tick[new_index] = tick;
            cs.tick_to_packed[tick] = new_index;
            int id = header->id; //id probably cant be -1 if we are directly resending it
            for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){//set all the previous associated slots to the new index
                if(cs.id_to_sequence[id_tick][i] != -1){
                    uint16_t previous_sequence = cs.id_to_sequence[id_tick][i];
                    uint16_t previous_tick = previous_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
                    uint16_t previous_index = cs.tick_to_packed[previous_tick];

                    if(cs.in_flight_sequences[previous_tick] == previous_sequence){ //sometimes newer sequences can overwrite older ones, in which case we need to check so we don't overwrite the newer sequences
                        LOG_DEBUG(NetworkState, "%s swapping old tick_to_packed[%d]:%d to %d, tick: %d, id: %d, previous_sequence: %d, previous_index: %d",connection, previous_tick, cs.tick_to_packed[previous_tick], new_index,  tick, id, previous_sequence, previous_index);
                        cs.tick_to_packed[previous_tick] = new_index;
                    }
                    else{//don't want to overwrite this slot, since it contains NEW information that has overwritten this outdated piece of information
                        LOG_DEBUG(NetworkState, "%s, tick: %d contains sequence:id: %d:%d which must have overwritten the previously stored sequence:id of %d:%d", connection, cs.in_flight_sequences[previous_tick], cs.in_flight_IDs[previous_tick], previous_sequence, cs.in_flight_IDs[tick]);
                    }
                }
                else{
                    LOG_DEBUG(NetworkState, "%s id_to_sequence[%d][%d] = %d, tick: %d, id: %d, id_tick: %d", connection, id_tick, i, cs.id_to_sequence[id_tick][i], tick, id, id_tick );
                }
            }
            print_arrays(NetworkState, &cs, LogTypes::Debug);

            if(overwritten){
                // if(new_index == old_index){//probably not a problem after all, i'm on a case where we replace 357:357 with 421:357 so it makes sense they are the same
                    // LOG_ERROR(NetworkState, "%s resending AND overwritten. new_index: %d, old_index: %d SHOULD NOT BE HAPPENING", connection, new_index, old_index);
                    // LOG_ERROR(NetworkState, "%s resending AND overwritten. new_index: %d, old_index: %d SHOULD NOT BE HAPPENING", connection, new_index, old_index);
                    // LOG_ERROR(NetworkState, "%s resending AND overwritten. new_index: %d, old_index: %d SHOULD NOT BE HAPPENING", connection, new_index, old_index);
                // }
                if(old_index == -1){
                    LOG_ERROR(NetworkState, "%s old_index: %d ERROR", connection, old_index);
                }
                else{
                    //need to pop and swap old index of what we just resent
                    int last_index = cs.in_flight_packet_count - 1 & (SNAPSHOT_BUFFER_SIZE - 1);
                    if(cs.packed_sequences[last_index] == -1){
                        LOG_DEBUG(NetworkState, "%s why is the last index in packed sequences -1 when we try to pop and swap for overwrite and resend? last index: %d", connection, last_index);
                        LOG_DEBUG(NetworkState, "%s why is the last index in packed sequences -1 when we try to pop and swap for overwrite and resend? last index: %d", connection, last_index);
                    }
                    int swapped_tick = cs.packed_to_tick[last_index];
                    int swapped_id = cs.in_flight_IDs[swapped_tick];
                    int swapped_id_tick = swapped_id & (ID_BUFFER_SIZE - 1);
                    LOG_DEBUG(NetworkState, "%s RESENDING AND OVERWRITTEN! swapped_tick: %d, swapped_id: %d, swapped_id_tick: %d, last_index: %d, old_index: %d", connection, swapped_tick, swapped_id, swapped_id_tick, last_index, old_index );

                    LOG_DEBUG(NetworkState, "%s cs.tick_to_packed[%d] = %d", connection,swapped_tick, old_index );
                    cs.tick_to_packed[swapped_tick] = old_index;

                    LOG_DEBUG(NetworkState, "%s cs.packed_to_tick[%d] = %d", connection,old_index, swapped_tick );
                    cs.packed_to_tick[old_index] = swapped_tick;

                    LOG_DEBUG(NetworkState, "%s cs.packed_sequences[%d]:%d  = cs.packed_sequences[%d]:%d", connection,old_index,cs.packed_sequences[old_index], last_index, cs.packed_sequences[last_index]);
                    cs.packed_sequences[old_index] = cs.packed_sequences[last_index];

                    // if(cs.packed_sequences[22] == 5122){
                    //     printf("");
                    // }
                    //NOTE: whenever we swap a sequence, it's ID could potentially be associated with more previously sent sequences
                    //so we need to set all those previous active sequences to point to that same new position
                    if(old_index == last_index)old_index = new_index;//i have observed that if they are the same, it will get cleared anyway, and the potential older sequences will not be set to the new location

                    for(int i = 0; i < MAX_SEND_ATTEMPTS; i++){//set all the previous associated slots to the new index
                        if(cs.id_to_sequence[swapped_id_tick][i] != -1){
                            uint16_t previous_sequence = cs.id_to_sequence[swapped_id_tick][i];
                            uint16_t previous_tick = previous_sequence & (SNAPSHOT_BUFFER_SIZE - 1);
                            uint16_t previous_index = cs.tick_to_packed[previous_tick];

                            if(cs.in_flight_sequences[previous_tick] == previous_sequence){ //sometimes newer sequences can overwrite older ones, in which case we need to check so we don't overwrite the newer sequences
                                LOG_DEBUG(NetworkState, "%s swapping old tick_to_packed[%d]:%d to %d, swapped_tick: %d, id: %d, previous_sequence: %d, previous_index: %d",connection, previous_tick, cs.tick_to_packed[previous_tick], new_index,  swapped_tick, swapped_id, previous_sequence, previous_index);
                                cs.tick_to_packed[previous_tick] = old_index;
                            }
                            else{//don't want to overwrite this slot, since it contains NEW information that has overwritten this outdated piece of information
                                LOG_DEBUG(NetworkState, "%s, swapped_tick: %d contains sequence:id: %d:%d which must have overwritten the previously stored sequence:id of %d:%d", connection, cs.in_flight_sequences[previous_tick], cs.in_flight_IDs[previous_tick], previous_sequence, cs.in_flight_IDs[swapped_tick]);
                            }
                        }
                        else{
                            LOG_DEBUG(NetworkState, "%s id_to_sequence[%d][%d] = %d, swapped_tick: %d, swapped_id: %d, swapped_id_tick: %d", connection, swapped_id_tick, i, cs.id_to_sequence[swapped_id_tick][i], swapped_tick, swapped_id, swapped_id_tick );
                        }
                    }

                    
                    cs.packed_to_tick[last_index] = -1;
                    cs.packed_sequences[last_index] = -1;
                    cs.in_flight_packet_count--;
                    //do we want to decrement the in flight count? yes, but we are resending, so wont that disrupt the loop?
                    // printf("%sDECREMENTING IN FLIGHT PACKET COUNT TO: %d%s\n", BRED, cs.in_flight_packet_count, RESET);

                }

            }


        }
        else{//handle packed_sequences here
            //not resending, putting a new packet in flight
            if(!overwritten){
                cs.in_flight_packet_count = (cs.in_flight_packet_count + 1) & (SNAPSHOT_BUFFER_SIZE - 1);
                // printf("%sINCREMENTING IN FLIGHT PACKET COUNT TO: %d%s\n", BRED, cs.in_flight_packet_count, RESET);
            }
            cs.packed_sequences[index] = header->sequence;


        }

        // LOGLINE_TRACE(NetworkState, "packed sequences:");
        // for(int m = 0; m < cs.in_flight_packet_count; m++){
        //     LOGLINE_TRACE(NetworkState, "%d ", cs.packed_sequences[m]);
        // }
        // LOGLINE_TRACE(NetworkState, "\n");

        print_arrays(NetworkState, &cs, LogTypes::Debug);


        // if(overwritten){
        //     print_arrays(NetworkState, &cs, LogTypes::Warn);
        // }

        cs.sent_packets++;

        //regardless of what we did it was another send attempt so get the id_tick and increment
        //we just cleared the slot so now we need to increment it since we attempted a send
        cs.id_to_sequence[id_tick][cs.id_send_attempts[id_tick]++] = header->sequence;
        cs.id_in_flight_count[id_tick]++;
        
        validate_packed_sequences(NetworkState, &cs);

        if(overwritten && resending)return 1; //so the resend loop will know not to increment since we swapped things around

        return 0;
    }





    
    int sim_send_binary_chunk(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, 
                            int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, char* buffer, 
                            uint32_t& buffer_size, bool is_server){
    // int packet_manager_send_binary_udp(network_state* NetworkState, int socket, packet_header header, char* payload, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token){
        PacketManager& pm = NetworkState->packetManager;

        //to help myself log who's sending what
        // const char* connection = BBLUE "[SERVER]" RESET;
        const char* connection = SERVER_STR;
        ConnectionState* receivingCS = nullptr;
        if(is_server){
            receivingCS = &NetworkState->netTest.client_connection_state[clientID]; //server is sending to client
        }
        else{
            receivingCS = &NetworkState->connectionState[clientID];//client is sending to server
            // const char* connection = BGREEN "[CLIENT]" RESET;
            connection = CLIENT_STR;
        }

        unsigned char* payload = (unsigned char*)buffer;

        int num_slices = ((buffer_size) / MAX_UDP_SIZE);
        int curr_slice = 0;

        assert(num_slices == cs.num_slices && "calculated num slices is not equal to the connection state number of slices, programmer error");

        uint32_t remaining_size = buffer_size;

        // if(!header){
        packet_header header = {};
        //TODO:
        //can definitely overflow 16 bits if we can send up to 256kb of data. how do we handle this?
        header.size = MAX_UDP_SIZE;//just set header size to max udp size

        header.token = token;
        header.id = UINT16_MAX;
        // header.sequence = pm.local_sequence[clientID];
        //end packet manager shit
        header.sequence = UINT16_MAX;
        // }

        header.ack = 0;
        header.ack_bitfield = 0;
        // header.time_sent = GetTimeMS();
        header.total_fragments = num_slices - 1;//because +1 to it on receive, since we only have 255 vals and we want to be able to send 256 slices for chunks
        header.current_fragment = curr_slice;

        if(header.size <= 0){
            //nothing to send, return
            return -1;
        }
        // uint32_t now = GetTimeMS();
        uint32_t now = GetTimeMS()/*REPLACE*/;

        LOG_DEBUG(NetworkState, "%s %sSENDING CHUNK%s header: ack: %d, token: %d, id: %d, time_sent: %d, sequence: %d: ", connection, MAGENTA, RESET, header.ack, header.token, header.id, now, header.sequence);

        int send_count = 0;
        cs.oldest_slice_send_time = now;

        cs.any_slices_sent = true;


        print_send_chunk_bitfield(NetworkState, cs);


        //TODO: CONGESTION CONTROL
        for (int i = 0; i < num_slices; i++) {
            int array_index = i / 64;     // Which uint64_t in the array
            int bit_index = i & 63;       // Which bit within that uint64_t (same as i % 64)
            
            //setup the header info, paste in the binary data, and send
            assert(remaining_size != 0 && "send packet, remaining size is 0?? ERROR");

            if(remaining_size > MAX_UDP_SIZE){
                header.size = MAX_UDP_SIZE;
            }else{
                header.size = remaining_size;
            }


            // check which entries have not been acked
            if (!(cs.send_chunk_bitfield[array_index] & (1ULL << bit_index))) {

                if(now - cs.slice_send_time[i] > cs.latency * 2.5){ //trying to see if increasing the resend timeout will reduce bandwidth consumption, giving us time to receive an ack
                    LOG_ERROR(NetworkState, "%s SLICE %d NOT YET ACKED, SENT %d ms ago, RESENDING, times sent: %d", CLIENT_STR, i, now - cs.slice_send_time[i], cs.times_slice_sent[i]);
                    //need to send slice again

                    char send_buffer[SINGLE_FRAGMENT_SIZE];


                    int headerSize = pack((unsigned char*)send_buffer, "HCCHHHHQ", header.token, header.total_fragments, curr_slice, header.id, header.sequence, header.ack, header.size, header.ack_bitfield);

                    if(headerSize + header.size > SINGLE_FRAGMENT_SIZE){
                        LOG_ERROR(NetworkState, "message too big for UDP");
                        return -3;
                    }

                    //copy payload right after header
                    if(header.size > 0){
                        memcpy(send_buffer + headerSize, payload + (curr_slice * MAX_UDP_SIZE), header.size);
                    }

                    int total_size = headerSize + header.size;
                    if(NetworkState->netTest.enabled){
                        int random_val = rand() % 100;
                        // Drop packet if random number is less than the loss percentage
                        bool dropped = random_val < NetworkState->netTest.packet_loss_percentage;

                        if(!cs.first_message_sent){dropped = true;} //always drop the first packet just as a test

                        if(!dropped){
                            sim_sendto(NetworkState, send_buffer, clientID, total_size, addr, is_server, receivingCS, &header);
                            send_count++;
                        }else{
                            LOG_DEBUG(NetworkState, "%s %sPACKET SEQUENCE: %d SLICE: %d, DROPPED!%s ", connection, RED, header.sequence, curr_slice, RESET);
                            cs.lost_slices++;
                        }
                    }
                    else{
                        if(sendto(socket, send_buffer, total_size, 0, addr, addr_len) != total_size){
                            LOG_ERROR(NetworkState, "UDP message sendto() FAILED\n");
                            return -1;
                        }
                        send_count++;
                    }

                    cs.slice_send_time[i] = now;
                    
                    *total_sent += total_size;

                    cs.times_slice_sent[curr_slice]++;
                    cs.send_slice_headers[curr_slice] = header;
                    cs.sent_slices++;
                    
                }


            }

            curr_slice++;
            
            remaining_size -= header.size;
                    


            // // To set a bit:
            // cs->send_chunk_bitfield[array_index] |= (1ULL << bit_index);
            
            // // To clear a bit:
            // cs->send_chunk_bitfield[array_index] &= ~(1ULL << bit_index);
        }


        return 0;

    }






    void setup_sim_client(network_state* NetworkState, uint8_t& clientID){
        NetTest& nt = NetworkState->netTest;
        if(nt.num_clients >= MAX_CLIENTS){
            LOG_DEBUG(NetworkState, "setup_sim_client() too many clients, returning");
        }

        uint32_t new_addr = 0;
        if(nt.free_s_addr_count > 0){
            nt.free_s_addr_count--;
            new_addr = nt.free_s_addr[nt.free_s_addr_count];
        }else{
            new_addr = nt.sim_s_addr;
            nt.sim_s_addr++;
        }

        clientID = 0;
        if(nt.free_clientID_count > 0){
            nt.free_clientID_count--;
            clientID = nt.free_clientIDs[nt.free_clientID_count];
        }else{
            clientID = nt.num_clients;
        }

        ConnectionState& cs = nt.client_connection_state[nt.num_clients];

        init_connection_state(NetworkState, cs);
        cs.socket_udp_addr.sin_addr.s_addr = new_addr; 
        cs.socket_udp_addr.sin_port = (uint16_t) new_addr;
        cs.is_client = true;
        cs.is_server = false;

        // nt.client_udp_addrs[nt.num_clients].sin_addr.s_addr = new_addr; 
        // nt.client_udp_addrs[nt.num_clients].sin_port = (uint16_t) new_addr;
        // nt.client_connection_state[nt.num_clients].is_client = true;
        

        nt.num_clients++;

    }

    void clear_sim_client(network_state* NetworkState, uint32_t clientID){
        NetTest& nt = NetworkState->netTest;
        ConnectionState& cs = nt.client_connection_state[clientID];

        // nt.free_s_addr[nt.free_s_addr_count] = nt.client_udp_addrs[clientID].sin_addr.s_addr;
        nt.free_s_addr[nt.free_s_addr_count] = cs.socket_udp_addr.sin_addr.s_addr;
        nt.free_s_addr_count++;

        nt.free_clientIDs[nt.free_clientID_count] = clientID;
        nt.free_clientID_count++;

        //nt.client_udp_addrs[clientID].sin_addr.s_addr = UINT32_MAX;
        //nt.client_udp_addrs[clientID].sin_port = UINT16_MAX;
        
        cs.socket_udp_addr.sin_addr.s_addr = UINT32_MAX;
        cs.socket_udp_addr.sin_port = UINT16_MAX;


        if(nt.num_clients - 1 > 0){
            int last_clientID = nt.num_clients - 1;
            if(last_clientID != clientID){
                ConnectionState& last_cs = nt.client_connection_state[last_clientID];
                //still have clients, need to pop and swap
                // nt.client_udp_addrs[clientID].sin_addr.s_addr = nt.client_udp_addrs[last_clientID].sin_addr.s_addr;
                // nt.client_udp_addrs[clientID].sin_port        = nt.client_udp_addrs[last_clientID].sin_port;    

                cs.socket_udp_addr.sin_addr.s_addr = last_cs.socket_udp_addr.sin_addr.s_addr;
                cs.socket_udp_addr.sin_port = last_cs.socket_udp_addr.sin_port;

                last_cs.socket_udp_addr.sin_addr.s_addr = UINT32_MAX;
                last_cs.socket_udp_addr.sin_port = UINT16_MAX;


                //TODO: clear server side connection as well and set it equal to the last client in the array
                //TODO: clear simulated socket buffer info like the char/bytes/read/write, all of it


            }

        }


        nt.num_clients--;



    }

    int sim_recvfrom(network_state* NetworkState, char* buffer, sockaddr* src_addr, packet_header& debug_header, uint8_t clientID, bool sim_client, ConnectionState* cs){
        NetTest& nt = NetworkState->netTest;
        //the socket is the slot I want to read from in the net test struct
        int bytes = 0;
        uint32_t now = GetTimeMS()/*REPLACE*/;


         if(!sim_client){//simulated server socket

            //determine which client we receive from
            if(nt.server_waiting_messages_total > 0){
                for(int client_id = 0; client_id < nt.num_clients; client_id++){
                    cs = &NetworkState->connectionState[client_id];
                    if(cs->socket_waiting_messages > 0){

                        int& write = cs->socket_read_index   ;
                        int& read  = cs->socket_write_index  ;
                        int& count = cs->socket_message_count;
                        if(count > 0){
                            // if(GetTimeMS() < cs->socket_arrival_time[read]){
                            // printf("now: %u, int now: %d, arrival time: %u, diff: %d\n", now, (int32_t)now, cs->socket_arrival_time[read], (int32_t)now - cs->socket_arrival_time[read]);
                            if((int32_t)(now - cs->socket_arrival_time[read]) < 0){//if its negative, that means arrival time is in the future and will skip
                                //message hasnt arrive yet
                                continue; //cycle to next client to see if we can receive
                            }

                            bytes = cs->socket_buffer_bytes[read];
                            memcpy(buffer, cs->socket_buffer[read], bytes);
                            memset(cs->socket_buffer[read], 0, MAX_UDP_SIZE + PACKED_MESSAGE_HEADER_SIZE);
                            debug_header = cs->socket_headers[read];
                                        
                            // Cast sockaddr to sockaddr_in and populate it
                            sockaddr_in* src_in = reinterpret_cast<sockaddr_in*>(src_addr);
                            src_in->sin_family = AF_INET; // Ensure address family is set

                            // Extract address and port from server_addr_buffer
                            const uint8_t* addr_data = reinterpret_cast<const uint8_t*>(&cs->socket_addr_buffer[read]);
                            src_in->sin_addr.s_addr = *reinterpret_cast<const uint32_t*>(addr_data); // First 4 bytes
                            src_in->sin_port = *reinterpret_cast<const uint16_t*>(addr_data + sizeof(uint32_t)); // Next 2 bytes
                            cs->socket_waiting_messages--;
                            cs->socket_message_count--;
                            LOG_DEBUG(NetworkState, "%s PULLING SEQUENCE: %d, size: %d, now: %u, read slot: %d",SERVER_STR, debug_header.sequence, debug_header.size, now, read);

                            count--;
                            read = (read + 1) & (SIM_BUFFER_SIZE - 1);

                            nt.server_waiting_messages_total--;

                            return bytes;
                        }
                        else{
                            LOG_DEBUG(NetworkState, "sim_recvfrom() WARNING, SERVER BUFFER EMPTY!");
                            continue;
                        } 

                    }
                }
            }


            

         }else{ //simulated client socket
         
            int& write = cs->socket_read_index   ;
            int& read  = cs->socket_write_index  ;
            int& count = cs->socket_message_count;

            if(count > 0){
                // if(GetTimeMS() < cs->socket_arrival_time[read]){
                if((int32_t)(now - cs->socket_arrival_time[read]) < 0){ //if its negative, that means arrival time is in the future and will skip
                    
                    //message hasnt arrive yet
                    return -1;
                }

                bytes = cs->socket_buffer_bytes[read];
                memcpy(buffer, cs->socket_buffer[read], bytes);
                memset(cs->socket_buffer[read], 0, MAX_UDP_SIZE + PACKED_MESSAGE_HEADER_SIZE);
                debug_header = cs->socket_headers[read];

                // Cast sockaddr to sockaddr_in and populate it
                sockaddr_in* src_in = reinterpret_cast<sockaddr_in*>(src_addr);
                src_in->sin_family = AF_INET; // Ensure address family is set

                // Extract address and port from server_addr_buffer
                const uint8_t* addr_data = reinterpret_cast<const uint8_t*>(&cs->socket_addr_buffer[read]);
                src_in->sin_addr.s_addr = *reinterpret_cast<const uint32_t*>(addr_data); // First 4 bytes
                src_in->sin_port = *reinterpret_cast<const uint16_t*>(addr_data + sizeof(uint32_t)); // Next 2 bytes
                cs->socket_waiting_messages--;
                cs->socket_message_count--;
                count--;
                
                LOG_DEBUG(NetworkState, "%s PULLING SEQUENCE: %d, size: %d, now: %u, read slot: %d",CLIENT_STR, debug_header.sequence, debug_header.size, now, read);
                read = (read + 1) & (SIM_BUFFER_SIZE - 1);


                nt.client_socket_total_count--;

                return bytes;
            }
                LOG_ERROR(NetworkState, "sim_recvfrom() ERROR, CLIENT %d BUFFER EMPTY!", clientID);
            return -1;
         }

        return 0;
    }

    void sim_sendto(network_state* NetworkState, char* buffer, uint8_t clientID, int len, sockaddr* dest_addr, bool server, ConnectionState* cs, packet_header* debug_header){
        NetTest& nt = NetworkState->netTest;

        //the socket is the slot I want to read from in the net test struct


        int& write = cs->socket_read_index   ;
        int& read  = cs->socket_write_index  ;
        int& count = cs->socket_message_count;
        if(count < SIM_BUFFER_SIZE){
            memcpy(cs->socket_buffer[write], buffer, len);
            cs->socket_buffer_bytes[write] = len;
            // cs->socket_arrival_time[write] = simulate_jitter(GetTimeMS() + cs->latency);//this will be the time the message arrives
            uint32_t arrival_time_jitter = simulate_jitter(GetTimeMS()/*REPLACE*/ + cs->latency);
            LOG_DEBUG(NetworkState, "arrival time: %u, latency: %u, write slot: %d", arrival_time_jitter, cs->latency, write);
            cs->socket_arrival_time[write] = arrival_time_jitter;//this will be the time the message arrives
            // Cast `dest_addr` to `sockaddr_in` and store the destination address
            sockaddr_in* dest_in = reinterpret_cast<sockaddr_in*>(dest_addr);
            cs->socket_addr_buffer[write] = dest_in->sin_addr.s_addr; // Store IPv4 address as uint32_t, used to be ULONG but thats incompatible on linux

            cs->socket_headers[write] = *debug_header;

            write = (write + 1) & (SIM_BUFFER_SIZE - 1);
            count++;
            cs->socket_message_count++;
            cs->socket_waiting_messages++;

            if(server){//server sending to client
                nt.client_socket_total_count++;
            }else{ //client sending to server
                nt.server_waiting_messages_total++;
            }

        }else{
            if(server){//server sending to client
                LOG_ERROR(NetworkState, "sim_sendto() ERROR, CLIENT %d BUFFER FULL!", clientID);
            }else{
                LOG_ERROR(NetworkState, "sim_sendto() ERROR, SERVER BUFFER FULL!");
            }
        }
    }


