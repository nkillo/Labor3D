

#define MAX_LAYERS 32
#define MAX_EXTENSIONS 512
#define MAX_FORMATS 512
#define MAX_PRESENT_MODES 512
#define MAX_DEVICES 16
#define MAX_EXTENSION_NAME 64
#define VALIDATION_LAYER_COUNT 1
#define DEVICE_EXTENSION_COUNT 8
#define MAX_SWAPCHAIN_IMAGES 8
#define MAX_SHADER_SIZE 65536
#define MAX_FRAMES_IN_FLIGHT 3
#define MAX_DYNAMIC_TEXTURES 4

#define PRINT_CYCLES 0
#define SCREEN_SPACE_TEXTURE_COUNT 8

#define DRAW_ENTITY 1
#define DRAW_SKELETAL_ENTITY 1
#define DRAW_WORLD_TEXT 1
#define DRAW_SCREEN_TEXT 1
#define DRAW_SCREEN_ELEMENTS 1
#define DRAW_VOXEL_MESH 1
#define DRAW_SKYBOX 1
#define DRAW_MULTI_TEXTURE 0
#define UPDATE_DYNAMIC_TEXTURE 0

#define CHUNK_STAGING_BUFFER 0
#define CHUNK_STAGING_SLICES 3

#define STB_IMAGE_IMPLEMENTATION
#include "3rdParty/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "3rdParty/stb_truetype.h"
#include "constants.h"
#define MAX_UI_TEXTURES 1

#define HASH_TABLE_SIZE 65536
#define VERTEX_HASH_BUCKETS 4

#define USE_MODEL 0

#define USE_MSAA 1
#define DRAW_ORIGIN 1
#define USE_COMPUTE 0

#define MAX_PARTICLES 1024



struct SkyboxPushConstants{
    mat4 model;
    vec4 misc;
    vec4 mouse; //x y are pos, z w are delta
    vec2 viewRect;
};

struct MultiTexturePushConstants {
    vec2 textPosition; 
    vec2 textScale;    
    vec4 textColor;    
    vec4 texCoords;    
    float drawTexture;
    u32 texture_index;
};


struct TestRayTracerPushConstants {
    vec4 cameraPos; 
    vec4 viewRect;    
};

struct VoxelVertexPullerPushConstants{
    vec4 vertexPullerSettings;
    mat4 model;
    uint32_t ssboIndex;
};

//face data is defined in constants.h

static inline const uint32_t get_vertex(uint32_t x, uint32_t y, uint32_t z, uint32_t type, uint32_t norm, uint32_t ao) {
    // PRINT("x: %d, y: %d, z: %d, type: %d, norm: %d, ao: %d\n", x-1, y-1, z-1, type, norm, ao);
    return (ao << 29) | (norm << 26) | (type << 18) | ((z - 1) << 12) | ((y - 1) << 6) | (x - 1);
    }


inline FaceData get_face(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t flipped,
    uint8_t packedAO, uint8_t type, uint8_t type2 , uint32_t face, uint8_t light = 15) {

    uint32_t faceInfo1 = (x & 0x3F) | ((y & 0x3F) << 6) | ((z & 0x3F) << 12) |
            ((width & 0x3F) << 18) | ((height & 0x3F) << 24) | ((flipped & 0x1) << 30);

    uint32_t faceInfo2 = packedAO | ((type & 0xFF) << 8) | ((type2 & 0xFF) << 16) |  ((light & 0xF) << 24) | ((face & 0x07) << 29);

    return {faceInfo1, faceInfo2};

}

struct LineVertex {
    vec3 pos;
    u32 color;
    // vec3 color;
};







struct TestMesh{
    LineVertex  vertices[2048];
    uint32_t    indices[2048];
    uint32_t    lineIndices[2048];
    vec3 min;
    vec3 max;
    vec3 extents;

    u32 numVertices;
    u32 numIndices;
    u32 numLineIndices;
};

static void generateHemisphere(TestMesh& mesh, float radius, int latSegments, int lonSegments){
    mesh.max = vec3_create(0);
    mesh.min = vec3_create(1000);
    LineVertex vertex = {};
    //generate vertices
    for(int lat = 0; lat <= latSegments; lat++){
        float phi = (lat / (float)latSegments) * (PI / 2.0f);
        for(int lon = 0; lon <= lonSegments; lon++){
            float theta = (lon / (float)lonSegments) * (2.0f * PI);
            float x = radius * cosf(theta) * sinf(phi);
            float y = radius * sinf(theta) * sinf(phi);
            float z = radius * cosf(phi);
            vertex.pos =  {x,y,z};
            vertex.color =  0xFF0000FF;
            mesh.vertices[mesh.numVertices++] = vertex;
        }
    }
    for(int lat = 0; lat <= latSegments; lat++){
        for(int lon = 0; lon <= lonSegments; lon++){
            int i0 = lat * (lonSegments+1) + lon;
            int i1 = i0 + 1;
            int i2 = i0 + (lonSegments+1);
            int i3 = i2 + 1;
            mesh.indices[mesh.numIndices++] = i0;
            mesh.indices[mesh.numIndices++] = i2;
            mesh.indices[mesh.numIndices++] = i1;

            mesh.indices[mesh.numIndices++] = i1;
            mesh.indices[mesh.numIndices++] = i2;
            mesh.indices[mesh.numIndices++] = i3;
             // Generate line indices
            if(mesh.numLineIndices + 2 <= MAX_VERTICES) {
                // Horizontal lines
                if(lon < lonSegments) {
                    mesh.lineIndices[mesh.numLineIndices++] = i0 + (lat - lonSegments / 2) * (lonSegments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = i0 + (lat - lonSegments / 2) * (lonSegments + 1) + lon + 1;
                }
                
                // Vertical lines
                if(lat < latSegments) {
                    mesh.lineIndices[mesh.numLineIndices++] = i0 + (lat - latSegments / 2) * (latSegments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = i0 + (lat + 1 - latSegments / 2) * (latSegments + 1) + lon;
                }
            }
        }
    }

}

static void generateSphere(TestMesh& mesh, float radius, int segments = 12, uint32_t color = 0) {
    mesh.max = vec3_create(0);
    mesh.min = vec3_create(655360000); //10000 * 2^16
    // strncpy(mesh.name, "testSphere", MAX_NAME_LENGTH - 1);
    // mesh.name[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination

    // Generate vertices
    for(int lat = 0; lat <= segments; lat++) {
        float theta = lat * PI / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for(int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2 * PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            LineVertex vertex;
            vertex.pos = vec3_create(x * radius, y * radius, z * radius);
            mesh.min = vec3_min(mesh.min, (vertex.pos));
            mesh.max = vec3_max(mesh.max, (vertex.pos));

            if(color == 0)vertex.color = 0xFF0000FF;
            else vertex.color = color;
            mesh.vertices[mesh.numVertices] = vertex;
            mesh.numVertices++;


    
            // Generate triangle indices for this vertex if we're not on the last row/column
            if(lat < segments && lon < segments) {
                unsigned int topLeft = lat * (segments + 1) + lon;
                unsigned int topRight = (lon == segments - 1) ? lat * (segments + 1) : topLeft + 1;
                unsigned int bottomLeft = (lat + 1) * (segments + 1) + lon;
                unsigned int bottomRight = (lon == segments - 1) ? (lat + 1) * (segments + 1) : bottomLeft + 1;
                
                // Add indices...
                if(mesh.numIndices + 6 <= MAX_VERTICES) {
                    mesh.indices[mesh.numIndices++] = topLeft;
                    mesh.indices[mesh.numIndices++] = topRight;    // Changed
                    mesh.indices[mesh.numIndices++] = bottomLeft;  // Changed
                    
                    // Second triangle - swap bottomRight and topRight
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    mesh.indices[mesh.numIndices++] = topRight;    // Changed
                    mesh.indices[mesh.numIndices++] = bottomRight; // Changed

                }
            }
            // Generate line indices - create lines along latitude and longitude
            if(mesh.numLineIndices + 2 <= MAX_VERTICES) {
                // Horizontal lines (along latitude)
                if(lon < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = lat * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = lat * (segments + 1) + lon + 1;
                }
                
                // Vertical lines (along longitude)
                if(lat < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = lat * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = (lat + 1) * (segments + 1) + lon;
                }
            }
        }
    }
    // mesh.staticBuffersInitialized = false;
    mesh.extents = (mesh.max - mesh.min) * 0.5f;
    // return index;
}

static void generateCapsule(TestMesh& mesh, float radius, float height, int segments = 12, uint32_t color = 0) {
    
    mesh.max = vec3_create(0);
    mesh.min = vec3_create(655360000); // 10000 * 2^16
    // handmade_strcpy(mesh.name, "capsule", MAX_NAME_LENGTH - 1);
    // mesh.name[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination
    
    // Calculate the half-height of the cylinder portion
    float halfHeight = height * 0.5f - radius;
    
    // Generate vertices
    // Top hemisphere
    for(int lat = 0; lat <= segments / 2; lat++) {
        float theta = lat * PI / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for(int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2 * PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            LineVertex vertex;
            // Move the hemisphere to the top of the cylinder
            vertex.pos = vec3_create(x * radius, y * radius + halfHeight, z * radius);
            mesh.min = vec3_min(mesh.min, (vertex.pos));
            mesh.max = vec3_max(mesh.max, (vertex.pos));
            
            if(color == 0) vertex.color = 0xFF0000FF;//RandomHexColor();
            else vertex.color = color;
            mesh.vertices[mesh.numVertices] = vertex;
            mesh.numVertices++;
            
            // Generate triangle indices for this vertex if we're not on the last row/column
            if(lat < segments / 2 && lon < segments) {
                unsigned int topLeft = lat * (segments + 1) + lon;
                unsigned int topRight = topLeft + 1;
                unsigned int bottomLeft = (lat + 1) * (segments + 1) + lon;
                unsigned int bottomRight = bottomLeft + 1;
                
                // Add indices...
                if(mesh.numIndices + 6 <= MAX_VERTICES) {
                    mesh.indices[mesh.numIndices++] = topLeft;
                    mesh.indices[mesh.numIndices++] = topRight;
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    mesh.indices[mesh.numIndices++] = topRight;
                    mesh.indices[mesh.numIndices++] = bottomRight;
                }
            }
            
            // Generate line indices
            if(mesh.numLineIndices + 2 <= MAX_VERTICES) {
                // Horizontal lines
                if(lon < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = lat * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = lat * (segments + 1) + lon + 1;
                }
                
                // Vertical lines
                if(lat < segments / 2) {
                    mesh.lineIndices[mesh.numLineIndices++] = lat * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = (lat + 1) * (segments + 1) + lon;
                }
            }
        }
    }
    
    // Store the starting vertex index for the cylinder
    int cylinderStartIndex = mesh.numVertices;
    
    // Generate cylinder
    for(int y = 0; y <= segments; y++) {
        float v = (float)y / segments;
        float yPos = halfHeight * (1.0f - 2.0f * v); // Map from [0,1] to [halfHeight, -halfHeight]
        
        for(int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2 * PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            LineVertex vertex;
            vertex.pos = vec3_create(radius * cosPhi, yPos, radius * sinPhi);
            mesh.min = vec3_min(mesh.min, (vertex.pos));
            mesh.max = vec3_max(mesh.max, (vertex.pos));
            
            if(color == 0) vertex.color = 0xFF0000FF;//RandomHexColor();
            else vertex.color = color;
            mesh.vertices[mesh.numVertices] = vertex;
            mesh.numVertices++;
            
            // Generate triangle indices for the cylinder
            if(y < segments && lon < segments) {
                unsigned int topLeft = cylinderStartIndex + y * (segments + 1) + lon;
                unsigned int topRight = topLeft + 1;
                unsigned int bottomLeft = topLeft + segments + 1;
                unsigned int bottomRight = bottomLeft + 1;
                
                if(mesh.numIndices + 6 <= MAX_VERTICES) {
                    mesh.indices[mesh.numIndices++] = topLeft;
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    mesh.indices[mesh.numIndices++] = topRight;
                    
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    mesh.indices[mesh.numIndices++] = bottomRight;
                    mesh.indices[mesh.numIndices++] = topRight;
                }
            }
            
            // Generate line indices
            if(mesh.numLineIndices + 2 <= MAX_VERTICES) {
                // Horizontal lines
                if(lon < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = cylinderStartIndex + y * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = cylinderStartIndex + y * (segments + 1) + lon + 1;
                }
                
                // Vertical lines
                if(y < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = cylinderStartIndex + y * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = cylinderStartIndex + (y + 1) * (segments + 1) + lon;
                }
            }
        }
    }
    
    // Store the starting vertex index for the bottom hemisphere
    int bottomHemisphereStartIndex = mesh.numVertices;
    
    // Bottom hemisphere
    for(int lat = segments / 2; lat <= segments; lat++) {
        float theta = lat * PI / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for(int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2 * PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            LineVertex vertex;
            // Move the hemisphere to the bottom of the cylinder
            vertex.pos = vec3_create(x * radius, y * radius - halfHeight, z * radius);
            mesh.min = vec3_min(mesh.min, (vertex.pos));
            mesh.max = vec3_max(mesh.max, (vertex.pos));
            
            if(color == 0) vertex.color = 0xFF0000FF;//RandomHexColor();
            else vertex.color = color;
            mesh.vertices[mesh.numVertices] = vertex;
            mesh.numVertices++;
            
            // Generate triangle indices for this vertex if we're not on the last row/column
            if(lat < segments && lon < segments) {
                unsigned int topLeft = bottomHemisphereStartIndex + (lat - segments / 2) * (segments + 1) + lon;
                unsigned int topRight = topLeft + 1;
                unsigned int bottomLeft = topLeft + segments + 1;
                unsigned int bottomRight = bottomLeft + 1;
                
                // Add indices...
                if(mesh.numIndices + 6 <= MAX_VERTICES) {
                    mesh.indices[mesh.numIndices++] = topLeft;
                    mesh.indices[mesh.numIndices++] = topRight;
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    
                    mesh.indices[mesh.numIndices++] = bottomLeft;
                    mesh.indices[mesh.numIndices++] = topRight;
                    mesh.indices[mesh.numIndices++] = bottomRight;
                }
            }
            
            // Generate line indices
            if(mesh.numLineIndices + 2 <= MAX_VERTICES) {
                // Horizontal lines
                if(lon < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = bottomHemisphereStartIndex + (lat - segments / 2) * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = bottomHemisphereStartIndex + (lat - segments / 2) * (segments + 1) + lon + 1;
                }
                
                // Vertical lines
                if(lat < segments) {
                    mesh.lineIndices[mesh.numLineIndices++] = bottomHemisphereStartIndex + (lat - segments / 2) * (segments + 1) + lon;
                    mesh.lineIndices[mesh.numLineIndices++] = bottomHemisphereStartIndex + (lat + 1 - segments / 2) * (segments + 1) + lon;
                }
            }
        }
    }
    
    // mesh.staticBuffersInitialized = false;
    mesh.extents = (mesh.max - mesh.min) * 0.5f;
    // return index;
}


#define BORDER_THICKNESS 0.025

TextVertex border_vertices[] = {                                                        //old version for non flipped vertex shader
       //bottom border                                                                  //top border
      /*  {{-0.5f                      , -0.5f                     , 0},    {0, 0}}, */ {{-0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {0, 0}},
      /*  {{ 0.5f                      , -0.5f                     , 0},    {1, 0}}, */ {{ 0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {1, 0}},
      /*  {{ 0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {1, 1}}, */ {{ 0.5f                      , -0.5f                     , 0},    {1, 1}},
      /*  {{-0.5f                      , -0.5f                     , 0},    {0, 0}}, */ {{-0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {0, 0}},
      /*  {{ 0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {1, 1}}, */ {{ 0.5f                      , -0.5f                     , 0},    {1, 1}},
      /*  {{-0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {0, 1}}, */ {{-0.5f                      , -0.5f                     , 0},    {0, 1}},
      /*  //right border                                                             */ //right border
      /*  {{ 0.5f - BORDER_THICKNESS   , -0.5f + BORDER_THICKNESS  , 0},    {0, 0}}, */ {{ 0.5f - BORDER_THICKNESS   ,  0.5f - BORDER_THICKNESS  , 0},    {0, 0}},
      /*  {{ 0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {1, 0}}, */ {{ 0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {1, 0}},
      /*  {{ 0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {1, 1}}, */ {{ 0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {1, 1}},
      /*  {{ 0.5f - BORDER_THICKNESS   , -0.5f + BORDER_THICKNESS  , 0},    {0, 0}}, */ {{ 0.5f - BORDER_THICKNESS   ,  0.5f - BORDER_THICKNESS  , 0},    {0, 0}},
      /*  {{ 0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {1, 1}}, */ {{ 0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {1, 1}},
      /*  {{ 0.5f - BORDER_THICKNESS   ,  0.5f - BORDER_THICKNESS  , 0},    {0, 1}}, */ {{ 0.5f - BORDER_THICKNESS   , -0.5f + BORDER_THICKNESS  , 0},    {0, 1}},
      /*  //top border                                                               */ //bottom border
      /*  {{-0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {0, 0}}, */ {{-0.5f                      ,  0.5f                     , 0},    {0, 0}},
      /*  {{ 0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {1, 0}}, */ {{ 0.5f                      ,  0.5f                     , 0},    {1, 0}},
      /*  {{ 0.5f                      ,  0.5f                     , 0},    {1, 1}}, */ {{ 0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {1, 1}},
      /*  {{-0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {0, 0}}, */ {{-0.5f                      ,  0.5f                     , 0},    {0, 0}},
      /*  {{ 0.5f                      ,  0.5f                     , 0},    {1, 1}}, */ {{ 0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {1, 1}},
      /*  {{-0.5f                      ,  0.5f                     , 0},    {0, 1}}, */ {{-0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {0, 1}},
      /*  //left border                                                              */ //left border
      /*  {{-0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {0, 0}}, */ {{-0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {0, 0}},
      /*  {{-0.5f + BORDER_THICKNESS   , -0.5f + BORDER_THICKNESS  , 0},    {1, 0}}, */ {{-0.5f + BORDER_THICKNESS   ,  0.5f - BORDER_THICKNESS  , 0},    {1, 0}},
      /*  {{-0.5f + BORDER_THICKNESS   ,  0.5f - BORDER_THICKNESS  , 0},    {1, 1}}, */ {{-0.5f + BORDER_THICKNESS   , -0.5f + BORDER_THICKNESS  , 0},    {1, 1}},
      /*  {{-0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {0, 0}}, */ {{-0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {0, 0}},
      /*  {{-0.5f + BORDER_THICKNESS   ,  0.5f - BORDER_THICKNESS  , 0},    {1, 1}}, */ {{-0.5f + BORDER_THICKNESS   , -0.5f + BORDER_THICKNESS  , 0},    {1, 1}},
      /*  {{-0.5f                      ,  0.5f - BORDER_THICKNESS  , 0},    {0, 1}}, */ {{-0.5f                      , -0.5f + BORDER_THICKNESS  , 0},    {0, 1}},

};
u32 border_indices[] = {
    //set these up once we know we can render borders fine with just vertex
    0,
};

TextVertex box_vertices[] = {                                                           //old version for non flipped vertex shader
     /*  {{-0.5f                      ,  0.5f                     , 0},    {0, 0}}, */  {{-0.5f                      , -0.5f                     , 0},    {0, 0}},
     /*  {{ 0.5f                      , -0.5f                     , 0},    {1, 1}}, */  {{ 0.5f                      ,  0.5f                     , 0},    {1, 1}},
     /*  {{ 0.5f                      ,  0.5f                     , 0},    {1, 0}}, */  {{ 0.5f                      , -0.5f                     , 0},    {1, 0}},
     /*  {{-0.5f                      ,  0.5f                     , 0},    {0, 0}}, */  {{-0.5f                      , -0.5f                     , 0},    {0, 0}},
     /*  {{-0.5f                      , -0.5f                     , 0},    {0, 1}}, */  {{-0.5f                      ,  0.5f                     , 0},    {0, 1}},
     /*  {{ 0.5f                      , -0.5f                     , 0},    {1, 1}}, */  {{ 0.5f                      ,  0.5f                     , 0},    {1, 1}},
};
// TextVertex box_vertices[] = {

// };

struct particle{
    vec2 pos;
    vec2 velocity;
    vec4 color;
};




// enum command_type{
//     type_draw_line,
//     type_draw_cube,
//     type_draw_model,
// };

// struct command{
//     command_type type;
//     model* model;
//     union{
//         vec3* position;
//     };
// };

// struct commandBuffer{
//     command commands[MAX_COMMANDS];
//     u32 command_count;
// };


// void entity_update(){
//     for(int i = 0; i < placeholder; i++){
//         //update enitty position
//         push_command(position, model);
//     }
// }

struct UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
};


struct ScreenSpaceUniformBufferObject{
    vec4 viewRect;  //padded to vec4 to prevent padding issues, we only use x and y for now
    vec4 misc;      //x = time
    vec4 mouse;     //x = mouseX, y = mouseY, z = x delta, w = y delta
};

struct ComputeUniformBufferObject{
    float deltaTime;
};




struct SkyBoxVertex{
    vec3 pos;
    vec3 color;
    vec2 texCoord;
    u32   faceID;
};



struct test_vertex{
    vec3 pos;
    vec3 color;
};


//Z is +.99 to appear behind all other elements
const test_vertex test_vertices[] = {
    {{-0.5f, -0.5f,  0.999f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.999f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.999f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.999f}, {1.0f, 1.0f, 1.0f}},
};



const u32 test_indices[] = {
    2, 1, 0, 2, 0, 3,
};



static LineVertex axisVertices[] =
{
        { {-100.f, 0.f,   0.f },    0xFF0000FF  }, //x axis
        { { 100.f, 0.f,   0.f },    0xFF0000FF  },
        { { 0.f,  -100.f, 0.f },    0xFFFF0000  }, //y axis
        { { 0.f,   100.f, 0.f },    0xFFFF0000  },
        { { 0.f,   0.f,  -100.f },  0xFF00FF00  }, //z axis
        { { 0.f,   0.f,   100.f },  0xFF00FF00  },
        { {-1.f,  -1.f,   0.f }, 0xFF800080  },//purple, goes from -1 to 1 y at x = -1
        { {-1.f,   1.f,   0.f }, 0xFF800080  },
        { { 1.f,  -1.f,   0.f }, 0xFF800080  },//purple, goes from -1 to 1 y at x = +1
        { { 1.f,   1.f,   0.f }, 0xFF800080  },
        { { -1.f,  1.f,   0.f },  0xFFFF7F00}, //orange
        { {  1.f,  1.f,   0.f },  0xFFFF7F00},
        { { -1.f, -1.f,   0.f },  0xFFFF7F00},
        { {  1.f, -1.f,   0.f },  0xFFFF7F00},
};


static LineVertex testCubeVertices[] =       
{      
    {{-0.5f, -0.5f,  0.5f}, 0xFF0000FF}, // Vertex 0
    {{ 0.5f, -0.5f,  0.5f}, 0xFFFF0000}, // 1
    {{ 0.5f,  0.5f,  0.5f}, 0xFFFF0000}, // 2
    {{-0.5f,  0.5f,  0.5f}, 0xFFFF0000}, // 3
    {{-0.5f, -0.5f, -0.5f}, 0xFFFF0000}, // 4
    {{ 0.5f, -0.5f, -0.5f}, 0xFFFF0000}, // 5
    {{ 0.5f,  0.5f, -0.5f}, 0xFF00FF00}, // 6
    {{-0.5f,  0.5f, -0.5f}, 0xFFFF0000}  // 7
};

static LineVertex directionVertices[] = {
    { {0.f, 0.f,  0.f },   0xFF00FF00 }, //z axis
    { {0.f, 0.f,  1.f },   0xFF00FF00 }, //green

    { {0.f, 0.f, 0.f },   0xFFFF0000 }, //y axis
    { {0.f, 1.f, 0.f },   0xFFFF0000 }, //blue

    { {0.f, 0.f, 0.f },   0xFF0000FF }, //x axis
    { {1.f, 0.f, 0.f },   0xFF0000FF }, //red
};

static uint32_t directionLineIndices[] =
{
    0, 1, 2, 3, 4, 5,
};


static SkyBoxVertex texturedCubeVertices[] =       
{      
    //front
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0, 0}, {0}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1, 0}, {0}}, 
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1, 1}, {0}}, 
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0, 1}, {0}}, 
    //back (+z is back)
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1, 1}, {1}}, 
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0, 1}, {1}}, 
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0, 0}, {1}}, 
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1, 0}, {1}},  
    //top
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0, 0}, {2}}, 
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1, 0}, {2}}, 
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1, 1}, {2}}, 
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0, 1}, {2}},  
    //bottom
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0, 0}, {3}}, 
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1, 0}, {3}}, 
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1, 1}, {3}}, 
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0, 1}, {3}},  
    //left
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0, 0}, {4}}, 
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1, 0}, {4}}, 
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1, 1}, {4}}, 
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0, 1}, {4}},  
    //right
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0, 0}, {5}}, 
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1, 0}, {5}}, 
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1, 1}, {5}}, 
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0, 1}, {5}},  
};

static uint32_t texturedCubeIndices[]=
{//36 indices
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7,
    8,  9,  10, 8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};

static uint32_t testCubeIndices[]=
{//36 indices
    0, 1, 2, 0, 2, 3,
    1, 5, 6, 1, 6, 2,
    5, 4, 7, 5, 7, 6,
    4, 0, 3, 4, 3, 7,
    3, 2, 6, 3, 6, 7,
    4, 5, 1, 4, 1, 0,
};

static LineVertex lineVertices[] = {
    {{    0.0f,  0.0f,  0.0f}, 0xFF0000FF}, 
    {{ 1000.0f,  0.0f,  0.0f}, 0xFFFF0000}, 
};
static uint32_t lineIndices[] = {
    0, 1,
};

static uint32_t testCubeLineIndices[]=
{//24 indices
    //to draw a cube with lines
    0, 1,  // Edge from p1 to p2
    1, 2,  // Edge from p2 to p3
    2, 3,  // Edge from p3 to p4
    3, 0,  // Edge from p4 to p1
    4, 5,  // Edge from p5 to p6
    5, 6,  // Edge from p6 to p7
    6, 7,  // Edge from p7 to p8
    7, 4,  // Edge from p8 to p5
    0, 4,  // Edge from p1 to p5
    1, 5,  // Edge from p2 to p6
    2, 6,  // Edge from p3 to p7
    3, 7   // Edge from p4 to p8
};


static u32 axisLineIndices[] =
{//14 indices
    0, 1, 2, 3, 4, 5, 6, 7 , 8, 9, 10, 11, 12, 13,
};

const Vertex tri_vertices[] = {
    {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f},},
    {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f},},
    {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f},},
    {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f},},
};

const u32 indices[] = {
    0, 1, 2, 2, 3, 0,
};



static VkVertexInputBindingDescription getBindingDescription(u32 binding, size_t stride){
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = binding; //index of the binding in the array of bindings
    bindingDescription.stride = stride; //number of bytes from one entry to the next
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //move to the next data entry after each vertex
    //VK_VERTEX_INPUT_RATE_INSTANCE //Move to the next data entry after each instance, for instanced rendering

    return bindingDescription;
}

#define HANDMADE_OFFSETOF(st, m) \
    ((size_t)((char *)&((st *)0)->m - (char *)0))

static void getAttributeDescriptions(VkVertexInputAttributeDescription* attributeDescriptions){
    PRINT("vertex stride: %zu\n", sizeof(Vertex));

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = HANDMADE_OFFSETOF(Vertex, pos);
    PRINT("offset of pos in Vertex: %zu\n", HANDMADE_OFFSETOF(Vertex, pos));
    

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = HANDMADE_OFFSETOF(Vertex, color);
    PRINT("offset of color in Vertex: %zu\n", HANDMADE_OFFSETOF(Vertex, color));

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = HANDMADE_OFFSETOF(Vertex, texCoord);
    PRINT("offset of texCoord in Vertex: %zu\n", HANDMADE_OFFSETOF(Vertex, texCoord));
}

static void setAttributeDescriptions(VkVertexInputAttributeDescription* attributeDescriptions, size_t description_count, u32 binding,  VkFormat* formats, size_t* offsets){
    for(size_t i = 0; i < description_count; i++){
        attributeDescriptions[i].binding = binding;
        attributeDescriptions[i].location = i;
        attributeDescriptions[i].format = formats[i];
        attributeDescriptions[i].offset = offsets[i];
    
    }
}
/*
attributeDescriptions.format can be
float: VK_FORMAT_R32_SFLOAT
vec2:  VK_FORMAT_R32G32_SFLOAT
vec3:  VK_FORMAT_R32G32B32_SFLOAT
vec4:  VK_FORMAT_R32G32B32A32_SFLOAT
*/


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger){
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr){
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }else{
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator){
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr){
        func(instance, debugMessenger, pAllocator);
    }
}

struct vertex_hash_entry{
    uvec3 indices;
    u32 index;
    b32 occupied;
};

struct TextureInfo{
    int texWidth;
    int texHeight;
    int texChannels;
    u32 mipLevels;
    VkImage textureImage;
    VkImageView textureImageView;
    VkDeviceMemory textureImageMemory;
};


static inline size_t alignUp(size_t v, size_t a) {return (v + (a-1)) & ~(a-1);};


struct vkTri{
    VkInstance instance;
    const char* extensions[MAX_EXTENSIONS];
    u32 extension_count;

      
    FILETIME skyboxVertWriteTime;
    FILETIME skyboxFragWriteTime;
    b32 skybox_shader_loaded;
    char reloadLockPath[256];

    FILETIME screenTestVertWriteTime;
    FILETIME screenTestFragWriteTime;
    b32 screen_test_shader_loaded;

    VkDebugUtilsMessengerEXT debugMessenger;

    //destroyed when VkInstance is destroyed
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physDeviceProperties;
    size_t                     required_chunk_ssbo_memory;
    size_t                     single_chunk_size;
    u32                        chunk_draw_distance_volume;
    // FaceData                   face_update_buffer[MAX_CORES * MAX_FACES];

    //chunk staging buffer
    size_t chunkStagingPerFrameSize;
    size_t chunkStagingSizeTotal;

    VkBuffer chunkStagingBuffer;
    VkDeviceMemory chunkStagingMemory;
    void* chunkStagingMapped;

    VkBuffer chunkDeviceSSBO;//device local ssbo used for rendering
    VkDeviceMemory chunkDeviceSSBOMemory;

    bool waitForTransfer;

    VkCommandPool transferCommandPool;
    // VkCommandBuffer transferCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    VkCommandBuffer transferCommandBuffers[CHUNK_STAGING_SLICES];
    VkFence transferFences[MAX_FRAMES_IN_FLIGHT];
    VkFence chunkStageFences[CHUNK_STAGING_SLICES];
    u32 chunkStageCurrentSlice;
    
    u32 voxelSSBOFreeIndices[MAX_CHUNK_DRAW_DISTANCE];
    u32 voxelSSBOFreeCount;
    u32 activeChunkSSBOs;
    u32 voxelSSBOActiveIndexCount;

    VkDevice logicalDevice;
    
    VkQueryPool timestampQueryPool[MAX_FRAMES_IN_FLIGHT];
    float timestampPeriod;
    //handle to the logical device's graphics queue
    //created/destroyed with the logical device, we don't need to worry about them
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    //surface to draw it
    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;

    VkImage swapChainImages[MAX_SWAPCHAIN_IMAGES];
    VkImageView swapChainImageViews[MAX_SWAPCHAIN_IMAGES];
    u32 swapChainImageCount;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;
    VkRenderPass offscreen_render_pass;
    VkPipelineLayout        offscreen_pipeline_layout;
    VkPipeline              offscreen_pipeline;

    VkDescriptorSetLayout ubo_sampler_descriptor_set_layout;




    //      SCREEN SPACE RENDERING
    VkDescriptorSet  screen_space_text_descriptor_sets[MAX_FRAMES_IN_FLIGHT];
    
