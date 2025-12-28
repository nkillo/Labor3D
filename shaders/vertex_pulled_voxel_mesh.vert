#version 450



struct faceData{
    uvec2 data;
};



layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
}ubo;




layout(std430, binding = 1) readonly buffer faceDataIn{
    faceData dataIn[ ];
};

// BUFFER_RO(u_faceBuffer, uvec2, 0);

layout(push_constant) uniform PushConstants {
    vec4 vertexPullerSettings;    
    mat4 model;
    uint ssboIndex;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragAO;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;



// static const vec3 normalLookup[6] = {
const vec3 normalLookup[6] = {
    vec3( 0,  0,  1),  // Z+
    vec3( 0,  0, -1),  // Z-
    vec3( 0, -1,  0),  // Y+ //swapped the y component for vulkan NDC
    vec3( 0,  1,  0),  // Y- //swapped the y component for vulkan NDC
    vec3( 1,  0,  0),  // X+
    vec3(-1,  0,  0)   // X-
};

// static const vec3 dummyQuad[4] = { //Z+ facing
const vec3 dummyQuad[4] = { //Z+ facing
    vec3( 0,  0,  1), //v0 bottom left  
    vec3( 0,  1,  1), //v1 top left
    vec3( 1,  0,  1), //v2 bottom right
    vec3( 1,  1,  1), //v3 top right
};

// static const int flipLookup[6] = {1, -1, -1, 1, 1, -1};
const int flipLookup[6] = {1, -1, -1, 1, 1, -1};

void main()
{

    // int vertexID = int(gl_VertexIndex & 3u);
    int vertexID = int(gl_VertexIndex % 6u);
    // uint ssboIndex = gl_VertexIndex >> 2u;
    uint faceIndex = gl_VertexIndex / 6u;
    uint ssboIndex = uint(push.ssboIndex) + faceIndex;

    uvec2 faceInfo = dataIn[ssboIndex].data;
    uint facedata = faceInfo.x;
    uint facedata2 = faceInfo.y;
    float lightValue = (float((facedata2 >> 24u) & 0xF) + 2) / 15.0f; // Normalize to 0-1
    // float lightValue = (float((facedata2 >> 24u) & 0xF) + 15) / 15.0f; // Normalize to 0-1
    uint face = (facedata2 >> 29u) & 7u; //7 = 111, so we can get face values from 0 to 5, 0 to 101 in binary

    //is quad flipped
    bool flipped = ((facedata >> 30) & 0x1) != 0u;
    // bool faceNotZ = face & 7;
    
    bool faceNotZPorXPorYN = ((face & 0x3) != 0) && face != 3;
    if(faceNotZPorXPorYN){ //we want to change the order of flipped Y faces
        flipped = !flipped;
    }

    /* given default vertex ordering:
    0 3 1       0 2 3
    flipped
    0 2 1       1 2 3
    */
    const uint indexOrder[2][6] = {
        {0, 3, 1, 0, 2, 3}, // Normal order
        {0, 2, 1, 1, 2, 3}  // Flipped order
    };
    const uint swizzleMask[2] = {0x1032, 0x3210}; // Normal: 0,1,2,3 -> 1,0,3,2; Flipped: 3,2,1,0
    uint mask = swizzleMask[uint(flipped)];
    uint swizzledVertexID = (mask >> (vertexID * 4)) & 0xF;
    uint correctedIndex = indexOrder[uint(flipped)][vertexID];

    float facex = float(facedata & 63u);
    float facey = float((facedata >> 6u) & 63u);
    float facez = float((facedata >> 12u) & 63u);
    float width = float((facedata >> 18u) & 63u);
    float height = float((facedata >> 24u) & 63u);
    
    ivec3 iVertexPos = ivec3(facedata, facedata >> 6u, facedata >> 12u) & 63;
    // iVertexPos += chunkOffsetPos;

    int w = int((facedata >> 18u)&63u), h = int((facedata >> 24u)&63u);
    //uint wDir = (face & 2) >> 1, hDir = 2 - (face >> 2);
    uint wDir = face >> 2;
    uint hDir = 1 + ((face >> 1) & 1) + ((face >> 2) & 1);

    int wMod = int(correctedIndex >> 1);
    int hMod = int(correctedIndex & 1);



    // Adjust vertex position
    if (wDir == 0) iVertexPos.x += (w * wMod * flipLookup[face]);
    else if (wDir == 1) iVertexPos.y += (w * wMod * flipLookup[face]);
    else iVertexPos.z += (w * wMod * flipLookup[face]);

    if (hDir == 0) iVertexPos.x += (h * hMod);
    else if (hDir == 1) iVertexPos.y += (h * hMod);
    else iVertexPos.z += (h * hMod);


    uint packedAO = facedata2 & 0xFF;
    uint ao = (packedAO >> (correctedIndex * 2u)) & 0x03;
    fragAO = clamp(float(ao) / 3.0, 0.5, 1.0);



    // v_position = iVertexPos;
    fragNormal = normalLookup[face];
    // v_ao = float((facedata >> 30u) & 3u) / 3.0;
    // fragColor = vec4(0.2, 0.659, 0.839, 1.0);

    // uint forward  = (facedata2 >> 8 ) & 0xff;
    // uint right = (facedata2 >> 16) & 0xff;

    uint type  = (facedata2 >> 8 ) & 0xff;
    uint type2 = (facedata2 >> 16) & 0xff;

    //forward is type
    //right is  type2

    // float testVal = 100.0/255.0;
    // vec4 gradient  = vec4(0.0, testVal, 0.0, 1.0);
    vec4 color     = vec4(1.0, 1.0,        1.0,         1.0);

    // vec4 gradient  = vec4(0.0, forward/255.0, right/255.0, 1.0);
    // vec4 gradient  = vec4(0.0, forward/255.0, 0.0,         1.0);
    // vec4 gradient  = vec4(0.0, 0.0,        forward/255.0, 1.0);
    // if(face == 0){
    //     // fragColor = color - gradient;
    //     // fragColor = vec4(forward/255.0, 1.0, right/255.0, 1.0);
    //     // fragColor = vec4(1.0, 1.0, forward/255.0, 1.0);
    //     fragColor = vec4(1.0, 1.0, right/255.0, 1.0);

    // }
    // else{
    //     fragColor = vec4(0.0,0.0,0.0,1.0);
    // }
    // fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    float colorVal = type / 255.0f;
    
    switch(type){
        //dirt
        case 1:{fragColor = vec4(0.52f, 0.31f, 0.16f, 1.0f);}break;
        //stone
        case 2:{fragColor = vec4(0.61f, 0.64f, 0.65f, 1.0f);}break;
        //grass
        case 3:{fragColor = vec4(0.2f, 0.8f, 0.3f, 1.0f);}break;
        //tree
        case 4:{fragColor = vec4(0.44f, 0.34f, 0.32f, 1.0f);}break;
        //leaves
        case 5:{fragColor = vec4(0.2116f, 0.9131f, 0.2116f, 1.0f);}break;
        //tin
        case 6:{fragColor = vec4(0.83f, 0.83f, 0.83f, 1.0f);}break;
        //copper
        case 7:{fragColor = vec4(1.0f, 0.62f, 0.27f, 1.0f);}break;
        //silver
        case 8:{fragColor = vec4(0.66f, 0.75f, 0.74f, 1.0f);}break;
        
        default:{
                fragColor = vec4(vec3(colorVal),1.0); 


                if(face == 0){ // UP IS RED
                    // fragColor = vec4(1.0, 0.0, 0.0, 1.0);
                    fragColor = mix(fragColor, vec4(1.0, 0.0, 0.0, 1.0), 0.25);
                }

                else if(face == 1){ // DOWN IS YELLOW
                // else if(vertexID == 1){ // DOWN IS YELLOW
                    // fragColor = vec4(.75, 0.75, 0.0, 1.0);
                    fragColor = mix(fragColor, vec4(.75, 0.75, 0.0, 1.0), 0.25);
                }
                else if(face == 2){ // 
                // else if(vertexID == 2){ // 
                    // fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                    fragColor = mix(fragColor, vec4(0.0, 1.0, 0.0, 1.0), 0.25);
                }
                else if(face == 3){
                // else if(vertexID == 3){
                    // fragColor = vec4(0.0, 0.75, 0.75, 1.0);
                    fragColor = mix(fragColor, vec4(0.0, 0.75, 0.75, 1.0), 0.25);
                }
                else if(face == 4){
                // else if(face == 4){
                    // fragColor = vec4(0.0, 0.0, 1.0, 1.0);
                    fragColor = mix(fragColor, vec4(0.0, 0.0, 1.0, 1.0), 0.25);
                }
                // else if(face == 5){
                else if(face == 5){
                    // fragColor = vec4(0.75, 0.0, 0.75, 1.0);
                    fragColor = mix(fragColor, vec4(0.75, 0.0, 0.75, 1.0), 0.25);
                }
        }break;
    }

    //color the different faces
    #if 0
    fragColor = vec4(vec3(colorVal),1.0); 


    if(face == 0){ // UP IS RED
        // fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        fragColor = mix(fragColor, vec4(1.0, 0.0, 0.0, 1.0), 0.25);
    }

    else if(face == 1){ // DOWN IS YELLOW
    // else if(vertexID == 1){ // DOWN IS YELLOW
        // fragColor = vec4(.75, 0.75, 0.0, 1.0);
        fragColor = mix(fragColor, vec4(.75, 0.75, 0.0, 1.0), 0.25);
    }
    else if(face == 2){ // 
    // else if(vertexID == 2){ // 
        // fragColor = vec4(0.0, 1.0, 0.0, 1.0);
        fragColor = mix(fragColor, vec4(0.0, 1.0, 0.0, 1.0), 0.25);
    }
    else if(face == 3){
    // else if(vertexID == 3){
        // fragColor = vec4(0.0, 0.75, 0.75, 1.0);
        fragColor = mix(fragColor, vec4(0.0, 0.75, 0.75, 1.0), 0.25);
    }
    else if(face == 4){
    // else if(face == 4){
        // fragColor = vec4(0.0, 0.0, 1.0, 1.0);
        fragColor = mix(fragColor, vec4(0.0, 0.0, 1.0, 1.0), 0.25);
    }
    // else if(face == 5){
    else if(face == 5){
        // fragColor = vec4(0.75, 0.0, 0.75, 1.0);
        fragColor = mix(fragColor, vec4(0.75, 0.0, 0.75, 1.0), 0.25);
    }
    // if( uint(u_vertexPullerSettings.x) >= 620000){
    //     fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    // }
    #endif

    fragColor = vec4(fragColor.xyz * lightValue,1.0f);

    vec3 vertexPos = iVertexPos;// - u_eyePositionInt; //they subtracted by the camera position for camera relative rendering. we do the same but on the CPU

    // Apply z-fighting offset
    float zFightOffset = 0.0007 * flipLookup[face] * (wMod * 2.0 - 1.0);
    float hFightOffset = 0.0007 * (hMod * 2.0 - 1.0);

    if (wDir == 0) vertexPos.x += zFightOffset;
    else if (wDir == 1) vertexPos.y += zFightOffset;
    else vertexPos.z += zFightOffset;

    if (hDir == 0) vertexPos.x += hFightOffset;
    else if (hDir == 1) vertexPos.y += hFightOffset;
    else vertexPos.z += hFightOffset;

    // if(face == 5){
    //     fragColor = vec4(.75, 1.0, 0.75, 1.0);
    //     vertexPos = dummyQuad[vertexID];
    // }
 vec2 scaledUV = vec2(
    float(wMod) * width,  // scale U by width
    float(hMod) * height  // scale V by height
);  // Divide by tile size (adjust 16.0 based on your texture size)

    // fragTexCoord = vec4(scaledUV, 0.0, 0.0);
    fragTexCoord = scaledUV;
    gl_Position = ubo.viewProj * push.model * vec4(vertexPos, 1.0);

}