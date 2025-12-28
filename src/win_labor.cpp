#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <gl/GL.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>




#include "constants.h"
#include "math.h"

#include "networkState.h"
#include "labor_platform.h"
#include "threadPool.h"
#include "win_labor.h"

#include "strUtil.h"
#include "networkProtocol.cpp"
#include "server.h"
#include "server.cpp"
#include "client.h"
#include "client.cpp"

#include "network.h"
#include "network.cpp"
#include "netTest.cpp"
#include "packet.cpp"


#if LABOR_INTERNAL
    global_variable debug_table GlobalDebugTable_;
    debug_table* GlobalDebugTable = &GlobalDebugTable_;
#endif

global_variable HINSTANCE g_instance;
global_variable HWND g_window;
global_variable HDC g_device_context;
global_variable HGLRC gl_context;
global_variable u32 g_window_resized;
global_variable b32 g_window_minimized;
global_variable u32 g_width;
global_variable u32 g_height;
global_variable s32 g_mouseX;
global_variable s32 g_mouseY;
global_variable s32 g_mouseDeltaX;
global_variable s32 g_mouseDeltaY;


#include "vkTri.cpp"

#include "cgltfTest.h"
#include "cgltfTest.cpp"



static void catStrings(size_t source_a_count, char* source_a, size_t source_b_count, char* source_b, size_t dest_count, char* dest){
    for(int i = 0; i < source_a_count; i++){
        *dest++ = *source_a++;
    }
    for(int i = 0; i < source_b_count; i++){
        *dest++ = *source_b++;
    }
    *dest++ = 0;
}
static void Win32GetEXEFileName(platform_state* state){
    DWORD filename_size = GetModuleFileNameA(0, state->exe_filename, sizeof(state->exe_filename));
    state->one_past_last_exe_filename_slash = state->exe_filename;
    for(char* scan = state->exe_filename; *scan; ++scan){
        if(*scan == '\\'){
            //comes out to win_labor.exe
            state->one_past_last_exe_filename_slash = scan + 1;
        }
    }
}

static void Win32BuildEXEPathFileName(platform_state* state, char* filename, int destSize, char* dest){
    size_t test = state->one_past_last_exe_filename_slash - state->exe_filename;
    catStrings(state->one_past_last_exe_filename_slash - state->exe_filename, state->exe_filename, handmade_strlen(filename), filename, destSize, dest);
    int fuck_the_debugger = 0;
}




global_variable bool g_running = true;

global_variable s64 GlobalPerfCountFrequency;

LARGE_INTEGER GlobalStartCounter; // Store the start time when app initializes

// Initialize timing system - call once at program start
void Win32InitTiming() {
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    GlobalPerfCountFrequency = perfCountFrequencyResult.QuadPart;
    
    // Store the app start time
    QueryPerformanceCounter(&GlobalStartCounter);
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end){
    float delta = ((float)(end.QuadPart - start.QuadPart) / (float)(GlobalPerfCountFrequency));
    return delta;
}

// Get absolute seconds elapsed since program start
inline float Win32GetAbsoluteSecondsElapsed() {
    LARGE_INTEGER currentCounter;
    QueryPerformanceCounter(&currentCounter);
    
    float secondsElapsedSinceStart = 
        (float)(currentCounter.QuadPart - GlobalStartCounter.QuadPart) / 
        (float)GlobalPerfCountFrequency;
    
    return secondsElapsedSinceStart;
}


//WGL extensions
typedef HGLRC (WINAPI *wglCreateContextAttribsARBProc)(HDC, HGLRC, const int*);
typedef BOOL  (WINAPI *wglChoosePixelFormatARBProc)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
typedef BOOL  (WINAPI *wglSwapIntervalEXTProc)(int);

global_variable wglCreateContextAttribsARBProc wglCreateContextAttribsARB;
global_variable wglSwapIntervalEXTProc wglSwapIntervalEXT;

LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    switch (msg) {
        case WM_CLOSE: g_running = false; return 0;
        case WM_KEYDOWN:{
            if(wparam == VK_ESCAPE){
                g_running = false; return 0;
            }
            return 0;
        }break;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_ACTIVATE: {
            //if the window becomes inactive, dont pause the app
            if(LOWORD(wparam == WA_INACTIVE)){
                printf("Window focus lost!\n");
            }
            return 0;
        }
        case WM_SETFOCUS:
            // Window regained focus
            printf("Window got focus\n");
            break;
        case WM_KILLFOCUS:
            // Window lost focus
            printf("Window lost focus\n");
            break;
            //window specific message, caught by the window callback
        case WM_SIZE:{
                g_window_resized++;
                g_window_minimized = false;
                g_width = LOWORD(lparam);
                g_height = HIWORD(lparam);
                printf("Window resized to %d %d\n", g_width, g_height);
                if(!g_width && !g_height){
                    g_window_minimized = true;
                }
        }break;
        case WM_GETMINMAXINFO:{
            MINMAXINFO* info = (MINMAXINFO*)lparam;
            //set minimum client area size
            RECT minRect = {0, 0, 1, 1};
            DWORD style = GetWindowLong(hwnd, GWL_STYLE);
            AdjustWindowRect(&minRect, style, FALSE);
            info->ptMinTrackSize.x = minRect.right - minRect.left;
            info->ptMinTrackSize.y = minRect.bottom - minRect.top;
            return 0;
        }break;

        // case WM_SETCURSOR:{
        //dont handle this if we dont have different cursor drawing modes        
        //     //triggers whenever the cursor moves
        //     // printf("set cursor event?\n");
        // }break;

        default: return DefWindowProc(hwnd, msg, wparam, lparam);

        
    }
    return 0;
}

