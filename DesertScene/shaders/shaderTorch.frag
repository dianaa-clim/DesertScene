#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D diffuseTexture;

uniform vec3 ambientColor;
uniform vec3 emissiveColor;
uniform float emissiveStrength;

void main()
{
    vec4 tex = texture(diffuseTexture, TexCoord);
    if (tex.a < 0.1) discard;

    vec3 base = tex.rgb;

    vec3 color =
        base * ambientColor +
        base * emissiveColor * emissiveStrength;

    FragColor = vec4(color, tex.a);
}
