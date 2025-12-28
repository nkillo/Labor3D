#pragma once
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
//need the define for using PRId32
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define pack754_16(f) (pack754((f), 16, 5))
#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_16(i) (unpack754((i), 16, 5))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

inline uint64_t 
pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (f == 0.0) return 0; // get this special case out of the way

    // check sign and begin normalization
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }

    // get the normalized form of f and track the exponent
    shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL<<significandbits) + 0.5f);

    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

    // return the final answer
    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

inline long double 
unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0) return 0.0;

    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }

    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;

    return result;
}

//packi16() store a 16 bit int into a char buffer (like htons())
inline void
packi16(unsigned char* buf, uint16_t i){
    *buf++ = i >> 8;
    *buf++ = i;
}

//packi32() store a 32 bit int into a char buffer (like htonl())
inline void
packi32(unsigned char* buf, uint32_t i){
    *buf++ = i >> 24; 
    *buf++ = i >> 16; 
    *buf++ = i >> 8; 
    *buf++ = i;
}

//packi64() store a 64 bit int into a char buffer (like htonl())
inline void
packi64(unsigned char* buf, uint64_t i){
    *buf++ = i >> 56;
    *buf++ = i >> 48;
    *buf++ = i >> 40;
    *buf++ = i >> 32;

    *buf++ = i >> 24;
    *buf++ = i >> 16;
    *buf++ = i >> 8;
    *buf++ = i;
}

//unpacki16() unpack a 16 bit int from a char buffer (like ntohs())
inline int 
unpacki16(unsigned char* buf){
    uint16_t i2 = ((uint16_t)buf[0]<<8) | buf[1];
    int16_t i;
    //change unsigned numbers to signed
    if(i2 <= 0x7fffu){i = i2;}
    else{i = -1 - (uint16_t)(0xffffu - i2);}
    
    return i;
}

//unpacku16() unpack a 16 bit unsigned from a char buffer (like ntohs())
inline uint16_t
unpacku16(unsigned char* buf){
    return ((uint16_t)buf[0]<<8) | buf[1];
}

//unpacki32() unpack a 32 bit int from a char buffer (like ntohl())
inline int32_t
unpacki32(unsigned char* buf){
    uint32_t i2 = ((uint32_t)buf[0]<<24) | 
                           ((uint32_t)buf[1]<<16) | 
                           ((uint32_t)buf[2]<<8) | 
                           ((uint32_t)buf[3]);
    int32_t i;
    //change unsigned numbers to signed
    if(i2 <= 0x7fffffffu){i = i2;} //0x7fffffffu is max signed value
    else{i = -1 - (int32_t)(0xffffffffu - i2);}

    return i;
}

//unpacku32() unpack a 32 bit unsigned from a char buffer (like ntohl())
inline uint32_t
unpacku32(unsigned char* buf){
    return((uint32_t)buf[0]<<24) | 
          ((uint32_t)buf[1]<<16) | 
          ((uint32_t)buf[2]<<8 ) |
          ((uint32_t)buf[3]);
}

//unpacki64() unpack a 64 bit int from a char buffer (like ntohl())
inline int64_t 
unpacki64(unsigned char* buf){
    
    uint64_t i2 = ((uint64_t)buf[0]<<56) |
                  ((uint64_t)buf[1]<<48) |
                  ((uint64_t)buf[2]<<40) |
                  ((uint64_t)buf[3]<<32) |
              
                  ((uint64_t)buf[4]<<24) |
                  ((uint64_t)buf[5]<<16) |
                  ((uint64_t)buf[6]<<8 ) |
                  ((uint64_t)buf[7]);

    int64_t i;
    //change unsigned numbers to signed
    if (i2 <= 0x7fffffffffffffffu) { i = i2; }
    else { i = -1 -(int64_t)(0xffffffffffffffffu - i2); }

    return i;
}

//unpacku64() unpack a 64 bit unsigned from a char buffer (like ntohl())
inline uint64_t
unpacku64(unsigned char* buf){
    return ((uint64_t)buf[0]<<56) |
           ((uint64_t)buf[1]<<48) |
           ((uint64_t)buf[2]<<40) |
           ((uint64_t)buf[3]<<32) |
       
           ((uint64_t)buf[4]<<24) |
           ((uint64_t)buf[5]<<16) |
           ((uint64_t)buf[6]<<8 ) |
           ((uint64_t)buf[7]);
}

