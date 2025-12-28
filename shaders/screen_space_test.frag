#version 450



layout(binding = 0) uniform UniformBufferObject{
    vec4 viewRect;
    vec4 misc;
    vec4 mouse;
}ubo;


layout(push_constant) uniform PushConstants {
    vec2 position; 
    vec2 scale;    
    vec4 color;    
    vec4 texCoords;   
    uvec4 misc; //x = drawTexture, y = type of shape to draw
    vec4 misc2;

} push;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[8];

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
    float time = ubo.misc.x;
    float mx = ubo.mouse.x;
    float my = ubo.mouse.y;
    vec2 uv = vec2(gl_FragCoord.x, -gl_FragCoord.y);
    vec2 st = vec2(fragTexCoord.x, 1-fragTexCoord.y);
    uint shape = push.misc.y;
    mx = mx  / uv.x;
    float borderpx = 2.0f;

    //doesn't work, need to find some other way of getting consistent sized borders
    // float border = 0.04 * (push.scale.x / push.scale.y);

    vec2 border = vec2(3) / push.scale;
    // vec2 border = vec2(.01);


    //border and filled center
    vec2 bl  = smoothstep(vec2(0.0), vec2(0.02) + border, st);
    vec2 tr  = smoothstep(vec2(1.0), vec2(0.98) - border, st);
    vec2 ebl = smoothstep(vec2(0.02), vec2(0.02) + border, st);
    vec2 etr = smoothstep(vec2(0.98), vec2(0.98) - border, st);

    vec2 ibl = smoothstep(vec2(0.00), border, st);
    vec2 itr = smoothstep(vec2(1.00), vec2(1.0) - border, st);


    float pct = bl.x * bl.y * tr.x * tr.y;
    float epct = ebl.x * ebl.y * etr.x * etr.y;
    float ipct = ibl.x * ibl.y * itr.x * itr.y;

    // vec4 test = vec4(vec4(pct * 0.5)) + (vec4(1 - (epct + (1-ipct))));
    vec4 fill = vec4(vec4(pct * 0.5));
    vec4 borderColor = (vec4(1 - ((ipct))));
    // vec4 test = borderColor + fill;
    vec4 test = borderColor;
    // vec3 color = vec3(.1,.9,.1);
    // vec3 color = vec3(.1,.9,.1);
            // outColor = vec4(texture(texSampler, fragTexCoord));

#if 1
    switch(shape){
        case 0:{//border
            outColor = vec4(borderColor* push.color);

              // Create ripple effect on border
            // float borderMask = 1 - ipct;

            // float rippleSpeed = 0.25;    // Speed of the ripple
            // float rippleFreq = 20.0;    // Frequency of the ripple
            
            // // Option 1: Linear ripple along the perimeter
            // // Calculate distance along perimeter (approximation)
            // float perimeterPos = max(
            //     min(st.x, 1.0-st.x) * 50.0,  // Horizontal position
            //     min(st.y, 1.0-st.y) * 50.0   // Vertical position
            // );
            // // float ripple = fract(borderMask + time * rippleSpeed);
            
            // // Option 2: Circular ripple pattern
            // float distFromCenter = length(st - 0.5) * 50.0;
            // float ripple = fract(distFromCenter - time * rippleSpeed);
            
            // // Option 3: Flowing corner-to-corner
            // // float cornerFlow = (st.x + st.y) * rippleFreq;
            // // float ripple = fract(cornerFlow + time * rippleSpeed);
            
            // // Apply ripple only to border
            // // vec4 borderColor = vec4(0.8, 0.2, 0.2, 0.5);
            // vec4 borderColor = vec4(1.0, 1.0, 1.0, 0.5);
            // vec4 rippleColor = mix(borderColor * 0.5, borderColor * 1.5, ripple);
            
            // // Inner solid color
            // vec4 innerColor = vec4(0.2, 0.0, 0.2, 0.6);
            
            // // Combine layers
            // vec4 finalColor = vec4(0.0);
            // finalColor = mix(finalColor, rippleColor, borderMask);  // Rippling border
            // // finalColor = mix(finalColor, innerColor, ipct);         // Solid inner
            
            // outColor = vec4(finalColor);
            
        }break;
        case 1:{//fill

            outColor = vec4((fill) * push.color);



        }break;
        case 2:{//border + fill
            // outColor = vec4((fill + borderColor) * push.color);
            outColor = vec4((fill + borderColor) * push.color);

        }break;
        case 3:{
            // outColor = vec4(fill * push.color * texture(texSampler, fragTexCoord));
            // outColor = vec4(texture(texSampler, fragTexCoord));
            outColor = vec4(texture(sampler2D(textures[push.misc.z], texSampler), fragTexCoord));
            // outColor = vec4(texture(sampler2D(textures[1], texSampler), fragTexCoord));

        }break;//texture
        case 4:{//border ripple + fill
             vec2 bl = smoothstep(vec2(0.0), vec2(0.05), st);
            vec2 tr = smoothstep(vec2(1.0), vec2(0.95), st);
            float pct = bl.x * bl.y * tr.x * tr.y;
            
            // Your inner area calculation
            vec2 ibl = smoothstep(vec2(0.05), vec2(0.1), st);
            vec2 itr = smoothstep(vec2(.95), vec2(0.9), st);
            float ipct = ibl.x * ibl.y * itr.x * itr.y;
            
            // Border mask (1.0 where the border is)
            float borderMask = pct - ipct;
    
            // Create ripple effect on border
            float rippleSpeed = 0.25;    // Speed of the ripple
            float rippleFreq = 20.0;    // Frequency of the ripple
            
            // Option 1: Linear ripple along the perimeter
            // Calculate distance along perimeter (approximation)
            float perimeterPos = max(
                min(st.x, 1.0-st.x) * 50.0,  // Horizontal position
                min(st.y, 1.0-st.y) * 50.0   // Vertical position
            );
            // float ripple = fract(borderMask + time * rippleSpeed);
            
            // Option 2: Circular ripple pattern
            float distFromCenter = length(st - 0.5) * 10.0;
            float ripple = fract(distFromCenter - time * rippleSpeed);
            
            // Option 3: Flowing corner-to-corner
            // float cornerFlow = (st.x + st.y) * rippleFreq;
            // float ripple = fract(cornerFlow + time * rippleSpeed);
            
            // Apply ripple only to border
            // vec4 borderColor = vec4(0.8, 0.2, 0.2, 0.5);
            vec4 borderColor = vec4(1.0, 1.0, 1.0, 0.5);
            vec4 rippleColor = mix(borderColor * 0.5, borderColor * 1.5, ripple);
            
            // Inner solid color
            vec4 innerColor = vec4(0.2, 0.0, 0.2, 0.6);
            
            // Combine layers
            vec4 finalColor = vec4(0.0);
            finalColor = mix(finalColor, rippleColor, borderMask);  // Rippling border
            finalColor = mix(finalColor, innerColor, ipct);         // Solid inner
            
            outColor = vec4(finalColor);
        }break;
        case 5:{
            outColor = push.color;
        }break;
        case 6:{//text
            outColor =      vec4(texture(sampler2D(textures[push.misc.z], texSampler), fragTexCoord) * push.color);
        }break;


        

    }