    VkDescriptorSet  screen_space_texture_descriptor_sets[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet  screen_space_test_descriptor_sets[MAX_FRAMES_IN_FLIGHT];

    VkDescriptorSetLayout   screen_space_descriptor_set_layout;
    VkDescriptorSetLayout   screen_space_font_descriptor_set_layout;
    VkDescriptorSetLayout   screen_space_texture_descriptor_set_layout;

    VkPipelineLayout        screen_space_text_pipeline_layout;
    VkPipeline              screen_space_text_pipeline;

    VkPipelineLayout        screen_space_texture_pipeline_layout;
    VkPipeline              screen_space_texture_pipeline;

    VkPipelineLayout        screen_space_test_pipeline_layout;
    VkPipeline              screen_space_test_pipeline;

    VkPipelineLayout        skeletal_mesh_pipeline_layout;
    VkPipeline              skeletal_mesh_pipeline;
    VkDescriptorSet         skeletal_mesh_descriptor_sets[MAX_FRAMES_IN_FLIGHT];


    VkDescriptorSetLayout   multi_texture_descriptor_set_layout;
    VkPipelineLayout        multi_texture_pipeline_layout;
    VkPipeline              multi_texture_pipeline;
    VkDescriptorSet         multi_texture_descriptor_sets[MAX_FRAMES_IN_FLIGHT];

    VkDescriptorSetLayout   test_ray_tracer_descriptor_set_layout;
    VkPipelineLayout        test_ray_tracer_pipeline_layout;
    VkDescriptorSet         test_ray_tracer_descriptor_sets[MAX_FRAMES_IN_FLIGHT];
    VkPipeline              test_ray_tracer_pipeline;

    VkDescriptorSetLayout   voxel_vertex_puller_descriptor_set_layout;
    VkPipelineLayout        voxel_vertex_puller_pipeline_layout;
    VkPipeline              voxel_vertex_puller_pipeline;
    VkDescriptorSet         voxel_vertex_puller_descriptor_sets[MAX_FRAMES_IN_FLIGHT];

    VkPipelineLayout        world_space_text_pipeline_layout;
    VkPipeline              world_space_text_pipeline;
    VkDescriptorSet         world_space_text_descriptor_sets[MAX_FRAMES_IN_FLIGHT];

    VkPipelineLayout        skybox_pipeline_layout;
    VkPipeline              skybox_pipeline;
    VkDescriptorSet         skybox_descriptor_sets[MAX_FRAMES_IN_FLIGHT];



    VkBuffer        screen_space_uniform_buffers        [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  screen_space_uniform_buffers_memory [MAX_FRAMES_IN_FLIGHT];
    void*           screen_space_uniform_buffers_mapped [MAX_FRAMES_IN_FLIGHT];

    TextureInfo screen_space_single_pixel_info[4];
    TextureInfo screen_space_texture_info;
    TextureInfo screen_space_font_info;
    TextureInfo windows_monospace_font_info;
    TextureInfo windows_font_info;

    u32             screen_space_mipLevels;

    VkBuffer        screenTextVertexBuffers       [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  screenTextVertexBuffersMemory [MAX_FRAMES_IN_FLIGHT];
    void*           screenTextVertexBuffersMapped [MAX_FRAMES_IN_FLIGHT];

    //END   SCREEN SPACE RENDERING

    //world space text

    VkBuffer        worldTextVertexBuffers       [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  worldTextVertexBuffersMemory [MAX_FRAMES_IN_FLIGHT];
    void*           worldTextVertexBuffersMapped [MAX_FRAMES_IN_FLIGHT];
    


    //end world space text


    u32      freeFaceMemoryIndices[MAX_CORES];
    u32      freeFaceMemoryCount;


    VkPipelineLayout    line_pipelineLayout;
    VkPipeline          graphics_line_pipeline;

    VkPipelineLayout    pipelineLayout; //used for uniforms
    VkPipeline          graphicsPipeline;
    
    VkDescriptorSetLayout ubo_ssbo_sampler_descriptor_set_layout;
    VkDescriptorSetLayout ubo_ssbo_descriptor_set_layout;
    VkDescriptorSetLayout ubo_descriptor_set_layout;
    VkPipelineLayout      default_entity_pipelineLayout;
    VkPipeline            default_entity_pipeline;

    VkPipelineLayout particle_pipelineLayout;
    VkPipeline       particle_pipeline;

    VkFramebuffer swapChainFramebuffers[MAX_SWAPCHAIN_IMAGES];
   
    VkFramebuffer offscreen_frame_buffer;

    VkCommandPool commandPool;
    // VkCommandPool transferCommandPool;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT]; //automatically freed when commandPool is destroyed

    VkCommandBuffer compute_commandBuffers[MAX_FRAMES_IN_FLIGHT]; //automatically freed when commandPool is destroyed

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore computeFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore transferCompleteSemaphores[MAX_FRAMES_IN_FLIGHT];

    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    VkFence computeInFlightFences[MAX_FRAMES_IN_FLIGHT];
    
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    //all the static meshes
    VkBuffer        entityMeshVertices              [MeshTypes::mesh_count];
    VkDeviceMemory  entityMeshVertexMemory          [MeshTypes::mesh_count];
    VkBuffer        entityMeshIndices               [MeshTypes::mesh_count];
    VkDeviceMemory  entityMeshIndexMemory           [MeshTypes::mesh_count];
    VkBuffer        entityMeshLineIndices           [MeshTypes::mesh_count];
    VkDeviceMemory  entityMeshLineIndexMemory       [MeshTypes::mesh_count];
    u32             entityMeshIndexCount            [MeshTypes::mesh_count];
    u32             entityMeshLineIndexCount        [MeshTypes::mesh_count];


    VkBuffer        skeletalMeshVertices              ;
    VkDeviceMemory  skeletalMeshVertexMemory          ;
    VkBuffer        skeletalMeshIndices               ;
    VkDeviceMemory  skeletalMeshIndexMemory           ;
    u32             skeletalMeshIndexCount            ;

    //do we even need each buffer with 2 frames in flight if we synchronize on the fence before the draw call?

    VkBuffer        uniformBuffers       [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  uniformBuffersMemory [MAX_FRAMES_IN_FLIGHT];
    void*           uniformBuffersMapped [MAX_FRAMES_IN_FLIGHT];

    VkBuffer        computeUniformBuffers       [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  computeUniformBuffersMemory [MAX_FRAMES_IN_FLIGHT];
    void*           computeUniformBuffersMapped [MAX_FRAMES_IN_FLIGHT];


    VkBuffer        shaderStorageBuffers       [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  shaderStorageBuffersMemory [MAX_FRAMES_IN_FLIGHT];
    void*           shaderStorageBuffersMapped [MAX_FRAMES_IN_FLIGHT];

    VkBuffer        perEntityShaderStorageBuffers       [MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  perEntityShaderStorageBuffersMemory [MAX_FRAMES_IN_FLIGHT];
    void*           perEntityShaderStorageBuffersMapped [MAX_FRAMES_IN_FLIGHT];

    
    VkBuffer        voxel_ssbo       ;
    VkDeviceMemory  voxel_ssbo_memory;
    void*           voxel_ssbo_mapped;

    //4718592 = 131072 voxels * 6 faces * 6 indices per face
    uint32_t faceIndices[4718592]; //shared between all buffers
    VkBuffer face_index_buffer;
    VkDeviceMemory face_index_buffer_memory;

    VkDescriptorPool ubo_sampler_ssbo_descriptor_pool;

    VkDescriptorSet graphics_descriptorSets[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet computeDescriptorSets[MAX_FRAMES_IN_FLIGHT];

    VkDescriptorSet  default_entity_descriptorSets[MAX_FRAMES_IN_FLIGHT];

    UniformBufferObject axis_ubo;

    UniformBufferObject ubo;
    ComputeUniformBufferObject compute_ubo;

    u32 mipLevels;

    VkSampler textureSampler;



    VkImage         depthImage;
    VkDeviceMemory  depthImageMemory;
    VkImageView     depthImageView;

    VkImage         colorImage;     //multisampling
    VkDeviceMemory  colorImageMemory;
    VkImageView     colorImageView;

    VkImage         offscreen_image;
    VkDeviceMemory  offscreen_image_memory;
    VkImageView     offscreen_image_view;

    VkImage         offscreen_depth_image;
    VkDeviceMemory  offscreen_depth_image_memory;
    VkImageView     offscreen_depth_image_view;

    Vertex model_vertices[12000];
    u32 model_indices[12000];
    u32 model_index_count;
    u32 model_vertex_count;

    vertex_hash_entry table[HASH_TABLE_SIZE][VERTEX_HASH_BUCKETS];
    u32 table_bucket_count[HASH_TABLE_SIZE];


    VkSampleCountFlagBits msaaSamples;

    particle particles[MAX_PARTICLES];

    TopologyTypes current_topology_type;
    MeshTypes current_mesh_type;

    chunk_voxel_ssbo_entry      chunkToSSBOMap          [MAX_CHUNKS];

    render_command_data RenderCommandData;
    
    


    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;

    TextureInfo model_texture_info;

    char* path;

    uint8_t fontBuffer[256 * 1024];   // 256KB // #define FONT_BUFFER_SIZE () 
    uint32_t fontBufferSize;
    uint8_t fontInfo[256];           // Big enough to hold stbtt_fontinfo
    float fontScale;                 // 0.015625 in your case
    uint8_t font_bitmap[512 * 512];

    //font info
    uint8_t charData[96 * 32];       // Raw storage for packed char data
    // uint8_t fontInfo[160];
    float ascent;
    float descent;
    float doubleDescent;
    float lineGap;
    float scale;
    float lineAdvance;
    float maxCharWidth;


    VkBuffer dynamic_texture_staging_buffer                 [MAX_DYNAMIC_TEXTURES][MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory dynamic_texture_staging_buffer_memory    [MAX_DYNAMIC_TEXTURES][MAX_FRAMES_IN_FLIGHT];
    void* dynamic_texture_staging_buffer_mapped             [MAX_DYNAMIC_TEXTURES][MAX_FRAMES_IN_FLIGHT];
    VkFence dynamic_texture_transfer_fence;
    uint8_t dynamic_texture_buffer[MAX_FRAMES_IN_FLIGHT][512 * 512 * 4];

    VkImage         dynamic_image       [MAX_DYNAMIC_TEXTURES];
    VkDeviceMemory  dynamic_image_memory[MAX_DYNAMIC_TEXTURES];
    VkImageView     dynamic_image_view  [MAX_DYNAMIC_TEXTURES];
    u32 numDynamicTextures;
    u32 curTexture;

    u32 currentFrame;   

    vec3 camPos;
    float elapsedTime;
    float deltaTime;
    u64 frameCount;
    u32 slowFrameCount;

    char frag_buffer[MAX_SHADER_SIZE];
    char vert_buffer[MAX_SHADER_SIZE];

    TestMesh sphereMesh;
    TestMesh capsuleMesh;
    TestMesh hemisphereMesh;


    u32 thorns[128*128];
    u32 cracked[64*64];
};




void GetWindowsPackedQuad(WindowsCharData* charData, int atlasSizeX, int atlasSizeY, int charIndex, float* xpos, float* ypos, WindowsAlignedQuad* quad, int align_to_integer){
    WindowsCharData* info = &charData[charIndex];

    if(align_to_integer){
        //round down position to nearest integer
        float ipx = (float)(int)(*xpos);
        float ipy = (float)(int)(*ypos);
        
        quad->x0 = ipx                             +  info->xoff                  ;
        quad->y0 = ipy                             -  info->yoff                  ;
        quad->x1 = ipx +                              info->xoff +    info->width ;
        quad->y1 = ipy -                              info->yoff +    info->height;
        int fuck_the_debugger = 0;                  //
    }else{                                          //
        //use exact floating point position         //  
        quad->x0 = *xpos                            + info->xoff                ;
        quad->y0 = *ypos                            - info->yoff                ;
        quad->x1 = *xpos +                            info->xoff +   info->width;
        quad->y1 = *ypos -                            info->yoff +   info->height;
        int fuck_the_debugger = 0;
    }

    //copy texture coords
    quad->s0 = info->s0;
    quad->t0 = info->t0;
    quad->s1 = info->s1;
    quad->t1 = info->t1;

    *xpos += info->xadvance;
    
    int fuck_the_debugger = 0;

}

// WINDOWS FONTS
#define USE_FONTS_FROM_WINDOWS 1

#ifdef _WIN32


void loadWindowsFont(vkTri* tri, font_data* FontData, const char* FileName, const char* fontName){
    HDC DeviceContext = 0;


    //drawing with a higher resolution gives us better anti aliasing
    u32 atlasSizeX = 1024;
    u32 atlasSizeY = 1024;
    u32 desiredFontSizePixels = 127;

    if(!DeviceContext){

        AddFontResourceExA(FileName, FR_PRIVATE, 0);
        HFONT Font = CreateFontA(desiredFontSizePixels, 0, 0, 0, 
                                FW_NORMAL,//weight 
                                FALSE, //italic
                                FALSE, //underline
                                FALSE,//strikeout 
                                DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, 
                                CLIP_DEFAULT_PRECIS, 
                                ANTIALIASED_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, 
                                fontName);



        
        DeviceContext = CreateCompatibleDC(0);

        BITMAPINFO bi = {0};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = 1024;
        bi.bmiHeader.biHeight = 1024;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* dibPixels = 0;
        HBITMAP bitmap = CreateDIBSection(DeviceContext, &bi, DIB_RGB_COLORS, &dibPixels, 0, 0);
        SelectObject(DeviceContext, bitmap);
        SelectObject(DeviceContext, Font);
        SetBkColor(DeviceContext, RGB(0,0,0));

        TEXTMETRIC textMetric;
        GetTextMetrics(DeviceContext, &textMetric);

        #define MIN_SUPPORTED_CHAR (32)  // Space
        #define MAX_SUPPORTED_CHAR (127) // Delete
        #define CHAR_RANGE (MAX_SUPPORTED_CHAR - MIN_SUPPORTED_CHAR + 1)

        DWORD KerningPairCount = GetKerningPairsW(DeviceContext, 0, 0);
        KERNINGPAIR* KerningPairs = (KERNINGPAIR*)plat_alloc_mem(KerningPairCount*sizeof(KERNINGPAIR));
        GetKerningPairsW(DeviceContext, KerningPairCount, KerningPairs);
        //extract kerning information
        int kernCount = 0;
        for(DWORD KerningPairIndex = 0; KerningPairIndex < KerningPairCount; KerningPairIndex++){
            KERNINGPAIR* pair = KerningPairs + KerningPairIndex;
                // Check if characters are in our supported range
            if(pair->wFirst >= MIN_SUPPORTED_CHAR && pair->wFirst <= MAX_SUPPORTED_CHAR &&
            pair->wSecond >= MIN_SUPPORTED_CHAR && pair->wSecond <= MAX_SUPPORTED_CHAR) {
                
                // Convert to our array indices
                u32 firstIndex = pair->wFirst - MIN_SUPPORTED_CHAR;
                u32 secondIndex = pair->wSecond - MIN_SUPPORTED_CHAR;
                FontData->fontKerningTable[((firstIndex) * 96) + (secondIndex)] = pair->iKernAmount;
                PRINT("Kerning: '%c'(%u) + '%c'(%u) = %d, total pairs: %d\n", pair->wFirst, pair->wFirst,pair->wSecond, pair->wSecond, pair->iKernAmount, ++kernCount);
            }
        }
        plat_dealloc_mem(KerningPairs, KerningPairCount*sizeof(KERNINGPAIR));

        float targetPixelHeight = (float)desiredFontSizePixels;
        // float scale = targetPixelHeight / 1024.0f;
        float scale = 0.25;

        // TODO: do we need to multiply everything by the scale? will this break stuff?
        FontData->scale = scale;
        FontData->ascent        = textMetric.tmAscent * scale;
        FontData->descent       = textMetric.tmDescent * scale;
        FontData->lineGap       = textMetric.tmExternalLeading * scale;
        FontData->doubleDescent = FontData->descent * 2.0f;
        FontData->lineAdvance   = (f32)textMetric.tmHeight + (f32)textMetric.tmExternalLeading;
        FontData->maxCharWidth  = textMetric.tmMaxCharWidth ;


        u32 atlasMemSize = atlasSizeX * atlasSizeY * 4;

        void* atlasMemory = plat_alloc_mem(atlasMemSize);
        memset(atlasMemory, 0, atlasMemSize);

        //allocate structure to store character data like (stbtt_packedchar)    
        WindowsCharData* charData = (WindowsCharData*)plat_alloc_mem(sizeof(WindowsCharData) * 96);
        memset(charData, 0, sizeof(WindowsCharData) * 96);

        memset(dibPixels, 0, sizeof(u32) * atlasSizeX * atlasSizeY);



        //abc widths will help with proper character spacing
        ABC abc; 

        //clear device context with black background
        PatBlt(DeviceContext, 0, 0, atlasSizeX, atlasSizeY, BLACKNESS);
        // SetBkMode(DeviceContext, TRANSPARENT);  // Transparent background

        //current position in the atlas
        u32 xCursor = 0;
        u32 yCursor = 0;
        u32 rowHeight = 0;

        u32 packxCursor = 0;
        u32 packyCursor = 0;
        u32 packrowHeight = 0;
        bool newRow = false;
        u32 tallestCharOnRow = 0;
        //get the specific char data/texcoords

        // Before drawing any text, fill with a pattern to confirm dibPixels is working
// for(int y = 0; y < atlasSizeY; y++) {
//     for(int x = 0; x < atlasSizeX; x++) {
//         ((u32*)dibPixels)[y * atlasSizeX + x] = 
//             ((x & 32) ^ (y & 32)) ? 0xFF404040 : 0xFF202020; // Checkerboard
//     }
// }
        // u8 r = 0xff;
        // u8 g = 0xff;
        // u8 b = 0xff;
        //extract each character and place it in the atlas
        for(u32 codePoint = 32; codePoint < 128; codePoint++){
            wchar_t charPoint = (wchar_t)codePoint;
            PRINT("charPoint: %c\n", charPoint);
            //for when I needed to test, I set every other char to red/white
            // if(codePoint & 1){
            //     g = 0x00;
            //     b = 0x00;
            // }
            // else{
            //     g = 0xff;
            //     b = 0xff;
            // } 
            GetCharABCWidthsW(DeviceContext, charPoint, charPoint, &abc);

            //needed to allow for the 'A' to not get clipped on the left, doubled in charWidth to prevent 'V' clipping on the right
            int prestepx = (abc.abcA < 0) ? -abc.abcA : 0;

            //measure the character size
            SIZE size;
            GetTextExtentPoint32W(DeviceContext, &charPoint, 1, &size);

            u32 charWidth  = size.cx + prestepx*2;
            u32 charHeight = size.cy;

            //check if we need to advance to the next row in atlas
            if(xCursor + charWidth + 2 > atlasSizeX){
                xCursor = 0;
                yCursor += rowHeight + 2; //add padding between rows
                rowHeight = 0;
            }


            //make sure we have room in the atlas
            if(yCursor + charHeight + 2 > atlasSizeY){
                //atlas is full
                break;
            }
            SetTextColor(DeviceContext, RGB(255, 255, 255));  // White text color

            //draw the character to the atlas at the current position
            TextOutW(DeviceContext, xCursor + prestepx, yCursor, &charPoint, 1);

            //find character bounds/trim empty space
            s32 minx = 10000;
            s32 miny = 10000;
            s32 maxx = -10000;
            s32 maxy = -10000;

            for(s32 y = 0; y < charHeight; y++){
                u32* row = (u32*)dibPixels + (atlasSizeY - 1 - (yCursor + y)) * atlasSizeX;
                for(s32 x = 0; x < charWidth; x++){
                    if(row[xCursor + x] != 0){
                        if(minx > x)minx = x;
                        if(miny > y)miny = y;
                        if(maxx < x)maxx = x;
                        if(maxy < y)maxy = y;
                    }
                }
            }



    if(minx < maxx){
        if(minx > 0) --minx;
        if(miny > 0) --miny;
        if(maxx < atlasSizeX - 1) ++maxx;
        if(maxy < atlasSizeY - 1) ++maxy;
        int width  = (maxx - minx) + 1;
        int height = (maxy - miny) + 1;

        //check if we need to advance to the next row in atlas
        if(packxCursor + width > atlasSizeX){
            packxCursor = 0;
            packyCursor += packrowHeight;
            packrowHeight = 0;
        }

        for(s32 y = 0; y <= height; y++) {
            u32* sourceRow = (u32*)dibPixels + (atlasSizeY - 1 - (yCursor +miny + y)) * atlasSizeX;
            u32* destRow   = (u32*)FontData->Bitmap + (packyCursor + y) * atlasSizeX;

            for(s32 x = 0; x <= width; x++) {
                //ignore minx and miny, those are for the dib padding

                u32 srcPixel = sourceRow[xCursor +minx+ x];

                // u8 srcr = (srcPixel >> 16) & 0xff;
                // u8 srcg = (srcPixel >> 8) & 0xff;
                // u8 srcb = srcPixel & 0xff;

                // u8 gray  = (u8)((srcr + srcg + srcb) / 3);

                // u8 alpha = (gray > 0) ? 0xFF : 0;

                u8 gray = (u8)(srcPixel & 0xFF);
                u8 alpha = gray;
                // u8 alpha = (gray > 30) ? 0xFF : 0;
                // u8 alpha = gray;  // <- this preserves antialiasing
                // destRow[packxCursor + x] = (alpha << 24) | (gray << 16) | (gray << 8) | gray;
                // u8 r = 0xff;
                // u8 g = 0xff;
                // u8 b = 0xff;
                #define DRAW_FONT_BORDERS 0
                #if DRAW_FONT_BORDERS
                //draw border to visualize where we're drawing
                if(y == 0 || y == height-1 || y == 1 || y == height-2 || x == 0 || x == 1 || x == width - 1 || x == width-2){
                    destRow[packxCursor + x] = 0xFF0000FF;
                }else{                
                    destRow[packxCursor + x] = (alpha << 24) | (gray << 16) | (gray << 8) | (gray << 0);
                }
                #else
                    destRow[packxCursor + x] = (alpha << 24) | (gray << 16) | (gray << 8) | (gray << 0);
                #endif
                // tri->windowsFontBitmap[(packyCursor + y) * atlasSizeX + (packxCursor + x)] = 0xffffffff;
            }
        }

        //store character information here
        WindowsCharData newChar = {};
        newChar.x0 = (f32)(packxCursor);
        newChar.y0 = (f32)(packyCursor);
        newChar.x1 = (f32)(packxCursor + (maxx - minx) + 1);
        newChar.y1 = (f32)(packyCursor + (maxy - miny) + 1);
        newChar.s0 = (f32)newChar.x0 / (f32)atlasSizeX;
        newChar.t0 = (f32)newChar.y0 / (f32)atlasSizeY;
        newChar.s1 = (f32)newChar.x1 / (f32)atlasSizeX;
        newChar.t1 = (f32)newChar.y1 / (f32)atlasSizeY;
        newChar.xadvance = (f32) abc.abcA + abc.abcB + abc.abcC;
        // newChar.xadvance = (f32) width;

        // newChar.xadvance = (f32)0;
        newChar.xoff = (f32)minx - prestepx;
        // newChar.xoff = (f32)abc.abcA;
        newChar.yoff = (f32)(textMetric.tmAscent - miny);
        
        newChar.width = (maxx - minx);
        newChar.height = (maxy - miny);

        u32 entry = codePoint - 32;
        FontData->windowsCharData[entry] = newChar;


        f32 alignPercentX = (1.0f) / (f32)width;
        f32 alignPercentY = (1.0f + (maxy - (size.cy - textMetric.tmDescent))) / (f32)height;

        //update position and row height for next character
        packxCursor += width + 5;//add padding between characters
        if(height + 5 > packrowHeight){
            packrowHeight = height + 5;
        }

    }
    //update position and row height for next character
    xCursor += charWidth + 2;//add padding between characters
    if(charHeight > rowHeight){
        rowHeight = charHeight;
    }
}

        //store the atlas and character data in vkTri structure
        // memcpy(tri->windowsFontBitmap, dibPixels, sizeof(u32) * 512 * 512);
        // memset(tri->windowsFontBitmap, 0, sizeof(u32) * 512 * 512);
        // memcpy(tri->windowsFontBitmap, result.memory, result.height * result.pitch);
        // memcpy(tri->windowsCharData, charData, sizeof(WindowsCharData) * 96);

        DeleteObject(Font);
        DeleteObject(bitmap);
        DeleteDC(DeviceContext);

        plat_dealloc_mem(atlasMemory, atlasMemSize);
        plat_dealloc_mem(charData, sizeof(WindowsCharData) * 96);
    }

    


}

#endif




//END WINDOWS FONTS


void worldSpaceText(vkTri* tri , VkCommandBuffer commandBuffer){
    render_command_data* rcd = &tri->RenderCommandData;
    u32& charCount = rcd->world_transient_char_count;

for(u32 i = 0; i < rcd->worldTextDrawCount; i++){
        char* text = rcd->world_transient_char_buffer + rcd->world_transient_char_offsets[i];

        if(handmade_strlen(text) <= 0)return;
        int msglen = handmade_strlen(text) + 1;

        // const WindowsCharData* chars = (const WindowsCharData*)tri->windowsCharData;
        const stbtt_packedchar* chars = (const stbtt_packedchar*)tri->charData;
        // const stbtt_packedchar* chars = (const stbtt_packedchar*)tri->windowsCharData;
        
        // TextVertex vertices[6*1024] = {};

        int vertexCount = 0;
        
        float startX = 0;
        float startY = 0;
        char prevChar = 0;
        // text_input->cursor_num_positions = 0;
        // text_input->cursor_positions[text_input->cursor_num_positions++] = {startX, startY}; 


        //do we need these if we're just rendering?
        float widthx = 0;
        float widthy = 0;
        int count = 0;
        for (const char* p = text; *p; p++) {
            if(count > msglen || count >= MAX_TEXT_CHARS){
                PRINT("count: %d is greater than MAX_TEXT_CHARS: %d, hardcoded max: %d\n", count, MAX_TEXT_CHARS, MAX_TEXT_CHARS); 
                break;
            }   
            
            if (*p == '\n') {
                startX = 0;
                startY -= tri->doubleDescent;
                prevChar = 0;
                // text_input->cursor_positions[text_input->cursor_num_positions++] = {startX, startY}; 
                continue; // skip normal drawing
            }

            int charIndex = *p - 32;
            if (charIndex < 0 || charIndex >= 96) continue;



            if (prevChar) {
                int kern = stbtt_GetCodepointKernAdvance((stbtt_fontinfo*)tri->fontInfo, prevChar, *p);
                startX += kern * tri->scale;
            }

            stbtt_aligned_quad quad;
            stbtt_GetPackedQuad(chars, 512, 512,
                            charIndex,
                            &startX, &startY,
                            &quad,
                            1);  // 1 = opengl style coords

            // text_input->cursor_positions[text_input->cursor_num_positions++] = {startX, startY}; 
            //world space text needs to be flipped back around, probably because of the projection matrix so it isnt back face culled??
            rcd->world_transient_text[rcd->world_transient_text_vertex_count++] = {{quad.x0, quad.y1, 0}, {quad.s0, quad.t1}};
            rcd->world_transient_text[rcd->world_transient_text_vertex_count++] = {{quad.x1, quad.y1, 0}, {quad.s1, quad.t1}};
            rcd->world_transient_text[rcd->world_transient_text_vertex_count++] = {{quad.x1, quad.y0, 0}, {quad.s1, quad.t0}};
            
            rcd->world_transient_text[rcd->world_transient_text_vertex_count++] = {{quad.x0, quad.y1, 0}, {quad.s0, quad.t1}};
            rcd->world_transient_text[rcd->world_transient_text_vertex_count++] = {{quad.x1, quad.y0, 0}, {quad.s1, quad.t0}};
            rcd->world_transient_text[rcd->world_transient_text_vertex_count++] = {{quad.x0, quad.y0, 0}, {quad.s0, quad.t0}};
            
            
            rcd->world_transient_text_entry_vertex_count[rcd->world_transient_text_entries] += 6;

            widthx = quad.x1;
            widthy = quad.y1 - quad.y0;

            prevChar = *p;
            count++;
        }
        rcd->world_transient_char_widths [rcd->world_transient_text_entries] = widthx;
        rcd->world_transient_char_heights[rcd->world_transient_text_entries] = widthy - startY;

        size_t next_offset = rcd->world_transient_text_entry_vertex_count[rcd->world_transient_text_entries];
        rcd->world_transient_text_entries++;
        rcd->world_transient_text_entry_offset[rcd->world_transient_text_entries] = next_offset;
                            
    }
    
    VkBuffer worldTextVertexBuffers[] = {tri->worldTextVertexBuffers[tri->currentFrame]};

    memcpy(tri->worldTextVertexBuffersMapped[tri->currentFrame], rcd->world_transient_text, sizeof(TextVertex) * rcd->world_transient_text_vertex_count);


    VkDeviceSize world_offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, worldTextVertexBuffers, world_offsets);

    u32 cumulative_offset = 0;
    for(u32 i = 0; i < rcd->worldTextDrawCount; i++){
            
        WorldTextPushConstants world_text_push_constants = {};
        world_text_push_constants = rcd->worldTextDrawCommandsSSBO[i];
        world_text_push_constants.model.m[12] -= (rcd->world_transient_char_widths  [i] * 0.5f) * world_text_push_constants.scale;
        world_text_push_constants.model.m[13] -= (rcd->world_transient_char_heights [i] * 0.5f) * world_text_push_constants.scale;

        vkCmdPushConstants(commandBuffer, tri->world_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(WorldTextPushConstants), &world_text_push_constants);

        // vkCmdDraw(commandBuffer, (u32)num to draw, 1, offset in the vertex array, 0);

        vkCmdDraw(commandBuffer, (u32)rcd->world_transient_text_entry_vertex_count[i], 1, cumulative_offset, 0);
        cumulative_offset += rcd->world_transient_text_entry_vertex_count[i];
        rcd->world_transient_text_entry_vertex_count[i] = 0;
    }

    

    rcd->world_transient_text_entries = 0;
    rcd->world_transient_text_vertex_count = 0;
    rcd->world_transient_char_count = 0;
    rcd->worldTextDrawCount = 0;
}



u32 getFontKerning(vkTri* tri, int currChar, int prevChar){
    // return tri->fontKerningTable[currChar * prevChar];
    return 80;
}



void screenSpaceTextCreateQuadsMonospace(vkTri* tri , font_data* FontData, char* text, int msglen, float& widthx, float& widthy, float& startX, float& startY){
    int vertexCount = 0;
    char prevChar = 0;
    int count = 0;
    float monospaceAdvance = FontData->maxCharWidth; // Fixed width for all glyphs

    for (const char* p = text; *p; p++) {
        if(count > msglen || count >= MAX_TEXT_CHARS){
            PRINT("count: %d is greater than MAX_TEXT_CHARS: %d, hardcoded max: %d\n", count, MAX_TEXT_CHARS, MAX_TEXT_CHARS); 
            break;
        }   
        
        if (*p == '\n') {
            startX = 0;
            startY += (FontData->lineAdvance * 0.75f);
            prevChar = 0;
            //need to draw quads for newline and spaces for text highlighting to work
            //otherwise vertex count will be incorrect and we will draw garbage data and chug the performance
        }


        // if (*p == ' ') {
            // startX += tri->maxCharWidth; //hardcoded, looks good enough for our scaled down Arial font, need a better way eventually
            // startX += FontData->maxCharWidth; //hardcoded, looks good enough for our scaled down Arial font, need a better way eventually
            // prevChar = 0;
        // }

        int charIndex = *p - 32;

        WindowsAlignedQuad quad = {};

        //need to make 6 vertices for each char, including newlines/spaces, so that highlighting works
        // if (charIndex < 0 || charIndex >= 96) continue;
        if (charIndex < 0 || charIndex >= 96){
            monospaceAdvance = 0; // Fixed width for all glyphs
            
        }else{
            WindowsCharData* info = &FontData->windowsCharData[charIndex];
            monospaceAdvance = FontData->maxCharWidth; // Fixed width for all glyphs
            float cellPadding = (monospaceAdvance - info->width) * 0.5f;

            // round to pixel
            float ipx = (float)(int)(startX + cellPadding);
            float ipy = (float)(int)(startY);

            quad.x0 = ipx;
            quad.y0 = ipy - info->yoff;
            quad.x1 = ipx + info->width;
            quad.y1 = ipy - info->yoff + info->height;

            quad.s0 = info->s0;
            quad.t0 = info->t0;
            quad.s1 = info->s1;
            quad.t1 = info->t1;
        }





        startX += monospaceAdvance;

        // text_input->cursor_positions[text_input->cursor_num_positions++] = {startX, startY}; 
        //screen space text needs to be flipped back around, probably because of the projection matrix so it isnt back face culled??
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x0, quad.y1, 0}, {quad.s0, quad.t1}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x1, quad.y1, 0}, {quad.s1, quad.t1}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x1, quad.y0, 0}, {quad.s1, quad.t0}};
        
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x0, quad.y1, 0}, {quad.s0, quad.t1}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x1, quad.y0, 0}, {quad.s1, quad.t0}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x0, quad.y0, 0}, {quad.s0, quad.t0}};
        
        
        tri->RenderCommandData.screen_transient_text_entry_vertex_count[tri->RenderCommandData.screen_transient_text_entries] += 6;

        widthx = quad.x1;
        widthy = quad.y1 - quad.y0;

        prevChar = *p;
        count++;
    }

       
}

void screenSpaceTextCreateQuads(vkTri* tri , font_data* FontData, char* text, int msglen, float& widthx, float& widthy, float& startX, float& startY){
    int vertexCount = 0;
    char prevChar = 0;
    int count = 0;
    for (const char* p = text; *p; p++) {
        if(count > msglen || count >= MAX_TEXT_CHARS){
            PRINT("count: %d is greater than MAX_TEXT_CHARS: %d, hardcoded max: %d\n", count, MAX_TEXT_CHARS, MAX_TEXT_CHARS); 
            break;
        }   
        
        if (*p == '\n') {
            startX = 0;
            startY += (FontData->lineAdvance * 0.75f);
            prevChar = 0;
            // text_input->cursor_positions[text_input->cursor_num_positions++] = {startX, startY}; 
            continue; // skip normal drawing
        }


        if (*p == ' ') {
            startX += 32; //hardcoded, looks good enough for our scaled down Arial font, need a better way eventually
            prevChar = 0;
            continue; // skip normal drawing
        }


        int charIndex = *p - 32;
        if (charIndex < 0 || charIndex >= 96) continue;


        int prevCharIndex = prevChar - 32;
        if (prevCharIndex >= 0 && prevCharIndex < 96) {
            // int kern = stbtt_GetCodepointKernAdvance((stbtt_fontinfo*)tri->fontInfo, prevChar, *p);
            // startX += getFontKerning(tri, charIndex, prevCharIndex);
            startX += FontData->fontKerningTable[((prevCharIndex) * 96) + (charIndex)];
            // startX += kern * tri->scale;
        }

        // stbtt_aligned_quad quad;
        // stbtt_GetPackedQuad(chars, 512, 512,
        //                 charIndex,
        //                 &startX, &startY,
        //                 &quad,
        //                 1);  // 1 = opengl style coords

        WindowsAlignedQuad quad = {};
        GetWindowsPackedQuad(FontData->windowsCharData, 1024, 1024, charIndex, &startX, &startY, &quad, 1);


        // text_input->cursor_positions[text_input->cursor_num_positions++] = {startX, startY}; 
        //screen space text needs to be flipped back around, probably because of the projection matrix so it isnt back face culled??
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x0, quad.y1, 0}, {quad.s0, quad.t1}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x1, quad.y1, 0}, {quad.s1, quad.t1}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x1, quad.y0, 0}, {quad.s1, quad.t0}};
        
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x0, quad.y1, 0}, {quad.s0, quad.t1}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x1, quad.y0, 0}, {quad.s1, quad.t0}};
        tri->RenderCommandData.screen_transient_text[tri->RenderCommandData.screen_transient_text_vertex_count++] = {{quad.x0, quad.y0, 0}, {quad.s0, quad.t0}};
        
        
        tri->RenderCommandData.screen_transient_text_entry_vertex_count[tri->RenderCommandData.screen_transient_text_entries] += 6;

        widthx = quad.x1;
        widthy = quad.y1 - quad.y0;

        prevChar = *p;
        count++;
    }

       
}

