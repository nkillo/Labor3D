 #pragma once


#include "binarySerialization.h"



struct MessageHeader {
    uint16_t type;
    uint16_t size;
    uint16_t token;
    uint16_t sequence;
    uint16_t ack;
    uint64_t ackBitfield;
    uint64_t timestamp;
    uint16_t flags;    // Could include things like "reliable", "ordered", etc.
};


    

int send_message_tcp(int socket, uint16_t type, const char* payload, uint16_t payload_size, int* total_sent, uint16_t token = 0, uint32_t sequence = 0, uint64_t timestamp = 0, uint16_t flags = 0);
int receive_message_tcp(int socket, MessageHeader* header, char* out_buffer, int buffer_size, ReadBuffer* read_buffer);
int send_message_udp(int socket, uint16_t type, const char* payload, uint16_t payload_size, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint16_t token = 0, uint32_t sequence = 0, uint64_t timestamp = 0, uint16_t flags = 0);
int receive_message_udp(int socket, MessageHeader* header, char* buffer, int buffer_size, sockaddr* src_addr, socklen_t* addr_len);

int send_binary_udp(int socket, uint16_t type, const unsigned char* payload, uint16_t payload_size, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint16_t token = 0, uint32_t sequence = 0, uint64_t timestamp = 0, uint16_t flags = 0);
int receive_binary_udp(int socket, MessageHeader* header, unsigned char* buffer, int buffer_size, sockaddr* src_addr, socklen_t* addr_len);

//is s1 greater than s2, greater_than(current sequence, last sequence)
inline bool sequence_greater_than( uint16_t s1, uint16_t s2 )
{
    return  ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
            ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

inline bool is_sequence_wrapped(uint16_t s1, uint16_t s2){
    return ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

inline bool is_next_sequence(uint16_t current, uint16_t last) {
    return ((last + 1) & 0xFFFF) == current;
}