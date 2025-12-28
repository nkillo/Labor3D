




#if 0
//shitty stack based approach
    
#define PUSH_STACK() \
offset += box.size;\
if(box.size == 0)offset += 8;\
if(offset >= size)break;\
box = parse_box(data, offset);\
stack[stack_size++] = box;    \

    Box stack[256] = {};
    u32 stack_size = 0;
    box = parse_box(data, offset);
    // offset += 8;

    stack[stack_size++] = box;
    
    while(stack_size && offset < size){
        //pop box
        stack_size--;
        box = stack[stack_size];  
        printf("stack size: %u\n", stack_size);
        printf("box size  : %u\n", box.size);
        printf("box name  :"); print_4byte(box.name_val);

        switch (box.name_val){

            case FOURCC('f', 't', 'y', 'p'):{
                printf("FTYP FOUND!\n");
                FileTypeBox ftyp = parse_ftyp(data, offset+8, box); 
                printf("major brand: "); print_4byte(ftyp.major_brand);
                printf("minor version: %u\n", ftyp.minor_version);    // print_4byte(ftyp.minor_version);
                PUSH_STACK();

            }break;
            case FOURCC('m', 'o', 'o', 'v'):{
                printf("moov box found\n");
                // offset += box.size;
                offset += 8;
                if(offset >= size)break;
                box = parse_box(data, offset);
                stack[stack_size++] = box;   
            }break;
            case FOURCC('m', 'v', 'h', 'd'):{
                printf("found mvhd box\n");
                // mvhdBox mvhd = parse_mvhd(data, offset + 8, subbox, depth+1);
                offset += box.size;
                if(offset >= size)break;
                box = parse_box(data, offset);
                stack[stack_size++] = box;  
            }break;
            case FOURCC('t', 'r', 'a', 'k'):{
                printf("found trak box\n");
                // parse_trak(data, offset + 8, subbox, depth+1);
                offset += box.size;
                if(offset >= size)break;
                box = parse_box(data, offset);
                stack[stack_size++] = box;  
            }break;
            case FOURCC('u', 'd', 't', 'a'):{ //user data atom
                printf("found udta box\n");
                // parse_udta(data, offset + 8, subbox, depth+1);
                // parse_trak(data, offset + 8);
                offset += box.size;
                if(offset >= size)break;
                box = parse_box(data, offset);
                stack[stack_size++] = box;  
            }break;
            case FOURCC('f', 'r', 'e', 'e'):{ 
                offset += box.size;
                if(offset >= size)break;
                box = parse_box(data, offset);
                stack[stack_size++] = box;  
            }break;
            default:{
                offset += box.size;
                if(box.size == 0)offset += 8;
                if(offset >= size)break;
                box = parse_box(data, offset);
                stack[stack_size++] = box;   
                printf("unhandled case!\n");
                __debugbreak();
            }

        }
    }

#endif










#define FOURCC(a, b, c, d) (((u32)(a) << 24) | ((u32)(b) << 16) | ((u32)(c) << 8) | ((u32)(d)))

struct Box{
    u32 size;
    char* name;
    u32 name_val;
    size_t offset_at_parse;
};

struct FileTypeBox{
    Box box;
    u32 major_brand;
    u32 minor_version;
    u32 compatible_brands[32];

};

struct mvhdBox{
    Box box;
    u8 version;
    u32 flags;
    u32 creationTime;
    u32 modificationTime;
    u32 timeScale;
    u32 duration;
    u32 FIXED32_rate;
    u16 FIXED16_volume;


    u32 window_geometry_a;
    u32 window_geometry_b;
    u32 window_geometry_u;
    u32 window_geometry_c;
    u32 window_geometry_d;
    u32 window_geometry_v;
    u32 window_geometry_x;
    u32 window_geometry_y;
    u32 window_geometry_w;
    // a b u
    // c d v
    // x y w


    u32 preview_time       ;
    u32 preview_duration   ;
    u32 poster_time        ;
    u32 selection_time     ;
    u32 selection_duration ;
    u32 current_time       ;
    u32 next_track_id      ;
};



struct mdhdBox{
    Box box;
    u8 version;
    u32 flags;
    u32 creationTime;
    u32 modificationTime;
    u32 timeScale;
    u32 duration;
    u16 language;
    u16 quality;
};




struct tkhdBox{
    Box box;
    u8 version;             //1 bytes
    u32 flags;              //3 bytes

    u32 creationTime;       //4 bytes
    u32 modificationTime;   //4 bytes
    u32 trackID;            //4 bytes
    u32 reserved1;          //4 bytes
    u32 duration;           //4 bytes
    
    u64 reserved2;          //8 bytes
    
    u16 layer;              //2 bytes
    u16 alternateGroup;     //2 bytes
    u16 FIXED16_volume;             //2 bytes
    u16 reserved;           //2 bytes

    u32 window_geometry_a;  //36 bytes
    u32 window_geometry_b;
    u32 window_geometry_u;
    u32 window_geometry_c;
    u32 window_geometry_d;
    u32 window_geometry_v;
    u32 window_geometry_x;
    u32 window_geometry_y;
    u32 window_geometry_w;
    // a b u
    // c d v
    // x y w

    u32 FIXED32_trackWidth;         //4 bytes
    u32 FIXED32_trackHeight;        //4 bytes
};


struct moovBox{
    Box box;
    mvhdBox* mvhd;
};



typedef struct {
    const u8* data;   // pointer to RBSP buffer
    u32 size;         // total size in bytes
    u32 byte_pos;     // current byte offset
    u8 bit_pos;       // bit position in current byte (0 = MSB, 7 = LSB)
} BitReader;

void bitreader_init(BitReader* br, const u8* data, u32 size) {
    br->data = data;
    br->size = size;
    br->byte_pos = 0;
    br->bit_pos = 0;
}

// Read a single bit
u8 read_bit(BitReader* br) {
    if (br->byte_pos >= br->size) return 0;
    u8 byte = br->data[br->byte_pos];
    u8 bit = (byte >> (7 - br->bit_pos)) & 0x01;
    br->bit_pos++;
    if (br->bit_pos == 8) {
        br->bit_pos = 0;
        br->byte_pos++;
    }
    return bit;
}

// Read n bits as an unsigned integer
u32 read_bits(BitReader* br, u32 n) {
    u32 result = 0;
    for (u32 i = 0; i < n; i++) {
        result = (result << 1) | read_bit(br);
    }
    return result;
}

// Read unsigned Exp-Golomb code (ue(v))
u32 read_ue(BitReader* br) {
    u32 zeros = 0;
    while (read_bit(br) == 0 && br->byte_pos < br->size) {
        zeros++;
    }
    if (zeros == 0) return 0;
    u32 value = (1 << zeros) - 1 + read_bits(br, zeros);
    return value;
}

// Read signed Exp-Golomb code (se(v))
int read_se(BitReader* br) {
    u32 ue_val = read_ue(br);
    int signed_val = (ue_val & 1) ? ((ue_val + 1) / 2) : -(int)(ue_val / 2);
    return signed_val;
}

//big endian
u32 read_u32_be(const u8* data){
    return (((u32)data[0] << 24) |
            ((u32)data[1] << 16) | 
            ((u32)data[2] << 8 ) |
            ((u32)data[3] << 0 ));
}

//big endian
u16 read_u16_be(const u8* data){
    return (((u32)data[0] << 8) |
            ((u32)data[1] << 0));
}


