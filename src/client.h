#pragma once

#include "networkProtocol.h"
#include "network.h"

#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"  // localhost for testing
#define TCP_PORT 12345
#define UDP_PORT 12346



struct MainClientSocketState {
    bool isConnected;
};



extern MainClientSocketState mainClientSocketState;


    bool init_client_sockets(network_state* NetworkState);
    void client_process_client_command(char* cmdString);
    
    void tell_network_thread_to_disconnect();//disconnects client

    void push_player_input_to_network(player_input* playerInput);
    void send_player_input(network_state* NetworkState, player_input* playerInput);


    void setup_server_connection(network_state* NetworkState, const char* ip, int udp_port);
    void connect_to_server(network_state* NetworkState);
    void disconnect_client(network_state* NetworkState, bool disconnectedFromServer = false);
    
    void setup_client(network_state* NetworkState);
    // Function to send UDP message
    void send_udp_message(network_state* NetworkState, const char* message);
    // Function to send TCP message
    void send_tcp_message(network_state* NetworkState, const char* message);

    // Poll for any incoming messages (similar to server's poll)
    int poll_client_messages(network_state* NetworkState);
    void send_disconnect(network_state* NetworkState);
    void send_ping(network_state* NetworkState);

    void handle_ping_response(network_state* NetworkState, char* buffer);
    
    void send_connected_handshake_request(network_state* NetworkState);
    void handle_connected_handshake_response(network_state* NetworkState, char* buffer, ConnectionState& cs);

    void send_time_sync(network_state* NetworkState);
    void handle_time_sync_response(network_state* NetworkState, char* buffer);