void
win32_create_opengl_context(){
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    g_device_context = GetDC(g_window);
    int pixel_format = ChoosePixelFormat(g_device_context, &pfd);
    SetPixelFormat(g_device_context, pixel_format, &pfd);

    //temp context to load wgl extensions
    HGLRC temp_context = wglCreateContext(g_device_context);
    wglMakeCurrent(g_device_context, temp_context);

    wglCreateContextAttribsARB = (wglCreateContextAttribsARBProc)wglGetProcAddress("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (wglSwapIntervalEXTProc)wglGetProcAddress("wglSwapIntervalEXT"); 

    int attribs[] = {
        0x2091, 4, //WGL_CONTEXT_MAJOR_VERSION_ARB
        0x2092, 6, //WGL_CONTEXT_MINOR_VERSION_ARB
        0x9126, 0x00000001, //WGL_CONTEXT_PROFILE_MASK_ARB, CORE
        0
    };
    gl_context = wglCreateContextAttribsARB(g_device_context, 0, attribs);
    wglMakeCurrent(0, 0);
    wglDeleteContext(temp_context);
    wglMakeCurrent(g_device_context, gl_context);

    if(wglSwapIntervalEXT){
        wglSwapIntervalEXT(1);//enable vsync
    }

}


void win32_create_window(const char* title, int width, int height){
    WNDCLASS wc = {};
    wc.lpfnWndProc = win32_window_proc;
    wc.hInstance = g_instance;
    wc.lpszClassName = "GameWindowClass";
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); //set background to black

    RegisterClass(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, style, false);

    g_window = CreateWindowEx(
        0, wc.lpszClassName, title, style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        0, 0, g_instance, 0
    );

    //set minimum window size (1x1 client area minimum)
    RECT minRect = {0, 0, 1, 1};
    AdjustWindowRect(&minRect, style, false);
    int minWidth = minRect.right - minRect.left;
    int minHeight = minRect.bottom - minRect.top;

    SetWindowLongPtr(g_window, GWL_STYLE, style);
    SetWindowPos(g_window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    //set minimum tracking size
    MINMAXINFO mmi;
    mmi.ptMinTrackSize.x = minWidth;
    mmi.ptMinTrackSize.y = minHeight;
    SetWindowLongPtr(g_window, GWLP_USERDATA, (LONG_PTR)&mmi);
}
static void process_button(player_input* newInput, InputTypes type, uint16_t* key, b32 endedDown){
    if(*key != endedDown){
        *key = endedDown;
        newInput->transitionCounts[type]++;
    }

}


static void
win_process_pending_messages(platform_state* platformState, game_memory* GameMemory, player_input* new_input){
    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
        switch(msg.message){
            case WM_QUIT:
            case WM_CLOSE:{
                g_running = false;
            }break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:{
                u32 VkCode = (u32)msg.wParam;
                b32 wasDown = ((msg.lParam & (1 << 30)) != 0);
                b32 isDown = ((msg.lParam & (1 << 31)) == 0);
                switch(VkCode){
                    case VK_ESCAPE:{
                        g_running = false;
                        if(new_input->function.keyESCAPE != isDown){new_input->function.keyESCAPE = isDown; new_input->transitionCounts[InputTypes::input_key_escape]++;}
                    }break;
                    case VK_SHIFT:{
                        if(new_input->function.shift != isDown){new_input->function.shift = isDown; new_input->transitionCounts[InputTypes::input_key_shift]++;}
                    }break;
                    case 'W':{
                        if(new_input->bits.forward != isDown){new_input->bits.forward = isDown; new_input->transitionCounts[InputTypes::input_key_forward]++;}
                    }break;
                    case 'A':{
                        if(new_input->bits.left != isDown){new_input->bits.left = isDown; new_input->transitionCounts[InputTypes::input_key_left]++;}
                    }break;
                    case 'S':{
                         if(new_input->bits.back != isDown){new_input->bits.back = isDown; new_input->transitionCounts[InputTypes::input_key_back]++;}
                    }break;
                    case 'D':{
                         if(new_input->bits.right != isDown){new_input->bits.right = isDown; new_input->transitionCounts[InputTypes::input_key_right]++;}
                    }break;
                    case 'Q':{
                         if(new_input->flags.freeCam != isDown){new_input->flags.freeCam = isDown; new_input->transitionCounts[InputTypes::input_key_Q]++;}
                    }break;
                    case 'E':{
                         if(new_input->bits.interact != isDown){new_input->bits.interact = isDown; new_input->transitionCounts[InputTypes::input_key_interact]++;}
                    }break;
                    case '1':{if(new_input->numbers.key1 != isDown){new_input->numbers.key1 = isDown; new_input->transitionCounts[InputTypes::input_key_1]++;}}break;
                    case '2':{if(new_input->numbers.key2 != isDown){new_input->numbers.key2 = isDown; new_input->transitionCounts[InputTypes::input_key_2]++;}}break;
                    case '3':{if(new_input->numbers.key3 != isDown){new_input->numbers.key3 = isDown; new_input->transitionCounts[InputTypes::input_key_3]++;}}break;
                    case '4':{if(new_input->numbers.key4 != isDown){new_input->numbers.key4 = isDown; new_input->transitionCounts[InputTypes::input_key_4]++;}}break;
                    case '5':{if(new_input->numbers.key5 != isDown){new_input->numbers.key5 = isDown; new_input->transitionCounts[InputTypes::input_key_5]++;}}break;
                    case '6':{if(new_input->numbers.key6 != isDown){new_input->numbers.key6 = isDown; new_input->transitionCounts[InputTypes::input_key_6]++;}}break;
                    case '7':{if(new_input->numbers.key7 != isDown){new_input->numbers.key7 = isDown; new_input->transitionCounts[InputTypes::input_key_7]++;}}break;
                    case '8':{if(new_input->numbers.key8 != isDown){new_input->numbers.key8 = isDown; new_input->transitionCounts[InputTypes::input_key_8]++;}}break;
                    case '9':{if(new_input->numbers.key9 != isDown){new_input->numbers.key9 = isDown; new_input->transitionCounts[InputTypes::input_key_9]++;}}break;
                    case '0':{if(new_input->numbers.key0 != isDown){new_input->numbers.key0 = isDown; new_input->transitionCounts[InputTypes::input_key_0]++;}}break;
                    case VK_CONTROL:{
                         if(new_input->bits.down != isDown){new_input->bits.down = isDown; new_input->transitionCounts[InputTypes::input_key_down]++;}
                         if(new_input->function.keyCTRL != isDown){new_input->function.keyCTRL = isDown; new_input->transitionCounts[InputTypes::input_key_ctrl]++;}
                    }break;
                    case VK_MENU:{
                         if(new_input->function.alt != isDown){new_input->function.alt = isDown; new_input->transitionCounts[InputTypes::input_key_alt]++;}
                    }break;
                    case VK_SPACE:{
                         if(new_input->bits.up != isDown){new_input->bits.up = isDown; new_input->transitionCounts[InputTypes::input_key_up]++;}

                    }break;
                    case VK_TAB:{
                         if(new_input->function.keyTAB != isDown){new_input->function.keyTAB = isDown; new_input->transitionCounts[InputTypes::input_key_tab]++;}
                    }break;
                    case VK_RETURN:{
                        if(new_input->function.keyReturn != isDown){
                            new_input->function.keyReturn = isDown; new_input->transitionCounts[InputTypes::input_key_return]++;
                            // if(isDown){//game layer should handle this
                            //     GameMemory->textInputEnabled = !GameMemory->textInputEnabled;
                            //     printf("toggled text input: %d\n", GameMemory->textInputEnabled);

                            // }
                        }
                    }break;
                    case VK_UP   :{
                        if(new_input->function.up != isDown){new_input->function.up = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_up]++;}
                    }break;
                    case VK_DOWN :{
                        if(new_input->function.down != isDown){new_input->function.down = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_down]++;}
                    }break;
                    case VK_LEFT :{
                        if(new_input->function.left != isDown){new_input->function.left = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_left]++;}
                    }break;
                    case VK_RIGHT:{
                        if(new_input->function.right != isDown){new_input->function.right = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_right]++;}
                    }break;
                    case VK_OEM_PLUS:{
                        if(new_input->function.equals != isDown){new_input->function.equals = isDown; new_input->transitionCounts[InputTypes::input_key_equals]++;}
                    }break;
                    case VK_OEM_MINUS:{
                        if(new_input->function.minus != isDown){new_input->function.minus = isDown; new_input->transitionCounts[InputTypes::input_key_minus]++;}
                    }break;
                    case VK_PRIOR:{
                        if(new_input->function.pageUp != isDown){new_input->function.pageUp = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_pageUp]++;}
                    }break;
                    case VK_NEXT :{
                        if(new_input->function.pageDown != isDown){new_input->function.pageDown = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_pageDown]++;}
                    }break;
                    case VK_HOME :{
                        if(new_input->function.home != isDown){new_input->function.home = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_home]++;}
                    }break;
                    case VK_END  :{
                        if(new_input->function.end != isDown){new_input->function.end = isDown; new_input->transitionCounts[InputTypes::input_key_arrow_end]++;}
                    }break;

                }
                // if(GameMemory->textInputEnabled){
                    TranslateMessage(&msg);
                // }
                // printf("keydown: %u\n", VkCode);
                //if text input is on
                // TranslateMessage(&msg);

            }break;

        
            case WM_MOUSEWHEEL:{
                short delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
                if(delta > 0) {
                    // printf("Wheel up %d\n", delta);
                    new_input->mouse_wheel = 1;
                } else if(delta < 0) {
                    // printf("Wheel down %d\n", delta);
                    new_input->mouse_wheel = -1;
                }
            }break;
       
            case WM_CHAR:{
                // if(GameMemory->textInputEnabled){
                wchar_t wc = (wchar_t)msg.wParam;
                if(wc >= 0xD800 && wc <= 0xDBFF){
                    //high surrogate, store until we get the low surrogate
                    GameMemory->pendingHighSurrogate = wc;
                }else if(wc >= 0xDC00 && wc <= 0xDFFF){
                    //low surrogate, combine with pending high surrogate
                    if(GameMemory->pendingHighSurrogate != 0){
                        printf("processing low surrogate codepoint %d\n", wc);
                        u32 high = GameMemory->pendingHighSurrogate - 0xD800;
                        u32 low  = wc - 0xDC00;
                        u32 codepoint = (high << 10) + low + 0x10000;
                        GameMemory->codePointInput[GameMemory->textInputLen++] = codepoint;
                        GameMemory->pendingHighSurrogate = 0;

                    }else{
                        printf("orphan low surrogate? %d\n", wc);
                    }
                }else{

                    // printf("Got character %c (%d)\n", character, character);
                    GameMemory->codePointInput[GameMemory->textInputLen++] = wc;
                    // GameMemory->textInput[GameMemory->textInputLen++] = wc;
                    GameMemory->pendingHighSurrogate = 0;
                }
                // }
            }break;

            case WM_SETFOCUS:
                // Window regained focus
                printf("Window got focus\n");
                break;
            case WM_KILLFOCUS:
                // Window lost focus
                printf("Window lost focus\n");
                break;
            default:{
                // printf("Got message: %u\n", msg);
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }


}


