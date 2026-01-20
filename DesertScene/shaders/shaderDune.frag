#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D sandTexture;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;

uniform vec3 fogColor;
uniform float fogDensity;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);

    float diff = max(dot(N, L), 0.0);

    vec3 color = ambientColor * 0.7 +
             diff * lightColor * 0.9;


    // nisip procedural (fără UV)
    vec2 uv = FragPos.xz * 0.04;
    vec3 sand = texture(sandTexture, uv).rgb;

    vec3 result = sand * color;

    float d = length(viewPos - FragPos);
    float fogFactor = exp(-fogDensity * d);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    FragColor = vec4(mix(fogColor, result, fogFactor), 1.0);
}
