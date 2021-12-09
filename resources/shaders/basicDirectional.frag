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
layout(location = 1) in vec3 inVertPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inOgNormal;

layout(location = 0) out vec4 outColour;

void main()
{
    vec2 coord = inTexCoord.xy;
    coord.x *= pc.texOffset.z;
    coord.y *= pc.texOffset.w;
    coord.x += pc.texOffset.x;
    coord.y += pc.texOffset.y;

    vec3 objectColour = texture(sampler2D(textures[pc.texID], texSamp), coord).xyz;

    vec3 ambient = vec3(lighting.ambient) * lighting.ambient.w * objectColour;

    vec3 normal = normalize(inNormal);
    vec3 ogNormal = normalize(inOgNormal);
    vec3 lightDir = normalize(-lighting.directional.xyz);
    float lambertian = max(dot(ogNormal, lightDir), 0.0);
    vec3 diffuse = lighting.directionalColour.xyz * lambertian * objectColour;

    vec3 viewDir = normalize(-inVertPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularIntensity = pow(max(dot(viewDir, reflectDir), 0.0), 4.0);
    vec3 specular = vec3(1.0) * specularIntensity * objectColour;

    outColour = vec4(ambient + diffuse + specular, 1.0);
}
