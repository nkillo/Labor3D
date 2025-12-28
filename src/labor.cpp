
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG_CAMERA 0
#define DRAW_UI 1

#include "math.h"
#include "strUtil.h"
#include "networkState.h"
#include "labor.h"
#include "threadPool.h"

#include "fptvec.h"

#include "frustum.cpp"
#include "camera.cpp"
#include "entity.cpp"
#include "brickmap.cpp"
#include "chunkManager.cpp"
#include "binaryMesher.cpp"
#include "voxelHelpers.cpp"
#include "bvh.cpp"
#include "ChunkSystem.cpp"
#include "cgltfTest.h"

#include "ui.h"
#include "ui.cpp"

#include "bbSystem.h"
#include "noiseTests.cpp"


#if LABOR_INTERNAL
#include "labor_debug.cpp"
#else
extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd) {
    return 0;
}
#endif


#ifdef LABOUR_DEBUG
#define ASSERT_NO_NAN_T(t, msg) \
    if (t != t) { \
        printf("NaN detected in t: %s (%10.5f)\n", msg, t); \
        __debugbreak(); \
    }

#define ASSERT_NO_INF_T(t, msg) \
    if (t == (1.0/0.0) || t == (-1.0/0.0)) { \
        printf("inf detected in t: %s (%10.5f)\n", msg, t); \
        __debugbreak(); \
    }

#define ASSERT_NO_NAN_POS(vec, msg) \
    if (vec.x != vec.x || vec.y != vec.y || vec.z != vec.z) { \
        printf("NaN detected in position: %s (%10.5f %10.5f %10.5f)\n", msg, vec.x, vec.y, vec.z); \
        __debugbreak(); \
    }

#define ASSERT_NO_NAN_ROT(vec, msg) \
    if (vec.x != vec.x || vec.y != vec.y || vec.z != vec.z || vec.w != vec.w) { \
        printf("NaN detected in rotation: %s (%10.5f %10.5f %10.5f %10.5f)\n", msg, vec.x, vec.y, vec.z, vec.w); \
        __debugbreak(); \
    }
#else
#define ASSERT_NO_NAN_T(t, msg) 
#define ASSERT_NO_INF_T(t, msg)
#define ASSERT_NO_NAN_POS(vec, msg) // Does nothing in release builds
#define ASSERT_NO_NAN_ROT(vec, msg)
#endif


inline void resetTextEditTimers(textEditInputTEST* timer, u32 active = true) {
    timer->active = active;
    timer->ticksSincePressed = 0;
    timer->ticksSinceRepeat = 0;
}


inline int gapBufferInsert(game_state* GameState, TextInputState* dest, wchar_t c) {
    if (dest->gapStart >= dest->gapEnd) {
        return 0;
    }
    dest->buffer[dest->gapStart++] = c;
    dest->cursor_pos++;
    if (c == '\n') {
        //TODO charsSinceNewLine doesnt work, since it doesnt account for backspace
        dest->cursor_positions[dest->cursor_pos].y = (dest->cursor_positions[dest->cursor_pos - 1].y) + GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale;
        dest->cursor_positions[dest->cursor_pos].x = 0;
        dest->charsSinceNewLine = 0;

    }
    else {
        dest->charsSinceNewLine++;
        dest->cursor_positions[dest->cursor_pos].y = dest->cursor_positions[dest->cursor_pos - 1].y;
        dest->cursor_positions[dest->cursor_pos].x = dest->charsSinceNewLine * (GameState->RenderCommandData->monospacedScreenFont.maxCharWidth * GameState->RenderCommandData->monospacedScreenFont.scale);
    }
    dest->length++;
    dest->cursor_num_positions++;
    return 1;

}
inline int gapBufferRemove(game_state* GameState, TextInputState* dest, u32 numToRemove = 1) {
    if (dest->gapStart > dest->gapEnd) {
        return 0;
    }
    if (dest->length == 0 || dest->gapStart == 0 || dest->cursor_pos == 0) {
        return 0;
    }

    dest->gapStart--;
    dest->cursor_pos--;
    dest->length--;
    dest->cursor_num_positions--;

    return 1;

}

inline int gapMoveLeft(game_state* GameState, TextInputState* dest) {
    if (dest->cursor_pos > 0) {
        dest->cursor_pos--;
        dest->gapStart--;
        dest->gapEnd--;
        dest->buffer[dest->gapEnd] = dest->buffer[dest->gapStart];
    }
    return 0;
}

inline int gapMoveRight(game_state* GameState, TextInputState* dest) {
    if (dest->cursor_pos < dest->length) {
        Assert(dest->gapEnd != dest->buffer_size);
        dest->buffer[dest->gapStart] = dest->buffer[dest->gapEnd];
        dest->cursor_pos++;
        dest->cursor_positions[dest->cursor_pos].x = dest->cursor_pos * (GameState->RenderCommandData->monospacedScreenFont.maxCharWidth * GameState->RenderCommandData->monospacedScreenFont.scale);
        dest->gapStart++;
        dest->gapEnd++;
    }
    return 0;
}

inline void process_text_input(game_memory* Memory, game_state* GameState, player_input& currInput) {
    if (!GameState->text_dest) {
        Assert(!"Null text destination!");
    }
    TextInputState* dest = GameState->text_dest;
    for (u32 i = 0; i < Memory->textInputLen; i++) {
        wchar_t c = Memory->codePointInput[i];
        // printf("processing char %c, (%d)\n", Memory->textInput[i], Memory->textInput[i]);
        switch (c) {
        case 1: { printf("unhandled char 1\n"); }break;
        case 2: { printf("unhandled char 2\n"); }break;
        case 3: { printf("CTRL+C\n"); }break;
        case 4: { printf("unhandled char 4\n"); }break;
        case 5: { printf("unhandled char 5\n"); }break;
        case 6: { printf("unhandled char 6\n"); }break;
        case 7: {
            printf("ESCAPE\n");
        }break;
        case 8: {
            printf("BACKSPACE\n");
            gapBufferRemove(GameState, dest);
        }break;
        case 9: {
            printf("TAB\n");
        }break;

        case 10: { printf("unhandled char 10\n"); }break;
        case 11: { printf("unhandled char 11\n"); }break;
        case 12: { printf("unhandled char 12\n"); }break;
        case 13: {
            // printf("newline\n");
            gapBufferInsert(GameState, dest, '\n');
        }break;
        case 14: { printf("unhandled char 14\n"); }break;
        case 15: { printf("unhandled char 15\n"); }break;
        case 16: { printf("unhandled char 16\n"); }break;
        case 17: { printf("unhandled char 17\n"); }break;
        case 18: { printf("unhandled char 18\n"); }break;
        case 19: { printf("unhandled char 19\n"); }break;
        case 20: { printf("unhandled char 20\n"); }break;
        case 21: { printf("unhandled char 21\n"); }break;

        case 22: { printf("CTRL+V\n"); }break;
        case 23: { printf("unhandled char 23\n"); }break;
        case 24: { printf("unhandled char 24\n"); }break;
        case 25: { printf("CTRL+Y\n"); }break;
        case 26: { printf("CTRL+Z\n"); }break;
        case 27: { printf("unhandled char 27\n"); }break;
        case 28: { printf("unhandled char 28\n"); }break;
        case 29: { printf("unhandled char 29\n"); }break;
        case 30: { printf("unhandled char 30\n"); }break;
        case 31: { printf("unhandled char 31\n"); }break;
        case 127: { printf("CTRL+BACKSPACE\n"); }break;
        default: {
            if (dest->cursor_pos + 1 < dest->buffer_size) {
                if (c >= 32 && c < 127) {

                    printf("default char: %c\n", c);

                    gapBufferInsert(GameState, dest, c);
                }
                else {
                    printf("undrawable character: %d\n", c);
                }
            }
            else {
                printf("buffer too large!\n");
            }
        }
        }
    }


    bool ctrl = WAS_PRESSED((currInput), function.keyCTRL, InputTypes::input_key_ctrl);
    bool alt = WAS_PRESSED((currInput), function.alt, InputTypes::input_key_alt);
    bool upArrow = WAS_PRESSED((currInput), function.up, InputTypes::input_key_arrow_up);
    bool downArrow = WAS_PRESSED((currInput), function.down, InputTypes::input_key_arrow_down);
    bool leftArrow = WAS_PRESSED((currInput), function.left, InputTypes::input_key_arrow_left);
    bool rightArrow = WAS_PRESSED((currInput), function.right, InputTypes::input_key_arrow_right);
    bool pageUp = WAS_PRESSED((currInput), function.pageUp, InputTypes::input_key_arrow_pageUp);
    bool pageDown = WAS_PRESSED((currInput), function.pageDown, InputTypes::input_key_arrow_pageDown);
    bool home = WAS_PRESSED((currInput), function.home, InputTypes::input_key_arrow_home);
    bool end = WAS_PRESSED((currInput), function.end, InputTypes::input_key_arrow_end);
    if (alt)printf("alt pressed!\n");

    bool ctrlHeld = currInput.function.keyCTRL;
    bool shiftHeld = currInput.function.shift;
    // if(shiftHeld){printf("shift held\n");}

    if (ctrl) {
        printf("CTRL pressed!\n");
    }
    if (upArrow) {
        if (ctrlHeld) {
            printf("ctrl+up arrow pressed!\n");

        }
        else {

            printf("up arrow pressed!\n");
        }
        resetTextEditTimers(GameState->textEditTimers + text_edit_up);
        resetTextEditTimers(GameState->textEditTimers + text_edit_down, false);

    }
    if (downArrow) {
        if (ctrlHeld) {
            printf("ctrl+down arrow pressed!\n");

        }
        else {

            printf("down arrow pressed!\n");
        }
        resetTextEditTimers(GameState->textEditTimers + text_edit_up, false);
        resetTextEditTimers(GameState->textEditTimers + text_edit_down);
    }
    if (leftArrow) {
        if (ctrlHeld) {
            printf("ctrl+left arrow pressed!\n");

        }
        else {

            printf("left arrow pressed!\n");
        }
        resetTextEditTimers(GameState->textEditTimers + text_edit_left);
        resetTextEditTimers(GameState->textEditTimers + text_edit_right, false);
        gapMoveLeft(GameState, dest);

    }
    if (rightArrow) {
        if (ctrlHeld) {
            printf("ctrl+right arrow pressed!\n");

        }
        else {

            printf("right arrow pressed!\n");
        }

        //disable orthoganol state
        resetTextEditTimers(GameState->textEditTimers + text_edit_left, false);
        resetTextEditTimers(GameState->textEditTimers + text_edit_right);
        gapMoveRight(GameState, dest);

    }
    if (pageUp) {
        printf("pageUp\n");
        resetTextEditTimers(GameState->textEditTimers + text_edit_pageUp);
    }
    if (pageDown) {
        printf("pageDown\n");
        resetTextEditTimers(GameState->textEditTimers + text_edit_pageDown);
    }
    if (home) {
        printf("home\n");
        // dest->cursor_pos = 0;
        resetTextEditTimers(GameState->textEditTimers + text_edit_home);
    }
    if (end) {
        printf("end\n");
        // dest->cursor_pos = dest->length;
        resetTextEditTimers(GameState->textEditTimers + text_edit_end);

    }


    textEditInputTEST* pageUp_RepeatTimer = GameState->textEditTimers + text_edit_pageUp;
    textEditInputTEST* pageDown_RepeatTimer = GameState->textEditTimers + text_edit_pageDown;
    textEditInputTEST* home_RepeatTimer = GameState->textEditTimers + text_edit_home;
    textEditInputTEST* end_RepeatTimer = GameState->textEditTimers + text_edit_end;

    textEditInputTEST* left_RepeatTimer = GameState->textEditTimers + text_edit_left;
    if (currInput.function.left && left_RepeatTimer->active) {
        left_RepeatTimer->ticksSincePressed++;
        if (left_RepeatTimer->ticksSincePressed >= 10) {
            left_RepeatTimer->ticksSinceRepeat++;
            if (left_RepeatTimer->ticksSinceRepeat >= 1) {
                left_RepeatTimer->ticksSinceRepeat = 0;
                gapMoveLeft(GameState, dest);
            }
        }
    }


    textEditInputTEST* right_RepeatTimer = GameState->textEditTimers + text_edit_right;
    if (currInput.function.right && right_RepeatTimer->active) {
        right_RepeatTimer->ticksSincePressed++;
        if (right_RepeatTimer->ticksSincePressed >= 10) {
            right_RepeatTimer->ticksSinceRepeat++;
            if (right_RepeatTimer->ticksSinceRepeat >= 1) {
                right_RepeatTimer->ticksSinceRepeat = 0;
                gapMoveRight(GameState, dest);
            }
        }
    }


    textEditInputTEST* up_RepeatTimer = GameState->textEditTimers + text_edit_up;
    if (currInput.function.up && up_RepeatTimer->active) {
        up_RepeatTimer->ticksSincePressed++;
        if (up_RepeatTimer->ticksSincePressed >= 10) {
            up_RepeatTimer->ticksSinceRepeat++;
            if (up_RepeatTimer->ticksSinceRepeat >= 1) {
                up_RepeatTimer->ticksSinceRepeat = 0;
                //logic here
            }
        }
    }

    textEditInputTEST* down_RepeatTimer = GameState->textEditTimers + text_edit_down;
    if (currInput.function.down && down_RepeatTimer->active) {
        down_RepeatTimer->ticksSincePressed++;
        if (down_RepeatTimer->ticksSincePressed >= 10) {
            down_RepeatTimer->ticksSinceRepeat++;
            if (down_RepeatTimer->ticksSinceRepeat >= 1) {
                down_RepeatTimer->ticksSinceRepeat = 0;
                //logic here
            }
        }
    }
    Memory->textInputLen = 0;
}


//fpt input
inline vec3 CameraRelativePositionFPT(CameraComp& cameraComp, ivec3 chunk_coords, fpt_vec3 local_chunk_pos = fpt_vec3_create(0)) {
    vec3 result = fpt_to_flt_vec3(local_chunk_pos - cameraComp.pos_in_chunk) + ivec3_add_scaled(chunk_coords, -cameraComp.chunk_coords, CHUNK_SIZE);

    return result;
}

//vec3 input
inline vec3 CameraRelativePosition(CameraComp& cameraComp, ivec3 chunk_coords, vec3 local_chunk_pos = { 0,0,0 }) {
    vec3 result = (local_chunk_pos - fpt_to_flt_vec3(cameraComp.pos_in_chunk)) + ivec3_add_scaled(chunk_coords, -cameraComp.chunk_coords, CHUNK_SIZE);

    return result;
}



//UI drawing functions

            // FontManager::render_transient_screenspace_text(GameState, coord_buffer, pos, color, scale, false);
void render_screenspace_text(game_state* GameState, vec2& pos, vec4& color, vec2& scale, bool center_text, char* buffer, u32 font_index = 4) {
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    u32& screenCharCount = GameState->RenderCommandData->screen_transient_char_count;
    Assert(GameState->RenderCommandData->screenTextDrawCount + 1 <= MAX_SSBO_ENTITIES);
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = { 0.0f, 0.0f, 1.0f, 1.0f };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 0, 6, font_index, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].center_text = center_text;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };//clear highlight info    

    GameState->RenderCommandData->screen_transient_char_offsets[(GameState->RenderCommandData->screenTextDrawCount)++] = screenCharCount;
    screenCharCount += handmade_strcpy(GameState->RenderCommandData->screen_transient_char_buffer + screenCharCount, buffer) + 1;
    Assert(screenCharCount <= MAX_TEXT_CHARS);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;
}

void render_screenspace_text_monospace_scissor_highlight(game_state* GameState, vec2& pos, vec4& color, vec2& scale, bool center_text, char* buffer, vec4& scissor, u32 font_index = 4, u32 highlightStart = 0, u32 highlightLen = 0) {
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    u32& screenCharCount = GameState->RenderCommandData->screen_transient_char_count;
    Assert(GameState->RenderCommandData->screenTextDrawCount + 1 <= MAX_SSBO_ENTITIES);
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = { 0.0f, 0.0f, 1.0f, 1.0f };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 1, 6, font_index, 0 };//x = 1 for scissored
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].center_text = center_text;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].scissor = { scissor.x >= 0 ? scissor.x : 0, scissor.y >= 0 ? scissor.y : 0, scissor.z, scissor.w };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].miscColor = { 1, 0, 0, 1 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 1, highlightStart, highlightLen, 0 };

    GameState->RenderCommandData->screen_transient_char_offsets[(GameState->RenderCommandData->screenTextDrawCount)++] = screenCharCount;
    u32 len = handmade_strcpy(GameState->RenderCommandData->screen_transient_char_buffer + screenCharCount, buffer) + 1;
    screenCharCount += len;
    Assert(screenCharCount <= MAX_TEXT_CHARS);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;
}


void render_screenspace_text_scissored(game_state* GameState, vec2& pos, vec4& color, vec2& scale, bool center_text, char* buffer, vec4& scissor, u32 font_index = 4) {
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    u32& screenCharCount = GameState->RenderCommandData->screen_transient_char_count;
    Assert(GameState->RenderCommandData->screenTextDrawCount + 1 <= MAX_SSBO_ENTITIES);
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = { 0.0f, 0.0f, 1.0f, 1.0f };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 1, 6, font_index, 0 };//x = 1 for scissored
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].center_text = center_text;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].scissor = { scissor.x >= 0 ? scissor.x : 0, scissor.y >= 0 ? scissor.y : 0, scissor.z, scissor.w };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };

    GameState->RenderCommandData->screen_transient_char_offsets[(GameState->RenderCommandData->screenTextDrawCount)++] = screenCharCount;
    screenCharCount += handmade_strcpy(GameState->RenderCommandData->screen_transient_char_buffer + screenCharCount, buffer) + 1;
    Assert(screenCharCount <= MAX_TEXT_CHARS);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;
}


void render_screenspace_box(game_state* GameState, vec2& pos, vec4& color, vec2& scale) {
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = { 0.0f, 0.0f, 1.0f, 1.0f };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 0, 1, 0, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };

    Assert(screenElementCount <= MAX_SSBO_ENTITIES);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;
}



void render_screenspace_border(game_state* GameState, vec2& pos, vec4& color, vec2& scale, u32 filled = 0) {
    //screen element draw test
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = { 0.0f, 0.0f, 1.0f, 1.0f };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 0, filled, 0, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };

    Assert(screenElementCount <= MAX_SSBO_ENTITIES);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;

}

void render_screenspace_border_scissored(game_state* GameState, vec2& pos, vec4& color, vec2& scale, vec4& scissor, u32 filled = 0) {
    //screen element draw test
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = { 0.0f, 0.0f, 1.0f, 1.0f };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 1, filled, 0, 0 };//misc.x is for scissor
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    vec4 tempScissor = scissor;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].scissor = { scissor.x >= 0 ? scissor.x : 0, scissor.y >= 0 ? scissor.y : 0, scissor.z, scissor.w };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };


    Assert(screenElementCount <= MAX_SSBO_ENTITIES);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;

}


void render_ui_texture(game_state* GameState, vec2& pos, vec2& scale, vec4& uv_coords, vec4& color) {
    //screen element draw test
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = uv_coords;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 0, 3, 0, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };
    Assert(screenElementCount <= MAX_SSBO_ENTITIES);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;
}


void render_ui_texture_id(game_state* GameState, vec2& pos, vec2& scale, vec4& uv_coords, vec4& color, vec4& scissor, u32 textureID) {
    //screen element draw test
    size_t& screenElementCount = GameState->RenderCommandData->screenElementDrawCount;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.position = pos;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.scale = scale;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.color = color;
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.texCoords = uv_coords;

    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc = { 1, 3, textureID, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].push.misc2 = { 0, 0, GameState->currentUIDepth, 0 };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].scissor = { scissor.x >= 0 ? scissor.x : 0, scissor.y >= 0 ? scissor.y : 0, scissor.z, scissor.w };
    GameState->RenderCommandData->screenElementDrawCommands[screenElementCount].misc = { 0,0,0,0 };

    Assert(screenElementCount <= MAX_SSBO_ENTITIES);
    GameState->currentUIDepth -= GameState->UIDepthIncrement;
    screenElementCount++;
}






void draw_ui_window_base(game_state* GameState, ui_element* base, char* label, vec2& pos, vec4& uv_coords, vec4& color) {
    pos.x = base->posx; pos.y = (base->posy);
    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = .1f;
    vec2 scale = base->scale;

    render_screenspace_box(GameState, pos, color, scale);


    color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.25f;
    if (base->hovered) { color.x = 0.5f; color.y = 0.5f; color.z = 0.5f; color.w = 0.25f; }
    if (base->clicked) { color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.50f; }
    render_screenspace_border(GameState, pos, color, scale);

    //EQUIPMENT WINDOW WIDGET
    pos.x = base->posx + (base->width * 0.5f) - (MIN_WINDOW_DIMENSIONS * 0.5f); pos.y = (base->posy) + (base->height * 0.5f) - (MIN_WINDOW_DIMENSIONS * 0.5f);
    color.x = 0.5f; color.y = 0.5f; color.z = 0.5f; color.w = 1.0f;
    if (base->widget_selected) { color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f; }
    scale.x = 20.0f; scale.y = 20.0f;
    render_screenspace_border(GameState, pos, color, scale);


    pos.x = base->posx; pos.y = (base->posy) - base->scale.y * 0.5f;//shift up;
    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;
    scale.x = 1.0f; scale.y = 1.0f;
    render_screenspace_text(GameState, pos, color, scale, true, label);

}



void draw_inventory_region(game_state* GameState, ui_element* bounds, vec2& pos, vec2& scale, vec4& uv_coords, vec4& color, float rootx, float rooty) {
    //DRAW INVENTORY BOUNDS
    pos.x = rootx + bounds->posx; pos.y = rooty + (bounds->posy);
    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;
    render_screenspace_border(GameState, pos, color, bounds->scale);

}


void set_item_texture(game_state* GameState, u32 itemID, vec4& uv_coords) {
    item_data& item = GameState->entityComponent->inventory_items[itemID].data;


    switch (item.type) {
    case item_types::item_type_tool: {
        switch (item.tool.type) {
        case tool_types::tool_type_sword: {
            uv_coords.x = 0.00f; uv_coords.y = 0.0f; uv_coords.z = 0.5f; uv_coords.w = 0.5f;

        }break;
        case tool_types::tool_type_shield: {
            uv_coords.x = 0.51f; uv_coords.y = 0.0f; uv_coords.z = 1.0f; uv_coords.w = 0.51f;

        }break;
        }
    }break;
    default: break;
    }
}

void draw_window_slots(game_state* GameState, ui_element* bounds, ui_element* slot_array,
    int iterations, vec2& pos, vec2& scale, vec4& uv_coords, vec4& color,
    float rootx, float rooty, ui_element_inventory_types inventory_type, u32* entity_inventory_slots) {

    //DRAW INVENTORY BOUNDS
    vec2 regionScale = bounds->scale;
    draw_inventory_region(GameState, bounds, pos, regionScale, uv_coords, color, rootx, rooty);


    rootx = rootx + (bounds->posx - (bounds->width * 0.5f));
    rooty = rooty + (bounds->posy - (bounds->height * 0.5f));

    //DRAW INVENTORY ITEM SLOTS

    for (int i = 0; i < iterations; i++) {

        vec2 slotScale = (slot_array + i)->scale;
        draw_inventory_region(GameState, slot_array + i, pos, slotScale, uv_coords, color, rootx, rooty);
        // if(slotScale.x > MIN_WINDOW_DIMENSIONS && slotScale.y > MIN_WINDOW_DIMENSIONS){
        if (slotScale.x > 10 && slotScale.y > 10) {//want to allow for smaller drawing for debugging

            if (entity_inventory_slots && entity_inventory_slots[i]) {
                set_item_texture(GameState, entity_inventory_slots[i], uv_coords);


                color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;

                render_ui_texture(GameState, pos, slotScale, uv_coords, color);

            }


            float y_offset = (i & 1) ? 10.0f : -10.0f;
            pos.y = rooty + y_offset + (slot_array[i].posy);
            color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;

            vec2 textScale = slotScale * 0.01f;

            char str[16];
            switch (inventory_type) {
            case ui_element_inventory_type_none: {

            }break;
            case ui_element_inventory_type_item_slot: {
                int_to_string(i, str, 16);
            }break;
            case ui_element_inventory_type_trinket_slot: {
                str[0] = 'T';//T for trinket
                int_to_string(i, str + 1, 16);
            }break;
            case ui_element_inventory_type_equipment_slot: {
                const char* slot_name;
                slot_name = entity_body_parts_to_string((entity_body_parts)(i + 1));
                handmade_strcpy(str, slot_name);
            }break;
            case ui_element_inventory_type_left_hotbar_slot: {
                str[0] = 'L';//L for left hand
                int_to_string(i, str + 1, 16);
            }break;
            case ui_element_inventory_type_right_hotbar_slot: {
                str[0] = 'R';//L for left hand
                int_to_string(i, str + 1, 16);
            }break;

            }
            render_screenspace_text(GameState, pos, color, textScale, true, str);


        }

    }
}



void draw_ui_border(game_state* GameState, ui_data* uiData, ui_element* element, vec2& pos, vec4& color, vec2& scale, vec4& uv_coords, float rootx, float rooty) {
    pos.x = rootx + element->posx; pos.y = rooty + element->posy;
    // color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = 0.5f;
    render_screenspace_border(GameState, pos, color, element->scale);
}



void draw_inventory_hotbar(game_state* GameState, ui_data* uiData, ui_window* window, ui_element* bounds, ui_element* slots, vec2& pos, vec2& scale, vec4& uv_coords, vec4& color) {

    u32* entity_slots = nullptr;

    ui_element* inv = &window->base;

    float rootx = inv->posx - (inv->width * 0.5f);
    float rooty = inv->posy - (inv->height * 0.5f);


    u32 invIndex = GameState->entityComponent->entityToInventoryMap[GameState->localPlayerEntityIDs[0]];

    InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 


    rootx = rootx + (bounds->posx - (bounds->width * 0.5f));
    rooty = rooty + (bounds->posy - (bounds->height * 0.5f));


    color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.5f;
    pos.x = rootx + (slots + invComp.current_left_hand)->posx; pos.y = rooty + ((slots + invComp.current_left_hand)->posy);

    render_screenspace_border(GameState, pos, color, (slots + invComp.current_left_hand)->scale);
    render_screenspace_box(GameState, pos, color, (slots + invComp.current_left_hand)->scale);

    pos.x = rootx + (slots + 5 + invComp.current_right_hand)->posx; pos.y = rooty + ((slots + 5 + invComp.current_right_hand)->posy);

    render_screenspace_border(GameState, pos, color, (slots + 5 + invComp.current_right_hand)->scale);
    render_screenspace_box(GameState, pos, color, (slots + 5 + invComp.current_right_hand)->scale);

    rootx = inv->posx - (inv->width * 0.5f);
    rooty = inv->posy - (inv->height * 0.5f);


    entity_slots = invComp.left_hand;
    draw_window_slots(GameState, bounds, slots,
        MAX_ITEMS_PER_HAND, pos, (slots)->scale, uv_coords, color, rootx, rooty, ui_element_inventory_type_left_hotbar_slot, entity_slots);

    entity_slots = invComp.right_hand;
    draw_window_slots(GameState, bounds, slots + 5,
        MAX_ITEMS_PER_HAND, pos, (slots + 5)->scale, uv_coords, color, rootx, rooty, ui_element_inventory_type_right_hotbar_slot, entity_slots);

    if (window->selected_element) {

        ui_element* element = window->selected_element;
        float rootx = window->selected_element_rootx;
        float rooty = window->selected_element_rooty;
        color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 1.0f;
        pos.x = rootx + element->posx; pos.y = rooty + element->posy;
        render_screenspace_border(GameState, pos, color, element->scale);

    }
}




