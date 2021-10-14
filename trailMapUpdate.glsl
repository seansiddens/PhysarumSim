#version 430
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D trailMap;

uniform float u_deltaTime;
uniform float u_decaySpeed;
uniform float u_diffuseStrength;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    vec3 prev_col = imageLoad(trailMap, pixel_coords).rgb;
    vec3 new_col = prev_col;

    vec3 sum = vec3(0.0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            int sampleX = pixel_coords.x + x;
            int sampleY = pixel_coords.y + y;

            sum += imageLoad(trailMap, ivec2(sampleX, sampleY)).rgb;
        }
    }
    vec3 mean = sum / 9.0;

    // Diffuse
    new_col = mix(prev_col, mean, u_diffuseStrength * u_deltaTime);

    // Decay
    new_col = max(vec3(0.0), new_col - u_decaySpeed * u_deltaTime);


    imageStore(trailMap, pixel_coords, vec4(new_col, 1.0));
}


