#ifndef WINLABOR_H
#define WINLABOR_H

#ifdef LABOR_DEBUG
#define EXECUTE_BAT(workingDir, batFilePath) execute_bat(workingDir, batFilePath);
#else
#define EXECUTE_BAT(workingDir, batFilePath)
#endif


inline void execute_bat(LPCSTR workingDir, LPCSTR batFilePath){
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    // LPCSTR batFilePath = "C:\\labor\\shaders\\compile.bat";
    // LPCSTR workingDir = "C:\\labor\\shaders";
    
    CreateProcess(
        NULL,                   // No module name
        (LPSTR)batFilePath,     // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Don't inherit handles
        0,                      // No creation flags
        NULL,                   // Use parent's environment block
        workingDir,             // Working directory
        &si,                    // STARTUPINFO
        &pi                     // PROCESS_INFORMATION
    );
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
}

// Function to compile a specific shader
inline bool compile_shader(LPCSTR shaderFilePath) {
    // Path to the compile-shader.bat script
    LPCSTR batFilePath = "C:\\labor\\shaders\\compile_shader.bat";
    
    // Working directory for the batch process
    LPCSTR workingDir = "C:\\labor\\shaders";
    
    // Construct the complete command with the shader file as parameter
    char commandLine[MAX_PATH * 2];
    sprintf_s(commandLine, "%s \"%s\"", batFilePath, shaderFilePath);
    
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    // Create the process to compile the shader
    BOOL result = CreateProcess(
        NULL,                // No module name
        commandLine,         // Command line with shader path
        NULL,                // Process handle not inheritable
        NULL,                // Thread handle not inheritable
        FALSE,               // Don't inherit handles
        0,                   // No creation flags
        NULL,                // Use parent's environment block
        workingDir,          // Working directory
        &si,                 // STARTUPINFO
        &pi                  // PROCESS_INFORMATION
    );
    
    if (!result) {
        DWORD error = GetLastError();
        char errorMsg[256];
        sprintf_s(errorMsg, "Failed to compile shader. Error code: %lu", error);
        OutputDebugString(errorMsg);
        return false;
    }
    
    // Wait for the process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // Get the exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    // Clean up
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return exitCode == 0;
}

inline LARGE_INTEGER Win32GetWallClock(){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}




struct win32_game_code{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    game_update_and_render* updateAndRender;
    game_fixed_update* fixedUpdate;
    debug_game_frame_end*   debugGameFrameEnd;
    game_get_sound_samples* gameGetSoundSamples;
    game_init* gameInit;
    game_draw_ui* gameDrawUI;

    b32 is_valid;
};

#endif