void draw_vertical_scrollbar(game_state* GameState, ui_element* bar, vec2& pos, vec4& color, vec2& scale, vec4& scissor, float rootx = 0.0f, float rooty = 0.0f) {

    float vis_range = bar->data.slider_float.visible_range;
    float curval = bar->data.slider_float.current_value;

    float scrollMin = (bar->miny); //minx
    float scrollMax = (bar->maxy); //maxx
    float scrollDiff = scrollMax - scrollMin - (bar->height * vis_range); //the range from 0 to 1 we can slide in //width


    float scrollbar_posX = rootx + bar->posx;// * (0.25f));
    float scrollbar_posY = rooty + (scrollMin + (bar->height * vis_range * 0.5f)) + (scrollDiff * curval);
    float scrollbar_scaleX = bar->scale.x;
    float scrollbar_scaleY = bar->scale.y * vis_range;

    color.x = 0.0f; color.y = 1.0f; color.z = 0.0f; color.w = 0.5f;
    pos.x = scrollbar_posX; pos.y = scrollbar_posY;
    scale.x = scrollbar_scaleX; scale.y = scrollbar_scaleY;
    u32 shapeType = 2;//for border + filled shape
    render_screenspace_border_scissored(GameState, pos, color, scale, scissor, shapeType);
}



void draw_horizontal_scrollbar(game_state* GameState, ui_element* bar, vec2& pos, vec4& scissor, float rootx = 0.0f, float rooty = 0.0f) {
    float vis_range = bar->data.slider_float.visible_range;
    float curval = bar->data.slider_float.current_value;

    float scrollMin = (bar->minx); //minx
    float scrollMax = (bar->maxx); //maxx
    float scrollDiff = scrollMax - scrollMin - (bar->width * vis_range); //the range from 0 to 1 we can slide in //width


    float scrollbar_posX = rootx + (scrollMin + (bar->width * vis_range * 0.5f)) + (scrollDiff * curval);// * (0.25f));
    float scrollbar_posY = rooty + bar->posy;
    float scrollbar_scaleX = bar->scale.x * vis_range;
    float scrollbar_scaleY = bar->scale.y;

    vec2 scale = {};
    vec4 color = { 0, 1, 0, .5f };
    pos.x = scrollbar_posX; pos.y = scrollbar_posY;
    scale.x = scrollbar_scaleX; scale.y = scrollbar_scaleY;
    u32 shapeType = 2;//for border + filled shape
    render_screenspace_border_scissored(GameState, pos, color, scale, scissor, shapeType);
}



void draw_horizontal_debug_slider(game_state* GameState, ui_element* bar, vec2& pos, vec4& color, vec2& scale, vec4& scissor, float rootx = 0.0f, float rooty = 0.0f) {
    float vis_range = bar->data.debug_slider_float.visible_range;
    float normval = bar->data.debug_slider_float.norm_value;

    float scrollMin = (bar->minx); //minx
    float scrollMax = (bar->maxx); //maxx
    float scrollDiff = scrollMax - scrollMin - (bar->width * vis_range); //the range from 0 to 1 we can slide in //width

    float scrollbar_posX = rootx + (scrollMin + (bar->width * vis_range * 0.5f)) + (scrollDiff * (normval));// * (0.25f));
    float scrollbar_posY = rooty + bar->posy;
    float scrollbar_scaleX = bar->scale.x * vis_range;
    float scrollbar_scaleY = bar->scale.y;

    pos.x = scrollbar_posX; pos.y = scrollbar_posY;
    scale.x = scrollbar_scaleX; scale.y = scrollbar_scaleY;
    u32 shapeType = 2;//for border + filled shape
    render_screenspace_border_scissored(GameState, pos, color, scale, scissor, shapeType);
}
void draw_vertical_debug_slider(game_state* GameState, ui_element* bar, vec2& pos, vec4& color, vec2& scale, vec4& scissor, float rootx = 0.0f, float rooty = 0.0f) {
    float vis_range = bar->data.debug_slider_float.visible_range;
    float normval = bar->data.debug_slider_float.norm_value;

    float scrollMin = (bar->miny);
    float scrollMax = (bar->maxy);
    float scrollDiff = scrollMax - scrollMin - (bar->height * vis_range); //the range from 0 to 1 we can slide in //width

    float scrollbar_posX = rootx + bar->posx;
    float scrollbar_posY = rooty + (scrollMin + (bar->height * vis_range * 0.5f)) + (scrollDiff * (1.0f - normval)) + bar->posy;// * (0.25f));

    float scrollbar_scaleX = bar->scale.x;
    float scrollbar_scaleY = bar->scale.y * vis_range;

    pos.x = scrollbar_posX; pos.y = scrollbar_posY;
    scale.x = scrollbar_scaleX; scale.y = scrollbar_scaleY;
    u32 shapeType = 2;//for border + filled shape
    render_screenspace_border_scissored(GameState, pos, color, scale, scissor, shapeType);
}

//end UI drawing functions

void draw_entity_command(game_state* GameState, mat4& model, MeshTypes mesh_type, TopologyTypes topology_type, vec4 color) {
    size_t& entityDrawCount = GameState->RenderCommandData->entityDrawCount;

    GameState->RenderCommandData->entityDrawCommands[entityDrawCount].mesh_type = mesh_type;
    GameState->RenderCommandData->entityDrawCommands[entityDrawCount].topology_type = topology_type;
    GameState->RenderCommandData->entityDrawCommandsSSBO[entityDrawCount].model = model;
    Assert(entityDrawCount <= MAX_SSBO_ENTITIES);

    GameState->RenderCommandData->entityDrawCommandsSSBO[entityDrawCount++].color = color;

}

void draw_cube(game_state* GameState, CameraComp& cameraComp, ivec3& chunk_coords, vec3& pos, float scale, vec4 color) {
    size_t& entityDrawCount = GameState->RenderCommandData->entityDrawCount;

    vec3 relPos = CameraRelativePosition(cameraComp, chunk_coords, pos);
    mat4 model = {};
    model = mat4_translate(relPos);
    scale_mat4(model, scale);

    GameState->RenderCommandData->entityDrawCommands[entityDrawCount].mesh_type = MeshTypes::mesh_cube;
    GameState->RenderCommandData->entityDrawCommands[entityDrawCount].topology_type = TopologyTypes::topology_triangles;
    GameState->RenderCommandData->entityDrawCommandsSSBO[entityDrawCount].model = model;
    Assert(entityDrawCount <= MAX_SSBO_ENTITIES);

    GameState->RenderCommandData->entityDrawCommandsSSBO[entityDrawCount++].color = color;

}



void destroy_entity(game_state* GameState, uint16_t entityID) {
    entityDestroyed(GameState, entityID);
    destroyEntity(*GameState->entityComponent, entityID);
}



void ui_test(game_state* GameState) {
    //populate a few fields in the ui to see if we can replicate imgui funtionality
    //window creation, text, checkboxes, and buttons
    ui_data* uiData = GameState->uiData;


#if 1//text editor info
    char window_label[32] = "TEST DEBUG";

    ui_window* debugWindow = ui_begin_window(GameState, window_label);

    ui_window* window = uiData->windows + ui_window_types::window_type_book_text;
    size_t cursor_pos = window->data.book_text.text_input.cursor_pos;
    ui_text(GameState, "UI current state : %s", ui_state_to_str(uiData->current_state));
    ui_text(GameState, "UI previous state: %s", ui_state_to_str(uiData->previous_state));

    ui_text(GameState, "book text cursor X: %f", window->data.book_text.cursor_positions[cursor_pos].x);
    ui_text(GameState, "book text cursor Y: %f", window->data.book_text.cursor_positions[cursor_pos].y);
    ui_text(GameState, "cursor index: %d", (int)cursor_pos);
    char left_char = window->data.book_text.text_input.buffer[cursor_pos - 1];
    char right_char = window->data.book_text.text_input.buffer[cursor_pos];
    if ((int)left_char == 10) {
        ui_text(GameState, "left char   : %s", "NEWLINE");
    }
    else {
        ui_text(GameState, "left char   : %c", cursor_pos == 0 ? 0 : left_char);
    }
    if ((int)right_char == 10) {
        ui_text(GameState, "right char  : %s", "NEWLINE");
    }
    else {
        ui_text(GameState, "right char  : %c", cursor_pos >= window->data.book_text.text_input.cursor_num_positions ? 0 : right_char);
    }
    uint32_t new_index = GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1);

    TextInputState* dest = GameState->text_dest;
    ui_text(GameState, "length        : %d", (dest->length));
    ui_text(GameState, "gap Start     : %d", (dest->gapStart));
    ui_text(GameState, "gap End       : %d", (dest->gapEnd));
    ui_text(GameState, "gap start char: %c", (dest->buffer[dest->gapStart]));
    ui_text(GameState, "gap End char  : %c", (dest->buffer[dest->gapEnd]));

    ui_text(GameState, "xDeriv min:max    : %f:%f:%f", GameState->xDerivMin, GameState->xDerivMax, GameState->xDerivMax-GameState->xDerivMin);
    ui_text(GameState, "yDeriv min:max    : %f:%f:%f", GameState->yDerivMin, GameState->yDerivMax, GameState->yDerivMax-GameState->yDerivMin);
    ui_text(GameState, "zDeriv min:max    : %f:%f:%f", GameState->zDerivMin, GameState->zDerivMax, GameState->zDerivMax-GameState->zDerivMin);


    if(GameState->valNoise3dHits > 0){
        GameState->valNoise3dCycles = GameState->valNoise3dCyclesTotal / GameState->valNoise3dHits;
    }

    if(GameState->chunkData->perlinNoise3dHits > 0){
        GameState->chunkData->perlinNoise3dCycles = GameState->chunkData->perlinNoise3dCyclesTotal / GameState->chunkData->perlinNoise3dHits;
        GameState->chunkData->perlinNoise3dScalarLookup = GameState->chunkData->perlinNoise3dScalarLookupTotal / GameState->chunkData->perlinNoise3dHits;
        GameState->chunkData->perlinNoise3dSIMDLookup   = GameState->chunkData->perlinNoise3dSIMDLookupTotal / GameState->chunkData->perlinNoise3dHits;
        GameState->chunkData->perlinNoise3dScalar = GameState->chunkData->perlinNoise3dScalarTotal / GameState->chunkData->perlinNoise3dHits;
        GameState->chunkData->perlinNoise3dSIMD = GameState->chunkData->perlinNoise3dSIMDTotal / GameState->chunkData->perlinNoise3dHits;
    }

    if(GameState->chunkData->grad4xHits > 0){
        GameState->chunkData->grad4x = GameState->chunkData->grad4xTotal / GameState->chunkData->grad4xHits;
    }
    if(GameState->drawTextureHits > 0){
        GameState->drawTextureCycles = GameState->drawTextureTotal / GameState->drawTextureHits;
    }

    if(GameState->chunkData->perlinThreadHits > 0){
        GameState->chunkData->perlinThreadCycles = GameState->chunkData->perlinThreadTotal / GameState->chunkData->perlinThreadHits;
    }

    if(GameState->chunkData->workThreadHits > 0){
        GameState->chunkData->workThreadCycles = GameState->chunkData->workThreadTotalCycles / GameState->chunkData->workThreadHits;
    }

    if(GameState->chunkData->workThreadAccumulateVoxelsHits > 0){
        GameState->chunkData->workThreadAccumulateVoxelsCycles = GameState->chunkData->workThreadAccumulateVoxelsTotalCycles / GameState->chunkData->workThreadAccumulateVoxelsHits;
    }

    // ui_text(GameState, "valNoise3d    Total  : %zu", GameState->valNoise3dCyclesTotal);
    // ui_text(GameState, "valNoise3d    Hits   : %zu", GameState->valNoise3dHits);
    // ui_text(GameState, "valNoise3d    Cycles : %zu", GameState->valNoise3dCycles);

    //for 2d texture perlin generation
    // ui_text(GameState, "perlinNoise3d Total  : %zu", GameState->chunkData->perlinNoise3dCyclesTotal);
    // ui_text(GameState, "perlinNoise3d Hits   : %zu", GameState->chunkData->perlinNoise3dHits);
    // ui_text(GameState, "perlinNoise3d Cycles : %zu", GameState->chunkData->perlinNoise3dCycles);
    // ui_text(GameState, "perlinNoise3d SCLRUP : %zu", GameState->chunkData->perlinNoise3dScalarLookup);
    // ui_text(GameState, "perlinNoise3d Scalar : %zu", GameState->chunkData->perlinNoise3dScalar);
    // ui_text(GameState, "perlinNoise3d SIMDP  : %zu", GameState->chunkData->perlinNoise3dSIMDLookup);
    // ui_text(GameState, "perlinNoise3d SIMD   : %zu", GameState->chunkData->perlinNoise3dSIMD);
    // ui_text(GameState, "grad4x Total         : %zu", GameState->chunkData->grad4xTotal);
    // ui_text(GameState, "grad4x Hits          : %zu", GameState->chunkData->grad4xHits);
    // ui_text(GameState, "grad4x Cycles        : %zu", GameState->chunkData->grad4x);

    // ui_text(GameState, "Texture Total         : %zu", GameState->drawTextureTotal);
    // ui_text(GameState, "Texture Hits          : %zu", GameState->drawTextureHits);
    // ui_text(GameState, "Texture Cycles        : %zu", GameState->drawTextureCycles);

    ui_text(GameState, "Thread Perlin Total  : %zu", GameState->chunkData->perlinThreadTotal);
    ui_text(GameState, "Thread Perlin Hits   : %zu", GameState->chunkData->perlinThreadHits);
    ui_text(GameState, "Thread Perlin Cycles : %zu", GameState->chunkData->perlinThreadCycles);

    ui_text(GameState, "Worker Total         : %zu", GameState->chunkData->workThreadTotalCycles);
    ui_text(GameState, "Worker Hits          : %zu", GameState->chunkData->workThreadHits);
    ui_text(GameState, "Worker Cycles        : %zu", GameState->chunkData->workThreadCycles);
    
    ui_text(GameState, "AccumVox Total       : %zu", GameState->chunkData->workThreadAccumulateVoxelsTotalCycles);
    ui_text(GameState, "AccumVox Hits        : %zu", GameState->chunkData->workThreadAccumulateVoxelsHits);
    ui_text(GameState, "AccumVox Cycles      : %zu", GameState->chunkData->workThreadAccumulateVoxelsCycles);
    
    char buf[256];
    int len = 0;
    for (int i = 0; i < dest->buffer_size; i++) {
        if (dest->buffer[i] == 0) {
            buf[i] = 127;
        }
        else if (dest->buffer[i] == 13) {
            buf[i] = '~';
        }
        else {
            buf[i] = dest->buffer[i];
        }
        len++;
    }
    buf[len] = 0;
    // ui_text(GameState, buf);
    ui_text_highlight(GameState, 30.0f, dest->gapStart, dest->gapEnd - dest->gapStart, buf);

    //text editor buttons, clear/reset text editor
    ui_hash_entry* inlineElementHash = ui_begin_inline(GameState, 50.0f,"testInline0");
    bool testClicked = false;//not used for anything, needed for the function args because im retarded
    ui_element_data appendedData = {};
    ui_append_inline(GameState, ui_element_type_button, "Clear", testClicked, 0, 50, 0, 50, inlineElementHash, appendedData);
    TextInputState* editorDest = &uiData->windows[ui_window_types::window_type_book_text].data.book_text.text_input;
    if (appendedData.button.clicked) {
        memset(editorDest->buffer, 0, sizeof(char) * editorDest->buffer_size);
        memset(editorDest->cursor_positions, 0, sizeof(vec2) * editorDest->buffer_size);
        memset(editorDest->newLines, 0, sizeof(size_t) * editorDest->buffer_size);
        editorDest->length = 0;
        editorDest->gapStart = 0;
        editorDest->gapEnd = editorDest->buffer_size - 1;
        editorDest->lineCount = 0;
        editorDest->cursor_pos = 0;
        editorDest->cursor_num_positions = 0;

    }

if(GameState->windowResized){
   debugWindow->fixed = true; 
#if LABOR_INTERNAL
#else
    GameState->windowResized = false;
#endif
}
if(debugWindow->fixed){
    ui_end_window(GameState, {0.895f, 0.5f, 260.0f, 600.0f});

}else{
    ui_end_window(GameState);
}
#endif


}

void advance_entity_air_state(game_state* GameState, StateComp* state, PhysicsComp* phys){   
    EntityComponent& ec = *GameState->entityComponent;
    if (state->smAir.cur != entity_state_none) {
    // printf("entity state: %s\n", get_entity_state_name(state->current));
        uint32_t& curr_action_index = state->smAir.action.curr_action_index;
        uint32_t& prev_action_index = state->smAir.action.prev_action_index;

        entity_state_action current_action = {};
        for (; curr_action_index < MAX_STATE_ACTIONS; curr_action_index++) {
            current_action = ec.entity_state_actions[state->body_type][state->smAir.cur][curr_action_index];
            // printf("current action type: %d, tick: %u, current action index: %d\n", current_action.type, current_action.tick, curr_action_index);
            if (state->smAir.cur == entity_movement_state_land && state->landingIntensity < fl2fpt(19.0f)){//move the state faster if the landing intensity is small
                state->smAir.action.tick+=5;
            }
            if (current_action.tick > state->smAir.action.tick || current_action.type == action_type_none)break;

            switch (current_action.type) {
                case action_type_hitbox: {
                    // printf("HITBOX ACTION AT TICK %u\n", current_action.tick);
                    // if (trans_index != NULL_ENTITY) {
                        // TransComp& transComp = ec.TransComps[trans_index];
                        // GameState->chunkData->test_hitbox_pos = transComp.pos_in_chunk + (transComp.forward * FPT_TWO);
                        state->debug_print_hitbox_positions = true;
                    // }
                    break;
                }
                case action_type_lock_movement: {
                    // printf("LOCK MOVEMENT ACTION AT TICK %u\n", current_action.tick);        
                    // state->smMove.locked = true;
                    break;
                }
                case action_type_unlock_movement: {
                    // printf("UNLOCK MOVEMENT ACTION AT TICK %u\n", current_action.tick);      
                    state->smMove.locked = false;

                    break;
                }
                case action_type_lock_interact: {
                    // printf("LOCK STATE ACTION AT TICK %u\n", current_action.tick);           
                    state->smAct.locked = true;
                    break;
                }
                case action_type_velocity: {
                      phys->velocity += current_action.velocity;
                      state->grounded = false;
                }break;
                case action_type_end_state: {
                    // printf("END STATE ACTION AT TICK %u\n", current_action.tick);            
                    state->smAir.locked = false;
                    state->debug_print_hitbox_positions = false;
                    state->smMove.locked = false;
                    state->smAir.action.tick = 0; //reset
                    break;
                }
            }

            prev_action_index = curr_action_index;
        }

        state->smAir.action.tick++;

    }
}

void advance_entity_movement_state(game_state* GameState, StateComp* state, PhysicsComp* phys){   
    EntityComponent& ec = *GameState->entityComponent;
    if (state->smMove.cur != entity_state_none) {
    // printf("entity state: %s\n", get_entity_state_name(state->current));
        uint32_t& curr_action_index = state->smMove.action.curr_action_index;
        uint32_t& prev_action_index = state->smMove.action.prev_action_index;

        entity_state_action current_action = {};
        for (; curr_action_index < MAX_STATE_ACTIONS; curr_action_index++) {
            current_action = ec.entity_state_actions[state->body_type][state->smMove.cur][curr_action_index];
            // printf("current action type: %d, tick: %u, current action index: %d\n", current_action.type, current_action.tick, curr_action_index);
            if (state->smMove.cur == entity_movement_state_land && state->landingIntensity < fl2fpt(19.0f)){//move the state faster if the landing intensity is small
                state->smMove.action.tick+=5;
            }
            if (current_action.tick > state->smMove.action.tick || current_action.type == action_type_none)break;

            switch (current_action.type) {
                case action_type_hitbox: {
                    // printf("HITBOX ACTION AT TICK %u\n", current_action.tick);
                    // if (trans_index != NULL_ENTITY) {
                        // TransComp& transComp = ec.TransComps[trans_index];
                        // GameState->chunkData->test_hitbox_pos = transComp.pos_in_chunk + (transComp.forward * FPT_TWO);
                        state->debug_print_hitbox_positions = true;
                    // }
                    break;
                }
                case action_type_lock_movement: {
                    // printf("LOCK MOVEMENT ACTION AT TICK %u\n", current_action.tick);        
                    state->smMove.locked = true;
                    break;
                }
                case action_type_unlock_movement: {
                    // printf("UNLOCK MOVEMENT ACTION AT TICK %u\n", current_action.tick);      
                    state->smMove.locked = false;

                    break;
                }
                case action_type_lock_interact: {
                    // printf("LOCK STATE ACTION AT TICK %u\n", current_action.tick);           
                    state->smAct.locked = true;
                    break;
                }
                case action_type_velocity: {
                      phys->velocity += current_action.velocity;
                      state->grounded = false;
                }break;
                case action_type_end_state: {
                    // printf("END STATE ACTION AT TICK %u\n", current_action.tick);            
                    state->smAct.locked = false;
                    state->debug_print_hitbox_positions = false;
                    state->smMove.locked = false;
                    state->smMove.action.tick = 0; //reset
                    break;
                }
            }

            prev_action_index = curr_action_index;
        }

        state->smMove.action.tick++;

    }
}


void advance_entity_interact_state(game_state* GameState, StateComp* state){
    EntityComponent& ec = *GameState->entityComponent;
    if (state->smAct.cur != entity_state_none) {
    // printf("entity state: %s\n", get_entity_state_name(state->current));
        uint32_t& curr_action_index = state->smAct.action.curr_action_index;
        uint32_t& prev_action_index = state->smAct.action.prev_action_index;

        entity_state_action current_action = {};
        for (; curr_action_index < MAX_STATE_ACTIONS; curr_action_index++) {
            current_action = ec.entity_state_actions[state->body_type][state->smAct.cur][curr_action_index];
            // printf("current action type: %d, tick: %u, current action index: %d\n", current_action.type, current_action.tick, curr_action_index);
            if (current_action.tick > state->smAct.action.tick || current_action.type == action_type_none)break;

            switch (current_action.type) {
                case action_type_hitbox: {
                    // printf("HITBOX ACTION AT TICK %u\n", current_action.tick);
                    // if (trans_index != NULL_ENTITY) {
                        // TransComp& transComp = ec.TransComps[trans_index];
                        // GameState->chunkData->test_hitbox_pos = transComp.pos_in_chunk + (transComp.forward * FPT_TWO);
                        state->debug_print_hitbox_positions = true;
                    // }
                    break;
                }
                case action_type_lock_movement: {
                    // printf("LOCK MOVEMENT ACTION AT TICK %u\n", current_action.tick);        
                    state->smMove.locked = true;
                    break;
                }
                case action_type_unlock_movement: {
                    // printf("UNLOCK MOVEMENT ACTION AT TICK %u\n", current_action.tick);      
                    state->smMove.locked = false;

                    break;
                }
                case action_type_lock_interact: {
                    // printf("LOCK STATE ACTION AT TICK %u\n", current_action.tick);           
                    state->smAct.locked = true;
                    break;
                }
                case action_type_end_state: {
                    // printf("END STATE ACTION AT TICK %u\n", current_action.tick);            
                    state->smAct.locked = false;
                    state->debug_print_hitbox_positions = false;
                    state->smMove.locked = false;
                    state->smAct.action.tick = 0; //reset

                    break;
                }
            }

            prev_action_index = curr_action_index;
        }

        state->smAct.action.tick++;

    }
}

void physics_update(game_state* GameState, EntityComponent& ec, TransComp* trans, PhysicsComp* phys){
    TIMED_BLOCK("physics_update");
    //update gravity and integrate
    fpt timeStep = GameState->fptFixedTimeStep;
    fpt_vec3 gravity = {0, -6553600, 0};//-100 in fixed point
    phys->acceleration = gravity;

    fpt_vec3 angular_acceleration = fpt_vec3_flatmat3_transform(phys->torque_accumulator,phys->inverse_inertia_tensor_world);

    fpt_vec3 zero_vector = fpt_vec3_create(0);
    if(angular_acceleration     == zero_vector
        && phys->force_accumulator == zero_vector
        && phys->acceleration == zero_vector
        && phys->velocity     == zero_vector
        && phys->rotation     == zero_vector
        ){return;}

    //linear acceleration update
    phys->last_frame_acceleration = phys->acceleration;
    fpt_vec3 resulting_acceleration = phys->acceleration;

    fpt_vec3_add_scaled(resulting_acceleration, phys->force_accumulator, timeStep);
    fpt_vec3_add_scaled(phys->velocity, resulting_acceleration, timeStep);
    phys->velocity *= fpt_pow(phys->linear_damping, timeStep);
    phys->rotation *= fpt_pow(phys->angular_damping, timeStep);

    //position update
    fpt_vec3 movement = {};
    fpt_vec3_add_scaled(movement, phys->velocity, timeStep);
    apply_desired_movement(*trans, movement);
    
    //nullify force accumulator?
    phys->force_accumulator = {};

}