void test_logger(FILE* log, int tick){
    fprintf(log, "Tick %d\n", tick++);
    fflush(log);


        // Get the current size of the file
    fseek(log, 0, SEEK_END);
    long file_size = ftell(log);
    
    // If the file size exceeds the threshold, clear the file
    if (file_size > 1000) {
        freopen("test_log.txt", "w", log); // Truncate the file by reopening it in write mode
    }

}




static win32_game_code Win32LoadGameCode(char* sourceDllName, char* tempDllName, char* lockFileName){
    win32_game_code code = {};
    printf("Win32 Load Game Code!\n");
    WIN32_FILE_ATTRIBUTE_DATA ignored;
    if(!GetFileAttributesEx(lockFileName, GetFileExInfoStandard, &ignored)){
        code.DLLLastWriteTime = Win32GetLastWriteTime(sourceDllName);
        CopyFile(sourceDllName, tempDllName, FALSE);

        code.GameCodeDLL = LoadLibraryA(tempDllName);
        if(code.GameCodeDLL){
            code.updateAndRender = (game_update_and_render*)GetProcAddress(code.GameCodeDLL, "GameUpdateAndRender");
            code.fixedUpdate = (game_fixed_update*)GetProcAddress(code.GameCodeDLL, "GameFixedUpdate");
            
            code.debugGameFrameEnd = (debug_game_frame_end*)GetProcAddress(code.GameCodeDLL, "DEBUGGameFrameEnd");
            code.gameGetSoundSamples = (game_get_sound_samples*)GetProcAddress(code.GameCodeDLL, "GameGetSoundSamples");
            
            code.gameInit = (game_init*)GetProcAddress(code.GameCodeDLL, "GameInit");
            code.gameDrawUI = (game_draw_ui*)GetProcAddress(code.GameCodeDLL, "GameDrawUI");

            code.is_valid = code.updateAndRender && code.fixedUpdate && code.gameInit && code.debugGameFrameEnd && code.gameGetSoundSamples && code.gameDrawUI;
        }else{
            printf("failed to load library?\n");
        }
    }

    if(!code.is_valid){
        code.updateAndRender = 0;
        code.fixedUpdate = 0;
        code.debugGameFrameEnd = 0;
        code.gameGetSoundSamples = 0;
        code.gameInit = 0;
        code.gameDrawUI = 0;
    }

    return code;
}


static void Win32UnloadGameCode(win32_game_code *code){
    if(code->GameCodeDLL){
        FreeLibrary(code->GameCodeDLL);
        code->GameCodeDLL = 0;
    }
    code->is_valid = false;
    code->updateAndRender = 0;
    code->gameInit = 0;
}



// Input: sine value from normalized vector (-1 to 1)
fpt fpt_asin(fpt sine_value) {
    // sine_value is in fixed point (-1 to 1)
    // We need to scale it to 0-1024 range for table lookup
    int index = (sine_value * 1024) >> 16;
    
    // Handle negative input
    bool is_negative = sine_value < 0;
    if(is_negative) index = -index;
    
    // Clamp to valid range
    if(index > 1024) index = 1024;
    
    fpt angle = fpt_asin_table[index];
    if(is_negative) angle = -angle;
    
    return angle;
}


