// Opengl version >= 1.3
#version 130

out vec2 texcoord;

in  vec3 aPos;
in  vec2 aTexCoord;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    texcoord = aTexCoord;
}
