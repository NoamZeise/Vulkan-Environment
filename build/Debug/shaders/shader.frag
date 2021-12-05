#version 450

layout(push_constant) uniform fragconstants
{
    layout(offset = 64)vec4 colour;
    vec4 texOffset;
    uint texID;
} pc;

layout(set = 1, binding = 0) uniform sampler texSamp;
layout(set = 1, binding = 1) uniform texture2D textures[200];


layout(location = 0) in vec2 inTexCoord;
layout(location = 1) flat in uint TexID;

layout(location = 0) out vec4 outColour;

void main()
{
    vec2 coord = inTexCoord.xy;
    coord.x *= pc.texOffset.z;
    coord.y *= pc.texOffset.w;
    coord.x += pc.texOffset.x;
    coord.y += pc.texOffset.y;
    //highp int texCoord = int(inTexCoord.z);
    if(pc.texID == 0)
        outColour = texture(sampler2D(textures[TexID], texSamp), coord) * pc.colour;
    else
        outColour = texture(sampler2D(textures[pc.texID], texSamp), coord) * pc.colour;
}