void createScreenTextVertices(vkTri* tri){
    TIMED_BLOCK("Create Screen Text Vertices");

    //start the buffer with ui element vertices

    memcpy(tri->RenderCommandData.screen_transient_text,  box_vertices, sizeof(TextVertex) * 6);
    tri->RenderCommandData.screen_transient_text_vertex_count += 6;
    // tri->RenderCommandData.screen_transient_text_entry_vertex_count[tri->RenderCommandData.screen_transient_text_entries] += 6;
    // tri->RenderCommandData.screen_transient_text_entries++;


    for(u32 i = 0; i < tri->RenderCommandData.screenElementDrawCount; i++){

        if(tri->RenderCommandData.screenElementDrawCommands[i].push.misc.y != 6)continue;

        char* text = tri->RenderCommandData.screen_transient_char_buffer + tri->RenderCommandData.screen_transient_char_offsets[tri->RenderCommandData.screen_transient_text_entries];
        float widthx = 0;
        float widthy = 0;
        float startX = 0;
        float startY = 0;

        if(handmade_strlen(text) <= 0)return;
        
        int msglen = handmade_strlen(text) + 1;
        
        //eventually use different paths for different text to be drawn
        //this is the default path, text editor needs to use one with a different approach for monospace fonts
        if(tri->RenderCommandData.screenElementDrawCommands[i].push.misc.z == 5){
            screenSpaceTextCreateQuadsMonospace(tri, &tri->RenderCommandData.monospacedScreenFont, text, msglen, widthx, widthy, startX, startY);
        }else{
            screenSpaceTextCreateQuads(tri, &tri->RenderCommandData.defaultScreenFont, text, msglen, widthx, widthy, startX, startY);
        }

        tri->RenderCommandData.screen_transient_char_widths [tri->RenderCommandData.screen_transient_text_entries] = widthx;
        tri->RenderCommandData.screen_transient_char_heights[tri->RenderCommandData.screen_transient_text_entries] = widthy - startY;

        size_t next_offset = tri->RenderCommandData.screen_transient_text_entry_vertex_count[tri->RenderCommandData.screen_transient_text_entries];
        tri->RenderCommandData.screen_transient_text_entries++;
        tri->RenderCommandData.screen_transient_text_entry_offset[tri->RenderCommandData.screen_transient_text_entries] = next_offset;

    }
    
}




void drawCombinedScreenElementsAndText(vkTri* tri , VkCommandBuffer commandBuffer){
    TIMED_BLOCK("Draw Screen Elements & Text");
    u32& charCount = tri->RenderCommandData.screen_transient_char_count;
    
    VkBuffer screenTextVertexBuffers[] = {tri->screenTextVertexBuffers[tri->currentFrame]};
    
    memcpy(tri->screenTextVertexBuffersMapped[tri->currentFrame], tri->RenderCommandData.screen_transient_text, sizeof(TextVertex) * tri->RenderCommandData.screen_transient_text_vertex_count);
    
    VkDeviceSize screen_offsets[] = {0};

    //start at 6 to account for filling the beginning with an empty quad for ui elements
    //this allows us to get around the vertex bind cost
    u32 cumulative_offset = 6;
    u32 textDrawCount = 0; 

    size_t& count = tri->RenderCommandData.screenElementDrawCount;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_test_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_test_pipeline_layout, 0, 1, &tri->screen_space_test_descriptor_sets[tri->currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenTextVertexBuffers, screen_offsets);

    for(u32 i = 0; i < count; i++){
            


        ScreenPushConstants screen_push_constants = {};
        screen_push_constants = tri->RenderCommandData.screenElementDrawCommands[i].push;

        switch(tri->RenderCommandData.screenElementDrawCommands[i].push.misc.y){

              case 6:{//text
                // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline);
                // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline_layout, 0, 1, &tri->screen_space_text_descriptor_sets[tri->currentFrame], 0, nullptr);
                

                 
                if(screen_push_constants.misc.x){
                        VkRect2D scissor = {};
                        scissor.offset = {(int)tri->RenderCommandData.screenElementDrawCommands[i].scissor.x, (int)tri->RenderCommandData.screenElementDrawCommands[i].scissor.y};
                        scissor.extent = {(u32)tri->RenderCommandData.screenElementDrawCommands[i].scissor.z, (u32)tri->RenderCommandData.screenElementDrawCommands[i].scissor.w};
                        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                }

                screen_push_constants.scale = screen_push_constants.scale * 0.25;

                //center the text
                if(tri->RenderCommandData.screenElementDrawCommands[i].center_text){
                    screen_push_constants.position.x -= (tri->RenderCommandData.screen_transient_char_widths  [textDrawCount] * 0.5f) * (screen_push_constants.scale.x) ;
                    screen_push_constants.position.y += (tri->RenderCommandData.screen_transient_char_heights [textDrawCount] * 0.5f) * (screen_push_constants.scale.y) ;
                }
                
                vec2 origPos = screen_push_constants.position;
                screen_push_constants.color = {0.5, 0.5, 0.5, 0.5};
                screen_push_constants.position.x += (5 * 0.25);
                screen_push_constants.position.y += (5 * 0.25);

                vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
                vkCmdDraw(commandBuffer, (u32)tri->RenderCommandData.screen_transient_text_entry_vertex_count[textDrawCount], 1, cumulative_offset, 0);


                screen_push_constants.color = tri->RenderCommandData.screenElementDrawCommands[i].push.color;
                screen_push_constants.position = origPos;

                vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
                vkCmdDraw(commandBuffer, (u32)tri->RenderCommandData.screen_transient_text_entry_vertex_count[textDrawCount], 1, cumulative_offset, 0);


                //only works for monospace fonts for now
                if(tri->RenderCommandData.screenElementDrawCommands[i].misc.x == 1){//draw highlighted text
                    
                    screen_push_constants.color = tri->RenderCommandData.screenElementDrawCommands[i].miscColor;
                    u32 vertCount = (tri->RenderCommandData.screenElementDrawCommands[i].misc.z * 6);
                    u32 vertStart = cumulative_offset + (tri->RenderCommandData.screenElementDrawCommands[i].misc.y * 6);
                    screen_push_constants.position.x -= (5 * 0.25);
                    screen_push_constants.position.y -= (5 * 0.25);

                    vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
                    vkCmdDraw(commandBuffer, vertCount, 1, vertStart, 0);

                }

                cumulative_offset += tri->RenderCommandData.screen_transient_text_entry_vertex_count[textDrawCount];
                tri->RenderCommandData.screen_transient_text_entry_vertex_count[textDrawCount] = 0;

                //reset cliprect
                if(screen_push_constants.misc.x){
                    VkRect2D scissor = {};
                    scissor.offset = {0, 0};
                    scissor.extent = tri->swapChainExtent;
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                }
                
                textDrawCount++;
            }break;

            default:{//element

                // vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[MeshTypes::mesh_uiBox], screen_offsets);
                
                if(screen_push_constants.misc.x){
                        VkRect2D scissor = {};
                        scissor.offset = {(int)tri->RenderCommandData.screenElementDrawCommands[i].scissor.x, (int)tri->RenderCommandData.screenElementDrawCommands[i].scissor.y};
                        scissor.extent = {(u32)tri->RenderCommandData.screenElementDrawCommands[i].scissor.z, (u32)tri->RenderCommandData.screenElementDrawCommands[i].scissor.w};
                        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                }



                vkCmdPushConstants(commandBuffer, tri->screen_space_test_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);

                // vkCmdDraw(commandBuffer, (u32)tri->entityMeshIndexCount[MeshTypes::mesh_uiBox] , 1, 0, 0);
                vkCmdDraw(commandBuffer, (u32)6, 1, 0, 0);
            
                        //reset cliprect
                if(screen_push_constants.misc.x){
                    VkRect2D scissor = {};
                    scissor.offset = {0, 0};
                    scissor.extent = tri->swapChainExtent;
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                }
                
            }break;
            
          

        }
    }


        
    tri->RenderCommandData.screen_transient_text_entries = 0;
    tri->RenderCommandData.screen_transient_text_vertex_count = 0;
    tri->RenderCommandData.screen_transient_char_count = 0;
    tri->RenderCommandData.screenTextDrawCount = 0;
    tri->RenderCommandData.screenElementDrawCount = 0;

}

void screenSpaceText(vkTri* tri , VkCommandBuffer commandBuffer){
    u32& charCount = tri->RenderCommandData.screen_transient_char_count;


    VkBuffer screenTextVertexBuffers[] = {tri->screenTextVertexBuffers[tri->currentFrame]};

    memcpy(tri->screenTextVertexBuffersMapped[tri->currentFrame], tri->RenderCommandData.screen_transient_text, sizeof(TextVertex) * tri->RenderCommandData.screen_transient_text_vertex_count);


    VkDeviceSize screen_offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenTextVertexBuffers, screen_offsets);

    u32 cumulative_offset = 0;
    for(u32 i = 0; i < tri->RenderCommandData.screenTextDrawCount; i++){
            


        ScreenPushConstants screen_push_constants = {};
        screen_push_constants = tri->RenderCommandData.screenTextDrawCommands[i].push;
        //need to massively reduce scale on all draws
        
        if(screen_push_constants.misc.x){
                VkRect2D scissor = {};
                scissor.offset = {(int)tri->RenderCommandData.screenTextDrawCommands[i].scissor.x, (int)tri->RenderCommandData.screenTextDrawCommands[i].scissor.y};
                scissor.extent = {(u32)tri->RenderCommandData.screenTextDrawCommands[i].scissor.z, (u32)tri->RenderCommandData.screenTextDrawCommands[i].scissor.w};
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }

        screen_push_constants.scale = screen_push_constants.scale * 0.25;

        //center the text
        if(tri->RenderCommandData.screenTextDrawCommands[i].center_text){
            screen_push_constants.position.x -= (tri->RenderCommandData.screen_transient_char_widths  [i] * 0.5f) * (screen_push_constants.scale.x) ;
            screen_push_constants.position.y += (tri->RenderCommandData.screen_transient_char_heights [i] * 0.5f) * (screen_push_constants.scale.y) ;
        }
        
        vec2 origPos = screen_push_constants.position;
        screen_push_constants.color = {0.5, 0.5, 0.5, 0.5};
        screen_push_constants.position.x += (5 * 0.25);
        screen_push_constants.position.y += (5 * 0.25);

        vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
        vkCmdDraw(commandBuffer, (u32)tri->RenderCommandData.screen_transient_text_entry_vertex_count[i], 1, cumulative_offset, 0);


        screen_push_constants.color = tri->RenderCommandData.screenTextDrawCommands[i].push.color;
        screen_push_constants.position = origPos;

        vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
        vkCmdDraw(commandBuffer, (u32)tri->RenderCommandData.screen_transient_text_entry_vertex_count[i], 1, cumulative_offset, 0);


        //only works for monospace fonts for now
        if(tri->RenderCommandData.screenTextDrawCommands[i].misc.x == 1){//draw highlighted text
            
            screen_push_constants.color = tri->RenderCommandData.screenTextDrawCommands[i].miscColor;
            // u32 vertCount = (u32)tri->RenderCommandData.screen_transient_text_entry_vertex_count[i] - (tri->screenTextDrawCommands[i].misc.z * 6);
            u32 vertCount = (tri->RenderCommandData.screenTextDrawCommands[i].misc.z * 6);
            u32 vertStart = cumulative_offset + (tri->RenderCommandData.screenTextDrawCommands[i].misc.y * 6);
            screen_push_constants.position.x -= (5 * 0.25);
            screen_push_constants.position.y -= (5 * 0.25);
            // screen_push_constants.position.x += (50 * 0.25) * tri->screenTextDrawCommands[i].misc.y;

            vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
            vkCmdDraw(commandBuffer, vertCount, 1, vertStart, 0);

        }

        cumulative_offset += tri->RenderCommandData.screen_transient_text_entry_vertex_count[i];
        tri->RenderCommandData.screen_transient_text_entry_vertex_count[i] = 0;

        //reset cliprect
        if(screen_push_constants.misc.x){
            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = tri->swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }
    }

    
    tri->RenderCommandData.screen_transient_text_entries = 0;
    tri->RenderCommandData.screen_transient_text_vertex_count = 0;
    tri->RenderCommandData.screen_transient_char_count = 0;
    tri->RenderCommandData.screenTextDrawCount = 0;



}



VkFormat findSupportedFormat(vkTri* tri, VkFormat* candidates, u32 candidate_count, VkImageTiling tiling, VkFormatFeatureFlags features){
    for(u32 i = 0; i < candidate_count; i++){
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(tri->physicalDevice, candidates[i], &props);
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features){
            return candidates[i];
        }else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
            return candidates[i];
        }
    }
    PRINT("FAILED TO FIND SUPPORTED FORMAT!\n");
    return VK_FORMAT_D32_SFLOAT;
}


VkFormat findDepthFormat(vkTri* tri){
    VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    u32 candidate_count = 3;
    return findSupportedFormat(tri, candidates, candidate_count, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}



u32 findMemoryType(vkTri* tri, u32 typeFilter, VkMemoryPropertyFlags properties){
    
    VkPhysicalDeviceMemoryProperties memProperties = {};
    vkGetPhysicalDeviceMemoryProperties(tri->physicalDevice, &memProperties);
    PRINT("memory type count: %u\n", memProperties.memoryTypeCount);
    // for(u32 i = 0; i < memProperties.memoryTypeCount; i++){
    //     PRINT("\t property flags: %d\n", memProperties.memoryTypes[i].propertyFlags);
    //     VkMemoryPropertyFlags flags = memProperties.memoryTypes[i].propertyFlags;
    //     PRINT("Memory Properties: 0x%08X\n", flags);
    
    //     if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    //         PRINT("  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    //         PRINT("  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    //         PRINT("  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
    //         PRINT("  VK_MEMORY_PROPERTY_HOST_CACHED_BIT\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
    //         PRINT("  VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
    //         PRINT("  VK_MEMORY_PROPERTY_PROTECTED_BIT\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
    //         PRINT("  VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
    //         PRINT("  VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD\n");
        
    //     if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
    //         PRINT("  VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV\n");
    //     PRINT("\t heap index: %d\n", memProperties.memoryTypes[i].heapIndex);
    // }
    
    // PRINT("memory heap count: %u\n", memProperties.memoryHeapCount);
    // for(u32 i = 0; i < memProperties.memoryHeapCount; i++){
    //     PRINT("\t size: %zu\n", memProperties.memoryHeaps[i].size);
    //     PRINT("\t flags: %u\n", memProperties.memoryHeaps[i].flags);

    // }

    for(u32 i = 0; i < memProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
            return i;
        }
    }




    PRINT("FAILED to find suitable memory type!\n");



    return 0;
}


//OFFSCREEN RENDERING SETUP
void create_offscreen_texture(vkTri* tri){
    // VkFormat = colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = 512;
    imageInfo.extent.height = 512;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateImage(tri->logicalDevice, &imageInfo, nullptr, &tri->offscreen_image) != VK_SUCCESS){
        PRINT("FAILED to create offscreen texture!\n");
    }else{
        PRINT("successfully created offscreen texture!\n");
    }

    //allocate memory for image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(tri->logicalDevice, tri->offscreen_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(tri, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vkAllocateMemory(tri->logicalDevice, &allocInfo, nullptr, &tri->offscreen_image_memory) != VK_SUCCESS){
        PRINT("FAILED to allocate offscreen image memory!\n");
    }else{
        PRINT("successfully allocated offscreen image memory!\n");
    }

    vkBindImageMemory(tri->logicalDevice, tri->offscreen_image, tri->offscreen_image_memory, 0);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tri->offscreen_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = colorFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if(vkCreateImageView(tri->logicalDevice, &viewInfo, nullptr, &tri->offscreen_image_view) != VK_SUCCESS){
        PRINT("failed to create offscreen texture image view!\n");
    }else{
        PRINT("successfully created offscreen texture image view!\n");
    }


}   

void create_offscreen_render_pass(vkTri* tri){
    VkAttachmentDescription attachments[2] = {};
    attachments[0].format           = VK_FORMAT_R8G8B8A8_SRGB;
    attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//important for sampling later

    attachments[1].format =  findDepthFormat(tri);
    attachments[1].samples =  VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp =  VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp =  VK_ATTACHMENT_STORE_OP_DONT_CARE;//dont need to read it later
    attachments[1].stencilLoadOp =  VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp =  VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout =  VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout =  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    //dependency for layout transitions
    VkSubpassDependency dependencies[2] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    if(vkCreateRenderPass(tri->logicalDevice, &renderPassInfo, nullptr, &tri->offscreen_render_pass) != VK_SUCCESS){
        PRINT("failed to create offscreen render pass!\n");
    }else{
        PRINT("successfully created offscreen render pass!\n");
    }

}


void create_offscreen_frame_buffer(vkTri* tri){
    VkImageView attachments[2] = {tri->offscreen_image_view, tri->offscreen_depth_image_view};
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = tri->offscreen_render_pass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = 512;
    framebufferInfo.height = 512;
    framebufferInfo.layers = 1;

    if(vkCreateFramebuffer(tri->logicalDevice, &framebufferInfo, nullptr, &tri->offscreen_frame_buffer) != VK_SUCCESS){
        PRINT("Failed to create offscreen framebuffer!\n");
    }else{
        PRINT("successfully created offscreen framebuffer!\n");
    }
}

void create_offscreen_depth_resources(vkTri* tri){
    VkFormat depthFormat = findDepthFormat(tri);
    //create depth image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = 512;
    imageInfo.extent.height = 512;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateImage(tri->logicalDevice, &imageInfo, nullptr, &tri->offscreen_depth_image) != VK_SUCCESS){
        PRINT("failed to create offscreen depth image!\n");
    }else{
        PRINT("successfully created offscreen depth image!\n");
    }

    //allocate memory for the depth image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(tri->logicalDevice, tri->offscreen_depth_image, &memRequirements);

    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(tri, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vkAllocateMemory(tri->logicalDevice, &allocInfo, nullptr, &tri->offscreen_depth_image_memory) != VK_SUCCESS){
        PRINT("FAILED to allocate offscreen image memory!\n");
    }else{
        PRINT("successfully allocated offscreen image memory!\n");
    }

    vkBindImageMemory(tri->logicalDevice, tri->offscreen_depth_image, tri->offscreen_depth_image_memory, 0);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tri->offscreen_depth_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if(vkCreateImageView(tri->logicalDevice, &viewInfo, nullptr, &tri->offscreen_depth_image_view) != VK_SUCCESS){
        PRINT("failed to create offscreen depth image view!\n");
    }else{
        PRINT("successfully created offscreen depth image view!\n");
    }

}

//1 create command resourcesz
//2 create texture image
//3 create staging buffer
//4 create sampler (done)
//5 create descriptor resources (done)


// Update the texture with new data from CPU and transfer to GPU
void update_dynamic_texture(vkTri* tri, u32* textureMemory, u32 textureWidth, u32 textureHeight, VkBuffer& buffer, VkDeviceMemory& memory, void* mapped, VkImage& image) {
    // Flush mapped memory to make writes visible to the device

    // u32* pixels = (u32*)tri->dynamic_texture_staging_buffer_mapped[0];
    // for(u32 y = 0; y < 512; y++){
    //     for(u32 x = 0; x < 512; x++){
    //         u8 r = (u8)((x + tri->frameCount) % 256);
    //         u8 g = (u8)((y + tri->frameCount) % 256);
    //         u8 b = (u8)((x + y + tri->frameCount) % 256);
    //         u8 a = 127;

    //         pixels[y * 512 + x] = (a << 24 | b << 16 | g << 8 | r);
    //     }
    // }
    memcpy(mapped, textureMemory, sizeof(u32)*textureWidth*textureHeight);
    // memset(tri->dynamic_texture_staging_buffer_mapped[0], 255, sizeof(u32)*512*512);

    // If not using HOST_COHERENT memory, we would need to flush the memory range here:
    // VkMappedMemoryRange mappedRange{};
    // mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    // mappedRange.memory = stagingMemory;
    // mappedRange.offset = 0;
    // mappedRange.size = VK_WHOLE_SIZE;
    // vkFlushMappedMemoryRanges(device, 1, &mappedRange);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Previous state
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;     // Prepare for transfer
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;           // Previous usage
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;        // New usage
    
    vkCmdPipelineBarrier(
        tri->commandBuffers[tri->currentFrame],
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,   // Previous stage
        VK_PIPELINE_STAGE_TRANSFER_BIT,          // Next stage
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    // Copy the staging buffer to the texture image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;     // Tightly packed pixels
    region.bufferImageHeight = 0;   // Tightly packed pixels
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {textureWidth, textureHeight, 1};
    
    vkCmdCopyBufferToImage(
        tri->commandBuffers[tri->currentFrame],
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    
    // Transition the texture back to shader readable layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;    // Current state after copy
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Make it readable by shader
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;        // Previous usage
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;           // New usage
    
    vkCmdPipelineBarrier(
        tri->commandBuffers[tri->currentFrame],
        VK_PIPELINE_STAGE_TRANSFER_BIT,          // Previous stage
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,   // Next stage
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    // STEP 2.3: Continue with normal rendering commands
    // Begin render pass, bind pipeline, etc.
    
    // Bind the updated texture descriptor set for use in your shaders


}

// // Wait for texture update to complete
// void waitForCompletion() {
//     vkWaitForFences(tri->logicalDevice, 1, &transferFence, VK_TRUE, UINT64_MAX);
// }

void create_dynamic_texture_image(vkTri* tri, u32 textureWidth, u32 textureHeight, VkDeviceMemory& memory, VkImage& image, VkImageView& view){
       // Create texture image
       VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
       VkImageCreateInfo imageInfo{};
       imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
       imageInfo.imageType = VK_IMAGE_TYPE_2D;
       imageInfo.extent.width = textureWidth;
       imageInfo.extent.height = textureHeight;
       imageInfo.extent.depth = 1;
       imageInfo.mipLevels = 1;
       imageInfo.arrayLayers = 1;
       imageInfo.format = textureFormat;

       imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
       imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
       imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
       imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
       imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
       
       if (vkCreateImage(tri->logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
           PRINT("Failed to create texture image!\n");
       }
       
       // Allocate device local memory for the image
       VkMemoryRequirements memRequirements;
       vkGetImageMemoryRequirements(tri->logicalDevice, image, &memRequirements);
       
       VkMemoryAllocateInfo allocInfo{};
       allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
       allocInfo.allocationSize = memRequirements.size;
       allocInfo.memoryTypeIndex = findMemoryType(tri, memRequirements.memoryTypeBits, 
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
       
       if (vkAllocateMemory(tri->logicalDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
           PRINT("Failed to allocate texture memory!\n");
       }
       
       vkBindImageMemory(tri->logicalDevice, image, memory, 0);
       
       // Create image view
       VkImageViewCreateInfo viewInfo{};
       viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
       viewInfo.image = image;
       viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
       viewInfo.format = textureFormat;
       viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       viewInfo.subresourceRange.baseMipLevel = 0;
       viewInfo.subresourceRange.levelCount = 1;
       viewInfo.subresourceRange.baseArrayLayer = 0;
       viewInfo.subresourceRange.layerCount = 1;
       
       if (vkCreateImageView(tri->logicalDevice, &viewInfo, nullptr, &view) != VK_SUCCESS) {
           PRINT("Failed to create texture image view!\n");
       }
       
       // Initial layout transition to shader read
       // Record and submit a one-time command buffer
       VkCommandBufferAllocateInfo allocInfo2{};
       allocInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
       allocInfo2.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
       allocInfo2.commandPool = tri->commandPool;
       allocInfo2.commandBufferCount = 1;
       
       VkCommandBuffer initCmdBuffer;
       vkAllocateCommandBuffers(tri->logicalDevice, &allocInfo2, &initCmdBuffer);
       
       VkCommandBufferBeginInfo beginInfo{};
       beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
       beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
       
       vkBeginCommandBuffer(initCmdBuffer, &beginInfo);
       
       VkImageMemoryBarrier barrier{};
       barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
       barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
       barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
       barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.image = image;
       barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       barrier.subresourceRange.baseMipLevel = 0;
       barrier.subresourceRange.levelCount = 1;
       barrier.subresourceRange.baseArrayLayer = 0;
       barrier.subresourceRange.layerCount = 1;
       barrier.srcAccessMask = 0;
       barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
       
       vkCmdPipelineBarrier(
           initCmdBuffer,
           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
           0,
           0, nullptr,
           0, nullptr,
           1, &barrier
       );
       
       vkEndCommandBuffer(initCmdBuffer);
       
       VkSubmitInfo submitInfo{};
       submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
       submitInfo.commandBufferCount = 1;
       submitInfo.pCommandBuffers = &initCmdBuffer;
       
       vkQueueSubmit(tri->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
       vkQueueWaitIdle(tri->graphicsQueue);
       
       vkFreeCommandBuffers(tri->logicalDevice, tri->commandPool, 1, &initCmdBuffer);
       
}

void create_dynamic_texture_staging_buffer(vkTri* tri, u32 textureWidth, u32 textureHeight, VkBuffer* staging_buffer, VkDeviceMemory* staging_buffer_memory, void** staging_buffer_mapped){
    VkDeviceSize bufferSize = textureWidth * textureHeight * 4;
    //create buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        if(vkCreateBuffer(tri->logicalDevice, &bufferInfo, nullptr, staging_buffer + i) != VK_SUCCESS){
            PRINT("failed to create dynamic texture staging buffer!\n");
        }else{
            PRINT("successfully created dynamic texture staging buffer!\n");
        }
    
        //get memory requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(tri->logicalDevice, staging_buffer[i], &memRequirements);
    
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(tri, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
        if(vkAllocateMemory(tri->logicalDevice, &allocInfo, nullptr, staging_buffer_memory + i) != VK_SUCCESS){
            PRINT("FAILED to allocate offscreen image memory!\n");
        }else{
            PRINT("successfully allocated offscreen image memory!\n");
        }
        vkBindBufferMemory(tri->logicalDevice, staging_buffer[i], staging_buffer_memory[i], 0);
        //persistently map memory for CPU access
        vkMapMemory(tri->logicalDevice, staging_buffer_memory[i], 0, bufferSize, 0, &staging_buffer_mapped[i]);
    }
  

}

//END OFFSCREEN RENDERING SETUP

u32 hash_entry(vkTri* tri, vertex_hash_entry& entry){
    u32  vi = entry.indices.x;
    u32 vni = entry.indices.y;
    u32 vti = entry.indices.z;

    u32 hash = 7993*vi + 99131*vni ^ 137*vti;

    return hash;
}


#define OBJ_DEBUG 0

#if OBJ_DEBUG
#define OBJPRINT(str, ...) PRINT(str, ##__VA_ARGS__)
#else
#define OBJPRINT(str, ...)
#endif

void parse_int3(char* buf, char** new_pos, u32* vi, u32* vti, u32* vni){

    number v = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%d/", v.value.i_value);
    *vi = v.value.i_value; 
    
    number vt = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%d/", vt.value.i_value);
    *vti = vt.value.i_value; 

    number vn = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%d ", vn.value.i_value);
    *vni = vn.value.i_value; 
    
    *new_pos = buf;
}

void parse_float3(char* buf, char** new_pos, vec3* vec){

    number x = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%f ", x.value.f_value);
    vec->x = x.value.f_value;
    
    number y = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%f ", y.value.f_value);
    vec->y = y.value.f_value;
    
    number z = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%f\n", z.value.f_value);
    vec->z = z.value.f_value;

    *new_pos = buf;

}

void parse_float2(char* buf, char** new_pos, vec2* vec){
    number x = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%f ", x.value.f_value);
    vec->x = x.value.f_value;

    number y = string_to_number(buf, &buf);
    buf++;
    OBJPRINT("%f\n", y.value.f_value);
    vec->y = y.value.f_value;

    *new_pos = buf;

}

/////////////////////////////////////// OBJ LOADER ATTEMPT ////////////////////////////////////////////////////////


void load_obj_test(vkTri* tri){

    char filename[32] = "C:/Libraries/viking_room.obj";

    debug_file debugFile = {};
    debugFile = Win32ReadFile(filename);

    size_t len = (size_t)debugFile.filesize.QuadPart;
    char* buf = (char*)debugFile.memory;
    char* eof = buf + len;
     int fuck_the_debugger = 0;
     Vertex x = {};
     u32 vi   = 0;
     u32 vti  = 0;
     u32 vni  = 0;
     u32 index_count = 0;
     u32 vert_count = 0;
     u32 vertt_count = 0;
     u32 vertn_count = 0;
     size_t test_offset = 0;

    //  Vertex model_vertices[4675];
    //  u32 model_indices[11484];
    size_t max_vertices = (5000);//total count for the viking room is 4675
    size_t max_indices = (5000 + 1) * 3;
    vec3* vert_pos =   (vec3*)plat_alloc_mem(max_vertices * sizeof(vec3));
    vec3* vert_norm =  (vec3*)plat_alloc_mem(max_vertices * sizeof(vec3));
    vec2* vert_tex =   (vec2*)plat_alloc_mem(max_vertices * sizeof(vec2));
    u32*  pos_indices = (u32*)plat_alloc_mem(max_indices * sizeof(u32)  );
    u32* norm_indices = (u32*)plat_alloc_mem(max_indices * sizeof(u32)  );
    u32*  tex_indices = (u32*)plat_alloc_mem(max_indices * sizeof(u32)  );

    u32 vert_pos_count = 0;
    u32 vert_norm_count = 0;
    u32 vert_tex_count = 0;
    u32 pos_indices_count = 0;
    u32 norm_indices_count = 0;
    u32 tex_indices_count = 0;


    memset(vert_pos, 0, sizeof(vec3) *  max_vertices);
    memset(vert_norm, 0, sizeof(vec3) * max_vertices);
    memset(vert_tex, 0, sizeof(vec2) *  max_vertices);
    memset( pos_indices, 0, sizeof(u32) *    max_indices);
    memset(norm_indices, 0, sizeof(u32) *    max_indices);
    memset( tex_indices, 0, sizeof(u32) *    max_indices);

    //first pass to read in all the geometry data
    while(buf < eof){
        if(*buf != 'v' && *buf != 'f'){
            //skip the entire line
            while(*buf != '\n' && buf < eof){
                buf++;
            }
            if(*buf == '\n')buf++;
            OBJPRINT("\n");
        }
        else{
            if(*buf == 'v' && *(buf+1) == ' '){
                buf+=2;
                OBJPRINT("v ");
                parse_float3(buf, &buf, vert_pos + vert_pos_count);
                vert_pos_count++;
            }
            else if(*buf == 'v' && *(buf+1) == 'n' && *(buf+2) == ' '){
                buf+=3;
                OBJPRINT("vn ");
                // parse_float3(buf, &buf, &tri->model_vertices[vertn_count].color);
                parse_float3(buf, &buf, vert_norm + vert_norm_count);
                vert_norm_count++;
            }
            else if(*buf == 'v' && *(buf+1) == 't' && *(buf+2) == ' '){
                buf+=3;
                OBJPRINT("vt ");
                parse_float2(buf, &buf, vert_tex + vert_tex_count);
                vert_tex_count++;
            }
            else if(*buf == 'f' && *(buf+1) == ' '){
                buf+=2;
                OBJPRINT("f ");
                parse_int3(buf, &buf, pos_indices + pos_indices_count, tex_indices + tex_indices_count, norm_indices + norm_indices_count);
                 pos_indices_count++;
                norm_indices_count++;
                 tex_indices_count++;

                parse_int3(buf, &buf, pos_indices + pos_indices_count, tex_indices + tex_indices_count, norm_indices + norm_indices_count);
                 pos_indices_count++;
                norm_indices_count++;
                 tex_indices_count++;

                 parse_int3(buf, &buf, pos_indices + pos_indices_count, tex_indices + tex_indices_count, norm_indices + norm_indices_count);
                OBJPRINT("\n");
                 pos_indices_count++;
                norm_indices_count++;
                 tex_indices_count++;
                 
            }
        }
     }

     //second pass to actually set all the geometry data
     u32 duplicates = 0;
     for(u32 i = 0; i < pos_indices_count; i++){
        u32  pos_index  =  pos_indices[i] ; //1 indexed line number to read from
        u32 norm_index  = norm_indices[i] ; 
        u32  tex_index  =  tex_indices[i] ; 
        // if(!pos_index || !norm_index || !tex_index){
        //     PRINT("index is 0???\n");
        // } 
        vertex_hash_entry entry = {};
        entry.indices.x =  pos_index - 1;
        entry.indices.y = norm_index - 1;
        entry.indices.z =  tex_index - 1;
        entry.index = tri->model_vertex_count;

        u32 hash = hash_entry(tri, entry);
        u32 index = hash & (HASH_TABLE_SIZE - 1);
        
        bool found_entry = false;
        u32 index_to_use = tri->model_vertex_count;
        for(int i = 0; i < VERTEX_HASH_BUCKETS; i++){
            if(tri->table_bucket_count[index] >= VERTEX_HASH_BUCKETS || i >= VERTEX_HASH_BUCKETS){
                PRINT("ERROR COULDNT INSERT VERTEX INTO HASH TABLE!\n");
                break;
            }
            if(tri->table[index][i].occupied){
                // PRINT("collision!\n");
                if(tri->table[index][i].indices == entry.indices){
                    // PRINT("same vertex found! reusing!\n");
                    found_entry = true;
                    index_to_use = tri->table[index][i].index;
                    break;
                }
            }else{//found an unoccupied slot
                tri->table[index][i] = entry;
                tri->table[index][i].occupied = true;
                tri->table_bucket_count[index]++;
                index_to_use = entry.index;
                break;
            }
        }

        if(!found_entry){

            tri->model_vertices[tri->model_vertex_count].pos        =  vert_pos[pos_index  - 1];
            // PRINT("vert %u: %f %f %f\n", i, tri->model_vertices[i].pos.x, tri->model_vertices[i].pos.y, tri->model_vertices[i].pos.z);
    
            tri->model_vertices[tri->model_vertex_count].color      = vert_norm[norm_index - 1];
            tri->model_vertices[tri->model_vertex_count].texCoord.x   =  vert_tex[tex_index  - 1].x;
            tri->model_vertices[tri->model_vertex_count].texCoord.y   =  1 - vert_tex[tex_index  - 1].y; //flip vertical component for vulkan
            tri->model_indices [i] = index_to_use;
            tri->model_vertex_count++;

        }else{
            tri->model_indices[i] = index_to_use;
            duplicates++;
        }
        tri->model_index_count++;
     
     }
    Win32FreeFile(&debugFile);
     PRINT("vertex count: %u\n", vert_pos_count);
     PRINT("vertex normal: %u\n",  vert_norm_count);
     PRINT("vertex texture: %u\n", vert_tex_count);
     PRINT("index count: %u\n", tri->model_index_count);
     PRINT("DUPLICATE vertices found: %u\n", duplicates);
     
    fuck_the_debugger = 0;


    plat_dealloc_mem((void*)vert_pos    , max_vertices * sizeof(vec3));
    plat_dealloc_mem((void*)vert_norm   , max_vertices * sizeof(vec3));
    plat_dealloc_mem((void*)vert_tex    , max_vertices * sizeof(vec2));
    plat_dealloc_mem((void*) pos_indices, max_indices * sizeof(u32)  );
    plat_dealloc_mem((void*)norm_indices, max_indices * sizeof(u32)  );
    plat_dealloc_mem((void*) tex_indices, max_indices * sizeof(u32)  );

    fuck_the_debugger = 0;

}



///////////////////////////////////////END END END OBJ LOADER ATTEMPT END END END ////////////////////////////////////////////////////////




#ifdef LABOR_DEBUG
global_variable const char* validationLayers[VALIDATION_LAYER_COUNT] = {
    "VK_LAYER_KHRONOS_validation",
};

    global_variable bool enableValidationLayers = true;
#else
    global_variable bool enableValidationLayers = false;
#endif

global_variable u32 deviceExtensionCount;
global_variable const char* deviceExtensions[DEVICE_EXTENSION_COUNT];// = {
//     VK_KHR_SWAPCHAIN_EXTENSION_NAME,
// };


global_variable VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT mesageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    PRINT("validation layer: %s \n", pCallbackData->pMessage);

    //id number check prevents this: 
    /*
        validation layer: vkCreateInstance(): Attempting to enable extension VK_EXT_debug_utils, but this extension is intended to support use by applications when debugging and it 
        is strongly recommended that it be otherwise avoided.
    */
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT && pCallbackData->messageIdNumber  != 1734198062 && pCallbackData->messageIdNumber != 0) {
        PRINT("ERROR OR HIGHER RECEIVED!\n");

        __debugbreak();
        //look at the stack trace to see what caused this message
    }



    return VK_FALSE;
}

/*
The pCallbackData parameter refers to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message itself, with the most important members being:

pMessage: The debug message as a null-terminated string
pObjects: Array of Vulkan object handles related to the message
objectCount: Number of objects in array
*/

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, vkTri* tri){
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; 
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = tri; //optional
}

void setupDebugMessenger(vkTri* tri){


    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo, tri);

    if(CreateDebugUtilsMessengerEXT(tri->instance, &createInfo, nullptr, &tri->debugMessenger) != VK_SUCCESS){
        PRINT("FAILED to set up debug messenger!\n");
    }else{
        PRINT("successfully setup debug messenger!\n");
    }
}

bool checkValidationLayerSupport(){
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties availableLayers[MAX_LAYERS] = {};
    if(layerCount >= MAX_LAYERS){
        PRINT("TOO MANY VALIDATION LAYERS: %d, we only allocated size for %d!\n", layerCount, MAX_LAYERS);
    }else{
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
        
        for(int i = 0; i < VALIDATION_LAYER_COUNT; i++){
            bool layerFound = false;

            for(int j = 0; j < layerCount; j++){
                PRINT("\t strcmp %s AND %s\n", validationLayers[i], availableLayers[j].layerName);
                if(handmade_strcmp(validationLayers[i], availableLayers[j].layerName)){
                    layerFound = true;
                    break;
                } 
            }
            if(!layerFound){
                return false;
            }
        }
    
    }

    return true;


}

int getRequiredExtensions(const char** extensions, u32& count){

    count = 0;

    extensions[count++] = "VK_KHR_surface";
    extensions[count++] = "VK_KHR_win32_surface";

    #ifdef LABOR_DEBUG
    extensions[count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    #endif

    return count;

}

void createVkInstance(vkTri* tri){
    
    #ifdef LABOR_DEBUG
    if(!checkValidationLayerSupport()){
        PRINT("validation layers requested, but not available!\n");
    }else{
        PRINT("successfully got validation layers!\n");
    }
    #endif

    u32 extensionCount = 0; 
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    PRINT("extension count: %d extensions supported\n", extensionCount);

    VkExtensionProperties extensionProps[MAX_EXTENSIONS] = {};
    if(extensionCount >= MAX_EXTENSIONS){
        PRINT("TOO MANY AVAILABLE EXTENSIONS: %d, we only allocated size for %d!\n", extensionCount, MAX_EXTENSIONS);
    }else{
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProps);
        PRINT("available extensions: \n");
        for (int i = 0; i < extensionCount; i++)
        {
            PRINT("\t %s\n", extensionProps[i].extensionName);
        }
        
    }


    char* path = getenv("VK_LAYER_SETTINGS_PATH");
    if (path) {
        PRINT("VK_LAYER_SETTINGS_PATH is set to: %s\n", path);
    } else {
        PRINT("VK_LAYER_SETTINGS_PATH is NOT set!\n");
    }

    tri->extension_count = 0;
    getRequiredExtensions(tri->extensions, tri->extension_count);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Raw Vulkan";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Labor";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;


    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = tri->extension_count;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledExtensionNames = tri->extensions;
    create_info.pNext = nullptr;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    
    
    #ifdef LABOR_DEBUG
    create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    create_info.ppEnabledLayerNames = validationLayers;

    populateDebugMessengerCreateInfo(debugCreateInfo, tri);
    create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    #endif


    VkResult result = vkCreateInstance(&create_info, NULL, &tri->instance);
    if(result != VK_SUCCESS){
        PRINT("FAILED to create Vulkan instance!\n");
        return;
    }



    PRINT("Vulkan instance created!\n");
}



//TODO: need to eventually put this in platform layer for cleaner separation between layers
void createSurface(vkTri* tri){

    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = g_window;
    createInfo.hinstance = g_instance;

    if(vkCreateWin32SurfaceKHR(tri->instance, &createInfo, nullptr, &tri->surface) != VK_SUCCESS){
        PRINT("FAILED to create window surface!\n");
    }else{
        PRINT("successfully created window surface!\n");
    }

    int fuck_the_debugger = 0; 
}



// SELECTING PHYSICAL DEVICE

// int rateDeviceSuitability(VkPhysicalDevice device){
//     int score = 0;

//     //discrete GPUs have a significant performance advantage
//     if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
//         score += 1000;
//     }

//     //maximum possible size of textures affects graphics quality
//     score += deviceProperties.limits.maxImageDimension2d;

//     //application cant function without geometry shaders (as an example)
//     if(!deviceFeatures.geometryShader){
//         return 0;
//     }

//     return score;
// }

//test arena for vulkan
typedef struct {
    u8* base;
    size_t used;
    size_t size;
} Arena;

//must destroy vulkan objects before unwinding arena

void* my_alloc(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope) {
    Arena* arena = (Arena*)pUserData;

    uintptr_t current = (uintptr_t)(arena->base + arena->used);
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t offset = aligned - (uintptr_t)arena->base;

    if (offset + size > arena->size) return NULL;

    arena->used = offset + size;
    return arena->base + offset;
}


struct QueueFamilyIndices{
    u32 graphicsAndComputeFamily;
    u32 presentFamily;
    u32 transferFamily;
    int graphics_has_value;//tells us if there is useful info here
    int present_has_value;//tells us if there is useful info here
    int transfer_has_value;
};

#define MAX_QUEUE_FAMILIES 64

QueueFamilyIndices findQueueFamilies(vkTri* tri, VkPhysicalDevice device){
    //logic to find graphics queue family
    QueueFamilyIndices indices = {};

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties queueFamilies[MAX_QUEUE_FAMILIES] = {};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    PRINT("queue family count: %u\n", queueFamilyCount);

    //first pass looking for transfer queue
    for (int i = 0; i < queueFamilyCount; i++){
        if((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
        !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)){
          indices.transferFamily = i;
            indices.transfer_has_value++;
            break;
          }
    }

    for (int i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 presentSupport = false;
        //i tells the earlier call vkGetPhysicalDeviceQueueFamilyProperties which present family index to use
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, tri->surface, &presentSupport);
        if(presentSupport){
            indices.presentFamily = i;
            indices.present_has_value++;
        }

        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT){
            indices.graphicsAndComputeFamily = i;
            indices.graphics_has_value++;
        }

        if(!indices.transfer_has_value && (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)){
            indices.transferFamily = i;
            indices.transfer_has_value++;
        }   



        if(indices.graphics_has_value && indices.present_has_value && indices.transfer_has_value)break;
    }
    
    if(!indices.graphics_has_value && indices.present_has_value && indices.transfer_has_value){
        PRINT("ERROR! queue family index has no set value! ERROR!\n");
    }


    return indices;

}