//big endian
u8 read_u8_be(const u8* data){
    return (((u32)data[0] << 0));
}


//little endian
u32 read_u32_le(const u8* data){
    return (((u32)data[0] << 0) |
            ((u32)data[1] << 8) | 
            ((u32)data[2] << 16)|
            ((u32)data[3] << 24));
}


u64 read_u64_be(const u8* data){
    return (((u64)data[0] << 56) |
            ((u64)data[1] << 48) | 
            ((u64)data[2] << 40) |
            ((u64)data[3] << 32) |
            ((u64)data[4] << 24) |
            ((u64)data[5] << 16) |
            ((u64)data[6] << 8 ) |
            ((u64)data[7] << 0 ));
}


void print_hex(const u8* data, size_t offset, size_t length){
    data = data + offset;    
    for(size_t i = 0; i < length; i+= 16){
        printf("%08zx | ", offset+ i);
        for(int j = 0; j < 16; j++){
            if(i + j < length)
                printf("%02x ", data[i + j]);
            else
                printf("    ");
        }
        printf(" ");
        for(int j = 0; j < 16; ++j){
            if(i + j < length){
                char c = data[i + j];
                printf("%c",( c >= 32 && c <= 126) ? c : '.');
            }
        }
        printf("\n");
    }
}

void parse_boxes(u8* data, size_t size){
    size_t offset = 0;
    while(offset + 8 <= size){
        u32 box_size = read_u32_be(data + offset);
        char* type = (char*)data + (offset + 4);
        printf("Found box '%c%c%c%c' at offset %zu, size %u\n",
            type[0], type[1], type[2], type[3],
            offset, box_size);

        if (box_size == 0) break; // end of file

        if(box_size == 1){
            printf("box size 1? read 64 bit size?\n");
        }else if (box_size < 8) break;  // invalid box

        offset += box_size;
    }
}

Box parse_box(u8* data, size_t offset/* , char* name_buffer, size_t name_buffer_size */){
    Box box = {};
    box.size = read_u32_be(data + offset);
    char* type = (char*)data + (offset + 4);
    box.name_val = read_u32_be(data + (offset + 4));
    box.name = type;
    box.offset_at_parse = offset;
    return box;
}

void print_4byte(u32 val){
    printf("%c%c%c%c\n", (char)(val >> 24), (char)(val >> 16), (char)(val >> 8), (char)(val >> 0));
}

void print_2byte(u16 val){
    printf("%c%c\n",  (char)(val >> 8), (char)(val >> 0));
}

void print_byte(u8 val){
    printf("%c\n", (char)(val >> 0));
}

FileTypeBox parse_ftyp(u8* data, size_t offset, Box base, int depth){
    FileTypeBox ftyp = {};
    ftyp.major_brand = read_u32_be(data + (offset));
    ftyp.minor_version = read_u32_be(data + (offset + 4));
    int brand = 0;
    //- 16 for size, name from box read, major_brand and minor_version from box type read
    for(size_t i = 0; i < (base.size - 16); i+=4){
        ftyp.compatible_brands[brand] = read_u32_be(data + (offset + 8 + i));
        printf("compatible brand %d: ", brand); print_4byte(ftyp.compatible_brands[brand]);
        brand++;
    }
    return ftyp;
}

#define CREATE_INDENT() do{\
    for(int i = 0; i < depth; i++){\
        indent[i] = '\t';\
    }\
    indent[depth] = 0;\
}while(0)\

void print_fixed32(u32 fixed32){
    printf("%d.%d ", fixed32 >> 16, (u16)fixed32);
}

void parse_tkhd(uint8_t* data, size_t offset, Box box, int depth = 0){
    size_t start = offset - 8;//to account for size and name of box already read
    char indent[64] = {};
    CREATE_INDENT();
    tkhdBox tkhd = {};
    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 

    tkhd.version            = 0x00FFFFFF & version_and_flags >> 0;
    tkhd.flags              =     (0xFF & version_and_flags >> 24);
    tkhd.creationTime       = read_u32_be(data + offset); offset+=4;
    tkhd.modificationTime   = read_u32_be(data + offset); offset+=4;
    tkhd.trackID            = read_u32_be(data + offset); offset+=4;           
    tkhd.reserved1          = read_u32_be(data + offset); offset+=4;
    tkhd.duration           = read_u32_be(data + offset); offset+=4;         
    
    tkhd.reserved2          = read_u64_be(data + offset); offset+=8;        
    
    tkhd.layer              = read_u16_be(data + offset); offset+=2;
    tkhd.alternateGroup     = read_u16_be(data + offset); offset+=2;
    tkhd.FIXED16_volume     = read_u16_be(data + offset); offset+=2;
    tkhd.reserved           = read_u16_be(data + offset); offset+=2;
    tkhd.window_geometry_a  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_b  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_u  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_c  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_d  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_v  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_x  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_y  = read_u32_be(data + offset); offset+=4;
    tkhd.window_geometry_w  = read_u32_be(data + offset); offset+=4;
    // a b u
    // c d v
    // x y w
    tkhd.FIXED32_trackWidth         = read_u32_be(data + offset); offset+=4;
    tkhd.FIXED32_trackHeight        = read_u32_be(data + offset); offset+=4;
        
    printf("%stkhd.version           : %x\n",    indent,         tkhd.version          );
    printf("%stkhd.flags             : %x\n",    indent,         tkhd.flags            );
    printf("%stkhd.creationTime      : %x\n",    indent,         tkhd.creationTime     );
    printf("%stkhd.modificationTime  : %x\n",    indent,         tkhd.modificationTime );
    printf("%stkhd.trackID           : %x\n",    indent,         tkhd.trackID          );
    printf("%stkhd.reserved1         : %x\n",    indent,         tkhd.reserved1        );
    printf("%stkhd.duration          : %x\n",    indent,         tkhd.duration         );
    printf("%stkhd.reserved2         : %llx\n",  indent,         tkhd.reserved2        );
    printf("%stkhd.layer             : %x\n",    indent,         tkhd.layer            );
    printf("%stkhd.alternateGroup    : %x\n",    indent,         tkhd.alternateGroup   );
    printf("%stkhd.volume            : %d.%d\n", indent,   (int)(tkhd.FIXED16_volume >> 8), 0x000000FF & tkhd.FIXED16_volume);

    printf("%stkhd.reserved          : %x\n",   indent,          tkhd.reserved         );
    printf("%swindow geometry A      : %d.%d\n",indent, (int)(tkhd.window_geometry_a >> 16), 0x0000FFFF & tkhd.window_geometry_a);
    printf("%swindow geometry B      : %d.%d\n",indent, (int)(tkhd.window_geometry_b >> 16), 0x0000FFFF & tkhd.window_geometry_b);
    printf("%swindow geometry U      : %d.%d\n",indent, (int)(tkhd.window_geometry_u >> 16), 0x0000FFFF & tkhd.window_geometry_u);
    printf("%swindow geometry C      : %d.%d\n",indent, (int)(tkhd.window_geometry_c >> 16), 0x0000FFFF & tkhd.window_geometry_c);
    printf("%swindow geometry D      : %d.%d\n",indent, (int)(tkhd.window_geometry_d >> 16), 0x0000FFFF & tkhd.window_geometry_d);
    printf("%swindow geometry V      : %d.%d\n",indent, (int)(tkhd.window_geometry_v >> 16), 0x0000FFFF & tkhd.window_geometry_v);
    printf("%swindow geometry X      : %d.%d\n",indent, (int)(tkhd.window_geometry_x >> 16), 0x0000FFFF & tkhd.window_geometry_x);
    printf("%swindow geometry Y      : %d.%d\n",indent, (int)(tkhd.window_geometry_y >> 16), 0x0000FFFF & tkhd.window_geometry_y);
    printf("%swindow geometry W      : %d.%d\n",indent, (int)(tkhd.window_geometry_w >> 16), 0x0000FFFF & tkhd.window_geometry_w);
    printf("%strack width            : %d.%d\n",indent, (int)(tkhd.FIXED32_trackWidth  >> 16), 0x0000FFFF & tkhd.FIXED32_trackWidth );
    printf("%strack height           : %d.%d\n",indent, (int)(tkhd.FIXED32_trackHeight >> 16), 0x0000FFFF & tkhd.FIXED32_trackHeight);
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    int fuck_the_debugger = 0;

}



