#pragma once
#include "networkProtocol.h"



    void print_id_sequences(network_state* NetworkState, ConnectionState* cs, int id, LogTypes logtype);

    void validate_packed_sequences(network_state* NetworkState, ConnectionState* cs);

    void clear_sequence(network_state* NetworkState, ConnectionState* cs, int tick, bool timeout = false);
    
    void print_arrays(network_state* NetworkState, ConnectionState* cs, LogTypes logtype = LogTypes::Debug);
    void print_receive_chunk_bitfield(network_state* NetworkState, ConnectionState& cs);
    void print_send_chunk_bitfield(network_state* NetworkState, ConnectionState& cs);


    void sim_test(network_state* NetworkState);
    void sim_test_update(network_state* NetworkState);

    int send_all_queued_data(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, bool is_server);

    //need seperate buffers to put in communications between the endpoints
    int sim_network_command_append_to_packet(network_state* NetworkState, char* append_slot, uint16_t& append_buffer_size, char* buffer, uint16_t size);
    int sim_append_network_command(network_state* NetworkState, ConnectionState& cs, char* buffer, uint16_t packetsize);

    int sim_append_to_packet(network_state* NetworkState, ConnectionState& cs, char* buffer, uint16_t packetsize);

    void sim_send_to_server(network_state* NetworkState, uint8_t clientID);
    void setup_sim_client(network_state* NetworkState, uint8_t& clientID);
    void sim_send_connected_handshake_request(network_state* NetworkState, uint8_t clientID);
    int sim_network_thread_append_to_packet(network_state* NetworkState, char* append_slot, uint16_t& append_buffer_size, char* buffer, uint16_t size);
    
    int sim_send_binary_udp(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, 
                            int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, char* buffer, 
                            uint16_t& buffer_size, bool is_server, packet_header* header = nullptr, bool resending = false, int resent_tick = 0);

                            
    int sim_send_binary_chunk(network_state* NetworkState, int socket, sockaddr* addr, socklen_t addr_len, 
                            int* total_sent, uint32_t clientID, uint16_t token, ConnectionState& cs, char* buffer, 
                            uint32_t& buffer_size, bool is_server);


    int sim_recvfrom(network_state* NetworkState, char* buffer, sockaddr* src_addr, packet_header& debug_header,
                    uint8_t clientID = 0, bool sim_client = false, ConnectionState* cs = nullptr);

    void sim_sendto(network_state* NetworkState, char* buffer, uint8_t clientID, int len, sockaddr* dest_addr, 
                    bool is_server = true, ConnectionState* cs = nullptr, packet_header* debug_header = nullptr);
    