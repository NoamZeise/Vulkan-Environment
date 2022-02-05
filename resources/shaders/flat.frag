#version 450

layout(push_constant) uniform fragconstants
{
    layout(offset = 128) vec4 colour;
    vec4 texOffset;
    uint texID;
} pc;


layout(set = 2, binding = 0) uniform sampler texSamp;
layout(set = 2, binding = 1) uniform texture2D textures[200];

layout(set = 3, binding = 0) readonly buffer PerInstanceBuffer {
    vec4 colour;
    vec4 texOffset;
    uint texID;
} pib[300];

layout(location = 0) in vec3 inTexCoord;
layout(location = 1) in vec3 inVertPos;

layout(location = 0) out vec4 outColour;

vec4 calcColour(vec4 texOffset, vec4 colour, uint texID)
{
    vec2 coord = inTexCoord.xy;
    coord.x *= texOffset.z;
    coord.y *= texOffset.w;
    coord.x += texOffset.x;
    coord.y += texOffset.y;

    vec4 col = texture(sampler2D(textures[texID], texSamp), coord) * colour;

    if(col.w == 0)
        discard;
    return col;
}

void main()
{
    if(pc.texID == 0)
    {
        uint index = uint(inTexCoord.z);
        outColour = calcColour(pib[index].texOffset, pib[index].colour, pib[index].texID);
    }
    else
    {
      outColour = calcColour(pc.texOffset, pc.colour, pc.texID);
    }
}
