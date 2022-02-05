#version 450

layout(push_constant) uniform fragconstants
{
    layout(offset = 128) vec4 colour;
    vec4 texOffset;
    uint texID;
} pc;

//maybe add uniform buffer for lighting

layout(set = 2, binding = 0) uniform sampler texSamp;
layout(set = 2, binding = 1) uniform texture2D textures[200];
layout(set = 3, binding = 0) uniform LightingUBO
{
    vec4 ambient;
    vec4 diffuse;  
    vec4 specular;
    vec4 direction;
} lighting;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inFragPos;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColour;


void main()
{
    vec2 coord = inTexCoord.xy;
    coord.x *= pc.texOffset.z;
    coord.y *= pc.texOffset.w;
    coord.x += pc.texOffset.x;
    coord.y += pc.texOffset.y;

    vec4 objectColour = texture(sampler2D(textures[pc.texID], texSamp), coord) * pc.colour;

    if(objectColour.w == 0.0)
        discard;

    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(-lighting.direction.xyz);

    vec3 ambient = lighting.ambient.xyz * lighting.ambient.w;

    float lambertian = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lighting.diffuse.xyz * lighting.diffuse.w * lambertian;

    float specularIntensity = 0.0;
    if(lambertian > 0.0)
    {
        vec3 viewDir = normalize(-inFragPos);
        
        vec3 halfDir = normalize(lightDir + viewDir);
        specularIntensity = pow(max(dot(normal, halfDir), 0.0), lighting.specular.w);
    }
    vec3 specular = lighting.specular.xyz * specularIntensity;

    outColour = vec4(ambient + diffuse + specular, 1.0) * objectColour;
}
