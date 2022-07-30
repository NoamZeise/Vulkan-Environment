#version 450

layout(push_constant) uniform fragconstants
{
    vec4 colour;
    vec4 texOffset;
    uint texID;
    uint normalMap;
} pc;

//maybe add uniform buffer for lighting

layout(set = 2, binding = 0) uniform sampler texSamp;
layout(set = 2, binding = 1) uniform texture2D textures[20];
layout(set = 3, binding = 0) uniform LightingUBO
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 direction;
} lighting;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inFragPos_world;
layout(location = 2) in vec3 inLightDir_cam;
layout(location = 3) in vec3 inEyeDir_cam;
layout(location = 4) in vec3 inLightDir_tangent;
layout(location = 5) in vec3 inEyeDir_tangent;

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
        
    vec3 normal = normalize(vec3(0, 0, 0.5));
    if(pc.normalMap != 0)
    {
        normal =
               normalize(
                texture(
                    sampler2D(
                        textures[pc.normalMap], texSamp
                        ), coord).rgb * 2.0 - 1.0);
    }

    vec3 ambient = objectColour.rgb * lighting.ambient.xyz * lighting.ambient.w;

    vec3 d = lighting.direction.xyz - inFragPos_world;
    
    float dist = sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
    float atten = 1.0 / (1.0f + 0.014f * dist + 0.007f * dist * dist);

    vec3 l = normalize(inLightDir_tangent);
    vec3 e = normalize(inEyeDir_tangent);

    float lambertian = clamp(dot(l, normal), 0.0, 1.0);
    vec3 diffuse = objectColour.xyz * lighting.diffuse.xyz * lighting.diffuse.w * lambertian;
    
    
    float specularIntensity = 0.0f;
    if (lambertian > 0.0f)
    {
       vec3 r = reflect(-l, normal);   
       specularIntensity = pow(clamp(dot(e, r), 0.0, 1.0), lighting.specular.w);
    }
    vec3 specular = lighting.specular.xyz * specularIntensity;

    outColour = vec4((ambient + diffuse + specular) * atten, objectColour.a);
    //outColour.xyz = ambient + diffuse;
 }