bool checkDeviceExtensionSupport(VkPhysicalDevice device){
    //setup global variables
    deviceExtensionCount = 0;
    deviceExtensions[deviceExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    
    u32 extensionCount = 0;
    
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    if(extensionCount > MAX_EXTENSIONS){
        PRINT("extensionCount too large! %d, Resizing to %d!\n", extensionCount, MAX_EXTENSIONS);
        extensionCount = MAX_EXTENSIONS;
    }

    VkExtensionProperties availableExtensions[MAX_EXTENSIONS] = {};
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);
    //extensionCount could be reset in the enumerate extension properties
    if(extensionCount > MAX_EXTENSIONS){
        PRINT("extensionCount too large! %d, Resizing to %d!\n", extensionCount, MAX_EXTENSIONS);
        extensionCount = MAX_EXTENSIONS;
    }

    for(u32 i = 0; i < deviceExtensionCount; i++){
        const char* required = deviceExtensions[i];
        bool found = false;

        for(u32 j = 0; i < extensionCount; j++){
            PRINT("required extension: %s, available extension: %s\n", required, availableExtensions[j].extensionName);
            if(handmade_strcmp(required, availableExtensions[j].extensionName)){
                found = true;
                break;
            }
        }
        if(!found){
            PRINT("REQUIRED EXTENSIONS NOT FOUND!\n");
            return false;
        }

    }

    PRINT("All required extensions found!\n");
    return true;
    
}
    

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR& capabilities){
    if(capabilities.currentExtent.width != UINT32_MAX){
        return capabilities.currentExtent;
    }else{
        u32 width = g_width;
        u32 height = g_height;

        if(!width){
            width = 1;  //if the window is ever 0 in any dimension
        }
        if(!height){
            height = 1;
        }

        PRINT("chooseSwapExtent() current window bounds: %u %u\n", width, height);

        VkExtent2D actualExtent = {};
        actualExtent.width =  (u32)width;
        actualExtent.height = (u32)height;

        actualExtent.width = uclamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = uclamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        PRINT("actual extent: %u %u\n", actualExtent.width ,actualExtent.height);

        return actualExtent;
    }
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* available_present_modes, u32 present_mode_count){

    for (u32 i = 0; i < present_mode_count; i++)
    {
        if(available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            return available_present_modes[i];
        }
    }
    
    PRINT("couldnt find preferred present mode of VK_PRESENT_MODE_MAILBOX_KHR, returning VK_PRESENT_MODE_FIFO_KHR instead\n");
    //guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* available_formats, u32 format_count){
    VkSurfaceFormatKHR surface_format = {};

    for(u32 i = 0; i < format_count; i++){
        if(available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return available_formats[i];
        }
    }

    PRINT("couldnt find preferred format of VK_FORMAT_B8G8R8A8_SRGB and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n");
    //good enough to go with first format specific if we couldn't find preferred format
    return available_formats[0];
}




struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    u32 present_mode_count;
    VkSurfaceFormatKHR formats[MAX_FORMATS];
    VkPresentModeKHR presentModes[MAX_PRESENT_MODES];
};

SwapChainSupportDetails querySwapChainSupportDetails(vkTri* tri, VkPhysicalDevice device){
    SwapChainSupportDetails details = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, tri->surface, &details.capabilities);

    details.format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, tri->surface, &details.format_count, nullptr);

    if(details.format_count < MAX_FORMATS){
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, tri->surface, &details.format_count, details.formats);
    }else{
        PRINT("TOO MANY FORMATS! FORMAT COUNT: %d\n", details.format_count);
    }

    details.present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, tri->surface, &details.present_mode_count, nullptr);
    if(details.present_mode_count < MAX_PRESENT_MODES){
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, tri->surface, &details.present_mode_count, details.presentModes);
    }else{
        PRINT("TOO MANY PRESENT MODES! FORMAT COUNT: %d\n", details.present_mode_count);
    }

    return details;
}


VkImageView createImageView(vkTri* tri, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels){

    
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask      = aspectFlags; 
    viewInfo.subresourceRange.baseMipLevel    = 0;
    viewInfo.subresourceRange.levelCount      = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer  = 0;
    viewInfo.subresourceRange.layerCount      = 1;

    VkImageView imageView = {};
    if(vkCreateImageView(tri->logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS){
        PRINT("FAILED to create texture image view!\n");
    }else{
        PRINT("successfully created texture image view!\n");
    }

    return imageView;
}


void createImageViews(vkTri* tri){
    for (size_t i = 0; i < tri->swapChainImageCount; i++)
    {
        tri->swapChainImageViews[i] = createImageView(tri, tri->swapChainImages[i], tri->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    
}



void createTextureSampler(vkTri* tri){
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    /*
        //could also not use it by conditionally setting
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

    */

    samplerInfo.maxAnisotropy = tri->physDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    // samplerInfo.minLod = tri->mipLevels - 5;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    if(vkCreateSampler(tri->logicalDevice, &samplerInfo, nullptr, &tri->textureSampler) != VK_SUCCESS){
        PRINT("FAILED to create texture sampler!\n");
    }else{
        PRINT("successfully created texture sampler!\n");
    }

}

void createSwapChain(vkTri* tri){
    SwapChainSupportDetails swap_chain_support = querySwapChainSupportDetails(tri, tri->physicalDevice);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats, swap_chain_support.format_count);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.presentModes, swap_chain_support.present_mode_count);
    VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

    //recommended to request at least one more image than the minimum to avoid waiting on the driver
    u32 image_count = swap_chain_support.capabilities.minImageCount + 1;

    if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount){
        PRINT("eiter no max image count or image count > max image count. image count: %d, max image count: %d, setting image count to max image count\n", image_count, swap_chain_support.capabilities.maxImageCount);
        image_count = swap_chain_support.capabilities.maxImageCount;
        
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = tri->surface;

    createInfo.minImageCount        = image_count;
    createInfo.imageFormat          = surface_format.format;
    createInfo.imageColorSpace      = surface_format.colorSpace;
    createInfo.imageExtent          = extent;
    createInfo.imageArrayLayers     = 1;

    //for post processing like rendering to a texture we would use VK_IMAGE_USAGE_TRANSFER_DST_BIT
    createInfo.imageUsage           = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(tri, tri->physicalDevice);
    u32 queueFamilyIndices[2] = {};
    queueFamilyIndices[0] = indices.graphicsAndComputeFamily;
    queueFamilyIndices[1] = indices.presentFamily;

    if(indices.graphicsAndComputeFamily != indices.presentFamily){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }else{
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;//optional
        createInfo.pQueueFamilyIndices = nullptr;//optional
    }

    createInfo.preTransform = swap_chain_support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present_mode;
    createInfo.clipped = VK_TRUE;

    //if the window is resized or the swapchain otherwise becomes invalid, we need to recreate it and pass an old pointer
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(tri->logicalDevice, &createInfo, nullptr, &tri->swapchain) != VK_SUCCESS){
        PRINT("FAILED to create swap chain!\n");
    }else{
        PRINT("successfully created swapchain!\n");
    }

    PRINT("image count: %d\n", image_count);
    if(image_count >= MAX_SWAPCHAIN_IMAGES){
        PRINT("image count is too large: %d,  setting to %d\n", image_count, MAX_SWAPCHAIN_IMAGES);
        image_count = MAX_SWAPCHAIN_IMAGES;
    }
    vkGetSwapchainImagesKHR(tri->logicalDevice, tri->swapchain, &image_count, nullptr);
    vkGetSwapchainImagesKHR(tri->logicalDevice, tri->swapchain, &image_count, tri->swapChainImages);

    tri->swapChainImageCount = image_count;
    tri->swapChainImageFormat = surface_format.format;
    tri->swapChainExtent = extent;

}