/*
**pack() -- store data dictated by the format string in the buffer
**
**  bits / signed   unsigned    float   string
**  -----+-------------------------------------
**  8    /  c           C       
**  16   /  h           H         f
**  32   /  l           L         d
**  64   /  q           Q         g
**  -    /                                s
**
** (16 bit unsigned length is automatically prepended to strings)
*/

inline unsigned int 
pack(unsigned char *buf, const char *format, ...)
{
    va_list ap;

    signed char c;              // 8-bit
    unsigned char C;

    int h;                      // 16-bit
    unsigned int H;

    long int l;                 // 32-bit
    unsigned long int L;

    long long int q;            // 64-bit
    unsigned long long int Q;

    float f;                    // floats
    double d;
    long double g;
    unsigned long long int fhold;

    char *s;                    // strings
    unsigned int len;

    unsigned int size = 0;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
        case 'c': // 8-bit
            size += 1;
            c = (signed char)va_arg(ap, int); // promoted
            *buf++ = c;
            break;

        case 'C': // 8-bit unsigned
            size += 1;
            C = (unsigned char)va_arg(ap, unsigned int); // promoted
            *buf++ = C;
            break;

        case 'h': // 16-bit
            size += 2;
            h = va_arg(ap, int);
            packi16(buf, h);
            buf += 2;
            break;

        case 'H': // 16-bit unsigned
            size += 2;
            H = va_arg(ap, unsigned int);
            packi16(buf, H);
            buf += 2;
            break;

        case 'l': // 32-bit
            size += 4;
            l = va_arg(ap, long int);
            packi32(buf, l);
            buf += 4;
            break;

        case 'L': // 32-bit unsigned
            size += 4;
            L = va_arg(ap, unsigned long int);
            packi32(buf, L);
            buf += 4;
            break;

        case 'q': // 64-bit
            size += 8;
            q = va_arg(ap, long long int);
            packi64(buf, q);
            buf += 8;
            break;

        case 'Q': // 64-bit unsigned
            size += 8;
            Q = va_arg(ap, unsigned long long int);
            packi64(buf, Q);
            buf += 8;
            break;

        case 'f': // float-16
            size += 2;
            f = (float)va_arg(ap, double); // promoted
            fhold = pack754_16(f); // convert to IEEE 754
            packi16(buf, fhold);
            buf += 2;
            break;

        case 'd': // float-32
            size += 4;
            d = va_arg(ap, double);
            fhold = pack754_32(d); // convert to IEEE 754
            packi32(buf, fhold);
            buf += 4;
            break;

        case 'g': // float-64
            size += 8;
            g = va_arg(ap, long double);
            fhold = pack754_64(g); // convert to IEEE 754
            packi64(buf, fhold);
            buf += 8;
            break;

        case 's': // string
            s = va_arg(ap, char*);
            len = strlen(s);
            size += len + 2;
            packi16(buf, len);
            buf += 2;
            memcpy(buf, s, len);
            buf += len;
            break;
        }
    }

    va_end(ap);

    return size;
}

//can do this too but the pack/unpack functions make it cleaner
   // int offset = 0;
        // sequence = unpacku16((unsigned char*)buffer + offset);
        // offset += 2;
        // ping_time = unpacku64((unsigned char*)buffer + offset);
        // offset += 8;
        // magicNumber = unpacku16((unsigned char*)buffer + offset);
        // offset += 2;


