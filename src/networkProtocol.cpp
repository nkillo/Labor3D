#include "networkProtocol.h"



int send_message_tcp(int socket, uint16_t type, const char* payload, uint16_t payload_size, int* total_sent, uint16_t token, uint32_t sequence, uint64_t timestamp, uint16_t flags){
    if(socket == INVALID_SOCKET){
        printf("send_message_tcp INVALID SOCKET, CANCEL SEND\n");
        return -1;
    }
    char send_buffer[MAX_TCP_SIZE];

    //put header at start of message
    int headerSize = pack((unsigned char*)send_buffer, "HHHLQH", type, payload_size, token, sequence, timestamp, flags);
    
    if(payload_size > 0){
        memcpy(send_buffer + headerSize, payload, payload_size);
    }

    int total_size = headerSize + payload_size;

    int bytes_left = total_size; // left to send
    int n;

    while(*total_sent < total_size) {
        n = send(socket, send_buffer + *total_sent, bytes_left, 0);

        if (n == -1) { break; }
        *total_sent += n;
        bytes_left -= n;
    }


    if(n == -1){
        printf("Failed to send TCP Message!\n");
        return -1;
    }
    return 0;
}

int receive_message_tcp(int socket, MessageHeader* header, char* out_buffer, int buffer_size, ReadBuffer* read_buffer) {
    // Try to read whatever data is available
    int bytes = recv(socket, 
                    read_buffer->buffer + read_buffer->bytes_stored,
                    MAX_TCP_SIZE - read_buffer->bytes_stored,
                    0);
    
    #ifdef _WIN32
        // Windows
        if (bytes < 0) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                return 0;  // No data available
            }
            return -1;  // Error
        }
    #else
        // Unix/Linux/Mac
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;  // No data available
            }
            return -1;  // Error
        }
    #endif

    if (bytes == 0) return -2;  // Disconnected
    
    read_buffer->bytes_stored += bytes;

    // Do we have enough for a header yet?
    if (read_buffer->bytes_stored < PACKED_MESSAGE_HEADER_SIZE) {
        return 0;  // Need more data, try again later
    }

    // We have a header, check if we have the full message
    unpack((unsigned char*)read_buffer->buffer, "HHHLQH", &header->type, &header->size, &header->token, &header->sequence, &header->timestamp, &header->flags);

    int total_needed = PACKED_MESSAGE_HEADER_SIZE + header->size;
    
    if (read_buffer->bytes_stored < total_needed) {
        return 0;  // Need more data, try again later
    }

    // We have a complete message!
    if (header->size > 0) {
        memcpy(out_buffer, read_buffer->buffer + PACKED_MESSAGE_HEADER_SIZE, header->size);
    }

    // Move any remaining data to start of buffer
    int remaining = read_buffer->bytes_stored - total_needed;
    if (remaining > 0) {
        memmove(read_buffer->buffer, 
                read_buffer->buffer + total_needed, 
                remaining);
    }
    read_buffer->bytes_stored = remaining;

    return 1;  // Got a complete message
}




int send_message_udp(int socket, uint16_t type, const char* payload, uint16_t payload_size, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint16_t token, uint32_t sequence, uint64_t timestamp, uint16_t flags){
    
    //send in sizeof(client->server_addr_udp); for the addr_len field

    //for udp we send header + payload together to avoid splitting packets
    char send_buffer[MAX_UDP_SIZE];
    int headerSize = pack((unsigned char*)send_buffer, "HHHLQH", type, payload_size, token, sequence, timestamp, flags);

    if(headerSize + payload_size > MAX_UDP_SIZE){
        printf("message too big for UDP\n");
        return -3; //message too big for UDP
    }

    //copy payload right after header
    if(payload_size > 0){
        memcpy(send_buffer + headerSize, payload, payload_size);
    }

    int total_size = headerSize + payload_size;
    if(sendto(socket, send_buffer, total_size, 0, dest_addr, addr_len) != total_size){
        printf("UDP message sendto() FAILED\n");
        return -1;
    }
    *total_sent = total_size;
    return 0;

}


int receive_message_udp(int socket, MessageHeader* header, char* buffer, int buffer_size, sockaddr* src_addr, socklen_t* addr_len){
    char recv_buffer[MAX_UDP_SIZE];

    int bytes = recvfrom(socket, recv_buffer, sizeof(recv_buffer), 0, src_addr, addr_len);
    if(bytes < 0) {
        // Consider adding error handling here - could be EAGAIN, EWOULDBLOCK, etc.
        // printf("receive_message_udp() bytes less than 0? bytes: %d\n", bytes);
        return -1;
    }
    
    if(bytes < PACKED_MESSAGE_HEADER_SIZE) {
        printf("received UDP message too small! bytes: %d\n", bytes);
        return -1;
    }
    unpack((unsigned char*)recv_buffer, "HHHLQH", &header->type, &header->size, &header->token, &header->sequence, &header->timestamp, &header->flags);
    
    if(header->size > buffer_size){
        printf("received UDP message buffer too small! header->size: %d, buffer_size: %d\n", header->size, buffer_size);
        return -2;
    }

    //copy payload to users buffer
    if(header->size > 0){
        memcpy(buffer, recv_buffer + PACKED_MESSAGE_HEADER_SIZE, header->size);
    }
    return header->size;
}





int send_binary_udp(int socket, uint16_t type, const unsigned char* payload, uint16_t payload_size, sockaddr* dest_addr, socklen_t addr_len, int* total_sent, uint16_t token, uint32_t sequence, uint64_t timestamp, uint16_t flags){
    
    //send in sizeof(client->server_addr_udp); for the addr_len field

    //for udp we send header + payload together to avoid splitting packets
    char send_buffer[MAX_UDP_SIZE];
    int headerSize = pack((unsigned char*)send_buffer, "HHHLQH", type, payload_size, token, sequence, timestamp, flags);

    if(headerSize + payload_size > MAX_UDP_SIZE){
        printf("message too big for UDP\n");
        return -3; //message too big for UDP
    }

    //copy payload right after header
    if(payload_size > 0){
        memcpy(send_buffer + headerSize, payload, payload_size);
    }

    int total_size = headerSize + payload_size;
    if(sendto(socket, send_buffer, total_size, 0, dest_addr, addr_len) != total_size){
        printf("UDP message sendto() FAILED\n");
        return -1;
    }
    *total_sent = total_size;
    return 0;

}


int receive_binary_udp(int socket, MessageHeader* header, unsigned char* buffer, int buffer_size, sockaddr* src_addr, socklen_t* addr_len){
    char recv_buffer[MAX_UDP_SIZE];

    int bytes = recvfrom(socket, recv_buffer, sizeof(recv_buffer), 0, src_addr, addr_len);
    if(bytes < 0) {
        // Consider adding error handling here - could be EAGAIN, EWOULDBLOCK, etc.
        printf("receive_message_udp() bytes less than 0? bytes: %d\n", bytes);
        return -1;
    }
    
    if(bytes < PACKED_MESSAGE_HEADER_SIZE) {
        printf("received UDP message too small! bytes: %d\n", bytes);
        return -1;
    }
    unpack((unsigned char*)recv_buffer, "HHHLQH", &header->type, &header->size, &header->token, &header->sequence, &header->timestamp, &header->flags);
    
    if(header->size > buffer_size){
        printf("received UDP message buffer too small! header->size: %d, buffer_size: %d\n", header->size, buffer_size);
        return -2;
    }

    //copy payload to users buffer
    if(header->size > 0){
        memcpy(buffer, recv_buffer + PACKED_MESSAGE_HEADER_SIZE, header->size);
    }
    return header->size;
}