bool isDeviceSuitable(vkTri* tri, VkPhysicalDevice device){

    QueueFamilyIndices indices = findQueueFamilies(tri, device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if(extensionsSupported){
        SwapChainSupportDetails swapChainSupport = querySwapChainSupportDetails(tri, device);
        swapChainAdequate = swapChainSupport.format_count && swapChainSupport.present_mode_count;
    }

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return swapChainAdequate && indices.graphics_has_value && indices.present_has_value && indices.transfer_has_value && supportedFeatures.samplerAnisotropy;

}


void createBuffer(vkTri* tri, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size  = size;
    bufferInfo.usage = usage;
    bufferInfo.flags = 0;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


    if(vkCreateBuffer(tri->logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS){
        PRINT("FAILED to create buffer!\n");
    }else{
        PRINT("successfully created buffer!\n");


        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(tri->logicalDevice, buffer, &memRequirements);
        PRINT("memRequirements size: %zu, alignment: %zu, memoryTypeBits: %u\n", memRequirements.size, memRequirements.alignment, memRequirements.memoryTypeBits);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(tri, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if(vkAllocateMemory(tri->logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS){
            PRINT("FAILED to allocate buffer memory!\n");
        }else{
            PRINT("successfully allocated buffer memory!\n");

            //last parameter is the offset in the region of memory, if its non zero, we need to make it divisible by memRequirements.alignment
            vkBindBufferMemory(tri->logicalDevice, buffer, bufferMemory, 0);

  

        }

    }
}

int pickPhysicalDevice(vkTri* tri){

    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(tri->instance, &deviceCount, nullptr);
    if(deviceCount == 0){
        PRINT("FAILED to find GPUs with Vulkan support!\n");
        return -1;
    }else{
        PRINT("found %d GPUs with Vulkan support!\n", deviceCount);
    }


    VkPhysicalDevice devices[MAX_DEVICES] = {};
    u32 deviceScores[MAX_DEVICES] = {};
    u32 maxDeviceScore = 0;

    vkEnumeratePhysicalDevices(tri->instance, &deviceCount, devices);
    tri->physDeviceProperties = {};

    for(int i = 0; i < deviceCount; i++){
        if(isDeviceSuitable(tri, devices[i])){
            vkGetPhysicalDeviceProperties(devices[i], &tri->physDeviceProperties);
            if (tri->physDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                deviceScores[i] += 1000;
            }
            if(maxDeviceScore < deviceScores[i])maxDeviceScore = deviceScores[i];

        }
    }

    for(int i = 0; i < deviceCount; i++){
        if(deviceScores[i] == maxDeviceScore){
            tri->physicalDevice = devices[i];
            break;
        }
    }

    if(tri->physicalDevice == VK_NULL_HANDLE){
        PRINT("FAILED to find a suitable GPU\n");
        return -1;
    }else{
        PRINT("found suitable GPU!\n");
        vkGetPhysicalDeviceProperties(tri->physicalDevice, &tri->physDeviceProperties);
        VkDeviceSize max_storage_buffer_range = tri->physDeviceProperties.limits.maxStorageBufferRange;

        VkBool32 timestampComputeAndGraphics = tri->physDeviceProperties.limits.timestampComputeAndGraphics;
        tri->timestampPeriod = tri->physDeviceProperties.limits.timestampPeriod;
        PRINT("timestampComputeAndGraphics: %d, timestampPeriod: %f\n", timestampComputeAndGraphics, tri->timestampPeriod);
        
        PRINT("device size: %zu\n", max_storage_buffer_range);
                                          //8 bytes per face    131072 * 6 faces per chunk * draw distance of 7x7x7 chunks
        u32 max_draw_distance = 7*7*7;
        u32 mid_draw_distance = 5*5*5;
        // u32 min_draw_distance = 3*1*1;
        u32 min_draw_distance = 3*3*3;
        uvec3 max_draw_vec = {7,7,7};
        uvec3 mid_draw_vec = {5,5,5};
        uvec3 min_draw_vec = {3,3,3};
        // uvec3 min_draw_vec = {3,1,1};
        uvec3 no_draw_vec =   {3,1,1};
        u32 no_draw_distance = no_draw_vec.x * no_draw_vec.y * no_draw_vec.z;
        tri->single_chunk_size = sizeof(FaceData) * ((64*64*64 * 6) * 0.5);
        size_t required_chunk_memory = tri->single_chunk_size;
        #if 1
        if(max_storage_buffer_range > (required_chunk_memory * max_draw_distance)){
            PRINT("MAX DRAW DISTANCE SELECTED!\n");
            //hardcoded to use the lowest case for now
            // required_chunk_memory *= max_draw_distance;
            // tri->chunk_draw_distance = max_draw_vec;
            required_chunk_memory               *= min_draw_distance;
            tri->RenderCommandData.chunk_draw_distance             = min_draw_vec;
            tri->chunk_draw_distance_volume      = min_draw_distance;


       
        }else if(max_storage_buffer_range > (required_chunk_memory * mid_draw_distance)){
            PRINT("MID DRAW DISTANCE SELECTED!\n");
            required_chunk_memory *= mid_draw_distance;
            tri->RenderCommandData.chunk_draw_distance = mid_draw_vec;
            tri->chunk_draw_distance_volume = mid_draw_distance;

       
        }else if(max_storage_buffer_range > (required_chunk_memory * min_draw_distance)){
            PRINT("MIN DRAW DISTANCE SELECTED!\n");
            required_chunk_memory *= min_draw_distance;
            tri->RenderCommandData.chunk_draw_distance = min_draw_vec;
            tri->chunk_draw_distance_volume = min_draw_distance;

        #else
                    if(max_storage_buffer_range > (required_chunk_memory * min_draw_distance)){
                    PRINT("MIN DRAW DISTANCE SELECTED!\n");
                    required_chunk_memory *= no_draw_distance;
                    tri->RenderCommandData.chunk_draw_distance = no_draw_vec;
                    tri->chunk_draw_distance_volume = no_draw_distance;


        #endif
        }else{
            PRINT("GPU CANNOT SUPPORT MIN CHUNK SIZE!\n");
        }
        PRINT("required max chunk draw distnace of %zu fits within max storage buffer range of %zu!\n",  required_chunk_memory, max_storage_buffer_range); 

        tri->required_chunk_ssbo_memory = required_chunk_memory;

        tri->chunkStagingPerFrameSize = (64*64*32)*6 * sizeof(FaceData) * tri->chunk_draw_distance_volume;
        tri->chunkStagingSizeTotal = tri->chunkStagingPerFrameSize * CHUNK_STAGING_SLICES;


        //fill up the free list indices
        tri->voxelSSBOFreeCount = tri->chunk_draw_distance_volume;
        for(u32 i = 0; i < tri->chunk_draw_distance_volume; i++){
            tri->voxelSSBOFreeIndices[i] = i;
        }
    

        VkSampleCountFlags counts = tri->physDeviceProperties.limits.framebufferColorSampleCounts & tri->physDeviceProperties.limits.framebufferDepthSampleCounts;
#if USE_MSAA
        if     (counts & VK_SAMPLE_COUNT_64_BIT){ tri->msaaSamples =  VK_SAMPLE_COUNT_64_BIT; PRINT("msaa samples: 64 bit!\n"); }
        else if(counts & VK_SAMPLE_COUNT_32_BIT){ tri->msaaSamples =  VK_SAMPLE_COUNT_32_BIT; PRINT("msaa samples: 32 bit!\n"); }
        else if(counts & VK_SAMPLE_COUNT_16_BIT){ tri->msaaSamples =  VK_SAMPLE_COUNT_16_BIT; PRINT("msaa samples: 16 bit!\n"); }
        else if(counts & VK_SAMPLE_COUNT_8_BIT) { tri->msaaSamples =  VK_SAMPLE_COUNT_8_BIT;  PRINT("msaa samples: 8 bit!\n");  }
        else if(counts & VK_SAMPLE_COUNT_4_BIT) { tri->msaaSamples =  VK_SAMPLE_COUNT_4_BIT;  PRINT("msaa samples: 4 bit!\n");  }
        else if(counts & VK_SAMPLE_COUNT_2_BIT) { tri->msaaSamples =  VK_SAMPLE_COUNT_2_BIT;  PRINT("msaa samples: 2 bit!\n");  }
        else                                    { tri->msaaSamples =  VK_SAMPLE_COUNT_1_BIT;  PRINT("msaa samples: 1 bit!\n");  }
#else
        tri->msaaSamples = VK_SAMPLE_COUNT_1_BIT;
#endif
    }





    return 0;
}



/// LOGICAL DEVICE CREATION

void createLogicalDevice(vkTri* tri){

    
    QueueFamilyIndices indices = findQueueFamilies(tri, tri->physicalDevice);

    VkDeviceQueueCreateInfo queueCreateInfos[MAX_QUEUE_FAMILIES];

    float queuePriority = 1.0f;
    u32 familyCount = 0;
    //queue family of commands we need
    if(indices.graphics_has_value){
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsAndComputeFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[familyCount++] = queueCreateInfo;
    }
    //check thats its not the same value
    if(indices.presentFamily == indices.graphicsAndComputeFamily){
        PRINT("Graphics and Present queues share the same family index: %d\n", indices.graphicsAndComputeFamily);
    }

    if(indices.present_has_value && indices.presentFamily != indices.graphicsAndComputeFamily){
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.presentFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[familyCount++] = queueCreateInfo;
    }


    if(indices.transfer_has_value && (indices.transferFamily != indices.presentFamily) && (indices.transferFamily != indices.graphicsAndComputeFamily) ){
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.transferFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[familyCount++] = queueCreateInfo;
    }




    VkPhysicalDeviceFeatures deviceFeatures =  {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;//optional but needed for texture sampling

    //create logical device
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = familyCount;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtensionCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    createInfo.enabledLayerCount = 0;

    #ifdef LABOR_DEBUG
        createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        createInfo.ppEnabledLayerNames = validationLayers;
    #endif


    if(vkCreateDevice(tri->physicalDevice, &createInfo, nullptr, &tri->logicalDevice) != VK_SUCCESS){
        PRINT("FAILED to create logical device!\n");
    }else{
        PRINT("successfully created logical device!\n");
    }

    vkGetDeviceQueue(tri->logicalDevice, indices.graphicsAndComputeFamily, 0, &tri->graphicsQueue);
    vkGetDeviceQueue(tri->logicalDevice, indices.graphicsAndComputeFamily, 0, &tri->computeQueue);
    vkGetDeviceQueue(tri->logicalDevice, indices.presentFamily, 0, &tri->presentQueue);

    //TODO: (nate) how do we gracefully fallback to some other queue if no transfer queue is detected?
    if(indices.transfer_has_value){
        vkGetDeviceQueue(tri->logicalDevice, indices.transferFamily, 0, &tri->transferQueue);
    }else{
        PRINT("[VULKAN] NO TRANSFER QUEUE DETECTED?? WHAT DO WE DO??\n");
        Assert(!"VULKAN ERROR!");
    }

    


}







VkShaderModule createShaderModule(VkDevice device, char* code, u32 size) {
    VkShaderModule shaderModule = {};
    
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    
    //TODO need to align memory to 4 bytes
    createInfo.pCode = (uint32_t*)(code);
    
    // Create the shader module
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        // Handle error
        return VK_NULL_HANDLE;
    }
    PRINT("successfully created shader!\n");
    return shaderModule;
}



void createRenderPass(vkTri* tri){
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format  = tri->swapChainImageFormat;
    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.samples = tri->msaaSamples;
#if USE_MSAA
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#else
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#endif


    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format  = findDepthFormat(tri);
    // depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.samples = tri->msaaSamples;
    depthAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = tri->swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; //directly reference in layout(location = 0) out vec4 outColor in the fragment shader
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;



    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

#if USE_MSAA
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
#else
#endif




    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
#if USE_MSAA
dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
#else
dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
#endif
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


#if USE_MSAA
    VkAttachmentDescription attachments[3] = {colorAttachment, depthAttachment, colorAttachmentResolve};
    u32 attachmentCount = 3;
#else
    VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
    u32 attachmentCount = 2;
#endif

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachmentCount;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;


    if(vkCreateRenderPass(tri->logicalDevice, &renderPassInfo, nullptr, &tri->renderPass) != VK_SUCCESS){
        PRINT("FAILED to create render pass!\n");
    }else{
        PRINT("successfully created render pass!\n");
    }




}

void create_descriptor_set_layout(vkTri* tri, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutBinding* bindings, u32 binding_count){
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = binding_count;
    layoutInfo.pBindings = bindings;

    if(vkCreateDescriptorSetLayout(tri->logicalDevice, &layoutInfo, nullptr, layout) != VK_SUCCESS){
        PRINT("FAILED to create %u binding descriptor set layout!\n", binding_count);
    }else{
        PRINT("successfully created %u binding descriptor set layout!\n", binding_count);
    }
}






void createDefaultEntityDescriptorSets(vkTri* tri){
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){layouts[frame] = tri->ubo_ssbo_descriptor_set_layout;}
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    
    if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, tri->default_entity_descriptorSets) != VK_SUCCESS){
        PRINT("failed to allocate default entity descriptor sets!\n");
    }else{
        PRINT("successfully allocated default entity descriptor sets!\n");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = tri->uniformBuffers[i]; //default uniform buffer for camera matrices
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorBufferInfo ssboInfo = {};
        ssboInfo.buffer = tri->perEntityShaderStorageBuffers[i]; //default uniform buffer for camera matrices
        ssboInfo.offset = 0;
        ssboInfo.range = (sizeof(PerEntitySSBO) * MAX_SSBO_ENTITIES);


        VkWriteDescriptorSet descriptorWrites[2] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = tri->default_entity_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = tri->default_entity_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &ssboInfo;


        vkUpdateDescriptorSets(tri->logicalDevice, 2, descriptorWrites, 0, nullptr);
    }



}



void createComputePipeline(vkTri* tri){
    char comp_shader[MAX_SHADER_SIZE];
    size_t comp_shader_size = MAX_SHADER_SIZE;
    debug_file debugFile = Win32ReadFileToGivenBuffer("C:/labor/shaders/particle_comp.spv", comp_shader, &comp_shader_size);

    VkShaderModule comp_shader_module = createShaderModule(tri->logicalDevice, comp_shader, debugFile.filesize.QuadPart);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = comp_shader_module;
    computeShaderStageInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &tri->computeDescriptorSetLayout;

    if(vkCreatePipelineLayout(tri->logicalDevice, &pipelineLayoutInfo, nullptr, &tri->computePipelineLayout) != VK_SUCCESS){
        PRINT("FAILED to create compute pipeline layout!\n");
    }else{
        PRINT("successfully created compute pipeline layout!\n");
    }

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = tri->computePipelineLayout;
    pipelineInfo.stage = computeShaderStageInfo;

    if(vkCreateComputePipelines(tri->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &tri->computePipeline) != VK_SUCCESS){
        PRINT("failed to create compute pipeline!\n");
    }else{
        PRINT("successfully created compute pipeline!\n");
    }

        vkDestroyShaderModule(tri->logicalDevice, comp_shader_module, nullptr);

}

void recompile_shaders(vkTri* tri){
    //currently handles all sets of shaders
    EXECUTE_BAT("C:\\labor\\shaders", "C:\\labor\\shaders\\compile.bat");
}

void createVertFragStageInfo(VkPipelineShaderStageCreateInfo* stageInfo, VkShaderModule vertShader, VkShaderModule fragShader){
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShader;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr; //can use this to set constants for more efficient shader logic. how do we do that?

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShader;
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.pSpecializationInfo = nullptr; 

    stageInfo[0] = vertShaderStageInfo;
    stageInfo[1] = fragShaderStageInfo;
}


VkPipelineColorBlendAttachmentState createColorBlendAttachment(bool transparent = false){
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if(!transparent){
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }else{
        //for alpha blending
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 
    }
    return colorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo createPipelineColorBlend(VkPipelineColorBlendAttachmentState& colorBlendAttachment){

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; //optional
    colorBlending.blendConstants[1] = 0.0f; //optional
    colorBlending.blendConstants[2] = 0.0f; //optional
    colorBlending.blendConstants[3] = 0.0f; //optional

    return colorBlending;
}

VkPipelineRasterizationStateCreateInfo createRasterizer(VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE){

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; //using depth clamping requires enabling a GPU feature
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // VK_POLYGON_MODE_FILL  //fill the area of the polygon with fragments
    // VK_POLYGON_MODE_LINE  //polygon edges are drawn as lines
    // VK_POLYGON_MODE_POINT //polygon vertices are drawn as points
    rasterizer.lineWidth = 1.0f; //thicker lines required enabling the wideLines GPU feature
    //TODO:
    //BACK FACE CULLING DISABLED FOR TESTING
    rasterizer.cullMode  = VK_CULL_MODE_BACK_BIT;  
    // rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.frontFace = frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  //optional
    rasterizer.depthBiasClamp = 0.0f;           //optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     //optional
    return rasterizer;
}

VkPipelineMultisampleStateCreateInfo createMultiSampler(VkSampleCountFlagBits samples){

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType     = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    // multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.rasterizationSamples  = samples;
    multisampling.minSampleShading      = 1.0f;
    multisampling.pSampleMask   = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    return multisampling;
}


VkPipelineDepthStencilStateCreateInfo createDepthStencil(VkBool32 depthWriteEnable = VK_TRUE){
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;//dont write to depth buffer
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //lower depth == closer
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;//optional
    depthStencil.maxDepthBounds = 0.0f;//optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};//optional
    depthStencil.back = {};//optional
    return depthStencil;
}





void create2StagePipeline(vkTri* tri, char* vert_shader, char* frag_shader, VkVertexInputBindingDescription* bindingDescription,
                          VkVertexInputAttributeDescription* attributeDescriptions, u32 attribute_count, VkPrimitiveTopology topology,
                          VkSampleCountFlagBits samples, VkRenderPass renderPass, VkDescriptorSetLayout* descriptorSetLayout, VkPipelineLayout* pipelineLayout, VkPipeline* pipeline,
                          bool transparent = false, u32 push_constant_range_count = 0, VkPushConstantRange* push_constant_range = nullptr,
                          VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, VkBool32 depthWriteEnable = VK_TRUE){



    // char vert_shader_storage[MAX_SHADER_SIZE];moved to vkTri struct to minimize stack allocation
    // char frag_shader_storage[MAX_SHADER_SIZE];moved to vkTri struct to minimize stack allocation
    size_t max_vert_shader_size = MAX_SHADER_SIZE;
    size_t max_frag_shader_size = MAX_SHADER_SIZE;
    debug_file vert_file = Win32ReadFileToGivenBuffer(vert_shader, tri->vert_buffer, &max_vert_shader_size);
    debug_file frag_file = Win32ReadFileToGivenBuffer(frag_shader, tri->frag_buffer, &max_frag_shader_size);

    VkShaderModule vert_shader_module = createShaderModule(tri->logicalDevice, tri->vert_buffer, vert_file.filesize.QuadPart);
    VkShaderModule frag_shader_module = createShaderModule(tri->logicalDevice, tri->frag_buffer, frag_file.filesize.QuadPart);


    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    createVertFragStageInfo(shaderStages, vert_shader_module, frag_shader_module);



    u32 dynamicStateSize = 2;
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStateSize; 
    dynamicState.pDynamicStates = dynamicStates;


    //vertex buffer data (empty for now since we put the vertices directly in the vertex shader)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attribute_count;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    //what kind of geometry will be drawn from the vertices (point/line/strip lists), and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;



    VkPipelineRasterizationStateCreateInfo rasterizer = createRasterizer(frontFace);
    VkPipelineMultisampleStateCreateInfo multisampling = createMultiSampler(samples);
    VkPipelineDepthStencilStateCreateInfo depthStencil = createDepthStencil(depthWriteEnable);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = createColorBlendAttachment(transparent);
    VkPipelineColorBlendStateCreateInfo colorBlending = createPipelineColorBlend(colorBlendAttachment);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = push_constant_range_count;
    pipelineLayoutInfo.pPushConstantRanges = push_constant_range;

    if(vkCreatePipelineLayout(tri->logicalDevice, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS){
        PRINT("FAILED to create pipeline layout!\n");
    }else{
        PRINT("successfully created pipeline layout!\n");
    }


    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState  = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState    = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState  = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState   = &colorBlending;
    pipelineInfo.pDynamicState  = &dynamicState;
    pipelineInfo.pDepthStencilState  = &depthStencil;

    pipelineInfo.layout = *pipelineLayout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //optional
    pipelineInfo.basePipelineIndex  = -1;             //optional

    if(vkCreateGraphicsPipelines(tri->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline) != VK_SUCCESS){
        PRINT("FAILED to create graphics pipeline!\n");
    }else{
        PRINT("successfully created graphics pipeline!\n");
    }


    vkDestroyShaderModule(tri->logicalDevice, frag_shader_module, nullptr);
    vkDestroyShaderModule(tri->logicalDevice, vert_shader_module, nullptr);

}





void createFramebuffers(vkTri* tri){

    for(size_t i = 0; i < tri->swapChainImageCount; i++){
#if USE_MSAA
            VkImageView attachments[3] = {tri->colorImageView, tri->depthImageView, tri->swapChainImageViews[i]};
            u32 attachmentCount = 3;
#else
            VkImageView attachments[2] = {tri->swapChainImageViews[i], tri->depthImageView};
            u32 attachmentCount = 2;
#endif

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = tri->renderPass;
        framebufferInfo.attachmentCount = attachmentCount;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = tri->swapChainExtent.width;
        framebufferInfo.height = tri->swapChainExtent.height;
        framebufferInfo.layers = 1;
    
        if(vkCreateFramebuffer(tri->logicalDevice, &framebufferInfo, nullptr, &tri->swapChainFramebuffers[i]) != VK_SUCCESS){
            PRINT("FAILED to create framebuffer!\n");
        }else{
            PRINT("successfully created framebuffer!\n");
        }
    }



}

void createTransferCommandPool(vkTri* tri){
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(tri, tri->physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if(queueFamilyIndices.transfer_has_value){
        poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
    }else{
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily;
    }
    
    if(vkCreateCommandPool(tri->logicalDevice, &poolInfo, NULL, &tri->transferCommandPool) != VK_SUCCESS){
        PRINT("FAILED to create transfer command pool!\n");
    } else {
        PRINT("Successfully created transfer command pool!\n");
    }

}



void createCommandPool(vkTri* tri){
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(tri, tri->physicalDevice);

    //graphics command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily;

    if(vkCreateCommandPool(tri->logicalDevice, &poolInfo, nullptr, &tri->commandPool) != VK_SUCCESS){
        PRINT("FAILED to create command pool!\n");
    }else{
        PRINT("successfully created command pool!\n");
    }

    // //transfer command pool
    // VkCommandPoolCreateInfo transferPoolInfo = {};
    // transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // transferPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;

    // if(vkCreateCommandPool(tri->logicalDevice, &transferPoolInfo, nullptr, &tri->transferCommandPool) != VK_SUCCESS){
    //     PRINT("FAILED to create transfer command pool!\n");
    // }else{
    //     PRINT("successfully created transfer command pool!\n");
    // }


}


VkCommandBuffer beginSingleTimeCommands(vkTri* tri){
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // allocInfo.commandPool = tri->transferCommandPool;
    allocInfo.commandPool = tri->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(tri->logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(vkTri* tri, VkCommandBuffer commandBuffer){
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // vkQueueSubmit(tri->transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    // vkQueueWaitIdle(tri->transferQueue);
    vkQueueSubmit(tri->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(tri->graphicsQueue);


    // vkFreeCommandBuffers(tri->logicalDevice, tri->transferCommandPool, 1, &commandBuffer);
    vkFreeCommandBuffers(tri->logicalDevice, tri->commandPool, 1, &commandBuffer);
}


bool hasStencilComponent(VkFormat format){
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
}

void transitionImageLayout(vkTri* tri, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(tri);


    QueueFamilyIndices indices = findQueueFamilies(tri, tri->physicalDevice);


    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(hasStencilComponent(format)){
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }else{
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    VkPipelineStageFlags sourceStage = {};
    VkPipelineStageFlags destinationStage = {};
    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    }else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    }else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }else{
        PRINT("unsupported layout transition!\n");
    }






    vkCmdPipelineBarrier(commandBuffer, 
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    


    endSingleTimeCommands(tri, commandBuffer);
}

void copyBufferToImage(vkTri* tri, VkBuffer buffer, VkImage image, u32 width, u32 height){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(tri);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};


    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);




    endSingleTimeCommands(tri, commandBuffer);
}


void copyBuffer(vkTri* tri, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(tri);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;//optional
    copyRegion.dstOffset = 0;//optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
   

    endSingleTimeCommands(tri, commandBuffer);


}




void createImage(vkTri* tri, u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory){
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = (u32)width;
    imageInfo.extent.height = (u32)height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;

    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.samples = numSamples;
    imageInfo.flags = 0;//optional


    

    if(vkCreateImage(tri->logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS){
        PRINT("FAILED to create image!\n");
    }else{
        PRINT("successfully created image!\n");
    }


    //allocate memory for the image
    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(tri->logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(tri, memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(tri->logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS){
        PRINT("failed to allocate image memory!\n");
    }else{
        PRINT("successfully allocated image memory!\n");
    }

    vkBindImageMemory(tri->logicalDevice, image, imageMemory, 0);

}


void generateMipmaps(vkTri* tri, VkImage image, VkFormat imageFormat, s32 texWidth, s32 texHeight, u32 mipLevels){

    //check if the image format supports linear blitting
    VkFormatProperties formatProperties = {};
    vkGetPhysicalDeviceFormatProperties(tri->physicalDevice, imageFormat, &formatProperties);

    if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)){
        PRINT("texture image format does not support linear blitting!\n");
        return;
    }else{
        PRINT("texture image format supports linear blitting!\n");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(tri);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    s32 mipWidth = texWidth;
    s32 mipHeight = texHeight;
    for(u32 i = 1; i < mipLevels; i++){
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1 , 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if(mipWidth > 1) mipWidth  *= 0.5f;
        if(mipHeight > 1)mipHeight *= 0.5f;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(tri, commandBuffer);


}



//TextureInfo texInfo = {};
//load_texture_pixels("poopoo", texInfo);
unsigned char* load_texture_pixels(char* filename, TextureInfo& texInfo){
    stbi_uc* pixels = stbi_load(filename, &texInfo.texWidth, &texInfo.texHeight, &texInfo.texChannels, STBI_rgb_alpha);
    if(texInfo.texChannels == 3)texInfo.texChannels = 4; //increase to 4 to allow for enough size, Vulkan increases to 4 channels if there are only 3
    if(!pixels){
        PRINT("failed to load texture image!\n");
    }else{
        PRINT("successfully loaded texture image!\n");
    }
    texInfo.mipLevels = (u32)log2_floor(max(texInfo.texWidth, texInfo.texHeight)) + 1;

    return pixels;
}
//create texture inbetween here
//create_texture_image(tri, pixels, VK_FORMAT_R8G8B8A8_SRGB);
void free_texture_pixels(unsigned char* pixels){
    stbi_image_free(pixels);
}

void create_texture_image(vkTri* tri, unsigned char* pixels, VkFormat format, TextureInfo& texInfo){


    PRINT("mipLevels: %u, texWidth: %u, texHeight: %u\n", texInfo.mipLevels, texInfo.texWidth, texInfo.texHeight);

    VkDeviceSize imageSize = texInfo.texWidth * texInfo.texHeight * texInfo.texChannels;
   
        
    VkBuffer stagingBuffer = {};
    VkDeviceMemory stagingBufferMemory = {};

    createBuffer(tri, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(tri->logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(tri->logicalDevice, stagingBufferMemory);

                                                                                                            //src for mipLevel blit operations
    createImage(tri, texInfo.texWidth, texInfo.texHeight, texInfo.mipLevels, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texInfo.textureImage, texInfo.textureImageMemory);

    transitionImageLayout(tri, texInfo.textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texInfo.mipLevels);
    
    // transitionImageLayout(tri, tri->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tri->mipLevels);
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    transitionImageLayout(tri, texInfo.textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texInfo.mipLevels);
    copyBufferToImage(tri, stagingBuffer, texInfo.textureImage, (u32)texInfo.texWidth, (u32)texInfo.texHeight);

    generateMipmaps(tri, texInfo.textureImage, format, texInfo.texWidth, texInfo.texHeight, texInfo.mipLevels);

    vkDestroyBuffer(tri->logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(tri->logicalDevice, stagingBufferMemory, nullptr);

}




void createShaderStorageBuffers(vkTri* tri){
    size_t memSize = (sizeof(particle) * MAX_PARTICLES);
    particle* heap_particles = (particle*)plat_alloc_mem(memSize); //test to see what happens when we dealloc in the same function
    // particle particles[MAX_PARTICLES] = {};
    //initialize particles
    u32 rngState = 0;
    u32 seed = 12345;
    rng_seed(&rngState, seed);

    for(int i = 0; i < MAX_PARTICLES; i++){
        float next_float = rng_next_f32(&rngState);
        float rad = 0.25f * fast_sqrtf(next_float);
        // PRINT("rngState: %u\n", rngState);?
        float theta = next_float * TWOPI;
        float x = rad * fast_cosf(theta) * (float)(g_width / g_height);
        float y = rad * fast_sinf(theta);
        vec2 pos = {x, y};
        // PRINT("pos: %f %f\n", pos.x, pos.y);
        vec2 vel = vec2_scale(vec2_normalize(pos), 0.00025f);
        tri->particles[i].pos = pos;
        tri->particles[i].velocity = vel;
        float r = next_float;
        float g = rng_next_f32(&rngState);
        float b = rng_next_f32(&rngState);
        tri->particles[i].color = vec4_create(vec3_create(r, g, b), 1.0f);
        // PRINT("color: %f %f %f\n", r, g, b);

    }

    VkDeviceSize bufferSize = memSize;


    //create staging buffer to upload data to gpu
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(tri, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(tri->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, tri->particles, (size_t)bufferSize);
    vkUnmapMemory(tri->logicalDevice, stagingBufferMemory);

    //copy initial particle data to all storage buffers
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        createBuffer(tri, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tri->shaderStorageBuffers[i], tri->shaderStorageBuffersMemory[i]);
        copyBuffer(tri, stagingBuffer, tri->shaderStorageBuffers[i], bufferSize);
        // vkMapMemory(tri->logicalDevice, tri->shaderStorageBuffersMemory[i], 0, bufferSize, 0, &tri->shaderStorageBuffersMapped[i]);
    }

    vkDestroyBuffer(tri->logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(tri->logicalDevice, stagingBufferMemory, nullptr);

    plat_dealloc_mem(heap_particles, memSize);

}


void mapPersistentBuffer(vkTri* tri, size_t size, VkBufferUsageFlags flags, VkBuffer* buffer, VkDeviceMemory* memory, void** mapped){
    VkDeviceSize bufferSize = size;
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        createBuffer(tri, bufferSize, flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer[i], memory[i]);
        vkMapMemory(tri->logicalDevice, memory[i], 0, bufferSize, 0, &mapped[i]);
    }
}

void mapSinglePersistentBuffer(vkTri* tri, size_t size, VkBufferUsageFlags flags, VkBuffer* buffer, VkDeviceMemory* memory, void** mapped){
    VkDeviceSize bufferSize = size;
    createBuffer(tri, bufferSize, flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *buffer, *memory);
    vkMapMemory(tri->logicalDevice, *memory, 0, bufferSize, 0, mapped);
}



void createDepthResources(vkTri* tri){
    VkFormat depthFormat = findDepthFormat(tri);

    createImage(tri, tri->swapChainExtent.width, tri->swapChainExtent.height, 1, tri->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tri->depthImage, tri->depthImageMemory);
    tri->depthImageView = createImageView(tri, tri->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    //dont need to do this because we handle it in the render pass, but for exercise i will do it
    transitionImageLayout(tri, tri->depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);


}



void createColorResources(vkTri* tri){
    VkFormat colorFormat = tri->swapChainImageFormat;
    createImage(tri, tri->swapChainExtent.width, tri->swapChainExtent.height, 1, tri->msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tri->colorImage, tri->colorImageMemory); 
    tri->colorImageView = createImageView(tri, tri->colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}





void createMeshBuffer(vkTri* tri, void* meshData, size_t size, VkBufferUsageFlagBits usage, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
    VkDeviceSize bufferSize = size;

    VkBuffer stagingBuffer = {};
    VkDeviceMemory stagingBufferMemory = {};

    createBuffer(tri, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(tri->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
 
    memcpy(data, meshData, (size_t)bufferSize);

    vkUnmapMemory(tri->logicalDevice, stagingBufferMemory);

    createBuffer(tri, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);

    copyBuffer(tri, stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(tri->logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(tri->logicalDevice, stagingBufferMemory, nullptr);
}


void createComputeCommandBuffers(vkTri* tri){
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tri->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if(vkAllocateCommandBuffers(tri->logicalDevice, &allocInfo, tri->compute_commandBuffers) != VK_SUCCESS){
        PRINT("failed to allocate compute command buffers!\n");
    }else{
        PRINT("successfully allocated compute command buffers!\n");
    }
}


void createCommandBuffers(vkTri* tri){
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tri->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if(vkAllocateCommandBuffers(tri->logicalDevice, &allocInfo, tri->commandBuffers) != VK_SUCCESS){
        PRINT("FAILED to allocate command buffers!\n");
    }else{
        PRINT("successfully allocated command buffers!\n");
    }
}

void drawEntities(vkTri* tri, VkCommandBuffer commandBuffer){
    size_t& count = tri->RenderCommandData.entityDrawCount;

    tri->RenderCommandData.entityDrawCommands[count].mesh_type = MeshTypes::mesh_Axes;
    tri->RenderCommandData.entityDrawCommands[count].topology_type = TopologyTypes::topology_lines;
    tri->RenderCommandData.entityDrawCommandsSSBO[count].model = tri->axis_ubo.model;
    tri->RenderCommandData.entityDrawCommandsSSBO[count++].color = vec4_create(1.0f, 1.0f, 1.0f, 1.0f);

    memcpy(tri->perEntityShaderStorageBuffersMapped[tri->currentFrame], tri->RenderCommandData.entityDrawCommandsSSBO, sizeof(PerEntitySSBO) * count);
    //copy over the SSBO

    //setup entity rendering pass, start with lines
    tri->current_topology_type = TopologyTypes::topology_lines;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->graphics_line_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->line_pipelineLayout, 0, 1, &tri->default_entity_descriptorSets[tri->currentFrame], 0, nullptr);
    VkPipelineLayout layout = tri->line_pipelineLayout;
    u32* index_count_array = tri->entityMeshLineIndexCount;
    VkBuffer* index_buffer = tri->entityMeshLineIndices;

    VkDeviceSize entity_offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[MeshTypes::mesh_cube], entity_offsets);
    vkCmdBindIndexBuffer(commandBuffer,  index_buffer[MeshTypes::mesh_cube], 0, VK_INDEX_TYPE_UINT32);
    tri->current_mesh_type = mesh_cube;
    
    #if 1
    for(u32 i = 0; i < count; i++){
        TopologyTypes topology_type = tri->RenderCommandData.entityDrawCommands[i].topology_type;
        MeshTypes mesh_type = tri->RenderCommandData.entityDrawCommands[i].mesh_type;
        if(topology_type != tri->current_topology_type){
            switch(topology_type){
                case TopologyTypes::topology_lines:{
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->graphics_line_pipeline);
                    layout = tri->line_pipelineLayout;
                    index_count_array = tri->entityMeshLineIndexCount;
                    index_buffer = tri->entityMeshLineIndices;
                                        
                    tri->current_mesh_type = mesh_type;
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[mesh_type], entity_offsets);
                    vkCmdBindIndexBuffer(commandBuffer,  index_buffer[mesh_type], 0, VK_INDEX_TYPE_UINT32);
                }break;
                case TopologyTypes::topology_triangles:{
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->default_entity_pipeline);
                    layout = tri->default_entity_pipelineLayout;
                    index_count_array = tri->entityMeshIndexCount;
                    index_buffer = tri->entityMeshIndices;
                    
                    tri->current_mesh_type = mesh_type;
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[mesh_type], entity_offsets);
                    vkCmdBindIndexBuffer(commandBuffer,  index_buffer[mesh_type], 0, VK_INDEX_TYPE_UINT32);
                }break;
            }
            tri->current_topology_type = topology_type;//set new topology type for current draw call
        }
        if(mesh_type != tri->current_mesh_type){
            tri->current_mesh_type = mesh_type;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[mesh_type], entity_offsets);
            vkCmdBindIndexBuffer(commandBuffer,  index_buffer[mesh_type], 0, VK_INDEX_TYPE_UINT32);
        }
    
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &i);
        vkCmdDrawIndexed(commandBuffer, (u32)index_count_array[mesh_type], 1, 0, 0, 0);
    
    }
    #endif

    tri->RenderCommandData.entityDrawCount = 0;

}


void drawSkeletalMeshedEntities(vkTri* tri, VkCommandBuffer commandBuffer){
    u32& count = tri->RenderCommandData.skelMeshDrawCount;

    //setup entity rendering pass, start with lines
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->skeletal_mesh_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->skeletal_mesh_pipeline_layout, 0, 1, &tri->skeletal_mesh_descriptor_sets[tri->currentFrame], 0, nullptr);
    VkDeviceSize entity_offsets[] = {0};
    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->skeletalMeshVertices, entity_offsets);
    vkCmdBindIndexBuffer(commandBuffer,  tri->skeletalMeshIndices, 0, VK_INDEX_TYPE_UINT32);


    #if 1
    for(u32 i = 0; i < count; i++){
        skeletal_mesh_command command = tri->RenderCommandData.skeletalMeshCommands[i];
    
        PerEntitySSBO push = {};
        push.model = command.model;
        push.color = command.color;
        vkCmdPushConstants(commandBuffer, tri->skeletal_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PerEntitySSBO), &push);
        vkCmdDrawIndexed(commandBuffer, (u32)command.indexCount, 1, command.indexOffset, 0, 0);
    
    }
    #endif

    tri->RenderCommandData.skelMeshDrawCount = 0;

}



void drawScreenElementShaderTest(vkTri* tri, VkCommandBuffer commandBuffer){

    size_t& count = tri->RenderCommandData.screenElementDrawCount;


    //setup entity rendering pass, start with lines
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_test_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_test_pipeline_layout, 0, 1, &tri->screen_space_test_descriptor_sets[tri->currentFrame], 0, nullptr);

    VkDeviceSize screen_offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[MeshTypes::mesh_uiBox], screen_offsets);

    for(u32 i = 0; i < count; i++){
    
        ScreenPushConstants screen_push_constants = {};
        screen_push_constants = tri->RenderCommandData.screenElementDrawCommands[i].push;


        if(screen_push_constants.misc.x){
                VkRect2D scissor = {};
                scissor.offset = {(int)tri->RenderCommandData.screenElementDrawCommands[i].scissor.x, (int)tri->RenderCommandData.screenElementDrawCommands[i].scissor.y};
                scissor.extent = {(u32)tri->RenderCommandData.screenElementDrawCommands[i].scissor.z, (u32)tri->RenderCommandData.screenElementDrawCommands[i].scissor.w};
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }



        vkCmdPushConstants(commandBuffer, tri->screen_space_test_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);

        vkCmdDraw(commandBuffer, (u32)tri->entityMeshIndexCount[MeshTypes::mesh_uiBox] , 1, 0, 0);
    
                //reset cliprect
        if(screen_push_constants.misc.x){
            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = tri->swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }
    }


    tri->RenderCommandData.screenElementDrawCount = 0;
}




// void drawScreenElements(vkTri* tri, VkCommandBuffer commandBuffer){
//     size_t& count = tri->RenderCommandData.screenElementDrawCount;


//     //setup entity rendering pass, start with lines
//     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_texture_pipeline);
//     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_texture_pipeline_layout, 0, 1, &tri->screen_space_texture_descriptor_sets[tri->currentFrame], 0, nullptr);

//     VkDeviceSize screen_offsets[] = {0};

//     tri->current_mesh_type = MeshTypes::mesh_uiBox;
//     vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[tri->current_mesh_type], screen_offsets);

//     for(u32 i = 0; i < count; i++){
//         MeshTypes mesh_type = tri->RenderCommandData.screenElementDrawCommands[i].mesh_type;
        
//         if(mesh_type != tri->current_mesh_type){
//             tri->current_mesh_type = mesh_type;
//             vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[mesh_type], screen_offsets);
//         }
    
//         ScreenPushConstants screen_push_constants = {};
//         screen_push_constants = tri->screenElementPushConstants[i];
        
//         vkCmdPushConstants(commandBuffer, tri->screen_space_texture_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);

//         vkCmdDraw(commandBuffer, (u32)tri->entityMeshIndexCount[mesh_type] , 1, 0, 0);
    
//     }


//     tri->RenderCommandData.screenElementDrawCount = 0;

// }

void recordCommandBuffer(vkTri* tri, VkCommandBuffer commandBuffer, u32 imageIndex){
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//optional
    beginInfo.pInheritanceInfo = nullptr;//optional

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
        PRINT("FAILED to begin recording command buffer!\n");

    }

    
    vkCmdResetQueryPool(commandBuffer, tri->timestampQueryPool[tri->currentFrame], 0, 2);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, tri->timestampQueryPool[tri->currentFrame], 0);

    // else{
        // PRINT("successfully began recording command buffer!\n");
    // }

    // #if UPDATE_DYNAMIC_TEXTURE
    for(u32 i = 0; i < tri->RenderCommandData.screenSpaceTextureUpdateCommandCount; i++){
        Assert(i < 4);//cant have more than 4 commands. can only update each texture ONCE per frame
        texture_update_command command = tri->RenderCommandData.screenSpaceTextureUpdateCommands[i];
        update_dynamic_texture(tri, command.textureMemory, command.textureWidth, command.textureHeight, tri->dynamic_texture_staging_buffer[i][0], tri->dynamic_texture_staging_buffer_memory[i][0], tri->dynamic_texture_staging_buffer_mapped[i][0] , tri->dynamic_image[i]);
        int fuckTheDebugger = 0;
    }
    tri->RenderCommandData.screenSpaceTextureUpdateCommandCount = 0;
    // #endif

    //offscreen render target
    #if 1
    { 
        
        

        VkRenderPassBeginInfo offscreenRenderPassInfo = {};
        offscreenRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        offscreenRenderPassInfo.renderPass = tri->offscreen_render_pass;
        offscreenRenderPassInfo.framebuffer = tri->offscreen_frame_buffer;
        offscreenRenderPassInfo.renderArea.offset = {0, 0};
        offscreenRenderPassInfo.renderArea.extent = {512, 512};
    
        VkClearValue clearValues[2] = {}; 
        clearValues[0].color = {0.0, 0.0, 0.0, 0.0};
        clearValues[1].depthStencil = {1, 0};
        offscreenRenderPassInfo.clearValueCount = 2;
        offscreenRenderPassInfo.pClearValues = clearValues;
    
        
        vkCmdBeginRenderPass(commandBuffer, &offscreenRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->offscreen_pipeline);
    
    
        //viewports define the transformation from the image to the framebuffer
        //viewports and scissors
        VkViewport viewport = {};
        viewport.x  = 0.0f;
        viewport.y  = 0.0f;
        viewport.width  = (float)512;
        viewport.height = (float)512;
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;
            //viewports define the transformation from the image to the framebuffer
        
        //scissor rectangles define in which regions pixels will actually be stored

        //scissor rectangles define in which regions pixels will actually be stored
        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {512, 512};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


        UniformBufferObject ubo = {};
        memcpy(tri->uniformBuffersMapped[tri->currentFrame], &tri->axis_ubo, sizeof(ubo));

        // u32 index = 0;


        // #if USE_MODEL
        //     VkBuffer vertexBuffers[] = {tri->vertexBuffer};
        // #else
        //     VkBuffer vertexBuffers[] = {tri->default_quad_vertex_buffer};
        // #endif
        // VkDeviceSize offsets[] = {0};
        // vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        // vkCmdBindIndexBuffer(commandBuffer, tri->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->offscreen_pipeline_layout, 0, 1, &tri->graphics_descriptorSets[tri->currentFrame], 0, nullptr);
        // // vkCmdDraw(commandBuffer, (u32)ArrayCount(tri_vertices), 1, 0, 0);

        // #if USE_MODEL
        // vkCmdDrawIndexed(commandBuffer, (u32)tri->model_index_count, 1, 0, 0, 0);
        // #else
        // vkCmdDrawIndexed(commandBuffer, (u32)ArrayCount(indices), 1, 0, 0, 0);
        // #endif
        // drawSkeletalMeshedEntities(tri, commandBuffer);


        //setup entity rendering pass, start with lines
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->skeletal_mesh_pipeline_layout, 0, 1, &tri->skeletal_mesh_descriptor_sets[tri->currentFrame], 0, nullptr);
        VkDeviceSize entity_offsets[] = {0};
        
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->skeletalMeshVertices, entity_offsets);
        vkCmdBindIndexBuffer(commandBuffer,  tri->skeletalMeshIndices, 0, VK_INDEX_TYPE_UINT32);


        #if 1
        u32& count = tri->RenderCommandData.skelMeshDrawCount;

        for(u32 i = 0; i < count; i++){
            skeletal_mesh_command command = tri->RenderCommandData.skeletalMeshCommands[i];
        
            PerEntitySSBO push = {};
            push.model = command.model;
            push.color = command.color;
            vkCmdPushConstants(commandBuffer, tri->skeletal_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PerEntitySSBO), &push);
            vkCmdDrawIndexed(commandBuffer, (u32)command.indexCount, 1, command.indexOffset, 0, 0);
        
        }
        #endif


        }



    vkCmdEndRenderPass(tri->commandBuffers[tri->currentFrame]);
    #endif

    VkClearValue clearValues[2] = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = tri->renderPass;
    renderPassInfo.framebuffer = tri->swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = tri->swapChainExtent;
    
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;



    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    //viewports define the transformation from the image to the framebuffer
    //viewports and scissors
    VkViewport viewport = {};
    viewport.x  = 0.0f;
    viewport.y  = 0.0f;
    viewport.width  = (float)tri->swapChainExtent.width;
    viewport.height = (float)tri->swapChainExtent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
        //viewports define the transformation from the image to the framebuffer
    
    //scissor rectangles define in which regions pixels will actually be stored

    //scissor rectangles define in which regions pixels will actually be stored
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = tri->swapChainExtent;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    UniformBufferObject ubo = {};
    memcpy(tri->uniformBuffersMapped[tri->currentFrame], &tri->axis_ubo, sizeof(ubo));
#if DRAW_SKYBOX
u64 skyboxDrawStart = __rdtsc();

    // SKYBOX
    if(tri->skybox_shader_loaded){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->skybox_pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->skybox_pipeline_layout, 0, 1, &tri->skybox_descriptor_sets[tri->currentFrame], 0, nullptr);
        VkDeviceSize test_offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->entityMeshVertices[MeshTypes::mesh_skybox], test_offsets);
        vkCmdBindIndexBuffer(commandBuffer, tri->entityMeshIndices[MeshTypes::mesh_skybox], 0, VK_INDEX_TYPE_UINT32);
    
    
        SkyboxPushConstants skyboxPushConstants = {};
        skyboxPushConstants.model = mat4_identity();//want the skybox to follow 0, which is always the camera position
        scale_mat4(skyboxPushConstants.model, 500);
        //use the color parameter to store other stuff, like elapsed time
        skyboxPushConstants.misc = {tri->elapsedTime, 0, 0, 0};    
        skyboxPushConstants.mouse = {(float)g_mouseX, (float)g_mouseY, (float)g_mouseDeltaX, (float)g_mouseDeltaY};
        skyboxPushConstants.viewRect = {(float)g_width, (float)g_height};
    
        vkCmdPushConstants(commandBuffer, tri->skybox_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPushConstants), &skyboxPushConstants);
    
    
        vkCmdDrawIndexed(commandBuffer, (u32)36, 1, 0, 0, 0);
        //END SKYBOX
        
    }
u64 skyboxDrawEnd = __rdtsc() - skyboxDrawStart;
// PRINT("skybox draw time: %zu\n", skyboxDrawEnd);

#endif

#if 0
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->graphicsPipeline);

    #if USE_MODEL
        VkBuffer vertexBuffers[] = {tri->vertexBuffer};
    #else
        VkBuffer vertexBuffers[] = {tri->default_quad_vertex_buffer};
    #endif
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, tri->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->pipelineLayout, 0, 1, &tri->graphics_descriptorSets[tri->currentFrame], 0, nullptr);
    // vkCmdDraw(commandBuffer, (u32)ArrayCount(tri_vertices), 1, 0, 0);

    #if USE_MODEL
    vkCmdDrawIndexed(commandBuffer, (u32)tri->model_index_count, 1, 0, 0, 0);
    #else
    vkCmdDrawIndexed(commandBuffer, (u32)ArrayCount(indices), 1, 0, 0, 0);
    #endif
#endif

    // //particle commands
#if USE_COMPUTE
    VkDeviceSize particle_offsets[] = {0};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->particle_pipeline);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tri->shaderStorageBuffers[tri->currentFrame], particle_offsets);
    vkCmdDraw(commandBuffer, MAX_PARTICLES, 1, 0, 0);
    //END PARTICLE COMMANDS
#endif


#if 1

//process chunk destruction commands
for(u32 i = 0; i < tri->RenderCommandData.chunkDestroyCommandCount; i++){
    chunk_destroy_command command = tri->RenderCommandData.chunkDestroyCommands[i];
    tri->chunkToSSBOMap[command.chunkID].faceCount = 0;
    if(tri->chunkToSSBOMap[command.chunkID].ssboIndex == NULL_CHUNK)continue;
    //give ssbo slot back to the free list
    PRINT("[VULKAN] removing chunkID: %u ssboID: %u, freeCount: %u\n", command.chunkID, tri->chunkToSSBOMap[command.chunkID].ssboIndex, tri->voxelSSBOFreeCount+1); 
    tri->voxelSSBOFreeIndices[tri->voxelSSBOFreeCount++] = tri->chunkToSSBOMap[command.chunkID].ssboIndex;
    tri->activeChunkSSBOs--;

    tri->chunkToSSBOMap[command.chunkID].ssboIndex = NULL_CHUNK;

}

tri->RenderCommandData.chunkDestroyCommandCount = 0;
//process chunk creation commands
u64 chunkUpdateStart = __rdtsc();
bool chunksUpdated = false;
#if CHUNK_STAGING_BUFFER
tri->waitForTransfer = false;
if(tri->chunkCreateCommandCount){
    //wait for fence
    // vkWaitForFences(tri->logicalDevice, 1, &tri->transferFences[tri->currentFrame], VK_TRUE, UINT64_MAX);
    // vkResetFences(tri->logicalDevice, 1, &tri->transferFences[tri->currentFrame]);

    //wait for staging slice fence
    vkWaitForFences(tri->logicalDevice, 1, &tri->chunkStageFences[tri->chunkStageCurrentSlice], VK_TRUE, UINT64_MAX);
    vkResetFences  (tri->logicalDevice, 1, &tri->chunkStageFences[tri->chunkStageCurrentSlice]);

    //copy all data into staging buffer

    //record transfer command buffer for this frame
    // VkCommandBuffer cmdBuf = tri->transferCommandBuffers[tri->currentFrame];
    VkCommandBuffer cmdBuf = tri->transferCommandBuffers[tri->chunkStageCurrentSlice];
    vkResetCommandBuffer(cmdBuf, 0);
    VkCommandBufferBeginInfo transferBeginInfo = {};
    transferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    transferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuf, &transferBeginInfo);
    


    for(u32 i = 0; i < tri->chunkCreateCommandCount; i++){
        TIMED_BLOCK("VK CHUNK CREATE COMMANDS");
        chunksUpdated = true;
        chunk_create_command command = tri->chunkCreateCommands[i];
        if(tri->chunkToSSBOMap[command.chunkID].ssboIndex == NULL_CHUNK){
            //get ssbo slot from free list
            if(tri->voxelSSBOFreeCount > 0){
                u32 ssboIndex = tri->voxelSSBOFreeIndices[--tri->voxelSSBOFreeCount];
                tri->activeChunkSSBOs++;
                PRINT("[VULKAN] chunkID: %u tri->voxelSSBOFreeCount: %u , assigning SSBO %u \n", command.chunkID, tri->voxelSSBOFreeCount, ssboIndex);
                tri->chunkToSSBOMap[command.chunkID].ssboIndex = ssboIndex;
                int fuckTheDebugger = 0;
            }else{
                PRINT("[VULKAN] chunkID: %u tri->voxelSSBOFreeCount: %u  OUT OF FREE INDICES ERROR! \n", command.chunkID, tri->voxelSSBOFreeCount);
                Assert(!"OUT OF CHUNK SSBO INDICES!");

            }
        }
        tri->chunkToSSBOMap[command.chunkID].faceCount = command.faceCount;
        size_t byte_offset =  (tri->chunkToSSBOMap[command.chunkID].ssboIndex * 786432 * sizeof(FaceData)) + (tri->chunkStageCurrentSlice * tri->chunkStagingPerFrameSize);

        memcpy((u8*)tri->chunkStagingMapped + byte_offset, command.faceMemory, sizeof(FaceData) * command.faceCount);
        
        
        int fuckTheDebugger = 0;

    }

    VkBufferCopy copyRegion = {};
    size_t stagingOffset = tri->chunkStagingPerFrameSize * tri->chunkStageCurrentSlice;
    copyRegion.srcOffset = stagingOffset;
    copyRegion.dstOffset = stagingOffset;
    copyRegion.size = tri->chunk_draw_distance_volume * (64*64*32*6*sizeof(FaceData));
    vkCmdCopyBuffer(cmdBuf, tri->chunkStagingBuffer, tri->chunkDeviceSSBO, 1, &copyRegion);
    vkEndCommandBuffer(cmdBuf);
    //submit transfer command buffer and signal fence
    VkSubmitInfo transferSubmit = {};
    transferSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transferSubmit.commandBufferCount = 1;
    transferSubmit.signalSemaphoreCount = 1;
    tri->waitForTransfer = true;
    transferSubmit.pSignalSemaphores = &tri->transferCompleteSemaphores[tri->chunkStageCurrentSlice];
    transferSubmit.pCommandBuffers = &tri->transferCommandBuffers[tri->chunkStageCurrentSlice];
    // vkQueueSubmit(tri->transferQueue, 1, &submitInfo, tri->transferFences[tri->currentFrame]);
    vkQueueSubmit(tri->transferQueue, 1, &transferSubmit, tri->chunkStageFences[tri->chunkStageCurrentSlice]);


}