/*
** unpack() -- unpack data dictated by the format string into the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C         
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a max length)
*/
inline void 
unpack(unsigned char *buf, const char *format, ...)
{
    va_list ap;

    signed char *c;              // 8-bit
    unsigned char *C;

    int16_t *h;                      // 16-bit
    uint16_t *H;

    int32_t *l;                 // 32-bit
    uint32_t *L;

    int64_t *q;            // 64-bit
    uint64_t *Q;

    float *f;                    // floats
    double *d;
    long double *g;
    unsigned long long int fhold;

    char *s;
    unsigned int len, maxstrlen=0, count;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
        case 'c': // 8-bit
            c = va_arg(ap, signed char*);
            if (*buf <= 0x7f) { *c = *buf;} // re-sign
            else { *c = -1 - (unsigned char)(0xffu - *buf); }
            buf++;
            break;

        case 'C': // 8-bit unsigned
            C = va_arg(ap, unsigned char*);
            *C = *buf++;
            break;

        case 'h': // 16-bit
            h = va_arg(ap, int16_t*);
            *h = unpacki16(buf);
            buf += 2;
            break;

        case 'H': // 16-bit unsigned
            H = va_arg(ap, uint16_t*);
            *H = unpacku16(buf);
            buf += 2;
            break;

        case 'l': // 32-bit
            l = va_arg(ap, int32_t*);
            *l = unpacki32(buf);
            buf += 4;
            break;

        case 'L': // 32-bit unsigned
            L = va_arg(ap, uint32_t*);
            *L = unpacku32(buf);
            buf += 4;
            break;

        case 'q': // 64-bit
            q = va_arg(ap, int64_t*);
            *q = unpacki64(buf);
            buf += 8;
            break;

        case 'Q': // 64-bit unsigned
            Q = va_arg(ap, uint64_t*);
            *Q = unpacku64(buf);
            buf += 8;
            break;

        case 'f': // float
            f = va_arg(ap, float*);
            fhold = unpacku16(buf);
            *f = unpack754_16(fhold);
            buf += 2;
            break;

        case 'd': // float-32
            d = va_arg(ap, double*);
            fhold = unpacku32(buf);
            *d = unpack754_32(fhold);
            buf += 4;
            break;

        case 'g': // float-64
            g = va_arg(ap, long double*);
            fhold = unpacku64(buf);
            *g = unpack754_64(fhold);
            buf += 8;
            break;

        case 's': // string
            s = va_arg(ap, char*);
            len = unpacku16(buf);
            buf += 2;
            if (maxstrlen > 0 && len >= maxstrlen) count = maxstrlen - 1;
            else count = len;
            memcpy(s, buf, count);
            s[count] = '\0';
            buf += len;
            break;

        default:
            if (isdigit(*format)) { // track max str len
                maxstrlen = maxstrlen * 10 + (*format-'0');
            }
        }

        if (!isdigit(*format)) maxstrlen = 0;
    }

    va_end(ap);
}

inline void 
serialize_test(){
    unsigned char buf[1024];
    int8_t magic;
    int16_t monkeycount;
    int32_t altitude;
    float absurdityfactor;
    const char *s = "Great unmitigated Zot! You've found the Runestaff!";
    char s2[96];
    int16_t packetsize, ps2;

    packetsize = pack(buf, "chhlsf", (int8_t)'B', (int16_t)0, (int16_t)37, 
            (int32_t)-5, s, (float)-3490.6677);
    packi16(buf+1, packetsize); // store packet size in packet for kicks

    printf("packet is %" PRId32 " bytes\n", packetsize);

    unpack(buf, "chhl96sf", &magic, &ps2, &monkeycount, &altitude, s2,
        &absurdityfactor);

    printf("'%c' %" PRId32" %" PRId16 " %" PRId32
            " \"%s\" %f\n", magic, ps2, monkeycount,
            altitude, s2, absurdityfactor);

}


inline void extraPackTest(){
    
    //just to demonstrate how we can put extra information in the message for the reciever to gradually deconstruct

    uint64_t now = GetTimeMS();
    //0 used to be GameState->frameCount but we dont have that here
    uint16_t sequence = 0 & 0xFFFF;

    uint16_t magicNumber = 0x5005;//soos
    
    int total_bytes_sent = 0;
    unsigned char buffer[256];

    const char* extra = "HQH";

    const char* msg = "SHIT MYSELF";
    int len = strlen(msg) + 1;
    int extraLen = strlen(extra)+1;

    uint64_t now2 =0;
    uint16_t sequence2 =1;
    uint16_t magic2 =2;


    // pack it all together
    // int packetsize = pack(buffer, "HQH16s4sHQH", sequence, now, magicNumber, msg, extra, sequence2, now2, magic2);
    
    //or in pieces
    int packetsize = pack(buffer, "HQH16s4s", sequence, now, magicNumber, msg, extra);
    int packetSize2 = pack(buffer + (12 + len + extraLen + 2), extra, sequence2, now2, magic2);
    //12 because uint64 + uint16 + uint16 = 12, +2 because of the string length preceding the strings in the buffer
    uint64_t getNow = 0;
    uint16_t getSequence = 0;
    uint16_t getMagicNumber = 0;
    
    uint64_t now3 =0;
    uint16_t sequence3 =0;
    uint16_t magic3 =0;

    char msg2[16];
    char extra2[16];
    unpack(buffer, "HQH16s4s", &getSequence,&getNow, &getMagicNumber, msg2, extra2);

    printf("test unpack, packetsize: %d, now: %" UINT64_FORMAT ", sequence; %d, magicNumber: %d, %s, %s\n", packetsize, getNow, getSequence, getMagicNumber, msg2, extra2);

    unpack(buffer + (12 + len + extraLen + 2), extra2, &sequence3, &now3, &magic3);
    printf("extra: %" UINT64_FORMAT ", %d, %d\n", now3, sequence3, magic3);

}

