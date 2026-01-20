#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;

uniform vec3  materialTint;
uniform float specStrength;
uniform float shininess;

uniform vec3  fogColor;
uniform float fogDensity;

#define MAX_TORCH 2
uniform int   uTorchCount;
uniform vec3  uTorchPos[MAX_TORCH];
uniform vec3  uTorchColor[MAX_TORCH];
uniform float uTorchRadius[MAX_TORCH];
uniform float uTorchIntensity[MAX_TORCH];

uniform float uEmissive;
uniform vec3  uEmissiveColor;

float computeShadow(vec4 fragPosLightSpace, vec3 normal)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    // clamp corect
    float ndotl = clamp(dot(normalize(normal), normalize(-lightDir)), 0.0, 1.0);

    // bias STABIL (foarte important)
    float bias = mix(0.0025, 0.0008, ndotl);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(
                shadowMap,
                projCoords.xy + vec2(x, y) * texelSize
            ).r;

            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;

    // intensitate umbră (opțional, dar recomandat)
    shadow *= 0.4;

    return shadow;
}



void main()
{
    vec4 tex = texture(diffuseTexture, TexCoord);
    if (tex.a < 0.1) discard;

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(-lightDir);

    float diff = max(dot(N, L), 0.0);

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    vec3 ambient  = ambientColor;
    vec3 diffuse  = diff * lightColor;
    vec3 specular = specStrength * spec * lightColor;

    float shadow = computeShadow(FragPosLightSpace, N);
    vec3 color = ambient + (1.0 - shadow) * (diffuse + specular);

    vec3 baseColor = tex.rgb * materialTint;
    color *= baseColor;

    for (int i = 0; i < uTorchCount && i < MAX_TORCH; i++)
    {
        vec3 toL = uTorchPos[i] - FragPos;
        float dist = length(toL);

        float att = 1.0 - clamp(dist / uTorchRadius[i], 0.0, 1.0);
        att *= att;

        vec3 Lp = normalize(toL);
        float d = max(dot(N, Lp), 0.0);

        vec3 Hp = normalize(Lp + V);
        float s = pow(max(dot(N, Hp), 0.0), shininess);

        vec3 c = uTorchColor[i] * (uTorchIntensity[i] * att);

        color += d * c * baseColor + specStrength * s * c;
    }

    color += uEmissive * uEmissiveColor;

    float dCam = length(viewPos - FragPos);
    float fogFactor = exp(-dCam * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    FragColor = vec4(mix(fogColor, color, fogFactor), tex.a);
}
