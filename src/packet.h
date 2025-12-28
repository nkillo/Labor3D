#ifndef PACKET_H
#define PACKET_H



    int AdjustBandwidthBudget(network_state* NetworkState, uint32_t& bandwidth_accumulator, uint32_t bytes);
    void BandwidthBudget(network_state* NetworkState, uint32_t& local_time_ms, uint32_t& bandwidth_accumulator);

    int receive_packet(network_state* NetworkState, bool sim_client = false, uint8_t sim_clientID = 0);
    int packet_manager_receive_binary_udp(network_state* NetworkState, int socket, packet_header& header, packet_header& debug_header, unsigned char* buffer, int buffer_size, sockaddr* src_addr, socklen_t* addr_len, uint8_t clientID = 0, bool sim_client = false);

    int check_in_flight_packets(network_state* NetworkState, ConnectionState& cs, int clientID = 0);

    int packet_manager_send_binary_udp(network_state* NetworkState, int socket, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token = 0);
    // int packet_manager_send_binary_udp(network_state* NetworkState, int socket, packet_header header, char* payload, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint32_t clientID, uint16_t token = 0);
    void init_packet_manager(network_state* NetworkState);

#endif