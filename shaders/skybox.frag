#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint fragFaceID;

layout(location = 0) out vec4 outColor;


layout(push_constant) uniform PushConstants {
    mat4 model;    
    vec4 misc;
    vec4 mouse; //x y are pos, z w are delta
    vec2 viewRect;
} push;

#define PI 3.14159265
#define TWO_PI 6.28318530
#define HPI 1.57079632

float plot(vec2 st, float pct) {
  return smoothstep(pct-0.02, pct, st.y) - smoothstep(pct, pct+0.02, st.y);
    // return smoothstep(0.0, 0.1, st.x - st.y);
}
float plotxw(vec2 st, float pct, float width){
    return smoothstep(pct - width, pct, st.y) - smoothstep(pct, pct + width, st.y);
}

float plotyw(vec2 st, float pct, float width){
    return smoothstep(pct - width, pct, st.x) - smoothstep(pct, pct + width, st.x);
}


vec3 fillPlot(vec2 st, float fx, float width, float time){
    float test  = plotxw(st, fx, 0.02);
    float test1 = plotxw(st, fx + 0.1, 0.02);
    float band  = plotxw(st, fx + 0.05, 0.07);
    // float test2 = plotw(st, 
    vec3 color = mix(vec3(0), vec3(1.0, 0.0, 0.0), band);
    color = mix(color, vec3(1), test);
    color = mix(color, vec3(1), test1);
    return color;
}


//draw a smoothed UI border
/*
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec3 color = vec3(0.0);

    // bottom-left
    // vec2 bl = step(vec2(0.1),st);
	float b = smoothstep(0.01, 0.08, st.y);
	// float b = step(0.1, st.y);

    float l = smoothstep(0.01, 0.2, st.x);
    float pct = b * l;

    // top-right
    vec2 tr = step(vec2(0.1),1.0-st);
    float t = smoothstep(0.01, 0.2, 1.0 - st.y);
    float r = smoothstep(0.2, 0.4, 1.0 - st.x);
    // float r = step(0.2, 1.0 - st.x);

    pct *= t * r;
    float il = step(0.2, st.x);
    float ib = step(0.2, st.y);
    
    float it = step(0.2, 1.0 - st.y);
    float ir = step(0.4, 1.0 - st.x);

	float inner = il * ib * it * ir;
    
    vec3 innerBox = vec3(inner) ;
    
    color = vec3(pct) - innerBox;
	color *= vec3(abs(sin(u_time*3.14)), 1.0, cos(u_time*3.14));
*/

//draw circle
/*
    //a. distance from pixel to center of circle
    pct = distance(st,vec2(0.5));

    // b. The LENGTH of the vector
    //    from the pixel to the center
    vec2 toCenter = vec2(0.5)-st;
    pct = length(toCenter);

    // c. The SQUARE ROOT of the vector
    //    from the pixel to the center
    // vec2 tC = vec2(0.5)-st;
    // pct = sqrt(tC.x*tC.x+tC.y*tC.y);

    vec3 color = vec3(1.0 - smoothstep(0.01, 0.5, pct));
*/

// float doubleCubicSeat(float x, float a, float b){
//     float epsilon = 0.00001;
//     float min_param_a = 0.0 + epsilon;
//     float max_param_a = 1.0 - epsilon;
//     float min_param_b = 0.0;
//     float max_param_b = 1.0;
//     a = min(max_param_a, max(min_param_a, a));
//     b - min(max_param_b, max(min_param_b, b));
    
//     float y = 0.0;
//     if(x <= a){
//         y = b - b * pow(1.0 - (x/a), 3.0);
        
//     }else{
//         y = b + (1.0-b) * pow((x-a) / (1.0-a), 3.0);
//     }
//     return y;

// }



// float wrapneg1to1(float x){
//     //map to 0 to 2
//     x = mod(x + 1.0, 2.0);
//     //map to -1 to 1
//     x = x - 1.0;
//     return x;
// }

