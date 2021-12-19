#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "Shader.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>

#define AGENT_WRK_GRP_SIZE 8
#define NUM_AGENTS AGENT_WRK_GRP_SIZE * 10000

struct Agent {
    float x, y, heading;
};

// Returns a random real in [0, 1)
float random_uniform() {
    return (float) random() / (float) ((long int) RAND_MAX + 1);
}

int main() {
    int windowWidth = 800;
    int windowHeight = 800;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("Physarum Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }

    // Initialize rendering context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    SDL_GL_SetSwapInterval(0); // No vsync

    // Initialize GL Extension Wrangler (GLEW)
    glewExperimental = GL_TRUE; // Please expose OpenGL 3.x+ interfaces
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup platform/renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 430");


    // Compile and link our shaders

    // Shader for reading from the trail map image texture and displaying it to the screen
    Shader displayTrailMap("../displayVert.glsl", "../displayFrag.glsl");

    // Compute shader for updating agents and writing to the trail map image
    ComputeShader agentUpdate("../agentUpdate.glsl");

    // Compute shader for updating the trail map with the diffuse and decay steps
    // Compute threads are run 1 per pixel in the trail map
    ComputeShader trailMapUpdate("../trailMapUpdate.glsl");

    // Set up vertex data for a fullscreen quad
    float vertices[] = {
            // Positions          // Texture coords
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,       // Top right
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,       // Bottom right
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,       // Bottom left
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f        // Top left
    };
    // Indexed drawing of quad
    unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
    };

    // Create VAO for fullscreen quad
    unsigned int quadVAO, quadVBO, quadEBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) nullptr);
    glEnableVertexAttribArray(0);
    // Tex coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Shader Storage Buffer Object (SSBO) for storing positions and headings of agents
    GLuint agentSSBO;
    glGenBuffers(1, &agentSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, agentSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_AGENTS * sizeof(struct Agent), nullptr, GL_STATIC_DRAW);
    // Map data store to CPU address space, allowing us to directly write to it
    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    auto agents = (struct Agent *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                      NUM_AGENTS * sizeof(struct Agent),
                                                      bufMask);
    // Initialize with random positions
    for (int i = 0; i < NUM_AGENTS; i++) {
        agents[i].x = random_uniform();
        agents[i].y = random_uniform();
        agents[i].heading = random_uniform();
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, agentSSBO);

    // Create texture for the trail map which will eventually be rendered to screen
    GLuint trailMap;
    int trailMapWidth = 2048;
    int trailMapHeight = 2048;
    glGenTextures(1, &trailMap);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, trailMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, trailMapWidth, trailMapHeight, 0, GL_RGBA, GL_FLOAT,
                 nullptr);

    // Simulation parameters
    float agentSpeed = 0.05;
    float decaySpeed = 0.0500;
    float diffuseStrength = 20.0;
    float sensorDistance = 0.010;
    float sensorAngle = .20;
    float rotationSpeed = 5.0;

    // MAIN RENDER LOOP
    uint64_t performanceFreq = SDL_GetPerformanceFrequency();
    double deltaTime;
    uint64_t currTime;
    uint64_t prevTime = SDL_GetPerformanceCounter();
    unsigned int frame = 0;
    bool shouldLoop = true;
    bool showDemoWindow = true;
    bool updateAgents = false;
    while (shouldLoop) {
        // Timing
        currTime = SDL_GetPerformanceCounter();
        deltaTime = (1000.0 * (double) (currTime - prevTime)) / (double) performanceFreq;
        prevTime = currTime;

        // Process event queue
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Forward to Imgui
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    shouldLoop = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            shouldLoop = false;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            glViewport(0, 0, event.window.data1, event.window.data2);
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            shouldLoop = false;
                            break;
                        case SDLK_RETURN:
                            updateAgents = !updateAgents;
                            break;
                    }
                    break;
            }
        }  // End of event processing

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        const float PAD = 10.0f;
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = work_pos.x + work_size.x - PAD;
        window_pos_pivot.x = 1.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::Begin("Physarum Sim");
        ImGui::Text("Application average:\n%.3f ms/frame (%.1f FPS)", deltaTime, 1000.0 / deltaTime);
        ImGui::Separator();
        ImGui::DragInt("Trail Map Width", &trailMapWidth, 1.0f, 0, 1920);
        ImGui::DragInt("Trail Map Height", &trailMapHeight, 1.0f, 0, 1080);
        ImGui::SliderFloat("Agent speed", &agentSpeed, 0.0, 1.0f);
        ImGui::SliderFloat("Diffuse strength", &diffuseStrength, 0.0, 100.0f);
        ImGui::SliderFloat("Decay speed", &decaySpeed, 0.0f, 1.0f);
        ImGui::SliderFloat("Sensor Angle", &sensorAngle, 0.0f, 0.5f);
        ImGui::SliderFloat("Sensor Distance", &sensorDistance, 0.0f, 0.2f);
        ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 10.0f);
        ImGui::End();

        // Update trail map texture resolution
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, trailMapWidth, trailMapHeight, 0, GL_RGBA, GL_FLOAT,
                     nullptr);

        if (updateAgents) {
            // Rendering
            // Dispatch agentUpdate to update agent positions
            agentUpdate.use();
            agentUpdate.uniform1f("u_deltaTime", (float) (deltaTime / 1000.0));
            agentUpdate.uniform2f("u_trailMapResolution", (float) trailMapWidth, (float) trailMapHeight);
            agentUpdate.uniform1ui("u_frame", frame);
            agentUpdate.uniform1f("u_agentSpeed", agentSpeed);
            agentUpdate.uniform1f("u_sensorDistance", sensorDistance);
            agentUpdate.uniform1f("u_sensorAngle", sensorAngle);
            agentUpdate.uniform1f("u_rotationSpeed", rotationSpeed);
            glBindImageTexture(0, trailMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glDispatchCompute(NUM_AGENTS / AGENT_WRK_GRP_SIZE, 1, 1);

            // Make sure writing to image has finished before read
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            // Dispatch trailMapUpdate to write to image texture
            trailMapUpdate.use();
            trailMapUpdate.uniform1f("u_deltaTime", (float) (deltaTime / 1000.0));
            trailMapUpdate.uniform1f("u_decaySpeed", decaySpeed);
            trailMapUpdate.uniform1f("u_diffuseStrength", diffuseStrength);
            glBindImageTexture(0, trailMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glDispatchCompute((GLuint) trailMapWidth, (GLuint) trailMapHeight, 1);

            // Make sure writing to image has finished before read
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        }

        // Read from image texture and display to fullscreen quad
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        displayTrailMap.use();
        displayTrailMap.uniform2f("u_trailMapResolution", (float) trailMapWidth, (float) trailMapHeight);
        glBindVertexArray(quadVAO);
        glBindImageTexture(0, trailMap, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // Render GUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData((ImGui::GetDrawData()));

        // Swap buffers
        SDL_GL_SwapWindow(window);

        // Increment frame counter
        frame += 1;
    } // End of main loop

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
