


vec4 fbm(game_state* GameState, vec3 input, float scale){
    vec4 valueNoise = {};
    float val2d = 0.0f;
    vec3 derivatives = {};
    // derivatives = vec3_normalize(derivatives);
    // derivatives = (derivatives+1.0f)*0.5f;
    float octave = 1.0f;
    float higherScale = 1.0f;
    for(int i = 0; i < 4; i++){
        vec2 inputi = {input.x * higherScale, input.y * higherScale};

        vec4 valueNoisei = valueNoise3dTiled(GameState, GameState->chunkData, {inputi.x, inputi.y, (GameState->tiledOffsetZ * scale)});
        valueNoisei.y *= (scale * higherScale);
        valueNoisei.z *= (scale * higherScale);
        valueNoisei.w *= (scale * higherScale);
        valueNoisei.x = (valueNoisei.x + 3.0f) * 0.25f;
        val2d += (valueNoisei.x * octave);
        vec3 derivativesi = {valueNoisei.y, valueNoisei.z, valueNoisei.w};
        derivatives.x += valueNoisei.x;
        derivatives.y += valueNoisei.y;
        derivatives.z += valueNoisei.z;
        higherScale *= 2;
        octave *= 0.5f;
    }
    val2d /= 1.75f;
    return {val2d, derivatives.x, derivatives.y, derivatives.z};

}

void drawLine(int x0, int y0, int x1, int y1, uint32_t* buffer, uint32_t color, int width) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < width)
            buffer[y0 * width + x0] = color;

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void perlinFlowArrows(game_state* GameState){
        int hStep = (int)GameState->coarsePerlinX;
        int vStep = (int)GameState->coarsePerlinY;
        int dStep = (int)GameState->coarsePerlinZ;
                int coarseWidth = (512 / hStep) + hStep;
        int coarseHeight = (512 / vStep) + vStep;
               for (int y = 0; y < 512; y++) {
                for (int x = 0; x < 512; x++) {
                    float val = GameState->perlinVals[y * 512 + x];
                    u8 color = val*255;
                    u32 colorVal = 0xFF000000 | (color << 16) | (color << 8) | (color << 0);
                    GameState->textureTestMem[(y) * 512 + (x)] = colorVal;
             
                }
            }
       for (int y = 0; y < 512; y += 20) {
                for (int x = 0; x < 512; x += 20) {
                    float gx = (float)x / hStep;
                    float gy = (float)y / vStep;
                    int x0 = (int)gx;
                    int y0 = (int)gy;

                    // vec3 derivatives = GameState->valueNoiseDerivatives[y0 * coarseWidth + x0];
                    // vec3 normalizedGrad = vec3_normalize(derivatives);
                    vec2 dirs = GameState->perlinFlow[y0 * coarseWidth + x0];
                    // float angle = val2d * TWOPI;
                    
                    // vec3 dirs = {cosf(val2d), sinf(val2d), val2d};
                    // vec3 normalizedGrad = vec3_normalize(dirs);


                    int arrowLength = 15;
                    // int endX = x + (int)(normalizedGrad.x * arrowLength);
                    // int endY = y + (int)(normalizedGrad.y * arrowLength);
                    int endX = x + (int)(dirs.x * arrowLength);
                    int endY = y + (int)(dirs.y * arrowLength);
                    endX = fclamp(endX, 0, 511);
                    endY = fclamp(endY, 0, 511);

                    drawLine(x, y, endX, endY, GameState->textureTestMem, 0xff0000ff, 512);
                }
            }
            
    texture_update_command newCommand = {};
    newCommand.textureWidth = 512;
    newCommand.textureHeight = 512;
    newCommand.textureMemory = GameState->textureTestMem;
    GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
}

