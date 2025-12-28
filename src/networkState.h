#pragma once

    // Use time.h for timestamps
    #include <time.h>
        static uint32_t GetTime() {
        return (uint32_t)time(NULL); // Or timespec for higher precision
    }



#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (SOCKET)(~0)
    #define CLOSE_SOCKET closesocket
    #define UINT64_FORMAT "I64u"
    typedef SOCKET socket_t;

       #include <windows.h>
   static uint32_t GetTimeMS() {
       LARGE_INTEGER frequency, count;
       QueryPerformanceFrequency(&frequency);
       QueryPerformanceCounter(&count);
       return (uint32_t)(count.QuadPart * 1000 / frequency.QuadPart);
   }

    
    
#else
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (-1)
    #define CLOSE_SOCKET close
    #define BOOL int
    #define TRUE 1
    #define UINT64_FORMAT "lu"
    typedef int socket_t;

       #include <sys/time.h>
   static uint32_t GetTimeMS() {
       struct timeval tv;
       gettimeofday(&tv, NULL);
       return tv.tv_sec * 1000 + tv.tv_usec / 1000;
   }

#endif







#include "fptvec.h"
#include "constants.h"
// #include "labour/core/binarySerialization.h"

//these use the color constants from labour/core/constants.h
#define CLIENT_STR    (BGREEN "[CLIENT]" RESET)
#define SERVER_STR     (BBLUE "[SERVER]" RESET)

#define MAX_UDP_SIZE 1024
#define SEND_QUEUE_SIZE 16
#define SIM_BUFFER_SIZE 512
#define MAX_FRAGMENTS 8
#define MAX_SLICES 256
#define MAX_PACKET_SIZE (MAX_UDP_SIZE * MAX_FRAGMENTS) //1024 * 8 = 8192 //DOESNT CONTAIN THE HEADERS

#define TARGET_BANDWIDTH 262144 //256kb lower estimate, maybe later change to 16777216,  16,777,216 bits per second. 16 mega bits per second
//around 32 KB per second
//accumulator will be 32000 * dt, dt = 0.01666 if using standard 16.66 ms or 60 ticks per second
//packet bytes + packet/header overhead, compare this against available bytes in the bandwidth budget
//if not enough bytes in the accumulator, stop. otherwise, accumulator -= bytes_required and repeat for next slice
//need minimum resend delay per slice. 

/*
gaffer on games notes
maintain array of last send time per slice, estimate RTT and only resend if it hast been acked within RTT * 1.25
dont bother using a lookup table. cycle through the entire thing up to num_slices_in_chunk and skip if its been acknowledged
uint32_t last_send_time_per_slice[MAX_SLICES];

if too many clients, send a message back to the connecting client with server full/connection denied
if no packet received for 5 second, client times out and the client is cleared from its slot

server receives connection request
server sends challenge packet to ip+port of request
client sends challenge response and then the connection is accepted
client/server salt, uint64_t in the packets, is this like a token?
2 connection states: connection request and challenge response
xor client and server salt together, ignore packets without matching signature

to disconnect, one side fires 10 disconnect packets over the network hoping some will get through



send an initial burst of packets on client connection
dont send burst if its very large, or theres other time critical data needing to be sent on the network

TODO::

we can worry about encryption once the basics are working

a client could me masquerading. they could connect regularly and then send too much data or malformed data
the server needs to identify an attacker and terminate the connection

*/


/*
test suite ideas
variable packet loss rate,                      rand()
variable send time/packet travel speed          rand()
variable jitter that specific packet arrival    rand()
clients connect/disconnect randomly             rand()

simulated clients that construct their own acks and simple test data in packets
simple function that sets up the fake clients, 
and enters an infinite loop that simulates the server flow, 
but disables the actual network sending code,
and potentially macro defines a fake send function that just specifies the clientID (0 to clientNum) to send the packet to instead
accumulator to send packets every 16.66ms
*/

#define MAX_TCP_SIZE 1400
// #define PACKED_MESSAGE_HEADER_SIZE 20 //2 + 2 + 2 + 4 + 8 + 2
#define PACKED_MESSAGE_HEADER_SIZE 20 //2 + 1 + 1 + 2 + 2 + 2 + 2 + 8
#define SINGLE_FRAGMENT_SIZE (MAX_UDP_SIZE + PACKED_MESSAGE_HEADER_SIZE) //the size of the actual sent data + the header
#define PING_TIMEOUT 5000  // 5 seconds in ms
#define PING_INTERVAL 1000 // 1 second in ms
#define BUFFER_SIZE 1024
#define TCP_PORT 12345
#define UDP_PORT 12346
#define MAX_SOCKET_ID 16384

#define MAX_INFLIGHT 1024
#define MAX_SEQUENCE 65536
#define MAX_LOCAL_PLAYERS 4


// static char* get_timestamp() {
//     // Allocate memory for the buffer
//     static char buffer[32];

//     // Get the current time as a time_t value
//     time_t now = time(NULL);

//     // Convert the time to a string representation
//     strftim(buffer, sizeof(buffer), "hhmmss", localtime(&now));

//     // Return the timestamp as a static char array
//     return buffer;
// }


// Timestamp helper (returns static buffer, not thread-safe)
static char* get_timestamp() {
    static char buffer[32];
    time_t now = time(NULL);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buffer;
}

static char* get_timeWithMilliseconds() {
    static char buffer[64];
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Format time with milliseconds
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", 
             st.wHour, 
             st.wMinute, 
             st.wSecond, 
             st.wMilliseconds);
    
    return buffer;
}