#else
for(u32 i = 0; i < tri->RenderCommandData.chunkCreateCommandCount; i++){
    TIMED_BLOCK("VK CHUNK CREATE COMMANDS");
    chunksUpdated = true;
    chunk_create_command command = tri->RenderCommandData.chunkCreateCommands[i];
    // Assert(tri->activeChunkSSBOs < tri->chunk_draw_distance_volume);
    // if(!command.edit){//not an edit, new chunk, give us an SSBO slot
    if(tri->chunkToSSBOMap[command.chunkID].ssboIndex == NULL_CHUNK){
        //get ssbo slot from free list
        if(tri->voxelSSBOFreeCount > 0){
            u32 ssboIndex = tri->voxelSSBOFreeIndices[--tri->voxelSSBOFreeCount];
            tri->activeChunkSSBOs++;
            PRINT("[VULKAN] chunkID: %u tri->voxelSSBOFreeCount: %u , assigning SSBO %u \n", command.chunkID, tri->voxelSSBOFreeCount, ssboIndex);
            // char temp[256];
            // u32 tempOffset = 0;
            // tempOffset = sPRINT(temp, "[VULKAN] ACTIVE CHUNKS: ");
            // for(u32 j = 0; j < tri->activeChunkSSBOs; j++){
            //     sPRINT("%2d, \n", temp);

            // }
            // PRINT("%s\n", temp);
            tri->chunkToSSBOMap[command.chunkID].ssboIndex = ssboIndex;
            int fuckTheDebugger = 0;
        }else{
            PRINT("[VULKAN] chunkID: %u tri->voxelSSBOFreeCount: %u  OUT OF FREE INDICES ERROR! \n", command.chunkID, tri->voxelSSBOFreeCount);
            Assert(!"OUT OF CHUNK SSBO INDICES!");

        }
    }
    // Assert(tri->chunkToSSBOMap[command.chunkID].ssboIndex != NULL_CHUNK);
    tri->chunkToSSBOMap[command.chunkID].faceCount = command.faceCount;
    size_t byte_offset =  (tri->chunkToSSBOMap[command.chunkID].ssboIndex * 786432 * sizeof(FaceData));

    if(byte_offset + (786432 * sizeof(FaceData)) > tri->required_chunk_ssbo_memory){
        __debugbreak();
    }   

    memcpy((u8*)tri->voxel_ssbo_mapped + byte_offset, command.faceMemory, sizeof(FaceData) * command.faceCount);
    // PRINT("chunkID: %u copied %u faces to ssboIndex: %u, facePointer: %p, chunkCreateCommandCount: %u\n", command.chunkID, command.faceCount, tri->chunkToSSBOMap[command.chunkID].ssboIndex, command.threadFaces, tri->chunkCreateCommandCount);
    
    
    int fuckTheDebugger = 0;

}
#endif
if(chunksUpdated){
    //max of 400,000 cycles here, significant slowdown happening somewhere else in the render logic
    // PRINT("chunk Update cycles: %zu\n", __rdtsc() - chunkUpdateStart);
}

#if DRAW_VOXEL_MESH
u64 voxDrawStart = __rdtsc();
  // voxel puller 
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->voxel_vertex_puller_pipeline);
    
        VoxelVertexPullerPushConstants voxel_vertex_puller_push_constants = {};
        VkDeviceSize voxel_offsets[] = {0};
        VkBuffer voxel_vertexBuffers[] = {tri->entityMeshVertices[MeshTypes::mesh_uiBox]};//just a placeholder, we dont need a vertex buffer
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, voxel_vertexBuffers, voxel_offsets);
        vkCmdBindIndexBuffer(commandBuffer, tri->face_index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->voxel_vertex_puller_pipeline_layout, 0, 1, &tri->voxel_vertex_puller_descriptor_sets[tri->currentFrame], 0, nullptr);

//process chunk draw commands
for(u32 i = 0; i < tri->RenderCommandData.chunkDrawCommandCount; i++){
        chunk_voxel_draw_command command = tri->RenderCommandData.chunkDrawCommands[i];
        voxel_vertex_puller_push_constants.vertexPullerSettings = {0,0,0,0}; 
        // voxel_vertex_puller_push_constants.model = tri->axis_ubo.model; 
        voxel_vertex_puller_push_constants.model = command.model;
        voxel_vertex_puller_push_constants.ssboIndex  = tri->chunkToSSBOMap[command.chunkID].ssboIndex * 786432;

        vkCmdPushConstants(commandBuffer, tri->voxel_vertex_puller_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VoxelVertexPullerPushConstants), &voxel_vertex_puller_push_constants);
    
        // vkCmdDraw(commandBuffer, (u32)ArrayCount(tri_vertices), 1, 0, 0);
        u32 face_count = 6;
        vkCmdDrawIndexed(commandBuffer, (u32)tri->chunkToSSBOMap[command.chunkID].faceCount * 6, 1, 0, 0, 0);
        // vkCmdDrawIndexed(commandBuffer, (u32)6 * 6, 1, 0, 0, 0);
}
u64 voxDrawEnd = __rdtsc() - voxDrawStart;
// PRINT("voxel draw time: %zu\n", voxDrawEnd);
#endif

tri->RenderCommandData.chunkCreateCommandCount = 0;

#endif
tri->RenderCommandData.chunkDrawCommandCount = 0;

// //end voxel puller test
     

    

    //LINE commands
    //DRAW AXIS
    
//make 2 different pipelines for opaque and then transparent meshes
//enable depth writing for opaque calls, draw those first
//for transparents, draw them after all opaques, disable depth writing, and sort them
#if DRAW_SKELETAL_ENTITY
    drawSkeletalMeshedEntities(tri, commandBuffer);
#endif

#if DRAW_ENTITY //should we create a separate buffer for transparent draws?
    drawEntities(tri, commandBuffer);
#endif

#if DRAW_WORLD_TEXT
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->world_space_text_pipeline);
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->world_space_text_pipeline_layout, 0, 1, &tri->world_space_text_descriptor_sets[tri->currentFrame], 0, nullptr);
worldSpaceText(tri, commandBuffer);
#endif
//ray tracer test
#if 0

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->test_ray_tracer_pipeline);
    
        TestRayTracerPushConstants tracer_push_constants = {};
        tracer_push_constants.cameraPos = {tri->camPos.x,tri->camPos.y,tri->camPos.z,0}; 
        tracer_push_constants.viewRect = {480, 360};    
        
        vkCmdPushConstants(commandBuffer, tri->test_ray_tracer_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TestRayTracerPushConstants), &tracer_push_constants);
    
        VkDeviceSize tracer_offsets[] = {0};
        VkBuffer screenQuadVertexBuffers[] = {tri->screenQuadVertexBuffer};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuadVertexBuffers, tracer_offsets);
        vkCmdBindIndexBuffer(commandBuffer, tri->test_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->test_ray_tracer_pipeline_layout, 0, 1, &tri->test_ray_tracer_descriptor_sets[tri->currentFrame], 0, nullptr);
        // vkCmdDraw(commandBuffer, (u32)ArrayCount(tri_vertices), 1, 0, 0);
    
        vkCmdDrawIndexed(commandBuffer, (u32)6, 1, 0, 0, 0);
        //end ray tracer test
#endif



ScreenSpaceUniformBufferObject screen_ubo = {};
screen_ubo.viewRect.x = (float)g_width;
screen_ubo.viewRect.y = (float)g_height;
screen_ubo.misc   =  {tri->elapsedTime, 0, 0, 0};    
screen_ubo.mouse  =  {(float)g_mouseX, (float)g_mouseY, (float)g_mouseDeltaX, (float)g_mouseDeltaY};
memcpy(tri->screen_space_uniform_buffers_mapped[tri->currentFrame], &screen_ubo, sizeof(screen_ubo));
    //END LINE COMMANDS
    //SCREEN SPACE UI TEST
#if DRAW_SCREEN_ELEMENTS
    
createScreenTextVertices(tri);

// u64 screenElementStart = __rdtsc();
// //old legacy path where we pushed the actual vertices to the shader
// // drawScreenElements(tri, commandBuffer);
// drawScreenElementShaderTest(tri, commandBuffer);
// u64 screenElementEnd = __rdtsc() - screenElementStart;
// // PRINT("text: %zu, element: %zu, total: %zu\n", screenTextEnd, screenElementEnd, screenTextEnd+screenElementEnd);

// //TODO: combine text into the same pass as ui elements
// u64 screenTextStart = __rdtsc();

// #if DRAW_SCREEN_TEXT
// vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline);
// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline_layout, 0, 1, &tri->screen_space_text_descriptor_sets[tri->currentFrame], 0, nullptr);
// screenSpaceText(tri, commandBuffer);
drawCombinedScreenElementsAndText(tri, commandBuffer);
// #endif 
// u64 screenTextEnd = __rdtsc() - screenTextStart;


#endif
// #if 0
//     // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline);
//     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline_layout, 0, 1, &tri->screen_space_text_descriptor_sets[tri->currentFrame], 0, nullptr);
//     VkDeviceSize screen_offsets[] = {0};


//     //draws the entire font atlas texture
//     ScreenPushConstants screen_push_constants = {};
//     screen_push_constants.textPosition = {240, 180}; 
//     screen_push_constants.textScale = {200, 200};    
//     screen_push_constants.textColor = {1.0f, 0.0f, 0.0f, 1.0f};    
//     screen_push_constants.texCoords = {0.0f, 0.0f, 1.0f, 1.0f};    
//     screen_push_constants.misc = {1,0,0,0};    
    
//     vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
//     VkBuffer uiBorderVertexBuffers[] = {tri->entityMeshVertices[MeshTypes::mesh_uiBorder]};
//     VkBuffer uiBoxVertexBuffers[] = {tri->entityMeshVertices[MeshTypes::mesh_uiBox]};
//     vkCmdBindVertexBuffers(commandBuffer, 0, 1, uiBoxVertexBuffers, screen_offsets);
//     // vkCmdDraw(commandBuffer, (u32)24, 1, 0, 0);
//     vkCmdDraw(commandBuffer, (u32)6, 1, 0, 0);
    


    
//     // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline);
//     // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_text_pipeline_layout, 0, 1, &tri->screen_space_text_descriptor_sets[tri->currentFrame], 0, nullptr);
//     // VkDeviceSize screen_offsets[] = {0};

//     // ScreenSpaceUniformBufferObject screen_ubo = {};
//     // screen_ubo.viewRect.x = (float)g_width;
//     // screen_ubo.viewRect.y = (float)g_height;
//     // memcpy(tri->screen_space_uniform_buffers_mapped[tri->currentFrame], &screen_ubo, sizeof(screen_ubo));
//     // ScreenPushConstants screen_push_constants = {};






//     //ITEM TEXTURE TEST
//     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_texture_pipeline);
//     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->screen_space_texture_pipeline_layout, 0, 1, &tri->screen_space_texture_descriptor_sets[tri->currentFrame], 0, nullptr);

//     // screen_push_constants.textPosition = {300, 180}; 
//     // screen_push_constants.textScale = {200, 200};    
//     // screen_push_constants.textColor = {1.0f, 0.0f, 0.0f, 1.0f};    
//     // screen_push_constants.texCoords = {0.0f, 0.0f, 1.0f, 1.0f};    
//     // screen_push_constants.drawTexture = 0;    
    
//     // vkCmdPushConstants(commandBuffer, tri->screen_space_text_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
//     // vkCmdBindVertexBuffers(commandBuffer, 0, 1, uiBorderVertexBuffers, screen_offsets);
//     // // vkCmdDraw(commandBuffer, (u32)24, 1, 0, 0);
//     // vkCmdDraw(commandBuffer, (u32)24, 1, 0, 0);
    

//     screen_push_constants.textPosition = {240, 280}; 
//     screen_push_constants.textScale = {200, 200};    
//     screen_push_constants.textColor = {1.0f, 1.0f, 1.0f, 1.0f};    
//     // screen_push_constants.texCoords = {0.51f, 0.0f, 1.0f, 0.5f};    //shield
//     // screen_push_constants.texCoords = {0.0f, 0.0f, 0.5f, 0.5f};    //sword
//     // screen_push_constants.texCoords = {0.0f, 0.5f, 0.51f, 1.0f};    //helm
//     // screen_push_constants.texCoords = {0.5f, 0.0f, 0.5f, 0.5f};    //old scaling and offsetting method
//     screen_push_constants.texCoords = {0.0f, 0.0f, 1.0f, 1.0f};    //entire texture
//     screen_push_constants.drawTexture = 1;    
//     vkCmdPushConstants(commandBuffer, tri->screen_space_texture_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScreenPushConstants), &screen_push_constants);
//     vkCmdBindVertexBuffers(commandBuffer, 0, 1, uiBorderVertexBuffers, screen_offsets);
//     // vkCmdDraw(commandBuffer, (u32)24, 1, 0, 0);
//     vkCmdDraw(commandBuffer, (u32)6, 1, 24, 0);
    
//     //END ITEM TEXTURE TEST
// #else
//     VkBuffer uiBorderVertexBuffers[] = {tri->entityMeshVertices[MeshTypes::mesh_uiBox]};
//     VkDeviceSize screen_offsets[] = {0};
// #endif
    
//     //MULTI TEXTURE TEST
// #if DRAW_DYNAMIC_TEXTURE
//     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->multi_texture_pipeline);
//     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri->multi_texture_pipeline_layout, 0, 1, &tri->multi_texture_descriptor_sets[tri->currentFrame], 0, nullptr);
//     vkCmdBindVertexBuffers(commandBuffer, 0, 1, uiBorderVertexBuffers, screen_offsets);


//     MultiTexturePushConstants texture_push_constants = {};
//     // texture_push_constants.textPosition  = {200, 380}; 
//     // texture_push_constants.textScale     = {400, 400};    
//     // texture_push_constants.textColor     = {1.0f, 0.0f, 0.0f, 1.0f};    
//     // texture_push_constants.texCoords     = {0.0f, 0.0f, 1.0f, 1.0f};    
//     // texture_push_constants.drawTexture   = 1;    
//     // texture_push_constants.texture_index = 0;//max of 4 textures bound    
    
//     // vkCmdPushConstants(commandBuffer, tri->multi_texture_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MultiTexturePushConstants), &texture_push_constants);
//     // vkCmdDraw(commandBuffer, (u32)tri->entityMeshIndexCount[MeshTypes::mesh_uiBox], 1, 0, 0);


//     texture_push_constants.textPosition = {700, 380}; 
//     texture_push_constants.textScale = {400, 400};    
//     texture_push_constants.textColor = {1.0f, 0.0f, 0.0f, 1.0f};    
//     texture_push_constants.texCoords = {0.0f, 0.0f, 1.0f, 1.0f};    
//     texture_push_constants.drawTexture = 1;    
//     texture_push_constants.texture_index = 0;//max of 4 textures bound    
    
//     vkCmdPushConstants(commandBuffer, tri->multi_texture_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MultiTexturePushConstants), &texture_push_constants);
//     vkCmdDraw(commandBuffer, (u32)tri->entityMeshIndexCount[MeshTypes::mesh_uiBox], 1, 0, 0);
//     //END MULTI TEXTURE TEST
// #endif




    
    
    vkCmdEndRenderPass(commandBuffer);


    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tri->timestampQueryPool[tri->currentFrame], 1);


    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        PRINT("FAILED to record command buffer!\n");
    }
    // else{
        // PRINT("successfully recorded command buffer!\n");
    // }

    tri->frameCount++;
    if(tri->frameCount % 1000 == 0)tri->slowFrameCount++;

}

void createSyncObjects(vkTri* tri){
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//fence starts signaled so we dont infinitely wait on first pass

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        if(vkCreateSemaphore(tri->logicalDevice, &semaphoreInfo, nullptr, &tri->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(tri->logicalDevice, &semaphoreInfo, nullptr, &tri->renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(tri->logicalDevice, &semaphoreInfo, nullptr, &tri->transferCompleteSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(tri->logicalDevice, &fenceInfo, nullptr, &tri->inFlightFences[i]) != VK_SUCCESS){
            PRINT("FAILED to create sync objects for frame %d!\n", i);
        }else{
            PRINT("successfully created sync objects for frame %d!\n", i);
        }
 
        if(vkCreateSemaphore(tri->logicalDevice, &semaphoreInfo, nullptr, &tri->computeFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(tri->logicalDevice, &fenceInfo, nullptr, &tri->computeInFlightFences[i]) != VK_SUCCESS){
            PRINT("FAILED to create compute sync objects for frame %d!\n", i);
        }else{
            PRINT("successfully created compute sync objects for frame %d!\n", i);
        }
 
    }

}

void cleanupSwapChain(vkTri* tri){



    vkDestroyImageView( tri->logicalDevice, tri->colorImageView, nullptr);
    vkDestroyImage(     tri->logicalDevice, tri->colorImage, nullptr);
    vkFreeMemory(       tri->logicalDevice, tri->colorImageMemory, nullptr);

    vkDestroyImageView( tri->logicalDevice, tri->depthImageView, nullptr);
    vkDestroyImage(     tri->logicalDevice, tri->depthImage, nullptr);
    vkFreeMemory(       tri->logicalDevice, tri->depthImageMemory, nullptr);

    for(int i = 0; i < tri->swapChainImageCount; i++){
        vkDestroyFramebuffer(tri->logicalDevice, tri->swapChainFramebuffers[i], nullptr);
    }

    for(int i = 0; i < tri->swapChainImageCount; i++){
        vkDestroyImageView(tri->logicalDevice, tri->swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(tri->logicalDevice, tri->swapchain, nullptr);

}



void recreateSwapChain(vkTri* tri){
    vkDeviceWaitIdle(tri->logicalDevice);
    cleanupSwapChain(tri);

    createSwapChain(tri);

    createImageViews(tri);
    // create_offscreen_texture(tri);
    createColorResources(tri);
    createDepthResources(tri);
    createFramebuffers(tri);

    //will want to handle recreating the render pass as well
}

//drop the entire glm folder into the workspace to use this
// #include "glm/glm.hpp"              
// #include "glm/gtc/matrix_transform.hpp"
// #include "glm/gtc/type_ptr.hpp"
// #include "glm/gtc/matrix_access.hpp"
// #include "glm/ext/matrix_clip_space.hpp"

// struct glmUniformBufferObject{
//     glm::mat4 model;
//     glm::mat4 view;
//     glm::mat4 proj;
// };

void updateComputeUniformBuffer(vkTri* tri){
    ComputeUniformBufferObject ubo = {};
    ubo.deltaTime = tri->deltaTime * 2000;
    memcpy(tri->computeUniformBuffersMapped[tri->currentFrame], &ubo, sizeof(ubo));

}


void create_ubo_sampler_descriptor_sets(vkTri* tri, VkDescriptorSet* sets, VkBuffer* buffers, size_t bufferSize, VkImageView textureImageView){
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){layouts[frame] = tri->ubo_sampler_descriptor_set_layout;}
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    
    if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, sets) != VK_SUCCESS){
        PRINT("failed to allocate descriptor sets!\n");
    }else{
        PRINT("successfully allocated descriptor sets!\n");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = tri->textureSampler;

        VkWriteDescriptorSet descriptorWrites[2] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;
        descriptorWrites[1].pTexelBufferView = nullptr;//optional


        vkUpdateDescriptorSets(tri->logicalDevice, 2, descriptorWrites, 0, nullptr);
    }
}




void create_ubo_multi_texture_descriptor_sets(vkTri* tri, VkDescriptorSetLayout* layouts, VkDescriptorSet* sets, VkBuffer* buffers, size_t bufferSize, VkDescriptorImageInfo* textures, u32 textureCount){

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    
    if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, sets) != VK_SUCCESS){
        PRINT("failed to allocate descriptor sets!\n");
    }else{
        PRINT("successfully allocated descriptor sets!\n");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = tri->textureSampler;

        VkWriteDescriptorSet descriptorWrites[3] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &samplerInfo;
        descriptorWrites[1].pTexelBufferView = nullptr;//optional


        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = sets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[2].descriptorCount = textureCount;
        descriptorWrites[2].pImageInfo = textures;
        descriptorWrites[2].pTexelBufferView = nullptr;//optional

        vkUpdateDescriptorSets(tri->logicalDevice, 3, descriptorWrites, 0, nullptr);
    }

}




void create_ubo_sampler_ssbo_descriptor_pool(vkTri* tri){
    VkDescriptorPoolSize poolSizes[5] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT * 10;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * MAX_UI_TEXTURES * 10;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizes[2].descriptorCount = MAX_FRAMES_IN_FLIGHT * MAX_UI_TEXTURES * 10;

    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = MAX_FRAMES_IN_FLIGHT * 10;

    poolSizes[4].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[4].descriptorCount = MAX_FRAMES_IN_FLIGHT * 10; // fix: missing sampler alloc

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 5;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT * 30;

    if(vkCreateDescriptorPool(tri->logicalDevice, &poolInfo, nullptr, &tri->ubo_sampler_ssbo_descriptor_pool) != VK_SUCCESS){
        PRINT("failed to create descriptor pool!\n");
    }else{
        PRINT("successfully created descriptor pool!\n");
    }
}


void createComputeDescriptorSets(vkTri* tri){
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){layouts[frame] = tri->computeDescriptorSetLayout;}

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
    allocInfo.descriptorSetCount = (u32)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, tri->computeDescriptorSets) != VK_SUCCESS){
        PRINT("failed to allocate compute descriptor sets!");
    }else{
        PRINT("successfully allocated compute descriptor sets!");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo uniformBufferInfo = {};
        uniformBufferInfo.buffer = tri->computeUniformBuffers[i];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(ComputeUniformBufferObject);

        VkWriteDescriptorSet descriptorWrites[3] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = tri->computeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

        VkDescriptorBufferInfo storageBufferInfoLastFrame = {};
        storageBufferInfoLastFrame.buffer = tri->shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = sizeof(particle) * MAX_PARTICLES;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = tri->computeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame = {};
        storageBufferInfoCurrentFrame.buffer = tri->shaderStorageBuffers[i];
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = sizeof(particle) * MAX_PARTICLES;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = tri->computeDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

        vkUpdateDescriptorSets(tri->logicalDevice, 3, descriptorWrites, 0, nullptr);
    }
}

void recordComputeCommandBuffer(vkTri* tri, VkCommandBuffer commandBuffer){
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
        PRINT("failed to begin recording compute command buffer!\n");
    }else{
        // PRINT("successfully began recording compute command buffer!\n");
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, tri->computePipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, tri->computePipelineLayout, 0, 1, &tri->computeDescriptorSets[tri->currentFrame], 0, nullptr);

    vkCmdDispatch(commandBuffer, MAX_PARTICLES / 256, 1, 1);

    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        PRINT("failed to record compute command buffer!\n");
    }else{
        // PRINT("successfully recorded compute command buffer!\n");
    }
}


void reload_skybox_pipeline(vkTri* tri){

   

        vkDeviceWaitIdle(tri->logicalDevice);
        PRINT("RELOADING SKYBOX!\n");
    
        vkDestroyPipeline(tri->logicalDevice,       tri->skybox_pipeline, nullptr);
        vkDestroyPipelineLayout(tri->logicalDevice, tri->skybox_pipeline_layout, nullptr);
        
        VkPushConstantRange pushConstantRange = {};
    
        VkVertexInputBindingDescription skyboxBindingDescription = getBindingDescription(0, sizeof(SkyBoxVertex));
        VkVertexInputAttributeDescription skyboxAttributeDescriptions[4] = {};
        VkFormat    skyBoxformats[4] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32_UINT};
        size_t      skyBoxoffsets[4] = {HANDMADE_OFFSETOF(SkyBoxVertex, pos), HANDMADE_OFFSETOF(SkyBoxVertex, color), HANDMADE_OFFSETOF(SkyBoxVertex, texCoord), HANDMADE_OFFSETOF(SkyBoxVertex, faceID),};
        setAttributeDescriptions(skyboxAttributeDescriptions, 4, 0, skyBoxformats, skyBoxoffsets); 
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(float) * 26;
        create2StagePipeline(tri, "C:/labor/shaders/skybox_vert.spv", "C:/labor/shaders/skybox_frag.spv", &skyboxBindingDescription,
            skyboxAttributeDescriptions, 4, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
        tri->renderPass, &tri->ubo_sampler_descriptor_set_layout, &tri->skybox_pipeline_layout, &tri->skybox_pipeline, true, 1, &pushConstantRange, VK_FRONT_FACE_CLOCKWISE, VK_FALSE);
    
      
        tri->skyboxVertWriteTime = Win32GetLastWriteTime("C:/labor/shaders/skybox_vert.spv");
        tri->skyboxFragWriteTime = Win32GetLastWriteTime("C:/labor/shaders/skybox_frag.spv");

        tri->skybox_shader_loaded = true;
}


void reload_screen_test_pipeline(vkTri* tri){

    vkDeviceWaitIdle(tri->logicalDevice);
    PRINT("RELOADING SCREEN TEST!\n");

    vkDestroyPipeline(tri->logicalDevice,       tri->screen_space_test_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->screen_space_test_pipeline_layout, nullptr);
    
    //create UI border/element drawing pipeline
    VkVertexInputAttributeDescription attributeDescriptions[2] = {};

    VkVertexInputBindingDescription bindingDescription = getBindingDescription(0, sizeof(TextVertex));
    VkFormat  formats[2] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
    size_t    offsets[2] = {HANDMADE_OFFSETOF(TextVertex, pos), HANDMADE_OFFSETOF(TextVertex, texCoord)};
    setAttributeDescriptions(attributeDescriptions, 2, 0, formats, offsets); 
    bool transparent = true;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 20;

    create2StagePipeline(tri, "C:/labor/shaders/screen_space_text_vert.spv", "C:/labor/shaders/screen_space_test_frag.spv", &bindingDescription,
                            attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->screen_space_descriptor_set_layout, &tri->screen_space_test_pipeline_layout, &tri->screen_space_test_pipeline, transparent, 1, &pushConstantRange, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE);
        
    
        tri->screenTestVertWriteTime = Win32GetLastWriteTime("C:/labor/shaders/screen_space_test_vert.spv");
        tri->screenTestFragWriteTime = Win32GetLastWriteTime("C:/labor/shaders/screen_space_test_frag.spv");

        tri->screen_test_shader_loaded = true;
}




bool IsValidSPIRV(void *data, size_t size) {
    if (size < 4) return false;
    uint32_t *u32 = (uint32_t *)data;
    return u32[0] == 0x07230203;
}

#if PRINT_CYCLES
#define PRINT_TIMING(fmt, ...)            PRINT(fmt, ##__VA_ARGS__)
#else
#define PRINT_TIMING(fmt, ...) 
#endif