mvhdBox parse_mvhd(uint8_t* data, size_t offset, Box box, int depth = 0){
    size_t start = offset - 8;
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    mvhdBox mvhd = {};
    u32 flags_and_version = read_u32_be(data + (offset));   offset+=4;
    mvhd.creationTime =     read_u32_be(data + (offset));   offset+=4;
    mvhd.modificationTime = read_u32_be(data + (offset));   offset+=4;
    mvhd.timeScale =        read_u32_be(data + (offset));   offset+=4;
    mvhd.duration =         read_u32_be(data + (offset));   offset+=4;
    mvhd.FIXED32_rate =     read_u32_be(data + (offset));   offset+=4;
    mvhd.FIXED16_volume =   read_u16_be(data + (offset));   offset+=2;
    mvhd.flags =           0x00FFFFFF & (flags_and_version);
    mvhd.version =          (flags_and_version >> 24);
    printf("%smvhd data:\n",indent);
    printf("%sflags              : %x\n",indent, mvhd.flags              ); //print_4byte(mvhd.flags              );
    printf("%sversion            : %x\n",indent, mvhd.version            ); //print_byte (mvhd.version            );
    printf("%screationTime       : %x\n",indent, mvhd.creationTime       ); //print_4byte(mvhd.creationTime       );
    printf("%smodificationTime   : %x\n",indent, mvhd.modificationTime   ); //print_4byte(mvhd.modificationTime   );
    printf("%stimeScale          : %x\n",indent, mvhd.timeScale          ); //print_4byte(mvhd.timeScale          );
    printf("%sduration           : %x\n",indent, mvhd.duration           ); //print_4byte(mvhd.duration           );
    printf("%sFIXED32_rate       : %x\n",indent, mvhd.FIXED32_rate       ); //print_4byte(mvhd.FIXED32_rate       );
    printf("%sFIXED16_volume     : %x\n",indent, mvhd.FIXED16_volume     ); //print_2byte(mvhd.FIXED16_volume     );
    if(mvhd.flags == 1){
        printf("%sFLAG IS 1! OUR INTERPRETATION IS WRONG!!!\n",indent);
        __debugbreak();
    }
    u16 top_rate = (mvhd.FIXED32_rate >> 16);
    u16 bottom_rate = (mvhd.FIXED32_rate);
    printf("%stop rate: %d, bottom rate: %d\n",indent, top_rate, bottom_rate);
    u8 top_volume = (mvhd.FIXED16_volume >> 8);
    u8 bottom_volume = (mvhd.FIXED16_volume);
    printf("%stop volume: %d, bottom volume: %d\n",indent, top_volume, bottom_volume);
    offset += 10;//skip padding
    mvhd.window_geometry_a = read_u32_be(data + offset    ); printf("%swindow geometry A: %d.%d\n",indent, (int)(mvhd.window_geometry_a >> 16), 0x0000FFFF & mvhd.window_geometry_a);
    mvhd.window_geometry_b = read_u32_be(data + offset+ 4 ); printf("%swindow geometry B: %d.%d\n",indent, (int)(mvhd.window_geometry_b >> 16), 0x0000FFFF & mvhd.window_geometry_b);
    mvhd.window_geometry_u = read_u32_be(data + offset+ 8 ); printf("%swindow geometry U: %d.%d\n",indent, (int)(mvhd.window_geometry_u >> 16), 0x0000FFFF & mvhd.window_geometry_u);
    mvhd.window_geometry_c = read_u32_be(data + offset+ 12); printf("%swindow geometry C: %d.%d\n",indent, (int)(mvhd.window_geometry_c >> 16), 0x0000FFFF & mvhd.window_geometry_c);
    mvhd.window_geometry_d = read_u32_be(data + offset+ 16); printf("%swindow geometry D: %d.%d\n",indent, (int)(mvhd.window_geometry_d >> 16), 0x0000FFFF & mvhd.window_geometry_d);
    mvhd.window_geometry_v = read_u32_be(data + offset+ 20); printf("%swindow geometry V: %d.%d\n",indent, (int)(mvhd.window_geometry_v >> 16), 0x0000FFFF & mvhd.window_geometry_v);
    mvhd.window_geometry_x = read_u32_be(data + offset+ 24); printf("%swindow geometry X: %d.%d\n",indent, (int)(mvhd.window_geometry_x >> 16), 0x0000FFFF & mvhd.window_geometry_x);
    mvhd.window_geometry_y = read_u32_be(data + offset+ 28); printf("%swindow geometry Y: %d.%d\n",indent, (int)(mvhd.window_geometry_y >> 16), 0x0000FFFF & mvhd.window_geometry_y);
    mvhd.window_geometry_w = read_u32_be(data + offset+ 32); printf("%swindow geometry W: %d.%d\n",indent, (int)(mvhd.window_geometry_w >> 16), 0x0000FFFF & mvhd.window_geometry_w);
    if(mvhd.window_geometry_w == 0x40000000){printf("%sW is %d but just interpret as 1.0!\n",indent, (int)(mvhd.window_geometry_w >> 16));}
    // a b u
    // c d v
    // x y w


    mvhd.preview_time        = read_u32_be(data + offset + 36);printf("%spreview_time        %x\n",indent,mvhd.preview_time        ); 
    mvhd.preview_duration    = read_u32_be(data + offset + 40);printf("%spreview_duration    %x\n",indent,mvhd.preview_duration    ); 
    mvhd.poster_time         = read_u32_be(data + offset + 44);printf("%sposter_time         %x\n",indent,mvhd.poster_time         ); 
    mvhd.selection_time      = read_u32_be(data + offset + 48);printf("%sselection_time      %x\n",indent,mvhd.selection_time      ); 
    mvhd.selection_duration  = read_u32_be(data + offset + 52);printf("%sselection_duration  %x\n",indent,mvhd.selection_duration  ); 
    mvhd.current_time        = read_u32_be(data + offset + 56);printf("%scurrent_time        %x\n",indent,mvhd.current_time        ); 
    mvhd.next_track_id       = read_u32_be(data + offset + 60);printf("%snext_track_id       %x\n",indent,mvhd.next_track_id       ); 
    printf("%sbytes parsed: %zu\n", indent, (offset+64) - start);
    if(box.size == ((offset+64) - start))printf("%sall box bytes parsed!\n", indent);
    return mvhd;
}