void entity_sim_update(game_state* GameState) {
    TIMED_BLOCK("entity_sim_update");

    Assert(GameState->fptFixedTimeStep > 0 && "FIDXED TIME STEP IS 0?? ERROR");
    
    uint32_t new_index = GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1);
    uint32_t prev_index = (GameState->currentTick - 1) & (SNAPSHOT_BUFFER_SIZE - 1);
    player_input* prevInput = &GameState->playerInputs[0][prev_index];
    player_input* currInput = &GameState->playerInputs[0][new_index];


    EntityComponent& ec = *GameState->entityComponent;

    //al entities will have a data component attached to them, so loop over all data components
    for (int dataIndex = 0; dataIndex < ec.DataCount; dataIndex++) {
        uint32_t entityID = ec.DataToEntityMap[dataIndex];
        DataComp& dataComp = ec.DataComps[dataIndex];

        uint32_t state_index = ec.entityToStateMap[entityID];
        uint32_t trans_index = ec.entityToTransMap[entityID];
        uint32_t physicsIndex = ec.entityToPhysicsMap[entityID];
        PhysicsComp* phys = ec.PhysicsComps + physicsIndex;


        if (trans_index != NULL_ENTITY) {
            TransComp& trans = ec.TransComps[trans_index];
            trans.desired_movement = fpt_vec3_create(0);
            trans.move_delta = fpt_vec3_create(0);
        }


        //need to remove this once entity system is more fleshed out

        uint32_t stateIndex = ec.entityToStateMap[entityID];



        //move camera entity around with the camera
        uint32_t transIndex = ec.entityToTransMap[entityID];



        TransComp& trans = ec.TransComps[transIndex];
        trans.desired_movement = fpt_vec3_create(0);
        trans.move_delta = fpt_vec3_create(0);

        //just update/move the selected/main player
        uint32_t focusedEntityID = GameState->localPlayerEntityIDs[0];
        uint8_t playerIndex = ec.entityToPlayerMap[focusedEntityID];
        if (entityID == focusedEntityID && playerIndex != NULL_ENTITY) {

            uint16_t invIndex = ec.entityToInventoryMap[entityID];

            InventoryComp& invComp = ec.InventoryComps[invIndex];


            uint32_t cameraIndex = ec.entityToCameraMap[focusedEntityID];
            CameraComp& cameraComp = ec.CameraComps[cameraIndex];


            OnKeyboardEvent(prevInput, currInput, cameraComp);


            //if freecam mode turned off
            bool freeCamJustDisabled = WAS_RELEASED((*currInput), flags.freeCam, InputTypes::input_key_Q);
            if (freeCamJustDisabled)GameState->chunkData->lockMouseMotion = false;
            bool camera_dirty = false;
            if ((!currInput->consumedMouse.left && (currInput->mouse.left || currInput->mouse.middle) && (!currInput->consumedMouse.delta && (currInput->mouse_dx != 0 || currInput->mouse_dy != 0)))) {
                // if(currInput->mouse_x >= 0 && currInput->mouse_x <= *GameState->window_width &&currInput->mouse_y >= 0 && currInput->mouse_y <= *GameState->window_height){
                OnMouseEvent(cameraComp, currInput->mouse_x, currInput->mouse_y, currInput->mouse_dx, currInput->mouse_dy);
                camera_dirty = true;
                GameState->playerInputs[0][GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1)].fptAngleH = cameraComp.fptAngleH;
                GameState->playerInputs[0][GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1)].fptAngleV = cameraComp.fptAngleV;
                // } 

            }


            PlayerComp& PlayerComp = ec.PlayerComps[playerIndex];

            StateComp& state = ec.StateComps[stateIndex];
            fpt_vec3 translation = fpt_vec3_create(0, 0, 0);
            fpt_vec3 movement = fpt_vec3_create(0, 0, 0);


            if (!cameraComp.freeMode && currInput->function.keyCTRL && currInput->mouse_wheel) {
                fpt camera_zoom = i2fpt(-currInput->mouse_wheel);//invert it because scrolling up should move TOWARD the entity
                fpt final_zoom = fpt_add(cameraComp.third_person_offset, camera_zoom);
                if (final_zoom > FPT_MAX_CAMERA_ZOOM) final_zoom = FPT_MAX_CAMERA_ZOOM;
                else if (final_zoom < 0)final_zoom = 0;
                cameraComp.third_person_offset = final_zoom;
                // if(final_zoom == 0){
                //     GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].head_socket].draw = false;
                // }else{
                //     GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].head_socket].draw = true;

                // }
            }

            bool freeCamJustEnabled = WAS_PRESSED((*currInput), flags.freeCam, InputTypes::input_key_Q);
            if ((!cameraComp.freeMode || freeCamJustEnabled) && isEntityActive(ec, focusedEntityID)) {
                // PlayerComp.rayDir = currInput->rayDir;


                // if (!state.is_movement_locked) {
                    fpt_vec3 temp_pos = trans.pos_in_chunk;
                    // handle_player_entity_inputs(currInput, trans.forward, trans.up, trans.speed, trans.pos_in_chunk, trans.chunk_coords, &trans.inNewChunk, GameState->camera->toroidal_space_enabled);
                    if (currInput->bits.forward) {
                        movement = fpt_vec3_normalize(cameraComp.fptTarget);
                        movement.y = 0;
                        movement = fpt_vec3_normalize(movement);
                        translation = translation + movement;
                    }
                    if (currInput->bits.back) {
                        movement = fpt_vec3_normalize(cameraComp.fptTarget);
                        movement = fpt_vec3_normalize(movement);
                        movement.y = 0;
                        translation = translation - movement;
                    }
                    if (currInput->bits.left) {
                        movement = fpt_vec3_normalize(fpt_vec3_cross(cameraComp.fptUp, cameraComp.fptTarget));
                        movement.y = 0;
                        movement = fpt_vec3_normalize(movement);
                        translation = translation + movement;

                    }
                    if (currInput->bits.right) {
                        movement = fpt_vec3_normalize(fpt_vec3_cross(cameraComp.fptTarget, cameraComp.fptUp));
                        movement.y = 0;
                        movement = fpt_vec3_normalize(movement);
                        translation = translation + movement;
                    }
                    //up and down disabled for gravity
                    #if 0
                    if (currInput->bits.up) {
                        translation = translation + fpt_vec3_create(0, FPT_ONE, 0);
                    }
                    if (currInput->bits.down) {
                        translation = translation - fpt_vec3_create(0, FPT_ONE, 0);
                    }
                    #endif
                    if (!(translation.x == 0 && translation.y == 0 && translation.z == 0)) {
                        translation = fpt_vec3_normalize(translation);
                        translation = fpt_vec3_mul_scalar(translation, trans.speed);
                        apply_desired_movement(trans, translation);
                        camera_dirty = true;

                    }
                // }
                if (camera_dirty)update_cameraComp_position(GameState, focusedEntityID, cameraComp, trans);


            }

            //we definitely need to refactor this into something cleaner
            if (!state.smAct.locked) {
                entity_state_type new_interact_state = {};
                //left punch
                // printf("mouse left: %u, consumed: %u\n", WAS_PRESSED((*currInput), mouse.left, InputTypes::input_mouse_left), currInput->consumedMouse.left);
                 if (!currInput->consumedMouse.right && WAS_PRESSED((*currInput), mouse.right, InputTypes::input_mouse_right)) {
                    if (state.smAct.cur != entity_combat_state_right_punch) {
                        new_interact_state = entity_combat_state_right_punch;
                        state.smAct.preTrack = state.smAct.curTrack;
                        transition_entity_state(state, &state.smAct, new_interact_state, true);
                        state.smAct.curTrack.anim_type = state_to_animation_type(new_interact_state);
                        get_blend_weights(&ec, &state.smAct.curTrack); 
                        // printf("transition to RIGHT PUNCH\n");
                        state.smAct.blendTarget = 1.0f;
                        state.smAct.blendstep = 0.01f;
                    }
                }
                else if (!currInput->consumedMouse.left && WAS_PRESSED((*currInput), mouse.left, InputTypes::input_mouse_left)) {
                    if (state.smAct.cur != entity_combat_state_left_punch) {
                        new_interact_state = entity_combat_state_left_punch;
                        state.smAct.preTrack = state.smAct.curTrack;
                        transition_entity_state(state, &state.smAct, new_interact_state, true);
                        state.smAct.curTrack.anim_type = state_to_animation_type(new_interact_state);
                        get_blend_weights(&ec, &state.smAct.curTrack); 
                        // printf("transition to LEFT PUNCH\n");
                        state.smAct.blendTarget = 1.0f;
                        state.smAct.blendstep = 0.01f;
                    }
                }
                else {
                    if (state.smAct.cur != entity_interact_state_idle) {
                        new_interact_state = entity_interact_state_idle;
                        state.smAct.preTrack = state.smAct.curTrack;
                        transition_entity_state(state, &state.smAct, new_interact_state);
                        state.smAct.curTrack.anim_type = state_to_animation_type(new_interact_state);
                        get_blend_weights(&ec, &state.smAct.curTrack); 
                        // printf("transition to IDLE\n");
                        state.smAct.blendTarget = 0.0f;
                        state.smAct.blendstep = 0.2f;
                    }
                }
                               
                if      (WAS_PRESSED((*currInput), numbers.key1, InputTypes::input_key_1)) { invComp.current_left_hand  = 0; }
                else if (WAS_PRESSED((*currInput), numbers.key2, InputTypes::input_key_2)) { invComp.current_left_hand  = 1; }
                else if (WAS_PRESSED((*currInput), numbers.key3, InputTypes::input_key_3)) { invComp.current_left_hand  = 2; }
                else if (WAS_PRESSED((*currInput), numbers.key4, InputTypes::input_key_4)) { invComp.current_left_hand  = 3; }
                else if (WAS_PRESSED((*currInput), numbers.key5, InputTypes::input_key_5)) { invComp.current_left_hand  = 4; }
                if      (WAS_PRESSED((*currInput), numbers.key6, InputTypes::input_key_6)) { invComp.current_right_hand = 0; }
                else if (WAS_PRESSED((*currInput), numbers.key7, InputTypes::input_key_7)) { invComp.current_right_hand = 1; }
                else if (WAS_PRESSED((*currInput), numbers.key8, InputTypes::input_key_8)) { invComp.current_right_hand = 2; }
                else if (WAS_PRESSED((*currInput), numbers.key9, InputTypes::input_key_9)) { invComp.current_right_hand = 3; }
                else if (WAS_PRESSED((*currInput), numbers.key0, InputTypes::input_key_0)) { invComp.current_right_hand = 4; }


            }
            if(!state.smMove.locked){
                entity_state_type new_move_state = {};
               
                if (translation.x || translation.z) {
                    if (state.smMove.cur != entity_movement_state_walk) {
                        new_move_state = entity_movement_state_walk;
                        state.smMove.preTrack = state.smMove.curTrack;
                        transition_entity_state(state, &state.smMove, new_move_state);
                        state.smMove.curTrack.anim_type = state_to_animation_type(new_move_state);
                        get_blend_weights(&ec, &state.smMove.curTrack); 
                        state.smMove.blend = 0.1f;
                        state.smMove.blendTarget = 1.0f;
                        state.smMove.blendstep = 0.01f;

                    }
                }
                else {
                    if (state.smMove.cur != entity_movement_state_idle) {
                        new_move_state = entity_movement_state_idle;
                        state.smMove.preTrack = state.smMove.curTrack;
                        transition_entity_state(state, &state.smMove, new_move_state);
                        state.smMove.curTrack.anim_type = state_to_animation_type(new_move_state);
                        get_blend_weights(&ec, &state.smMove.curTrack); 
                        state.smMove.blend = 0.1f;
                        state.smMove.blendTarget = 1.0f;
                        // printf("transition to IDLE\n");
                        state.smMove.blendstep = 0.01f;
                    }
                }
                
            }
            if(!state.smAir.locked){
                entity_state_type new_move_state = {};

                 if (WAS_PRESSED((*currInput), bits.up, InputTypes::input_key_up) && state.grounded) {
                    if (state.smAir.cur != entity_movement_state_jump) {
                        new_move_state = entity_movement_state_jump;
                        state.smAir.preTrack = state.smAir.curTrack;
                        transition_entity_state(state, &state.smAir, new_move_state);
                        state.smAir.curTrack.anim_type = state_to_animation_type(new_move_state);
                        get_blend_weights(&ec, &state.smAir.curTrack); 
                        state.smAir.blendTarget = 1.0f;
                        state.smAir.blendstep = 0.01f;
                    }
                }
                else if (!state.grounded) {
                    if (state.smAir.cur != entity_movement_state_airborn) {
                        new_move_state = entity_movement_state_airborn;
                        state.smAir.preTrack = state.smAir.curTrack;
                        transition_entity_state(state, &state.smAir, new_move_state);
                        state.smAir.curTrack.anim_type = state_to_animation_type(new_move_state);
                        get_blend_weights(&ec, &state.smAir.curTrack); 
                        state.smAir.blendTarget = 1.0f;
                        state.smAir.blendstep = 0.01f;
                    }
                }
                else if (state.landed) {
                    state.landed = false;
                    if (state.smAir.cur != entity_movement_state_land) {
                        new_move_state = entity_movement_state_land;
                        state.smAir.preTrack = state.smAir.curTrack;
                        transition_entity_state(state, &state.smAir, new_move_state, true);
                        state.smAir.curTrack.anim_type = state_to_animation_type(new_move_state);
                        get_blend_weights(&ec, &state.smAir.curTrack); 
                        state.smAir.blendTarget = 1.0f;
                        state.smAir.blendstep = 0.01f;
                    }
                }
                else if(state.smAir.cur != entity_movement_state_jump){//as long as we arent jumping 
                    if (state.smAir.cur != entity_movement_state_idle) {
                        new_move_state = entity_movement_state_idle;
                        state.smAir.preTrack = state.smAir.curTrack;
                        transition_entity_state(state, &state.smAir, new_move_state);
                        state.smAir.curTrack.anim_type = state_to_animation_type(new_move_state);
                        get_blend_weights(&ec, &state.smAir.curTrack); 
                        // printf("transition to IDLE\n");
                        state.smAir.blendTarget = 0.0f;
                        state.smAir.blendstep = 0.01f;

                    }
                }
            }




            if (!cameraComp.freeMode && camera_dirty)chunk_camera_updated(GameState);//slop to update the chunks since its still tied to the camera ((for now))



        }



        if (state_index != NULL_ENTITY) {
            StateComp& stateComp = ec.StateComps[state_index];
            //advance entity states
           advance_entity_interact_state(GameState, &stateComp);
           advance_entity_movement_state(GameState, &stateComp, phys);
           advance_entity_air_state(GameState, &stateComp, phys);

        }


        //check if the entity can interact with something in the chunk
        if (dataIndex != NULL_ENTITY) {
            if (dataComp.entity_interaction_lookup_chunkID != NULL_CHUNK && dataComp.entity_interaction_lookup_index != NULL_ENTITY) {
                entity_interaction_data& data = GameState->chunkData->entity_interactions[dataComp.entity_interaction_lookup_chunkID][dataComp.entity_interaction_lookup_index];

                bool data_invalidated = true;
                if (data.interacting_entityID == entityID && ec.versionIDs[entityID] == data.interacting_versionID &&
                    ec.versionIDs[data.interactable_entityID] == data.interactable_versionID &&
                    data.interacting_type == dataComp.type) {
                    //valid interaction data available
                    //if its a player entity, check for input to see if the item is interacted with/picked up
                    //just update/move the selected/main player
                    uint16_t invIndex = ec.entityToInventoryMap[entityID];

                    if (entityID == focusedEntityID && playerIndex != NULL_ENTITY) {
                        if (WAS_PRESSED((*currInput), bits.interact, InputTypes::input_key_interact)) {
                            printf("entity %d interacting with entity %d!\n", entityID, data.interactable_entityID);
                            if (invIndex != NULL_ENTITY) {
                                InventoryComp& invComp = ec.InventoryComps[invIndex];
                                uint16_t interactable_dataIndex = ec.entityToDataMap[data.interactable_entityID];
                                DataComp& interactable_dataComp = ec.DataComps[interactable_dataIndex];

                                inventory_item& inv_item = ec.inventory_items[interactable_dataComp.item_index];
                                bool equipped = false;
                                //doesnt matter what the type is, any item that is picked up will go to the hotbar first if theres a free slot
                                u32* equipped_slot = nullptr;
                                //this pickup code will need to evolve with my preferencess
                                //check for slot imbalance and give that picked up item to the lesser populated hand
                                if (invComp.left_hand_count < invComp.right_hand_count && invComp.left_hand_count < MAX_ITEMS_PER_HAND) {
                                    //item goes to left hand
                                    for (int i = 0; i < MAX_ITEMS_PER_HAND; i++) {
                                        if (!invComp.left_hand[i]) {
                                            equipped = true;
                                            equipped_slot = invComp.left_hand + i;
                                            if (!invComp.left_hand_count)invComp.current_left_hand = i;
                                            invComp.left_hand_count++;
                                            break;
                                        }
                                    }
                                }
                                else if (invComp.right_hand_count < invComp.left_hand_count && invComp.right_hand_count < MAX_ITEMS_PER_HAND) {
                                    //item goes to right hand
                                    for (int i = 0; i < MAX_ITEMS_PER_HAND; i++) {
                                        if (!invComp.right_hand[i]) {
                                            equipped = true;
                                            equipped_slot = invComp.right_hand + i;
                                            if (!invComp.right_hand_count)invComp.current_right_hand = i;
                                            invComp.right_hand_count++;
                                            break;

                                        }
                                    }
                                }
                                else if (invComp.left_hand_count < MAX_ITEMS_PER_HAND) {
                                    //item goes to left hand
                                    for (int i = 0; i < MAX_ITEMS_PER_HAND; i++) {
                                        if (!invComp.left_hand[i]) {
                                            equipped = true;
                                            equipped_slot = invComp.left_hand + i;
                                            if (!invComp.left_hand_count)invComp.current_left_hand = i;
                                            invComp.left_hand_count++;
                                            break;
                                        }
                                    }
                                }
                                else if (invComp.right_hand_count < MAX_ITEMS_PER_HAND) {
                                    //item goes to right hand
                                    for (int i = 0; i < MAX_ITEMS_PER_HAND; i++) {
                                        if (!invComp.right_hand[i]) {
                                            equipped = true;
                                            equipped_slot = invComp.right_hand + i;
                                            if (!invComp.right_hand_count)invComp.current_right_hand = interactable_dataComp.item_index;
                                            invComp.right_hand_count++;
                                            break;

                                        }
                                    }

                                }
                                //if the hands are full, check inventory
                                else if (invComp.item_count < MAX_ITEMS) {
                                    for (int i = 0; i < MAX_ITEMS; i++) {
                                        equipped = true;
                                        equipped_slot = invComp.items + i;
                                        invComp.item_count++;
                                        break;

                                    }
                                }
                                else if (inv_item.data.type == item_types::item_type_trinket && invComp.item_count < MAX_TRINKETS) {
                                    for (int i = 0; i < MAX_TRINKETS; i++) {
                                        equipped = true;
                                        equipped_slot = invComp.trinkets + i;
                                        invComp.trinket_count++;

                                        break;

                                    }
                                }
                                else if (inv_item.data.type == item_types::item_type_equipment && invComp.equipment_count < MAX_EQUIPMENT) {
                                    switch (inv_item.data.equipment.type) {

                                    case equipment_type_helmet: {
                                        if (!invComp.equipment[equipment_socket_type::equip_socket_head]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_head;
                                            invComp.equipment_count++;

                                        }
                                    }break;

                                    case equipment_type_torso: {
                                        if (!invComp.equipment[equipment_socket_type::equip_socket_torso]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_torso;
                                            invComp.equipment_count++;


                                        }
                                    }break;

                                    case equipment_type_upper_arm: {
                                        if (!invComp.equipment[equipment_socket_type::equip_socket_left_upper_arm]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_left_upper_arm;
                                            invComp.equipment_count++;


                                        }
                                        else if (!invComp.equipment[equipment_socket_type::equip_socket_right_upper_arm]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_right_upper_arm;
                                            invComp.equipment_count++;


                                        }
                                    }break;

                                    case equipment_type_lower_arm: {
                                        if (!invComp.equipment[equipment_socket_type::equip_socket_left_fore_arm]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_left_fore_arm;
                                            invComp.equipment_count++;


                                        }
                                        else if (!invComp.equipment[equipment_socket_type::equip_socket_right_fore_arm]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_right_fore_arm;
                                            invComp.equipment_count++;

                                        }
                                    }break;

                                    case equipment_type_upper_leg: {
                                        if (!invComp.equipment[equipment_socket_type::equip_socket_left_thigh]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_left_thigh;
                                            invComp.equipment_count++;

                                        }
                                        else if (!invComp.equipment[equipment_socket_type::equip_socket_right_thigh]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_right_thigh;
                                            invComp.equipment_count++;

                                        }
                                    }break;

                                    case equipment_type_lower_leg: {
                                        if (!invComp.equipment[equipment_socket_type::equip_socket_left_shin]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_left_shin;
                                            invComp.equipment_count++;


                                        }
                                        else if (!invComp.equipment[equipment_socket_type::equip_socket_right_shin]) {
                                            equipped = true;
                                            equipped_slot = invComp.equipment + equipment_socket_type::equip_socket_right_shin;
                                            invComp.equipment_count++;


                                        }
                                    }break;
                                    }
                                }

                                if (equipped && equipped_slot) {
                                    *equipped_slot = interactable_dataComp.item_index;
                                    destroy_entity(GameState, data.interactable_entityID);
                                    //setup logic to destroy picked up entity and move that data to the interacting entity
                                }
                            }

                        }


                        //else its an AI entity, do some other logic here to try to pick up and start using whatever item it is


                    }



                    data_invalidated = false;
                }
                else if (data_invalidated) {//invalid interaction, reset slots
                    dataComp.entity_interaction_lookup_chunkID = NULL_CHUNK;
                    dataComp.entity_interaction_lookup_index = NULL_ENTITY;
                }
            }
        }//end entity/item interaction check

        //physics update


        if (dataIndex != NULL_ENTITY && transIndex != NULL_ENTITY && physicsIndex != NULL_ENTITY) {
            //real shit gravity accumulation/integration
            
            physics_update(GameState, ec, &trans, phys);

        }//end physics update






    }//end entity update loop

}


#if 1
quat update_skeletal_animation_rotation(animation_data* anim_data, float &animation_time){
    int closest_frame = 0;
    int next_frame = 0;
    float t = 0.0f;
    int segment = 0;
    float segment_length = 0.0f;

    //find which segment we're in
    while (segment < anim_data->num_rotation_keyframes - 1 && animation_time > anim_data->rotation_keyframe_times[segment + 1])segment++;
    //loop case
    if (segment == anim_data->num_rotation_keyframes - 1) {
        segment = anim_data->num_rotation_keyframes - 1;
        closest_frame = segment;
        next_frame = 0; //loop back to first frame
        t = 1.0f;

        if (t > 1.0f)t -= 1.0f; //if t > 1 we've wrapped around
        ASSERT_NO_INF_T(t, "loop case t infinite");

    }
    else {//normal case
        closest_frame = segment;
        next_frame = segment + 1;
        t = (animation_time - anim_data->rotation_keyframe_times[closest_frame]) / (anim_data->rotation_keyframe_times[next_frame] - anim_data->rotation_keyframe_times[closest_frame]);
        ASSERT_NO_INF_T(t, "normal case t infinite");

    }
    // printf("rotation animation: %s, time: %10.5f, closest frame: %d, next frame: %d, num_keyframes: %d\n", get_anim_type_name(stateComp.anim_type), animation_time, closest_frame, next_frame, anim_data->num_rotation_keyframes);

    quat closest = quat_create(anim_data->keyframe_rotations[closest_frame].x, anim_data->keyframe_rotations[closest_frame].y, anim_data->keyframe_rotations[closest_frame].z, anim_data->keyframe_rotations[closest_frame].w);
    quat next = quat_create(anim_data->keyframe_rotations[next_frame].x, anim_data->keyframe_rotations[next_frame].y, anim_data->keyframe_rotations[next_frame].z, anim_data->keyframe_rotations[next_frame].w);

    quat rotation =  nlerp(closest, next, t);
    
    ASSERT_NO_NAN_POS(rotation, "Nan detected in position: ");

    // printf("current node rotation       : %3.3f %3.3f %3.3f %3.3f t: %3.3f\n", cur_node->rotation.x, cur_node->rotation.y, cur_node->rotation.z, cur_node->rotation.w, t);
    // printf("closest frame %d rotation   : %3.3f %3.3f %3.3f %3.3f \n", closest_frame, anim_data->keyframe_rotations[closest_frame].x, anim_data->keyframe_rotations[closest_frame].y, anim_data->keyframe_rotations[closest_frame].z, anim_data->keyframe_rotations[closest_frame].w);
    // printf("next frame    %d rotation   : %3.3f %3.3f %3.3f %3.3f \n",next_frame, anim_data->keyframe_rotations[next_frame].x,    anim_data->keyframe_rotations[next_frame].y, anim_data->keyframe_rotations[next_frame].z, anim_data->keyframe_rotations[next_frame].w);

    return rotation;
    
}
vec3 update_skeletal_animation_translation(animation_data* anim_data, float& animation_time){
    int closest_frame = 0;
    int next_frame = 0;
    float t = 0.0f;
    int segment = 0;
    float segment_length = 0.0f;
    //find which segment we're in
    while (segment < anim_data->num_position_keyframes - 1 && animation_time > anim_data->position_keyframe_times[segment + 1])segment++;
    //loop case
    if (segment == anim_data->num_position_keyframes - 1) {
        segment = anim_data->num_position_keyframes - 1;
        closest_frame = segment;
        next_frame = 0; //loop back to first frame
        t = 1.0f;
        ASSERT_NO_INF_T(t, "loop case t infinite");
        if (t > 1.0f)t -= 1.0f; //if t > 1 we've wrapped around
        ASSERT_NO_INF_T(t, "loop case t infinite");

    }
    else {//normal case
        closest_frame = segment;
        next_frame = segment + 1;
        t = (animation_time - anim_data->position_keyframe_times[closest_frame]) / (anim_data->position_keyframe_times[next_frame] - anim_data->position_keyframe_times[closest_frame]);
        ASSERT_NO_INF_T(t, "normal case t infinite");
    }
    // printf("translation animation: %s, time: %10.5f, closest frame: %d, next frame: %d, num_keyframes: %d\n", get_anim_type_name(stateComp.anim_type), animation_time, closest_frame, next_frame, anim_data->num_rotation_keyframes);
    vec3 next = vec3_create(anim_data->keyframe_positions[next_frame].x, anim_data->keyframe_positions[next_frame].y, anim_data->keyframe_positions[next_frame].z);
    vec3 closest = vec3_create(anim_data->keyframe_positions[closest_frame].x, anim_data->keyframe_positions[closest_frame].y, anim_data->keyframe_positions[closest_frame].z);
    vec3 interpolation = mix(closest, next, t);

    vec3 translation = vec3_create(interpolation.x, interpolation.y, interpolation.z);

    
    ASSERT_NO_NAN_POS(translation, "Nan detected in position: ");


    return translation;
}

void update_animation(entity_node* cur_node, animation_data* anim_data, float& animation_time, vec3* translation, quat* rotation){
    if (anim_data->valid_animation) {
        // Interpolate translation
        if (anim_data->has_translation) {
            *translation = update_skeletal_animation_translation(anim_data, animation_time);
        }else *translation = cur_node->base_translation; //set base position if this node has no animation data for this animation type
        // Interpolate rotation (using quaternion slerp for rotation)
        if (anim_data->has_rotation) {
            *rotation = update_skeletal_animation_rotation(anim_data, animation_time);
        }else *rotation = cur_node->base_rotation; //set base rotation if this node has no animation data for this animation type
    }
    else {
        *translation = cur_node->base_translation;
        *rotation = cur_node->base_rotation;
    }

}

