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

float computeShadow(vec3 N)
{
    vec3 proj = FragPosLightSpace.xyz / FragPosLightSpace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    vec3 Ln = normalize(-lightDir);
    float bias = max(0.006 * (1.0 - dot(normalize(N), Ln)), 0.0015);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float pcfDepth = texture(shadowMap,
            proj.xy + vec2(x, y) * texelSize).r;

        shadow += (proj.z - bias > pcfDepth) ? 1.0 : 0.0;
    }

    shadow /= 9.0;
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

    float hemi = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyAmb    = vec3(0.22, 0.28, 0.38) * 0.7;
    vec3 groundAmb = vec3(0.30, 0.22, 0.16) * 0.6;
    vec3 hemiAmb   = mix(groundAmb, skyAmb, hemi);

    vec3 ambient  = ambientColor + hemiAmb;
    vec3 diffuse  = diff * lightColor;
    vec3 specular = specStrength * spec * lightColor;

    vec3 baseColor = tex.rgb * materialTint;

    float shadow = computeShadow(N);

    vec3 color =
        (ambient + (1.0 - shadow) * diffuse) * baseColor +
        (1.0 - shadow) * specular;

    // TORȚE
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

    color = mix(fogColor, color, fogFactor);
    FragColor = vec4(color, tex.a);
}