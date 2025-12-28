#version 450


layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;


void main(){
    outColor = fragColor;
    // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    // outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
    // outColor = texture(texSampler, fragTexCoord);
    // outColor = texture(texSampler, fragTexCoord * 2); //shows 4 of the texture repeated ( *2 )




}