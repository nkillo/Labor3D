#ifndef STRUTIL_H
#define STRUTIL_H


static uint32_t hash_str(const char* key){
    uint32_t hash = 2166136261u;
    for(int i = 0; key[i]; i++){
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}   


static inline int handmade_strlen(const char* given){
    int size = 0;
    while(given[size] != '\0'/*  && given[size] != '\n' */){
        size++;
    } 
    return size;
}




static inline size_t handmade_strcpy(char* dest, const char* src){
    char* original_dest = dest;
    size_t count = 0;
    while(*src != '\0'){
        *dest = *src;
        dest++;
        src++;
        count++;
    }
    *dest = '\0';

    return count;
}


static inline size_t handmade_len_strcpy(char* dest, const char* src, size_t len){
    char* original_dest = dest;
    size_t count = 0;
    while(*src != '\0' && count < len){
        *dest = *src;
        dest++;
        src++;
        count++;
    }
    *dest = '\0';

    return count;
}



static inline bool handmade_strcmp(const char* given, const char* compared){
    size_t given_len = handmade_strlen(given);
    size_t compared_len = handmade_strlen(compared);
    
    if(given_len != compared_len)return false;
    
    for (size_t i = 0; i < given_len; i++)
    {
        if(given[i] != compared[i]){
            return false;
        }
    }
    return true;
}

static inline bool handmade_len_strcmp(const char* given, const char* compared, size_t len){
    
    for (size_t i = 0; i < len; i++)
    {
        if(given[i] != compared[i]){
            return false;
        }
    }
    return true;
}

static inline size_t handmade_strcat(char* given, const char* str1, const char* str2){
    size_t offset = handmade_strcpy(given, str1);
    offset += handmade_strcpy(given + offset, str2);
    return offset;
}

bool isNumeric(char c){
    bool result = (c >= '0' && c <= '9');
    return result;
}

bool isAlpha(char c){
    bool result = (c >= 'a' && c <= 'z' ||
                   c >= 'A' && c <= 'Z');
    return result;
}

bool isEndOfLine(char c){
    bool result = (c == '\n' || c == '\r');
    return result;
}

bool isWhiteSpace(char c){
    bool result = (c == ' ' || c == '\t' || isEndOfLine(c));
    return result;
}

static inline int lenToWhitespace(const char* given){
    int size = 0;
    while(given[size] != '\0'  && !isWhiteSpace(given[size])){
        size++;
    } 
    return size;
}

static inline size_t lenToNonNumber(const char* given){
    size_t size = 0;
    while(given[size] == '.' || given[size] == '-'  || given[size] == '+'  || isNumeric(given[size])){
        size++;
    } 
    return size;
}



enum number_type{
    number_none,
    number_int,
    number_float,
};

struct number{
    union{
        int i_value;
        float f_value;
    }value;
    number_type type;
};

//returns the position of the symbol
int str_find_char(char* str, char c, char* max){
    int loc = 0;
    bool found = false;
    if(!str || *str == '\0')return -1;
    
    while(*(str + loc) != '\0' && str < max){
        if(*(str + loc) == c){
            found = true;
            break;   
        }
        loc++;
    }
    if(found)return loc;
    
    return -1;
}

char* char_str_find_char(char* str, char c, char* max){
    if(!str || *str == '\0')return nullptr;

    char* p = str;
    while(*p != '\0' && str < max){
        if(*p == c)return p; //found, return address
        p++;
    }
    return nullptr;
}


int string_to_int(const char* str, const char** endOfNum){
    int result = 0; 
    const char* current = str;
    if(!current || *current == '\0' || *current == '/'){ //early return
        *endOfNum = current;
        return result;
    }

    bool negative = false;
    
    if(*current == '-'){
        negative = true;
        current++;
    }

    else if(*current == '+'){
        negative = false;
        current++;
    }
    
    const char* p = current;
    int integer = 0;
    while(isNumeric(*p)){
        //parse whole number, go as long as we have numbers

        if(integer > (INT_MAX - (*p - '0')) / 10){
            result = negative ? INT_MIN : INT_MAX;
            *endOfNum = p;
            return result;
        }

        integer = integer * 10 + (*p - '0');
        p++;
    }
    if(*p == '.'){
        float faction = 0.0f;
        float divisor = 1.0f;
        p++;
        while(isNumeric(*p)){
            //TODO: maybe add some checks to restrict the chars we parse if the fractional part has too many chars
            divisor *= 10.0f;
            faction = faction * 10.0f + (*p - '0');
            p++;            

        }

        //combine the parts
        float value = (float)integer + (faction / divisor);

        if(negative)value = -value;

        result = (int)value;

        //we have a float
    }else{
        //we finished parsing the number
        int value = integer;
        if(negative)value = -value;
        result = value;
    }

    *endOfNum = p;
    
    return result;

}

//for parsing a file all at once, which is why we split between whitespaces
//this really only works for the .obj file im learning to parse, nothing else lol
number string_to_number(char* str, char** endOfNum){
    number result = {};
    result.type = number_none;

    if(!str || *str == '\0' || *str == '/'){ //early return
        *endOfNum = str;
        return result;
    }

    bool negative = false;
    
    if(*str == '-'){
        negative = true;
        str++;
    }

    else if(*str == '+'){
        negative = false;
        str++;
    }
    
    char* p = str;
    int integer = 0;
    while(isNumeric(*p)){
        //parse whole number, go as long as we have numbers

        if(integer > (INT_MAX - (*p - '0')) / 10){
            result.value.i_value = negative ? INT_MIN : INT_MAX;
            result.type = number_int;
            *endOfNum = p;
            return result;
        }

        integer = integer * 10 + (*p - '0');
        p++;
    }
    if(*p == '.'){
        float faction = 0.0f;
        float divisor = 1.0f;
        p++;
        while(isNumeric(*p)){
            //TODO: maybe add some checks to restrict the chars we parse if the fractional part has too many chars
            divisor *= 10.0f;
            faction = faction * 10.0f + (*p - '0');
            p++;            

        }

        //combine the parts
        float value = (float)integer + (faction / divisor);

        if(negative)value = -value;

        result.value.f_value = value;
        result.type = number_float;

        //we have a float
    }else{
        //we finished parsing the number
        int value = integer;
        if(negative)value = -value;
        result.value.i_value = value;
        result.type = number_int;
    }

    *endOfNum = p;
    
    return result;

}


int int_to_string(int value, char* buffer, int buffer_size){
    if(!buffer || buffer_size <= 0)return 0;//invalid buffer

    //handle negative numbers
    bool negative = false;
    if(value < 0){
        negative = true;
        value = -value;

        //special case for INT_MIN to avoid overflow in negation
        if(value < 0){
            if(buffer_size >= 12){//-2147483648 + null terminator
                handmade_strcpy(buffer, "-2147483648");
                return 11;
            }else return 0;//buffer too small
        }
    }

    //find out how many digits we need
    int temp = value;
    int num_digits = 0;
    do{
        num_digits++;
        temp /= 10;
    }while (temp > 0);


    //check if we have enough space for digits + negative sign + null terminator
    int required_size = num_digits + (negative ? 1 : 0) + 1;
    if(buffer_size < required_size){
        return 0; //buffer too small
    }


    //null terminate the string
    buffer[num_digits + (negative ? 1 : 0)] = '\0';

    //fill in the digits from right to left
    int pos = num_digits + (negative ? 1 : 0) - 1;
    do{
        buffer[pos--] = '0' + (value % 10);
        value /= 10;
    }while (value > 0);

    //add negative sign if needed
    if(negative){
        buffer[0] = '-';
    }
    

    return num_digits + (negative ? 1 : 0);
}

int float_to_string(float value, char* buffer, int buffer_size, int precision){
    if(!buffer || buffer_size <= 0 || precision < 0)return 0; //invalid parameters
    
    //handle negative numbers
    bool negative = false;
    if(value < 0){
        negative = true;
        value = -value;
    }

    //extract integer part
    uint32_t int_part = (uint32_t)value;
    
    //calculate the fractional part based on precision 
    float scaled_frac = value - (float)int_part;
    
    int threshold = 1;
    for(int i = 0; i < precision; i++){
        threshold *= 10;
        scaled_frac *= 10;
    }
    
    uint32_t frac_part = (uint32_t)(scaled_frac + 0.5f);//round to nearest whole
    
    //frac part can be higher if the precision value given is large
    //100.10, precision of more than 2, float of .1 is more like .103, * (10 * 3) = 103 > 100, so we would round in this case below
    //handle carry from rounding
    if(frac_part >= threshold){ 
        int_part += 1;
        frac_part = 0;
    }

    //convert integer part to string
    int int_len = 0;
    uint32_t temp = int_part;
    do{
        int_len++;
        temp /= 10;
    }while(temp > 0);

    //calculate required buffer size
    int required_size = int_len + (precision > 0 ? precision + 1 : 0) + (negative ? 1 : 0) + 1;
    
    if(buffer_size < required_size)return 0;//buffer size too small

    //fill in the string from right to left
    int pos = required_size - 1;
    buffer[pos--] = '\0'; //null terminator

    //fill in the fractional part
    if(precision > 0){
        for(int i = 0; i < precision; i++){
            buffer[pos--] = '0' + (frac_part % 10);
            frac_part /= 10;
        }
        buffer[pos--] = '.';
    }

    do{
        buffer[pos--] = '0' + (int_part % 10);
        int_part /= 10;
    }while(int_part > 0);

    if(negative){
        buffer[pos--] = '-';
    }

    return required_size - 1;
}

#endif