// // Blinn-Wyville cosine approximation
// // This approximates (1-cos(π*x))/2 for x in [-1,1]
// float blinnWyvillCos(float x) {
//     // The input must already be in [-1,1] range
//     x = wrapneg1to1(x);
    
//     float x2 = x * x;
//     float x4 = x2 * x2;
//     float x6 = x4 * x2;
    
//     float fa = 4.0/9.0;
//     float fb = 17.0/9.0;
//     float fc = 22.0/9.0;
    
//     return (fa * x6) - (fb * x4) + (fc * x2);
// }

// Map the standard cosine to our approximation
// float cosProx(float angle) {
//     return 1.0 - blinnWyvillCos(angle);
// }

// // Sine is just cosine shifted by -π/2
// float sinProx(float angle) {
//     return 1.0 - cosProx(angle - HPI);
// }

float fastCos(float x) {
    // x in radians, small range (e.g., -π/2 to π/2)
    float x2 = x * x;
    return 1.0 - x2 * (0.5 - x2 * (1.0/24.0 - x2 * (1.0/720.0)));
}
float fastSin(float x){
    return fastCos(x - HPI);
}

float random(vec2 st){
    return fract(sin(dot(st.xy, vec2(1111.11, 1111.11)))*11111.111);
}

float pattern(vec2 st, vec2 v, float t){
    vec2 p = floor(st + v);
    return step(t, random(1111.0001 + p*0.0001)+random(vec2(p.x, 01.0))*.5);
}


vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

vec4 mod289(vec4 x) { 
    return x - floor(x * (1.0 / 289.0)) * 289.0; 
}

vec4 permute(vec4 x) { 
    return mod289(((x*34.0)+1.0)*x); 
}

vec4 taylorInvSqrt(vec4 r) { 
    return 1.79284291400159 - 0.85373472095314 * r; 
}

float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    
    // First corner
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    
    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    
    // Permutations
    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0))
        + i.y + vec4(0.0, i1.y, i2.y, 1.0))
        + i.x + vec4(0.0, i1.x, i2.x, 1.0));
        
    // Gradients: 7x7 points over a square, mapped onto an octahedron
    float n_ = 0.142857142857; // 1.0/7.0
    vec3 ns = n_ * D.wyz - D.xzx;
    
    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
    
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);
    
    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);
    
    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);
    
    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));
    
    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww;
    
    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);
    
    // Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;
    
    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

float snoise(vec2 v) {

    // Precompute values for skewed triangular grid
    const vec4 C = vec4(0.211324865405187,
                        // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,
                        // 0.5*(sqrt(3.0)-1.0)
                        -0.577350269189626,
                        // -1.0 + 2.0 * C.x
                        0.024390243902439);
                        // 1.0 / 41.0

    // First corner (x0)
    vec2 i  = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);

    // Other two corners (x1, x2)
    vec2 i1 = vec2(0.0);
    i1 = (x0.x > x0.y)? vec2(1.0, 0.0):vec2(0.0, 1.0);
    vec2 x1 = x0.xy + C.xx - i1;
    vec2 x2 = x0.xy + C.zz;

    // Do some permutations to avoid
    // truncation effects in permutation
    i = mod289(i);
    vec3 p = permute(
            permute( i.y + vec3(0.0, i1.y, 1.0))
                + i.x + vec3(0.0, i1.x, 1.0 ));

    vec3 m = max(0.5 - vec3(
                        dot(x0,x0),
                        dot(x1,x1),
                        dot(x2,x2)
                        ), 0.0);

    m = m*m ;
    m = m*m ;

    // Gradients:
    //  41 pts uniformly over a line, mapped onto a diamond
    //  The ring size 17*17 = 289 is close to a multiple
    //      of 41 (41*7 = 287)

    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt(a0*a0 + h*h);
    m *= 1.79284291400159 - 0.85373472095314 * (a0*a0+h*h);

    // Compute final noise value at P
    vec3 g = vec3(0.0);
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * vec2(x1.x,x2.x) + h.yz * vec2(x1.y,x2.y);
    return 130.0 * dot(m, g);
}