void render_animated_model_blend(game_state* GameState, CameraComp& cameraComp, StateComp& stateComp, model_type modelType = model_none, ivec3 chunk_coords = ivec3_create(0), vec3 pos = vec3_create(0), quat base_rotation = quat_identity(),
    float given_scale = 1.0f, vec3 mounting_point = vec3_create(0), bool draw_weapon_minmax = false, quat base_weapon_rotation = quat_identity(), vec3 base_weapon_translation = vec3_create(0)) {
    TIMED_BLOCK("render_animated_model_blend");
    // printf("render_animated_model() start, anim_time: %10.5f\n", stateComp.anim_time);
    gltf_data* cgltfData = GameState->gltfData;
    uint32_t root_node = cgltfData->models[modelType].root_node;

    vec3 test_scale = vec3_create(1);//toggle it and see what happens

    int node_stack[32] = { 0 };
    vec3 relative_translations[32];
    quat relative_rotations[32];
    memset(relative_translations, 0, sizeof(vec3) * 32);
    int node_stack_count = 0;
    relative_translations[node_stack_count] = base_weapon_translation;
    relative_rotations[node_stack_count] = base_weapon_rotation;//cgltfData->nodes[cgltfData->root_nodes[0]].rotation;

    entity_animation_track* cur_move_track          = &stateComp.smMove.curTrack;     
    entity_animation_track* pre_move_track          = &stateComp.smMove.preTrack;     
    entity_animation_track* cur_air_track           = &stateComp.smAir.curTrack;     
    entity_animation_track* pre_air_track           = &stateComp.smAir.preTrack;  
    entity_animation_track* cur_act_track           = &stateComp.smAct.curTrack;  
    entity_animation_track* pre_act_track           = &stateComp.smAct.preTrack;     
    // entity_animation_track* cur_spare_track         = &stateComp.smMove.cur_spare_track;       

    // switch(cur_move_track->anim_type){
    //     case anim_jump:{moveLoop = false;}break;
    // }

    // switch(cur_interact_track->anim_type){
    //     case anim_left_punch:{interactLoop = false;}break;
    //     case anim_right_punch:{interactLoop = false;}break;
    // }

    node_stack[node_stack_count++] = root_node;

    while (node_stack_count > 0) {
        int curID = node_stack[node_stack_count - 1];

        //relative values are for preprocessing the weapon's min/max hitboxes during debugging
        //eventually we will need to print out the values and store them in a look up table once we have more weapons to test with

        vec3 relative_cumulative_translation = relative_translations[node_stack_count - 1];
        quat relative_cumulative_rotation = relative_rotations[node_stack_count - 1];
        node_stack_count--;

        entity_node* cur_node = &cgltfData->nodes[curID];
        // printf("cur node: %s, ID: %d, node stack count: %d, has animation : %d\n", cur_node->name, curID, node_stack_count, cur_node->has_animation);
        // printf("cumulative translation: %10.5f %10.5f %10.5f, cumulative rotation   : %10.5f %10.5f %10.5f %10.5f \n",       cumulative_translation.x, cumulative_translation.y, cumulative_translation.z, cumulative_rotation.x, cumulative_rotation.y, cumulative_rotation.z, cumulative_rotation.w);

        ASSERT_NO_NAN_POS(relative_cumulative_translation, "Nan detected in position: ");
        ASSERT_NO_NAN_ROT(relative_cumulative_rotation, "Nan detected in rotation: ");
        
        quat total_rotation = cur_node->rotation;
        vec3 total_translation = cur_node->translation;
        //animation
        if (cur_node->has_animation && cgltfData->step_animation) {
            //apply animation info
            assert(cur_node->animated_nodeID > -1 && cur_node->animated_nodeID < MAX_ENTITY_NODES);
            animated_node* anim_node = &cgltfData->animated_nodes[cur_node->animated_nodeID];

            // animation_data* anim_data = &anim_node->animations[stateComp.anim_type];

                
 
            // entity_animation_track* next_move_track        = &stateComp.next_move_track;    
            // entity_animation_track* next_interaction_track = &stateComp.next_interaction_track; 
            // entity_animation_track* next_spare_track       = &stateComp.next_spare_track;       
            animation_data* cur_move_animData           = &anim_node->animations[cur_move_track->anim_type];
            animation_data* pre_move_animData           = &anim_node->animations[pre_move_track->anim_type];
            animation_data* cur_air_animData            = &anim_node->animations[cur_air_track->anim_type];
            animation_data* pre_air_animData            = &anim_node->animations[pre_air_track->anim_type];
            animation_data* cur_act_animData            = &anim_node->animations[cur_act_track->anim_type];
            // animation_data* cur_spare_animData          = &anim_node->animations[cur_spare_track->anim_type];
            // animation_data* next_move_animData          = &anim_node->animations[next_move_track->anim_type];
            // animation_data* next_interaction_animData   = &anim_node->animations[next_interaction_track->anim_type];
            // animation_data* next_spare_animData         = &anim_node->animations[next_spare_track->anim_type];

            vec3 mov_translation = total_translation;
            quat mov_rotation = total_rotation;
            vec3 premov_translation = total_translation;
            quat premov_rotation = total_rotation;
            
            vec3 act_translation = total_translation;
            quat act_rotation = total_rotation;

            vec3 air_translation = total_translation;
            quat air_rotation = total_rotation;
            vec3 preair_translation = total_translation;
            quat preair_rotation = total_rotation;


            // update_animation(cur_node, anim_data, stateComp.anim_time, &total_translation, &total_rotation);

            update_animation(cur_node, cur_move_animData, cur_move_track->anim_time, &mov_translation, &mov_rotation);
            update_animation(cur_node, pre_move_animData, pre_move_track->anim_time, &premov_translation, &premov_rotation);
            update_animation(cur_node, cur_air_animData, cur_air_track->anim_time, &air_translation, &air_rotation);
            update_animation(cur_node, pre_air_animData, pre_air_track->anim_time, &preair_translation, &preair_rotation);
            update_animation(cur_node, cur_act_animData, cur_act_track->anim_time, &act_translation, &act_rotation);
            // update_animation(cur_node, cur_spare_animData, cur_spare_track->anim_time, &spare_translation, &spare_rotation);

            float actblendIn = cur_act_track->blendIn;
            float actblendOut = cur_act_track->blendOut;
            float airToMoveWeight = 1.0f;
            float moveToAirWeight = stateComp.smAir.blend;
            float moveAirToActWeight = stateComp.smAct.blend;

            //determine per bone weight for the final movement animation
            //how do we set this up with the global blend value? we want the walk animation to have maximum weight on the lower legs
            //just start with that
            switch(anim_node->bone_type){
                case humanoid_bone_head                 :{int debug = 0;}break;
                case humanoid_bone_torso                :{int debug = 0;}break;
                case humanoid_bone_right_upper_arm      :{int debug = 0;}break;
                case humanoid_bone_right_lower_arm      :{int debug = 0;}break;
                case humanoid_bone_left_upper_arm       :{int debug = 0;}break;
                case humanoid_bone_left_lower_arm       :{int debug = 0;}break;
                case humanoid_bone_right_upper_leg      :{
                    if(cur_move_track->anim_type == anim_walk){
                        moveAirToActWeight = 0.1f;
                        if(stateComp.smAir.blend > 0.5f){
                            moveToAirWeight = 0.5f;
                        }
                    }
                    if(cur_air_track->anim_type != anim_idle)moveAirToActWeight = 0.1f;
                }break;
                case humanoid_bone_right_lower_leg      :{
                    if(cur_move_track->anim_type == anim_walk){
                        moveAirToActWeight = 0.1f;
                        if(stateComp.smAir.blend > 0.5f){
                            moveToAirWeight = 0.5f;
                        }
                    }
                    if(cur_air_track->anim_type != anim_idle)moveAirToActWeight = 0.1f;
                }break;
                case humanoid_bone_left_upper_leg       :{
                    if(cur_move_track->anim_type == anim_walk){
                        moveAirToActWeight = 0.1f;
                        if(stateComp.smAir.blend > 0.5f){
                            moveToAirWeight = 0.5f;
                        }
                    }
                    if(cur_air_track->anim_type != anim_idle)moveAirToActWeight = 0.1f;
                }break;
                case humanoid_bone_left_lower_leg       :{
                    if(cur_move_track->anim_type == anim_walk){
                        moveAirToActWeight = 0.1f;
                        if(stateComp.smAir.blend > 0.5f){
                            moveToAirWeight = 0.5f;
                        }
                    }
                    if(cur_air_track->anim_type != anim_idle)moveAirToActWeight = 0.1f;
                }break;
            }


            mov_translation   = mix(premov_translation, mov_translation, stateComp.smMove.blend);
            mov_rotation      = nlerp(premov_rotation, mov_rotation, stateComp.smMove.blend);

            air_translation   = mix(mov_translation, air_translation, moveToAirWeight);
            air_rotation      = nlerp(mov_rotation, air_rotation, moveToAirWeight);

            total_translation   = mix(air_translation, act_translation, moveAirToActWeight);
            total_rotation      = nlerp(air_rotation, act_rotation, moveAirToActWeight);

            // vec3 vecs[2] = {combat_translation, mov_translation};
            // float vecWeights[2] = {0.5f, 0.5f};

            // total_translation = vec3_weighted_sum(vecs, vecWeights, 2);
            // quat qs[2] = {combat_rotation, mov_rotation};
            // float weights[2] = {1.0f, 1.0f};
            // total_rotation = nlerp_weighted(qs, weights, 2); 
            // total_translation = mov_translation;
            // total_rotation = mov_rotation;

        }
        cur_node->translation = total_translation;
        cur_node->rotation = total_rotation;

        vec3 relative_pivot_translation = /* test_scale * */ cur_node->translation;
        vec3 relative_total_translation = (relative_pivot_translation)+relative_cumulative_translation;


        vec3 vec3RelPivotTrans = vec3_create(relative_pivot_translation.x, relative_pivot_translation.y, relative_pivot_translation.z);
        vec3 vec3RelTotalTrans = vec3_create(relative_total_translation.x, relative_total_translation.y, relative_total_translation.z);
        vec3 mountPoint = vec3_create(mounting_point.x, mounting_point.y, mounting_point.z);
        quat relCumRot = quat_create(relative_cumulative_rotation.x, relative_cumulative_rotation.y, relative_cumulative_rotation.z, relative_cumulative_rotation.w);


        mat4 mat4_RelTotalTrans = mat4_translate(vec3RelTotalTrans);
        mat4 mat4_negRelPivotTrans = mat4_translate(vec3_negate(vec3RelPivotTrans));
        mat4 mat4_mountPoint = mat4_translate(vec3RelPivotTrans - mountPoint);
        mat4 mat4_relCumRot = quat_to_mat4(relCumRot);



        mat4 relativeModel = mat4_identity();
        relativeModel = mat4_mul(relativeModel, mat4_RelTotalTrans);
        relativeModel = mat4_mul(relativeModel, mat4_negRelPivotTrans);
        // mat4_relativeModel = mat4_mul(mat4_relCumRot, mat4_relativeModel);
        relativeModel = mat4_mul(relativeModel, mat4_relCumRot);
        relativeModel = mat4_mul(relativeModel, mat4_mountPoint);


        vec3 relative_translation = vec3_create(relativeModel.m[12], relativeModel.m[13], relativeModel.m[14]);
        //COMPUTE AND STORE THE RELATIVE TRANSLATION FIRST!
        mat4 scaleMatrix = mat4_identity();
        scale_mat4(scaleMatrix, given_scale); // cgltfScale might be vec3(2.0f) for a 2x scale

        mat4 scaledrelativeModel = mat4_mul(scaleMatrix, relativeModel);

        mat4 worldModel = mat4_identity();
        worldModel = mat4_translate(pos);
        mat4 worldRot = quat_to_mat4(base_rotation);

        worldModel = mat4_mul(worldModel, worldRot);
        worldModel = mat4_mul(worldModel, scaledrelativeModel);


        draw_cube(GameState, cameraComp, chunk_coords, vec3_create(worldModel.m[12], worldModel.m[13], worldModel.m[14]), 0.05f, { 1,1,1,1 });



        vec3 weapon_pos_min = vec3_create(0);
        vec3 weapon_pos_max = vec3_create(0);

        // if(draw_weapon_minmax){
        if (cur_node->has_mesh && cur_node->draw) {
            //for weapon min/max
            vec4 localMin = vec4_create(cur_node->min, 1.0f);
            vec4 localMax = vec4_create(cur_node->max, 1.0f);

            // Transform the bounding box corners
            vec4 relativeMin4 = mat4_vec4_product(scaledrelativeModel, localMin);
            vec4 relativeMax4 = mat4_vec4_product(scaledrelativeModel, localMax);

            // Convert back to vec3 (ignoring the homogeneous coordinate)
            vec3 relativeMin = vec3_create(relativeMin4);
            vec3 relativeMax = vec3_create(relativeMax4);
            //relative min and max are the final preprocessed transforms of the min and max for that mesh

#if 1
            draw_cube(GameState, cameraComp, chunk_coords, quat_rotate_vec3(base_rotation, relativeMin) + pos, 0.06f, { 1,1,1,1 });
            draw_cube(GameState, cameraComp, chunk_coords, quat_rotate_vec3(base_rotation, relativeMax) + pos, 0.06f, { 1,1,1,1 });
#endif 


        }



        //apply the cumulative translations/rotations to the children before pushing them on the stack
        for (int i = 0; i < cur_node->child_count; i++)
        {
            // vec3 rotated_local_translation = cumulative_rotation * cur_node->translation;

            relative_translations[node_stack_count] = relative_translation;
            relative_rotations[node_stack_count] = quat_normalize(quat_mul(relative_cumulative_rotation, cur_node->rotation));

            ASSERT_NO_NAN_POS(relative_translations[node_stack_count], "Nan detected in position: ");
            ASSERT_NO_NAN_ROT(relative_rotations[node_stack_count], "Nan detected in rotation: ");

            quat node_rot = cur_node->rotation;
            quat cum_rot = relative_cumulative_rotation;
            float cum_float = (cum_rot.x * cum_rot.x) + (cum_rot.y * cum_rot.y) + (cum_rot.z * cum_rot.z) + (cum_rot.w * cum_rot.w);
            float node_float = (node_rot.x * node_rot.x) + (node_rot.y * node_rot.y) + (node_rot.z * node_rot.z) + (node_rot.w * node_rot.w);
            if (fabs(1.0f - cum_float) > 0.01f) {
                printf("CUMULATIVE ROTATION INVALID! %3.3f %3.3f %3.3f %3.3f SQRD == %3.3f \n", cum_rot.x, cum_rot.y, cum_rot.z, cum_rot.w, cum_float);
            }
            if (fabs(1.0f - node_float) > 0.01f) {
                printf("NODE       ROTATION INVALID! %3.3f %3.3f %3.3f %3.3f SQRD == %3.3f \n", node_rot.x, node_rot.y, node_rot.z, node_rot.w, node_float);

            }
            // rotations[node_stack_count] = normalize(cumulative_translation * cur_node->rotation);
            node_stack[node_stack_count++] = cur_node->children[i];
        }
        if (cur_node->has_mesh && cur_node->draw) {
            float scale = 1.0f;
            // vec3 relativePosition = chunkPos - cameraPosition;
            vec3 total_translation = cur_node->translation + relative_cumulative_translation + pos;
            vec3 pivot_translation = cur_node->translation;
            // printf("drawing node %s at position %10.5f %10.5f %10.5f  rotation: %10.5f %10.5f %10.5f %10.5f \n", cur_node->name, total_translation.x,total_translation.y,total_translation.z, cumulative_rotation.x,cumulative_rotation.y,cumulative_rotation.z,cumulative_rotation.w);



            // cur_node->relative_translation = vec3(relativeModel[3][0], relativeModel[3][1], relativeModel[3][2]) - pos;
            cur_node->relative_translation = relative_translation;

            cur_node->relative_translation_min = weapon_pos_min;
            cur_node->relative_translation_max = weapon_pos_max;
            cur_node->relative_rotation = relative_cumulative_rotation;
            // draw_cube_white(GameState, cameraComp, chunk_coords, total_translation, 0.055f);
            // draw_cube_white(GameState, cameraComp, chunk_coords, cur_node->translation + pos, 0.055f);


            // vec3 cgltfScale = test_scale;//vec3(scale);


            vec3 cameraRelativePosition = CameraRelativePosition(cameraComp, chunk_coords, vec3_create(worldModel.m[12], worldModel.m[13], worldModel.m[14]));

            mat4 cgltfModel = mat4_translate(cameraRelativePosition);
            cgltfModel.m[0] = worldModel.m[0];
            cgltfModel.m[1] = worldModel.m[1];
            cgltfModel.m[2] = worldModel.m[2];
            cgltfModel.m[3] = worldModel.m[3];
            cgltfModel.m[4] = worldModel.m[4];
            cgltfModel.m[5] = worldModel.m[5];
            cgltfModel.m[6] = worldModel.m[6];
            cgltfModel.m[7] = worldModel.m[7];
            cgltfModel.m[8] = worldModel.m[8];
            cgltfModel.m[9] = worldModel.m[9];
            cgltfModel.m[10] = worldModel.m[10];
            cgltfModel.m[11] = worldModel.m[11];

            skeletal_mesh_command command = {};
            command.indexOffset = cur_node->index_offset;
            command.indexCount = cur_node->index_count;
            command.model = cgltfModel;
            command.color = { 1,1,1,1 };

            GameState->RenderCommandData->skeletalMeshCommands[(GameState->RenderCommandData->skelMeshDrawCount)++] = command;


        }
        else {//must be the joint/bone node
            draw_cube(GameState, cameraComp, chunk_coords, vec3_create(worldModel.m[12], worldModel.m[13], worldModel.m[14]), 0.055f, { 1,0,0,1 });
        }

    }
    if (GameState->gltfData->pause_animation)cgltfData->step_animation = false;
    else cgltfData->step_animation = true;

}
#if 0
void render_animated_model(game_state* GameState, CameraComp& cameraComp, StateComp& stateComp, model_type type = model_none, ivec3 chunk_coords = ivec3_create(0), vec3 pos = vec3_create(0), quat base_rotation = quat_identity(),
    float given_scale = 1.0f, vec3 mounting_point = vec3_create(0), bool draw_weapon_minmax = false, quat base_weapon_rotation = quat_identity(), vec3 base_weapon_translation = vec3_create(0)) {
    TIMED_BLOCK("render_animated_model");
    // printf("render_animated_model() start, anim_time: %10.5f\n", stateComp.anim_time);
    gltf_data* cgltfData = GameState->gltfData;
    uint32_t root_node = cgltfData->models[type].root_node;

    vec3 test_scale = vec3_create(1);//toggle it and see what happens

    int node_stack[32] = { 0 };
    vec3 relative_translations[32];
    quat relative_rotations[32];
    memset(relative_translations, 0, sizeof(vec3) * 32);
    int node_stack_count = 0;
    relative_translations[node_stack_count] = base_weapon_translation;
    relative_rotations[node_stack_count] = base_weapon_rotation;//cgltfData->nodes[cgltfData->root_nodes[0]].rotation;


    node_stack[node_stack_count++] = root_node;
    while (node_stack_count > 0) {
        int curID = node_stack[node_stack_count - 1];

        //relative values are for preprocessing the weapon's min/max hitboxes during debugging
        //eventually we will need to print out the values and store them in a look up table once we have more weapons to test with

        vec3 relative_cumulative_translation = relative_translations[node_stack_count - 1];
        quat relative_cumulative_rotation = relative_rotations[node_stack_count - 1];
        node_stack_count--;

        entity_node* cur_node = &cgltfData->nodes[curID];
        // printf("cur node: %s, ID: %d, node stack count: %d, has animation : %d\n", cur_node->name, curID, node_stack_count, cur_node->has_animation);
        // printf("cumulative translation: %10.5f %10.5f %10.5f, cumulative rotation   : %10.5f %10.5f %10.5f %10.5f \n",       cumulative_translation.x, cumulative_translation.y, cumulative_translation.z, cumulative_rotation.x, cumulative_rotation.y, cumulative_rotation.z, cumulative_rotation.w);

        ASSERT_NO_NAN_POS(relative_cumulative_translation, "Nan detected in position: ");
        ASSERT_NO_NAN_ROT(relative_cumulative_rotation, "Nan detected in rotation: ");
        //animation
        if (cur_node->has_animation && cgltfData->step_animation) {
            //apply animation info
            assert(cur_node->animated_nodeID > -1 && cur_node->animated_nodeID < MAX_ENTITY_NODES);
            animated_node* anim_node = &cgltfData->animated_nodes[cur_node->animated_nodeID];

            animation_data* anim_data = &anim_node->animations[stateComp.anim_type];

            if (anim_data->valid_animation) {
                int closest_frame = 0;
                int next_frame = 0;

                float t = 0.0f;
                float animation_time = stateComp.anim_time;

                int segment = 0;
                float segment_length = 0.0f;

                // Interpolate translation
                if (anim_data->has_translation) {

                    //find which segment we're in
                    while (segment < anim_data->num_position_keyframes - 1 && animation_time > anim_data->position_keyframe_times[segment + 1])segment++;
                    //loop case
                    if (segment == anim_data->num_position_keyframes - 1) {
                        segment = anim_data->num_position_keyframes - 1;
                        closest_frame = segment;
                        next_frame = 0; //loop back to first frame
                        // segment_length = (anim_data->position_keyframe_times[0] + 1.0f) - anim_data->position_keyframe_times[closest_frame];
                        // t = (animation_time - anim_data->position_keyframe_times[closest_frame]) / segment_length;
                        t = 1.0f;
                        ASSERT_NO_INF_T(t, "loop case t infinite");
                        if (t > 1.0f)t -= 1.0f; //if t > 1 we've wrapped around
                        ASSERT_NO_INF_T(t, "loop case t infinite");

                    }
                    else {//normal case
                        closest_frame = segment;
                        next_frame = segment + 1;
                        t = (animation_time - anim_data->position_keyframe_times[closest_frame]) / (anim_data->position_keyframe_times[next_frame] - anim_data->position_keyframe_times[closest_frame]);
                        ASSERT_NO_INF_T(t, "normal case t infinite");
                    }
                    // printf("translation animation: %s, time: %10.5f, closest frame: %d, next frame: %d, num_keyframes: %d\n", get_anim_type_name(stateComp.anim_type), animation_time, closest_frame, next_frame, anim_data->num_rotation_keyframes);
                    vec3 next = vec3_create(anim_data->keyframe_positions[next_frame].x, anim_data->keyframe_positions[next_frame].y, anim_data->keyframe_positions[next_frame].z);
                    vec3 closest = vec3_create(anim_data->keyframe_positions[closest_frame].x, anim_data->keyframe_positions[closest_frame].y, anim_data->keyframe_positions[closest_frame].z);
                    vec3 interpolation = mix(closest, next, t);
                    cur_node->translation = vec3_create(interpolation.x, interpolation.y, interpolation.z);

                    ASSERT_NO_NAN_POS(cur_node->translation, "Nan detected in position: ");


                    if (stateComp.anim_time > anim_data->position_keyframe_times[anim_data->num_position_keyframes - 1]) {
                        stateComp.anim_time = anim_data->position_keyframe_times[0];
                    }
                }
                else cur_node->translation = cur_node->base_translation; //set base position if this node has no animation data for this animation type
                segment = 0;
                // Interpolate rotation (using quaternion slerp for rotation)
                if (anim_data->has_rotation) {

                    //find which segment we're in
                    while (segment < anim_data->num_rotation_keyframes - 1 && animation_time > anim_data->rotation_keyframe_times[segment + 1])segment++;
                    //loop case
                    if (segment == anim_data->num_rotation_keyframes - 1) {
                        segment = anim_data->num_rotation_keyframes - 1;
                        closest_frame = segment;
                        next_frame = 0; //loop back to first frame
                        // segment_length = (anim_data->rotation_keyframe_times[0] + 1.0f) - anim_data->rotation_keyframe_times[closest_frame];
                        // t = (animation_time - anim_data->rotation_keyframe_times[closest_frame]) / segment_length;
                        t = 1.0f;

                        if (t > 1.0f)t -= 1.0f; //if t > 1 we've wrapped around
                        ASSERT_NO_INF_T(t, "loop case t infinite");

                    }
                    else {//normal case
                        closest_frame = segment;
                        next_frame = segment + 1;
                        t = (animation_time - anim_data->rotation_keyframe_times[closest_frame]) / (anim_data->rotation_keyframe_times[next_frame] - anim_data->rotation_keyframe_times[closest_frame]);
                        ASSERT_NO_INF_T(t, "normal case t infinite");

                    }
                    // printf("rotation animation: %s, time: %10.5f, closest frame: %d, next frame: %d, num_keyframes: %d\n", get_anim_type_name(stateComp.anim_type), animation_time, closest_frame, next_frame, anim_data->num_rotation_keyframes);

                    quat closest = quat_create(anim_data->keyframe_rotations[closest_frame].x, anim_data->keyframe_rotations[closest_frame].y, anim_data->keyframe_rotations[closest_frame].z, anim_data->keyframe_rotations[closest_frame].w);
                    quat next = quat_create(anim_data->keyframe_rotations[next_frame].x, anim_data->keyframe_rotations[next_frame].y, anim_data->keyframe_rotations[next_frame].z, anim_data->keyframe_rotations[next_frame].w);
                    quat interpolation = nlerp(closest, next, t);

                    cur_node->rotation = quat_create(interpolation.x, interpolation.y, interpolation.z, interpolation.w);

                    ASSERT_NO_NAN_POS(cur_node->rotation, "Nan detected in position: ");

                    // printf("current node rotation       : %3.3f %3.3f %3.3f %3.3f t: %3.3f\n", cur_node->rotation.x, cur_node->rotation.y, cur_node->rotation.z, cur_node->rotation.w, t);
                    // printf("closest frame %d rotation   : %3.3f %3.3f %3.3f %3.3f \n", closest_frame, anim_data->keyframe_rotations[closest_frame].x, anim_data->keyframe_rotations[closest_frame].y, anim_data->keyframe_rotations[closest_frame].z, anim_data->keyframe_rotations[closest_frame].w);
                    // printf("next frame    %d rotation   : %3.3f %3.3f %3.3f %3.3f \n",next_frame, anim_data->keyframe_rotations[next_frame].x,    anim_data->keyframe_rotations[next_frame].y, anim_data->keyframe_rotations[next_frame].z, anim_data->keyframe_rotations[next_frame].w);


                    if (stateComp.anim_time > anim_data->rotation_keyframe_times[anim_data->num_rotation_keyframes - 1]) {
                        stateComp.anim_time = anim_data->rotation_keyframe_times[0];
                    }
                }
                else cur_node->rotation = cur_node->base_rotation; //set base rotation if this node has no animation data for this animation type



            }
            else {
                cur_node->translation = cur_node->base_translation;
                cur_node->rotation = cur_node->base_rotation;
            }

        }

        vec3 relative_pivot_translation = /* test_scale * */ cur_node->translation;
        vec3 relative_total_translation = (relative_pivot_translation)+relative_cumulative_translation;


        vec3 vec3RelPivotTrans = vec3_create(relative_pivot_translation.x, relative_pivot_translation.y, relative_pivot_translation.z);
        vec3 vec3RelTotalTrans = vec3_create(relative_total_translation.x, relative_total_translation.y, relative_total_translation.z);
        vec3 mountPoint = vec3_create(mounting_point.x, mounting_point.y, mounting_point.z);
        quat relCumRot = quat_create(relative_cumulative_rotation.x, relative_cumulative_rotation.y, relative_cumulative_rotation.z, relative_cumulative_rotation.w);


        mat4 mat4_RelTotalTrans = mat4_translate(vec3RelTotalTrans);
        mat4 mat4_negRelPivotTrans = mat4_translate(vec3_negate(vec3RelPivotTrans));
        mat4 mat4_mountPoint = mat4_translate(vec3RelPivotTrans - mountPoint);
        mat4 mat4_relCumRot = quat_to_mat4(relCumRot);



        mat4 relativeModel = mat4_identity();
        relativeModel = mat4_mul(relativeModel, mat4_RelTotalTrans);
        relativeModel = mat4_mul(relativeModel, mat4_negRelPivotTrans);
        // mat4_relativeModel = mat4_mul(mat4_relCumRot, mat4_relativeModel);
        relativeModel = mat4_mul(relativeModel, mat4_relCumRot);
        relativeModel = mat4_mul(relativeModel, mat4_mountPoint);


        vec3 relative_translation = vec3_create(relativeModel.m[12], relativeModel.m[13], relativeModel.m[14]);
        //COMPUTE AND STORE THE RELATIVE TRANSLATION FIRST!
        mat4 scaleMatrix = mat4_identity();
        scale_mat4(scaleMatrix, given_scale); // cgltfScale might be vec3(2.0f) for a 2x scale

        mat4 scaledrelativeModel = mat4_mul(scaleMatrix, relativeModel);

        mat4 worldModel = mat4_identity();
        worldModel = mat4_translate(pos);
        mat4 worldRot = quat_to_mat4(base_rotation);

        worldModel = mat4_mul(worldModel, worldRot);
        worldModel = mat4_mul(worldModel, scaledrelativeModel);


        draw_cube(GameState, cameraComp, chunk_coords, vec3_create(worldModel.m[12], worldModel.m[13], worldModel.m[14]), 0.05f, { 1,1,1,1 });



        vec3 weapon_pos_min = vec3_create(0);
        vec3 weapon_pos_max = vec3_create(0);

        // if(draw_weapon_minmax){
        if (cur_node->has_mesh && cur_node->draw) {
            //for weapon min/max
            vec4 localMin = vec4_create(cur_node->min, 1.0f);
            vec4 localMax = vec4_create(cur_node->max, 1.0f);

            // Transform the bounding box corners
            vec4 relativeMin4 = mat4_vec4_product(scaledrelativeModel, localMin);
            vec4 relativeMax4 = mat4_vec4_product(scaledrelativeModel, localMax);

            // Convert back to vec3 (ignoring the homogeneous coordinate)
            vec3 relativeMin = vec3_create(relativeMin4);
            vec3 relativeMax = vec3_create(relativeMax4);
            //relative min and max are the final preprocessed transforms of the min and max for that mesh

#if 1
            draw_cube(GameState, cameraComp, chunk_coords, quat_rotate_vec3(base_rotation, relativeMin) + pos, 0.06f, { 1,1,1,1 });
            draw_cube(GameState, cameraComp, chunk_coords, quat_rotate_vec3(base_rotation, relativeMax) + pos, 0.06f, { 1,1,1,1 });
#endif 


        }



        //apply the cumulative translations/rotations to the children before pushing them on the stack
        for (int i = 0; i < cur_node->child_count; i++)
        {
            // vec3 rotated_local_translation = cumulative_rotation * cur_node->translation;

            relative_translations[node_stack_count] = relative_translation;
            relative_rotations[node_stack_count] = quat_normalize(quat_mul(relative_cumulative_rotation, cur_node->rotation));

            ASSERT_NO_NAN_POS(relative_translations[node_stack_count], "Nan detected in position: ");
            ASSERT_NO_NAN_ROT(relative_rotations[node_stack_count], "Nan detected in rotation: ");

            quat node_rot = cur_node->rotation;
            quat cum_rot = relative_cumulative_rotation;
            float cum_float = (cum_rot.x * cum_rot.x) + (cum_rot.y * cum_rot.y) + (cum_rot.z * cum_rot.z) + (cum_rot.w * cum_rot.w);
            float node_float = (node_rot.x * node_rot.x) + (node_rot.y * node_rot.y) + (node_rot.z * node_rot.z) + (node_rot.w * node_rot.w);
            if (fabs(1.0f - cum_float) > 0.01f) {
                printf("CUMULATIVE ROTATION INVALID! %3.3f %3.3f %3.3f %3.3f SQRD == %3.3f \n", cum_rot.x, cum_rot.y, cum_rot.z, cum_rot.w, cum_float);
            }
            if (fabs(1.0f - node_float) > 0.01f) {
                printf("NODE       ROTATION INVALID! %3.3f %3.3f %3.3f %3.3f SQRD == %3.3f \n", node_rot.x, node_rot.y, node_rot.z, node_rot.w, node_float);

            }
            // rotations[node_stack_count] = normalize(cumulative_translation * cur_node->rotation);
            node_stack[node_stack_count++] = cur_node->children[i];
        }
        if (cur_node->has_mesh && cur_node->draw) {
            float scale = 1.0f;
            // vec3 relativePosition = chunkPos - cameraPosition;
            vec3 total_translation = cur_node->translation + relative_cumulative_translation + pos;
            vec3 pivot_translation = cur_node->translation;
            // printf("drawing node %s at position %10.5f %10.5f %10.5f  rotation: %10.5f %10.5f %10.5f %10.5f \n", cur_node->name, total_translation.x,total_translation.y,total_translation.z, cumulative_rotation.x,cumulative_rotation.y,cumulative_rotation.z,cumulative_rotation.w);



            // cur_node->relative_translation = vec3(relativeModel[3][0], relativeModel[3][1], relativeModel[3][2]) - pos;
            cur_node->relative_translation = relative_translation;

            cur_node->relative_translation_min = weapon_pos_min;
            cur_node->relative_translation_max = weapon_pos_max;
            cur_node->relative_rotation = relative_cumulative_rotation;
            // draw_cube_white(GameState, cameraComp, chunk_coords, total_translation, 0.055f);
            // draw_cube_white(GameState, cameraComp, chunk_coords, cur_node->translation + pos, 0.055f);


            // vec3 cgltfScale = test_scale;//vec3(scale);


            vec3 cameraRelativePosition = CameraRelativePosition(cameraComp, chunk_coords, vec3_create(worldModel.m[12], worldModel.m[13], worldModel.m[14]));

            mat4 cgltfModel = mat4_translate(cameraRelativePosition);
            cgltfModel.m[0] = worldModel.m[0];
            cgltfModel.m[1] = worldModel.m[1];
            cgltfModel.m[2] = worldModel.m[2];
            cgltfModel.m[3] = worldModel.m[3];
            cgltfModel.m[4] = worldModel.m[4];
            cgltfModel.m[5] = worldModel.m[5];
            cgltfModel.m[6] = worldModel.m[6];
            cgltfModel.m[7] = worldModel.m[7];
            cgltfModel.m[8] = worldModel.m[8];
            cgltfModel.m[9] = worldModel.m[9];
            cgltfModel.m[10] = worldModel.m[10];
            cgltfModel.m[11] = worldModel.m[11];

            skeletal_mesh_command command = {};
            command.indexOffset = cur_node->index_offset;
            command.indexCount = cur_node->index_count;
            command.model = cgltfModel;
            command.color = { 1,1,1,1 };

            GameState->RenderCommandData->skeletalMeshCommands[(GameState->RenderCommandData->skelMeshDrawCount)++] = command;


        }
        else {//must be the joint/bone node
            draw_cube(GameState, cameraComp, chunk_coords, vec3_create(worldModel.m[12], worldModel.m[13], worldModel.m[14]), 0.055f, { 1,0,0,1 });
        }

    }
    if (GameState->gltfData->pause_animation)cgltfData->step_animation = false;
    else cgltfData->step_animation = true;

}
#endif
#endif 