#ifdef _WIN32
HANDLE ConsoleHandle;
HANDLE NetworkHandle;
#else
pthread_t ConsoleHandle;
pthread_t NetworkHandle;
#endif   


//network threading


bool isServer = false;
bool isClient = false;
//client functions

void InitNetworkCommand(){
    NetworkCommand netcmd = {};
    netcmd.type = NetworkThreadCommandTypes::NETWORK_CMD_INIT;
    PushToNetwork(netcmd);
}
    
void StartHostNetworkCommand(){
    NetworkCommand netcmd = {};
    netcmd.type = NetworkThreadCommandTypes::NETWORK_CMD_START_HOST;
    PushToNetwork(netcmd);
}

    
void StartClientNetworkCommand(){
    NetworkCommand netcmd = {};
    netcmd.type = NetworkThreadCommandTypes::NETWORK_CMD_START_CLIENT;
    PushToNetwork(netcmd);
}

#ifndef SERVER_BUILD
void startListenServer(game_memory* GameMemory){
    isServer = true;
    StartHostNetworkCommand();
}

void startClient(game_memory* GameMemory){
    isClient = true;
    StartClientNetworkCommand();

}


void ClientNetworkThreadUpdate(game_memory* GameMemory){

    //SEND COMMANDS TO NETWORK THREAD
    NetworkCommand netCmd = {};

    while(NetworkPopToMain(&netCmd)){
        if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_EXIT){
            //tell network we are shutting down before setting isRunning to false
        }
        else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_BROADCAST){

        }
        else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_CONNECTING){
            printf("connecting to server...\n");
        }
        else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_CONNECTED){
            printf("successfully connnected!!\n");
            mainClientSocketState.isConnected = true;
            // netCmd.type = NetworkThreadCommandTypes::NETWORK_CMD_CONNECTED_HANDSHAKE;
            // PushToNetwork(netCmd);
        }
        else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_CONNECT_FAILED){
            mainClientSocketState.isConnected = false;
            printf("connection failed!!\n");


        }
        else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_DISCONNECTED){
            mainClientSocketState.isConnected = false;
            printf("disconnected!!\n");
            // if(gameInitNetwork)gameInitNetwork(GameMemory, sharedState);
            printf("restart network here??\n");
            

        }
        
    }


}

#endif



    void ServerNetworkThreadUpdate(game_memory* GameMemory){
    
        //SEND COMMANDS TO NETWORK THREAD
        NetworkCommand netCmd = {};

        while(NetworkPopToMain(&netCmd)){
            if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_EXIT){
                //probably need to send a message saying we are disconnecting or something
            }
            else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_BROADCAST){
                
            }
            else if(netCmd.type == NetworkThreadCommandTypes::NETWORK_CMD_DISCONNECTED){
                // if(gameInitNetwork)gameInitNetwork(GameMemory, sharedState);
                printf("restart network here??\n");
            }

        }


    }

//end network threading




//MULTITHREADING
static bool
DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    bool WeShouldSleep = false;

    uint32_t OriginalNextEntryToRead = Queue->NextEntryToRead;
    uint32_t NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);

    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        uint32_t Index = AtomicCompareExchange(&Queue->NextEntryToRead, NewNextEntryToRead, OriginalNextEntryToRead);

        if(Index == OriginalNextEntryToRead)
        {
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            CompletePastReadWritesBeforeFutureReadWrites;
            AtomicIncrement(&Queue->CompletionCount);

        }
    }
    else{
        WeShouldSleep = true;
    }

    return (WeShouldSleep);
}


static void
CompleteAllWork(platform_work_queue *Queue)
{
    while(Queue->CompletionGoal != Queue->CompletionCount)
    {
        DoNextWorkQueueEntry(Queue);
    }
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

static bool
CheckAllWorkCompleted(platform_work_queue *Queue)
{
    if(Queue->CompletionGoal == Queue->CompletionCount)
    {
        Queue->CompletionGoal = 0;
        Queue->CompletionCount = 0;
        return true;    // Work is completed
    }
    return false;   // Work is still in progress
}


void AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data){
    //TODO (nate) switch to AtomicCompareExchange eventually so any thread can add
    uint32_t NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    AtomicIncrement(&Queue->CompletionGoal);
    CompletePastWritesBeforeFutureWrites;
    
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleasePlatformSemaphore(Queue->SemaphoreHandle);
}





// static void
// PushString(platform_work_queue *Queue, const char *String)
// {
//     AddWorkQueueEntry(Queue, (void*)String);
// }



static PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork){

    #ifdef _WIN32
        // char Buffer[256];
        // wsprintf(Buffer, "Thread %u: %s\n", GetOSThreadID(), (char*)Data);
        // OutputDebugStringA(Buffer);
        printf("Thread %u: %s\n", GetOSThreadID(), (char*)Data);

    #else
        printf("Thread %u: %s\n", GetOSThreadID(), (char*)Data);
        fflush(stdout);
    #endif

}

#ifdef _WIN32
DWORD WINAPI ThreadProc(LPVOID Parameter)
#else
void* ThreadProc(void* Parameter)
#endif
{
    thread_info* ThreadInfo = (thread_info*)Parameter;
    AtomicIncrement(&ActiveThreadCount);
    
    DWORD_PTR procMask;
    DWORD_PTR sysMask;
    if(GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask)){
        PROCESSOR_NUMBER pn;
        GetCurrentProcessorNumberEx(&pn);
        printf("Thread %d actually running on Group %u CPU %u\n", ThreadInfo->LogicalThreadIndex, pn.Group, pn.Number);
    }

    while(!ShouldThreadsExit) {  // Check exit condition
        if(DoNextWorkQueueEntry(ThreadInfo->Queue))
        {
            AtomicAdd(&ActiveThreadCount, -1);  // Decrement before waiting
            WaitOnPlatformSemaphore(ThreadInfo->Queue->SemaphoreHandle);
            AtomicIncrement(&ActiveThreadCount);      // Increment after waking
        }
    }

    #ifdef _WIN32
        return 0;
    #else
        return NULL;
    #endif
}

atomic_uint32 EntryCompletionCount;
atomic_uint32 NextEntryToDo;
atomic_uint32 EntryCount;
atomic_uint32 ActiveThreadCount;
atomic_uint32 ShouldThreadsExit;
uint32_t ThreadCount = 0;

#define PARSE_TEST 0
#define VM_TEST 0
#define NOISE_TEST 0


