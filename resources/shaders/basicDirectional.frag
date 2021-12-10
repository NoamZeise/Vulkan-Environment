#version 450

layout(push_constant) uniform fragconstants
{
    layout(offset = 128) vec4 texOffset;
    uint texID;
} pc;

//maybe add uniform buffer for lighting

layout(set = 1, binding = 0) uniform sampler texSamp;
layout(set = 1, binding = 1) uniform texture2D textures[50];
layout(set = 2, binding = 0) uniform LightingUBO
{
    vec4 ambient;
    vec4 directionalColour;  
    vec4 directional;
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

    vec4 objectColour = texture(sampler2D(textures[pc.texID], texSamp), coord);

    if(objectColour.w == 0.0)
        discard;

    vec3 normal = normalize(inNormal);

    vec3 ambient = vec3(lighting.ambient) * lighting.ambient.w;

    vec3 lightDir = normalize(-lighting.directional.xyz);
    float lambertian = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lighting.directionalColour.xyz * lambertian;


    vec3 viewDir = normalize(-inFragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float specularIntensity = pow(max(dot(normal, halfDir), 0.0), 12.0);
    //specularIntensity = lambertian == 0.0 ? 0.0 : specularIntensity;
    vec3 specular = vec3(0.7) * specularIntensity;

    outColour = vec4(ambient + diffuse + specular, 1.0) * objectColour;
}
