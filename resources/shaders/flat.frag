#version 450

layout(push_constant) uniform fragconstants
{
    layout(offset = 128) vec4 colour;
    vec4 texOffset;
    uint texID;
} pc;


layout(set = 2, binding = 0) uniform sampler texSamp;
layout(set = 2, binding = 1) uniform texture2D textures[200];


layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inVertPos;

layout(location = 0) out vec4 outColour;

void main()
{
    vec2 coord = inTexCoord;
    coord.x *= pc.texOffset.z;
    coord.y *= pc.texOffset.w;
    coord.x += pc.texOffset.x;
    coord.y += pc.texOffset.y;

    vec4 col = texture(sampler2D(textures[pc.texID], texSamp), coord) * pc.colour;

    if(col.w == 0)
        discard;
    outColour = col;
}