void perlinParticleFlow(game_state* GameState){

    
    if (GameState->perFramePerlin) {

        int hStep = (int)GameState->coarsePerlinX;
        int vStep = (int)GameState->coarsePerlinY;
        int dStep = (int)GameState->coarsePerlinZ;
        int coarseWidth = (512 / hStep) + hStep;
        int coarseHeight = (512 / vStep) + vStep;
        for (int y = 0; y < 512; y++) {
            for (int x = 0; x < 512; x++) {
                float val = GameState->perlinVals[y * 512 + x];
                u8 color = val*255;
                u32 colorVal = 0xFF000000 | (color << 16) | (color << 8) | (color << 0);
                GameState->textureTestMem[(y) * 512 + (x)] = colorVal;
            }
        }
        for (int y = 0; y < 512; y += 20) {
                for (int x = 0; x < 512; x += 20) {
                    float gx = (float)x / hStep;
                    float gy = (float)y / vStep;
                    int x0 = (int)gx;
                    int y0 = (int)gy;

                    // vec3 derivatives = GameState->valueNoiseDerivatives[y0 * coarseWidth + x0];
                    // vec3 normalizedGrad = vec3_normalize(derivatives);
                    vec2 dirs = GameState->perlinFlow[y0 * coarseWidth + x0];
                    // float angle = val2d * TWOPI;
                    
                    // vec3 dirs = {cosf(val2d), sinf(val2d), val2d};
                    // vec3 normalizedGrad = vec3_normalize(dirs);


                    int arrowLength = 15;
                    // int endX = x + (int)(normalizedGrad.x * arrowLength);
                    // int endY = y + (int)(normalizedGrad.y * arrowLength);
                    int endX = x + (int)(dirs.x * arrowLength);
                    int endY = y + (int)(dirs.y * arrowLength);
                    endX = fclamp(endX, 0, 511);
                    endY = fclamp(endY, 0, 511);

                    drawLine(x, y, endX, endY, GameState->textureTestMem, 0xff0000ff, 512);
                }
            }
            
        }
    for(u32 i = 0; i < 512; i++){
        float particleX = GameState->particleX[i]; 
        float particleY = GameState->particleY[i]; 
        u32 x = ((u32)particleX) & 511;
        u32 y = ((u32)particleY) & 511;

        float rad = GameState->particleRad[i];
        int urad = (u32)rad;

        u8 r = GameState->particleR[i];
        u8 g = GameState->particleG[i];
        u8 b = GameState->particleB[i];
        float speed = GameState->particleSpeed[i];
        u32 color = (0xFF << 24) |(b << 16) |(g << 8) |(r << 0); 
        u32 shape = GameState->particleShape[i];
        for(int dy = -rad; dy < rad; dy++){
            int yy = dy+y;
            if(yy >= 512)continue;
            if(yy <  0)continue;

            for(int dx = -rad; dx < rad; dx++){
                int xx = dx+x;
                if(xx >= 512)continue;
                if(xx <  0)continue;

                if(shape){//circle
                    if(dx*dx + dy*dy <= rad*rad) {
                        GameState->textureTestMem[(yy) * 512 + (xx)] = color;
                    }
                }else{//square
                    GameState->textureTestMem[(yy) * 512 + (xx)] = color;
                }
                
            }
        }
        r++;
        g++;
        b++;
        GameState->particleR[i] = r;
        GameState->particleG[i] = g;
        GameState->particleB[i] = b;

        bool grow = GameState->particleGrow[i];
        if(grow)rad += (0.1f * speed);
        else    rad -= (0.1f * speed);
        if(rad > 2.9f){
            GameState->particleGrow[i] = false;
        }else if (rad < 1.1f){
            GameState->particleGrow[i] = true;
        }
        rad += 0.1f;
        GameState->particleRad[i];

        float devX = GameState->particleDevX[i];
        float devY = GameState->particleDevY[i];
        float ddevX = GameState->particledDevX[i];
        float ddevY = GameState->particledDevY[i];

        
        vec2 angle = GameState->perlinFlow[y * 512 + x];
        if((i & 1) == 0){
            angle = GameState->perlinGradientFlow[y * 512 + x];
        }

        devX += ddevX;
        devY += ddevY;
        if(devX > 1.0f || devX < -1.0f){
            ddevX =-ddevX;
        }
        if(devY > 1.0f || devY < -1.0f){
            ddevY =-ddevY;
        }
        GameState->particleDevX[i] = devX;
        GameState->particleDevY[i] = devY;
        GameState->particledDevX[i] = ddevX;
        GameState->particledDevY[i] = ddevY;

        particleX += ((angle.x+devX) * speed);
        particleY += ((angle.y+devY) * speed);

        if(particleX > 512.0f)particleX = 0.0f;
        if(particleX < 0.0f  )particleX = 511.99f;
        if(particleY > 512.0f)particleY = 0.0f;
        if(particleY < 0.0f  )particleY = 511.99f;

        GameState->particleX[i] = particleX;
        GameState->particleY[i] = particleY;
    }

    texture_update_command newCommand = {};
    newCommand.textureWidth = 512;
    newCommand.textureHeight = 512;
    newCommand.textureMemory = GameState->textureTestMem;
    GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
}

