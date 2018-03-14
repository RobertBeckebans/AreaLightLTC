-- Vertex
// IN
layout (location = 0) in vec3 inPosition;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    mat4 worldViewProj = uProjection*uView*uWorld;
	gl_Position = worldViewProj * vec4(inPosition, 1.0);
}