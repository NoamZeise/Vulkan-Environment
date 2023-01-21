#version 450

layout(push_constant) uniform fragconstants
{
    vec4 colour;
    vec4 texOffset;
    uint texID;
} pc;

layout(set = 3, binding = 0) uniform sampler texSamp;
layout(set = 3, binding = 1) uniform texture2D textures[20];
layout(set = 4, binding = 0) uniform LightingUBO
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 direction;
    vec4 camPos;
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

    vec4 objectColour = vec4(1);
    if(pc.texID == 0)
        objectColour = pc.colour;
    else
        objectColour = texture(sampler2D(textures[pc.texID], texSamp), coord) * pc.colour;


    if(objectColour.w == 0.0)
        discard;

    vec3 ambient = lighting.ambient.xyz * lighting.ambient.w;

    vec3 normal = normalize(inNormal);
    
    vec3 lightDir = normalize(-lighting.direction.xyz);

    float lambertian = max(dot(lightDir, normal), 0.0);
    
    vec3 diffuse = lighting.diffuse.xyz * lighting.diffuse.w * lambertian;

    float specularIntensity = 0.0;
    if(lambertian > 0.0)
    {
        vec3 viewDir = normalize(lighting.camPos.xyz - inFragPos);

        vec3 halfDir = normalize(lightDir + viewDir);
        specularIntensity = pow(max(dot(normal, halfDir), 0.0), lighting.specular.w);
    }
    vec3 specular = lighting.specular.xyz * specularIntensity;

    outColour = vec4(ambient + diffuse + specular, 1.0) * objectColour;
}
