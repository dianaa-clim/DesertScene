#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;

uniform float uTime;
uniform vec3  uWindDir;
uniform float uWindStrength;
uniform int   uEnableWind;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

void main()
{
    vec3 pos = aPos;

    if (uEnableWind == 1)
    {
        float factor = clamp((aPos.y - 0.3) * 1.5, 0.0, 1.0);
        float w = sin(uTime * 1.8 + aPos.y * 2.0) * uWindStrength;
        pos += uWindDir * w * factor;
    }

    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = worldPos.xyz;

    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    TexCoord = aTexCoord;

    FragPosLightSpace = lightSpaceTrMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}