uint16_t createItemEntry(game_state* GameState, inventory_item& item) {
    EntityComponent& ec = *GameState->entityComponent;

    uint16_t item_index = ec.inventory_item_count++;

    ec.inventory_items[item_index] = item;


    return item_index;
}


uint32_t createItemEntity(game_state* GameState, fpt_vec3 pos, char* name, uint16_t item_index) {
    EntityComponent& ec = *GameState->entityComponent;

    uint32_t newEntityID = createEntity(ec, name);
    printf("ITEM newEntityID: %d\n", newEntityID);
    MeshComp meshComp = {};
    meshComp.meshIndex = MeshTypes::mesh_cube;

    ModelComp modelComp = {};
    modelComp.type = ec.inventory_items[item_index].data.item_model;

    HealthComp healthComp = {};
    healthComp.health = 100;

    addMeshComp(ec, newEntityID, &meshComp); //we will init the cube mesh to index 1 in mesh array in renderSystem
    addModelComp(ec, newEntityID, &modelComp);
    addHealthComp(ec, newEntityID, &healthComp);

    TransComp transComp = {};
    transComp.rotation = { 0, 0, 0, FPT_ONE };
    transComp.pos_in_chunk = pos;
    transComp.scale = fpt_vec3_create(FPT_ONE);
    transComp.speed = FPT_QUARTER;
    transComp.forward = fpt_vec3_create(0, 0, FPT_ONE);
    transComp.up = fpt_vec3_create(0, FPT_ONE, 0);
    transComp.right = fpt_vec3_create(FPT_ONE, 0, 0);

    StateComp stateComp = {};
    // stateComp.action_index = 0;//base offset into the array for entity state actions
    stateComp.body_type = entityBody_item;//base offset into the array for entity state actions

    DataComp dataComp = {};
    dataComp.type = entity_types::entity_item;
    dataComp.item_index = item_index;

    addTransComp(ec, newEntityID, &transComp);
    addAabbComp(ec, newEntityID);
    addObbComp(ec, newEntityID);
    addPhysicsComp(ec, newEntityID);
    addStateComp(ec, newEntityID, &stateComp);
    addInventoryComp(ec, newEntityID);
    addBodyComp(ec, newEntityID);
    addDataComp(ec, newEntityID, &dataComp);

    return newEntityID;
}

void drop_item(game_state* GameState, fpt_vec3 pos, char* name, uint16_t item_index) {
    createItemEntity(GameState, pos, name, item_index);
}


uint32_t createPlayerEntity(game_state* GameState, fpt_vec3 pos, char* name) {
    EntityComponent& ec = *GameState->entityComponent;

    uint32_t newEntityID = createEntity(ec, name);
    printf("newPlayerEntityID: %d\n", newEntityID);
    MeshComp meshComp = {};
    meshComp.meshIndex = MeshTypes::mesh_Frustum;
    ModelComp modelComp = {};
    modelComp.type = model_humanoid;

    HealthComp healthComp = {};
    healthComp.health = 100;

    addMeshComp(ec, newEntityID, &meshComp); //we will init the cube mesh to index 1 in mesh array in renderSystem
    addModelComp(ec, newEntityID, &modelComp);
    addHealthComp(ec, newEntityID, &healthComp);

    TransComp transComp = {};
    transComp.rotation = fpt_quat_create(0, 0, 0, FPT_ONE);
    transComp.pos_in_chunk = pos;
    transComp.scale = fpt_vec3_create(FPT_ONE);
    transComp.speed = FPT_QUARTER;
    transComp.forward = fpt_vec3_create(0, 0, FPT_ONE);
    transComp.up = fpt_vec3_create(0, FPT_ONE, 0);
    transComp.right = fpt_vec3_create(FPT_ONE, 0, 0);

    StateComp stateComp = {};
    // stateComp.action_index = 0;//base offset into the array for entity state actions
    stateComp.body_type = entityBody_humanoid;//base offset into the array for entity state actions
    stateComp.smMove.blend = 1.0f;

    DataComp dataComp = {};
    dataComp.type = entity_types::entity_actor;

    addTransComp(ec, newEntityID, &transComp);
    addAabbComp(ec, newEntityID);
    addObbComp(ec, newEntityID);
    addPhysicsComp(ec, newEntityID);
    addStateComp(ec, newEntityID, &stateComp);
    addInventoryComp(ec, newEntityID);
    addBodyComp(ec, newEntityID);
    addDataComp(ec, newEntityID, &dataComp);
    addPlayerComp(ec, newEntityID, 255);
    addCameraComp(GameState, ec, newEntityID, pos);


    //TEST TO SETUP EQUIPMENT ON THE ENTITY
    // inventory_item sword = {};
    // sword.data.item_model = model_sword;

    // inventory_item shield = {};
    // shield.data.item_model = model_shield;

    // int sword_socket = ec.inventory_item_count;
    // ec.inventory_items[ec.inventory_item_count++] = sword;
    // int shield_socket = ec.inventory_item_count;
    // ec.inventory_items[ec.inventory_item_count++] = shield;

    // ec.InventoryComps[ec.entityToEquipmentMap[newEntityID]].right_fore_arm_socket = sword_socket; 
    // ec.InventoryComps[ec.entityToEquipmentMap[newEntityID]].left_fore_arm_socket = shield_socket; 


    // ec.body_parts[
    return newEntityID;
}


extern "C" GAME_INIT(GameInit) {
    printf("GAME INIT!\n");
    #if LABOR_INTERNAL
    GlobalDebugTable = Memory->GlobalDebugTablePtr;
    #endif
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    GameState->firstFrame = true;
    GameState->window_width = &Memory->width;
    GameState->window_height = &Memory->height;

    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
        (uint8_t*)Memory->PermanentStorage + sizeof(game_state));

    GameState->deltaTimeScale = 1.0f;
    GameState->threadCount = Memory->threadCount;
    GameState->PlatformMemorySize = Memory->PlatformMemorySize;

    GameState->fixedTimeStep = 1.0f / 60.0f;
    GameState->fixed_tick_rate = 60;
    GameState->fptFixedTimeStep = 1092; // 1092 = 0.016
    GameState->tick = 0;


    GameState->Queue = Memory->HighPriorityQueue;
    GameState->PlatformAddEntry = Memory->PlatformAddEntry;
    GameState->PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
    GameState->PlatformCheckAllWorkCompleted = Memory->PlatformCheckAllWorkCompleted;
    GameState->platform_fpt_asin = Memory->platform_fpt_asin;

    GameState->entityComponent = PushStruct(&GameState->WorldArena, EntityComponent);
    ecs_init(GameState, *GameState->entityComponent);

    fpt_vec3 cameraPos = fpt_vec3_create(i2fpt(0), i2fpt(0), i2fpt(10));
    fpt_vec3 playerPos = fpt_vec3_create(i2fpt(0), i2fpt(0), i2fpt(5));
    char playerName[16] = "Player0";
    GameState->localPlayerEntityIDs[0] = createPlayerEntity(GameState, playerPos, playerName);
    GameState->local_player_count++;

    //second entity test
    cameraPos = fpt_vec3_create(i2fpt(2), i2fpt(-6), i2fpt(5));
    char player1Name[16] = "Player1";
    // GameState->localPlayerEntityIDs[1] = createPlayerEntity(GameState, cameraPos, player1Name);
    GameState->local_player_count++;

    fpt_vec3 item_pos = fpt_vec3_create(i2fpt(-2), i2fpt(-6), i2fpt(5));
    char itemName[16] = "Sword0";


    inventory_item basic_sword = {};
    basic_sword.data.item_model = model_sword;
    basic_sword.data.type = item_types::item_type_tool;
    basic_sword.data.tool.type = tool_types::tool_type_sword;

    uint16_t sword_item_index = createItemEntry(GameState, basic_sword);
    // createItemEntity(GameState, item_pos, itemName, sword_item_index);

    item_pos = fpt_vec3_create(i2fpt(-4), i2fpt(-6), i2fpt(5));
    char itemName2[16] = "Shield0";


    inventory_item basic_shield = {};
    basic_shield.data.item_model = model_shield;
    basic_shield.data.type = item_types::item_type_tool;
    basic_shield.data.tool.type = tool_types::tool_type_shield;

    uint16_t shield_item_index = createItemEntry(GameState, basic_shield);
    // createItemEntity(GameState, item_pos, itemName2, shield_item_index);

    GameState->RenderCommandData = Memory->RenderCommandData;


    //for old texture testing
    // for(int y = 0; y < 128;y++){
    //     for(int x = 0; x < 128;x++){
    //         GameState->textureTestMem[y * 512 + x] = GameState->thorns[y * 128 + x];
        
    //     }
    // }


        

    GameState->doubleDescent = Memory->doubleDescent;
    GameState->lineAdvance = Memory->lineAdvance;
    GameState->maxCharWidth = Memory->maxCharWidth;

    GameState->gltfData = Memory->gltfData;


    GameState->uiData = PushStruct(&GameState->WorldArena, ui_data);

    //TODO: add cycle counters here and in the old engine around chunk data initialization to find why its significantly slower
    GameState->chunkData = PushStruct(&GameState->WorldArena, chunk_data);
    GameState->chunkData->threadCount = GameState->threadCount;
    GameState->chunkData->chunkDrawDistance = GameState->RenderCommandData->chunk_draw_distance;
    GameState->chunkData->Queue = GameState->Queue;
    GameState->chunkData->PlatformAddEntry = GameState->PlatformAddEntry;
    GameState->chunkData->PlatformCompleteAllWork = GameState->PlatformCompleteAllWork;
    GameState->chunkData->PlatformCheckAllWorkCompleted = GameState->PlatformCheckAllWorkCompleted;


    GameState->draw_aabb = true;
    GameState->draw_obb = true;
    GameState->draw_obbaabb = true;
    GameState->draw_bvh = true;

    chunk_start(GameState);
    ui_start(GameState);
    bb_start(GameState);

    //the chat window is the default text input destination unless we click into another window
    GameState->text_dest = &GameState->uiData->windows[ui_window_types::window_type_book_text].data.book_text.text_input;
    GameState->chat_text_dest = &GameState->uiData->windows[ui_window_types::window_type_chat].data.chat.text_input;

    GameState->currentUIDepth = 0.1f;
    GameState->UIDepthIncrement = 0.1f / (float)(MAX_SSBO_ENTITIES);
    GameState->perlinXSlider = 0.0f;
    GameState->perlinYSlider = 0.0f;
    GameState->perlinZSlider = 0.0f;
    GameState->perlinSliderMinRange = -100.0f;
    GameState->perlinSliderMaxRange = 100.0f;
    GameState->perlinScale = 0.009953125f;
    GameState->coarsePerlinX = 8.0f;
    GameState->coarsePerlinY = 8.0f;
    GameState->coarsePerlinZ = 8.0f;
    GameState->tiledScale = 6.0f;
    GameState->perlinRedistribution = 1.0f;
    
    int seed = 12345;
    GameState->perlinRandState = 0;
    rng_seed(&GameState->perlinRandState, seed);
    GameState->perlinSeed = rng_next_u32(&GameState->perlinRandState);

    GameState->xDerivMin =  1000.0f;
    GameState->xDerivMax = -1000.0f;
    GameState->yDerivMin =  1000.0f;
    GameState->yDerivMax = -1000.0f;
    GameState->zDerivMin =  1000.0f;
    GameState->zDerivMax = -1000.0f;
    GameState->valueNoiseMin =  1000.0f;
    GameState->valueNoiseMax = -1000.0f;
    GameState->xDerivBlend = 0.0f;
    GameState->yDerivBlend = 0.0f;
    GameState->zDerivBlend = 0.0f;
    GameState->timeScale   = 0.0f;
    GameState->drawDerivArrows = false;
    GameState->scaleFactor = 1023.0f;

    for(u32 i = 0; i < 512; i++){
        float x = rng_next_f32(&GameState->perlinRandState) * 511.99f;
        float y = rng_next_f32(&GameState->perlinRandState) * 511.99f;
        float rad = rng_next_f32(&GameState->perlinRandState) * 3.0f;
        float r = rng_next_f32(&GameState->perlinRandState);
        float g = rng_next_f32(&GameState->perlinRandState);
        float b = rng_next_f32(&GameState->perlinRandState);
        float speed = rng_next_f32(&GameState->perlinRandState) + 1.0f;
        float grow = rng_next_f32(&GameState->perlinRandState);
        float devX = rng_next_f32(&GameState->perlinRandState) * 0.1f;
        float devY = rng_next_f32(&GameState->perlinRandState) * 0.1f;
        float ddevX = rng_next_f32(&GameState->perlinRandState) * 0.01f;
        float ddevY = rng_next_f32(&GameState->perlinRandState) * 0.01f;
        u32 shape = rng_next_u32(&GameState->perlinRandState) & 1;
        GameState->particleGrow[i] = grow > 0.5f ? 1 : 0;
        GameState->particleRad[i] = rad;
        GameState->particleSpeed[i] = speed;
        GameState->particleR[i] = (u8)(r * 255);
        GameState->particleG[i] = (u8)(g * 255);
        GameState->particleB[i] = (u8)(b * 255);
        GameState->particleX[i] = x;
        GameState->particleY[i] = y;
        GameState->particleDevX  [i] = devX ;
        GameState->particleDevY  [i] = devY ;
        GameState->particledDevX [i] = ddevX;
        GameState->particledDevY [i] = ddevY;
        GameState->particleShape [i] = shape;

    }
    populate2dPerlin(GameState);

    GameState->perlinDemoEnum = demo_coarse3d;
    GameState->chunkData->thorns = GameState->thorns;
    GameState->chunkData->cracked = GameState->cracked;

    GameState->testHalflife = 0.25f;
    GameState->testEaseTarget = 1.0f;


    //test for voxel coarse sampling

    #if 0
        printf("chunk 0 0 0 \n");
        ivec3 coords = {0,0,0}; 
        vec3 chunkPos = {};
        chunkPos.x = coords.x * CHUNK_SIZE;
        chunkPos.y = coords.y * CHUNK_SIZE;
        chunkPos.z = coords.z * CHUNK_SIZE;
        int xSample = 4;
        chunkPos = get_chunk_bottom_left_back_corner(chunkPos) - 1;//shift it back to account for padding
        int sampleCount = 0;
        int curX = -xSample;
        for (int x = -1; x < (62 / xSample) + 1; x++){
            float posx = (chunkPos.x + curX);
            printf("sample count: %d, posz : %f\n", sampleCount,  posx);
            curX += xSample; 
            sampleCount++;
        }
        
        printf("chunk -1 0 0 \n");
        coords = {-1,0,0}; 
        chunkPos.x = coords.x * CHUNK_SIZE;
        chunkPos.y = coords.y * CHUNK_SIZE;
        chunkPos.z = coords.z * CHUNK_SIZE;
        chunkPos = get_chunk_bottom_left_back_corner(chunkPos) - 1;//shift it back to account for padding

        sampleCount = 0;
        curX = -xSample;
        for (int x = -1; x < (62 / xSample) + 1; x++){
            float posx = (chunkPos.x + curX);
            printf("sample count: %d, posz : %f\n", sampleCount,  posx);
            curX += xSample;
            sampleCount++;
        }

        __debugbreak();
        #endif

}

#if LABOR_INTERNAL
game_memory* DebugGlobalMemory;
#endif

extern "C" GAME_FIXED_UPDATE(GameFixedUpdate) {
    printf("fixed update!\n");
}



extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    //set it every frame in case of hot reload
    #if LABOR_INTERNAL
        GlobalDebugTable = Memory->GlobalDebugTablePtr;
    #endif

    TIMED_BLOCK("GAME_UPDATE_AND_RENDER");

    // u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); 
    // u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF; 
    // Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0])); 
    // debug_event* Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; 
    // Event->clock = __rdtsc(); 
    // Event->Type = (u8)DebugType_BeginBlock; 
    // Event->CoreIndex = 0; 
    // Event->threadID = (u16)GetThreadID(); 
    // Event->GUID = "Game update and render"; 

    game_state* GameState = (game_state*)Memory->PermanentStorage;
    GameState->fixed_update = false;
    Memory->fixedUpdate = false;


#if LABOR_INTERNAL
    DebugGlobalMemory = Memory;
#endif



    //floating point errors accumulate rapidly here
    GameState->totalTime += Memory->deltaTime;

    GameState->deltaTime = Memory->deltaTime * GameState->deltaTimeScale;

    //clear draw commands
    GameState->RenderCommandData->entityDrawCount = 0;
    GameState->RenderCommandData->worldTextDrawCount = 0;
    GameState->RenderCommandData->screenElementDrawCount = 0;
    GameState->RenderCommandData->screenTextDrawCount = 0;
    GameState->RenderCommandData->screen_transient_char_count = 0;
    GameState->RenderCommandData->world_transient_char_count = 0;
    GameState->RenderCommandData->chunkDrawCommandCount = 0;
    GameState->RenderCommandData->skelMeshDrawCount = 0;
    GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount = 0;
    GameState->currentUIDepth = 0.1f;

    GameState->firstFrame = 0;

    
    GameState->valNoise3dCyclesTotal = 0;
    GameState->valNoise3dHits = 0;
    GameState->valNoise3dCycles = 0;

    GameState->chunkData->perlinNoise3dCyclesTotal = 0;
    GameState->chunkData->perlinNoise3dHits = 0;
    GameState->chunkData->perlinNoise3dCycles = 0;


    GameState->chunkData->perlinNoise3dScalarLookup = 0;
    GameState->chunkData->perlinNoise3dScalarLookupTotal = 0;
    GameState->chunkData->perlinNoise3dSIMDLookup = 0;
    GameState->chunkData->perlinNoise3dSIMDLookupTotal = 0;
    GameState->chunkData->perlinNoise3dScalar = 0;
    GameState->chunkData->perlinNoise3dScalarTotal = 0;
    GameState->chunkData->perlinNoise3dSIMD = 0;
    GameState->chunkData->perlinNoise3dSIMDTotal = 0;

    GameState->chunkData->grad4xTotal = 0;
    GameState->chunkData->grad4xHits = 0;
    GameState->chunkData->grad4x = 0;

    GameState->drawTextureTotal = 0;
    GameState->drawTextureHits = 0;
    GameState->drawTextureCycles = 0;

    if (Memory->executable_reloaded) {
        GameState->hotReloaded = true;
        printf("DLL HOT RELOADED!\n");
        clearUIHashTable(GameState->uiData);
        hardcode_entity_actions(GameState);
        hardcode_animation_weights(GameState);
    }


    if(GameState->perlinDemoEnum == demo_basic2d){
        draw2dNoise(GameState);

    }
    if(GameState->perlinDemoEnum >= demo_fbm && GameState->perlinDemoEnum < demo_flowArrows){
        draw2dNoiseOld(GameState);

    }


    mat4 ident = mat4_identity();


    u16 cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
    CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];




    if (Memory->window_resized) {
        Memory->window_resized = false;
        WindowResize(cameraComp);
        ui_resizeElements(GameState);
        GameState->windowResized = true;
    }




    float& accumulator = GameState->accumulator;

    accumulator += GameState->deltaTime;

    // printf("delta time: %f\n", GameState->deltaTime);
    while (accumulator >= GameState->fixedTimeStep) {
        GameState->fixed_update = true;
        Memory->fixedUpdate = true;
        GameState->currentTick = GameState->tick;
        GameState->uiData->curVersionID = GameState->currentTick;
        //can only update textures once per frame
        if(GameState->consecutiveFixedUpdates == 0 && (GameState->perlinDemoEnum == demo_flow)){
            perlinParticleFlow(GameState);

        }else if(GameState->consecutiveFixedUpdates == 0 && (GameState->perlinDemoEnum == demo_flowArrows)){
            perlinFlowArrows(GameState);
        }


        ui_flush_debug(GameState);


#if 1 //enable when we want text input
        // if(WAS_PRESSED((Memory->new_input), function.keyReturn, InputTypes::input_key_return)){
        //     GameState->textInputEnabled = !GameState->textInputEnabled;
        // }
        uint32_t new_index = GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1);
        GameState->playerInputs[0][new_index] = Memory->new_input;
        player_input& newInput = GameState->playerInputs[0][new_index];

        if (GameState->textInputEnabled) {
            process_text_input(Memory, GameState, Memory->new_input);

            //clearing these inputs works for now
            //clear problematic keypresses if we're processing text input
            //already processed all the important presses from these when we process text input
            newInput.numberKeys = 0;
            newInput.functionKeys = 0;
            newInput.buttons = 0;
            newInput.debugFlags = 0;

            // Clear transition counts with memset (much faster than loop)
            u32 start_index = InputTypes::input_key_up;
            u32 count_to_clear = InputTypes::input_count - start_index;
            memset(&newInput.transitionCounts[start_index], 0, count_to_clear * sizeof(u8));


        }