float otherrandom (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                 * 43758.5453123);
}

float noise(vec2 st){
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = otherrandom(i);
    float b = otherrandom(i + vec2(1.0, 0.0));
    float c = otherrandom(i + vec2(0.0, 1.0));
    float d = otherrandom(i + vec2(1.0, 1.0));

    //cubic interpolation, same as smoothstep
    vec2 u = f*f*(3.0 - 2.0*f);
    // u = f*f*f*(f*(f*6.-15.)+10.);
    //u = smoothstep(0.0, 1.0, f);

    //mix 4 corner percentages
    // return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);


}

vec2 random2(vec2 st){
    st = vec2(dot(st, vec2(127.1, 311.7)), dot(st, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(st)*43758.55453123);
}

float noiseGradient(vec2 st){
    vec2 i = floor(st);
    vec2 f = fract(st);
    vec2 u = f*f*(3.0 - 2.0*f);
    return mix( mix( dot(random2(i + vec2(0.0, 0.0) ), f - vec2(0.0, 0.0) ),
                     dot(random2(i + vec2(1.0, 0.0) ), f - vec2(1.0, 0.0) ), u.x),
                mix( dot(random2(i + vec2(0.0, 1.0) ), f - vec2(0.0, 1.0) ),
                     dot(random2(i + vec2(1.0, 1.0) ), f - vec2(1.0, 1.0) ), u.x), u.y);
}

void main(){
    float time = push.misc.x;
    float mx = push.mouse.x;
    float my = push.mouse.y;
    vec2 uv = vec2(gl_FragCoord.x, -gl_FragCoord.y);
    vec2 screenUV = uv/push.viewRect.xy;
    vec2 texCoord = vec2(fragTexCoord.x, 1-fragTexCoord.y); //flip tex coords to constrain them to the specific quad
    vec2 st = texCoord * 2.0 - 1.0;
    

    vec3 color = vec3(0);

    float yaxis = plotyw(st, 0, 0.01);
    float xaxis = plotxw(st, 0, 0.01);


    float normx = clamp((mx/uv.x/100), 0.0, 1.0);
    if(fragFaceID == 0)     {//forward face

#if 0
        color = vec3(texCoord.x,texCoord.y, 0);
        color = vec3(noise((fragTexCoord)));
        // color = vec3(fract((texCoord.x)*5), fract((texCoord.y)*5), 0);
        // color = vec3(noise(vec2(0.123, 0.456)));
        // color = vec3(fract(sin(dot(st, vec2(11111.1, 1.1)))), 0, 0);
        
#else
        vec3 backcolor = vec3(1.0, 0.25 + (cos(time) * 0.125), 0.0);
        backcolor = vec3(0);
        float n = snoise(vec3(st.xy, time * 0.0000001)*10 + time)*0.5 + 0.1;
        color = vec3(n, 0.0, 0.0);
        color = mix(backcolor, color, n);
#endif

        outColor = vec4(color, 1.0);
    }
    else if(fragFaceID == 1)     {//back face
        float noise = fract(sin(dot(gl_FragCoord.xy ,vec2(12.9898,78.233))) * 43758.5453 + time);
        vec3 color = vec3(noise, 0.1, 0.25);
        outColor = vec4(color, 1.0);
    }
    else if(fragFaceID == 2)     {//top face

        color = vec3(gl_FragCoord.x/push.viewRect.x, (push.viewRect.y- gl_FragCoord.y) / push.viewRect.y, 1.0);
        float r = sin(time)*fract(length(st)*3.5)*1.0;
        float a = atan(st.y, st.x) + time;
        
        float f = cos(a*3.);
        f = abs(cos(a*12.0+time)*sin(a*3.0+time))*0.8 + 0.1;
        // f = smoothstep(-0.5, 1.0, cos(a*10.0))*0.2+0.5;
        color = vec3((smoothstep(f-0.4, f, r) - smoothstep(f, f+0.02, r)));

        outColor = vec4(color, 1.0);
    }
    else if(fragFaceID == 3)     {//bottom face
        time += 100;//the larger time is, the more patterns emerge until it becomes noise
        float y = sin(PI * st.x *time)*1.0;
        float x = cos(PI * st.x + time)+sin(y) + sin(time);

        vec3 band = fillPlot(st, x, 0.07, time);
        vec3 band2 = fillPlot(st, y, 0.07, time);
        color = mix(color, band, 0.1);
        color = mix(color, band2, band2);
        // color = mix(color, vec3(1), yaxis);
        // color = mix(color, vec3(1), xaxis);

        outColor = vec4(color, 1.0);
    }
    else if(fragFaceID == 4)     {//left face
  vec2 grid = vec2(100.0, 51.0);
        // st = screenUV;
        st *= grid;
        vec2 ipos = floor(st);
        vec2 fpos = fract(st);
        vec2 vel = vec2(time*0.5*max(grid.x, grid.y));//time

        float isSeventhRow = 1.0 - step(0.01, mod(ipos.y, 7.0));

        // Direction multiplier: -1 for normal rows, +1 for every 7th row
        // We use 2.0 * isSeventhRow - 1.0 to map from [0,1] to [-1,1]
        float directionMultiplier = -1.0 * (1.0 - 2.0 * isSeventhRow);
        // Add variation within the row based on column position
        float columnFactor = random(vec2(ipos.x, 0.3));
        float speedVariation = 0.5 + columnFactor; // Speed varies from 0.5x to 1.5x
        speedVariation = 1.0;
        // Apply direction and per-element speed variation
        vel *= vec2(directionMultiplier * speedVariation, 0.0) * random(vec2(1.0 + ipos.y, 0.1));
        

        vec2 offset = vec2(.21, 0.0);
        color.x = vel.x;

        color.r = pattern(st+offset,vel,0.5) * 0.5;
        
        //create another speed offset on the overlapped pattern
        // columnFactor = random(vec2(ipos.y, 0.3));
        speedVariation = 0.9 + columnFactor; // Speed varies from 0.5x to 1.5x
        vel *= vec2(directionMultiplier * speedVariation, 1.0) * random(vec2(1.0 + ipos.y, 0.1));

        vec3 col2 = vec3(pattern(st+offset+vec2(0.1,0),vel,0.2), 0.1, 0.0) * 0.1;
        col2.b = sin(time * 0.5) * 0.25;
        color = mix(col2, color, color.r);
        // color *= vec3(sin(time), 0.5, 0.1);
        outColor =  vec4(color, 1.0);
    }
    else if(fragFaceID == 5)     {//right face
         float x2 = pow(st.x, 2.0);
        // float funcx = 1.0 - pow(abs(st.x), 0.5);    
        float funcx  = 1.0 - pow(abs(st.x), 0.5);
        float funcx2 = 1.0 - pow(abs(st.x), 2.0) - 0.1;
        // float ex2 =    1.0 - pow(abs(st.x), 1.0);
        // float ex3 =    1.0 - pow(abs(st.x), 1.5);
        // float ex4 =    1.0 - pow(abs(st.x), 2.0);
        // float ex5 =    1.0 - pow(abs(st.x), 2.5);
        // float ex6 =    1.0 - pow(abs(st.x), 3.0);
        // float ex7 =    1.0 - pow(abs(st.x), 3.5);
        // float cosEx1 = pow(fastCos(PI * st.x / 2.0), 0.5);
        // float cosEx2 = pow(fastCos(PI * st.x / 2.0), 1.0);
        // float cosEx3 = pow(fastCos(PI * st.x / 2.0), 1.5);
        // float cosEx4 = pow(fastCos(PI * st.x / 2.0), 2.0);
        // float cosEx5 = pow(fastCos(PI * st.x / 2.0), 2.5);
        // float cosEx6 = pow(fastCos(PI * st.x / 2.0), 3.0);
        // float cosEx7 = pow(fastCos(PI * st.x / 2.0), 3.5);
        // float sinEx1 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 0.5);
        // float sinEx2 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 1.0);
        // float sinEx3 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 1.5);
        // float sinEx4 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 2.0);
        // float sinEx5 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 2.5);
        // float sinEx6 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 3.0);
        // float sinEx7 = 1.0 - pow(abs(fastSin(PI * st.x / 2.0)), 3.5);
        // float minEx1 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 0.5);
        // float minEx2 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 1.0);
        // float minEx3 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 1.5);
        // float minEx4 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 2.0);
        // float minEx5 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 2.5);
        // float minEx6 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 3.0);
        // float minEx7 = pow(min(fastCos(PI * st.x / 2.0), 1.0 - abs(st.x)), 3.5);
        // float maxEx1 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 0.5);
        // float maxEx2 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 1.0);
        // float maxEx3 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 1.5);
        // float maxEx4 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 2.0);
        // float maxEx5 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 2.5);
        // float maxEx6 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 3.0);
        float maxEx7 = 1.0 - pow(max(0.0, abs(st.x) * 2.0 - 1.0), 3.5);
        float exResult = plot(st, maxEx7);

        // Initialize color to dark gray background
        vec3 color = vec3(0.15);
        float result = plot(st, funcx);
        float result2 = plot(st, cos(st.x + time) + 0.1);
        // float result2 = plot(st, funcx2);
        float result3 = plot(st, cos(st.x + time));
        float result4 = plot(st, sin(st.x + time));
        float result5 = plot(st, cos(st.x + time) - 0.1);
        // float result5 = plot(st, doubleCubicSeat(texCoord.x, .427 + cos(time)*0.25, .720 + sin(time)*0.25));

        float cutout1 = 1.0 - result;
        float cutout2 = 1.0 - result2;
        float cutout3 = 1.0 - result3;    
        float cutout4 = 1.0 - result4;
        float cutout5 = 1.0 - result5;
        float excutout = 1.0 - exResult;
        vec3 blueResult1 = result3 * vec3(0.0, 0.0, 1.0);
        vec3 greenResult1 = result * vec3(0.0, 1.0, 0.0);
        vec3 redResult2 = result2 * vec3(1.0, 0.0, 0.0);
        vec3 col4 = result4 * vec3(1.0, 1.0, 0.0);
        vec3 col5 = result5 * vec3(1.0, 0.0, 1.0);
        vec3 excol = exResult * vec3(0.5, 1.0, 1.0);

        color = vec3(cutout1*cutout2*cutout3*cutout4*cutout5*excutout)*st.x+ (greenResult1) + redResult2 + blueResult1 + col4 + col5 + excol;

        vec3 colorA = vec3(0.549,0.241,0.512);
        vec3 colorB = vec3(0.9,0.833,0.224);
        vec3 base = mix(colorA, colorB, vec3(st.x));
        vec3 color1 = mix(base, vec3(0.0, 1.0, 0.0), result);
        vec3 color2 = mix(color1, vec3(1.0, 0.0, 0.0), result2);
        vec3 color3 = mix(color2, vec3(0.0,0.0,1.0), result3);
        vec3 color4 = mix(color3, vec3(1.0,1.0,0.0), result4);
        vec3 color5 = mix(color4, vec3(1.0,0.0,1.0), result5);
        vec3 color6 = mix(color5, vec3(1.0,0.0,1.0), exResult);
        // vec3 color7 = mix(color6, vec3(0.0,1.0,0.0), result3 - result2);


        color = color6;

        outColor = vec4(color, 1.0);
    }


    // outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
    // outColor = texture(texSampler, fragTexCoord);
    // outColor = texture(texSampler, fragTexCoord * 2); //shows 4 of the texture repeated ( *2 )




}