void drawFrame(vkTri* tri){
    BEGIN_BLOCK("VK WAIT FOR FENCES");

    u64 drawFrameStart = __rdtsc();

    
    // WIN32_FILE_ATTRIBUTE_DATA ignored;
    // if (!GetFileAttributesEx(tri->reloadLockPath, GetFileExInfoStandard, &ignored)) {

        // char vert_shader_storage[MAX_SHADER_SIZE];
        // char frag_shader_storage[MAX_SHADER_SIZE];
        // size_t max_vert_shader_size = MAX_SHADER_SIZE;
        // size_t max_frag_shader_size = MAX_SHADER_SIZE;
        // debug_file vert_file = Win32ReadFileToGivenBuffer("C:/labor/shaders/skybox_vert.spv", vert_shader_storage, &max_vert_shader_size);
        // debug_file frag_file = Win32ReadFileToGivenBuffer("C:/labor/shaders/skybox_frag.spv", frag_shader_storage, &max_frag_shader_size);

        // if(IsValidSPIRV(vert_shader_storage, vert_file.filesize.QuadPart) && IsValidSPIRV(frag_shader_storage, frag_file.filesize.QuadPart)){
    u64 fileTimeStart = __rdtsc();
    BEGIN_BLOCK("SHADER RELOAD CHECK");

            FILETIME newSkyboxVertWriteTime = Win32GetLastWriteTime("C:/labor/shaders/skybox_vert.spv");
            FILETIME newSkyboxFragWriteTime = Win32GetLastWriteTime("C:/labor/shaders/skybox_frag.spv");
            if((CompareFileTime(&newSkyboxVertWriteTime, &tri->skyboxVertWriteTime) != 0) || (CompareFileTime(&newSkyboxFragWriteTime, &tri->skyboxFragWriteTime) != 0)){
                reload_skybox_pipeline(tri);
            }
        // }

      
      
            FILETIME newScreenTestVertWriteTime = Win32GetLastWriteTime("C:/labor/shaders/screen_space_test_vert.spv");
            FILETIME newScreenTestFragWriteTime = Win32GetLastWriteTime("C:/labor/shaders/screen_space_test_frag.spv");
            if((CompareFileTime(&newScreenTestVertWriteTime, &tri->screenTestVertWriteTime) != 0) || (CompareFileTime(&newScreenTestFragWriteTime, &tri->screenTestFragWriteTime) != 0)){
                reload_screen_test_pipeline(tri);
            }
    // }
    END_BLOCK("SHADER RELOAD CHECK");

    u64 fileTimeEnd = __rdtsc() - fileTimeStart;
    PRINT_TIMING("filetime:               %zu\n", fileTimeEnd);

    //compute submission (we dont care if the window is minimized for compute
    VkSubmitInfo submitInfo = {};

    #if USE_COMPUTE
    vkWaitForFences(tri->logicalDevice, 1, &tri->computeInFlightFences[tri->currentFrame], VK_TRUE, UINT64_MAX);
    updateComputeUniformBuffer(tri);
    vkResetFences(tri->logicalDevice, 1, &tri->computeInFlightFences[tri->currentFrame]);
    vkResetCommandBuffer(tri->compute_commandBuffers[tri->currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordComputeCommandBuffer(tri, tri->compute_commandBuffers[tri->currentFrame]);
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tri->compute_commandBuffers[tri->currentFrame];
   
    if (!g_window_minimized) {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &tri->computeFinishedSemaphores[tri->currentFrame];
    } else {
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = NULL;
    }
    

    if(vkQueueSubmit(tri->computeQueue, 1, &submitInfo, tri->computeInFlightFences[tri->currentFrame]) != VK_SUCCESS){
        PRINT("failed to submit compute command buffer!\n");
    }else{
        // PRINT("successfully submitted compute command buffer!\n");
    }
    #endif
    BEGIN_BLOCK("VK PRECISE WAIT FOR FENCES");
    //wait for the previous frame to finish
    #if 1
    u64 waitForFenceStart = __rdtsc();    
    vkWaitForFences(tri->logicalDevice, 1, &tri->inFlightFences[tri->currentFrame], VK_TRUE, UINT64_MAX);
    u64 waitForFenceEnd = __rdtsc() - waitForFenceStart;    
    u64 waitForFences = __rdtsc() - waitForFenceStart;
    PRINT_TIMING("fence wait            :             %zu\n", waitForFenceEnd);
    PRINT_TIMING("fence wait since start:             %zu\n", __rdtsc() - waitForFenceStart);
    // if(waitForFences > 10000000)__debugbreak();
    #endif
    END_BLOCK("VK PRECISE WAIT FOR FENCES");

    END_BLOCK("VK WAIT FOR FENCES");

    //keep running compute shader even if the window is minimized
    if(g_window_minimized){
  
        vkResetFences(tri->logicalDevice, 1, &tri->inFlightFences[tri->currentFrame]);
        
        if(tri->frameCount > 0){
            u32 prevFrame = (tri->currentFrame - 1) % (MAX_FRAMES_IN_FLIGHT);
            //read timestamps from previous frame
            u64 timestampResult[2] = {};
            vkGetQueryPoolResults( tri->logicalDevice, tri->timestampQueryPool[prevFrame], 0, 2, sizeof(u64) * 2, &timestampResult, sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
            PRINT_TIMING("GPU start: %zu, end: %zu\n", timestampResult[0],timestampResult[1]);
            u64 start = timestampResult[0];
            u64 end = timestampResult[1];
            double gpuMs = (end - start) * tri->timestampPeriod / 1e6;
            PRINT_TIMING("GPU time: %.3f ms\n", gpuMs);
                
        }
        VkCommandBuffer commandBuffer = tri->commandBuffers[tri->currentFrame];
        vkResetCommandBuffer(tri->commandBuffers[tri->currentFrame], 0);
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//optional
        beginInfo.pInheritanceInfo = nullptr;//optional
        if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
            PRINT("FAILED to begin recording command buffer!\n");

        }
        vkCmdResetQueryPool(commandBuffer, tri->timestampQueryPool[tri->currentFrame], 0, 2);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, tri->timestampQueryPool[tri->currentFrame], 0);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tri->timestampQueryPool[tri->currentFrame], 1);
        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
            PRINT("FAILED to record command buffer!\n");
        }

        
        //dont submit any semaphores since we dont need to wait on a frame
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = NULL;
        submitInfo.pWaitDstStageMask = NULL;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &tri->commandBuffers[tri->currentFrame];

        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = NULL;


        if(vkQueueSubmit(tri->graphicsQueue, 1, &submitInfo, tri->inFlightFences[tri->currentFrame]) != VK_SUCCESS){
            PRINT("FAILED to submit draw command buffer!\n");
        }

        tri->currentFrame = (tri->currentFrame + 1) % (MAX_FRAMES_IN_FLIGHT);

        
        return;
    }


    BEGIN_BLOCK("VK DRAW");
 
    //acquire an image from the swap chain
    u32 imageIndex;
    VkResult result = vkAcquireNextImageKHR(tri->logicalDevice, tri->swapchain, UINT64_MAX, tri->imageAvailableSemaphores[tri->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR && !g_window_minimized){
        recreateSwapChain(tri);
        END_BLOCK("VK DRAW");
        return;
    }else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        PRINT("FAILED to acquire swap chain image!\n");
    }


    //only reset the fence if we know we are submitting work
    vkResetFences(tri->logicalDevice, 1, &tri->inFlightFences[tri->currentFrame]);
    

    if(tri->frameCount > 0){
        u32 prevFrame = (tri->currentFrame - 1) % (MAX_FRAMES_IN_FLIGHT);
        //read timestamps from previous frame
        u64 timestampResult[2] = {};
        vkGetQueryPoolResults( tri->logicalDevice, tri->timestampQueryPool[prevFrame], 0, 2, sizeof(u64) * 2, &timestampResult, sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        PRINT_TIMING("GPU start: %zu, end: %zu\n", timestampResult[0],timestampResult[1]);
        u64 start = timestampResult[0];
        u64 end = timestampResult[1];
        double gpuMs = (end - start) * tri->timestampPeriod / 1e6;
        PRINT_TIMING("GPU time: %.3f ms\n", gpuMs);
            
    }

    //record a command buffer which draws the scene onto the image
    vkResetCommandBuffer(tri->commandBuffers[tri->currentFrame], 0);

    u64 recordCommandBufferStart = __rdtsc();    
    recordCommandBuffer(tri, tri->commandBuffers[tri->currentFrame], imageIndex);

    u64 recordCommandBufferEnd = __rdtsc() - recordCommandBufferStart;    
    PRINT_TIMING("record command buffer:  %zu\n", recordCommandBufferEnd);





    //submit the recorded command buffer
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    #if USE_COMPUTE
    VkSemaphore waitSemaphores[] = {tri->computeFinishedSemaphores[tri->currentFrame], tri->imageAvailableSemaphores[tri->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 2;
    #else
    VkSemaphore waitSemaphores[2] = {tri->imageAvailableSemaphores[tri->currentFrame]};
    VkPipelineStageFlags waitStages[2] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    if(tri->waitForTransfer){
        waitSemaphores[1] = tri->transferCompleteSemaphores[tri->chunkStageCurrentSlice];
        waitStages[1] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // or appropriate stage for your buffer update

        submitInfo.waitSemaphoreCount++;
    }
    #endif
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tri->commandBuffers[tri->currentFrame];

    VkSemaphore signalSemaphores[] = {tri->renderFinishedSemaphores[tri->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if(vkQueueSubmit(tri->graphicsQueue, 1, &submitInfo, tri->inFlightFences[tri->currentFrame]) != VK_SUCCESS){
        PRINT("FAILED to submit draw command buffer!\n");
    }
    // else{
        // PRINT("successfully submitted draw command buffer!\n");
    // }

    //present the swap chain image
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    //will almost always be a single swapchain
    VkSwapchainKHR swapChains[] = {tri->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;//optional, checks every VK_RESULT of every swapchain if theres more than one

    u64 presentStart = __rdtsc();
    result = vkQueuePresentKHR(tri->presentQueue, &presentInfo);
    u64 presentEnd = __rdtsc() - presentStart;
    PRINT_TIMING("QueuePresent:           %zu\n", presentEnd);

    if(!g_window_minimized && (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || g_window_resized)){
        g_window_resized = false;
        recreateSwapChain(tri);
    }else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        PRINT("FAILED to acquire swap chain image!\n");
    }



    tri->currentFrame = (tri->currentFrame + 1) % (MAX_FRAMES_IN_FLIGHT);
    if(tri->waitForTransfer){
        tri->chunkStageCurrentSlice = (tri->chunkStageCurrentSlice + 1) % (CHUNK_STAGING_SLICES);
        tri->waitForTransfer = false;
    }
    
    u64 drawFrameEnd = __rdtsc() - drawFrameStart;
    PRINT_TIMING("draw frame:             %zu\n", drawFrameEnd);
    
    END_BLOCK("VK DRAW");
}

void init_font(vkTri* tri){

    char font_path[256];
    const char* font_asset_path = "assets/fonts/ComicSansMS.ttf";
    size_t path_offset = handmade_strcat(font_path, tri->path, font_asset_path);
    
    debug_file debugFile = {};
    debugFile = Win32ReadFile(font_path);
    memcpy(tri->fontBuffer, debugFile.memory, debugFile.filesize.QuadPart);

    stbtt_pack_context pack_context;
    if(!stbtt_PackBegin(&pack_context, tri->font_bitmap, 512, 512, 0, 1, NULL)){
        PRINT("FAILED TO INIT FONT PACKING!\n");
        return;
    }

    //set quality options
    stbtt_PackSetOversampling(&pack_context, 2, 2);//better quality for small fonts

    //pack range of characters
    stbtt_packedchar chardata[96];
    float fontSize = 32.0f;
    if(!stbtt_PackFontRange(&pack_context, (unsigned char*)tri->fontBuffer, 0, fontSize, 32, 96, (stbtt_packedchar*)tri->charData)){
        PRINT("Failed to pack font!\n");
        return;
    }

    stbtt_PackEnd(&pack_context);


    int offset = stbtt_GetFontOffsetForIndex((unsigned char*)tri->fontBuffer, 0);//0 = first font in file
        
    stbtt_fontinfo* font = (stbtt_fontinfo*)tri->fontInfo;
    stbtt_InitFont(font, (unsigned char*)tri->fontBuffer, offset);

    // PRINT("Size of stbtt_fontinfo: %zu\n", sizeof(stbtt_fontinfo));
    
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(font, &ascent, &descent, &lineGap);

    float scale = stbtt_ScaleForMappingEmToPixels(font, 32.0f);  // 32 pixel height
    tri->scale = scale;
    tri->ascent = ascent * scale;
    tri->descent = descent * scale;
    tri->lineGap = lineGap * scale;
    tri->doubleDescent = (tri->descent) * 2.0f;

    PRINT("Font loaded: size=%zu, offset=%d, scale=%f\n", 
        debugFile.filesize.QuadPart, offset, scale);
    PRINT("size of stbtt_packdata %zu\n", sizeof(stbtt_packedchar));
    

    Win32FreeFile(&debugFile);

}

VkDescriptorSetLayoutBinding create_descriptor_set_layout_binding(u32 binding, VkDescriptorType type, u32 descriptor_count, VkShaderStageFlags flags){
    VkDescriptorSetLayoutBinding set_layout_binding = {};
    set_layout_binding.binding = binding;
    set_layout_binding.descriptorType = type;
    set_layout_binding.descriptorCount = descriptor_count;
    set_layout_binding.stageFlags = flags;
    set_layout_binding.pImmutableSamplers = nullptr;//optional
    return set_layout_binding;
}



void initVulkan(vkTri* tri){
    for(u32 i = 0; i < MAX_CHUNKS; i++){
        tri->chunkToSSBOMap[i].ssboIndex = NULL_CHUNK;
    }

    //init free face memory for chunk meshing
    for(u32 i = 0; i < MAX_CORES; i++){
        tri->freeFaceMemoryIndices[i] = i;
    }
    tri->freeFaceMemoryCount = MAX_CORES;

    size_t reloadPathoffset = handmade_strcat(tri->reloadLockPath, tri->path, "shaders\\lock.tmp");

    tri->currentFrame = 0;
    
    init_font(tri);

    // loadWindowsFont(tri, &tri->monospacedScreenFont, "C:/Windows/Fonts/cour.ttf", "Courier New");
    loadWindowsFont(tri, &tri->RenderCommandData.monospacedScreenFont, "C:/Windows/Fonts/Terminal.ttf", "Terminal");
    loadWindowsFont(tri, &tri->RenderCommandData.defaultScreenFont, "C:/Windows/Fonts/arial.ttf", "Arial");
    
    #if USE_MODEL
    load_obj_test(tri);
    #endif 

    createVkInstance(tri);
    
    #ifdef LABOR_DEBUG
    setupDebugMessenger(tri);
    #endif

    createSurface(tri);

    pickPhysicalDevice(tri);
    
    createLogicalDevice(tri);    

    createSwapChain(tri);
    createImageViews(tri);

    createRenderPass(tri);

    create_offscreen_depth_resources(tri);

    create_offscreen_render_pass(tri);

    //time stamp query
    VkQueryPoolCreateInfo timestampCreateInfo = {};
    timestampCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO; 
    timestampCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    timestampCreateInfo.queryCount = 2;
    VkResult timestampResult0 = vkCreateQueryPool( tri->logicalDevice, &timestampCreateInfo, NULL, tri->timestampQueryPool + 0);
    VkResult timestampResult1 = vkCreateQueryPool( tri->logicalDevice, &timestampCreateInfo, NULL, tri->timestampQueryPool + 1);
    VkResult timestampResult2 = vkCreateQueryPool( tri->logicalDevice, &timestampCreateInfo, NULL, tri->timestampQueryPool + 2);
    Assert(timestampResult0 == 0 && timestampResult1 == 0 && timestampResult2 == 0);

    VkDescriptorSetLayoutBinding uboLayoutBinding  = create_descriptor_set_layout_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT));
    VkDescriptorSetLayoutBinding sampler2DLayoutBinding  = create_descriptor_set_layout_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding default_bindings[2] = {uboLayoutBinding, sampler2DLayoutBinding};
    u32 binding_count = 2;
    create_descriptor_set_layout(tri, &tri->ubo_sampler_descriptor_set_layout, default_bindings, binding_count);

    // create_descriptor_set_layout(tri, &tri->screen_space_descriptor_set_layout, default_bindings, binding_count);

    VkDescriptorSetLayoutBinding ssbo1_binding  = create_descriptor_set_layout_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding compute_ssbo2_binding  = create_descriptor_set_layout_binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding compute_bindings[3] = {uboLayoutBinding, ssbo1_binding, compute_ssbo2_binding};
    create_descriptor_set_layout(tri, &tri->computeDescriptorSetLayout, compute_bindings, 3);


    //ubo and ssbo
    VkDescriptorSetLayoutBinding entity_bindings[2] = {uboLayoutBinding, ssbo1_binding};
    create_descriptor_set_layout(tri, &tri->ubo_ssbo_descriptor_set_layout, entity_bindings, 2);
    

    //ubo ssbo and sampler
    VkDescriptorSetLayoutBinding sampler2DLayoutBinding2  = create_descriptor_set_layout_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding ubo_ssbo_sampler_bindings[3] = {uboLayoutBinding, ssbo1_binding, sampler2DLayoutBinding2};
    create_descriptor_set_layout(tri, &tri->ubo_ssbo_sampler_descriptor_set_layout, ubo_ssbo_sampler_bindings, 3);

    //ubo
    create_descriptor_set_layout(tri, &tri->ubo_descriptor_set_layout, &uboLayoutBinding, 1);

    //ubo and sampler
    create_descriptor_set_layout(tri, &tri->screen_space_texture_descriptor_set_layout, default_bindings, 2);
    
    //multi texture
    VkDescriptorSetLayoutBinding samplerLayoutBinding  = create_descriptor_set_layout_binding(1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding multiImageLayoutBinding  = create_descriptor_set_layout_binding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding multi_texture_bindings[3] = {uboLayoutBinding, samplerLayoutBinding, multiImageLayoutBinding};
    create_descriptor_set_layout(tri, &tri->multi_texture_descriptor_set_layout, multi_texture_bindings, 3);
    
    VkDescriptorSetLayoutBinding multiFontLayoutBinding  = create_descriptor_set_layout_binding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding multiScreenSpaceTextureLayoutBinding  = create_descriptor_set_layout_binding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, SCREEN_SPACE_TEXTURE_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding multi_font_bindings[3] = {uboLayoutBinding, samplerLayoutBinding, multiFontLayoutBinding};
    VkDescriptorSetLayoutBinding multi_screen_space_texture_bindings[3] = {uboLayoutBinding, samplerLayoutBinding, multiScreenSpaceTextureLayoutBinding};
    create_descriptor_set_layout(tri, &tri->screen_space_font_descriptor_set_layout, multi_font_bindings, 3);
    create_descriptor_set_layout(tri, &tri->screen_space_descriptor_set_layout, multi_screen_space_texture_bindings, 3);
    

    // recompile_shaders(tri);
    FILETIME lock_time = Win32GetLastWriteTime("C:/labor/shaders/lock.tmp");
    PRINT("last write time for lock.tmp: low %u, high: %u\n", lock_time.dwLowDateTime, lock_time.dwHighDateTime);

    VkVertexInputBindingDescription bindingDescription = getBindingDescription(0, sizeof(Vertex));
    VkVertexInputAttributeDescription attributeDescriptions[3] = {};
    VkFormat    formats[3] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
    size_t      offsets[3] = {HANDMADE_OFFSETOF(Vertex, pos), HANDMADE_OFFSETOF(Vertex, color), HANDMADE_OFFSETOF(Vertex, texCoord),};
    setAttributeDescriptions(attributeDescriptions, 3, 0, formats, offsets); 
#if 0 //textured model drawing
    create2StagePipeline(tri, "C:/labor/shaders/vert.spv", "C:/labor/shaders/frag.spv", &bindingDescription,
                            attributeDescriptions, 3, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->ubo_sampler_descriptor_set_layout, &tri->pipelineLayout, &tri->graphicsPipeline);


#endif

VkPushConstantRange pushConstantRange = {};
                pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                pushConstantRange.offset = 0;
                pushConstantRange.size = sizeof(float) * 20;
//skeletal mesh pipeline
  create2StagePipeline(tri, "C:/labor/shaders/skeletal_mesh_vert.spv", "C:/labor/shaders/skeletal_mesh_frag.spv", &bindingDescription,
                            attributeDescriptions, 3, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->ubo_sampler_descriptor_set_layout, &tri->skeletal_mesh_pipeline_layout, &tri->skeletal_mesh_pipeline,
                            true, 1, &pushConstantRange, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE);

//offscreen skeletal mesh pipeline
    create2StagePipeline(tri, "C:/labor/shaders/skeletal_mesh_vert.spv", "C:/labor/shaders/skeletal_mesh_frag.spv", &bindingDescription,
        attributeDescriptions, 3, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_SAMPLE_COUNT_1_BIT,
        tri->offscreen_render_pass, &tri->ubo_sampler_descriptor_set_layout, &tri->offscreen_pipeline_layout, &tri->offscreen_pipeline,
        true, 1, &pushConstantRange, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE);
        //create skybox
        // WIN32_FILE_ATTRIBUTE_DATA ignored;
        // if (!GetFileAttributesEx(tri->reloadLockPath, GetFileExInfoStandard, &ignored)) {

        //     char vert_shader_storage[MAX_SHADER_SIZE];
        //     char frag_shader_storage[MAX_SHADER_SIZE];
        //     size_t max_vert_shader_size = MAX_SHADER_SIZE;
        //     size_t max_frag_shader_size = MAX_SHADER_SIZE;
        //     debug_file vert_file = Win32ReadFileToGivenBuffer("C:/labor/shaders/skybox_vert.spv", vert_shader_storage, &max_vert_shader_size);
        //     debug_file frag_file = Win32ReadFileToGivenBuffer("C:/labor/shaders/skybox_frag.spv", frag_shader_storage, &max_frag_shader_size);

        //     if(IsValidSPIRV(vert_shader_storage, vert_file.filesize.QuadPart) && IsValidSPIRV(frag_shader_storage, frag_file.filesize.QuadPart)){
                VkVertexInputBindingDescription skyboxBindingDescription = getBindingDescription(0, sizeof(SkyBoxVertex));
                VkVertexInputAttributeDescription skyboxAttributeDescriptions[4] = {};
                VkFormat    skyBoxformats[4] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32_UINT};
                size_t      skyBoxoffsets[4] = {HANDMADE_OFFSETOF(SkyBoxVertex, pos), HANDMADE_OFFSETOF(SkyBoxVertex, color), HANDMADE_OFFSETOF(SkyBoxVertex, texCoord), HANDMADE_OFFSETOF(SkyBoxVertex, faceID),};
                setAttributeDescriptions(skyboxAttributeDescriptions, 4, 0, skyBoxformats, skyBoxoffsets); 
                pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                pushConstantRange.offset = 0;
                pushConstantRange.size = sizeof(float) * 26;
                create2StagePipeline(tri, "C:/labor/shaders/skybox_vert.spv", "C:/labor/shaders/skybox_frag.spv", &skyboxBindingDescription,
                    skyboxAttributeDescriptions, 4, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                tri->renderPass, &tri->ubo_sampler_descriptor_set_layout, &tri->skybox_pipeline_layout, &tri->skybox_pipeline, true, 1, &pushConstantRange, VK_FRONT_FACE_CLOCKWISE, VK_FALSE);
            
              
                tri->skyboxVertWriteTime = Win32GetLastWriteTime("C:/labor/shaders/skybox_vert.spv");
                tri->skyboxFragWriteTime = Win32GetLastWriteTime("C:/labor/shaders/skybox_frag.spv");
    
                tri->skybox_shader_loaded = true;
        //     }

         
        // }
       

      
        
        
        
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(float) * 24;
        bool transparent = true;
    
        create2StagePipeline(tri, "C:/labor/shaders/vertex_pulled_voxel_mesh_vert.spv", "C:/labor/shaders/vertex_pulled_voxel_mesh_frag.spv", &bindingDescription,
            attributeDescriptions, 3, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
            tri->renderPass, &tri->ubo_ssbo_sampler_descriptor_set_layout, &tri->voxel_vertex_puller_pipeline_layout, &tri->voxel_vertex_puller_pipeline,  transparent, 1, &pushConstantRange);


#if 0
        bindingDescription = getBindingDescription(0, sizeof(test_vertex));
        formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
        formats[1] = VK_FORMAT_R32G32B32_SFLOAT;
        offsets[0] = HANDMADE_OFFSETOF(test_vertex, pos);
        offsets[1] = HANDMADE_OFFSETOF(test_vertex, color);
        setAttributeDescriptions(attributeDescriptions, 2, 0, formats, offsets); 
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.size = sizeof(float) * 8;
        create2StagePipeline(tri, "C:/labor/shaders/test_ray_tracer_vert.spv", "C:/labor/shaders/test_ray_tracer_frag.spv", &bindingDescription,
            attributeDescriptions, 3, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
            tri->renderPass, &tri->ubo_descriptor_set_layout, &tri->test_ray_tracer_pipeline_layout, &tri->test_ray_tracer_pipeline,  transparent, 1, &pushConstantRange);
#endif


    bindingDescription = getBindingDescription(0, sizeof(LineVertex));
    formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
    formats[1] = VK_FORMAT_R8G8B8A8_UNORM;
    offsets[0] = HANDMADE_OFFSETOF(LineVertex, pos);
    offsets[1] = HANDMADE_OFFSETOF(LineVertex, color);
    setAttributeDescriptions(attributeDescriptions, 2, 0, formats, offsets); 
    pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(uint32_t);
    transparent = true;

    create2StagePipeline(tri, "C:/labor/shaders/colorVertex_vert.spv", "C:/labor/shaders/colorVertex_frag.spv", &bindingDescription,
                            attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->ubo_ssbo_descriptor_set_layout, &tri->line_pipelineLayout, &tri->graphics_line_pipeline, transparent, 1, &pushConstantRange);

                
    create2StagePipeline(tri, "C:/labor/shaders/colorVertex_vert.spv", "C:/labor/shaders/colorVertex_frag.spv", &bindingDescription,
        attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
        tri->renderPass, &tri->ubo_ssbo_descriptor_set_layout, &tri->default_entity_pipelineLayout, &tri->default_entity_pipeline, transparent, 1, &pushConstantRange);

                    
    //create UI border/element drawing pipeline
    bindingDescription = getBindingDescription(0, sizeof(TextVertex));
    formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
    formats[1] = VK_FORMAT_R32G32_SFLOAT;
    offsets[0] = HANDMADE_OFFSETOF(TextVertex, pos);
    offsets[1] = HANDMADE_OFFSETOF(TextVertex, texCoord);
    setAttributeDescriptions(attributeDescriptions, 2, 0, formats, offsets); 
    transparent = true;

    pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 20;
    create2StagePipeline(tri, "C:/labor/shaders/screen_space_text_vert.spv", "C:/labor/shaders/screen_space_text_frag.spv", &bindingDescription,
                            attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->screen_space_font_descriptor_set_layout, &tri->screen_space_text_pipeline_layout, &tri->screen_space_text_pipeline, transparent, 1, &pushConstantRange);


    create2StagePipeline(tri, "C:/labor/shaders/screen_space_text_vert.spv", "C:/labor/shaders/screen_space_texture_frag.spv", &bindingDescription,
                            attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->screen_space_descriptor_set_layout, &tri->screen_space_texture_pipeline_layout, &tri->screen_space_texture_pipeline, transparent, 1, &pushConstantRange, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE);

    pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 20;
    create2StagePipeline(tri, "C:/labor/shaders/screen_space_test_vert.spv", "C:/labor/shaders/screen_space_test_frag.spv", &bindingDescription,
                            attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
                            tri->renderPass, &tri->screen_space_descriptor_set_layout, &tri->screen_space_test_pipeline_layout, &tri->screen_space_test_pipeline, transparent, 1, &pushConstantRange, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE);

                tri->screenTestVertWriteTime = Win32GetLastWriteTime("C:/labor/shaders/screen_space_test_vert.spv");
                tri->screenTestFragWriteTime = Win32GetLastWriteTime("C:/labor/shaders/screen_space_test_frag.spv");
                tri->screen_test_shader_loaded = true;
#if 1
    //multi texture test
    pushConstantRange.size = sizeof(float) * 14;
    create2StagePipeline(tri, "C:/labor/shaders/multi_texture_vert.spv", "C:/labor/shaders/multi_texture_frag.spv", &bindingDescription,
        attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
        tri->renderPass, &tri->multi_texture_descriptor_set_layout, &tri->multi_texture_pipeline_layout, &tri->multi_texture_pipeline, transparent, 1, &pushConstantRange);
#endif
    
        
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 21;
    //world space text
    create2StagePipeline(tri, "C:/labor/shaders/world_space_text_vert.spv", "C:/labor/shaders/world_space_text_frag.spv", &bindingDescription,
        attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, tri->msaaSamples,
        tri->renderPass, &tri->ubo_sampler_descriptor_set_layout, &tri->world_space_text_pipeline_layout, &tri->world_space_text_pipeline, transparent, 1, &pushConstantRange);


#if 0
    bindingDescription = getBindingDescription(0, sizeof(particle));
    formats[0] = VK_FORMAT_R32G32_SFLOAT;
    formats[1] = VK_FORMAT_R32G32B32A32_SFLOAT;
    offsets[0] = HANDMADE_OFFSETOF(particle, pos);
    offsets[1] = HANDMADE_OFFSETOF(particle, color);
    setAttributeDescriptions(attributeDescriptions, 2, 0, formats, offsets); 
    transparent = false;
    create2StagePipeline(tri, "C:/labor/shaders/particle_vert.spv", "C:/labor/shaders/particle_frag.spv", &bindingDescription,
        attributeDescriptions, 2, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, tri->msaaSamples,
        tri->renderPass, &tri->ubo_sampler_descriptor_set_layout, &tri->particle_pipelineLayout, &tri->particle_pipeline, transparent);
        
        
        createComputePipeline(tri);
#endif


    createCommandPool(tri);
    createShaderStorageBuffers(tri);

    create_offscreen_texture(tri);
    createColorResources(tri);
    createDepthResources(tri);

    //needs to happen after depth resources have been created
    createFramebuffers(tri);
    create_offscreen_frame_buffer(tri);
    //TEXTURE LOADING

    char texture_path[256];
    size_t offset = 0;
    // const char* texture = "assets/textures/texture.jpg";
    #if USE_MODEL
        offset = handmade_strcpy(texture_path, "C:/Libraries/viking_room.png");
    #else
        offset = handmade_strcat(texture_path, tri->path, "assets/textures/texture.jpg");
    #endif
    unsigned char* pixels = load_texture_pixels(texture_path, tri->model_texture_info);
    create_texture_image(tri, pixels, VK_FORMAT_R8G8B8A8_SRGB, tri->model_texture_info);
    tri->model_texture_info.textureImageView = createImageView(tri, tri->model_texture_info.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tri->model_texture_info.mipLevels);
    free_texture_pixels(pixels);
    

    createTextureSampler(tri);


    offset = handmade_strcat(texture_path, tri->path, "assets/textures/item_texture_atlas.png");
    pixels = load_texture_pixels(texture_path, tri->screen_space_texture_info);
    create_texture_image(tri, pixels, VK_FORMAT_R8G8B8A8_SRGB, tri->screen_space_texture_info);
    tri->screen_space_texture_info.textureImageView = createImageView(tri, tri->screen_space_texture_info.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tri->screen_space_texture_info.mipLevels);
    free_texture_pixels(pixels);
    
    //FONT LOADING
    tri->screen_space_font_info.texWidth = 512;
    tri->screen_space_font_info.texHeight = 512;
    tri->screen_space_font_info.texChannels = 1;
    tri->screen_space_font_info.mipLevels = 1;//font will be blurry if its not set to 1
    create_texture_image(tri, tri->font_bitmap, VK_FORMAT_R8_UNORM, tri->screen_space_font_info);
    tri->screen_space_font_info.textureImageView = createImageView(tri, tri->screen_space_font_info.textureImage, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, tri->screen_space_font_info.mipLevels);


    tri->windows_font_info.texWidth = 1024;
    tri->windows_font_info.texHeight = 1024;
    tri->windows_font_info.texChannels = 4;
    tri->windows_font_info.mipLevels = 1;
    create_texture_image(tri, (unsigned char* )  tri->RenderCommandData.defaultScreenFont.Bitmap, VK_FORMAT_R8G8B8A8_SRGB, tri->windows_font_info);
    tri->windows_font_info.textureImageView = createImageView(tri, tri->windows_font_info.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tri->windows_font_info.mipLevels);


    tri->windows_monospace_font_info.texWidth = 1024;
    tri->windows_monospace_font_info.texHeight = 1024;
    tri->windows_monospace_font_info.texChannels = 4;
    tri->windows_monospace_font_info.mipLevels = 1;
    create_texture_image(tri, (unsigned char* )  tri->RenderCommandData.monospacedScreenFont.Bitmap, VK_FORMAT_R8G8B8A8_SRGB, tri->windows_monospace_font_info);
    tri->windows_monospace_font_info.textureImageView = createImageView(tri, tri->windows_monospace_font_info.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tri->windows_monospace_font_info.mipLevels);

    memset(texture_path, 0, sizeof(char) * 256);
    offset = handmade_strcat(texture_path, tri->path, "assets/textures/thorns.png");
    int thornX = 128;
    int thornY = 128;
    int thornChannels = 4;
    stbi_uc* thornPixels = stbi_load(texture_path, &thornX, &thornY, &thornChannels, STBI_rgb_alpha);

    if (thornPixels) {
        // Convert from 4 bytes (RGBA) to u32
        for (int i = 0; i < 128 * 128; i++) {
            u32 r = thornPixels[i * 4 + 0];
            u32 g = thornPixels[i * 4 + 1];
            u32 b = thornPixels[i * 4 + 2];
            u32 a = thornPixels[i * 4 + 3];
            
            // Pack into u32 (matching your JavaScript format: RGBA)
            tri->thorns[i] = r | (g << 8) | (b << 16) | (a << 24);
        }
        
        // Free the stbi memory
        stbi_image_free(thornPixels);
    } else {
        // Handle error - image failed to load
        PRINT("Failed to load texture: %s\n", texture_path);
    }

    memset(texture_path, 0, sizeof(char) * 256);
    // offset = handmade_strcat(texture_path, tri->path, "assets/textures/cracked.png");
    // offset = handmade_strcat(texture_path, tri->path, "assets/textures/bark.jpg");
    // offset = handmade_strcat(texture_path, tri->path, "assets/textures/crest.png");
    // offset = handmade_strcat(texture_path, tri->path, "assets/textures/skulls.png");
    offset = handmade_strcat(texture_path, tri->path, "assets/textures/skulls2.png");
    int crackedX = 64;
    int crackedY = 64;
    int crackedChannels = 4;
    stbi_uc* crackedPixels = stbi_load(texture_path, &crackedX, &crackedY, &crackedChannels, STBI_rgb_alpha);

    if (crackedPixels) {
        // Convert from 4 bytes (RGBA) to u32
        for (int i = 0; i < 64 * 64; i++) {
            u32 r = crackedPixels[i * 4 + 0];
            u32 g = crackedPixels[i * 4 + 1];
            u32 b = crackedPixels[i * 4 + 2];
            u32 a = crackedPixels[i * 4 + 3];
            
            // Pack into u32 (matching your JavaScript format: RGBA)
            tri->cracked[i] = r | (g << 8) | (b << 16) | (a << 24);
        }
        
        // Free the stbi memory
        stbi_image_free(crackedPixels);
    } else {
        // Handle error - image failed to load
        PRINT("Failed to load texture: %s\n", texture_path);
    }

    unsigned char single_pixel[4][4] = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF, 0x00, 0x00, 0xFF,
                                        0x00, 0xFF, 0x00, 0xFF,
                                        0x00, 0x00, 0xFF, 0xFF,}; //in order of rgba 

    //SINGLE PIXEL TEXTURE
    uint8_t* checkerboard0 = (uint8_t*)plat_alloc_mem(512 * 512 * 4);
    uint8_t* checkerboard1 = (uint8_t*)plat_alloc_mem(512 * 512 * 4);
    uint8_t* checkerboard2 = (uint8_t*)plat_alloc_mem(512 * 512 * 4);
    uint8_t* checkerboard3 = (uint8_t*)plat_alloc_mem(512 * 512 * 4);
    uint8_t* checkerboards[4] = {checkerboard0,checkerboard1,checkerboard2,checkerboard3}; 
    u32 colorA = 0xFFFFFFFF;
    #define CREATE_CHECKERBOARDS 0
    for (int i = 0; i < 4; i++){
        uint8_t* checkerboard = checkerboards[i];
        #if CREATE_CHECKERBOARDS
        u32 colorB = 0x00000000;
        if(i == 1) colorA = 0xFFFFFF00; 
        if(i == 2) colorA = 0xFF00FFFF; 
        if(i == 3) colorA = 0xFFFF00FF; 
        for(int j = 0; j < 512; j++){
            for(int k = 0; k < 512; k++){
                uint32_t color = colorA;
                if(((j / 64) + (k / 64)) % (i + 1) == 0)color = colorB;
                int index = (j * 512 + k) * 4;
                checkerboard[index + 0] = (color >> 0)  & 0xFF;
                checkerboard[index + 1] = (color >> 8)  & 0xFF;
                checkerboard[index + 2] = (color >> 16) & 0xFF;
                checkerboard[index + 3] = (color >> 24) & 0xFF;
                // if(((j / 128) % 2) == 0)color = 0x00FFFFFF;
                // PRINT("%u  ", j * k * 4);
            }
        }
        #else
        memset(checkerboard, 0xFF, 512 * 512 * 4);
        #endif
    }
    
    for(int i = 0; i < 4; i++){
        tri->screen_space_single_pixel_info[i].texWidth = 512;
        tri->screen_space_single_pixel_info[i].texHeight = 512;
        tri->screen_space_single_pixel_info[i].texChannels = 4;
        tri->screen_space_single_pixel_info[i].mipLevels = 1;//font will be blurry if its not set to 1
        // create_texture_image(tri, single_pixel[i], VK_FORMAT_R8G8B8A8_SRGB, tri->screen_space_single_pixel_info[i]);
        create_texture_image(tri, checkerboards[i], VK_FORMAT_R8G8B8A8_SRGB, tri->screen_space_single_pixel_info[i]);
        tri->screen_space_single_pixel_info[i].textureImageView = 
        createImageView(tri, tri->screen_space_single_pixel_info[i].textureImage, VK_FORMAT_R8G8B8A8_SRGB, 
                        VK_IMAGE_ASPECT_COLOR_BIT, tri->screen_space_single_pixel_info[i].mipLevels);
    }

    plat_dealloc_mem(checkerboard0, 512 * 512 * 4);
    plat_dealloc_mem(checkerboard1, 512 * 512 * 4);
    plat_dealloc_mem(checkerboard2, 512 * 512 * 4);
    plat_dealloc_mem(checkerboard3, 512 * 512 * 4);

    //END TEXTURE LOADING

    VkBufferUsageFlagBits vertex_flag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkBufferUsageFlagBits index_flag = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    //for transient text
    mapPersistentBuffer(tri, sizeof(TextVertex) * MAX_TEXT_CHARS, vertex_flag, tri->screenTextVertexBuffers, tri->screenTextVertexBuffersMemory, (void**)tri->screenTextVertexBuffersMapped);
    
    //for world space text
    mapPersistentBuffer(tri, sizeof(TextVertex) * MAX_TEXT_CHARS, vertex_flag, tri->worldTextVertexBuffers, tri->worldTextVertexBuffersMemory, (void**)tri->worldTextVertexBuffersMapped);
    
    //per entity SSBOs
    mapPersistentBuffer(tri, sizeof(PerEntitySSBO) * MAX_SSBO_ENTITIES, vertex_flag | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                        tri->perEntityShaderStorageBuffers, tri->perEntityShaderStorageBuffersMemory, (void**)tri->perEntityShaderStorageBuffersMapped);


    //viking room model
    #if USE_MODEL
    createMeshBuffer(tri, tri->model_vertices, sizeof(Vertex) * tri->model_vertex_count, vertex_flag,  tri->vertexBuffer, tri->vertexBufferMemory);
    createMeshBuffer(tri, tri->model_indices, sizeof(u32) * tri->model_index_count, index_flag,  tri->indexBuffer, tri->indexBufferMemory);
    #endif



    //create camera ubo
    mapPersistentBuffer(tri, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, tri->uniformBuffers, tri->uniformBuffersMemory, (void**)tri->uniformBuffersMapped);
    //create compute ubo (just has deltaTime)
    mapPersistentBuffer(tri, sizeof(ComputeUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, tri->computeUniformBuffers, tri->computeUniformBuffersMemory, (void**)tri->computeUniformBuffersMapped);

    //init vertex pulling index buffer

    //131072 * 6 = 786432
    u32 face_max = 786432;
    mapSinglePersistentBuffer(tri, tri->required_chunk_ssbo_memory, vertex_flag | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                        &tri->voxel_ssbo, &tri->voxel_ssbo_memory, &tri->voxel_ssbo_mapped);


    uint32_t count = 0;
    for(int i = 0; i < face_max; i++) {
    //the vertex puller will determine which indices to use based on if the quad is flipped for AO or not
        tri->faceIndices[count + 0] = ((i * 6) + 0u);
        tri->faceIndices[count + 1] = ((i * 6) + 1u);
        tri->faceIndices[count + 2] = ((i * 6) + 2u);
        tri->faceIndices[count + 3] = ((i * 6) + 3u);
        tri->faceIndices[count + 4] = ((i * 6) + 4u);
        tri->faceIndices[count + 5] = ((i * 6) + 5u);
        count += 6;
    }
    createMeshBuffer(tri, tri->faceIndices, sizeof(tri->faceIndices), index_flag,  tri->face_index_buffer, tri->face_index_buffer_memory);


    //populate entity meshes
    //no mesh defaults to a cube
    
    //cube mesh
    MeshTypes meshType = MeshTypes::mesh_cube;
    createMeshBuffer(tri, testCubeVertices, sizeof(testCubeVertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_cube], tri->entityMeshVertexMemory[MeshTypes::mesh_cube]);
    createMeshBuffer(tri, testCubeIndices, sizeof(testCubeIndices), index_flag,  tri->entityMeshIndices[MeshTypes::mesh_cube], tri->entityMeshIndexMemory[MeshTypes::mesh_cube]);
    createMeshBuffer(tri, testCubeLineIndices, sizeof(testCubeLineIndices), index_flag,  tri->entityMeshLineIndices[MeshTypes::mesh_cube], tri->entityMeshLineIndexMemory[MeshTypes::mesh_cube]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_cube]  = 36;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_cube]  = 24;


    meshType = MeshTypes::mesh_DirectionWidgets;
    createMeshBuffer(tri, directionVertices, sizeof(directionVertices), vertex_flag,  tri->entityMeshVertices[meshType], tri->entityMeshVertexMemory[meshType]);
    createMeshBuffer(tri, directionLineIndices, sizeof(directionLineIndices), index_flag,  tri->entityMeshIndices[meshType], tri->entityMeshIndexMemory[meshType]);
    createMeshBuffer(tri, directionLineIndices, sizeof(directionLineIndices), index_flag,  tri->entityMeshLineIndices[meshType], tri->entityMeshLineIndexMemory[meshType]);
    tri->entityMeshIndexCount       [meshType]  = 6;
    tri->entityMeshLineIndexCount   [meshType]  = 6;

    //skybox
    meshType = MeshTypes::mesh_skybox;
    createMeshBuffer(tri, texturedCubeVertices, sizeof(texturedCubeVertices), vertex_flag,  tri->entityMeshVertices[meshType], tri->entityMeshVertexMemory[meshType]);
    createMeshBuffer(tri, texturedCubeIndices, sizeof(texturedCubeIndices), index_flag,  tri->entityMeshIndices[meshType], tri->entityMeshIndexMemory[meshType]);
    tri->entityMeshIndexCount       [meshType]  = 36;

    //axis mesh
    createMeshBuffer(tri, (void*)axisVertices, sizeof(axisVertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_Axes], tri->entityMeshVertexMemory[MeshTypes::mesh_Axes]);
    createMeshBuffer(tri, (void*)axisLineIndices, sizeof(axisLineIndices), index_flag,  tri->entityMeshLineIndices[MeshTypes::mesh_Axes], tri->entityMeshLineIndexMemory[MeshTypes::mesh_Axes]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_Axes]  = 14;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_Axes]  = 14;

    //screen quad mesh
    createMeshBuffer(tri, (void*)test_vertices, sizeof(test_vertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_screenQuad], tri->entityMeshVertexMemory[MeshTypes::mesh_screenQuad]);
    createMeshBuffer(tri, (void*)test_indices, sizeof(test_indices), index_flag,  tri->entityMeshIndices[MeshTypes::mesh_screenQuad], tri->entityMeshIndexMemory[MeshTypes::mesh_screenQuad]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_quad]  = 6;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_quad]  = 6;

    //ui border
    createMeshBuffer(tri, (void*)border_vertices, sizeof(border_vertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_uiBorder], tri->entityMeshVertexMemory[MeshTypes::mesh_uiBorder]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_uiBorder]  = 24;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_uiBorder]  = 24;

    //ui box
    createMeshBuffer(tri, (void*)box_vertices, sizeof(box_vertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_uiBox], tri->entityMeshVertexMemory[MeshTypes::mesh_uiBox]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_uiBox]  = 6;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_uiBox]  = 6;


    //default quad
    createMeshBuffer(tri, (void*)tri_vertices, sizeof(tri_vertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_quad], tri->entityMeshVertexMemory[MeshTypes::mesh_quad]);
    createMeshBuffer(tri, (void*)indices, sizeof(indices), index_flag,  tri->entityMeshIndices[MeshTypes::mesh_quad], tri->entityMeshIndexMemory[MeshTypes::mesh_quad]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_quad]  = 6;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_quad]  = 6;
        
    //ray
    createMeshBuffer(tri, (void*)lineVertices, sizeof(lineVertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_ray], tri->entityMeshVertexMemory[MeshTypes::mesh_ray]);
    createMeshBuffer(tri, (void*)indices, sizeof(indices), index_flag,  tri->entityMeshLineIndices[MeshTypes::mesh_ray], tri->entityMeshLineIndexMemory[MeshTypes::mesh_ray]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_ray]  = 2;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_ray]  = 2;
        

    generateSphere(tri->sphereMesh, 1.0f, 6, 0xFF0000FF);
    meshType = MeshTypes::mesh_Sphere;
    createMeshBuffer(tri, tri->sphereMesh.vertices,    sizeof(LineVertex) * tri->sphereMesh.numVertices, vertex_flag,  tri->entityMeshVertices[meshType], tri->entityMeshVertexMemory[meshType]);
    createMeshBuffer(tri, tri->sphereMesh.indices,     sizeof(uint32_t)   * tri->sphereMesh.numIndices, index_flag,  tri->entityMeshIndices[meshType], tri->entityMeshIndexMemory[meshType]);
    createMeshBuffer(tri, tri->sphereMesh.lineIndices, sizeof(uint32_t)   * tri->sphereMesh.numLineIndices, index_flag,  tri->entityMeshLineIndices[meshType], tri->entityMeshLineIndexMemory[meshType]);
    tri->entityMeshIndexCount       [meshType]  = tri->sphereMesh.numIndices;
    tri->entityMeshLineIndexCount   [meshType]  = tri->sphereMesh.numLineIndices;

    generateCapsule(tri->capsuleMesh, 0.5f, 1.0f, 6, 0xFF0000FF);
    meshType = MeshTypes::mesh_Capsule;
    createMeshBuffer(tri, tri->capsuleMesh.vertices,    sizeof(LineVertex)  * tri->capsuleMesh.numVertices, vertex_flag,  tri->entityMeshVertices[meshType], tri->entityMeshVertexMemory[meshType]);
    createMeshBuffer(tri, tri->capsuleMesh.indices,     sizeof(uint32_t)    * tri->capsuleMesh.numIndices, index_flag,  tri->entityMeshIndices[meshType], tri->entityMeshIndexMemory[meshType]);
    createMeshBuffer(tri, tri->capsuleMesh.lineIndices, sizeof(uint32_t)    * tri->capsuleMesh.numLineIndices, index_flag,  tri->entityMeshLineIndices[meshType], tri->entityMeshLineIndexMemory[meshType]);
    tri->entityMeshIndexCount       [meshType]  = tri->capsuleMesh.numIndices;
    tri->entityMeshLineIndexCount   [meshType]  = tri->capsuleMesh.numLineIndices;


    generateHemisphere(tri->hemisphereMesh, 1.0f, 12, 12);
    meshType = MeshTypes::mesh_Hemisphere;
    createMeshBuffer(tri, tri->hemisphereMesh.vertices,    sizeof(LineVertex)  * tri->hemisphereMesh.numVertices, vertex_flag,  tri->entityMeshVertices[meshType], tri->entityMeshVertexMemory[meshType]);
    createMeshBuffer(tri, tri->hemisphereMesh.indices,     sizeof(uint32_t)    * tri->hemisphereMesh.numIndices, index_flag,  tri->entityMeshIndices[meshType], tri->entityMeshIndexMemory[meshType]);
    createMeshBuffer(tri, tri->hemisphereMesh.lineIndices, sizeof(uint32_t)    * tri->hemisphereMesh.numLineIndices, index_flag,  tri->entityMeshLineIndices[meshType], tri->entityMeshLineIndexMemory[meshType]);
    tri->entityMeshIndexCount       [meshType]  = tri->hemisphereMesh.numIndices;
    tri->entityMeshLineIndexCount   [meshType]  = tri->hemisphereMesh.numLineIndices;


    uint32_t vertex_color_state = 54321;
    LineVertex colorCubeVertices[8] =       
    {      
        {{-0.5f, -0.5f,  0.5f}, rng_next_u32(&vertex_color_state)}, // Vertex 0
        {{ 0.5f, -0.5f,  0.5f}, rng_next_u32(&vertex_color_state)}, // 1
        {{ 0.5f,  0.5f,  0.5f}, rng_next_u32(&vertex_color_state)}, // 2
        {{-0.5f,  0.5f,  0.5f}, rng_next_u32(&vertex_color_state)}, // 3
        {{-0.5f, -0.5f, -0.5f}, rng_next_u32(&vertex_color_state)}, // 4
        {{ 0.5f, -0.5f, -0.5f}, rng_next_u32(&vertex_color_state)}, // 5
        {{ 0.5f,  0.5f, -0.5f}, rng_next_u32(&vertex_color_state)}, // 6
        {{-0.5f,  0.5f, -0.5f}, rng_next_u32(&vertex_color_state)}  // 7
    };
    
    //color cube mesh
    createMeshBuffer(tri, colorCubeVertices, sizeof(colorCubeVertices), vertex_flag,  tri->entityMeshVertices[MeshTypes::mesh_ColorCube], tri->entityMeshVertexMemory[MeshTypes::mesh_ColorCube]);
    createMeshBuffer(tri, testCubeIndices, sizeof(testCubeIndices), index_flag,  tri->entityMeshIndices[MeshTypes::mesh_ColorCube], tri->entityMeshIndexMemory[MeshTypes::mesh_ColorCube]);
    createMeshBuffer(tri, testCubeLineIndices, sizeof(testCubeLineIndices), index_flag,  tri->entityMeshLineIndices[MeshTypes::mesh_ColorCube], tri->entityMeshLineIndexMemory[MeshTypes::mesh_ColorCube]);
    tri->entityMeshIndexCount       [MeshTypes::mesh_ColorCube]  = 36;
    tri->entityMeshLineIndexCount   [MeshTypes::mesh_ColorCube]  = 24;



    //populate the voxel ssbo with test data

    FaceData test_faces[6] = {
        get_face(0, 0, 1,  1, 1,  0,  255,  1, 1,  0),//red Z+
        get_face(1, 0, 0,  1, 1,  1,  255,  1, 1,  1),//yellow Z-
        get_face(1, 1, 0,  1, 1,  1,  255,  1, 1,  2),//green Y+
        get_face(0, 0, 0,  1, 1,  1,  255,  1, 1,  3),//teal Y-
        get_face(1, 0, 0,  1, 1,  1,  255,  1, 1,  4),//blue X+
        get_face(0, 1, 0,  1, 1,  1,  255,  1, 1,  5),//pink X-
    };
    memcpy(tri->voxel_ssbo_mapped, test_faces, sizeof(FaceData) * 6);


    //screen space ubo (viewRect(window width and height))
    mapPersistentBuffer(tri, sizeof(ScreenSpaceUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, tri->screen_space_uniform_buffers, tri->screen_space_uniform_buffers_memory, (void**)tri->screen_space_uniform_buffers_mapped);
    create_dynamic_texture_image(tri, 512, 512, tri->dynamic_image_memory[0], tri->dynamic_image[0], tri->dynamic_image_view[0]);
    create_dynamic_texture_image(tri, 512, 512, tri->dynamic_image_memory[1], tri->dynamic_image[1], tri->dynamic_image_view[1]);
    create_dynamic_texture_image(tri, 512, 512, tri->dynamic_image_memory[2], tri->dynamic_image[2], tri->dynamic_image_view[2]);
    create_dynamic_texture_image(tri, 512, 512, tri->dynamic_image_memory[3], tri->dynamic_image[3], tri->dynamic_image_view[3]);

    create_dynamic_texture_staging_buffer(tri, 512, 512, tri->dynamic_texture_staging_buffer[0], tri->dynamic_texture_staging_buffer_memory[0], tri->dynamic_texture_staging_buffer_mapped[0]);
    create_dynamic_texture_staging_buffer(tri, 512, 512, tri->dynamic_texture_staging_buffer[1], tri->dynamic_texture_staging_buffer_memory[1], tri->dynamic_texture_staging_buffer_mapped[1]);
    create_dynamic_texture_staging_buffer(tri, 512, 512, tri->dynamic_texture_staging_buffer[2], tri->dynamic_texture_staging_buffer_memory[2], tri->dynamic_texture_staging_buffer_mapped[2]);
    create_dynamic_texture_staging_buffer(tri, 512, 512, tri->dynamic_texture_staging_buffer[3], tri->dynamic_texture_staging_buffer_memory[3], tri->dynamic_texture_staging_buffer_mapped[3]);

    create_ubo_sampler_ssbo_descriptor_pool(tri);
    createDefaultEntityDescriptorSets(tri);



    //CHUNK STAGING BUFFER CREATION
    createTransferCommandPool(tri);
    //chunk mesh staging buffer
    createBuffer(tri, tri->chunkStagingSizeTotal, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            tri->chunkStagingBuffer, tri->chunkStagingMemory);
    vkMapMemory(tri->logicalDevice, tri->chunkStagingMemory, 0, tri->chunkStagingSizeTotal, 0, &tri->chunkStagingMapped);
    //create transfer command pool

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        // VkCommandBufferAllocateInfo allocInfo = {};
        // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // allocInfo.commandPool = tri->transferCommandPool;
        // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        // allocInfo.commandBufferCount = 1;
        // vkAllocateCommandBuffers(tri->logicalDevice, &allocInfo, &tri->transferCommandBuffers[i]);

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if(vkCreateFence(tri->logicalDevice, &fenceInfo, NULL, &tri->transferFences[i]) != VK_SUCCESS){
            PRINT("FAILED to create transfer sync objects for frame %d!\n", i);
        }else{
            PRINT("successfully created transfer sync objects for frame %d!\n", i);
        }
    }
    for(int i = 0; i < CHUNK_STAGING_SLICES; i++){

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = tri->transferCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(tri->logicalDevice, &allocInfo, &tri->transferCommandBuffers[i]);


        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if(vkCreateFence(tri->logicalDevice, &fenceInfo, NULL, &tri->chunkStageFences[i]) != VK_SUCCESS){
            PRINT("FAILED to create chunk stage sync objects for slice %d!\n", i);
        }else{
            PRINT("successfully created chunk stage sync objects for slice %d!\n", i);
        }
    }
    //create device local ssbo
    createBuffer(tri, tri->required_chunk_ssbo_memory * MAX_FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        tri->chunkDeviceSSBO, tri->chunkDeviceSSBOMemory);
    //END CHUNK STAGING BUFFER CREATION
    


    //skybox descriptor sets
    // create_ubo_sampler_descriptor_sets(tri, tri->skybox_descriptor_sets, tri->uniformBuffers, sizeof(UniformBufferObject), tri->screen_space_texture_info.textureImageView);
    create_ubo_sampler_descriptor_sets(tri, tri->skybox_descriptor_sets, tri->uniformBuffers, sizeof(UniformBufferObject), tri->screen_space_texture_info.textureImageView);
    //skeletal mesh set
    create_ubo_sampler_descriptor_sets(tri, tri->skeletal_mesh_descriptor_sets, tri->uniformBuffers, sizeof(UniformBufferObject), tri->screen_space_texture_info.textureImageView);
    //world space model descriptor sets
    create_ubo_sampler_descriptor_sets(tri, tri->graphics_descriptorSets, tri->uniformBuffers, sizeof(UniformBufferObject), tri->model_texture_info.textureImageView);
    //world space text descriptor sets
    create_ubo_sampler_descriptor_sets(tri, tri->world_space_text_descriptor_sets, tri->uniformBuffers, sizeof(UniformBufferObject), tri->screen_space_font_info.textureImageView);
    //screen space text descriptor sets
    // create_ubo_sampler_descriptor_sets(tri, tri->screen_space_text_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), tri->screen_space_font_info.textureImageView);
    //screen space texture descriptor sets
    create_ubo_sampler_descriptor_sets(tri, tri->screen_space_texture_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), tri->screen_space_texture_info.textureImageView);
    // create_ubo_sampler_descriptor_sets(tri, tri->screen_space_test_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), tri->screen_space_texture_info.textureImageView);
    

    //when we used to use a single font
    // create_ubo_sampler_descriptor_sets(tri, tri->screen_space_text_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), tri->windows_font_info.textureImageView);

    VkDescriptorImageInfo multiFontInfos[2] = {};
    multiFontInfos[0].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiFontInfos[0].imageView      = tri->windows_font_info.textureImageView;
    multiFontInfos[0].sampler        = tri->textureSampler;
    multiFontInfos[1].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiFontInfos[1].imageView      = tri->windows_monospace_font_info.textureImageView;
    multiFontInfos[1].sampler        = tri->textureSampler;

    VkDescriptorSetLayout font_layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){font_layouts[frame] = tri->screen_space_font_descriptor_set_layout;}
    
    create_ubo_multi_texture_descriptor_sets(tri, font_layouts, tri->screen_space_text_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), multiFontInfos, 2);


    VkDescriptorImageInfo multiScreenSpaceTextureInfos[SCREEN_SPACE_TEXTURE_COUNT] = {};
    multiScreenSpaceTextureInfos[0].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[0].imageView      = tri->screen_space_texture_info.textureImageView;
    multiScreenSpaceTextureInfos[0].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[1].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[1].imageView      = tri->dynamic_image_view[0];
    multiScreenSpaceTextureInfos[1].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[2].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[2].imageView      = tri->dynamic_image_view[1];
    multiScreenSpaceTextureInfos[2].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[3].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[3].imageView      = tri->screen_space_single_pixel_info[2].textureImageView;
    multiScreenSpaceTextureInfos[3].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[4].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[4].imageView      = tri->windows_font_info.textureImageView;
    multiScreenSpaceTextureInfos[4].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[5].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[5].imageView      = tri->windows_monospace_font_info.textureImageView;
    multiScreenSpaceTextureInfos[5].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[6].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[6].imageView      = tri->offscreen_image_view;
    multiScreenSpaceTextureInfos[6].sampler        = tri->textureSampler;
    multiScreenSpaceTextureInfos[7].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiScreenSpaceTextureInfos[7].imageView      = tri->offscreen_image_view;
    multiScreenSpaceTextureInfos[7].sampler        = tri->textureSampler;
    VkDescriptorSetLayout texture_layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){texture_layouts[frame] = tri->screen_space_descriptor_set_layout;}
    
    create_ubo_multi_texture_descriptor_sets(tri, texture_layouts, tri->screen_space_test_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), multiScreenSpaceTextureInfos, SCREEN_SPACE_TEXTURE_COUNT);





    createComputeDescriptorSets(tri);



        ///MULTI TEXTURE DESCRIPTOR SET TEST

    VkDescriptorImageInfo multiImageInfos[4] = {};
    multiImageInfos[0].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // multiImageInfos[0].imageView      = tri->screen_space_single_pixel_info[0].textureImageView;
    // multiImageInfos[0].imageView      = tri->offscreen_image_view;
    // multiImageInfos[0].imageView      = tri->dynamic_image_view;
    multiImageInfos[0].imageView      = tri->screen_space_font_info.textureImageView;
    multiImageInfos[0].sampler        = tri->textureSampler;
    multiImageInfos[1].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // multiImageInfos[1].imageView      = tri->screen_space_single_pixel_info[1].textureImageView;
    multiImageInfos[1].imageView      = tri->windows_font_info.textureImageView;
    multiImageInfos[1].sampler        = tri->textureSampler;
    multiImageInfos[2].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiImageInfos[2].imageView      = tri->screen_space_single_pixel_info[2].textureImageView;
    multiImageInfos[2].sampler        = tri->textureSampler;
    multiImageInfos[3].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    multiImageInfos[3].imageView      = tri->screen_space_single_pixel_info[3].textureImageView;
    multiImageInfos[3].sampler        = tri->textureSampler;
        
    VkDescriptorSetLayout multi_texture_layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){multi_texture_layouts[frame] = tri->multi_texture_descriptor_set_layout;}
    
    create_ubo_multi_texture_descriptor_sets(tri, multi_texture_layouts, tri->multi_texture_descriptor_sets, tri->screen_space_uniform_buffers, sizeof(ScreenSpaceUniformBufferObject), multiImageInfos, 4);


    {
        VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){layouts[frame] = tri->ubo_descriptor_set_layout;}

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts;
        
        if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, tri->test_ray_tracer_descriptor_sets) != VK_SUCCESS){
            PRINT("failed to allocate descriptor sets!\n");
        }else{
            PRINT("successfully allocated descriptor sets!\n");
        }
    
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = tri->uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

    
            VkWriteDescriptorSet descriptorWrites[1] = {};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = tri->test_ray_tracer_descriptor_sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
    
            vkUpdateDescriptorSets(tri->logicalDevice, 1, descriptorWrites, 0, nullptr);
    
    
        }
    
    
    
    }
   
