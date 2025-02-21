#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform vec3 skyColor;

void main()
{    
    vec4 texColor = texture(skybox, TexCoords);
    FragColor = mix(vec4(skyColor, 1.0), texColor, 0.5);
}