void parse_x(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%sx SUB box size: %u\n",indent, subbox.size);
        printf("%sx SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('x','x','x','x'):{printf("%sx box found!\n",indent); offset += subbox.size; }break;
            default:
                printf("%sUNHANDLED  x CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }
}

void parse_meta(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT(); '\t';
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%smeta SUB box size: %u\n",indent, subbox.size);
        printf("%smeta SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('h', 'd', 'l', 'r'):{
                printf("%shdlr box found!\n",indent);
                u32 version_flags = read_u32_be(data + offset + 8);
                u8  flags                   = (u8)(version_flags >> 24);
                u32 version                 = (0x00FFFFFF & version_flags);
                u32 component_type          = read_u32_be(data + offset + 8 + 4);
                u32 component_subtype       = read_u32_be(data + offset + 8 + 8);
                u32 component_manufacturer  = read_u32_be(data + offset + 8 + 12);
                u32 component_flags         = read_u32_be(data + offset + 8 + 16);
                u32 component_flagsmask     = read_u32_be(data + offset + 8 + 20);
                printf("%sflags                   :",indent);   print_byte(flags                   );
                printf("%sversion                 :",indent);   print_4byte(version                 );
                printf("%scomponent_type          :",indent);   print_4byte(component_type          );
                printf("%scomponent_subtype       :",indent);   print_4byte(component_subtype       );
                printf("%scomponent_manufacturer  :",indent);   print_4byte(component_manufacturer  );
                printf("%scomponent_flags         :",indent);   print_4byte(component_flags         );
                printf("%scomponent_flagsmask     :",indent);   print_4byte(component_flagsmask     );

                offset += subbox.size;
            }break;            
            // case 0x796164a9:{
            case 0xa9646179:{
                printf("%scday box found\n",indent);
                offset += subbox.size;
            }break;
            case FOURCC('i', 'l', 's', 't'):{
                printf("%silst box found!\n",indent);
                // offset += subbox.size;
                offset += 8;
            }break;
            case FOURCC('d', 'a', 't', 'a'):{
                printf("%sdata box found!\n",indent);
                offset += subbox.size;
            }break;
            default:
                printf("%sUNHANDLED  meta CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                // __debugbreak();
        }
    }
}

void parse_udta(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%sudta SUB box size: %u\n",indent, subbox.size);
        printf("%sudta SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('m', 'e', 't', 'a'):{
                printf("%smeta box found!\n",indent);
                parse_meta(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            default:
                printf("%sUNHANDLED meta CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }


}




void parse_edts(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%sedts SUB box size: %u\n",indent, subbox.size);
        printf("%sedts SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        offset += 8;
        switch(subbox.name_val){
            case FOURCC('e', 'l', 's', 't'):{
                printf("%selst box found!\n",indent);
                u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 

                u8  version             = 0x00FFFFFF & version_and_flags >> 0;
                u32 flags               =     (0xFF & version_and_flags >> 24);
                u32 number_of_entries   = read_u32_be(data + offset); offset+=4; 
                printf("%sversion           : %x\n",indent,             version          );
                printf("%sflags             : %x\n",indent,             flags            );
                printf("%snumber of entries : %x\n",indent,             number_of_entries     );
                for(u32 i = 0; i < number_of_entries; i++){
                    u32 track_duration  = read_u32_be(data + offset); offset+=4;
                    u32 media_time      = read_u32_be(data + offset); offset+=4;
                    u32 media_rate      = read_u32_be(data + offset); offset+=4;
                    printf("%strack_duration      : %x\n",indent, track_duration);
                    printf("%smedia rate          : %x\n",indent, media_time);
                    printf("%smedia rate          : %d.%d\n",indent, (int)(media_rate  >> 16), 0x0000FFFF & media_rate );
                }
                //28 bytes
                //4 bytes for version and flag
                //4 bytes for number of entries
                //3 * 4 bytes per entry
                //4 + 4 + (12) + box name and size 28
                // parse_meta(data, offset + 8, subbox, depth+1);
                
                offset += subbox.size;

            }break;

            default:
                printf("%sUNHANDLED edts CASE!\n",indent);
                u32 test = read_u32_be(data + offset);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }
}





void parse_mdhd(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    
    mdhdBox mdhd = {};
    
    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 


    mdhd.flags            = (0xFF & version_and_flags >> 24);
    mdhd.version          = 0x00FFFFFF & version_and_flags >> 0;
    if (mdhd.version == 1) {
        mdhd.creationTime       = read_u64_be(data + offset); offset += 8;
        mdhd.modificationTime   = read_u64_be(data + offset); offset += 8;
        mdhd.timeScale          = read_u32_be(data + offset); offset += 4;
        mdhd.duration           = read_u64_be(data + offset); offset += 8;
    } else { // version == 0
        mdhd.creationTime       = read_u32_be(data + offset); offset += 4;
        mdhd.modificationTime   = read_u32_be(data + offset); offset += 4;
        mdhd.timeScale          = read_u32_be(data + offset); offset += 4;
        mdhd.duration           = read_u32_be(data + offset); offset += 4;
    }
    mdhd.language = read_u16_be(data + offset); offset+=2;
    mdhd.quality = read_u16_be(data + offset); offset+=2;
    printf("%smdhd data: VERSION: %u\n",indent, mdhd.version);
    printf("%sflags              : %x\n",indent, mdhd.flags              ); //print_4byte(mvhd.flags              );
    printf("%sversion            : %x\n",indent, mdhd.version            ); //print_byte (mvhd.version            );
    printf("%screationTime       : %x\n",indent, mdhd.creationTime       ); //print_4byte(mvhd.creationTime       );
    printf("%smodificationTime   : %x\n",indent, mdhd.modificationTime   ); //print_4byte(mvhd.modificationTime   );
    printf("%stimeScale          : %x\n",indent, mdhd.timeScale          ); //print_4byte(mvhd.timeScale          );
    printf("%sduration           : %x\n",indent, mdhd.duration           ); //print_4byte(mvhd.duration           );
    printf("%slanguage           : %x\n",indent, mdhd.language       ); //print_4byte(mvhd.FIXED32_rate       );
    printf("%squality            : %x\n",indent, mdhd.quality     ); //print_2byte(mvhd.FIXED16_volume     );
    char lang[4] = {};

    lang[0] = 'a' + (0x1F & (mdhd.language >> 10) - 1);
    lang[1] = 'a' + (0x1F & (mdhd.language >> 5 ) - 1);
    lang[2] = 'a' + (0x1F & (mdhd.language >> 0 ) - 1);
    printf("%slang:              : %s\n", indent, lang);
    printf("%sbytes parsed: %zu\n", indent, (offset) - start);
    if(box.size == ((offset) - start))printf("%sall box bytes parsed!\n", indent);
}

u32 g_first_chunk_offset = 0;
u32 g_first_chunk_sample_size = 0;
struct stcoBox{
    Box box;
    u8 version;
    u32 flags;
    u32 number_of_entries;
};

void parse_stco(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header

    stcoBox stco = {};
    stco.box = box;

    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 
    stco.flags            = (0xFF & version_and_flags >> 24);
    stco.version          = 0x00FFFFFF & version_and_flags >> 0;
    stco.number_of_entries = read_u32_be(data + offset); offset+=4;
    printf("%smdhd data: VERSION : %u\n",indent,  stco.version);
    printf("%sflags              : %x\n",indent, stco.flags            );
    printf("%snumber of entries  : %u\n",indent, stco.number_of_entries);
    if(!g_first_chunk_offset)g_first_chunk_offset = read_u32_be(data + offset); 
    for(u32 i = 0; i < stco.number_of_entries; i++){
        u32 chunk_offset = read_u32_be(data + offset); offset+=4;
        printf("%schunk %u offset: %x\n",indent,i,chunk_offset);
    }
    
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    int fuck_the_debugger = 0; 

}

struct stscBox{
    Box box;
    u8 version;
    u32 flags;
    u32 number_of_entries;
};

void parse_stsc(uint8_t* data, size_t offset, Box box, int depth = 0){//sample to chunk
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header

    stscBox stsc = {};
    stsc.box = box;

    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 
    stsc.flags            = (0xFF & version_and_flags >> 24);
    stsc.version          = 0x00FFFFFF & version_and_flags >> 0;
    stsc.number_of_entries = read_u32_be(data + offset); offset+=4;
    printf("%sstsc data: VERSION   : %u\n",indent, stsc.version);
    printf("%sflags                : %x\n",indent, stsc.flags            );
    printf("%snumber of entries    : %u\n",indent, stsc.number_of_entries);
    for(u32 i = 0; i < stsc.number_of_entries; i++){
        u32 first_chunk             = read_u32_be(data + offset); offset+=4;
        u32 samples_per_chunk       = read_u32_be(data + offset); offset+=4;
        u32 sample_description_id   = read_u32_be(data + offset); offset+=4;
        printf("%sentry: %u\n", indent, i);
        printf("%sfirst chunk          : %u\n",indent,first_chunk);
        printf("%ssamples per chunk    : %u\n",indent,samples_per_chunk);
        printf("%ssample description id: %u\n",indent,sample_description_id);
    }
    
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    int fuck_the_debugger = 0; 

}



struct stszBox{
    Box box;
    u8 version;
    u32 flags;
    u32 sample_size;
    u32 number_of_entries;
};

void parse_stsz(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header

    stszBox stsz = {};
    stsz.box = box;

    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 
    stsz.flags            = (0xFF & version_and_flags >> 24);
    stsz.version          = 0x00FFFFFF & version_and_flags >> 0;
    stsz.sample_size       = read_u32_be(data + offset); offset+=4;
    stsz.number_of_entries = read_u32_be(data + offset); offset+=4;
    printf("%sstsz data: VERSION   : %u\n",indent, stsz.version);
    printf("%sflags                : %x\n",indent, stsz.flags            );
    printf("%ssample size          : %u\n",indent, stsz.sample_size);
    printf("%snumber of entries    : %u\n",indent, stsz.number_of_entries);
    if(!g_first_chunk_sample_size)g_first_chunk_sample_size = read_u32_be(data + offset); 
    for(u32 i = 0; i < stsz.number_of_entries; i++){
        u32 sample                  = read_u32_be(data + offset); offset+=4;
        printf("%ssample %u : %u\n",indent,i,sample);
    }
    
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    int fuck_the_debugger = 0; 

}

void parse_avcC(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header

    //specified at page 18 in ISO_IEC_14496-15-AVC-format-2017-e51a22786929a5fbc2f9dc9fc4761e09.pdf
    uint8_t configurationVersion = read_u8_be(data + offset) ; offset++;        // = 1
    uint8_t AVCProfileIndication  = read_u8_be(data + offset); offset++;          // from SPS
    uint8_t profile_compatibility = read_u8_be(data + offset); offset++;  
    uint8_t AVCLevelIndication    = read_u8_be(data + offset); offset++;  
    uint8_t lengthSizeMinusOne    = read_u8_be(data + offset); offset++;            // 6 bits reserved + 2 bits lengthSizeMinusOne
    lengthSizeMinusOne = lengthSizeMinusOne & 0x03;
    uint8_t numOfSequenceParameterSets = read_u8_be(data + offset); offset++;    // 3 bits reserved + 5 bits actual count
    numOfSequenceParameterSets = numOfSequenceParameterSets & 0x1F;//mask lower 5 bits
    printf("%sconfiguration version: %u\n",indent, configurationVersion);
    printf("%sAVCProfileIndication  : %u\n",indent, AVCProfileIndication  );
    printf("%sprofile_compatibility : %u\n",indent, profile_compatibility );
    printf("%sAVCLevelIndication    : %u\n",indent, AVCLevelIndication    );
    printf("%slengthSizeMinusOne    : %u\n",indent, lengthSizeMinusOne    );
    printf("%snumOfSequenceParameterSets    : %u\n",indent, numOfSequenceParameterSets    );

    for(u8 i = 0; i < numOfSequenceParameterSets; i++){
        u16 sequenceParameterSetLength    = read_u16_be(data + offset); offset+=2;  
         printf("%s  sequenceParameterSetLength    : %u\n",indent, sequenceParameterSetLength    );
        
         printf("%s  sequenceParameterSetNALUnit    : ",indent);
        for(int i = 0; i < sequenceParameterSetLength; i++){
            printf("%u ", *(data+offset + i));
        }
        printf("\n");

        u8* sequenceParameterSetNALUnit = data + offset;
        offset += sequenceParameterSetLength;


    }

    u8 numOfPictureParameterSets = read_u8_be(data + offset); offset++; 
        printf("%snumOfPictureParameterSets    : %u\n",indent, numOfPictureParameterSets    );
        for(u8 i = 0; i < numOfPictureParameterSets; i++){
        u16 pictureParameterSetLength    = read_u16_be(data + offset); offset+=2;  
       
        printf("%s  pictureParameterSetLength    : %u\n",indent, pictureParameterSetLength    );
         printf("%s  pictureParameterSetNALUnit    : ",indent);
        for(int i = 0; i < pictureParameterSetLength; i++){
            printf("%u ", *(data+offset + i));
        }
        printf("\n");
       
        u8* pictureParameterSetNALUnit = data + offset;
        offset += pictureParameterSetLength;

    }
    if(AVCProfileIndication == 100 || AVCProfileIndication == 110 || AVCProfileIndication == 122 || AVCProfileIndication == 144){
        u8 chroma_format = read_u8_be(data + offset); offset++; 
        chroma_format = chroma_format & 0x03;
        printf("%schroma_format    : %u\n",indent, chroma_format    );
        
        u8 bit_depth_luma_minus8 = read_u8_be(data + offset); offset++; 
        bit_depth_luma_minus8 = bit_depth_luma_minus8 & 0x07;
        printf("%sbit_depth_luma_minus8    : %u\n",indent, bit_depth_luma_minus8    );
        
        u8 bit_depth_chroma_minus8 = read_u8_be(data + offset); offset++; 
        bit_depth_chroma_minus8 = bit_depth_chroma_minus8 & 0x07;
        printf("%sbit_depth_chroma_minus8    : %u\n",indent, bit_depth_chroma_minus8    );

        u8 numOfSequenceParameterSetExt = read_u8_be(data + offset); offset++; 
        printf("%snumOfSequenceParameterSetExt    : %u\n",indent, numOfSequenceParameterSetExt    );

        for(u8 i = 0; i < numOfSequenceParameterSetExt; i++){
            u16 sequenceParameterSetExtLength    = read_u16_be(data + offset); offset+=2;  
            printf("%s  sequenceParameterSetExtLength    : %u\n",indent, sequenceParameterSetExtLength    );

            printf("%s  sequenceParameterSetExtNALUnit    : ",indent);
            for(int i = 0; i < sequenceParameterSetExtLength; i++){
                printf("%u ", *(data+offset + i));
            }
            printf("\n");
            u8* sequenceParameterSetExtNALUnit = data + offset;
            offset += sequenceParameterSetExtLength;
    
        }

    }
    // __debugbreak();
    // for (i = 0; i < numOfSPS; i++) {
    //   uint16_t spsLength;
    //   uint8_t spsData[spsLength];
    // }
  
    // uint8_t numOfPictureParameterSets;
    // for (i = 0; i < numOfPPS; i++) {
    //   uint16_t ppsLength;
    //   uint8_t ppsData[ppsLength];
    // }
    
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    else{
        printf("DIDNT READ ALL THE BYTES IN THE BOX!\n");
        __debugbreak();
    }
}

void parse_avc1(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    offset+=6;//skips reserved
    u16 data_reference_index  = read_u16_be(data + offset); offset +=2;
    u16 pre_defined  = read_u16_be(data + offset); offset +=2;
    u16 reserved     = read_u16_be(data + offset); offset +=2;
    u32 pre_defined1 = read_u32_be(data + offset); offset +=4;
    u32 pre_defined2 = read_u32_be(data + offset); offset +=4;
    u32 pre_defined3 = read_u32_be(data + offset); offset +=4;
    u16 width        = read_u16_be(data + offset); offset +=2;
    u16 height       = read_u16_be(data + offset); offset +=2;
    u32 horiz_res       = read_u32_be(data + offset); offset +=4;
    u32 vert_res        = read_u32_be(data + offset); offset +=4;
    u32 reserved1       = read_u32_be(data + offset); offset +=4;
    u16 frame_count     = read_u16_be(data + offset); offset +=2;
    size_t string_count = 32;
    char* compressorName = (char*)(data + offset);
    for(size_t i = 0; i < string_count;i++){
        printf("%c", *(data+offset++));
    }
    u16 vis_depth     = read_u16_be(data + offset); offset +=2;
    s16 pre_defined_neg1   = read_u16_be(data + offset); offset +=2;
    
    printf("%sdepth : %x\n", indent, vis_depth);//name is depth, but need to distinguish it from our depth parameter
    printf("%spre_defined_neg1 : %d\n", indent, pre_defined_neg1);//name is depth, but need to distinguish it from our depth parameter

    printf("%swidth : %u\n", indent, data_reference_index);
    printf("%swidth : %u\n", indent, width);
    printf("%sheight: %u\n", indent, height);
    printf("%shorizontal resolution: %x\n", indent, horiz_res);
    printf("%svertical resolution  : %x\n", indent, vert_res);

    while(offset - start < box.size){
        Box optional_box = parse_box(data, offset);
        printf("%savc1 SUB box size: %u\n",indent,    optional_box.size);
        printf("%savc1 SUB box name: %x\n",indent,    optional_box.name_val);
        printf("%ssubbox name: ",indent); print_4byte(optional_box.name_val);
        switch(optional_box.name_val){
            case FOURCC('c','l','a','p'):{
                printf("%sclap box found!\n", indent);
            }break;
            case FOURCC('p','a','s','p'):{
                printf("%spasp box found!\n", indent);
            }break;
            case FOURCC('b','t','r','t'):{
                printf("%sbtrt box found!\n", indent);
            }break;
            case FOURCC('f','i','e','l'):{
                printf("%sfiel box found!\n", indent);
            }break;
            case FOURCC('g','a','m','a'):{
                printf("%sgama box found!\n", indent);
            }break;
            case FOURCC('c','o','l','r'):{
                printf("%scolr box found!\n", indent);
            }break;
            case FOURCC('a','v','c','C'):{
                printf("%savcC box found!\n", indent);
                parse_avcC(data, offset + 8, optional_box, depth);
            }break;

            default:{
                printf("%sunhandled box type:",indent);
                print_4byte(optional_box.name_val);
                printf("\n");
            }break;
        }
        offset += optional_box.size;
    }

    
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    else{
        printf("DIDNT READ ALL THE BYTES IN THE BOX!\n");
        __debugbreak();
    }
}

struct stsdBox{
    Box box;
    u8 version;
    u32 flags;
    u32 number_of_entries;
};

void parse_stsd(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header

    stsdBox stsd = {};
    stsd.box = box;

    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 
    stsd.flags            = (0xFF & version_and_flags >> 24);
    stsd.version          = 0x00FFFFFF & version_and_flags >> 0;
    stsd.number_of_entries = read_u32_be(data + offset); offset+=4;
    printf("%sstsd data: VERSION   : %u\n",indent, stsd.version);
    printf("%sflags                : %x\n",indent, stsd.flags            );
    printf("%snumber of entries    : %u\n",indent, stsd.number_of_entries);
    for(u32 i = 0; i < stsd.number_of_entries; i++){
        Box mediaBox = {};
        mediaBox = parse_box(data, offset);
        printf("%sstsd SUB box size: %u\n",indent,    mediaBox.size);
        printf("%sstsd SUB box name: %x\n",indent,    mediaBox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(mediaBox.name_val);
        switch(mediaBox.name_val){
            case FOURCC('a','v','c','1'):{
                printf("%savc1 box found!\n", indent);
                parse_avc1(data, offset + 8, mediaBox, depth);
                offset += mediaBox.size;
            }break;
            case FOURCC('m','p','4','a'):{
                printf("%smp4a box found!\n", indent);
                offset += mediaBox.size;
            }break;
            default:{
                print_4byte(mediaBox.name_val);
                printf("%sunhandled media box type!\n",indent);
                offset += mediaBox.size;
            }break;
        }
    }
    
    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    else{
        printf("DIDNT READ ALL THE BYTES IN THE BOX!\n");
        __debugbreak();
    }
    int fuck_the_debugger = 0; 

}


void parse_stts(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
}


void parse_stbl(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%sstbl SUB box size: %u\n",indent, subbox.size);
        printf("%sstbl SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('s','t','s','d'):{
                printf("%sstsd box found!\n",indent);
                parse_stsd(data, offset + 8, subbox, depth);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','t','s'):{//decode time -> sample index mapping
                printf("%sstts box found!\n",indent);
                parse_stts(data, offset + 8, subbox, depth);
                offset += subbox.size; 
            }break;
            case FOURCC('c','t','t','s'):{
                printf("%sctts box found!\n",indent);
                offset += subbox.size;
            }break;
            case FOURCC('c','s','l','g'):{
                printf("%scslg box found!\n",indent);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','s','s'):{
                printf("%sstss box found!\n",indent);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','p','s'):{
                printf("%sstps box found!\n",indent);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','s','c'):{//chunk to sample mapping
                printf("%sstsc box found!\n",indent);
                parse_stsc(data, offset + 8, subbox, depth);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','s','z'):{//sample sizes
                printf("%sstsz box found!\n",indent);
                parse_stsz(data, offset + 8, subbox, depth);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','c','o'):{ //chunk offsets
                printf("%sstco box found!\n",indent);
                parse_stco(data, offset + 8, subbox, depth);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','6','4'):{
                printf("%sst64 box found! NOT HANDLED!\n",indent);
                __debugbreak();
                offset += subbox.size; 
            }break;
            case FOURCC('s','d','t','p'):{
                printf("%ssdtp box found!\n",indent);
                offset += subbox.size; 
            }break;
            case FOURCC('s','t','s','h'):{
                printf("%sstsh box found!\n",indent);
                offset += subbox.size; 
            }break;
            case FOURCC('s','g','p','d'):{
                printf("%ssgpd box found!\n",indent);
                offset += subbox.size; 
            }break;
            case FOURCC('s','b','g','p'):{
                printf("%ssbgp box found!\n",indent);
                offset += subbox.size; 
            }break;
            default:
                printf("%sUNHANDLED stbl CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }
}


void parse_minf(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%sminf SUB box size: %u\n",indent, subbox.size);
        printf("%sminf SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('v', 'm', 'h', 'd'):{
                printf("%svmhd box found!\n",indent);
                offset += subbox.size;
            }break;
            case FOURCC('h', 'd', 'l', 'r'):{
                printf("%shdlr box found!\n",indent);
                offset += subbox.size;
            }break;
            case FOURCC('d', 'i', 'n', 'f'):{
                printf("%sdinf box found!\n",indent);
                offset += subbox.size;
            }break;
            case FOURCC('s', 't', 'b', 'l'):{
                printf("%sstbl box found!\n",indent);
                parse_stbl(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('s', 'm', 'h', 'd'):{
                printf("%ssmhd box found!\n",indent);
                offset += subbox.size;
            }break;
            default:
                printf("%sUNHANDLED minf CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }
}


struct hdlrBox{
    Box box;
    u8 version;
    u32 flags;
    u32 pre_defined;
    u32 handler_type;
    u32 reserved1;
    u32 reserved2;
    u32 reserved3;
    char* name;
};

void parse_hdlr(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header

    hdlrBox hdlr = {};
    hdlr.box = box;


    u32 version_and_flags   = read_u32_be(data + offset); offset+=4; 
    hdlr.flags              = (0xFF & version_and_flags >> 24);
    hdlr.version            = 0x00FFFFFF & version_and_flags >> 0;

    hdlr.pre_defined = read_u32_be(data + offset);  offset+=4; 
    hdlr.handler_type = read_u32_be(data + offset);  offset+=4; 
    hdlr.reserved1 = read_u32_be(data + offset);  offset+=4; 
    hdlr.reserved2 = read_u32_be(data + offset);  offset+=4; 
    hdlr.reserved3 = read_u32_be(data + offset);  offset+=4;
    printf("%shdlr data: VERSION      : %u\n",indent, hdlr.version);
    printf("%sflags                   : %x\n",indent,  hdlr.flags            );
    printf("%spre_defined    : %x\n",indent, hdlr.pre_defined); 
    printf("%shandler_type   : %x\n",indent, hdlr.handler_type);  
    printf("%sreserved1      : %x\n",indent, hdlr.reserved1);   
    printf("%sreserved2      : %x\n",indent, hdlr.reserved2);  
    printf("%sreserved3      : %x\n",indent, hdlr.reserved3);  
    switch(hdlr.handler_type){
        case FOURCC('v','i','d','e'):{
            printf("%sVIDEO track type!\n", indent);
        }break;
        case FOURCC('s','o','u','n'):{
            printf("%sSOUND track type!\n", indent);
        }break;
        case FOURCC('h','i','n','t'):{
            printf("%sHINT track type!\n", indent);
        }break;
        case FOURCC('m','e','t','a'):{
            printf("%sMETA track type!\n", indent);
        }break;
        case FOURCC('a','u','x','v'):{
            printf("%sAUXV track type!\n", indent);
        }break;
        default:{
            printf("%sunrecognized track type!\n", indent);
        }break;
    }


    hdlr.name = (char*)(data + offset);
    printf("%sname: ",indent);
    while(*(data + offset)){
        printf("%c",*(data + offset));
        offset++;
    } 
    offset++;//for null terminator
    printf("\n");

    printf("%sbytes parsed: %zu\n", indent, offset - start);
    if(box.size == (offset - start))printf("%sall box bytes parsed!\n", indent);
    else{
        printf("DIDNT READ ALL THE BYTES IN THE BOX!\n");
        __debugbreak();
    }
    int fuck_the_debugger = 0; 

}


void parse_mdia(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%smdia SUB box size: %u\n", indent, subbox.size);
        printf("%smdia SUB box name: %x\n", indent, subbox.name_val);
        printf("%ssubbox name: ", indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('m', 'd', 'h', 'd'):{
                printf("%smdhd box found!\n", indent);
                parse_mdhd(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('e', 'l', 'n', 'g'):{ 
                printf("%sfound elng box in mdia box!\n", indent);
                offset += subbox.size;
            }break;
            case FOURCC('h', 'd', 'l', 'r'):{ 
                printf("%sfound hdlr box in mdia box!\n", indent);
                parse_hdlr(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('m', 'i', 'n', 'f'):{ //video media information
                printf("%sfound minf box in mdia box!\n", indent);
                parse_minf(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('u', 'd', 't', 'a'):{ //user data atom
                printf("%sfound udta box in mdia box!\n", indent);
                parse_udta(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            default:
                printf("%sUNHANDLED  mdia CASE!\n", indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }
}

void parse_trak(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%strak SUB box size: %u\n",indent, subbox.size);
        printf("%strak SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('t', 'k', 'h', 'd'):{
                printf("%stkhd box found!\n",indent);
                parse_tkhd(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('m', 'd', 'i', 'a'):{
                printf("%smdia box found!\n",indent);
                parse_mdia(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('e', 'd', 't', 's'):{
                printf("%sedts box found!\n",indent);
                parse_edts(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            default:
                printf("%sUNHANDLED  trak CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
        }
    }
}

void parse_moov(uint8_t* data, size_t offset, Box box, int depth = 0){
    depth++;
    char indent[64] = {};
    CREATE_INDENT();
    size_t start = offset - 8;//because we need to count the size + name of the parent box header
    while(offset < box.size + start){//offset could be large, make the size relative
        Box subbox = parse_box(data, offset);
        printf("%sMOOV SUB box size: %u\n",indent, subbox.size);
        printf("%sMOOV SUB box name: %x\n",indent, subbox.name_val);
        printf("%ssubbox name: ",indent); print_4byte(subbox.name_val);
        switch(subbox.name_val){
            case FOURCC('m', 'v', 'h', 'd'):{
                mvhdBox mvhd = parse_mvhd(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('t', 'r', 'a', 'k'):{
                parse_trak(data, offset + 8, subbox, depth);
                offset += subbox.size;
            }break;
            case FOURCC('u', 'd', 't', 'a'):{ //user data atom
                printf("%sfound udta box\n",indent);
                parse_udta(data, offset + 8, subbox, depth);
                // parse_trak(data, offset + 8);
                offset += subbox.size;
            }break;
            default:{
                printf("%sUNHANDLED moov CASE!\n",indent);
                u32 test = read_u32_be(data + offset + 8);
                printf("%s",indent); print_4byte(test);
                offset+=4;
                __debugbreak();
            }break;
        }
    }

}

/*     u32 flags;
u8 version;
u32 creationTime;
u32 modificationTime;
u32 timeScale;
u32 duration;
u32 FIXED32_rate;
u16 FIXED16_volume; */

//     u32 size;
//     u32 major_brand;
//     u32 minor_version;
//     u32 compatible_brands[32];
//     char* name;

// };



void parse_binary_test(){
    // size_t name_buffer_size = 1024*1024*8;
    // char* name_buffer = (char*) plat_alloc_mem(name_buffer_size);

    debug_file file = {};
    // char filename[128] = "C:/Users/AAAAAAAAAAAAAAAAAAAA/Videos/SKELETAL_ANIMATION-3-1-25.mp4";
    // char filename[128] = "C:/Users/AAAAAAAAAAAAAAAAAAAA/Videos/vid.mp4";
    // char filename[128] = "C:/Users/AAAAAAAAAAAAAAAAAAAA/Videos/beer_towers.mp4";
    char filename[128] = "C:/labor/assets/wtf.mp4";
    file =  Win32ReadFile(filename);

    u8* data = (u8*)file.memory;
    size_t size = file.filesize.QuadPart;
    printf("size of file: %zu\n", size);
    size_t offset = 0;
    int depth = 0;
    Box box = {};
    size_t mdat_offset = 0;
    while(offset < size){
        box = parse_box(data, offset);
        printf("box size: %u\n", box.size);
        printf("box name: %.4s\n", box.name);
        printf("box name: %x\n", box.name_val);
        // print_4byte(box.name_val);

        switch (box.name_val){
            case FOURCC('f', 't', 'y', 'p'):{
                printf("FTYP FOUND!\n");
                FileTypeBox ftyp = parse_ftyp(data, offset+8, box, depth); 
                printf("major brand: "); print_4byte(ftyp.major_brand);
                printf("minor version: %u\n", ftyp.minor_version);    // print_4byte(ftyp.minor_version);

                offset += box.size;

            }break;
            case FOURCC('m', 'd', 'a', 't'):{
                //parse the sub mvhd box
                mdat_offset = box.offset_at_parse;
                offset += box.size;
            }break;
            case FOURCC('m', 'o', 'o', 'v'):{
                //parse the sub mvhd box

                parse_moov(data, offset + 8, box, depth+1);


                //beginning of trak
                offset += box.size;

            }break;
            case FOURCC('t', 'r', 'a', 'k'):{
                // u32 boxsize           = read_u32_be(data + offset + 16 + 36 + 64); printf("size: %u\n", boxsize);
                // u32 boxName        = read_u32_be(data + offset + 16 + 36 + 68); printf("box name: %x\n", boxName);

                    printf("found TRAK box\n");
             

                    offset += box.size;

            }break;
            default:{
                offset += box.size;

            }break;
        }


    }

    printf("location of mdat test\n");
    box = parse_box(data, mdat_offset);
    printf("box size: %u\n", box.size);
    printf("box name: %.4s\n", box.name);
    printf("box name: %x\n", box.name_val);


    if(g_first_chunk_offset && g_first_chunk_sample_size){
        printf("PARSING FIRST CHUNK!\n");
        printf("first chunk offset      : %x\n",g_first_chunk_offset);
        printf("first chunk sample size : %u\n",g_first_chunk_sample_size);
        offset = g_first_chunk_offset;
        print_hex(data, offset, 16);


        u32 nal_size = read_u32_be(data + offset); offset+=4;
        u8 nal_header = read_u8_be(data + offset); offset++;
        u32 nal_unit_type = nal_header & 0x1f;

        printf("nal size: %u\n", nal_size);
        printf("forbidden bit: %u\n", nal_header >> 7);
        printf("nal reference IDC: %u\n", nal_header >> 5);
        printf("nal unit type: %u\n", nal_unit_type);// ox1f is 11111, 5 bits

        //find SPS and PPS NAL units
        //find IDR (keyframe) slice
        
        u32 nalUnitHeaderBytes = 1;  // already consumed 1 byte
        u32 nal_payload_offset = offset;
        u8 svc_ext_flag = 0;
        u8 avc_3d_ext_flag = 0;

        if (nal_unit_type == 14 || nal_unit_type == 20 || nal_unit_type == 21) {
            if (nal_unit_type != 21) {
                svc_ext_flag = (data[offset] >> 7) & 0x01;
            } else {
                avc_3d_ext_flag = (data[offset] >> 7) & 0x01;
            }
            if (svc_ext_flag) {
                // svc extension (3 extra bytes)
                nalUnitHeaderBytes += 3;
            } else if (avc_3d_ext_flag) {
                // 3D AVC extension (2 extra bytes)
                nalUnitHeaderBytes += 2;
            } else {
                // mvc extension (3 extra bytes)
                nalUnitHeaderBytes += 3;
            }
            offset += nalUnitHeaderBytes - 1;  // already moved 1 byte earlier
        }

        // Convert NAL payload to RBSP by removing emulation prevention bytes
        u32 NumBytesInRBSP = 0;
        u8 rbsp[65536] = {}; // static buffer for now; size should be safe for most NALs
        u32 i = nal_payload_offset + (nalUnitHeaderBytes - 1);
        u32 end = nal_payload_offset + nal_size - 1;

        while (i <= end) {
            if (i + 2 <= end && data[i] == 0x00 && data[i+1] == 0x00) {
                printf("00 00 %02x at offset %d\n", data[i+2], i); //just a test to help visualize data
            }
            if (i + 2 <= end && data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x03) {
                rbsp[NumBytesInRBSP++] = data[i++];
                rbsp[NumBytesInRBSP++] = data[i++];
                i++; // skip emulation_prevention_three_byte (0x03)
                printf("stripping emulation prevention bits!\n");
            } else {
                rbsp[NumBytesInRBSP++] = data[i++];
            }
        }

        switch(nal_unit_type){
            case 6:{
                printf("parse SEI NAL here!\n");
            }break;
            default:{
                printf("just do SPS parsing here, need to find the number for it later\n");
                //SPS data
                //RBSP = raw byte sequenmce payload
                size_t rbspOffset = 0;
                u8 bit = 0;
                u8 profile_idc = rbsp[rbspOffset++];
                u8 byte = rbsp[rbspOffset++];
                printf("profile idc: %d\n", profile_idc);
                u8 constaint_set0_flag = 0x1 & (byte >> 7);
                u8 constaint_set1_flag = 0x1 & (byte >> 6);
                u8 constaint_set2_flag = 0x1 & (byte >> 5);
                u8 constaint_set3_flag = 0x1 & (byte >> 4);
                u8 constaint_set4_flag = 0x1 & (byte >> 3);
                u8 constaint_set5_flag = 0x1 & (byte >> 2);
                u8 reserved_zero_2bits = 0x3 & (byte);
                bit = 0;//reset bit
                printf("constaint set 0: %d\n", constaint_set0_flag);
                printf("constaint set 1: %d\n", constaint_set1_flag);
                printf("constaint set 2: %d\n", constaint_set2_flag);
                printf("constaint set 3: %d\n", constaint_set3_flag);
                printf("constaint set 4: %d\n", constaint_set4_flag);
                printf("constaint set 5: %d\n", constaint_set5_flag);
                printf("reserved zero 2 bits: %d\n", reserved_zero_2bits);

                u8 level_idc = rbsp[rbspOffset++];
                printf("level idc: %d\n", level_idc);
            }
            
        }
   

        __debugbreak();
        int fuck_the_debugger = 0;
    }

    print_hex(data, 0, 16);

    printf("%u\n", read_u32_be(data));

    parse_boxes(data, 32);

    // plat_dealloc_mem(name_buffer, name_buffer_size);

    Win32FreeFile(&file);
}