#if PARSE_TEST
    #include "binary_test.cpp"
#endif

#if VM_TEST
    #include "vm_test.cpp"
#endif

#if NOISE_TEST
    #include "noiseTest.cpp"
#endif

int main(){
    Win32InitTiming();
#if PARSE_TEST
    parse_binary_test();
    return 0;
#endif
#if VM_TEST
    vm_test();
    return 0;
#endif
#if NOISE_TEST
    noise_test();
    return 0;
#endif
    printf("hello labor\n");



    //radix test
    //radix sort
    //find max time
    // u32 numCount = 8;
    // u32 max = 0;
    // u32 maxPos = 0;
    // u32 nums[8] = {212, 3, 2, 64, 832, 384, 320, 8};
    // u32 temp[8] = {};
    // u32 radixNums[8] = {};
    // for (u32 i = 0; i < numCount; i++) {
    //     printf("unsorted nums %d : %3u\n", i, nums[i]);
    //     u32 num = nums[i];
    //     if(max < num){max = num; maxPos = i;}
    //     radixNums[i] = num;
    // }
    // // __debugbreak();
    // for(int byte = 0; byte < 4; byte++){
    //     u32 count[256] = {};
    //     //count frequencies for this byte
    //     for(u32 i = 0; i < numCount; i++){
    //         u32 digit = (radixNums[i] >> (byte * 8)) & 0xFF;
    //         count[digit]++;
    //     }
    //     // __debugbreak();

    //     //prefix sum
    //     for(int i = 1; i < 256; i++){
    //         count[i] += count[i-1];
    //     }
    //     // for(int i = 254; i >= 0; i--){
    //         // count[i] += count[i+1];
    //     // }
    //     // __debugbreak();

    //     //build output (go backwards to preserve stability)
    //     for(int i = numCount - 1; i >= 0; i--){
    //     // for(int i = 0; i < numCount; i++){
    //         u32 digit = (radixNums[i] >> (byte * 8)) & 0xFF;
    //         temp[--count[digit]] = radixNums[i];
    //     }
    //     // __debugbreak();

    //     //copy back
    //     for(u32 i = 0; i < numCount; i++){
    //         radixNums[i] = temp[i];
    //     }


    // }
    // __debugbreak();

    //end radix test

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    platform_state platformState = {};
    game_input input[2] = {};
    game_input* newInput = &input[0];
    game_input* oldInput = &input[1];

    g_instance = GetModuleHandle(0);

    // g_width = 1280;
    // g_height = 720;

    //setup threads
    int reservedPhysicalCores = 2;
    int logicalThreadCount = 0;
    int physicalCoreCount = 0;
    int coreMax = 32;

    #ifdef _WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        int numCores = sysInfo.dwNumberOfProcessors;
        
        DWORD len = 0;


        GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
        printf("size of system logical processor information ex: %zu\n", sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX));
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buf[1024] = {};
        if(!GetLogicalProcessorInformationEx(RelationProcessorCore, buf, &len)){
            printf("Error: %lu\n", GetLastError());
            return 1;
        }

        u64 popcntTest1 = 0b0000000000000000000000000001000000000000000000000000000000000000;
        u64 popcntTest2 = 0b1000000000000000000000000000000000000000000000000000000000000001;
        u64 popcntTest3 = 0b1000000000000000000000000001000000000010000000000000000000000001;
        //sets the value purely on the number of set bits (1)s regardless of order
        popcntTest1 = __popcnt64(popcntTest1);
        popcntTest2 = __popcnt64(popcntTest2);
        popcntTest3 = __popcnt64(popcntTest3);

        int physicalCount = 0;
        int workerCount = 0;
        int logicalTotal = 0;
        char* ptr = (char*)buf;
        DWORD_PTR coreMasks[128] = {};
        DWORD_PTR workerMasks[128] = {};
        int availableWorkers = 0;
        int coreLogicalCounts[128] = {};
        while(ptr < (char*)buf + len){
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)ptr;
            if(info->Relationship == RelationProcessorCore){//RelationProcessorCore = one per physicalCore
                DWORD_PTR mask = 0;
                for(WORD g = 0; g < info->Processor.GroupCount; g++){
                    mask |= info->Processor.GroupMask[g].Mask;
                }
                coreMasks[physicalCount] = mask;
                coreLogicalCounts[physicalCount] = __popcnt64(mask);//count bits set = logical threads;
                logicalTotal += coreLogicalCounts[physicalCount];
                KAFFINITY maskk = info->Processor.GroupMask[0].Mask;
                printf("Physical core: mask = 0x%llu\n", (unsigned long long)maskk); 
                physicalCount++;
            }
            ptr += info->Size;
        }

        workerCount = physicalCount;
        printf("Physical Cores: %d\n", physicalCount);
        printf("Logical total : %d\n", logicalTotal);

        //print mapping
        for(int i = 0; i < physicalCount; i++){
            printf("Physical core %d: mask:=0x%llx, logicals=%d\n", i, (unsigned long long)coreMasks[i], coreLogicalCounts[i]);
        }
        //build worker affinity mask
        DWORD_PTR workerMask = 0;
        //leave last physical core completely unused for GPU driver
        for(int i = 0; i < physicalCount - 1; i++){
            //use only one logical thread per physical core for workers
            DWORD_PTR m = coreMasks[i];
            DWORD_PTR oneLogical = m & -m; //pick lowest set bit
            //pick lowest bit per core
            //should we check if theres 2 cores first? if(coreLogicalCounts[i] >= 2){}
            workerMask |= oneLogical;

            if(!m)continue;
            workerMasks[availableWorkers++] = oneLogical;

        }

        printf("worker thread affinity mask: 0x%llx\n", (unsigned long long)workerMask);

        // __debugbreak();

        //intersect with process affinity in case user changed it
        DWORD_PTR procMask = 0;
        DWORD_PTR sysMask = 0;
        if(GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask)){
            for(int i = 0; i < availableWorkers; i++){
                workerMasks[i] &= procMask;
            }
        }

        //drop masks that became zero after intersection
        int compact = 0;
        for(int i = 0; i < availableWorkers; i++){
            if(workerMasks[i])workerMasks[compact++] = workerMasks[i];
        }
        availableWorkers = compact;

    #else
        int numCores = sysconf(_SC_NPROCESSORS_ONLN);
    #endif
    printf("numCores: %d\n",numCores);


    EntryCompletionCount = 0;
    NextEntryToDo = 0;
    EntryCount = 0;
    ActiveThreadCount = 0;
    ShouldThreadsExit = 0;

    platform_work_queue Queue = {};

    thread_info ThreadInfo[MAX_CORES];

    // uint32_t ThreadCount = ArrayCount(ThreadInfo);
    ThreadCount = (numCores - 2) >= 1 ? numCores - 2 : 0;

    uint32_t InitialCount = 0;
    ThreadCount = ThreadCount > MAX_CORES ? MAX_CORES : ThreadCount;
    ThreadCount = physicalCount;
    ThreadCount = ThreadCount > reservedPhysicalCores ? ThreadCount - reservedPhysicalCores : 1;

    Queue.SemaphoreHandle = CreatePlatformSemaphore(0, ThreadCount);

    #ifdef _WIN32
        DWORD_PTR availableMask = workerMask;
        DWORD_PTR usedMask = 0;
        HANDLE ThreadHandles[MAX_CORES];
        DWORD ThreadIDs[MAX_CORES];
    #else
        pthread_t ThreadHandles[MAX_CORES];
    #endif

    for(int ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex) {
        DWORD_PTR  mask = workerMasks[ThreadIndex];
        if(!mask){printf("INVALID WORKER MASK AT THREAD INDEX: %d, SKIPPING!\n", ThreadIndex); continue;}

        thread_info* Info = &ThreadInfo[ThreadIndex];
        Info->Queue = &Queue;
        Info->LogicalThreadIndex = ThreadIndex;

        #ifdef _WIN32
            ThreadHandles[ThreadIndex] = CreateThread(0, 0, ThreadProc, Info, CREATE_SUSPENDED, &ThreadIDs[ThreadIndex]);
            DWORD_PTR prev = SetThreadAffinityMask(ThreadHandles[ThreadIndex], workerMasks[ThreadIndex]);
            if(!prev){
                printf("SetThreadAffinityMask failed %lu\n", GetLastError());
            }else{
                printf("Thread %d pinned to mask 0x%llx (previous mask = 0x%llx)\n", ThreadIndex, (unsigned long long)workerMasks[ThreadIndex], (unsigned long long)prev);
            }
            
            unsigned long idx = 0;
            int cpuIndex = _BitScanForward64(&idx, mask);
            SetThreadIdealProcessor(ThreadHandles[ThreadIndex], cpuIndex);
            ResumeThread(ThreadHandles[ThreadIndex]);


            // printf("created thread ID: %u\n", ThreadIDs[ThreadIndex]);
        #else
            pthread_create(&ThreadHandles[ThreadIndex], NULL, ThreadProc, Info);
        #endif
    }

    
    platform_state state = {};
    game_memory GameMemory = {};
    network_thread_state NetworkThreadState = {};

    size_t platformDataSize = sizeof(vkTri) + sizeof(gltf_data);//sizeof(mesh_data) + sizeof(shader_data) + (sizeof(network_state) * 3);// + /* other data sizes */;
    GameMemory.PlatformMemorySize = platformDataSize;
    // GameMemory.PermanentStorageSize = Gigabytes(3.5);
    GameMemory.PermanentStorageSize = Gigabytes(2);
    GameMemory.TransientStorageSize = Megabytes(64); //was Gigabytes(1) before
    GameMemory.DebugStorageSize = Megabytes(64);
    state.TotalSize = GameMemory.PlatformMemorySize + GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize + GameMemory.DebugStorageSize;
    // state.TotalMemoryBlock = malloc(state.TotalSize);
    platform_allocate_memory* allocate = Win32AllocateMemory;
    platform_deallocate_memory* deallocate = Win32DeallocateMemory;
    // state.TotalMemoryBlock = allocate(state.TotalSize);
    state.TotalMemoryBlock = plat_alloc_mem(state.TotalSize);
    // state.TotalMemoryBlock = VirtualAlloc(0, (size_t)state.TotalSize,
    //                                     MEM_RESERVE|MEM_COMMIT,
    //                                     PAGE_READWRITE);

    uint8_t* currentPtr = (uint8_t*)state.TotalMemoryBlock;
    
    memset(currentPtr, 0, state.TotalSize);
    
    vkTri* tri = (vkTri*)currentPtr;
    currentPtr += sizeof(vkTri);

    gltf_data* gltfData = (gltf_data*)currentPtr;
    currentPtr += sizeof(gltf_data);

    // Properly construct the objects in place
    // mesh_data* meshData = new(currentPtr) mesh_data();  // Placement new for proper initialization
    // currentPtr += sizeof(mesh_data);

    // shader_data* shaderData = new(currentPtr) shader_data();  // Placement new
    // currentPtr += sizeof(shader_data);

    // network_state* SharedNetworkStates = new(currentPtr) network_state[3]();
    // currentPtr += sizeof(network_state) * 3;
    
    GameMemory.PermanentStorage = (void*)currentPtr;
    currentPtr += GameMemory.PermanentStorageSize;
    
    GameMemory.TransientStorage = (void*)currentPtr;
    currentPtr += GameMemory.TransientStorageSize;
    
    GameMemory.DebugStorage     = (void*)currentPtr;

    
    g_width = 480;
    g_height = 360;


    
    GameMemory.width = g_width;
    GameMemory.height = g_height;

    GameMemory.threadCount = ThreadCount;

    GameMemory.HighPriorityQueue = &Queue;
    GameMemory.PlatformAddEntry = AddEntry;
    GameMemory.PlatformCompleteAllWork = CompleteAllWork;
    GameMemory.PlatformCheckAllWorkCompleted = CheckAllWorkCompleted;
    
    GameMemory.platform_fpt_asin = fpt_asin;
    thread_info ConsoleInfo = {};