void populate2dPerlin(game_state* GameState){
    float scale = GameState->perlinScale;
    scale = 1/pow(2,(int)GameState->tiledScale);
    int period = (int)(512.0f*scale);
    if(period < 1) period = 1;


    for(int y = 0; y < 512; y++){
        for(int x = 0; x < 512; x++){
            float val = (perlin2dTiled(GameState->chunkData, x * scale, y * scale, period));
            float angle = val * TWOPI;

            // Gradient approach (alternative method)
            float right = (perlin2dTiled(GameState->chunkData, (x+1) * scale, (y) * scale, period));
            float down = (perlin2dTiled(GameState->chunkData, (x) * scale, (y+1) * scale, period));
            float center = val;
            // Gradient vector points toward steepest increase
            float dx = right - center;
            float dy = down - center;
            float length = sqrtf(dx*dx + dy*dy);
            if(length > 0.0001f) { // avoid division by zero
                dx /= length;
                dy /= length;
            }
            GameState->perlinVals[y * 512 + x] = (val+1.0f)*0.5f;
            GameState->perlinFlow[y * 512 + x] = {cosf(angle), sinf(angle)};
            // GameState->perlinFlow[y * 512 + x] = {dx*2.0f, dy*2.0f};

            //calculate curl/divergent flow so particles don't get trapped at minima/maxima
            GameState->perlinGradientFlow[y * 512 + x] = {-dy*2.0f, dx*2.0f};

            u8 color = ((val+1.0f)*0.5f) * 255;
            GameState->textureTestMem[y * 512 + x] = (0xFF << 24) | (color << 16) | (color << 8) | (color << 0);

        }
    }
}