#endif


}



/*


void main() {
    //border and filled center
    vec2 bl  = smoothstep(vec2(0.0), vec2(0.08), st);
    vec2 tr  = smoothstep(vec2(1.0), vec2(0.92), st);
    vec2 ebl = smoothstep(vec2(0.02), vec2(0.05), st);
    vec2 etr = smoothstep(vec2(0.98), vec2(0.95), st);

    vec2 ibl = smoothstep(vec2(0.00), vec2(0.04), st);
    vec2 itr = smoothstep(vec2(1.00), vec2(0.96), st);


    float pct = bl.x * bl.y * tr.x * tr.y;
    float epct = ebl.x * ebl.y * etr.x * etr.y;
    float ipct = ibl.x * ibl.y * itr.x * itr.y;

    float alpha = 1 - epct;
    vec4 test = vec4(vec4(pct * 0.5)) + (vec4(1 - (epct + (1-ipct))));
    vec3 color = vec3(.1,.9,.1);
    outColor = vec4(test.xyz * color, 0.5);
}
*/


/*

    vec2 bl = smoothstep(vec2(0.0), vec2(0.05), st);
    vec2 tr = smoothstep(vec2(1.0), vec2(0.95), st);
    float pct = bl.x * bl.y * tr.x * tr.y;
    
    // Your inner area calculation
    vec2 ibl = smoothstep(vec2(0.05), vec2(0.1), st);
    vec2 itr = smoothstep(vec2(.95), vec2(0.9), st);
    float ipct = ibl.x * ibl.y * itr.x * itr.y;
    
    // Border mask (1.0 where the border is)
    float borderMask = pct - ipct;
    
    // Create ripple effect on border
    float rippleSpeed = 0.25;    // Speed of the ripple
    float rippleFreq = 20.0;    // Frequency of the ripple
    
    // Option 1: Linear ripple along the perimeter
    // Calculate distance along perimeter (approximation)
    float perimeterPos = max(
        min(st.x, 1.0-st.x) * 50.0,  // Horizontal position
        min(st.y, 1.0-st.y) * 50.0   // Vertical position
    );
    float ripple = fract(borderMask + u_time * rippleSpeed);
    
    // Option 2: Circular ripple pattern
    float distFromCenter = length(st - 0.5) * 10.0;
    ripple = fract(distFromCenter - u_time * rippleSpeed);
    
    // Option 3: Flowing corner-to-corner
    // float cornerFlow = (st.x + st.y) * rippleFreq;
    // float ripple = fract(cornerFlow + time * rippleSpeed);
    
    // Apply ripple only to border
    vec4 borderColor = vec4(0.8, 0.2, 0.2, 0.5);
    vec4 rippleColor = mix(borderColor * 0.5, borderColor * 1.5, ripple);
    
    // Inner solid color
    vec4 innerColor = vec4(0.2, 0.0, 0.2, 0.6);
    
    // Combine layers
    vec4 finalColor = vec4(0.0);
    finalColor = mix(finalColor, rippleColor, borderMask);  // Rippling border
    finalColor = mix(finalColor, innerColor, ipct);         // Solid inner
    
    gl_FragColor = vec4(finalColor);

*/