#ifdef _WIN32
    DWORD ConsoleThreadID;
    ConsoleHandle = CreateThread(0, 0, ConsoleThreadProc, &ConsoleInfo, 0, &ConsoleThreadID);

    DWORD NetworkThreadID;
    NetworkHandle = CreateThread(0, 0, NetworkThreadProc, &NetworkThreadState, 0, &NetworkThreadID);
#else
    ConsoleHandle;
    pthread_create(&ConsoleHandle, NULL, ConsoleThreadProc, &ConsoleInfo);

    NetworkHandle;
    pthread_create(&NetworkHandle, NULL, NetworkThreadProc, &NetworkThreadState);
#endif



    win32_create_window("Raw Win32 Vulkan", g_width, g_height);
    

    // win32_create_opengl_context();

    FILE* log = fopen("test_log.txt", "w");
    if(!log){
        MessageBoxA(0, "Failed to open log file",  "ERROR", MB_OK | MB_ICONERROR);
        return -1;
    }

    int tick = 0;

    Win32GetEXEFileName(&state);

    char sourceGameCodeDLLFullPath[MAX_PATH];
    Win32BuildEXEPathFileName(&state, "labor.dll", sizeof(sourceGameCodeDLLFullPath), sourceGameCodeDLLFullPath);
    char tempGameCodePath[MAX_PATH];
    Win32BuildEXEPathFileName(&state, "labor_temp.dll", sizeof(tempGameCodePath), tempGameCodePath);
    char gameCodeLockPath[MAX_PATH];
    Win32BuildEXEPathFileName(&state, "lock.tmp", sizeof(gameCodeLockPath), gameCodeLockPath);

    char* build_folder = "build";
    size_t build_size = 5;
    char main_folder[MAX_PATH];
    char* filename = state.exe_filename;
    size_t test_count = state.one_past_last_exe_filename_slash - state.exe_filename - 1 - build_size; //- 1 for the slash
    for(int i = 0; i < test_count; i++){
        main_folder[i] = *filename++;
    }
    main_folder[test_count] = 0;

    printf("main folder: %s\n", main_folder);
    int fuck_the_debugger = 0;

    GameMemory.path = main_folder;
    tri->path = main_folder;
    

    load_gltf_models(gltfData, main_folder);
    init_cgltf_buffer(gltfData);
    
    initVulkan(tri);

    //setup skeletal mesh that we initialized
    VkBufferUsageFlagBits vertex_flag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkBufferUsageFlagBits index_flag = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    createMeshBuffer(tri, (void*)gltfData->test_vertices, sizeof(Vertex) * gltfData->vertex_count, vertex_flag,  tri->skeletalMeshVertices, tri->skeletalMeshVertexMemory);
    createMeshBuffer(tri, (void*)gltfData->indices, sizeof(uint32_t) * gltfData->total_index_count, index_flag,  tri->skeletalMeshIndices, tri->skeletalMeshIndexMemory);
    tri->skeletalMeshIndexCount  = gltfData->total_index_count;
        
    GameMemory.gltfData = gltfData;

    GameMemory.RenderCommandData = &tri->RenderCommandData;
    



    GameMemory.doubleDescent = tri->doubleDescent;
    GameMemory.lineAdvance = tri->lineAdvance;
    GameMemory.maxCharWidth = tri->maxCharWidth;
    
    //end vulkan buffer pointer setup
    GameMemory.mainThreadID = (u16)GetThreadID();  
    
    #if LABOR_INTERNAL
        GameMemory.GlobalDebugTablePtr = GlobalDebugTable;
    #endif

    win32_game_code gameCode = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodePath, gameCodeLockPath);
    if(gameCode.gameInit)gameCode.gameInit(&GameMemory);

    

    LARGE_INTEGER lastCounter = Win32GetWallClock();
    LARGE_INTEGER flipWallClock = Win32GetWallClock();


    u64 lastCycleCount = __rdtsc();
    POINT MouseP;
    GetCursorPos(&MouseP);
    ScreenToClient(g_window, &MouseP);
    s32 last_mouseX = MouseP.x;
    s32 last_mouseY = MouseP.y;
    s32 mouseX = MouseP.x;
    s32 mouseY = MouseP.y;

    while (g_running){
        BEGIN_BLOCK("TOTAL FRAME TIME");
        
        u64 start = __rdtsc();


        last_mouseX = mouseX;
        last_mouseY = mouseY;
        
        GameMemory.old_input = GameMemory.new_input;

      
        //the game memory input is updated much more frequently than in the game layer
        //the game layer resets this. we only accumulate here

        MouseP = {};
        GetCursorPos(&MouseP);
        ScreenToClient(g_window, &MouseP);
        mouseX = MouseP.x;
        mouseY = MouseP.y;
        g_mouseX = mouseX;
        g_mouseY = mouseY;

        
        GameMemory.new_input.mouse_dx += mouseX - last_mouseX; 
        GameMemory.new_input.mouse_dy += mouseY - last_mouseY; 
        g_mouseDeltaX = GameMemory.new_input.mouse_dx;
        // float test = (float)g_mouseX;
        // printf("g mouseX float: %f\n", test);
        g_mouseDeltaY = GameMemory.new_input.mouse_dy;
        GameMemory.new_input.mouse_x = mouseX;
        GameMemory.new_input.mouse_y = mouseY;

        // printf("mouse delta: %d %d, mouse pos  : %d %d\n", GameMemory.new_input.mouse_dx, GameMemory.new_input.mouse_dy, GameMemory.new_input.mouse_x, GameMemory.new_input.mouse_y);

        win_process_pending_messages(&platformState, &GameMemory, &GameMemory.new_input);
        bool mouseInWindow = (mouseX >= 0 && mouseX <= g_width && mouseY >= 0 && mouseY <= g_height);


        //mouse input processing
        {
            b32 isDown  = (GetKeyState(VK_LBUTTON ) & (1 << 15)) != 0; 
            if(GameMemory.new_input.mouse.left != isDown){ 
                if((isDown && mouseInWindow) || !isDown){
                    GameMemory.new_input.mouse.left = isDown; GameMemory.new_input.transitionCounts[InputTypes::input_mouse_left]++;            
                }
            }
            
            isDown  = (GetKeyState(VK_MBUTTON ) & (1 << 15)) != 0; 
            if(GameMemory.new_input.mouse.middle != isDown){
                if((isDown && mouseInWindow) || !isDown){
                    GameMemory.new_input.mouse.middle = isDown; GameMemory.new_input.transitionCounts[InputTypes::input_mouse_middle]++;
                }
            }
            
            isDown  = (GetKeyState(VK_RBUTTON ) & (1 << 15)) != 0; 
            if(GameMemory.new_input.mouse.right != isDown){
                if((isDown && mouseInWindow) || !isDown){
                    GameMemory.new_input.mouse.right = isDown; GameMemory.new_input.transitionCounts[InputTypes::input_mouse_right]++;
                }
            }
            
            isDown  = (GetKeyState(VK_XBUTTON1) & (1 << 15)) != 0; 
            if(GameMemory.new_input.mouse.sideBack != isDown){
                if((isDown && mouseInWindow) || !isDown){
                    GameMemory.new_input.mouse.sideBack = isDown; GameMemory.new_input.transitionCounts[InputTypes::input_mouse_sideBack]++;
                }
            }
            
            isDown  = (GetKeyState(VK_XBUTTON2) & (1 << 15)) != 0; 
            if(GameMemory.new_input.mouse.sideFront != isDown){
                if((isDown && mouseInWindow) || !isDown){
                    GameMemory.new_input.mouse.sideFront = isDown; GameMemory.new_input.transitionCounts[InputTypes::input_mouse_sideFront]++;
                }
            }
        }


        if(g_window_resized){
            g_window_resized = 0;
            GameMemory.window_resized = true;
            GameMemory.width = g_width;
            GameMemory.height = g_height;
        }

        GameMemory.executable_reloaded = false;
        //prevents multiple reloads as long as lock is in place
        WIN32_FILE_ATTRIBUTE_DATA ignored;
        if (!GetFileAttributesEx(gameCodeLockPath, GetFileExInfoStandard, &ignored)) {
            FILETIME newDllWriteTime = Win32GetLastWriteTime(sourceGameCodeDLLFullPath);
            if(CompareFileTime(&newDllWriteTime, &gameCode.DLLLastWriteTime) != 0){
                printf("COMPLETE ALL WORK ON THREADS HERE BEFORE RELOADING GAME CODE!\n");
                CompleteAllWork(&Queue);

#if LABOR_INTERNAL
                GlobalDebugTable = &GlobalDebugTable_;
                //reset event storage
#endif

                Win32UnloadGameCode(&gameCode);
                gameCode = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodePath, gameCodeLockPath);
                GameMemory.executable_reloaded = true;

            }
        }
        float absoluteTime = Win32GetAbsoluteSecondsElapsed();

        u64 preGameUpdateCycles = __rdtsc() - start;
        //spikes happen when clicking in/out of the game window
        // if(preGameUpdateCycles > 10000000){
        //     printf("pre game update cycle count: %zu\n", preGameUpdateCycles);
        //             __debugbreak();
        // }

        if(gameCode.updateAndRender)gameCode.updateAndRender(&GameMemory);
        
        //to experiment with if we decide to move to a specific fixed update function
        // if(gameCode.fixedUpdate)gameCode.fixedUpdate(&GameMemory);
        
        tri->elapsedTime = fmodf(absoluteTime, 600.0f); ;
        tri->deltaTime = GameMemory.deltaTime;
        tri->ubo = {};
        tri->axis_ubo = {};
        tri->ubo.model = GameMemory.model;
        tri->ubo.view = GameMemory.view;
        tri->ubo.proj = GameMemory.proj;
        tri->ubo.viewProj = GameMemory.viewProj;
        tri->ubo.invViewProj = GameMemory.invViewProj;

        tri->axis_ubo.model = GameMemory.axis_model;
        tri->axis_ubo.view = GameMemory.view;
        tri->axis_ubo.proj = GameMemory.proj;
        tri->axis_ubo.viewProj = GameMemory.viewProj;
        tri->axis_ubo.invViewProj = GameMemory.invViewProj;
        tri->camPos = GameMemory.camPos;




