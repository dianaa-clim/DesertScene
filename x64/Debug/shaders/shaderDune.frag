#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D sandTexture;
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;

uniform vec3 fogColor;
uniform float fogDensity;

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
    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);

    float diff = max(dot(N, L), 0.0);

    vec3 ambient  = ambientColor;
    vec3 diffuse  = diff * lightColor;

    float shadow = computeShadow(FragPosLightSpace, N);
    vec3 lighting = ambient + (1.0 - shadow) * diffuse;

    vec2 uv = FragPos.xz * 0.04;
    vec3 sand = texture(sandTexture, uv).rgb;

    vec3 color = sand * lighting;

    float d = length(viewPos - FragPos);
    float fogFactor = exp(-fogDensity * d);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    FragColor = vec4(mix(fogColor, color, fogFactor), 1.0);
}