#endif

        if(GameState->consecutiveFixedUpdates)printf("Consecutive Fixed Updates: %u\n", GameState->consecutiveFixedUpdates);

        ui_process_input(GameState, newInput);

        entity_sim_update(GameState);

        //skeletal animation debug window
        char* animlabel = "SKELETAL ANIMATION";
        ui_begin_window(GameState, animlabel);


        //        
        EntityComponent& ec = *GameState->entityComponent;
        uint32_t entityID = GameState->localPlayerEntityIDs[0];
        uint8_t playerIndex = ec.entityToPlayerMap[entityID];
        uint16_t invIndex = ec.entityToInventoryMap[entityID];
        uint16_t transIndex = ec.entityToTransMap[entityID];
        uint32_t cameraIndex = ec.entityToCameraMap[entityID];
        uint32_t stateIndex = ec.entityToStateMap[entityID];
        uint32_t modelIndex = ec.entityToModelMap[entityID];
        uint32_t physicsIndex = ec.entityToPhysicsMap[entityID];
        InventoryComp& invComp = ec.InventoryComps[invIndex];
        CameraComp& cameraComp = ec.CameraComps[cameraIndex];
        PlayerComp& PlayerComp = ec.PlayerComps[playerIndex];
        StateComp& state = ec.StateComps[stateIndex];
        ModelComp& model = ec.ModelComps[modelIndex];
        PhysicsComp* phys = ec.PhysicsComps + physicsIndex;
        TransComp* trans = ec.TransComps + transIndex;

        entity_animation_track* cur_move_track          = &state.smMove.curTrack;// cur_move_track;     
        entity_animation_track* pre_move_track          = &state.smMove.preTrack;// prev_move_track;    

        entity_animation_track* cur_act_track           = &state.smAct.curTrack; //cur_interact_track;  
        entity_animation_track* pre_act_track           = &state.smAct.preTrack; //prev_interact_track; 

        entity_animation_track* cur_air_track           = &state.smAir.curTrack;// prev_move_track;    
        entity_animation_track* pre_air_track           = &state.smAir.preTrack; //prev_interact_track; 

        char* inlineLabel0 = "TimeInline";

        ui_element_data appendedData = {};
        bool stubvalue = false;
        vec4 color = {0,0,0,0};
        vec4 sliderColor = { 0,1,0,1 };
        float minOffset = 0.0f;
        float maxOffset = 4.0f;

        ui_hash_entry* inlineElementHash0 = ui_begin_inline(GameState, 50.0f, inlineLabel0);


        ui_append_inline_slider_float(GameState, "DeltaTimeScale", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, inlineElementHash0, appendedData, 0.1f, 2.0f, GameState->deltaTimeScale, 1.0f, true, &sliderColor);
    

        fpt_vec3 posInChunk = trans->pos_in_chunk;
        ui_text(GameState, "PosInChunk  : %f %f %f", fpt2fl(posInChunk.x), fpt2fl(posInChunk.y), fpt2fl(posInChunk.z));
        ui_text(GameState, "Velocity    : %f %f %f", fpt2fl(phys->velocity.x), fpt2fl(phys->velocity.y), fpt2fl(phys->velocity.z));
        ui_text(GameState, "Acceleration: %f %f %f", fpt2fl(phys->acceleration.x), fpt2fl(phys->acceleration.y), fpt2fl(phys->acceleration.z));
        ui_text(GameState, "Grounded    : %d", state.grounded);
        ui_text(GameState, "Jump        : %d", Memory->new_input.bits.up);
        ui_text(GameState, "MovementLock: %d", state.smMove.locked);
        ui_text(GameState, "InteractLock: %d", state.smAct.locked);
        
        //animation data
        ui_text(GameState, "%s", get_model_type_name(model.type));
        // ui_text(GameState, "%s", get_anim_type_name(state.anim_type));
        ui_text(GameState, "Landing Intensity  : %f", fpt2fl(state.landingIntensity));
        ui_text(GameState, "Cur  Move State    : %10s, moveBlend: %f", get_entity_state_name(state.smMove.cur), state.smMove.blend);
        ui_text(GameState, "Prev Move State    : %10s, moveBlend: %f", get_entity_state_name(state.smMove.prev), state.smMove.blend);
        ui_text(GameState, "Current Movement   : %10s, time: %f", get_anim_type_name(cur_move_track->anim_type), cur_move_track->anim_time);
        ui_text(GameState, "PreviousMovement   : %10s, time: %f", get_anim_type_name(pre_move_track->anim_type), pre_move_track->anim_time);
        ui_text(GameState, "Current Interact   : %10s, time: %f", get_anim_type_name(cur_act_track->anim_type), cur_act_track->anim_time);
        ui_text(GameState, "PreviousInteract   : %10s, time: %f", get_anim_type_name(pre_act_track->anim_type), pre_act_track->anim_time);
        ui_text(GameState, "Current Air        : %10s, time: %f", get_anim_type_name(cur_air_track->anim_type), cur_air_track->anim_time);
        ui_text(GameState, "PreviousAir        : %10s, time: %f", get_anim_type_name(pre_air_track->anim_type), pre_air_track->anim_time);
        // ui_text(GameState, "Current Spare      : %10s, time: %f", get_anim_type_name(cur_spare_track->anim_type), cur_spare_track->anim_time);
        // ui_text(GameState, "PreviousSpare      : %10s, time: %f", get_anim_type_name(prev_spare_track->anim_type), prev_spare_track->anim_time);

        //movement
        ui_hash_entry* moveElementHash = ui_begin_inline(GameState, 50.0f, "MoveAnimInline");
        float maxAnimTime = GameState->gltfData->models[model.type].animations[cur_move_track->anim_type].max_time;
        ui_append_inline(GameState, ui_element_type_checkbox, "Pause", GameState->pauseAnimTest , 0, 100, 0, 50, moveElementHash, appendedData);
        ui_append_inline_slider_float(GameState, "MoveTime", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, moveElementHash, appendedData, minOffset, maxAnimTime, cur_move_track->anim_time, 0.0, true, &sliderColor);
        ui_append_inline_slider_float(GameState, "MoveBlend", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, moveElementHash, appendedData, 0.0f, 1.0f, state.smMove.blend, 0.0, true, &sliderColor);

        //airborn
        ui_hash_entry* airElementHash = ui_begin_inline(GameState, 50.0f, "AirAnimInline");
        maxAnimTime = GameState->gltfData->models[model.type].animations[cur_air_track->anim_type].max_time;
        ui_append_inline(GameState, ui_element_type_checkbox, "Pause", GameState->pauseAnimTest , 0, 100, 0, 50, airElementHash, appendedData);
        ui_append_inline_slider_float(GameState, "AirTime", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, airElementHash, appendedData, minOffset, maxAnimTime, cur_air_track->anim_time, 0.0, true, &sliderColor);
        ui_append_inline_slider_float(GameState, "AirBlend", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, airElementHash, appendedData, 0.0f, 1.0f, state.smAir.blend, 0.0, true, &sliderColor);
        
        //act
        ui_hash_entry* actElementHash = ui_begin_inline(GameState, 50.0f, "ActAnimInline");
        maxAnimTime = GameState->gltfData->models[model.type].animations[cur_act_track->anim_type].max_time;
        ui_append_inline(GameState, ui_element_type_checkbox, "Pause", GameState->pauseAnimTest , 0, 100, 0, 50, actElementHash, appendedData);
        ui_append_inline_slider_float(GameState, "ActTime", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, actElementHash, appendedData, minOffset, maxAnimTime, cur_act_track->anim_time, 0.0, true, &sliderColor);
        ui_append_inline_slider_float(GameState, "ActBlend", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, actElementHash, appendedData, 0.0f, 1.0f, state.smAct.blend, 0.0, true, &sliderColor);
        

        //test
        ui_hash_entry* testElementHash = ui_begin_inline(GameState, 50.0f, "TestAnimInline");
        maxAnimTime = GameState->gltfData->models[model.type].animations[cur_act_track->anim_type].max_time;
        ui_append_inline(GameState, ui_element_type_checkbox, "Pause", GameState->pauseAnimTest , 0, 100, 0, 50, testElementHash, appendedData);
        ui_append_inline_slider_float(GameState, "HalfLife", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, testElementHash, appendedData, minOffset, 2.0f, GameState->testHalflife, 0.25f, true, &sliderColor);
        GameState->testEase = HalfLifeStep(GameState->testHalflife, GameState->testEase, GameState->testEaseTarget, GameState->deltaTime);
        ui_append_inline_slider_float(GameState, "TestBlend", ui_element_type_horizontal_slider_float, 0, 128 * 4, 0, 50, testElementHash, appendedData, minOffset, 1.0f, GameState->testEase, 0.0, true, &sliderColor);
        if(GameState->testEase >= 0.99f)GameState->testEaseTarget = 0.0f;
        else if(GameState->testEase <= 0.01f)GameState->testEaseTarget = 1.0f;
        
        
        #if 0

        // ui_append_inline(GameState, ui_element_type_texture, "texture", stubvalue, 0, 512, 0, 512,  inlineElementHash, appendedData, &color, 6);

        //create stack of all animation nodes to display
        gltf_data* cgltfData = GameState->gltfData;
        u32 rootNode = cgltfData->models[model.type].root_node;
        int nodeStackCount = 0;
        int nodeStack[32] = {};
        nodeStack[nodeStackCount++] = rootNode;
        while(nodeStackCount > 0){
            nodeStackCount--;
            int curID = nodeStack[nodeStackCount];
            entity_node* curNode = cgltfData->nodes + curID;
            int animNodeID = curNode->animated_nodeID;
            ui_text(GameState, "Depth: %d, %20s, Children: %d, Mesh: %d, Anim: %d, animNodeID: %2d", nodeStackCount, curNode->name, curNode->child_count, curNode->has_mesh, curNode->has_animation, animNodeID);
            if(curNode->has_animation){
                animated_node* animNode = cgltfData->animated_nodes + curNode->animated_nodeID;
                animation_data* cur_move_animData           = &animNode->animations[cur_move_track->anim_type];
                animation_data* cur_interact_animData       = &animNode->animations[cur_interact_track->anim_type];
                animation_data* cur_spare_animData          = &animNode->animations[cur_spare_track->anim_type];
                animation_data* next_move_animData          = &animNode->animations[next_move_track->anim_type];
                animation_data* next_interact_animData      = &animNode->animations[next_interact_track->anim_type];
                animation_data* next_spare_animData         = &animNode->animations[next_spare_track->anim_type];
                // animation_data* animData = animNode->animations + state.anim_type;
                ui_text(GameState, "AnimNode");

            }
            
            for(int i = 0; i < curNode->child_count; i++){
                nodeStack[nodeStackCount++] = curNode->children[i];
            }
        }
        #endif
        
        ui_end_window(GameState);


        bb_update(GameState);

        chunk_processInput(GameState, newInput, GameState->localPlayerEntityIDs[0]);
        chunk_update(GameState);


        ui_test(GameState);

        
        accumulator -= GameState->fixedTimeStep;
        //crude workaround to stop simulation catch up after hitting a breakpoint
        accumulator = 0.0f;


        //     // Prevent tiny floating-point accumulation
        if (accumulator < GameState->fixedTimeStep * 0.001f) {
            accumulator = 0.0f;
        }

        GameState->tick++;
        GameState->ticks_this_second++;

        //clear game memory inputs for next round of processing

        for (u32 i = 0; i < InputTypes::input_count; i++) {
            Memory->new_input.transitionCounts[i] = 0;
        }
        Memory->new_input.mouse_dx = 0;
        Memory->new_input.mouse_dy = 0;
        Memory->new_input.mouse_wheel = 0;
        Memory->new_input.consumedMouseFunctions = 0;
        Memory->textInputLen = 0; //clear text input regardless if we're accepting text input or not, it gets processed before this if we do
        GameState->consecutiveFixedUpdates++;
        GameState->hotReloaded = false;

    }


    // printf("chunk screen space: %f %f\n", chunk_screenspace.x, chunk_screenspace.y);


    Memory->model = ident;

    Memory->axis_model = ident;

    // mat4_rotate(&Memory->model, &ident, GameState->totalTime * radians(90.0f), vec3_create(0.0f, 0.0f, 1.0f));

    Memory->camPos.x = (fpt2fl(cameraComp.pos_in_chunk.x));
    Memory->camPos.y = (fpt2fl(cameraComp.pos_in_chunk.y));
    Memory->camPos.z = (fpt2fl(cameraComp.pos_in_chunk.z));









    size_t& entityDrawCount = GameState->RenderCommandData->entityDrawCount;
    u32& worldCharCount = GameState->RenderCommandData->world_transient_char_count;
    size_t& worldTextDrawCount = GameState->RenderCommandData->worldTextDrawCount;








    vec3 zero_vec = { 0,0,0 };
    vec3 originPos = CameraRelativePosition(cameraComp, ivec3_create(0), zero_vec);

    //vestigial things from setting up vulkan draw at the absolute origin
    Memory->model.m[12] = (originPos.x);
    Memory->model.m[13] = (originPos.y);
    Memory->model.m[14] = (originPos.z);

    Memory->axis_model.m[12] = (originPos.x);
    Memory->axis_model.m[13] = (originPos.y);
    Memory->axis_model.m[14] = (originPos.z);



    //submit draw commands for chunk outlines

    chunk_data* chunkData = GameState->chunkData;

#if 1
    for (u32 i = 0; i < GameState->chunkData->cameraGridVisibleCount; i++) {
        TIMED_BLOCK("CHUNK DRAW COMMANDS");

        u32 chunkID = GameState->chunkData->cameraGridVisible[i];
        ivec3 coords = GameState->chunkData->coords[chunkID];

        // if(chunkData->toroidal_space_enabled){
        //     ivec3 toroidal_coords = ivec3(0);//TOROIDAL SPACE TEST
        //     toroidal_coords.x = (coords.x % (chunkData->chunk_coord_bounds.x + 1));
        //     toroidal_coords.y = (coords.y % (chunkData->chunk_coord_bounds.y + 1));
        //     toroidal_coords.z = (coords.z % (chunkData->chunk_coord_bounds.z + 1));
        //     chunkID = ChunkManager::findOrCreateChunk(chunkData, toroidal_coords, nullptr, false);
        //     if(chunkID == NULL_CHUNK)continue;
        // }


        vec3 relPos = CameraRelativePosition(cameraComp, coords, zero_vec);
        mat4 voxelModel = {};
        mat4 chunkModel = {};
        chunkModel = mat4_translate(relPos);
        scale_mat4(chunkModel, CHUNK_SIZE - 1);

        GameState->RenderCommandData->entityDrawCommands[entityDrawCount].mesh_type = MeshTypes::mesh_cube;
        GameState->RenderCommandData->entityDrawCommands[entityDrawCount].topology_type = TopologyTypes::topology_lines;
        GameState->RenderCommandData->entityDrawCommandsSSBO[entityDrawCount].model = chunkModel;
        Assert(entityDrawCount <= MAX_SSBO_ENTITIES);

        GameState->RenderCommandData->entityDrawCommandsSSBO[entityDrawCount++].color = vec4_create(1.0f, 1.0f, 1.0f, 1.0f);

        Assert(worldTextDrawCount + 1 <= MAX_SSBO_ENTITIES);
        //chunk coord text
        mat4& model1 = GameState->RenderCommandData->worldTextDrawCommandsSSBO[worldTextDrawCount].model;
        model1 = mat4_identity();
        mat4_billboard(&model1, relPos, { 0, 0, 0 }, { 0, 1, 0 });//camera is always 0,0,0 relative to the relative position, up vector is always 0,1,0
        GameState->RenderCommandData->worldTextDrawCommandsSSBO[worldTextDrawCount].color = { 1.0f, 0.0f, 0.0f, 1.0f };
        GameState->RenderCommandData->worldTextDrawCommandsSSBO[worldTextDrawCount].scale = 0.1f;
        GameState->RenderCommandData->world_transient_char_offsets[worldTextDrawCount++] = worldCharCount;
        //test chunk coords
        char temp_buffer[16];
        Assert(coords.x != INT32_MAX && coords.y != INT32_MAX && coords.z != INT32_MAX);
        size_t temp_count = int_to_string(coords.x, temp_buffer, 16);
        temp_buffer[temp_count++] = ',';
        temp_count += int_to_string(coords.y, temp_buffer + temp_count, 16);
        temp_buffer[temp_count++] = ',';
        temp_count += int_to_string(coords.z, temp_buffer + temp_count, 16);
        // worldCharCount += handmade_strcpy(GameState->world_transient_char_buffer + worldCharCount, "TEST") + 1;
        worldCharCount += handmade_strcpy(GameState->RenderCommandData->world_transient_char_buffer + worldCharCount, temp_buffer) + 1;
        Assert(worldCharCount <= MAX_TEXT_CHARS);

        voxelModel = mat4_translate(relPos - HALF_CHUNK_SIZE);
        chunk_voxel_draw_command command = {};
        command.chunkID = chunkID;
        command.model = voxelModel;
        GameState->RenderCommandData->chunkDrawCommands[(GameState->RenderCommandData->chunkDrawCommandCount)++] = command;


        //bvh tree
        bvh_tree& tree = chunkData->bvhTrees[chunkID];
        for (int i = 0; i < tree.nodeCount; i++) {
            uint32_t nodeID = tree.nodesToDraw[i];
            if (GameState->draw_bvh) {
                mat4 bvhModel = mat4_identity();

                vec3 min = fpt_to_flt_vec3(tree.box[nodeID].min);
                vec3 max = fpt_to_flt_vec3(tree.box[nodeID].max);

                vec3 bvhPos = (min + max) * 0.5f;
                vec3 bvhScale = (max - min) + 0.1f;

                vec3 bvhRelativePos = CameraRelativePosition(cameraComp, coords, bvhPos);

                bvhModel = mat4_translate(bvhRelativePos);
                scale_mat4(bvhModel, bvhScale);

                draw_entity_command(GameState, bvhModel, MeshTypes::mesh_ColorCube, TopologyTypes::topology_lines, { 1,1,1,1 });
            }
        }


        //entity interaction collisions
        for (int i = 0; i < tree.colliding_node_count; i++) {
            uint16_t node = tree.colliding_node_ids[i];

            vec3 min = fpt_to_flt_vec3(tree.box[node].min);
            vec3 max = fpt_to_flt_vec3(tree.box[node].max);

            vec3 colPos = (min + max) * 0.5f;
            vec3 colScale = (max - min) + 0.1f;

            vec3 colRelPos = CameraRelativePosition(cameraComp, coords, colPos);

            mat4 model = mat4_translate(colRelPos);
            scale_mat4(model, colScale);

            draw_entity_command(GameState, model, MeshTypes::mesh_ColorCube, TopologyTypes::topology_triangles, { 1,1,1,0.25 });

        }

    }
#endif

    //draw skeletal mesh test
#if 1
    EntityComponent& ec = *GameState->entityComponent;

    for (u32 i = 0; i < ec.DataCount; i++) {
        uint32_t entityID = ec.DataToEntityMap[i];
        uint32_t dataIndex = ec.entityToDataMap[entityID];
        uint32_t transIndex = ec.entityToTransMap[entityID];
        uint32_t modelIndex = ec.entityToModelMap[entityID];
        uint32_t aabbIndex = ec.entityToAabbMap[entityID];
        uint32_t obbIndex = ec.entityToObbMap[entityID];
        uint32_t stateIndex = ec.entityToStateMap[entityID];

        ModelComp& modelComp = ec.ModelComps[modelIndex];
        StateComp& state = ec.StateComps[stateIndex];
        AabbComp& aabb = ec.AabbComps[aabbIndex];
        TransComp& trans = ec.TransComps[transIndex];
        ObbComp& obb = ec.ObbComps[obbIndex];

        fpt_vec3 adjusted_pos_in_chunk = trans.pos_in_chunk + trans.collide_movement;

        if (modelIndex != NULL_ENTITY) {

            fpt_vec3 entity_foot_position = adjusted_pos_in_chunk;
            if (aabbIndex != NULL_ENTITY) {
                //shift the entity down by half the aabb y position
                entity_foot_position.y -= fpt_mul(aabb.scale.y, FPT_HALF);
            }
            model* curr_model = &GameState->gltfData->models[modelComp.type];


            quat entity_rotation = fpt_to_flt_quat(trans.rotation);

            //advance animation state
            // state.anim_time += GameState->deltaTime;

            float curmaxMoveAnimTime = GameState->gltfData->models[modelComp.type].animations[state.smMove.curTrack.anim_type].max_time;
            float premaxMoveAnimTime = GameState->gltfData->models[modelComp.type].animations[state.smMove.preTrack.anim_type].max_time;
            float curmaxActAnimTime = GameState->gltfData->models[modelComp.type].animations[state.smAct.curTrack.anim_type].max_time;
            float premaxActAnimTime = GameState->gltfData->models[modelComp.type].animations[state.smAct.preTrack.anim_type].max_time;
            float curmaxAirAnimTime = GameState->gltfData->models[modelComp.type].animations[state.smAir.curTrack.anim_type].max_time;
            float premaxAirAnimTime = GameState->gltfData->models[modelComp.type].animations[state.smAir.preTrack.anim_type].max_time;

            bool curMoveLoop = true;
            bool preMoveLoop = true;
            bool curActLoop = true;
            bool preActLoop = true;
            bool curAirLoop = true;
            bool preAirLoop = true;

            
            switch(state.smMove.curTrack.anim_type){
                case anim_walk:{
                    state.smMove.curTrack.anim_time += GameState->deltaTime*2.0f;
                }break;
                default:{}break;
            }
            switch(state.smMove.preTrack.anim_type){
                case anim_walk:{
                    state.smMove.preTrack.anim_time += GameState->deltaTime*2.0f;
                }break;
                default:{}break;
            }

            switch(state.smAir.curTrack.anim_type){
                case anim_jump:{curAirLoop = false;}break;
                case anim_land:{curAirLoop = false;}break;
            }
            switch(state.smAir.preTrack.anim_type){
                case anim_jump:{preAirLoop = false;}break;
                case anim_land:{preAirLoop = false;}break;
            }



            switch(state.smAct.curTrack.anim_type){
                case anim_left_punch:{curActLoop = false;}break;
                case anim_right_punch:{curActLoop = false;}break;
            }
            switch(state.smAct.preTrack.anim_type){
                case anim_left_punch:{preActLoop = false;}break;
                case anim_right_punch:{preActLoop = false;}break;
            }


            if(!curMoveLoop && (state.smMove.curTrack.anim_time + GameState->deltaTime) >= curmaxMoveAnimTime){
                state.smMove.curTrack.anim_time = curmaxMoveAnimTime;
                // printf("move animation paused at timestep: %f out of %f\n", state.cur_move_track.anim_time, maxMoveAnimTime);
            }else{
                if(state.grounded)state.smMove.curTrack.anim_time += GameState->deltaTime;
            }
            if(!curActLoop && (state.smAct.curTrack.anim_time + GameState->deltaTime) >= curmaxActAnimTime){
                state.smAct.curTrack.anim_time = curmaxActAnimTime;
                // printf("interact animation paused at timestep: %f out of %f\n", state.cur_interact_track.anim_time, maxInteractAnimTime);
            }else{
                state.smAct.curTrack.anim_time += GameState->deltaTime;
            }
            if(!curAirLoop && (state.smAir.curTrack.anim_time + GameState->deltaTime) >= curmaxAirAnimTime){
                state.smAir.curTrack.anim_time = curmaxAirAnimTime;
                // printf("interact animation paused at timestep: %f out of %f\n", state.cur_interact_track.anim_time, maxInteractAnimTime);
            }else{
                state.smAir.curTrack.anim_time += GameState->deltaTime;
            }
            
            if(!preMoveLoop && (state.smMove.preTrack.anim_time + GameState->deltaTime) >= premaxMoveAnimTime){
                state.smMove.preTrack.anim_time = premaxMoveAnimTime;
                // printf("move animation paused at timestep: %f out of %f\n", state.cur_move_track.anim_time, maxMoveAnimTime);
            }else{
                state.smMove.preTrack.anim_time += GameState->deltaTime;
            }
            if(!preActLoop && (state.smAct.preTrack.anim_time + GameState->deltaTime) >= premaxActAnimTime){
                state.smAct.preTrack.anim_time = premaxActAnimTime;
                // printf("interact animation paused at timestep: %f out of %f\n", state.cur_interact_track.anim_time, maxInteractAnimTime);
            }else{
                state.smAct.preTrack.anim_time += GameState->deltaTime;
            }
            if(!preAirLoop && (state.smAir.preTrack.anim_time + GameState->deltaTime) >= premaxAirAnimTime){
                state.smAir.preTrack.anim_time = premaxActAnimTime;
                // printf("interact animation paused at timestep: %f out of %f\n", state.cur_interact_track.anim_time, maxInteractAnimTime);
            }else{
                state.smAir.preTrack.anim_time += GameState->deltaTime;
            }
            // state.cur_spare_track.anim_time += GameState->deltaTime;        
            // state.prev_spare_track.anim_time += GameState->deltaTime;      
            
            // if (animation_time > anim_data->position_keyframe_times[anim_data->num_position_keyframes - 1]) {
            if (state.smMove.curTrack.anim_time > curmaxMoveAnimTime && curMoveLoop) {
                state.smMove.curTrack.anim_time = 0.0f;
                state.smMove.curTrack.looped = true;
            }
            if (state.smAct.curTrack.anim_time > curmaxActAnimTime && curActLoop) {
                state.smAct.curTrack.anim_time = 0.0f;
                state.smAct.curTrack.looped = true;
            }
            if (state.smAir.curTrack.anim_time > curmaxAirAnimTime && curAirLoop) {
                state.smAir.curTrack.anim_time = 0.0f;
                state.smAir.curTrack.looped = true;
            }
            if (state.smMove.preTrack.anim_time > premaxMoveAnimTime && preMoveLoop) {
                state.smMove.preTrack.anim_time = 0.0f;
                state.smMove.preTrack.looped = true;
            }
            if (state.smAct.preTrack.anim_time > premaxActAnimTime && preActLoop) {
                state.smAct.preTrack.anim_time = 0.0f;
                state.smAct.preTrack.looped = true;
            }
            if (state.smAir.preTrack.anim_time > premaxAirAnimTime && preAirLoop) {
                state.smAir.preTrack.anim_time = 0.0f;
                state.smAir.preTrack.looped = true;
            }
            //validate the times
            // printf("max time for animation: %s on model: %s: time: %10.5f\n", get_anim_type_name(state.anim_type), get_model_type_name(modelComp.type), GameState->gltfData->models[modelComp.type].animations[state.anim_type].max_time);
            entity_foot_position.y -= fl2fpt(curr_model->min.y);
            float scale = 1.0f;
            // render_animated_model(GameState, cameraComp, state, modelComp.type, trans.chunk_coords, fpt_to_flt_vec3(entity_foot_position), fpt_to_flt_quat(trans.rotation), scale);
            render_animated_model_blend(GameState, cameraComp, state, modelComp.type, trans.chunk_coords, fpt_to_flt_vec3(entity_foot_position), fpt_to_flt_quat(trans.rotation), scale);

            state.smMove.blend = HalfLifeStep(state.smMove.blendstep, state.smMove.blend, state.smMove.blendTarget, GameState->deltaTime);
            state.smAct.blend  = HalfLifeStep(state.smAct.blendstep, state.smAct.blend, state.smAct.blendTarget, GameState->deltaTime);
            state.smAir.blend  = HalfLifeStep(state.smAir.blendstep, state.smAir.blend, state.smAir.blendTarget, GameState->deltaTime);

            uint32_t invIndex = ec.entityToInventoryMap[entityID];
            //render any equipment the entity has (THIS IS A TEST)
            if (invIndex != NULL_ENTITY) {
                InventoryComp& invComp = ec.InventoryComps[invIndex];

                //need to render all equipment slots/trinkets/current handheld items
                for (int i = 0; i < invComp.equipment_count; i++) {

                }
                for (int i = 0; i < invComp.trinket_count; i++) {

                }
                if (invComp.left_hand[invComp.current_left_hand]) {

                    vec3 item_translation = GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].left_fore_arm_socket].relative_translation;
                    quat item_rotation = GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].left_fore_arm_socket].relative_rotation;


                    inventory_item& item = ec.inventory_items[invComp.left_hand[invComp.current_left_hand]];
                    model_type item_model_type = item.data.item_model;

                    StateComp nullStateComp = {};
                    model* item_model = &GameState->gltfData->models[item_model_type];
                    entity_node* item_root_node = &GameState->gltfData->nodes[item_model->root_node];

                    //slop to rotate the shield model away from the entity's legs
                    quat test_quat = { 0, 0, 0, 1 };

                    if (item_model_type == model_shield) {
                        test_quat = quat_create(0 * sin(45 * .5),
                            0 * sin(45 * .5),
                            1 * sin(45 * .5),
                            1 * cos(45 * .5));
                    }

                    render_animated_model_blend(GameState, cameraComp, nullStateComp, item_model_type,
                        trans.chunk_coords, quat_rotate_vec3(entity_rotation, (item_translation * scale)) + fpt_to_flt_vec3(entity_foot_position), quat_mul(entity_rotation, quat_mul(item_rotation, test_quat)),
                        scale, item_root_node->base_translation * vec3_create(.5f));

                    entity_node* child_node = &GameState->gltfData->nodes[item_root_node->children[0]];

                }
                if (invComp.right_hand[invComp.current_right_hand]) {

                    vec3 item_translation = GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].right_fore_arm_socket].relative_translation;
                    quat item_rotation = GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].right_fore_arm_socket].relative_rotation;

                    inventory_item& item = ec.inventory_items[invComp.right_hand[invComp.current_right_hand]];
                    model_type item_model_type = item.data.item_model;

                    StateComp nullStateComp = {};
                    model* item_model = &GameState->gltfData->models[item_model_type];
                    entity_node* item_root_node = &GameState->gltfData->nodes[item_model->root_node];

                    //slop to rotate the shield model away from the entity's legs
                    quat test_quat = { 0, 0, 0, 1 };

                    if (item_model_type == model_shield) {
                        test_quat = quat_create(0 * sin(-45 * .5),
                            0 * sin(-45 * .5),
                            1 * sin(-45 * .5),
                            1 * cos(-45 * .5));
                    }

                    render_animated_model_blend(GameState, cameraComp, nullStateComp, item_model_type,
                        trans.chunk_coords, quat_rotate_vec3(entity_rotation, (item_translation * scale)) + fpt_to_flt_vec3(entity_foot_position), quat_mul(entity_rotation, quat_mul(item_rotation, test_quat)),
                        scale, item_root_node->base_translation * vec3_create(.5f));

                    entity_node* child_node = &GameState->gltfData->nodes[item_root_node->children[0]];

                }

                // printf("anim_type: %s, anim_time: %10.5f right fore arm position: %10.5f %10.5f %10.5f \n",      get_anim_type_name(state.anim_type), state.anim_time, item_translation.x, item_translation.y, item_translation.z);
                // printf("anim_type: %s, anim_time: %10.5f right fore arm rotation: %10.5f %10.5f %10.5f %10.5f\n",get_anim_type_name(state.anim_type), state.anim_time,    item_rotation.x, item_rotation.y, item_rotation.z, item_rotation.w   );

                //we would then take the final positon/rotation, apply them to the item attached, and calculate the final aabb hitbox based off that

                if (state.debug_print_hitbox_positions) {//to print out positions of the animation to hardcode hitbox positions for combat simulation

                    // cur_node->relative_rotation = glm::normalize(glm::inverse(base_rotation) * cumulative_rotation);

                    // draw_cube(GameState, cameraComp, glm::ivec3(0), relative_pos, 0.5f); 
                }

                if (invComp.right_item_equipped) {

                }



                // printf("anim_type: %s, anim_time: %10.5f right fore arm position: %10.5f %10.5f %10.5f \n",      get_anim_type_name(state.anim_type), state.anim_time, item_translation.x, item_translation.y, item_translation.z);
                // printf("anim_type: %s, anim_time: %10.5f right fore arm rotation: %10.5f %10.5f %10.5f %10.5f\n",get_anim_type_name(state.anim_type), state.anim_time,    item_rotation.x, item_rotation.y, item_rotation.z, item_rotation.w   );

                //we would then take the final positon/rotation, apply them to the item attached, and calculate the final aabb hitbox based off that

                if (state.debug_print_hitbox_positions) {//to print out positions of the animation to hardcode hitbox positions for combat simulation

                    // cur_node->relative_rotation = glm::normalize(glm::inverse(base_rotation) * cumulative_rotation);

                    // draw_cube(GameState, cameraComp, glm::ivec3(0), relative_pos, 0.5f); 
                }

                if (invComp.left_item_equipped) {

                }


                if (state.debug_print_hitbox_positions) {//to print out positions of the animation to hardcode hitbox positions for combat simulation
                    vec3 shield_translation = GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].left_fore_arm_socket].relative_translation;
                    quat shield_rotation = GameState->gltfData->nodes[GameState->gltfData->models[model_humanoid].left_fore_arm_socket].relative_rotation;

                    // printf("anim_type: %s, anim_time: %10.5f right fore arm position: %10.5f %10.5f %10.5f \n",       get_anim_type_name(state.anim_type), state.anim_time, shield_translation.x, shield_translation.y, shield_translation.z);
                    // printf("anim_type: %s, anim_time: %10.5f right fore arm rotation: %10.5f %10.5f %10.5f %10.5f\n", get_anim_type_name(state.anim_type), state.anim_time,    shield_rotation.x, shield_rotation.y, shield_rotation.z, shield_rotation.w   );

                    //we would then take the final positon/rotation, apply them to the item attached, and calculate the final aabb hitbox based off that
                    vec3 relative_pos = quat_rotate_vec3(entity_rotation, (shield_translation)) + fpt_to_flt_vec3(entity_foot_position);
                    // cur_node->relative_rotation = glm::normalize(glm::inverse(base_rotation) * cumulative_rotation);

                    draw_cube(GameState, cameraComp, ivec3_create(0), relative_pos, 0.5f, { 1, 1, 1, 1 });
                }


            }
        }


        vec3 relPos = CameraRelativePositionFPT(cameraComp, trans.chunk_coords, adjusted_pos_in_chunk);

        //draw direction widget


        mat4 widgetModel = {};
        widgetModel = mat4_translate(relPos);
        scale_mat4(widgetModel, 2.0f);
        mat4 rotationModel = quat_to_mat4(fpt_to_flt_quat(trans.rotation));

        widgetModel = mat4_mul(widgetModel, rotationModel);
        draw_entity_command(GameState, widgetModel, MeshTypes::mesh_DirectionWidgets, TopologyTypes::topology_lines, { 1,1,1,1 });

        //draw a hemisphere to help visualize where to highlight voxels
        draw_entity_command(GameState, widgetModel, MeshTypes::mesh_Hemisphere, TopologyTypes::topology_lines, { 1,1,1,1 });




        if (aabbIndex != NULL_ENTITY) {
            mat4 aabbModel = {};
            vec3 aabbPos = CameraRelativePositionFPT(cameraComp, trans.chunk_coords, adjusted_pos_in_chunk - aabb.offset);
            // vec3 aabbPos = -fpt_to_flt_vec3(cameraComp.pos_in_chunk);
            vec3 aabbScale = fpt_to_flt_vec3(aabb.scale);
            aabbModel = mat4_translate(aabbPos);

            scale_mat4(aabbModel, aabbScale);

            draw_entity_command(GameState, aabbModel, MeshTypes::mesh_ColorCube, TopologyTypes::topology_lines, { 1,1,1,1 });
        }
        if (obbIndex != NULL_ENTITY) {
            mat4 obbModel = {};
            obbModel = mat4_translate(relPos);
            scale_mat4(obbModel, fpt_to_flt_vec3(obb.scale) + 0.05f);

            mat4 obbRotationModel = quat_to_mat4(fpt_to_flt_quat(obb.rotation));
            obbModel = mat4_mul(obbModel, obbRotationModel);

            draw_entity_command(GameState, obbModel, MeshTypes::mesh_ColorCube, TopologyTypes::topology_lines, { 1,1,1,1 });




            mat4 obbaabbModel = mat4_translate(relPos);
            scale_mat4(obbaabbModel, fpt_to_flt_vec3(obb.aabbMax - obb.aabbMin)); // Full extents
            draw_entity_command(GameState, obbaabbModel, MeshTypes::mesh_ColorCube, TopologyTypes::topology_lines, { 1,1,1,1 });


        }

    }



    //draw selected voxel info
    if (GameState->chunkData->voxelRayCastResult.selected) {
        //draw all brush intersected brickmaps
        for (int i = 0; i < GameState->chunkData->brushChunkCoordsCount; i++) {
            mat4 bmmodel = mat4_identity();
            vec3 relpos = CameraRelativePositionFPT(cameraComp, GameState->chunkData->brushChunkCoords[i], fpt_vec3_create(0));
            bmmodel = mat4_translate(relpos);
            scale_mat4(bmmodel, g_ChunkSize);
            draw_entity_command(GameState, bmmodel, MeshTypes::mesh_cube, TopologyTypes::topology_lines, { 1,1,1,1 });
        }

#if 1 //transparent objects obscure stuff for now, i need a different queue to append these too, but then I would need to sort them
        float startSize = 1.0f / 10.0f;
        float endSize = 0.75f;
        for (int i = 0; i < chunkData->voxelPathCount; i++) {
            // if(i != chunkData->voxelPathCount-1)i = chunkData->voxelPathCount-1;
            // if(i < voxelStart)continue;
            // if(i > voxelsToDraw + voxelStart)continue;
            float t = (float)(i) / (float)(chunkData->voxelPathCount);
            float size = (1 - t) * startSize + t * endSize;
            if (chunkData->voxelPath[i].hit)size = endSize;

            vec3 relativePosition = CameraRelativePositionFPT(cameraComp, chunkData->voxelPath[i].chunk_coords, chunkData->voxelPath[i].fpt_voxel_position);


            mat4 voxelModel = mat4_translate(relativePosition);
            scale_mat4(voxelModel, size + 0.1f);
            draw_entity_command(GameState, voxelModel, MeshTypes::mesh_cube, TopologyTypes::topology_triangles, { 1,1,1,0.25 });

        }
#endif

        mat4 voxModel = mat4_identity();
        vec3 relpos = CameraRelativePosition(cameraComp, GameState->chunkData->voxelRayCastResult.chunk_coords, chunkData->voxelRayCastResult.selected_voxel_render_pos);
        voxModel = mat4_translate(relpos);
        scale_mat4(voxModel, 1.01f);
        draw_entity_command(GameState, voxModel, MeshTypes::mesh_cube, TopologyTypes::topology_triangles, { 1,1,1,1 });
    }
    vec3 minChunkCorner = {31.5f,31.5f,31.5f};
    for(int i = 0; i < chunkData->testVoxelHemisphereHighlightCount; i++){
        u32 voxIndex = chunkData->testVoxelHemisphereHighlights[i];
        float voxx = voxIndex % 64;
        float voxy = (voxIndex / 64) % 64;
        float voxz = voxIndex / 4096;
        vec3 voxPos = {voxx, voxy, voxz};
        voxPos -= minChunkCorner;
        mat4 voxModel = mat4_identity();
        vec3 relpos = CameraRelativePosition(cameraComp, {0,0,0}, voxPos);
        voxModel = mat4_translate(relpos);
        scale_mat4(voxModel, 1.01f);
        draw_entity_command(GameState, voxModel, MeshTypes::mesh_cube, TopologyTypes::topology_lines, { 1,1,1,1 });
    }


    mat4 sphereModel = mat4_identity();
    vec3 relpos = CameraRelativePosition(cameraComp, ivec3_create(0, 0, 0), vec3_create(0, 0, 0));
    sphereModel = mat4_translate(relpos);
    scale_mat4(sphereModel, 1.01f);
    // draw_entity_command(GameState, sphereModel, MeshTypes::mesh_Capsule, TopologyTypes::topology_triangles, { 1,1,1,1 });


    // draw_entity_command(GameState, sphereModel, MeshTypes::mesh_Sphere, TopologyTypes::topology_triangles, { 1,1,1,1 });
    draw_entity_command(GameState, sphereModel, MeshTypes::mesh_Hemisphere, TopologyTypes::topology_lines, { 1,1,1,1 });


    //draw brush mesh
    vec3 relativePosition = CameraRelativePositionFPT(cameraComp, chunkData->brushCenterchunk_coords, chunkData->fptBrushPos);
    sphereModel = mat4_translate(relativePosition);
    scale_mat4(sphereModel, fpt2fl(chunkData->fptBrushSize));
    draw_entity_command(GameState, sphereModel, MeshTypes::mesh_Sphere, TopologyTypes::topology_triangles, { 1,1,1,0.25 });


    sphereModel = mat4_translate(relativePosition);
    scale_mat4(sphereModel, fpt2fl(chunkData->fptBrushSize) * 2.0f);
    draw_entity_command(GameState, sphereModel, MeshTypes::mesh_cube, TopologyTypes::topology_lines, { 1,1,1,0.5 });

    //end selected voxel info
    // mat4 rayModel = mat4_translate(CameraRelativePosition(cameraComp, {0,0,0}, vec3_create(0, 1, 5)));
    //ray cast rendering
    // mat4 rayModel = mat4_translate(GetPointInFront(cameraComp, -1));
    mat4 rayModel = mat4_identity();

    relpos = CameraRelativePosition(cameraComp, chunkData->raychunk_coords, chunkData->rayOrigin);

    rayModel = mat4_translate(relpos + (chunkData->rayStart));
    // mat4 rayModel = mat4_translate(fpt_to_flt_vec3(cameraComp.fptTarget * -FPT_ONE));
    mat4 rayRotation = mat4_identity();
    vec3 rayDir = chunkData->rayDir;
    vec3 defaultDir = { 1, 0, 0 };
    if (rayDir != defaultDir) {
        float angle = acos(vec3_dot(defaultDir, rayDir));
        vec3 rotationAxis = vec3_cross(defaultDir, rayDir);
        if (vec3_length(rotationAxis) < 0.001f) {
            //if vectors are opposite, any perpendicular axis will work
            rotationAxis = { 0, 1, 0 };
        }
        else {
            rotationAxis = vec3_normalize(rotationAxis);
        }
        rayRotation = quat_to_mat4(quat_from_axis_angle(rotationAxis, angle));
    }
    rayModel = mat4_mul(rayModel, rayRotation);
    draw_entity_command(GameState, rayModel, MeshTypes::mesh_ray, TopologyTypes::topology_lines, { 1,1,1,1 });








    Memory->view = cameraComp.viewMatrix;
    Memory->proj = cameraComp.projectionMatrix;
    Memory->viewProj = cameraComp.viewProjMatrix;
    Memory->invViewProj = cameraComp.invViewProjMatrix;
    GameState->consecutiveFixedUpdates = 0;