static void debug_log(const char* color, const char* level, const char* format, ...) {
    // Print the header with timestamp
    printf("%s[%s][%s]%s ", color, get_timestamp(), level, RESET);
    
    // Print the formatted message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}




enum MessageTypes{
    MSG_PING = 0,
    MSG_PONG,
    MSG_STRING,
    MSG_CHAT,
    MSG_CHAT_UDP,
    MSG_UDP_BROADCAST,
    MSG_UDP_WHISPER,
    MSG_BROADCAST,
    MSG_CONNECT_REQUEST,
    MSG_DISCONNECT,
    MSG_CONNECTED_HANDSHAKE,
    PLAYER_INPUT,
    ENTITY_STATE,
    SIMULATION,
    CHUNK_SEND_REQUEST,     //sent by chunk sender
    CHUNK_RECEIVE_READY,    //sent by chunk receiver to the chunk sender
    CHUNK_RECEIVE_NOT_READY,    //sent by chunk receiver to the chunk sender
    CHUNK_ACK_BITFIELD,         //sent by chunk receiver to chunk sender, contains which slices have been acknowledged
    CHUNK_INFO,
    CHUNK_SEND_FINISHED,
    END_NET_CMDS,
    INITIATE_SYNC, //example to demonstrate one way of re establishing a synced state between endpoints, used in the simulation only
    SYNC_RESPONSE, 
    RESYNC_DUMMY,
};



enum PacketTypes{
    Normal = 0,
    Fragment = 1,
    Slice = 2,
};

enum LogTypes{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
};

struct debug_logger{
    FILE* log_file;

    bool console_output;
    LogTypes lowest_printed_type;
};

#define WAS_PRESSED(input, field, enum_val)\
    (((input).field && input.transitionCounts[enum_val]) || (!(input).field && (input.transitionCounts[enum_val] >= 2)))

#define WAS_RELEASED(input, field, enum_val)\
    ((!(input).field && (input.transitionCounts[enum_val] == 1)))

struct textEditInputTEST{
    uint32_t active;
    uint32_t ticksSincePressed;
    uint32_t ticksSinceRepeat;
};


struct player_input {
    union {
        uint16_t buttons;     // The whole byte
        struct {
            uint16_t up         : 1;    
            uint16_t down       : 1;  
            uint16_t left       : 1;  
            uint16_t right      : 1; 
            uint16_t forward    : 1;
            uint16_t back       : 1;
            uint16_t attack     : 1;
            uint16_t jump       : 1; //block? other inputs? just key presses we interpret as actions?

            uint16_t interact   : 1;    
            uint16_t unused0    : 1;  
            uint16_t unused1    : 1;  
            uint16_t unused2    : 1; 
            uint16_t unused3    : 1;
            uint16_t unused4    : 1;
            uint16_t unused5    : 1;
            uint16_t unused6    : 1;

        } bits; //need to name it to do something like button.bits.up to access individual bits
    };

    union{
        uint16_t mouseButtons;
        struct{
            uint16_t left :      1;
            uint16_t middle :    1;
            uint16_t right :     1;
            uint16_t sideFront : 1;
            uint16_t sideBack :  1;
            uint16_t unused1 :  1;
            uint16_t unused2 :  1;
            uint16_t unused3 :  1;

        } mouse;
    };

    union{//for the UI system to take precedence over general entity input if we click into a window
        uint16_t consumedMouseFunctions;
        struct{
            uint16_t left :      1;
            uint16_t middle :    1;
            uint16_t right :     1;
            uint16_t sideFront : 1;
            uint16_t sideBack :  1;
            uint16_t delta :  1;
            uint16_t wheel :  1;
            uint16_t unused3 :  1;

        } consumedMouse;
    };

    union{
        uint16_t numberKeys;
        struct{
            uint16_t key0 : 1;
            uint16_t key1 : 1;
            uint16_t key2 : 1;
            uint16_t key3 : 1;

            uint16_t key4 : 1;
            uint16_t key5 : 1;
            uint16_t key6 : 1;
            uint16_t key7 : 1;
            
            uint16_t key8 : 1;
            uint16_t key9 : 1;
            uint16_t unused0 : 1;
            uint16_t unused1 : 1;

            uint16_t unused2 : 1;
            uint16_t unused3 : 1;
            uint16_t unused4 : 1;
            uint16_t unused5 : 1;

        }numbers;
    };

    union{
        uint16_t functionKeys;
        struct{
            uint16_t alt        : 1;
            uint16_t equals     : 1; //plus key
            uint16_t minus      : 1; 
            uint16_t keyESCAPE  : 1;

            uint16_t keyTAB     : 1;
            uint16_t keyCTRL    : 1;
            uint16_t shift      : 1;
            uint16_t keyReturn  : 1;

            uint16_t up         : 1;
            uint16_t down       : 1;
            uint16_t left       : 1;
            uint16_t right      : 1;

            uint16_t pageUp     : 1;
            uint16_t pageDown   : 1;
            uint16_t home       : 1;
            uint16_t end        : 1;


        }function;
    };
    int8_t mouse_wheel;
    // Other input data could go here...
    int8_t look_x;      // Optional rotation/analog data
    int8_t look_y;

    uint8_t action; //placeholder
    
    int16_t mouse_dx;
    int16_t mouse_dy;

    uint16_t mouse_x;
    uint16_t mouse_y;


    uint32_t left_click_tick_interval;
    uint32_t right_click_tick_interval;

    uint32_t left_click_count;
    uint32_t right_click_count;
    
    bool left_double_clicked;
    bool left_triple_clicked;

    bool right_double_clicked;
    bool right_triple_clicked;

    fpt fptAngleH; //player camera orientations
    fpt fptAngleV; //player camera orientations
    fpt_vec3 rayDir;

    uint32_t tick;
    uint32_t time;
    uint16_t sequence;
    union{
        uint16_t debugFlags;
        struct{
            uint16_t received :      1;
            uint16_t freeCam :    1; //toggle on/off
            uint16_t camSpeedUp :     1;
            uint16_t camSpeedDown : 1;
            uint16_t unused4 :  1;
            uint16_t unused5 :  1;
            uint16_t unused6 :  1;
            uint16_t unused7 :  1;

        } flags;

    };
        fpt dt; //to determine if the client is processing faster/slower than the server so the server can update it accordingly
        //we can probably get away with sending a uint16 since the dt will always be under 1 second

    uint8_t transitionCounts[InputTypes::input_count];
    
};

// struct entity_state{
//     uint32_t entityID;
//     uint32_t inputTime; 
//     uint32_t timeProcessed; 
//     fpt_quat rotation; 
//     fpt_vec3 position; 
//     fpt_vec3 pos_in_chunk; 
//     fpt_vec3 forward; 
//     fpt_vec3 up; 
//     fpt_vec3 right; 
//     fpt speed; 
//     fpt brushSize;
//     fpt angleH;
//     fpt angleV;
//     bool received;
//     uint32_t timeReceived;
// };

struct ReadBuffer {
    char buffer[MAX_TCP_SIZE];
    int bytes_stored;  // How many bytes currently in buffer
};

 struct PingTracker{
        uint64_t last_ping_time;
        uint64_t last_stats_time;
        uint16_t next_sequence;
        uint64_t sent_times[MAX_INFLIGHT]; //store timestamp for each sequence
        int lost_packets;
        float latest_rtt;
    };

    struct InputTracker{
        //circular buffers
        player_input player_input_buffers[MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //input history = 64
        uint8_t currentOrderedSequence[MAX_CLIENTS];//0 - 63 
    };

    
    struct packet_header{
        //for the sender to identify themselves
        uint16_t token;
        uint8_t  total_fragments;
        uint8_t  current_fragment; //or slices, to get actual value we do fragments + 1 since we can have 256 max slices, so 255 + 1 = 256
        //unique ID for message (just the sequence but we keep it if we need to resent)
        uint16_t id;
        uint16_t sequence;
        uint16_t ack;
        uint16_t size;
        uint64_t ack_bitfield;
        // uint32_t time_sent;
    };

    struct packet_entry{
        uint8_t  type; //message enum
        uint16_t size; //size of entry
    };


    struct PacketManager{
        
        int             bandwidth_accumulator;
        uint32_t        local_time_ms;

        char            send_queue          [MAX_CLIENTS][SEND_QUEUE_SIZE][MAX_UDP_SIZE];
        uint16_t        send_queue_size     [MAX_CLIENTS][SEND_QUEUE_SIZE];
        packet_header   header_queue        [MAX_CLIENTS][SEND_QUEUE_SIZE];
        uint8_t         queue_write_index   [MAX_CLIENTS];
        uint8_t         queue_read_index    [MAX_CLIENTS];
        uint8_t         queue_count [MAX_CLIENTS];
        uint16_t        queue_total_count;


        char        current_buffer          [MAX_CLIENTS][MAX_UDP_SIZE];
        uint16_t    current_buffer_size     [MAX_CLIENTS];

        char        local_buffer_history    [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //stores sent packets
        char        remote_received_history [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //stores received packets

        //per client, a buffer of 8 fragments of 1024 bytes each, max of 8192 bytes
        //used to reconstruct the packet 
        //8 bits because we have a max of 8 fragments per packet, if we need more packets, increase the data type to 16 or 32 or 64 etc
        // uint8_t     packet_fragment_bitfield [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];//determines which segments of the buffer have been populated. each bit represents a stretch of 1024 bytes
        // char        remote_packet_history    [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE]; //this will cost us a 16*64*8192bytes * 4 copies of network state = 33 megabytes
        // char        remote_packet_history1   [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE]; 
        // char        remote_packet_history2   [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE];

        //only 2 chunks can be used at a time. one that stores the old received one before its processed, another to start receiving new chunk data

        // uint64_t    current_chunk_bitfield[MAX_CLIENTS][4];//4 * 64 = 256
        // uint64_t    old_chunk_bitfield[MAX_CLIENTS][4];//4 * 64 = 256
        // char        current_chunk[MAX_CLIENTS][MAX_SLICES * MAX_UDP_SIZE];
        // char        old_chunk[MAX_CLIENTS][MAX_SLICES * MAX_UDP_SIZE];
        
        //to process incoming packets
        char        ordered_buffer          [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //temp stores newest packets
        packet_header  ordered_header       [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];
        uint64_t    ordered_bitfield        [MAX_CLIENTS];
        uint8_t     ordered_buffer_count    [MAX_CLIENTS];

        //stores the tick/modulo'd sequence of the packet. access it in the remote_received_history
        uint8_t latest_unprocessed_packet   [MAX_CLIENTS]; 
        uint8_t unprocessed_packet_count    [MAX_CLIENTS];

        uint8_t current_processed_packet    [MAX_CLIENTS];
        uint8_t oldest_processed_packet     [MAX_CLIENTS];


        packet_header local_header_history  [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //stores sent packets
        packet_header remote_header_history [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //stores received packets


        //queue of sequence numbers
        //each time we receive an ack, look up this entry and note the differrence in local time
        //between the time we receive the ack, and the time we sent the packet. this becomes RTT
        uint64_t in_flight_bitfield         [MAX_CLIENTS];
        uint16_t latest_in_flight_sequence  [MAX_CLIENTS];
        uint16_t packet_resend_timeouts     [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //tracks each packet sent over the current window and checks if they've been acked/need resending
        uint16_t packet_resend_IDs          [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //id of the message we want to resend
        uint16_t in_flight_sequences        [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //sequence of the message we want to resend
        uint16_t in_flight_packet_count     [MAX_CLIENTS];
        uint16_t index_to_tick              [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];
        uint16_t tick_to_index              [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];

        uint32_t rtt                        [MAX_CLIENTS];

        uint8_t  client_tick                [MAX_CLIENTS];
        uint8_t  tick_offset                [MAX_CLIENTS];
        uint16_t local_sequence             [MAX_CLIENTS]; //increment every time we send a packet to that specific client
        uint16_t remote_sequence            [MAX_CLIENTS]; //sequence of sender

        //tracks when the client first connects and if it needs to sync. we set it and handle state in receive_packet(). need to understand thoroughly before using it more
        bool     client_synced              [MAX_CLIENTS];



        uint16_t sequences                  [MAX_CLIENTS];
        uint16_t ack                        [MAX_CLIENTS]; //latest acked packet
        uint64_t ack_bitfield               [MAX_CLIENTS]; //ring buffer, 0 or 1 per ack. goes ack - (bitPosition + 1) = sequence acknowledged  

        uint8_t local_tick                          ;
        
        uint32_t resent_count;
        uint32_t lost_count;

    };


    struct ConnectionState{
        uint16_t remote_sequence    ;    
        uint16_t remote_id    ;    

        uint16_t local_sequence     ;
        uint16_t local_id     ;

        uint64_t ack_bitfield       ; //bitfield of received packets that we have acked
        
        char scratch_buffer[MAX_PACKET_SIZE];
        char scratch_entry_buffer  [MAX_UDP_SIZE];
        bool latest_in_flight_sequence_acked;
        bool latest_in_flight_ID_acked;

        uint64_t in_flight_bitfield;  //bitfield of sent packets not yet acknowledged
        int      latest_in_flight_sequence;
        int      oldest_in_flight_sequence;
        int      latest_acked_in_flight_sequence;
        int      latest_acked_in_flight_ID;

        uint32_t packet_resend_timeouts [SNAPSHOT_BUFFER_SIZE]; //tracks each packet sent over the current window and checks if they've been acked/need resending
        uint16_t packet_resend_IDs      [SNAPSHOT_BUFFER_SIZE]; //id of the message we want to resend
        int      in_flight_sequences    [SNAPSHOT_BUFFER_SIZE]; //sequence of the message we want to resend
        int      in_flight_IDs          [SNAPSHOT_BUFFER_SIZE]; //id of the message we want to resend

        int      id_to_sequence         [ID_BUFFER_SIZE][MAX_SEND_ATTEMPTS]; //use the ID to index into the slot
        int      id_in_flight_count     [ID_BUFFER_SIZE]; //current packets we have in flight, can be decremented if overwriting an entry
        int      id_send_attempts       [ID_BUFFER_SIZE]; //maximum times we've tried sending the packet, stop trying after the 4th send

/*
    //what it looks like. not pretty but hopefully it works
        int id_tick = header.id & (ID_BUFFER_SIZE - 1);
        cs.id_to_sequence[id_tick][cs.id_in_flight_count[id_tick]++] = header.sequence;
        cs.id_send_attempts[id_tick]++;
        
        */

        uint32_t send_times             [SNAPSHOT_BUFFER_SIZE];
        int      latest_tick;

        int      tick_to_packed         [SNAPSHOT_BUFFER_SIZE];
        int      packed_to_tick         [SNAPSHOT_BUFFER_SIZE];
        int      packed_sequences       [SNAPSHOT_BUFFER_SIZE]; 
        int      packed_IDs             [SNAPSHOT_BUFFER_SIZE]; 
        bool     resent                 [SNAPSHOT_BUFFER_SIZE]; 
        uint16_t in_flight_packet_count ;

        int received_sequences          [SNAPSHOT_BUFFER_SIZE];
        uint8_t received_fragment_bitfield[SNAPSHOT_BUFFER_SIZE];
        int received_ids                [ID_BUFFER_SIZE];
        uint32_t received_sequence_times[SNAPSHOT_BUFFER_SIZE];

        uint16_t token;

        uint64_t sent_slices;
        uint16_t acked_slices;
        uint64_t lost_slices;
        uint64_t sent_chunks;

        uint64_t sent_fragments;
        uint64_t lost_fragments;
        
        uint64_t sent_packets;
        uint64_t dropped_packets;
        uint64_t overwritten_packets;
        uint64_t resend_count;

        // char        local_buffer_history    [SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //stores sent packets
        char        local_buffer_history    [SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE]; //stores sent packets
        char        remote_received_history [SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE]; //stores received packets
        uint16_t    received_history_size   [SNAPSHOT_BUFFER_SIZE];
        uint16_t    local_history_size      [SNAPSHOT_BUFFER_SIZE];

        // packet_header local_header_history  [SNAPSHOT_BUFFER_SIZE]; //stores sent packets
        packet_header local_header_history  [SNAPSHOT_BUFFER_SIZE][MAX_FRAGMENTS]; //stores sent packets
        packet_header remote_header_history [SNAPSHOT_BUFFER_SIZE][MAX_FRAGMENTS]; //stores received packets
        bool          packet_is_fragmented  [SNAPSHOT_BUFFER_SIZE];
        uint8_t       fragment_bitfield     [SNAPSHOT_BUFFER_SIZE];
        uint8_t       packet_num_fragments  [SNAPSHOT_BUFFER_SIZE];
        

        uint8_t         local_slice_ack ;
        uint8_t         remote_slice_ack;
        uint16_t        num_slices;
        uint64_t        send_chunk_bitfield     [4];//4 * 64 = 256
        uint64_t        receive_chunk_bitfield  [4];//4 * 64 = 256
        char            send_chunk              [MAX_SLICES * MAX_UDP_SIZE];
        char            receive_chunk           [MAX_SLICES * MAX_UDP_SIZE];
        uint32_t        send_chunk_size;
        uint32_t        receive_chunk_size;
        packet_header   send_slice_headers      [MAX_SLICES];
        packet_header   receive_slice_headers   [MAX_SLICES];
        uint32_t        slice_send_time         [MAX_SLICES];
        uint32_t        slice_receive_time      [MAX_SLICES];
        int             times_slice_sent        [MAX_SLICES];
        uint32_t        chunk_send_request_timeout;   //for the sender to wait to send another chunk send request, wait 1 second for now
        bool            sending_chunk;          //puts us into the state to send a chunk
        bool            receiving_chunk;
        bool            ready_to_receive_chunk;
        bool            waiting_to_send_chunk;
        bool            any_slices_sent;
        uint16_t        send_sequence_number; //sequence number for the chunk
        uint32_t        oldest_slice_send_time;
        uint32_t        chunk_send_timeout;
        bool received_slice_this_tick;

        //we will probly handle ordering in the game layer, network layer is a blind transport layer
        // char        ordered_buffer          [SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //temp stores newest packets
        // packet_header  ordered_header       [SNAPSHOT_BUFFER_SIZE];
        // uint64_t    ordered_bitfield        ;
        // uint8_t     ordered_buffer_count    ;

        //stores the tick/modulo'd sequence of the packet. access it in the remote_received_history
        uint8_t latest_unprocessed_packet   ; 
        uint8_t unprocessed_packet_count    ;

        uint8_t current_processed_packet    ;
        uint8_t oldest_processed_packet     ;



        uint32_t RTT                        ;
        uint32_t SRTT;//smoothed round trip time

        uint8_t  client_tick                ;
        uint8_t  tick_offset                ;

        //tracks when the client first connects and if it needs to sync. we set it and handle state in receive_packet(). need to understand thoroughly before using it more
        bool     client_synced              ;



        uint16_t sequences                  ;
        uint16_t ack                        ; //latest acked packet

        bool connected;
        bool connecting;
        uint16_t originalToken;

        bool        is_client;
        bool        is_server;
        uint32_t    latency;
        bool        first_message_sent; //is this the first message we are sending?




        //game layer commands
        char        send_queue[SEND_QUEUE_SIZE][MAX_PACKET_SIZE];
        uint16_t    send_queue_size[SEND_QUEUE_SIZE];
        uint8_t     send_queue_write_index;
        uint8_t     send_queue_read_index;
        uint8_t     send_queue_count;

        //for network layer commands
        // char        network_send_queue[SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE];
        char            network_command[MAX_PACKET_SIZE]; //we send this every tick, flush it to the front of any packet we send
        // uint16_t    network_send_queue_size[SNAPSHOT_BUFFER_SIZE];
        uint16_t        network_command_size;
        // uint8_t     network_send_queue_write_index;
        // uint8_t     network_send_queue_read_index;
        // uint8_t     network_send_queue_count;


        // //SIMULATION SECTION
        int         socket_read_index            ;
        int         socket_write_index           ;
        int         socket_message_count         ;//we already added this to deal with sim send/receive
        size_t      socket_buffer_bytes                 [SIM_BUFFER_SIZE];
        char        socket_buffer                       [SIM_BUFFER_SIZE][MAX_UDP_SIZE + PACKED_MESSAGE_HEADER_SIZE];
        uint32_t    socket_arrival_time                 [SIM_BUFFER_SIZE];
        packet_header    socket_headers                 [SIM_BUFFER_SIZE];
        int         socket_waiting_messages;
        uint32_t    socket_addr_buffer                  [SIM_BUFFER_SIZE]; //stores address of sender who appended to our buffer
        sockaddr_in socket_udp_addr;        
        int socket;

        //if too many packets have been lost, toggle this and spam until we are back in sync
        bool need_resync;
        int times_resynced;
        int times_desynced;


        int packet_loss_percentage;
        int jitter_amount;
        uint32_t debug_log_timer;
        uint32_t last_log_time;
    
        uint32_t last_update;
        uint64_t connection_duration;

        //until this is completed, the server will continuously send the connection message to the client at the front of the packet
        bool connection_handshake_completed; //its considered completed when the server gets the correct token, and when the client receives the token

        uint32_t        bandwidth_accumulator;
        uint32_t        local_time_ms;
        uint32_t        target_bandwidth;
        uint32_t lost_count;
        uint32_t overwritten_id_count;
    };


    struct NetTest{
        uint8_t send_accumulator; //sends packets every 16ms, about 1 game tick
        bool enabled;
        //how do I handle the clients? how would they be simulated?
        //send a connect request, with a fake udp port
        //each one needs its own message buffer
        //we essentially want to simulate a whole network/clients connecting to a server
        //we can just create the exact same network state struct inside net test, i think that should works

        int             packet_loss_percentage;
        int             bandwidth_accumulator;
        uint32_t        local_time_ms;
        uint32_t        tick_accumulator;
        uint32_t        tick_timestep_ms;

        char            send_queue          [MAX_CLIENTS][SEND_QUEUE_SIZE][MAX_UDP_SIZE];
        uint16_t        send_queue_size     [MAX_CLIENTS][SEND_QUEUE_SIZE];
        packet_header   header_queue        [MAX_CLIENTS][SEND_QUEUE_SIZE];
        uint8_t         queue_write_index   [MAX_CLIENTS];
        uint8_t         queue_read_index    [MAX_CLIENTS];
        uint8_t         queue_count [MAX_CLIENTS];
        uint16_t        queue_total_count;

        uint32_t    sim_s_addr;//increments after we assign it to each new simulated connecting client
        uint32_t    free_s_addr             [MAX_CLIENTS];//return the addr to this list when a client disconnects
        uint32_t    free_s_addr_count;
        uint32_t    client_latency          [MAX_CLIENTS];

        // int         server_socket_read_index            [MAX_CLIENTS];
        // int         server_socket_write_index           [MAX_CLIENTS];
        // int         server_socket_message_count         [MAX_CLIENTS];
        // size_t      server_buffer_bytes                 [MAX_CLIENTS][SIM_BUFFER_SIZE];
        // char        server_socket_buffer                [MAX_CLIENTS][SIM_BUFFER_SIZE][MAX_UDP_SIZE + PACKED_MESSAGE_HEADER_SIZE];
        // uint32_t    server_arrival_time                 [MAX_CLIENTS][SIM_BUFFER_SIZE];
        int         server_waiting_messages_per_client  [MAX_CLIENTS];
        int         server_waiting_messages_total;
        // sockaddr_in server_addr_buffer      [SIM_BUFFER_SIZE];
        // uint32_t    server_addr_buffer                  [MAX_CLIENTS][SIM_BUFFER_SIZE];
        sockaddr_in server_udp_addr;

        // int         client_socket_read_index            [MAX_CLIENTS];
        // int         client_socket_write_index           [MAX_CLIENTS];
        // int         client_socket_message_count         [MAX_CLIENTS];
        int         client_socket_total_count;

        // size_t      client_buffer_bytes                 [MAX_CLIENTS][SIM_BUFFER_SIZE];
        // char        client_socket_buffer                [MAX_CLIENTS][SIM_BUFFER_SIZE][MAX_UDP_SIZE + PACKED_MESSAGE_HEADER_SIZE];
        // uint32_t    client_arrival_time                 [MAX_CLIENTS][SIM_BUFFER_SIZE];
        // sockaddr_in client_addr_buffer      [MAX_CLIENTS][SIM_BUFFER_SIZE];
        // uint32_t client_addr_buffer[MAX_CLIENTS][SIM_BUFFER_SIZE];
        sockaddr_in client_udp_addrs        [MAX_CLIENTS];  // Store full UDP address (IP+port)

        char        current_buffer          [MAX_CLIENTS][MAX_UDP_SIZE];
        uint16_t    current_buffer_size     [MAX_CLIENTS];

        char        local_buffer_history    [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //stores sent packets
        char        remote_received_history [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //stores received packets


        // char            client_send_queue          [MAX_CLIENTS][SEND_QUEUE_SIZE][MAX_UDP_SIZE];
        // uint16_t        client_send_queue_size     [MAX_CLIENTS][SEND_QUEUE_SIZE];
        // uint8_t         client_queue_write_index   [MAX_CLIENTS];
        // uint8_t         client_queue_read_index    [MAX_CLIENTS];
        // uint8_t         client_queue_count [MAX_CLIENTS];
        // uint16_t        client_queue_total_count;
        // uint16_t        client_sequence_number     [MAX_CLIENTS];




        ////////////////////////////////////////  BEGIN SERVER PACKET MANAGEMENT ///////////////////////////////////////////////

        ConnectionState server_connection_state [MAX_CLIENTS];

        ////////////////////////////////////////  END SERVER PACKET MANAGEMENT ///////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////  BEGIN CLIENT PACKET MANAGEMENT ///////////////////////////////////////////////

        ConnectionState client_connection_state  [MAX_CLIENTS];
        uint8_t         active_clients           [MAX_CLIENTS];
        uint8_t         free_clientIDs           [MAX_CLIENTS];
        uint8_t         free_clientID_count;
        uint8_t         num_clients;


        ////////////////////////////////////////  END CLIENT PACKET MANAGEMENT ///////////////////////////////////////////////




        //per client, a buffer of 8 fragments of 1024 bytes each, max of 8192 bytes
        //used to reconstruct the packet 
        //8 bits because we have a max of 8 fragments per packet, if we need more packets, increase the data type to 16 or 32 or 64 etc
        // uint8_t     packet_fragment_bitfield [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];//determines which segments of the buffer have been populated. each bit represents a stretch of 1024 bytes
        // char        remote_packet_history    [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE]; //this will cost us a 16*64*8192bytes * 4 copies of network state = 33 megabytes
        // char        remote_packet_history1   [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE]; 
        // char        remote_packet_history2   [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_PACKET_SIZE];

        //only 2 chunks can be used at a time. one that stores the old received one before its processed, another to start receiving new chunk data

        // uint64_t    current_chunk_bitfield[MAX_CLIENTS][4];//4 * 64 = 256
        // uint64_t    old_chunk_bitfield[MAX_CLIENTS][4];//4 * 64 = 256
        // char        current_chunk[MAX_CLIENTS][MAX_SLICES * MAX_UDP_SIZE];
        // char        old_chunk[MAX_CLIENTS][MAX_SLICES * MAX_UDP_SIZE];
        
        //to process incoming packets
        char        ordered_buffer          [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE][MAX_UDP_SIZE]; //temp stores newest packets
        packet_header  ordered_header       [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];
        uint64_t    ordered_bitfield        [MAX_CLIENTS];
        uint8_t     ordered_buffer_count    [MAX_CLIENTS];

        //stores the tick/modulo'd sequence of the packet. access it in the remote_received_history
        uint8_t latest_unprocessed_packet   [MAX_CLIENTS]; 
        uint8_t unprocessed_packet_count    [MAX_CLIENTS];

        uint8_t current_processed_packet    [MAX_CLIENTS];
        uint8_t oldest_processed_packet     [MAX_CLIENTS];


        packet_header local_header_history  [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //stores sent packets
        packet_header remote_header_history [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //stores received packets


        //queue of sequence numbers
        //each time we receive an ack, look up this entry and note the differrence in local time
        //between the time we receive the ack, and the time we sent the packet. this becomes RTT
        uint64_t in_flight_bitfield         [MAX_CLIENTS];
        uint16_t packet_resend_timeouts     [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //tracks each packet sent over the current window and checks if they've been acked/need resending
        uint16_t packet_resend_IDs          [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //id of the message we want to resend
        uint16_t in_flight_sequences        [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE]; //sequence of the message we want to resend
        uint16_t in_flight_packet_count     [MAX_CLIENTS];
        uint16_t index_to_tick              [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];
        uint16_t tick_to_index              [MAX_CLIENTS][SNAPSHOT_BUFFER_SIZE];

        uint32_t rtt                        [MAX_CLIENTS];

        uint8_t  client_tick                [MAX_CLIENTS];
        uint8_t  tick_offset                [MAX_CLIENTS];
        uint16_t local_sequence             [MAX_CLIENTS]; //increment every time we send a packet to that specific client
        uint16_t remote_sequence            [MAX_CLIENTS]; //sequence of sender

        //tracks when the client first connects and if it needs to sync. we set it and handle state in receive_packet(). need to understand thoroughly before using it more
        bool     client_synced              [MAX_CLIENTS];



        uint16_t sequences                  [MAX_CLIENTS];
        uint16_t ack                        [MAX_CLIENTS]; //latest acked packet
        uint64_t ack_bitfield               [MAX_CLIENTS]; //ring buffer, 0 or 1 per ack. goes ack - (bitPosition + 1) = sequence acknowledged  

        uint8_t local_tick                          ;
        
        uint32_t resent_count;
        uint32_t lost_count;
    };


    //when a packet is received, check the sequence number against the remote sequence number. if the packet sequence is more recent, it becomes new remote sequence


    struct ReliableUDP{
        uint16_t    next_sequence_out;
        uint16_t    oldest_sequence;
        uint16_t    last_ack_received;
        uint16_t    sent_sequences      [MAX_INFLIGHT];
        uint16_t    sequence_to_index   [MAX_SEQUENCE];
        uint64_t    sent_times          [MAX_INFLIGHT];
        bool        pending_acks        [MAX_INFLIGHT];
        int         pending_ack_count;
        
        int lost_packets;
        uint16_t last_sequence_in;
        uint16_t ack_bitfield;
    };

    struct ClientSockets {
        int tcp_socket;
        int udp_socket;
        uint16_t udp_port;
        socklen_t udp_port_len;
        sockaddr_in client_addr;
        sockaddr_in server_addr_tcp;
        sockaddr_in server_addr_udp;
        bool isConnected;
        bool connecting;
        bool startup;
        uint64_t connectStart;

        uint16_t token;
        uint64_t clientTime;
        uint64_t clientTimeOffset;
        uint32_t sequence;
        uint32_t tick;
        bool active;
    };

    struct ServerSockets {
        int tcp_socket;
        int udp_socket;
        bool active;

    };

    struct ClientManager{
        
        uint32_t    socketToID          [MAX_SOCKET_ID];

        sockaddr_in udp_addrs           [MAX_CLIENTS];  // Store full UDP address (IP+port)
        uint16_t    client_udp_ports    [MAX_CLIENTS];
        ReadBuffer  readBuffers         [MAX_CLIENTS];

        uint64_t last_ping_times        [MAX_CLIENTS];
        uint16_t last_sequences         [MAX_CLIENTS];
        uint16_t tokenToID              [UINT16_MAX+1];
        uint16_t IDToToken              [MAX_CLIENTS];

        uint64_t last_client_times      [MAX_CLIENTS];    // Their reported time
        uint64_t server_receive_times   [MAX_CLIENTS];  // When we got it
        uint64_t client_clock_offsets   [MAX_CLIENTS];  // Difference between our clocks
        int tick;
        int numClients;

        uint16_t last_unreliable_sequence    [MAX_CLIENTS];
        uint16_t last_reliable_sequence      [MAX_CLIENTS];

        uint8_t freeClientIDs[MAX_CLIENTS];
        uint8_t freeClientIDCount;

        uint8_t recentlyConnectedClients     [MAX_CLIENTS];
        uint8_t recentlyConnectedClientCount;

        //this will always be populated alongside the connectedClients, so check this first from game update
        uint8_t recentlyJoinedPlayers        [MAX_CLIENTS][MAX_LOCAL_PLAYERS];
        uint8_t recentlyJoinedPlayerCount;
        

    };



struct network_state{
    uint64_t currentTime;
    uint16_t totalClients;
    uint16_t local_sequence;
    uint32_t lostPackets;
    uint32_t receivedPackets;
    uint32_t sendPackets;
    bool enabled;
    PingTracker pingTracker;
    ClientSockets clientSockets;
    ServerSockets serverSockets;
    ClientManager clientManager;
    PacketManager packetManager;
    NetTest       netTest;
    debug_logger  logger;
    ConnectionState connectionState[MAX_CLIENTS];
    // ReliableUDP reliableUDP [MAX_CLIENTS];
    
    player_input playerInputs[MAX_CLIENTS][MAX_LOCAL_PLAYERS][SNAPSHOT_BUFFER_SIZE];

    // entity_state entityState;
    uint32_t sim_time;
    uint32_t tick;
    uint32_t currentTick;
    uint32_t timeCopied;
    uint32_t total_bandwidth_accumulator;
    uint32_t local_time_ms;

};


static void init_connection_state(network_state* NetworkState, ConnectionState& cs){
    // uint32_t now = GetTimeMS();
    uint32_t now = GetTimeMS()/*REPLACE*/;
    cs.bandwidth_accumulator = TARGET_BANDWIDTH;

    for (int i = 0; i < SNAPSHOT_BUFFER_SIZE; i++)
    {
        cs.in_flight_sequences  [i] = -1;
        cs.packed_sequences     [i] = -1;
        cs.received_sequences   [i] = -1;
    }

    for (int i = 0; i < ID_BUFFER_SIZE; i++)
    {
        cs.received_ids[i] = -1;
        //MAX_SEND ATTEMPTS IS 4!!
        for(int j = 0; j < MAX_SEND_ATTEMPTS; j++){
            cs.id_to_sequence[i][j] = -1;
        }

    }
    
    
    memset(cs.id_in_flight_count, 0, sizeof(int)    * ID_BUFFER_SIZE);
    memset(cs.id_send_attempts, 0, sizeof(int)      * ID_BUFFER_SIZE);
    memset(cs.packet_resend_timeouts, 0, sizeof(uint32_t) * SNAPSHOT_BUFFER_SIZE);
    cs.latency = 64;
    cs.SRTT = 100;//100ms as a conservative starting estimate
    cs.debug_log_timer = cs.latency * SNAPSHOT_BUFFER_SIZE/*  * 1000 */; //how frequently we log the state of the connection. * 10 just because we're simulating the time, make it more sparse
    cs.last_log_time = now;
    // cs.latency = 2000;
    cs.remote_sequence = UINT16_MAX;
    cs.latest_in_flight_sequence = UINT16_MAX;
    cs.ack = UINT16_MAX;

    cs.packet_loss_percentage = 10;
    cs.jitter_amount = 10;

    //chunk/slice logic
    cs.ready_to_receive_chunk = true;
    cs.last_update = now;
    cs.last_log_time = now; 
    
    cs.target_bandwidth = TARGET_BANDWIDTH;
    cs.local_time_ms = now;


}

static void debug_logger_open_file(debug_logger* logger, const char* filename, bool use_console = true, LogTypes lowest_printed_type = LogTypes::Trace){
    logger->log_file = fopen(filename, "w"); //"a" to only append
    logger->console_output = use_console;
    logger->lowest_printed_type = lowest_printed_type;
}

static void debug_logger_close_file(debug_logger* logger){
    
    if(logger->log_file){
        fclose(logger->log_file);
    }
}

static void log_to_console(network_state* NetworkState, LogTypes logLevel, const char* format, ...){
    debug_logger* logger = &NetworkState->logger;
    if(logLevel < logger->lowest_printed_type)return;


    const char* level = "NONE";
    const char* color = RESET;
    switch (logLevel){
        case Trace:
        level = "TRACE";
        color = RESET;
        break;
        
        case Debug:
        level = "DEBUG";
        color = color = RESET;
        break;
        
        case Info:
        level = "INFO";
        color = GREEN;
        break;
        
        case Warn:
        level = "WARN";
        color = YELLOW;
        break;
        
        case Error:
        level = "ERROR";
        color = RED;
        break;
    }
    // if(logger->log_file){
    //     fprintf(logger->log_console, "test\n");
    //     fflush(logger->log_console);
    // }

    // if(logger->console_output){
    //     printf("test\n");
    // }

    va_list args;
    va_start(args, format);

    if(logger->console_output){

            // Print the header with timestamp to the console
        printf("%s[%s][%-5s]%s ", color, get_timestamp(), level, RESET);
        // printf("%s[%s][%-5s] ", color, get_timestamp(), level);

        // Handle the variadic arguments
        
        // Print the formatted message to the console
        vprintf(format, args);
        // printf("%s", RESET);

        // End the console output with a newline
    printf("\n");
    }

    // Also write the log to a file if needed
    // if (logger && logger->log_file) {
    
    // /*
    //     TOO CHECK WHEN TO CREATE A NEW FILE BECAUSE THIS CURRENT ONE IS TOO LARGE
    //         fseek(logger->log_file, 0, SEEK_END);
    //     long size = ftell(logger->log_file);
        
    //     // If over 100MB, start new file
    //     if(size > 100 * 1024 * 1024) {
    //         //close file
    //         //open new file
    //         //set a flag to tell this code to write to the new file now
    //     */


    //     fprintf(logger->log_file, "[%s][%s] ", get_timestamp(), level);
    //     va_list args_copy;
    //     va_copy(args_copy, args); // Copy the va_list to use with vfprintf
    //     vfprintf(logger->log_file, format, args_copy);
    //     va_end(args_copy);
    //     fprintf(logger->log_file, "\n"); // Add a newline
    //     fflush(logger->log_file); // Ensure the log is written to the file
    // }

    va_end(args);
    

}


static void log_console(debug_logger* logger, LogTypes logLevel, const char* format, ...){
    if(logLevel < logger->lowest_printed_type)return;


    const char* level = "NONE";
    const char* color = RESET;
    switch (logLevel){
        case Trace:
        level = "TRACE";
        color = RESET;
        break;
        
        case Debug:
        level = "DEBUG";
        color = color = RESET;
        break;
        
        case Info:
        level = "INFO";
        color = GREEN;
        break;
        
        case Warn:
        level = "WARN";
        color = YELLOW;
        break;
        
        case Error:
        level = "ERROR";
        color = RED;
        break;
    }
    // if(logger->log_file){
    //     fprintf(logger->log_file, "test\n");
    //     fflush(logger->log_file);
    // }

    // if(logger->console_output){
    //     printf("test\n");
    // }

    va_list args;
    va_start(args, format);

    if(logger->console_output){

            // Print the header with timestamp to the console
        printf("%s[%s][%-5s]%s ", color, get_timeWithMilliseconds(), level, RESET);
        // printf("%s[%s][%-5s] ", color, get_timestamp(), level);

        // Handle the variadic arguments
        
        // Print the formatted message to the console
        vprintf(format, args);
        // printf("%s", RESET);

        // End the console output with a newline
    printf("\n");
    }

    // Also write the log to a file if needed
    // if (logger && logger->log_file) {
    
    // /*
    //     TOO CHECK WHEN TO CREATE A NEW FILE BECAUSE THIS CURRENT ONE IS TOO LARGE
    //         fseek(logger->log_file, 0, SEEK_END);
    //     long size = ftell(logger->log_file);
        
    //     // If over 100MB, start new file
    //     if(size > 100 * 1024 * 1024) {
    //         //close file
    //         //open new file
    //         //set a flag to tell this code to write to the new file now
    //     */


    //     fprintf(logger->log_file, "[%s][%s] ", get_timestamp(), level);
    //     va_list args_copy;
    //     va_copy(args_copy, args); // Copy the va_list to use with vfprintf
    //     vfprintf(logger->log_file, format, args_copy);
    //     va_end(args_copy);
    //     fprintf(logger->log_file, "\n"); // Add a newline
    //     fflush(logger->log_file); // Ensure the log is written to the file
    // }

    va_end(args);
    

}



static void log_file(debug_logger* logger, LogTypes logLevel, const char* format, ...){
    if(logLevel < logger->lowest_printed_type)return;


    const char* level = "NONE";
    const char* color = RESET;
    switch (logLevel){
        case Trace:
        level = "TRACE";
        color = RESET;
        break;
        
        case Debug:
        level = "DEBUG";
        color = color = RESET;
        break;
        
        case Info:
        level = "INFO";
        color = GREEN;
        break;
        
        case Warn:
        level = "WARN";
        color = YELLOW;
        break;
        
        case Error:
        level = "ERROR";
        color = RED;
        break;
    }
    // if(logger->log_console){
    //     fprintf(logger->log_console, "test\n");
    //     fflush(logger->log_console);
    // }

    // if(logger->console_output){
    //     printf("test\n");
    // }

    va_list args;
    va_start(args, format);

    // Also write the log to a file if needed
    if (logger && logger->log_file) {
    
    /*
        TOO CHECK WHEN TO CREATE A NEW FILE BECAUSE THIS CURRENT ONE IS TOO LARGE
            fseek(logger->log_file, 0, SEEK_END);
        long size = ftell(logger->log_file);
        
        // If over 100MB, start new file
        if(size > 100 * 1024 * 1024) {
            //close file
            //open new file
            //set a flag to tell this code to write to the new file now
        */


        // fprintf(logger->log_file, "[%s][%s] ", get_timestamp(), level);
        va_list args_copy;
        va_copy(args_copy, args); // Copy the va_list to use with vfprintf
        vfprintf(logger->log_file, format, args_copy);
        va_end(args_copy);
        fprintf(logger->log_file, "\n"); // Add a newline
        fflush(logger->log_file); // Ensure the log is written to the file
    }

    va_end(args);
    

}


static void log_inline(network_state* NetworkState, LogTypes logLevel, const char* format, ...){
    debug_logger* logger = &NetworkState->logger;
    if(logLevel < logger->lowest_printed_type)return;


    const char* level = "NONE";
    const char* color = RESET;
    switch (logLevel){
        case Trace:
        level = "TRACE";
        color = RESET;
        break;
        
        case Debug:
        level = "DEBUG";
        color = color = RESET;
        break;
        
        case Info:
        level = "INFO";
        color = GREEN;
        break;
        
        case Warn:
        level = "WARN";
        color = YELLOW;
        break;
        
        case Error:
        level = "ERROR";
        color = RED;
        break;
    }
    // if(logger->log_file){
    //     fprintf(logger->log_file, "test\n");
    //     fflush(logger->log_file);
    // }

    // if(logger->console_output){
    //     printf("test\n");
    // }

    va_list args;
    va_start(args, format);

    if(logger->console_output){

            // Print the header with timestamp to the console
        // printf("%s[%s][%-5s]%s ", color, get_timestamp(), level, RESET);
        // printf("%s[%s][%-5s] ", color, get_timestamp(), level);

        // Handle the variadic arguments
        
        // Print the formatted message to the console
        vprintf(format, args);
        // printf("%s", RESET);

        // End the console output with a newline
    // printf("\n");
    }

    // Also write the log to a file if needed
    // if (logger && logger->log_file) {
    
    // /*
    //     TOO CHECK WHEN TO CREATE A NEW FILE BECAUSE THIS CURRENT ONE IS TOO LARGE
    //         fseek(logger->log_file, 0, SEEK_END);
    //     long size = ftell(logger->log_file);
        
    //     // If over 100MB, start new file
    //     if(size > 100 * 1024 * 1024) {
    //         //close file
    //         //open new file
    //         //set a flag to tell this code to write to the new file now
    //     */


    //     // fprintf(logger->log_file, "[%s][%s] ", get_timestamp(), level);
    //     va_list args_copy;
    //     va_copy(args_copy, args); // Copy the va_list to use with vfprintf
    //     vfprintf(logger->log_file, format, args_copy);
    //     va_end(args_copy);
    //     // fprintf(logger->log_file, "\n"); // Add a newline
    //     fflush(logger->log_file); // Ensure the log is written to the file
    // }

    va_end(args);
    

}

#define LOG_TRACE(NetworkState, format, ...) log_to_console(NetworkState, Trace, format, ##__VA_ARGS__)//## tells the preprocessor to remove the comma before VA ARGS when VA ARGS is empty
#define LOG_DEBUG(NetworkState, format, ...) log_to_console(NetworkState, Debug, format, ##__VA_ARGS__)
#define  LOG_INFO(NetworkState, format, ...) log_to_console(NetworkState, Info,  format, ##__VA_ARGS__)
#define  LOG_WARN(NetworkState, format, ...) log_to_console(NetworkState, Warn,  format, ##__VA_ARGS__)
#define LOG_ERROR(NetworkState, format, ...) log_to_console(NetworkState, Error, format, ##__VA_ARGS__)

#define  LOGLINE_TRACE(NetworkState, format, ...) log_inline(NetworkState, Trace, format, ##__VA_ARGS__)
#define  LOGLINE_INFO(NetworkState, format, ...)  log_inline(NetworkState, Info,  format, ##__VA_ARGS__)
#define  LOGLINE_DEBUG(NetworkState, format, ...) log_inline(NetworkState, Debug, format, ##__VA_ARGS__)
#define  LOGLINE_ERROR(NetworkState, format, ...) log_inline(NetworkState, Error, format, ##__VA_ARGS__)
#define  LOGLINE_WARN(NetworkState, format, ...)  log_inline(NetworkState, Warn,  format, ##__VA_ARGS__)

#define LOG_SERVER(NetworkState, Type, format, ...) log_to_console(NetworkState, Type, BBLUE  "[SERVER] " RESET format, ##__VA_ARGS__)
#define LOG_CLIENT(NetworkState, Type, format, ...) log_to_console(NetworkState, Type, BGREEN "[CLIENT] " RESET format, ##__VA_ARGS__)

static void log_connection_state(network_state* NetworkState, ConnectionState& cs, int clientID = 0){
    // uint32_t now = GetTimeMS();
    uint32_t now = GetTimeMS()/*REPLACE*/;

    if(now - cs.last_log_time > cs.debug_log_timer){
        const char* connection = SERVER_STR;
        cs.last_log_time = now;
        float bandwidth_consumption = (1.0f - ((float)cs.bandwidth_accumulator / (float)TARGET_BANDWIDTH)) * 100;
        float estimated_drop_rate = (float)cs.sent_packets /  (float)cs.resend_count;
        if(!cs.is_server){

            connection = CLIENT_STR;
            cs.connection_duration += (now - cs.last_update);
            cs.last_update = now; 
            // LOG_INFO(NetworkState, "%s SRTT: %dms, needs resync: %d, bandwidth use: %f, sent packets: %d, resent packets: %d, dropped packets: %d, overwritten packets: %d, connection duration: %"UINT64_FORMAT"ms, token: %d, lost count: %d", connection, cs.SRTT, cs.need_resync, bandwidth_consumption, cs.sent_packets, cs.resend_count, cs.dropped_packets, cs.overwritten_packets, cs.connection_duration, cs.token, cs.lost_count);    
            LOG_INFO(NetworkState, "%s SRTT: %dms, needs resync: %d, bandwidth use: %.3f%%, sent packets: %d, resent packets: %d, dropped packets: %d, overwritten packets: %d, connection duration: %" UINT64_FORMAT "ms, token: %d, lost count: %d estimated drop rate: %.2f%%", connection, cs.SRTT, cs.need_resync, bandwidth_consumption, cs.sent_packets, cs.resend_count, cs.dropped_packets, cs.overwritten_packets, cs.connection_duration, cs.token, cs.lost_count, estimated_drop_rate);    

        }else{
            float bandwidth_consumption = (1.0f - ((float)NetworkState->total_bandwidth_accumulator / (float)TARGET_BANDWIDTH)) * 100;
            // LOG_INFO(NetworkState, "%s SRTT: %dms, needs resync: %d, bandwidth use: %f, sent packets: %d, resent packets: %d, dropped packets: %d, overwritten packets: %d, clientID: %d, token: %d, lost count: %d times desynced: %d, times resynced: %d", connection, cs.SRTT, cs.need_resync, bandwidth_consumption, cs.sent_packets, cs.resend_count, cs.dropped_packets, cs.overwritten_packets, clientID, cs.token, cs.lost_count, cs.times_desynced, cs.times_resynced);    
            LOG_INFO(NetworkState, "%s SRTT: %dms, needs resync: %d, bandwidth use: %.3f%%, sent packets: %d, resent packets: %d, dropped packets: %d, overwritten packets: %d, clientID: %d, token: %d, lost count: %d times desynced: %d, times resynced: %d, estimated drop rate: %.2f%%", connection, cs.SRTT, cs.need_resync, bandwidth_consumption, cs.sent_packets, cs.resend_count, cs.dropped_packets, cs.overwritten_packets, clientID, cs.token, cs.lost_count, cs.times_desynced, cs.times_resynced, estimated_drop_rate);    
        }
    }



}


struct time_sync_request {
    uint64_t client_send_time;    // Client's local time when sent
};

struct time_sync_response {
    uint64_t client_send_time;    // Echo back their send time
    uint64_t server_receive_time; // When server got the request
    uint64_t server_send_time;    // When server sent response
};

struct time_sync_final {
    uint64_t client_send_time;    // Original send time
    uint64_t server_receive_time; // When server got request
    uint64_t server_send_time;    // When server sent response
    uint64_t client_receive_time; // When client got response
};





#if 0



static void trackReliableUDP(network_state* NetworkState, uint16_t clientID, uint16_t sequence, uint64_t time){
    ReliableUDP* reliableUDP = &NetworkState->reliableUDP[clientID];

    uint16_t index = reliableUDP->pending_ack_count;
    if(index >= MAX_INFLIGHT){
        printf("TOO MANY RELIABLE UDPs IN FLIGHT, CAN'T APPEND NEW\n");
        return;
    }
    if((reliableUDP->sent_sequences[index] == sequence)){
        printf("Sequnce: %d ALREADY TRACKED??\n", sequence);
        return;
    }

    reliableUDP->sent_sequences[index] = sequence;
    reliableUDP->sequence_to_index[sequence] = index;
    reliableUDP->sent_times[index] = time;
    reliableUDP->pending_acks[index] = true;

    reliableUDP->pending_ack_count++;
}

static void resolveReliableUDP(network_state* NetworkState, uint16_t clientID, uint16_t sequence, uint64_t time){
    ReliableUDP* reliableUDP = &NetworkState->reliableUDP[clientID];
    
    if(reliableUDP->pending_ack_count <= 0){
        printf("NO PACKETS IN FLIGHT?? ERROR??\n");
        return;
    }

    uint16_t index = reliableUDP->sequence_to_index[sequence];

    if(index < 0 || index >= MAX_INFLIGHT){
        printf("resolveReliableUPD() invalid index: %d??\n", index);
        return;
    }
    
    uint16_t lastIndex = reliableUDP->pending_ack_count - 1;

    if(lastIndex != index && lastIndex != 0){
        uint16_t removedIndex = index;

        uint16_t swappedSequence = reliableUDP->sent_sequences[lastIndex];
        reliableUDP->sequence_to_index[swappedSequence] = index;

        reliableUDP->sent_sequences[removedIndex] = reliableUDP->sent_sequences[lastIndex];
        reliableUDP->sent_times[removedIndex] = reliableUDP->sent_times[lastIndex];
        reliableUDP->pending_acks[removedIndex] = reliableUDP->pending_acks[lastIndex];
    }

    reliableUDP->sequence_to_index[sequence] = 0;
    reliableUDP->sent_sequences[lastIndex] = 0;
    reliableUDP->sent_times[lastIndex] = 0;
    reliableUDP->pending_acks[lastIndex] = false;
    
    reliableUDP->pending_ack_count--;

}


static void 
printTimeUnits(uint32_t timestamp) {
    uint32_t ms = timestamp % 1000;
    uint32_t seconds = (timestamp / 1000) % 60;
    uint32_t minutes = (timestamp / 60000) % 60;
    uint32_t hours = timestamp / 3600000;
    
    printf("%02u:%02u:%02u.%03u", hours, minutes, seconds, ms);
}




void SendTimeSyncRequest(void) {
    time_sync_request req;
    req.client_send_time = GetLocalTimeMS();
    SendToServer(&req, sizeof(req));
}



void HandleTimeSyncRequest(Client* client, time_sync_request* req) {
    time_sync_response resp;
    resp.client_send_time = req->client_send_time;
    resp.server_receive_time = GetServerTimeMS();
    resp.server_send_time = GetServerTimeMS();
    SendToClient(client, &resp, sizeof(resp));
}


void HandleTimeSyncResponse(time_sync_response* resp) {
    uint64_t now = GetLocalTimeMS();
    
    // Round trip time (RTT)
    uint64_t rtt = now - resp->client_send_time;
    
    // Estimated one-way latency (half RTT)
    uint64_t latency = rtt / 2;
    
    // Calculate server/client time difference
    // Assume packet took same time both ways
    uint64_t server_time_when_we_got_response = 
        resp->server_send_time + latency;
    
    // Calculate how far ahead/behind server we are
    int64_t time_offset = 
        server_time_when_we_got_response - now;
    
    // Store this offset to convert local time to server time
    client_time_offset = time_offset;
}

// Now we can always get server time
uint64_t GetEstimatedServerTime(void) {
    return GetLocalTimeMS() + client_time_offset;
}


void VerifyClientTimeSync(Client* client, uint64_t client_time) {
    uint64_t server_time = GetServerTimeMS();
    int64_t difference = abs(server_time - client_time);
    
    if (difference > MAX_TIME_DRIFT) {
        // Client has drifted too far, force a resync
        SendTimeSyncRequest(client);
    }
}
#endif