#if LABOR_INTERNAL
        u64 startCollation = __rdtsc();

        // BEGIN_BLOCK("Debug Collation");

        if(gameCode.debugGameFrameEnd) GlobalDebugTable = gameCode.debugGameFrameEnd(&GameMemory);
        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

        // END_BLOCK("Debug Collation");
        u64 endCollation = __rdtsc();
        u64 totalCollationCycles = endCollation - startCollation;
        // printf("total debug collation time: %zu\n", totalCollationCycles);
        // if(totalCollationCycles > 1000000){
        //     __debugbreak();
        // }
#endif

        if(gameCode.gameDrawUI)gameCode.gameDrawUI(&GameMemory);

        // BEGIN_BLOCK("DRAW FRAME");
        drawFrame(tri);
        // END_BLOCK("DRAW FRAME");


        LARGE_INTEGER endCounter = Win32GetWallClock();

        GameMemory.deltaTime = Win32GetSecondsElapsed(lastCounter, endCounter);
        // printf("seconds elapsed: %f\n", GameMemory.deltaTime);

        END_BLOCK("TOTAL FRAME TIME");

        FRAME_MARKER(GameMemory.deltaTime);
                    
        lastCounter = endCounter;

        u64 endCycleCount = __rdtsc();
        u64 cyclesElapsed = endCycleCount - lastCycleCount;
        lastCycleCount = endCycleCount;
        // printf("PLATFORM frame cycles: %zu\n", cyclesElapsed);

    }


    //complete all work on the threads
    CompleteAllWork(&Queue);

    //cleanup
    vkDeviceWaitIdle(tri->logicalDevice);
    cleanup_vulkan(tri);

    // wglMakeCurrent(0, 0);
    // wglDeleteContext(gl_context);
    ReleaseDC(g_window, g_device_context);
    DestroyWindow(g_window);
    
    fclose(log);

    // free(state.TotalMemoryBlock);
    // deallocate(state.TotalMemoryBlock);
    plat_dealloc_mem(state.TotalMemoryBlock, state.TotalSize);
    if(!g_allocs && !g_allocs_bytes){
        printf("all VirtualAllocs FREED!\n");
    }else{
        printf("NOT ALL VIRTUAL ALLOCS FREED! MEMORY LEAK? g_allocs (number of allocations): %u, unaccounted for bytes: %zu\n", g_allocs,  g_allocs_bytes);
    }


        // Signal threads to exit and wake them up
        AtomicIncrement(&ShouldThreadsExit);
        // closesocket(SharedNetworkStates[2].serverSockets.udp_socket);  // This interrupts blocked network calls
        // closesocket(SharedNetworkStates[2].clientSockets.udp_socket);  // This interrupts blocked network calls

        // Wake all waiting threads
        for(int i = 0; i < ThreadCount; ++i) {
            ReleasePlatformSemaphore(Queue.SemaphoreHandle);
        }

        // Wait for all threads to actually finish
        #ifdef _WIN32
            WaitForMultipleObjects(ThreadCount, ThreadHandles, TRUE, INFINITE);

            //to make the main thread wait for all threads to close before exiting
            //see above where we declare the threads to see how to group them together into AllHandles
            // WaitForMultipleObjects(ThreadCount + (SERVER_BUILD ? 2 : 1), AllHandles, TRUE, INFINITE);

            for(int i = 0; i < ThreadCount; ++i) {
                CloseHandle(ThreadHandles[i]);
                // printf("closed thread ID: %u\n", ThreadIDs[i]);

            }
            #ifdef SERVER_BUILD
            CloseHandle(ConsoleHandle);
            #endif

            CloseHandle(NetworkHandle);

        #else
            for(int i = 0; i < ThreadCount; ++i) {
                pthread_join(ThreadHandles[i], NULL);
            }
            #ifdef SERVER_BUILD
            pthread_join(ConsoleHandle, NULL);
            #endif

            pthread_join(NetworkHandle, NULL);

        #endif

            // #ifdef _WIN32
        //     Sleep(100);
        // #else
        //     usleep(100000);
        // #endif
        DestroyPlatformSemaphore(Queue.SemaphoreHandle);  // Clean up the semaphore
        printf("destroyed all semaphores, ending app\n");

    return 0;
}