#endif
    // ubo.proj[1][1] *= -1;
    //using a UBO is not the most efficient way to pass frequently changing values to the shader
    //a more efficient way to pass a small buffer of data to shaders are PUSH CONSTANTS, need to look into those
    // printf("HELLO FROM GAME UPDATE AND RENDER!\n");

    // printf("poopoo!\n");
    // printf("why is this working now?\n");


    // END_BLOCK("GAME_UPDATE_AND_RENDER");
}

void draw_inline_element(game_state* GameState, ui_data* uiData, ui_element* parent, ui_element* sub, vec4& scissor, float rootx, float rooty, vec4* color = NULL) {
    vec2 scale, pos;
    scale.x = (sub->maxx - sub->minx);
    scale.y = (sub->maxy - sub->miny);
    pos.x = sub->minx + rootx + (scale.x * 0.5f); ; pos.y = parent->miny + rooty + (scale.y * 0.5) + sub->miny;//centered position
    vec4 givenColor = color ? *color : sub->color;
    vec4 whitecolor = { 1,1,1,1 };
    switch (sub->type) {
    case ui_element_type_text: {
        u32 shapeType = 2;//for border + filled shape
        render_screenspace_border_scissored(GameState, pos, givenColor, scale, scissor, shapeType);
        pos.x -= (scale.x * 0.5f);
        pos.y += (scale.y * 0.5f);
        scale = { 1,1 };
        render_screenspace_text_scissored(GameState, pos, givenColor, scale, false, sub->data.text.label, scissor, 5);
    }break;
    case ui_element_type_text_highlight: {
        u32 shapeType = 2;//for border + filled shape
        render_screenspace_border_scissored(GameState, pos, givenColor, scale, scissor, shapeType);
        pos.x -= (scale.x * 0.5f);
        pos.y += (scale.y * 0.5f);
        scale = { 1,1 };
        render_screenspace_text_monospace_scissor_highlight(GameState, pos, givenColor, scale, false, sub->data.text.label, scissor, 5, sub->data.text.highlightStart, sub->data.text.highlightLen);
    }break;
    case ui_element_type_button: {
        u32 shapeType = 2;//for border + filled shape
        vec2 textPos = { pos.x - (scale.x * 0.5f),pos.y + (scale.y * 0.5f) };
        vec2 textScale = { 1,1 };
        render_screenspace_text_scissored(GameState, textPos, givenColor, textScale, false, sub->data.button.label, scissor, 5);
        render_screenspace_border_scissored(GameState, pos, givenColor, scale, scissor, shapeType);

    }break;
    case ui_element_type_checkbox: {
        u32 shapeType = 0;//for border
        render_screenspace_border_scissored(GameState, pos, givenColor, scale, scissor, shapeType);
        //draw smaller inner checkbox
        pos.x -= (scale.x * 0.25f);
        pos.y -= (scale.y * 0.25f);
        scale.x = (sub->maxx - sub->minx) * 0.5;
        scale.y = (sub->maxy - sub->miny) * 0.5;

        shapeType = *sub->data.checkbox.value ? 2 : 0;
        vec4 greenColor = { 0,1,0,1 };
        vec4 checkboxColor = *sub->data.checkbox.value ? greenColor : givenColor;
        render_screenspace_border_scissored(GameState, pos, checkboxColor, scale, scissor, shapeType);

        pos.x = sub->minx + rootx + (scale.x * 0.5f); ; pos.y = parent->maxy + rooty - (scale.y * 0.5);
        pos.x -= (scale.x * 0.5f);
        pos.y += (scale.y * 0.5f);
        scale = { 1,1 };
        render_screenspace_text_scissored(GameState, pos, givenColor, scale, false, sub->data.checkbox.label, scissor, 5);
    }break;
    case ui_element_type_texture: {
        vec4 uv_coords = { 0,0,1,1 };
        render_ui_texture_id(GameState, pos, scale, uv_coords, whitecolor, scissor, sub->data.texture.textureIndex);

    }break;
    case ui_element_types::ui_element_type_horizontal_slider_float: {
        u32 shapeType = 0;//for border
        render_screenspace_border_scissored(GameState, pos, whitecolor, scale, scissor, shapeType);
        // vec2 textPos = {pos.x - scale.x * 0.5f, pos.y};
        float textOffset = GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale;
        vec2 textPos = { pos.x - scale.x * 0.5f, parent->miny + rooty + textOffset + sub->miny };
        vec2 textScale = { 1,1 };
        vec4 greyColor = { 0.5, 0.5, 0.5, 0.5 };
        render_screenspace_text_scissored(GameState, textPos, greyColor, textScale, false, sub->data.debug_slider_float.label, scissor, 5);
        sub->posy = pos.y - rooty;
        // u32 shapeType = 5;//solid color
        // render_screenspace_border_scissored(GameState, pos, element->data.color.color, scale, scissor, shapeType);
        draw_horizontal_debug_slider(GameState, sub, pos, sub->color, scale, scissor, rootx, rooty);//debug slider drawing assumes y pos is absolute, sub with rooty
        char buffer[32] = {};
        int len = float_to_string(*sub->data.debug_slider_float.current_value, buffer, 32, 2);
        // float vis_range = element->data.debug_slider_float.visible_range;
        // float scrollbar_scaleX  = element->scale.x * vis_range;
        // float scrollbar_scaleY  = element->scale.y;
        // pos.y += scale.y * 0.5f;
        render_screenspace_text_scissored(GameState, pos, whitecolor, textScale, true, buffer, scissor, 5);

        int fuck_the_debugger = 0;

    }break;
    case ui_element_types::ui_element_type_vertical_slider_float: {
        u32 shapeType = 0;//for border
        render_screenspace_border_scissored(GameState, pos, whitecolor, scale, scissor, shapeType);
        // vec2 textPos = {pos.x - scale.x * 0.5f, pos.y};
        float textOffset = GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale;
        vec2 textPos = { pos.x - scale.x * 0.5f, parent->miny + rooty + textOffset + sub->miny };
        vec2 textScale = { 1,1 };
        vec4 greyColor = { 0.5, 0.5, 0.5, 0.5 };
        render_screenspace_text_scissored(GameState, textPos, greyColor, textScale, false, sub->data.debug_slider_float.label, scissor, 5);
        sub->posx = pos.x - rootx;
        sub->posy = parent->miny;
        // u32 shapeType = 5;//solid color
        // render_screenspace_border_scissored(GameState, pos, element->data.color.color, scale, scissor, shapeType);
        draw_vertical_debug_slider(GameState, sub, pos, sub->color, scale, scissor, rootx, rooty);//debug slider drawing assumes y pos is absolute, sub with rooty
        char buffer[32] = {};
        int len = float_to_string(*sub->data.debug_slider_float.current_value, buffer, 32, 2);
        // float vis_range = element->data.debug_slider_float.visible_range;
        // float scrollbar_scaleX  = element->scale.x * vis_range;
        // float scrollbar_scaleY  = element->scale.y;
        // pos.y += scale.y * 0.5f;
        render_screenspace_text_scissored(GameState, pos, whitecolor, textScale, true, buffer, scissor, 5);

        int fuck_the_debugger = 0;

    }break;
    default: {

    }break;
    }
}

extern "C" GAME_DRAW_UI(GameDrawUI) {
    //need to draw the UI after we collate the debug events
    game_state* GameState = (game_state*)Memory->PermanentStorage;


    TIMED_BLOCK("GAME_DRAW_UI");
    //RENDER UI

    ui_data* uiData = GameState->uiData;

    vec4 color = { .5f, .5f, .5f, .25f };  // transparent grey 
    vec2 pos = { 1000.0f, 100.0f };
    vec2 scale = { 1.0f, 1.0f };
    vec4 uv_coords = { 0.0f, 0.0f, 1.0f, 1.00f };//full atlas
    uint32_t new_index = GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1);
    player_input* currInput = &GameState->playerInputs[0][new_index];
#if DRAW_UI
    if (uiData->display_UI_window) {

        float rootx = 0.0f;
        float rooty = 0.0f;
        ui_window* windows = uiData->windows;


        for (int i = 1; i < MAX_WINDOWS; i++) {
            if (!windows[i].base.active)continue;
            char label[32];

            switch (windows[i].type) {
            case window_type_chat: {
                handmade_strcpy(label, windows[i].docked ? "CHT" : "CHAT");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);

            }break;

            case window_type_inventory_hotbar: {
                handmade_strcpy(label, windows[i].docked ? "HBR" : "HOTBAR");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);

                u32* entity_slots = nullptr;

                u32 invIndex = GameState->entityComponent->entityToInventoryMap[GameState->localPlayerEntityIDs[0]];

                InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 
                // printf("hotbar slot scale: %f %f\n", windows[i].data.inventory_hotbar.hotbar_slots[0].scale.x,windows[i].data.inventory_hotbar.hotbar_slots[0].scale.y);

                draw_inventory_hotbar(GameState, uiData, windows + i, &windows[i].data.inventory_hotbar.hotbar_bounds, windows[i].data.inventory_hotbar.hotbar_slots, pos, scale, uv_coords, color);


                rootx = windows[i].base.posx - (uiData->windows[i].base.width * 0.5f);
                rooty = windows[i].base.posy - (uiData->windows[i].base.height * 0.5f);

                //highlighted bounds
                if (windows[i].selected_element) {
                    ui_element* element = windows[i].selected_element;
                    pos.x = windows[i].selected_element_rootx + element->posx; pos.y = windows[i].selected_element_rooty + (element->posy);
                    color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 1.0f;
                    render_screenspace_border(GameState, pos, color, element->scale);
                }

                if (windows[i].selected_bounds) {
                    ui_element* element = windows[i].selected_bounds;
                    pos.x = windows[i].selected_bounds_rootx + element->posx; pos.y = windows[i].selected_bounds_rooty + (element->posy);
                    color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.5f;
                    render_screenspace_border(GameState, pos, color, element->scale);
                }


            }break;

            case window_type_inventory: {
                handmade_strcpy(label, windows[i].docked ? "INV" : "INVENTORY");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);

                if (!windows[i].docked) {

                    u32* entity_slots = nullptr;
                    ui_window& inv = windows[i];
                    u32 invIndex = GameState->entityComponent->entityToInventoryMap[GameState->localPlayerEntityIDs[0]];

                    InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 

                    //DRAW BODY EQUIPMENT SLOTS
                    rootx = inv.base.posx - (inv.base.width * 0.5f);
                    rooty = inv.base.posy - (inv.base.height * 0.5f);


                    entity_slots = invComp.equipment;
                    draw_window_slots(GameState, &inv.data.inventory.equipment_slot_bounds, inv.data.inventory.body_slots,
                        MAX_EQUIPMENT, pos, scale, uv_coords, color, rootx, rooty, ui_element_inventory_type_equipment_slot, entity_slots);

                    entity_slots = invComp.trinkets;
                    draw_window_slots(GameState, &inv.data.inventory.trinket_slot_bounds, inv.data.inventory.trinket_slots,
                        ITEM_SLOT_COUNT, pos, scale, uv_coords, color, rootx, rooty, ui_element_inventory_type_trinket_slot, entity_slots);

                    entity_slots = invComp.items;
                    draw_window_slots(GameState, &inv.data.inventory.inventory_slot_bounds, inv.data.inventory.item_slots,
                        ITEM_SLOT_COUNT, pos, scale, uv_coords, color, rootx, rooty, ui_element_inventory_type_item_slot, entity_slots);


                    draw_inventory_hotbar(GameState, uiData, windows + i, &windows[i].data.inventory.hotbar_bounds, windows[i].data.inventory.hotbar_slots, pos, scale, uv_coords, color);



                    rootx = inv.base.posx - (inv.base.width * 0.5f);
                    rooty = inv.base.posy - (inv.base.height * 0.5f);

                    //highlighted bounds
                    if (inv.selected_element) {
                        ui_element* element = inv.selected_element;
                        pos.x = inv.selected_element_rootx + element->posx; pos.y = inv.selected_element_rooty + (element->posy);
                        color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 1.0f;
                        render_screenspace_border(GameState, pos, color, element->scale);
                    }

                    if (inv.selected_bounds) {
                        ui_element* element = inv.selected_bounds;
                        pos.x = inv.selected_bounds_rootx + element->posx; pos.y = inv.selected_bounds_rooty + (element->posy);
                        color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.5f;
                        render_screenspace_border(GameState, pos, color, element->scale);
                    }





                }
            }
                                      break;
            case window_type_inventory_other: {
                handmade_strcpy(label, windows[i].docked ? "OTH" : "EXTERNAL INVENTORY");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);
            }break;
            case window_type_inventory_storage: {
                handmade_strcpy(label, windows[i].docked ? "STG" : "STORAGE INVENTORY");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);
            }break;
            case window_type_book_text: {
                handmade_strcpy(label, windows[i].docked ? "TXT" : "BOOK TEXT");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);

                if (!windows[i].docked) {
                    ui_window_data::book_text_data& book = uiData->windows[i].data.book_text;
                    //DRAW TEXT WINDOW PAGE BOUNDS
                    rootx = windows[i].base.posx - (uiData->windows[i].base.width * 0.5f);
                    rooty = windows[i].base.posy - (uiData->windows[i].base.height * 0.5f);

                    vec4 scissor = { rootx, rooty, windows[i].base.width, windows[i].base.height };

                    //DRAW BORDERS
                    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;

                    draw_ui_border(GameState, uiData, &book.top_border, pos, color, scale, uv_coords, rootx, rooty);
                    draw_ui_border(GameState, uiData, &book.bottom_border, pos, color, scale, uv_coords, rootx, rooty);
                    draw_ui_border(GameState, uiData, &book.left_border, pos, color, scale, uv_coords, rootx, rooty);
                    draw_ui_border(GameState, uiData, &book.right_border, pos, color, scale, uv_coords, rootx, rooty);

                    //draw sliders
                    draw_ui_border(GameState, uiData, &book.left_horizontal_scrollbar, pos, color, scale, uv_coords, rootx, rooty);
                    draw_ui_border(GameState, uiData, &book.left_vertical_scrollbar, pos, color, scale, uv_coords, rootx, rooty);

                    //draw slider sub elements TEST
                    ui_element* bar = &book.left_horizontal_scrollbar;
                    draw_horizontal_scrollbar(GameState, bar, pos, scissor, rootx, rooty);


                    bar = &uiData->windows[i].data.book_text.left_vertical_scrollbar;
                    draw_vertical_scrollbar(GameState, bar, pos, color, scale, scissor, rootx, rooty);



                    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;

                    //draw left page
                    draw_ui_border(GameState, uiData, &book.left_page, pos, color, scale, uv_coords, rootx, rooty);

                    //draw text into the left page if anything is in the buffer
                    if (book.text_input.length) {
                        pos.x = rootx + book.left_page.minx; pos.y = rooty + book.left_page.miny - GameState->doubleDescent;
                        scale.x = 1.0f; scale.y = 1.0f;
                        color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;


                        //remove the gap for rendering
                        char removedGap[1024];
                        u32 gapLen = book.text_input.gapEnd - book.text_input.gapStart;
                        memcpy(removedGap, book.text_input.buffer, book.text_input.gapStart);
                        if ((book.text_input.length - gapLen) > 0) {
                            memcpy(removedGap + book.text_input.gapStart, book.text_input.buffer + book.text_input.gapEnd, book.text_input.length - book.text_input.gapStart);
                        }
                        removedGap[book.text_input.length] = 0;

                        // render_screenspace_text_scissored(GameState, pos, color, scale, false, book.text_input.buffer, scissor, 1);
                        render_screenspace_text_scissored(GameState, pos, color, scale, false, removedGap, scissor, 5);


                        if (book.text_input.show_cursor) {
                            pos.x += book.text_input.cursor_positions[book.text_input.cursor_pos].x;
                            pos.y += book.text_input.cursor_positions[book.text_input.cursor_pos].y - 10;// + (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
                            scale.x = 5;
                            scale.y = 30;
                            color = { 10,10,10,20 };
                            // printf("RENDER TEXT EDITOR CURSOR HERE!\n");
                            // renderUIElement(GameState->messageCursor, pos, color, scale);
                            render_screenspace_border(GameState, pos, color, scale, 1);
                        }

                    }
                    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;




                    //draw right page
                    draw_ui_border(GameState, uiData, &uiData->windows[i].data.book_text.right_page, pos, color, scale, uv_coords, rootx, rooty);


                    if (uiData->windows[i].selected_bounds) {
                        color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.5f;

                        draw_ui_border(GameState, uiData, uiData->windows[i].selected_bounds, pos, color, scale, uv_coords, rootx, rooty);
                    }
                    if (uiData->windows[i].selected_element) {
                        color.x = 0.0f; color.y = 1.0f; color.z = 0.0f; color.w = 0.5f;

                        //depending on the element selected, draw/color it differently, currently only support scrollbars
                        if (uiData->windows[i].selected_element->type == ui_element_types::ui_element_type_horizontal_scrollbar) {
                            //intersect test to determine if mouse intersects scrollbar
                            ui_element* bar = uiData->windows[i].selected_element;
                            float vis_range = bar->data.slider_float.visible_range;
                            float min = bar->data.slider_float.min;
                            float max = bar->data.slider_float.max;
                            float curval = bar->data.slider_float.current_value;
                            float scrollMin = (bar->minx); //minx
                            float scrollMax = (bar->maxx); //maxx
                            float scrollDiff = scrollMax - scrollMin - (bar->width * vis_range); //the range from 0 to 1 we can slide in //width


                            float scrollbar_posX = rootx + (scrollMin + (bar->width * vis_range * 0.5f)) + (scrollDiff * curval);// * (0.25f));
                            float scrollbar_posY = rooty + bar->posy;
                            float scrollbar_scaleX = bar->scale.x * vis_range;
                            float scrollbar_scaleY = bar->scale.y;
                            color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;

                            pos.x = scrollbar_posX; pos.y = scrollbar_posY;
                            scale.x = scrollbar_scaleX; scale.y = scrollbar_scaleY;
                            render_screenspace_border(GameState, pos, color, scale);

                        }
                        else if (uiData->windows[i].selected_element->type == ui_element_types::ui_element_type_vertical_scrollbar) {
                            ui_element* bar = uiData->windows[i].selected_element;
                            float vis_range = bar->data.slider_float.visible_range;
                            float min = bar->data.slider_float.min;
                            float max = bar->data.slider_float.max;
                            float curval = bar->data.slider_float.current_value;
                            float scrollMin = (bar->miny); //minx
                            float scrollMax = (bar->maxy); //maxx
                            float scrollDiff = scrollMax - scrollMin - (bar->height * vis_range); //the range from 0 to 1 we can slide in //width


                            float scrollbar_posX = rootx + bar->posx;
                            float scrollbar_posY = rooty + (scrollMin + (bar->height * vis_range * 0.5f)) + (scrollDiff * curval);// * (0.25f));
                            float scrollbar_scaleX = bar->scale.x;
                            float scrollbar_scaleY = bar->scale.y * vis_range;

                            color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;
                            pos.x = scrollbar_posX; pos.y = scrollbar_posY;
                            scale.x = scrollbar_scaleX; scale.y = scrollbar_scaleY;
                            render_screenspace_border(GameState, pos, color, scale);
                        }

                        else {
                            color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.5f;
                            draw_ui_border(GameState, uiData, uiData->windows[i].selected_element, pos, color, scale, uv_coords, rootx, rooty);

                        }
                    }
                }






            }break;
            case window_type_book_cover: {
                handmade_strcpy(label, windows[i].docked ? "CVR" : "BOOK COVER");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);







            }break;
            case window_type_stats: {
                handmade_strcpy(label, windows[i].docked ? "STS" : "STATS");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);

            }break;
            case window_type_feedback: {
                handmade_strcpy(label, windows[i].docked ? "FBK" : "FEEDBACK");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);
                if (!windows[i].docked) {
                    ui_window_data::feedback_data& feedback = uiData->windows[i].data.feedback;

                    rootx = windows[i].base.posx - (uiData->windows[i].base.width * 0.5f);
                    rooty = windows[i].base.posy - (uiData->windows[i].base.height * 0.5f);
                    vec4 scissor = { rootx, rooty, windows[i].base.width, windows[i].base.height };

                    ui_element* bar = &uiData->windows[i].vertical_scrollbar;

                    //to show the superimposed window calculations, looks cool to demonstrate
                    // float combined_element_height = feedback.element_count * MIN_WINDOW_DIMENSIONS;
                    // float vis_range = bar->data.slider_float.visible_range;
                    // float curval = bar->data.slider_float.current_value;
                    // float test_height = combined_element_height;
                    // float ypos = windows[i].base.posy -(test_height -windows[i].base.height ) * 0.5f;
                    // //test combined element bounds
                    // pos[0] = windows[i].base.posx; pos[1] = ypos;
                    // scale[0] = windows[i].base.width; scale[1] = test_height;
                    // // render_screenspace_border(GameState, pos, color, scale);

                    // // //test combined element bounds
                    // float offset = ( test_height - windows[i].base.height);
                    // pos[0] = windows[i].base.posx; pos[1] = windows[i].base.posy + (offset * curval)- (offset);
                    // color[0] = 0.0f; color[1] = 0.0f; color[2] = 1.0f; color[3] = 0.5f;
                    // scale[0] = windows[i].base.width; scale[1] = windows[i].base.height;
                    // // render_screenspace_border(GameState, pos, color, scale);


                    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;
                    draw_ui_border(GameState, uiData, &windows[i].vertical_scrollbar, pos, color, scale, uv_coords, rootx, rooty);

                    if (uiData->windows[i].vertical_scrollbar.data.slider_float.max > uiData->windows[i].base.height) {
                        draw_vertical_scrollbar(GameState, bar, pos, color, scale, scissor, rootx, rooty);
                    }
                    //we need to invert the scrollbar current value to scroll UP
                    float scroll_offset = (bar->data.slider_float.max - windows[i].base.height) * (1 - bar->data.slider_float.current_value);
                    float min_visible_y = windows[i].base.miny;
                    float max_visible_y = windows[i].base.maxy;
                    rooty += scroll_offset;



                    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;
                    scale.x = 1.0f; scale.y = 1.0f;

                    int element_count = windows[i].data.feedback.element_count;
                    for (int j = 0; j < element_count; j++) {
                        // printf("j: %d\n", j);

                        int cur_element = (feedback.write_slot - 1 - j) & (MAX_FEEDBACK_ELEMENTS - 1);
                        // printf("cur element: %d\n", cur_element);
                        ui_element* element = windows[i].data.feedback.elements + cur_element;

                        pos.x = rootx; pos.y = rooty + windows[i].base.height - ((j)*MIN_WINDOW_DIMENSIONS);
                        if (pos.y > min_visible_y - 10.0f && pos.y < max_visible_y + 10.0f) {

                            vec4 scissor = { rootx, rooty, windows[i].base.width, windows[i].base.height };
                            // render_screenspace_text(GameState, pos, color, scale, false, element->data.text.label);
                            render_screenspace_text_scissored(GameState, pos, color, scale, false, element->data.text.label, scissor);


                            // char test_label[32];
                            // FontManager::intToStr(j, test_label);
                            // FontManager::render_transient_screenspace_text(GameState, test_label, pos, color, scale, false, windows[i].base.width, true);
                            int fuck_the_debugger = 0;
                        }


                    }
                }



            }break;
            case window_type_debug: {
                if (i >= ui_window_types::window_type_count) {
                    handmade_strcpy(label, windows[i].docked ? "BUG" : uiData->debug_window_labels[i - ui_window_types::window_type_count]);
                }
                else {
                    handmade_strcpy(label, windows[i].docked ? "BUG" : "DEBUG");
                }
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);

                if (windows[i].docked)break;

                rootx = windows[i].base.posx - (uiData->windows[i].base.width * 0.5f);
                rooty = windows[i].base.posy - (uiData->windows[i].base.height * 0.5f);
                vec4 scissor = { rootx, rooty, uiData->windows[i].base.width, uiData->windows[i].base.height };



                ui_element* bar = &uiData->windows[i].vertical_scrollbar;

                float scroll_offsety = (bar->data.slider_float.max - windows[i].base.height) * bar->data.slider_float.current_value;
                float min_visible_y = windows[i].base.miny;
                float max_visible_y = windows[i].base.maxy;

                bar = &uiData->windows[i].horizontal_scrollbar;
                float scroll_offsetx = (bar->data.slider_float.max - windows[i].base.width) * bar->data.slider_float.current_value;
                float min_visible_x = windows[i].base.minx;
                float max_visible_x = windows[i].base.maxx;


                rooty -= scroll_offsety;
                rootx -= scroll_offsetx;