void draw2dNoise(game_state* GameState){
#if 1

    if (GameState->perFramePerlin) {
        u64 startCycles = __rdtsc();
        // float scale = 0.009953125f;
        float scale = GameState->perlinScale;
        float scaledTime = GameState->totalTime * 0.25f;

    

        //for tiled noise
        int hStep = (int)GameState->coarsePerlinX;
        int vStep = (int)GameState->coarsePerlinY;
        int dStep = (int)GameState->coarsePerlinZ;
        int coarseWidth = (512 / hStep) + hStep;
        int coarseHeight = (512 / vStep) + vStep;
        scale = 1/pow(2,(int)GameState->tiledScale);
        int period = (int)(512.0f*scale);
        if(period < 1)period = 1;
        float valueNoiseMin = 10.0f;
        float valueNoiseMax = -10.0f;
        float time = scaledTime * GameState->timeScale;


        float xDerivMin =  1000.0f;
        float xDerivMax = -1000.0f;
        float yDerivMin =  1000.0f;
        float yDerivMax = -1000.0f;
        float zDerivMin =  1000.0f;
        float zDerivMax = -1000.0f;
        
        GameState->xDerivMin =  1000.0f;
        GameState->xDerivMax = -1000.0f;
        GameState->yDerivMin =  1000.0f;
        GameState->yDerivMax = -1000.0f;
        GameState->zDerivMin =  1000.0f;
        GameState->zDerivMax = -1000.0f;

        int count = 0;
        int start = 0;
        vec3 inputs[8] = {};
        float xinputs[8] = {};

        memset(GameState->textureTestMem, 0, sizeof(u32) * 512 * 512);

        float val = 0.0f;
        u8 color = 0;
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;

        if(vStep != 1 || hStep != 1){
            
            for (int tileY = 0; tileY < 512; tileY += TEXTILESIZE) {
                for (int tileX = 0; tileX < 512; tileX += TEXTILESIZE) {

                    int resultIDX = 0;

                    // Compute loop
                    for (int y = tileY; y < tileY + TEXTILESIZE; y += vStep) {
                        for (int x = tileX; x < tileX + TEXTILESIZE; x += hStep * 8) {
                            // 8 lanes
                            for (int lane = 0; lane < 8; lane++) {
                                float nx = (x + lane * hStep) * scale;
                                float ny = y * scale;
                                GameState->noiseResults[resultIDX++] =
                                    (perlin2dTiled(GameState->chunkData, nx, ny, period) + 1.0f) * 0.5f;
                            }
                        }
                    }

                    resultIDX = 0;

                    // Write loop
                    for (int y = tileY; y < tileY + TEXTILESIZE; y += vStep) {
                        for (int x = tileX; x < tileX + TEXTILESIZE; x += hStep * 8) {
                            for (int lane = 0; lane < 8; lane++) {
                                u8 color = (u8)(GameState->noiseResults[resultIDX++] * 255);
                                u32 pixelColor = 0xff000000 | (color << 16) | (color << 8) | color;

                                for (int ky = 0; ky < vStep; ky++) {
                                    for (int kx = 0; kx < hStep; kx++) {
                                        int px = x + lane * hStep + kx;
                                        int py = y + ky;

                                        if (px >= 512 || py >= 512) continue; // safeguard
                                        GameState->textureTestMem[py * 512 + px] = pixelColor;
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }else{//vStep and hStep are 1
            
            for (int y = 0; y < 512; y++) {
                for (int x = 0; x < 512; x+=8) {
                    for(int lane = 0; lane < 8; lane++){
                        float val = ((perlin2dTiled(GameState->chunkData, (x+lane)*scale, (y*scale), period) + 1.0f) * 0.5f);
                        u8 color = val * 255;
                        u32 pixelColor = 0xff000000 | (color << 16) | (color << 8) | color;
                        GameState->textureTestMem[y * 512 + (x + lane)] = pixelColor;
                    }
                }
            }

        }

    // for(int y = 0; y < 128;y++){
        // for(int x = 0; x < 128;x++){
            // GameState->textureTestMem[y * 512 + x] = GameState->thorns[y * 128 + x];
        // 
        // }
    // }

    
    // for(int y = 0; y < 64; y++){
    //     for(int x = 0; x < 64;x++){
    //         GameState->textureTestMem[y * 512 + x] = GameState->cracked[y * 64 + x];
        
    //     }
    // }

        // for(int i = 0; i < yTiles; i++){
        //     for(int j = 0; j < xTiles; j++){

        //         for(int y = (i * 128); y < (i * 128) + 128; y++){
        //             for(int x = (j * 128); x < (j * 128) + 128; x++){

        //                 if((x % hStep == 0) || (y % vStep == 0)){
        //                     val = (perlin2dTiled(GameState->chunkData, x * scale, y * scale, period)+1.0f)*0.5f;
        //                     color = val * 255;
        //                     r = color;
        //                     g = color;
        //                     b = color;
        //                 }
        //                 GameState->textureTestMem[y * 512 + x] = (0xff << 24) | ((b << 16)) | ((g << 8)) | ((r));

        //             }
        //         }


        //     }
        // }


        texture_update_command newCommand = {};
        newCommand.textureWidth = 512;
        newCommand.textureHeight = 512;
        newCommand.textureMemory = GameState->textureTestMem;

        GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
        newCommand.textureWidth = 512;
        newCommand.textureHeight = 512;
        newCommand.textureMemory = GameState->textureTestMem1;

        GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
    }

#endif


}






void draw2dNoiseOld(game_state* GameState){
#if 1

    if (GameState->perFramePerlin) {
        u64 startCycles = __rdtsc();
        // float scale = 0.009953125f;
        float scale = GameState->perlinScale;
        float scaledTime = GameState->totalTime * 0.25f;

    

        //for tiled noise
        int hStep = (int)GameState->coarsePerlinX;
        int vStep = (int)GameState->coarsePerlinY;
        int dStep = (int)GameState->coarsePerlinZ;
        int coarseWidth = (512 / hStep) + hStep;
        int coarseHeight = (512 / vStep) + vStep;
        scale = 1/pow(2,(int)GameState->tiledScale);
        int period = (int)(512.0f*scale);
        if(period < 1)period = 1;
        float valueNoiseMin = 10.0f;
        float valueNoiseMax = -10.0f;
        float time = scaledTime * GameState->timeScale;


        float xDerivMin =  1000.0f;
        float xDerivMax = -1000.0f;
        float yDerivMin =  1000.0f;
        float yDerivMax = -1000.0f;
        float zDerivMin =  1000.0f;
        float zDerivMax = -1000.0f;
        
        GameState->xDerivMin =  1000.0f;
        GameState->xDerivMax = -1000.0f;
        GameState->yDerivMin =  1000.0f;
        GameState->yDerivMax = -1000.0f;
        GameState->zDerivMin =  1000.0f;
        GameState->zDerivMax = -1000.0f;

bool tiled3d = 1;
bool domainWarpedFbm = GameState->perlinDemoEnum == demo_domainWarpedValueNoise;
bool octaves = GameState->perlinDemoEnum == demo_fbm;

        int count = 0;
        int start = 0;
        vec3 inputs[8] = {};
        float xinputs[8] = {};
        for (int y = 0; y < coarseHeight; y++) {
            for (int x = 0; x < coarseWidth; x++) {
                int worldX = (x) * hStep;
                int worldY = (y) * vStep;
                vec3 input = {  ((worldX+ GameState->tiledOffsetX) * scale),
                                ((worldY+ GameState->tiledOffsetY)* scale),
                                GameState->tiledOffsetZ * scale
                            };

// #define tiled3d 1
// #define domainWarpedFbm 0
// #define octaves 0
// #define octaves !domainWarpedFbm
// #if tiled3d
float val2d = 0.0f;
if(tiled3d){



    // #if domainWarpedFbm
    if(domainWarpedFbm){

                
                    // u64 noiseStart = __rdtsc();

                    vec4 valueNoise = valueNoise3dTiled(GameState, GameState->chunkData, {input.x+time, input.y+time, (GameState->tiledOffsetZ * scale)+time});
                    vec3 derivatives = {valueNoise.y*scale,valueNoise.z*scale,valueNoise.w*scale};
                    valueNoise.x = (valueNoise.x + 3.0f) * 0.25f;
                    GameState->valueNoiseDerivatives[y * coarseWidth + x] = derivatives;


                    // if(valueNoise.x < valueNoiseMin)valueNoiseMin = valueNoise.x; //to determine the max range to normalize to
                    // if(valueNoise.x > valueNoiseMax)valueNoiseMax = valueNoise.x;
                    // if(GameState->valueNoiseMin > valueNoise.x)GameState->valueNoiseMin = valueNoise.x;
                    // if(GameState->valueNoiseMax < valueNoise.x)GameState->valueNoiseMax = valueNoise.x;

                    // if(valueNoise.y < xDerivMin)xDerivMin = valueNoise.y; 
                    // if(valueNoise.y > xDerivMax)xDerivMax = valueNoise.y;
                    // if(GameState->xDerivMin > valueNoise.y)GameState->xDerivMin = valueNoise.y;
                    // if(GameState->xDerivMax < valueNoise.y)GameState->xDerivMax = valueNoise.y;
                    

                    // if(valueNoise.z < yDerivMin)yDerivMin = valueNoise.z; 
                    // if(valueNoise.z > yDerivMax)yDerivMax = valueNoise.z;
                    // if(GameState->yDerivMin > valueNoise.z)GameState->yDerivMin = valueNoise.z;
                    // if(GameState->yDerivMax < valueNoise.z)GameState->yDerivMax = valueNoise.z;
                    

                    // if(valueNoise.w < zDerivMin)zDerivMin = valueNoise.w; 
                    // if(valueNoise.w > zDerivMax)zDerivMax = valueNoise.w;
                    // if(GameState->zDerivMin > valueNoise.w)GameState->zDerivMin = valueNoise.w;
                    // if(GameState->zDerivMax < valueNoise.w)GameState->zDerivMax = valueNoise.w;
                    
                    //y can be from -100 to 100
                    //z can be from -160 to 160
                    vec4 result = fbm(GameState, input, scale);
                    input.x += 5.41;
                    input.y += 1.38;
                    vec4 result2 = fbm(GameState, input, scale);
                    float val2d = result.x;
                    float val2d2 = result2.x;
                    // vec3 derivatives = {};

                    input.x = val2d;
                    input.y = val2d2;

                    vec4 result3 = fbm(GameState, input, scale);

                    input.x += 5.41;
                    input.y += 1.38;

                    vec4 result4 = fbm(GameState, input, scale);

                    input.x = result3.x;
                    input.y = result4.x;

                    vec4 result5 = fbm(GameState, input, scale);

                    val2d = result5.x;
                    derivatives.x = result5.y;
                    derivatives.y = result5.z;
                    derivatives.z = result5.w;


                    GameState->valueNoiseDerivatives[y * coarseWidth + x] = derivatives;
    // #endif
    }


                // float val2d = perlin3dTiled(GameState->chunkData, input.x, input.y, GameState->tiledOffsetZ * scale,period,period,period);
                // u64 noiseEnd = __rdtsc() - noiseStart;
                // printf("noise time: %zu\n", noiseEnd);
                // float val2d = perlin4dTiled(GameState->chunkData, input.x, input.y, GameState->tiledOffsetZ * scale, GameState->tiledOffsetW * scale,period,period,period,period);
    // #if octaves
    if(octaves){
                    val2d = perlin3dTiled(GameState->chunkData, input.x, input.y, GameState->tiledOffsetZ * scale , period, period, period);

                    vec2 input2 = {input.x * 2.0f, input.y * 2.0f};
                    val2d += (0.5f*((perlin3dTiled(GameState->chunkData, input2.x,input2.y, GameState->tiledOffsetZ * scale * 2.0f,period,period,period*32))));
                    vec2 input3 = {input.x * 4.0f, input.y * 4.0f};
                    val2d += (0.25f*((perlin3dTiled(GameState->chunkData, input3.x,input3.y, GameState->tiledOffsetZ * scale * 4.0f,period,period,period))));
                    vec2 input4 = {input.x * 8.0f, input.y * 8.0f};
                    val2d +=  0.125f*perlin3dTiled(GameState->chunkData, input.x, input.y, GameState->tiledOffsetZ * scale * 8.0f,period,period,period);
                    // val2d = val2d >= 1.0f ? ((1.0f) - (val2d - (int)val2d)) : val2d;
                    // val2d = fclamp(val2d, 0.0f, 1.0f);
                    val2d /= 1.75f;
    // #endif
    }else{

        val2d = (perlin2dTiled(GameState->chunkData, input.x, input.y, period)+1.0f)*0.5f;
    }
    // #else
}else{
// #if octaves
    if(octaves){
        val2d = (perlin2dTiled(GameState->chunkData, input.x, input.y, period)+1.0f)*0.5f;

                vec2 input2 = {input.x * 2.0f, input.y * 2.0f};
                val2d += (0.5f*((perlin2dTiled(GameState->chunkData, input2.x,input2.y, period))));
                vec2 input3 = {input.x * 4.0f, input.y * 4.0f};
                val2d += (0.25f*((perlin2dTiled(GameState->chunkData, input3.x,input3.y, period))));
                vec2 input4 = {input.x * 8.0f, input.y * 8.0f};
                val2d += (0.125f*((perlin2dTiled(GameState->chunkData, input4.x,input4.y, period))));
                // val2d = val2d >= 1.0f ? ((1.0f) - (val2d - (int)val2d)) : val2d;
                // val2d = fclamp(val2d, 0.0f, 1.0f);
                val2d /= 1.75f;
// #endif
    }
// #endif
}


                //treat val2d as elevation and scale down
                // u8 noise2d = ((u8)(255 * val2d));

                GameState->coarseNoiseGrid[y * coarseWidth + x] = val2d;
            }

        }

//experimental perlin AVX2
//         __m256 xoffset = _mm256_setr_ps(0, 1, 2, 3, 4, 5, 6, 7);
//         __m256 wideScale = _mm256_set1_ps(scale);
//         __m256 wideHStep = _mm256_set1_ps(hStep);
//         __m256 wideVStep = _mm256_set1_ps(vStep);
//         __m256 tiledOffsetX = _mm256_set1_ps(GameState->tiledOffsetX);
//         __m256 tiledOffsetY = _mm256_set1_ps(GameState->tiledOffsetY);
//         __m256 tiledOffsetZ = _mm256_set1_ps(GameState->tiledOffsetZ);

//         // Process in tiles to keep data hot in cache
// int tileSize = 64; // or 32, experiment with this
// for (int tileY = 0; tileY < coarseHeight; tileY += tileSize) {
//     for (int tileX = 0; tileX < coarseWidth; tileX += tileSize) {
        
//         // 1. Generate coarse noise for this tile
//         int maxTileY = min(tileY + tileSize, coarseHeight);
//         int maxTileX = min(tileX + tileSize, coarseWidth);
        
//         for (int y = tileY; y < maxTileY; y++) {
//             for (int x = tileX; x < maxTileX; x += 8) {
//                 __m256 px = _mm256_set1_ps(x);
//                 px = _mm256_add_ps(px, xoffset);
//                 px = _mm256_mul_ps(px, wideHStep);
//                 px = _mm256_add_ps(px, tiledOffsetX);
//                 px = _mm256_mul_ps(px, wideScale);

//                 __m256 py = _mm256_set1_ps(y);
//                 py = _mm256_mul_ps(py, wideVStep);
//                 py = _mm256_add_ps(py, tiledOffsetY);
//                 py = _mm256_mul_ps(py, wideScale);

//                 __m256 pz = _mm256_mul_ps(tiledOffsetZ, wideScale);

//                 start = ((y * coarseWidth + (x)));
//                 perlinNoise8x(GameState->chunkData, px, py, pz, GameState->coarseNoiseGrid + start);
//             }
//         }
        
//         // 2. Immediately process pixels that use this coarse noise
//         int pixelStartY = tileY * vStep;
//         int pixelEndY = min(maxTileY * vStep, 512);
//         int pixelStartX = tileX * hStep;
//         int pixelEndX = min(maxTileX * hStep, 512);
        
//         for (int y = pixelStartY; y < pixelEndY; y++) {
//             for (int x = pixelStartX; x < pixelEndX; x++) {
//                 float gx = (float)x / hStep;
//                 float gy = (float)y / vStep;
//                 int x0 = (int)gx;
//                 int y0 = (int)gy;
                
//                 // Your pixel processing code here...
//                 float val2d = GameState->coarseNoiseGrid[y0 * coarseWidth + x0];
//                 u8 noise2d = ((u8)(255 * val2d));
//                 GameState->textureTestMem[(y * 512) + x] = (u32)(0xff000000 |  (noise2d << 16) | (noise2d << 8) | (noise2d));
//                 GameState->textureTestMem1[(y * 512) + x] = (u32)(0xff000000 | (noise2d << 16) | (noise2d << 8) | (noise2d));
//                 // ... rest of pixel processing ...
//             }
//         }
//     }
// }

#if 1 
        for (int y = 0; y < 512; y++) {
            for (int x = 0; x < 512; x++) {
                float gx = (float)x / hStep;
                float gy = (float)y / vStep;
                int x0 = (int)gx;
                int y0 = (int)gy;
                int x1 = (x0 + 1 < coarseWidth ) ? x0 + 1 : x0;
                int y1 = (y0 + 1 < coarseHeight) ? y0 + 1 : y0;

                vec2 input = { x * scale, y * scale };
                float val2d = GameState->coarseNoiseGrid[y0 * coarseWidth + x0];
                
                val2d = pow(val2d, GameState->perlinRedistribution);
                
                u8 noise2d = ((u8)(255 * val2d));

                u8 r = 0;
                u8 g = 0;
                u8 b = 0;
                // #if domainWarpedFbm
                if(domainWarpedFbm){
                // value noise colors
                vec3 derivatives = GameState->valueNoiseDerivatives[y0 * coarseWidth + x0];
                vec2 grad2d = { derivatives.x, derivatives.y };
                float length = sqrtf(grad2d.x*grad2d.x + grad2d.y*grad2d.y);
                vec2 norm2d = { grad2d.x / length, grad2d.y / length };
                float val2dr = (val2d + ((norm2d.x+1.0f)*0.5f))*0.5f;
                float val2dg = (val2d + ((norm2d.y+1.0f)*0.5f))*0.5f;
                float val2db = (val2d + ((derivatives.z+1.0f)*0.5f))*0.5f;
                u8 noise2dr = ((u8)(255 * val2dr));
                u8 noise2dg = ((u8)(255 * val2dg));
                u8 noise2db = ((u8)(255 * val2db));
                r = noise2dr;
                g = noise2dg;
                b = noise2db;
                

                // #else
                }else{
                r = noise2d;
                g = noise2d;
                b = noise2d;

                if(val2d < GameState->perlinWaterLevel){//water
                    b = 255;
                }else if(val2d < GameState->perlinWaterLevel + 0.1f){//beach
                    r = 105;
                    g = 150;
                    b = 90;
                }else if(val2d < GameState->perlinWaterLevel + 0.2f){//forest
                    r = 61;
                    g = 113;
                    b = 89;
                }//anything higher is grayscale
                // #endif
                }
                // if(x < 128 && y < 128)rednoise = 255;
                GameState->textureTestMem[(y * 512) + x] = (u32)(0xff000000 |  (b << 16) | (g << 8) | (r));
                GameState->textureTestMem1[(y * 512) + x] = (u32)(0xff000000 | (b << 16) | (g << 8) | (r));
            }
        }
//     #undef tiled3d
// #undef octaves
// #undef domainWarpedFbm
#endif

        GameState->drawTextureTotal += __rdtsc() - startCycles;
        GameState->drawTextureHits++;

        if(GameState->drawDerivArrows){
            for (int y = 0; y < 512; y += 20) {
                for (int x = 0; x < 512; x += 20) {
                    float gx = (float)x / hStep;
                    float gy = (float)y / vStep;
                    int x0 = (int)gx;
                    int y0 = (int)gy;

                    // vec3 derivatives = GameState->valueNoiseDerivatives[y0 * coarseWidth + x0];
                    // vec3 normalizedGrad = vec3_normalize(derivatives);
                    float val2d = GameState->coarseNoiseGrid[y0 * coarseWidth + x0];
                    float angle = val2d * TWOPI;
                    
                    vec3 dirs = {cosf(val2d), sinf(val2d), val2d};
                    // vec3 normalizedGrad = vec3_normalize(dirs);


                    int arrowLength = 15;
                    // int endX = x + (int)(normalizedGrad.x * arrowLength);
                    // int endY = y + (int)(normalizedGrad.y * arrowLength);
                    int endX = x + (int)(dirs.x * arrowLength);
                    int endY = y + (int)(dirs.y * arrowLength);
                    endX = fclamp(endX, 0, 511);
                    endY = fclamp(endY, 0, 511);

                    drawLine(x, y, endX, endY, GameState->textureTestMem, 0xff0000ff, 512);
                }
            }
        }


        u64 endCycles = __rdtsc() - startCycles;

        texture_update_command newCommand = {};
        newCommand.textureWidth = 512;
        newCommand.textureHeight = 512;
        newCommand.textureMemory = GameState->textureTestMem;

        GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
        newCommand.textureWidth = 512;
        newCommand.textureHeight = 512;
        newCommand.textureMemory = GameState->textureTestMem1;

        GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
    }

#endif

    // for(u32 y = 0; y < 512; y++){```
    //     for(u32 x = 0; x < 512; x++){
    //         u8 r = (u8)((x +     (*GameState->frameCount)) % 256);
    //         u8 g = (u8)((y +     (*GameState->frameCount)) % 256);
    //         u8 b = (u8)((x + y + (*GameState->frameCount)) % 256);
    //         u8 a = 127;

    //         GameState->textureTestMem[y * 512 + x] = (a << 24 | b << 16 | g << 8 | r);
    //     }
    // }
//dynamic texture update test, to help us visualize stuff from the game layer, like noise generation
#if 0
    memset(GameState->textureTestMem, 255, sizeof(u32) * 512 * 512);
    texture_update_command newCommand = {};
    newCommand.textureWidth = 512;
    newCommand.textureHeight = 512;
    newCommand.textureMemory = GameState->textureTestMem;

    GameState->RenderCommandData->screenSpaceTextureUpdateCommands[(GameState->RenderCommandData->screenSpaceTextureUpdateCommandCount)++] = newCommand;
#endif


}