//voxel vertex puller descriptor set setup
{
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){layouts[frame] = tri->ubo_ssbo_sampler_descriptor_set_layout;}
    
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    
    if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, tri->voxel_vertex_puller_descriptor_sets) != VK_SUCCESS){
        PRINT("failed to allocate descriptor sets!\n");
    }else{
        PRINT("successfully allocated descriptor sets!\n");
    }
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = tri->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);


        
        VkDescriptorBufferInfo ssboInfo = {};
        #if CHUNK_STAGING_BUFFER
            ssboInfo.buffer = tri->chunkDeviceSSBO;
            ssboInfo.offset = tri->required_chunk_ssbo_memory * i;
            ssboInfo.range = tri->required_chunk_ssbo_memory;
        #else
            ssboInfo.buffer = tri->voxel_ssbo;
            ssboInfo.offset = 0;
            ssboInfo.range = tri->required_chunk_ssbo_memory;
        #endif



        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = tri->dynamic_image_view[0];
        imageInfo.sampler = tri->textureSampler;
        

        VkWriteDescriptorSet descriptorWrites[3] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = tri->voxel_vertex_puller_descriptor_sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = tri->voxel_vertex_puller_descriptor_sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &ssboInfo;
        descriptorWrites[1].pTexelBufferView = nullptr;//optional


        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = tri->voxel_vertex_puller_descriptor_sets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &imageInfo;
        descriptorWrites[2].pTexelBufferView = nullptr;//optional

        vkUpdateDescriptorSets(tri->logicalDevice, 3, descriptorWrites, 0, nullptr);
        //END MULTI TEXTURE DESCRIPTOR SET TEST

    }
}


    //world space text descriptor set creation
    {

        VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
        for(int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++){layouts[frame] = tri->ubo_sampler_descriptor_set_layout;}
        
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = tri->ubo_sampler_ssbo_descriptor_pool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts;
        
        if(vkAllocateDescriptorSets(tri->logicalDevice, &allocInfo, tri->world_space_text_descriptor_sets) != VK_SUCCESS){
            PRINT("failed to allocate descriptor sets!\n");
        }else{
            PRINT("successfully allocated descriptor sets!\n");
        }
            
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = tri->uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = tri->screen_space_font_info.textureImageView;
            imageInfo.sampler = tri->textureSampler;
            
    
            VkWriteDescriptorSet descriptorWrites[2] = {};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = tri->world_space_text_descriptor_sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
    
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = tri->world_space_text_descriptor_sets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr;//optional
    
    
            vkUpdateDescriptorSets(tri->logicalDevice, 2, descriptorWrites, 0, nullptr);
        }
    //END MULTI TEXTURE DESCRIPTOR SET TEST

    }



    
    
    createCommandBuffers(tri);
    createComputeCommandBuffers(tri);
    createSyncObjects(tri);


    int fuckTheDebugger = 0;
}

void free_texture_info(vkTri* tri, TextureInfo* texInfo){
    vkDestroyImageView(tri->logicalDevice,  texInfo->textureImageView, nullptr);
    vkDestroyImage(tri->logicalDevice,      texInfo->textureImage, nullptr);
    vkFreeMemory(tri->logicalDevice,        texInfo->textureImageMemory, nullptr);
}


void cleanup_vulkan(vkTri* tri){

    cleanupSwapChain(tri);
    //for if we ever get offscreen buffers setup
    vkDestroyImageView( tri->logicalDevice, tri->offscreen_image_view, nullptr);
    vkDestroyImage(     tri->logicalDevice, tri->offscreen_image, nullptr);
    vkFreeMemory(       tri->logicalDevice, tri->offscreen_image_memory, nullptr);
    vkDestroyImageView( tri->logicalDevice, tri->offscreen_depth_image_view, nullptr);
    vkDestroyImage(     tri->logicalDevice, tri->offscreen_depth_image, nullptr);
    vkFreeMemory(       tri->logicalDevice, tri->offscreen_depth_image_memory, nullptr);

    for(int i = 0; i < MAX_DYNAMIC_TEXTURES; i++){
        vkDestroyImageView( tri->logicalDevice, tri->dynamic_image_view[i], nullptr);
        vkDestroyImage(     tri->logicalDevice, tri->dynamic_image[i], nullptr);
        vkFreeMemory(       tri->logicalDevice, tri->dynamic_image_memory[i], nullptr);
    }

    //what if we size it to be the size of the screen? we will probly need to recreate it there
    //yes if we want it to match the screen size then we will need to recreate it, but its fine for the moment
    vkDestroyFramebuffer(tri->logicalDevice, tri->offscreen_frame_buffer, nullptr);

    vkDestroySampler(tri->logicalDevice, tri->textureSampler, nullptr);


    free_texture_info(tri, &tri->model_texture_info);
    free_texture_info(tri, &tri->screen_space_texture_info);
    free_texture_info(tri, &tri->screen_space_font_info);
    free_texture_info(tri, &tri->windows_font_info);
    free_texture_info(tri, &tri->windows_monospace_font_info);
    for(int i = 0; i < 4; i++){
        free_texture_info(tri, &tri->screen_space_single_pixel_info[i]);
    }


    vkUnmapMemory(tri->logicalDevice,   tri->voxel_ssbo_memory);
    vkDestroyBuffer(tri->logicalDevice, tri->voxel_ssbo, nullptr);
    vkFreeMemory(tri->logicalDevice,    tri->voxel_ssbo_memory, nullptr);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroyBuffer(tri->logicalDevice, tri->uniformBuffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice, tri->uniformBuffersMemory[i], nullptr);

        vkDestroyBuffer(tri->logicalDevice, tri->computeUniformBuffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->computeUniformBuffersMemory[i], nullptr);

        //destroy per entity SSBOs
        vkDestroyBuffer(tri->logicalDevice, tri->perEntityShaderStorageBuffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->perEntityShaderStorageBuffersMemory[i], nullptr);


        vkDestroyBuffer(tri->logicalDevice, tri->screen_space_uniform_buffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->screen_space_uniform_buffers_memory[i], nullptr);

        vkUnmapMemory(tri->logicalDevice, tri->screenTextVertexBuffersMemory[i]);
        vkDestroyBuffer(tri->logicalDevice, tri->screenTextVertexBuffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice, tri->screenTextVertexBuffersMemory[i], nullptr);


        vkUnmapMemory(tri->logicalDevice,   tri->worldTextVertexBuffersMemory[i]);
        vkDestroyBuffer(tri->logicalDevice, tri->worldTextVertexBuffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->worldTextVertexBuffersMemory[i], nullptr);

    }


    vkUnmapMemory(tri->logicalDevice,   tri->chunkStagingMemory);
    vkDestroyBuffer(tri->logicalDevice, tri->chunkStagingBuffer, nullptr);
    vkFreeMemory(tri->logicalDevice,    tri->chunkStagingMemory, nullptr);

    //dont need to unmap since its never mapped in the first place
    // vkUnmapMemory(tri->logicalDevice,   tri->chunkDeviceSSBOMemory);
    vkDestroyBuffer(tri->logicalDevice, tri->chunkDeviceSSBO, nullptr);
    vkFreeMemory(tri->logicalDevice,    tri->chunkDeviceSSBOMemory, nullptr);

    for(int i = 0; i < MAX_DYNAMIC_TEXTURES; i++){
        for(int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++){
            vkUnmapMemory(tri->logicalDevice, tri->dynamic_texture_staging_buffer_memory[i][j]);
            vkDestroyBuffer(tri->logicalDevice, tri->dynamic_texture_staging_buffer[i][j], nullptr);
            vkFreeMemory(tri->logicalDevice, tri->dynamic_texture_staging_buffer_memory[i][j], nullptr);
        }
    }
    vkDestroyDescriptorPool(tri->logicalDevice, tri->ubo_sampler_ssbo_descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->ubo_sampler_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->screen_space_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->screen_space_texture_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->computeDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->ubo_ssbo_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->ubo_descriptor_set_layout, nullptr);
    
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->ubo_ssbo_sampler_descriptor_set_layout, nullptr);
    
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->multi_texture_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->screen_space_font_descriptor_set_layout, nullptr);
    
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->voxel_vertex_puller_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(tri->logicalDevice, tri->test_ray_tracer_descriptor_set_layout, nullptr);


    
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroyBuffer(tri->logicalDevice, tri->shaderStorageBuffers[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->shaderStorageBuffersMemory[i], nullptr);
    }

    vkDestroyBuffer(tri->logicalDevice, tri->indexBuffer, nullptr);
    vkFreeMemory(tri->logicalDevice, tri->indexBufferMemory, nullptr);

    vkDestroyBuffer(tri->logicalDevice, tri->vertexBuffer, nullptr);
    vkFreeMemory(tri->logicalDevice, tri->vertexBufferMemory, nullptr);

    

    vkDestroyBuffer(tri->logicalDevice, tri->face_index_buffer        , nullptr);
    vkFreeMemory(   tri->logicalDevice, tri->face_index_buffer_memory  , nullptr);

    for(int i = 0; i < MeshTypes::mesh_count; i++){
        vkDestroyBuffer(tri->logicalDevice, tri->entityMeshVertices[i]           , nullptr);
        vkDestroyBuffer(tri->logicalDevice, tri->entityMeshIndices[i]           , nullptr);
        vkDestroyBuffer(tri->logicalDevice, tri->entityMeshLineIndices[i]           , nullptr);

        vkFreeMemory(tri->logicalDevice,    tri->entityMeshVertexMemory[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->entityMeshIndexMemory[i], nullptr);
        vkFreeMemory(tri->logicalDevice,    tri->entityMeshLineIndexMemory[i], nullptr);

    }
    
    vkDestroyBuffer(tri->logicalDevice, tri->skeletalMeshVertices           , nullptr);
    vkDestroyBuffer(tri->logicalDevice, tri->skeletalMeshIndices          , nullptr);
    vkFreeMemory(tri->logicalDevice,    tri->skeletalMeshVertexMemory, nullptr);
    vkFreeMemory(tri->logicalDevice,    tri->skeletalMeshIndexMemory, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->computePipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->computePipelineLayout, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->world_space_text_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->world_space_text_pipeline_layout, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->skeletal_mesh_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->skeletal_mesh_pipeline_layout, nullptr);


    vkDestroyPipeline(tri->logicalDevice,       tri->skybox_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->skybox_pipeline_layout, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->particle_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->particle_pipelineLayout, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->offscreen_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->offscreen_pipeline_layout, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->multi_texture_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->multi_texture_pipeline_layout, nullptr);

    vkDestroyPipeline(      tri->logicalDevice, tri->screen_space_text_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->screen_space_text_pipeline_layout, nullptr);
    
    vkDestroyPipeline(      tri->logicalDevice, tri->screen_space_texture_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->screen_space_texture_pipeline_layout, nullptr);

    vkDestroyPipeline(      tri->logicalDevice, tri->screen_space_test_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->screen_space_test_pipeline_layout, nullptr);

    vkDestroyPipeline(      tri->logicalDevice, tri->test_ray_tracer_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->test_ray_tracer_pipeline_layout, nullptr);

    vkDestroyPipeline(      tri->logicalDevice, tri->voxel_vertex_puller_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->voxel_vertex_puller_pipeline_layout, nullptr);

    vkDestroyPipeline(tri->logicalDevice,       tri->default_entity_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->default_entity_pipelineLayout, nullptr);
    vkDestroyPipeline(tri->logicalDevice,       tri->graphics_line_pipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->line_pipelineLayout, nullptr);

    vkDestroyPipeline(tri->logicalDevice, tri->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(tri->logicalDevice, tri->pipelineLayout, nullptr);

    vkDestroyRenderPass(tri->logicalDevice, tri->renderPass, nullptr);
    vkDestroyRenderPass(tri->logicalDevice, tri->offscreen_render_pass, nullptr);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){

        vkDestroySemaphore(tri->logicalDevice, tri->imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(tri->logicalDevice, tri->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(tri->logicalDevice, tri->transferCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(tri->logicalDevice, tri->computeFinishedSemaphores[i], nullptr);
        vkDestroyFence(tri->logicalDevice, tri->inFlightFences[i], nullptr);
        vkDestroyFence(tri->logicalDevice, tri->computeInFlightFences[i], nullptr);
        vkDestroyFence(tri->logicalDevice, tri->transferFences[i], nullptr);
    
        vkDestroyQueryPool(tri->logicalDevice, tri->timestampQueryPool[i], NULL);

    }
    for(int i = 0; i < CHUNK_STAGING_SLICES; i++){
        vkDestroyFence(tri->logicalDevice, tri->chunkStageFences[i], nullptr);
    }

    // vkDestroyCommandPool(tri->logicalDevice, tri->transferCommandPool, nullptr);


    vkDestroyCommandPool(tri->logicalDevice, tri->transferCommandPool, nullptr);
    vkDestroyCommandPool(tri->logicalDevice, tri->commandPool, nullptr);


    //destroy logical device
    vkDestroyDevice(tri->logicalDevice, nullptr);

    #ifdef LABOR_DEBUG
    DestroyDebugUtilsMessengerEXT(tri->instance, tri->debugMessenger, nullptr);
    #endif

    vkDestroySurfaceKHR(tri->instance, tri->surface, nullptr);

    vkDestroyInstance(tri->instance, nullptr);


}