#if LABOR_INTERNAL
                {

                    debug_state* DebugState = (debug_state*)Memory->DebugStorage;
                    u32 index = ((DebugState->currentEventHistoryIndex) % 300);
                    //check that the current debug window is the profile debug window, to draw specific stuff in it
                    if (DebugState->profileWindowBase && DebugState->profileWindowBase == &(windows + i)->base) {

                        //to draw selected bar graph element
                        if (uiData->selected_bar_element && uiData->selected_bargraph_element) {
                            ui_element* bar = uiData->selected_bar_element;
                            scale.x = (bar->maxx - bar->minx);
                            scale.y = uiData->selected_bargraph_element->height;
                            pos.x = bar->minx + rootx + (scale.x * 0.5f); ; pos.y = uiData->selected_bargraph_element->maxy + rooty - (scale.y * 0.5);
                            color = { .5,.5,.5,0.5 };
                            u32 shapeType = 2;//for border + filled shape
                            render_screenspace_border_scissored(GameState, pos, color, scale, scissor, shapeType);
                        }


                        ui_element* bar = DebugState->historyGraph->data.barGraph.start + index;
                        scale.x = (bar->maxx - bar->minx);
                        scale.y = DebugState->historyGraph->height;
                        pos.x = bar->minx + rootx + (scale.x * 0.5f); ; pos.y = DebugState->historyGraph->maxy + rooty - (scale.y * 0.5);
                        color = { .5,.5,.5,0.5 };
                        u32 shapeType = 2;//for border + filled shape
                        render_screenspace_border_scissored(GameState, pos, color, scale, scissor, shapeType);
                    }

                }
#endif



                color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;

                for (int j = 0; j < windows[i].data.debug.element_count; j++) {
                    ui_element* element = windows[i].data.debug.elements + j;
                    pos.x = element->posx + rootx; pos.y = element->posy + rooty;
                    float halfHeight = element->scale.y * 0.5;
                    if (pos.y >= min_visible_y - halfHeight && pos.y <= max_visible_y + halfHeight) {
                        // render_screenspace_border(GameState, pos, color, element->scale);
                        render_screenspace_border_scissored(GameState, pos, color, element->scale, scissor);

                        scale.x = 1.0f; scale.y = 1.0f;


                        switch (element->type) {
                        case ui_element_types::ui_element_type_text: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            // render_screenspace_text(GameState, pos, color, scale, false, element->data.text.label);
                            render_screenspace_text_scissored(GameState, pos, color, scale, false, element->data.text.label, scissor, 5);

                        }break;
                        case ui_element_types::ui_element_type_text_highlight: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            render_screenspace_text_monospace_scissor_highlight(GameState, pos, color, scale, false, element->data.text.label, scissor, 5, element->data.text.highlightStart, element->data.text.highlightLen);
                        }break;
                        case ui_element_types::ui_element_type_checkbox: {

                            // render_screenspace_text(GameState, pos, color, scale, true, element->data.checkbox.label);
                            render_screenspace_text_scissored(GameState, pos, color, scale, true, element->data.checkbox.label, scissor);

                            scale.x = element->width * 0.25f;
                            scale.y = element->height;
                            pos.x = rootx + (element->minx + (scale.x * 0.5f)); pos.y = rooty + element->miny + (scale.y * 0.5f);
                            render_screenspace_border(GameState, pos, color, scale);
                            if (*element->data.checkbox.value) {
                                scale.x *= 0.75f; scale.y = element->height * 0.75f;
                                render_screenspace_box(GameState, pos, color, scale);
                                int fuck_the_debugger = 0;
                            }
                        }break;
                        case ui_element_types::ui_element_type_button: {
                            // render_screenspace_text(GameState, pos, color, scale, true, element->data.button.label);
                            render_screenspace_text_scissored(GameState, pos, color, scale, true, element->data.button.label, scissor);

                            scale.x = element->width * 0.25f;
                            scale.y = element->height;
                            pos.x = rootx + (element->minx + (scale.x * 0.5f)); pos.y = rooty + element->miny + (scale.y * 0.5f);
                            render_screenspace_border(GameState, pos, color, scale);

                            render_screenspace_box(GameState, pos, color, scale);
                            int fuck_the_debugger = 0;
                        }break;
                        case ui_element_types::ui_element_type_bar_graph: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            // render_screenspace_text(GameState, pos, color, scale, false, element->data.text.label);
                            render_screenspace_text_scissored(GameState, pos, color, scale, false, element->data.barGraph.label, scissor, 5);
                            for (u32 k = 0; k < element->data.barGraph.elementCount; k++) {
                                ui_element* bar = element->data.barGraph.start + k;
                                scale.x = (bar->maxx - bar->minx);
                                scale.y = bar->maxy - bar->miny;
                                // scale.x = element->width;
                                pos.x = bar->minx + rootx + (scale.x * 0.5f); ; pos.y = element->maxy + rooty - (scale.y * 0.5);
                                //test color
                                // color = {1,.5,.5,1.5};
                                u32 shapeType = 2;//for border + filled shape
                                render_screenspace_border_scissored(GameState, pos, bar->color, scale, scissor, shapeType);
                                // printf("barGraph element count: %u\n", element->data.barGraph.elementCount);
                                // printf("bar width: %f\n", scale.x);
                                int fuck_the_debugger = 0;
                            }
                        }break;
                        case ui_element_types::ui_element_type_inline: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            for (u32 k = 0; k < element->data.inlineElement.elementCount; k++) {
                                ui_element* inlineElement = element->data.inlineElement.start + k;
                                draw_inline_element(GameState, uiData, element, inlineElement, scissor, rootx, rooty);
                                int fuck_the_debugger = 0;
                            }
                            // if(element->data.inlineElement.selected_element){
                            //     vec4 redColor = {1, 0, 0, 1};
                            //     draw_inline_element(GameState, uiData, element, element->data.inlineElement.selected_element, scissor, rootx, rooty, &redColor);
                            //     int fuck_the_debugger = 0;
                            //     // printf("drawing selected inline element\n");
                            // }
                            int fuck_the_debugger = 0;
                        }break;
                        case ui_element_types::ui_element_type_texture: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            scale.x = element->height; //make the texture a square for now
                            scale.y = element->height;
                            pos.x += scale.x * 0.5;
                            pos.y -= scale.y * 0.5;
                            uv_coords = { 0,0,1,1 };
                            color = { 1,1,1,1 };
                            render_ui_texture_id(GameState, pos, scale, uv_coords, color, scissor, element->data.texture.textureIndex);
                            int fuck_the_debugger = 0;

                        }break;
                        case ui_element_types::ui_element_type_color: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            scale.x = element->width; //make the texture a square for now
                            scale.y = element->height;
                            pos.x += scale.x * 0.5;
                            pos.y -= scale.y * 0.5;
                            uv_coords = { 0,0,1,1 };
                            element->data.color.color.w *= 20;
                            u32 shapeType = 5;//solid color
                            render_screenspace_border_scissored(GameState, pos, element->data.color.color, scale, scissor, shapeType);
                            int fuck_the_debugger = 0;

                        }break;
                        case ui_element_types::ui_element_type_horizontal_slider_float: {
                            pos.x = element->minx + rootx; pos.y = element->maxy + rooty;
                            scale.x = element->width; //make the texture a square for now
                            scale.y = element->height;
                            // vec2 textPos = {element->minx, element->maxy};
                            vec2 textPos = pos;
                            pos.x += scale.x * 0.5;
                            pos.y -= scale.y * 0.5;
                            vec2 textScale = { 1,1 };
                            render_screenspace_text_scissored(GameState, textPos, color, textScale, false, element->data.debug_slider_float.label, scissor, 5);

                            uv_coords = { 0,0,1,1 };
                            // u32 shapeType = 5;//solid color
                            // render_screenspace_border_scissored(GameState, pos, element->data.color.color, scale, scissor, shapeType);
                            vec4 greenColor = { 0,1,0,1 };
                            draw_horizontal_debug_slider(GameState, element, pos, element->color, scale, scissor, rootx, rooty);
                            char buffer[32] = {};
                            int len = float_to_string(*element->data.debug_slider_float.current_value, buffer, 32, 2);
                            // float vis_range = element->data.debug_slider_float.visible_range;
                            // float scrollbar_scaleX  = element->scale.x * vis_range;
                            // float scrollbar_scaleY  = element->scale.y;
                            // pos.y += scale.y * 0.5f;
                            render_screenspace_text_scissored(GameState, pos, color, textScale, true, buffer, scissor, 5);

                            int fuck_the_debugger = 0;

                        }break;
                        }




                        int fuck_the_debugger = 0;



                    }
                }

                if (windows[i].selected_element) {
                    if (windows[i].selected_element->clicked) {
                        color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;
                    }
                    else { color.x = 1.0f; color.y = 0.0f; color.z = 0.0f; color.w = 0.5f; }
                    if (windows[i].selected_element->type == ui_element_types::ui_element_type_vertical_scrollbar) {//the scrollbar will be shifted up if we don't account for it
                        rooty += scroll_offsety;
                    }
                    vec4 scissor = { rootx, rooty, uiData->windows[i].base.width, uiData->windows[i].base.height };

                    render_screenspace_border_scissored(GameState, pos, color, scale, scissor);
                    color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.5f;

                }


                //reset position for scrollbar, need to draw it last so it appears overtop other elements
                rooty = windows[i].base.posy - (uiData->windows[i].base.height * 0.5f);
                rootx = windows[i].base.posx - (uiData->windows[i].base.width * 0.5f);

                if (uiData->windows[i].vertical_scrollbar.data.slider_float.max > uiData->windows[i].base.height) {
                    draw_vertical_scrollbar(GameState, &uiData->windows[i].vertical_scrollbar, pos, color, scale, scissor, rootx, rooty);
                }

                //reset position for scrollbar, need to draw it last so it appears overtop other elements
                if (uiData->windows[i].horizontal_scrollbar.data.slider_float.max > uiData->windows[i].base.width) {
                    draw_horizontal_scrollbar(GameState, &uiData->windows[i].horizontal_scrollbar, pos, scissor, rootx, rooty);
                }





            }break;
            default: {
                handmade_strcpy(label, windows[i].docked ? "ERR!" : "ERROR!");
                draw_ui_window_base(GameState, &windows[i].base, label, pos, uv_coords, color);
            }break;
            }


        }


        if (uiData->hovered_element) {
            char label[32] = "INFO PANEL";
            draw_ui_window_base(GameState, &uiData->information_panel, label, pos, uv_coords, color);
            pos.x = uiData->information_panel.minx; pos.y = uiData->information_panel.miny + GameState->lineAdvance * 0.25;
            vec4 scissor = { uiData->information_panel.minx, uiData->information_panel.miny, uiData->information_panel.width, uiData->information_panel.height };
            color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 0.7f;
            scale.x = 1.0f; scale.y = 1.0f;
            //for wrapping we need to account for rootx offset

            render_screenspace_text_scissored(GameState, pos, color, scale, false, uiData->information_panel_text, scissor);

        }







        char coord_buffer[32];
        size_t temp_count = int_to_string(currInput->mouse_x, coord_buffer, 16);
        coord_buffer[temp_count++] = ',';
        temp_count += int_to_string(currInput->mouse_y, coord_buffer + temp_count, 16);

        pos.x = (float)currInput->mouse_x; pos.y = (float)currInput->mouse_y;
        color.x = 1.0f; color.y = 0.0f; color.z = 1.0f; color.w = 1.0f;
        scale.x = 1.0f; scale.y = 1.0f;
        render_screenspace_text(GameState, pos, color, scale, false, coord_buffer);


    }
    else {//draw always visible elements
        ui_window* hotbar = uiData->windows + ui_window_types::window_type_inventory_hotbar;
        if(hotbar->base.active){
            draw_inventory_hotbar(GameState, uiData, hotbar, &hotbar->data.inventory_hotbar.hotbar_bounds, hotbar->data.inventory_hotbar.hotbar_slots, pos, scale, uv_coords, color);

        }

        //draw selected voxel coords at mouse position
        if (GameState->chunkData->voxelRayCastResult.selected) {

            char coord_buffer[32];
            ivec3 voxCoords = GameState->chunkData->voxelRayCastResult.voxel_coords;
            size_t temp_count = int_to_string(voxCoords.x, coord_buffer, 8);
            coord_buffer[temp_count++] = ',';
            temp_count += int_to_string(voxCoords.y, coord_buffer + temp_count, 8);
            coord_buffer[temp_count++] = ',';
            temp_count += int_to_string(voxCoords.z, coord_buffer + temp_count, 8);

            pos.x = (float)currInput->mouse_x; pos.y = (float)currInput->mouse_y;
            color.x = 1.0f; color.y = 0.0f; color.z = 1.0f; color.w = 1.0f;
            scale.x = 1.0f; scale.y = 1.0f;

            render_screenspace_text(GameState, pos, color, scale, false, coord_buffer);

            char voxValBuffer[8];
            char voxIndexBuffer[8];
            char voxNoiseBuffer[16];

            
            pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            u32 voxIndex = voxCoords.x + (voxCoords.y * 64) + (voxCoords.z * 4096);
            temp_count = int_to_string(voxIndex, voxIndexBuffer, 8);
            render_screenspace_text(GameState, pos, color, scale, false, voxIndexBuffer);

            fpt_vec3 fptbrushPos =  GameState->chunkData->fptBrushPos;
            vec3 brushPos = {fptbrushPos.x / (float)(1<<16), fptbrushPos.y / (float)(1<<16), fptbrushPos.z / (float)(1<<16)};

            // float brushSize = GameState->chunkData->brushSize;
            // vec3 brushMin = brushPos - brushSize;
            // vec3 brushMax = brushPos + brushSize;

            char brushPosBuffer[32];
            temp_count  = float_to_string(brushPos.x, brushPosBuffer, 8, 1); 
            brushPosBuffer[temp_count++] = ',';
            temp_count += float_to_string(brushPos.y, brushPosBuffer+temp_count, 8, 1); 
            brushPosBuffer[temp_count++] = ',';
            temp_count += float_to_string(brushPos.z, brushPosBuffer+temp_count, 8, 1); 
            pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            render_screenspace_text(GameState, pos, color, scale, false, brushPosBuffer);


            // u32 chunkID = GameState->chunkData->voxelRayCastResult.chunkID;
            // Assert(chunkID >= 0 && chunkID < MAX_CHUNKS);
            
            // u8 voxVal = GameState->chunkData->chunkVoxels[chunkID][voxIndex];
            // float noiseVal = GameState->chunkData->chunkNoise[chunkID][voxIndex];
            

            // temp_count = int_to_string(voxVal, voxValBuffer, 8);
            // pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            // render_screenspace_text(GameState, pos, color, scale, false, voxValBuffer);

            // float_to_string(noiseVal, voxNoiseBuffer, 16, 6);
            // pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            // render_screenspace_text(GameState, pos, color, scale, false, voxNoiseBuffer);

            //displays the unscaled position of the selected voxel's noise sample
            // char samplePos_buffer[32];
            // float samplePosX = GameState->chunkData->samplePosX[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // float samplePosY = GameState->chunkData->samplePosY[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // float samplePosZ = GameState->chunkData->samplePosZ[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // temp_count = float_to_string(samplePosX, samplePos_buffer, 8, 2);
            // samplePos_buffer[temp_count++] = ',';
            // temp_count += float_to_string(samplePosY, samplePos_buffer + temp_count, 8, 2);
            // samplePos_buffer[temp_count++] = ',';
            // temp_count += float_to_string(samplePosZ, samplePos_buffer + temp_count, 8, 2);
            // pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            // render_screenspace_text(GameState, pos, color, scale, false, samplePos_buffer);


            // char sampleTile_buffer[32];
            // u8 tileX = GameState->chunkData->tileX[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // u8 tileY = GameState->chunkData->tileY[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // u8 tileZ = GameState->chunkData->tileZ[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // temp_count = int_to_string(tileX, sampleTile_buffer, 8);
            // sampleTile_buffer[temp_count++] = ',';
            // temp_count += int_to_string(tileY, sampleTile_buffer + temp_count, 8);
            // sampleTile_buffer[temp_count++] = ',';
            // temp_count += int_to_string(tileZ, sampleTile_buffer + temp_count, 8);
            // pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            // render_screenspace_text(GameState, pos, color, scale, false, sampleTile_buffer);

            // char sampleRegion_buffer[32];
            // u8 sampleRegionX = GameState->chunkData->sampleRegionX[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // u8 sampleRegionY = GameState->chunkData->sampleRegionY[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // u8 sampleRegionZ = GameState->chunkData->sampleRegionZ[(chunkID & (DEBUG_CHUNKS-1))][voxIndex];
            // temp_count = int_to_string(sampleRegionX, sampleRegion_buffer, 8);
            // sampleRegion_buffer[temp_count++] = ',';
            // temp_count += int_to_string(sampleRegionY, sampleRegion_buffer + temp_count, 8);
            // sampleRegion_buffer[temp_count++] = ',';
            // temp_count += int_to_string(sampleRegionZ, sampleRegion_buffer + temp_count, 8);
            // pos.y += (GameState->RenderCommandData->monospacedScreenFont.lineAdvance * GameState->RenderCommandData->monospacedScreenFont.scale);
            // render_screenspace_text(GameState, pos, color, scale, false, sampleRegion_buffer);



        }


    }


    switch (uiData->current_state) {
    case ui_state_resize_window: {}break;
    case ui_state_drag_window: {}break;
    case ui_state_drag_item: {
        //render dragged item texture
        //use mouse position
        if (uiData->dragged_itemID) {
            set_item_texture(GameState, uiData->dragged_itemID, uv_coords);
            scale.x = uiData->dragged_item_scale_x; scale.y = uiData->dragged_item_scale_y;
            color.x = 1.0f; color.y = 1.0f; color.z = 1.0f; color.w = 1.0f;
            pos.x = (float)currInput->mouse_x; pos.y = (float)currInput->mouse_y;

            render_ui_texture(GameState, pos, scale, uv_coords, color);

        }

    }break;
    case ui_state_drag_slider: {}break;
    case ui_state_chat_input: {}break;
    case ui_state_text_editor_input: {}break;
    case ui_state_none://none falls through to default
    default: {}break;
    }



#endif

    //vestigial, need to work on more tooltip logic
    // if(uiData->selected_element && uiData->tooltipTextLen){

    //     pos.x = (float)currInput->mouse_x; pos.y = (float)currInput->mouse_y + 30; 
    //     color.x = 1.0f; color.y = 0.0f; color.z = 1.0f; color.w = 1.0f;
    //     scale.x = 1.0f; scale.y = 1.0f; 
    //     render_screenspace_text(GameState, pos, color, scale, false, uiData->tooltipText);

    // }
    // if(uiData->selected_element && uiData->tooltipTextLen){

    //highlighted bounds

    vec4 selectedColor = { 1,0,0,1 };
    if (uiData->debug_element_selected) {
        vec2 dscale = uiData->selected_debug_element_memory.scale;
        float dposx = uiData->debug_root.x + (uiData->selected_debug_element_memory.posx + uiData->debug_scrollx_offset);
        float dposy = uiData->debug_root.y + (uiData->selected_debug_element_memory.posy + uiData->debug_scrolly_offset);
        vec2 dpos = { dposx, dposy };
        // vec4 testScissor = {uiData->debug_root.x, uiData->debug_root.y, uiData->debug_scissor.z, uiData->debug_scissor.w};
        vec4 testScissor = uiData->debug_scissor;
        // 941 880
        // testScissor.z = 200;
        // testScissor.w = 750;
        float x = uiData->selected_debug_element_window_base_memory.posx - (uiData->selected_debug_element_window_base_memory.width / 2);
        float y = uiData->selected_debug_element_window_base_memory.posy - (uiData->selected_debug_element_window_base_memory.height / 2);
        float z = (uiData->selected_debug_element_window_base_memory.width);
        float w = (uiData->selected_debug_element_window_base_memory.height);
        // testScissor = {x,y,z,w}; 
        render_screenspace_border_scissored(GameState, dpos, selectedColor, dscale, testScissor);
    }


    if (uiData->inline_element_selected) {
        switch (uiData->selected_inline_element_hash->element->type) {
        
            case ui_element_type_texture: {
                
            }break;

            default:{
                // printf("type: %d\n", uiData->selected_inline_element_memory.type);
                ui_element* parent = uiData->selected_inline_element_hash->element->parentHash->element;
                Assert(parent->type == ui_element_type_inline);
                draw_inline_element(GameState, uiData, parent, uiData->selected_inline_element_hash->element, uiData->inline_scissor, uiData->inline_root.x, uiData->inline_root.y, &selectedColor);

            }break;

        }
        int fuck_the_debugger = 0;
        // printf("drawing selected inline element\n");
    }
    //END RENDER UI
    // END_BLOCK("GAME_DRAW_UI");


}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {

}

