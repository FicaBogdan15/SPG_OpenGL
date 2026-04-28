#version 330 core

#define MAX_LIGHTS 9

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D texture1;
uniform samplerCube shadowMap0;
uniform samplerCube shadowMap1;
uniform samplerCube shadowMap2;
uniform samplerCube shadowMap3;
uniform samplerCube shadowMap4;
uniform samplerCube shadowMap5;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float ambientStrength;

uniform int numLights;
uniform int numShadowLights;
uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightRanges[MAX_LIGHTS];
uniform vec3 viewPos;
uniform float shadowFarPlane;
uniform bool useLighting;
uniform float emissiveStrength;
uniform float uvScale;
uniform bool useWorldUV;
uniform vec3 tintColor;
uniform float tintStrength;

const vec3 gridSamplingDisk[20] = vec3[](
    vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0), vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0),
    vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0),
    vec3( 1.0,  1.0,  0.0), vec3( 1.0, -1.0,  0.0), vec3(-1.0, -1.0,  0.0), vec3(-1.0,  1.0,  0.0),
    vec3( 1.0,  0.0,  1.0), vec3(-1.0,  0.0,  1.0), vec3( 1.0,  0.0, -1.0), vec3(-1.0,  0.0, -1.0),
    vec3( 0.0,  1.0,  1.0), vec3( 0.0, -1.0,  1.0), vec3( 0.0, -1.0, -1.0), vec3( 0.0,  1.0, -1.0)
);

float sampleShadowMap(int lightIndex, vec3 sampleDir)
{
    if (lightIndex == 0) return texture(shadowMap0, sampleDir).r;
    if (lightIndex == 1) return texture(shadowMap1, sampleDir).r;
    if (lightIndex == 2) return texture(shadowMap2, sampleDir).r;
    if (lightIndex == 3) return texture(shadowMap3, sampleDir).r;
    if (lightIndex == 4) return texture(shadowMap4, sampleDir).r;
    return texture(shadowMap5, sampleDir).r;
}

float calculatePointShadow(int lightIndex, vec3 norm)
{
    if (lightIndex >= numShadowLights)
        return 0.0;

    vec3 fragToLight = FragPos - lightPositions[lightIndex];
    float currentDepth = length(fragToLight);

    if (currentDepth >= shadowFarPlane)
        return 0.0;

    vec3 lightDir = normalize(lightPositions[lightIndex] - FragPos);

    float cosTheta = clamp(dot(norm, lightDir), 0.0, 1.0);
    float bias = mix(0.04, 0.005, cosTheta);
    bias *= currentDepth / shadowFarPlane;
    bias = clamp(bias, 0.003, 0.035);

    float viewDistance = length(viewPos - FragPos);
    float diskRadius = 0.05 + (viewDistance / shadowFarPlane) * 0.1;

    float shadow = 0.0;
    const int samples = 20;

    for (int i = 0; i < samples; i++)
    {
        vec3 sampleVec = fragToLight + gridSamplingDisk[i] * diskRadius;
        float closestDepth = sampleShadowMap(lightIndex, sampleVec);
        closestDepth *= shadowFarPlane;

        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= float(samples);

    return shadow;
}

void main()
{
    vec2 finalUV;
    if (useWorldUV) {
        finalUV = FragPos.xz / 150.0;
    } else {
        finalUV = TexCoord * uvScale;
    }

    vec4 texColor = texture(texture1, finalUV);
    vec3 surfaceColor = mix(texColor.rgb, texColor.rgb * tintColor, tintStrength);

    if (!useLighting)
    {
        vec3 unlit = surfaceColor + emissiveStrength * surfaceColor;
        FragColor = vec4(unlit, texColor.a);
        return;
    }

    vec3 norm = normalize(Normal);

    vec3 ambient = ambientStrength * sunColor * surfaceColor;

    vec3 moonDir = normalize(-sunDirection);
    float moonDiff = max(dot(norm, moonDir), 0.0);
    vec3 result = ambient + moonDiff * sunColor * surfaceColor;

    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        vec3 toLight = lightPositions[i] - FragPos;
        float dist = length(toLight);
        vec3 lightDir = normalize(toLight);

        float diff = max(dot(norm, lightDir), 0.0);

        float radius = max(lightRanges[i], 0.001);
        float att_linear = 4.5 / radius;
        float att_quadratic = 75.0 / (radius * radius);
        float attenuation = 1.0 / (1.0 + att_linear * dist + att_quadratic * dist * dist);
        attenuation = max(attenuation - 0.01, 0.0);

        float shadow = calculatePointShadow(i, norm);

        vec3 diffuse = diff * lightColors[i] * surfaceColor;
        result += (1.0 - shadow) * diffuse * attenuation;
    }

    result += emissiveStrength * surfaceColor;

    FragColor = vec4(result, texColor.a);
}
