#version 430 core

in vec2 TexCoord;
out vec4 FragColor;

layout(rgba32f, binding = 0) uniform image2D trailMap;

uniform vec2 u_trailMapResolution;

void main() {
    vec2 uv = TexCoord;
    uv.y = 1.0 - uv.y; // Flip y coords
    ivec2 pixel_coords = ivec2(u_trailMapResolution * uv);
    FragColor = imageLoad(trailMap, pixel_coords);
}
