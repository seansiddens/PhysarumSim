#version 430
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D trailMap;

layout(std140, binding = 4) buffer Agent {
    vec3 Agents[ ];
};

#define EPSILON 0.0001

uniform vec2 u_trailMapResolution;
uniform float u_deltaTime;
uniform uint u_frame;
uniform float u_agentSpeed;
uniform float u_sensorDistance;
uniform float u_sensorAngle;
uniform float u_rotationSpeed;

const float PI = 3.141592653;

uint rng_state = u_frame;

uvec2 pcg2d(uvec2 v) {
    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v>>16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v>>16u);

    return v;
}

uint pcg_hash(uint i) {
    uint state = rng_state * i * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    rng_state = (word >> 22u) ^ word;
    return rng_state;
}

// Returns a random float in [0.0, 1.0]
float random_float() {
    uint hash = pcg_hash(rng_state);
    return hash * (1.0 / float(0xffffffffu));
}

// Returns the trail concentration (RGB) at a sensor
vec3 sense(vec2 pos, float heading, float sensorOffset) {
    // Directional vector towards the sensor
    vec2 sensorDir = vec2(cos(2.0 * PI * (heading + sensorOffset)), sin(2.0 * PI * (heading + sensorOffset)));

    // Position of the sensor
    vec2 sensePos = pos + sensorDir * u_sensorDistance;

    if (sensePos.x >= 0 && sensePos.x < 1.0 && sensePos.y >= 0 && sensePos.y < 1.0) {
        ivec2 senseCoords = ivec2(sensePos * u_trailMapResolution);
        return imageLoad(trailMap, senseCoords).rgb;

    } else {
        // Sensor lies outside of bounds, return 0
        return vec3(0.0);
    }
}


void main() {
    // Index into global workspace
    uint gid = gl_GlobalInvocationID.x;

    uint hash = pcg_hash(gid);

    // Sense
    vec3 senseForward = sense(Agents[gid].xy, Agents[gid].z, 0.0);
    vec3 senseLeft    = sense(Agents[gid].xy, Agents[gid].z, u_sensorAngle);
    vec3 senseRight   = sense(Agents[gid].xy, Agents[gid].z, -u_sensorAngle);

    float randomDirChance = 0.0;
    if (random_float() > randomDirChance) {
        // Rotate based on sense
        float randomScale = random_float() * 0.5 + 0.5;
        if (senseLeft.r > senseForward.r && senseLeft.r > senseRight.r) {
            // Turn left
            float angleOffset = mix(0.0, u_sensorAngle, u_rotationSpeed * u_deltaTime);
            Agents[gid].z += angleOffset * randomScale;
        } else if (senseRight.r > senseForward.r && senseRight.r > senseLeft.r) {
            // Turn right
            float angleOffset = mix(0.0, u_sensorAngle, u_rotationSpeed * u_deltaTime);
            Agents[gid].z -= angleOffset * randomScale;
        } else if (senseRight.r > senseForward.r && senseLeft.r > senseForward.r) {
            float angleOffset = mix(0.0, u_sensorAngle, u_rotationSpeed * u_deltaTime);
            // Turn randomly
            if (random_float() > 0.5) {
                Agents[gid].z -= angleOffset * randomScale;
            } else {
                Agents[gid].z += angleOffset * randomScale;
            }
        }
    } else {
        Agents[gid].z = random_float();
    }

    // Get direction of agent
    vec2 dir = vec2(cos(2.0 * PI * Agents[gid].z), sin(2.0 * PI * Agents[gid].z));

    // Update agent position
    vec2 newPos = Agents[gid].xy + dir * u_agentSpeed * u_deltaTime;
    newPos = mod(newPos, 1.0);

    // Set position
    Agents[gid].xy = newPos;

    // Write to image
    ivec2 pixel_coords = ivec2(Agents[gid].x * u_trailMapResolution.x, Agents[gid].y * u_trailMapResolution.y);
    imageStore(trailMap, pixel_coords, vec4(1.0));
}


