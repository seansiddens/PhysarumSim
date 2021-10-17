# Physarum Polycephalum Simulation

An agent-based simulation of the foraging and exploratory behavior of slime molds.
Uses OpenGL compute shaders to update the positions of agents every time step.

### Algorithm Overview:
1. Agents "sense" by sampling a trail map texture containing food source stimuli
2. Change agent position and heading depending on this sample
3. Deposit trail at the agent's position in the trail map texture
4. Diffuse trail map with a 3x3 mean filter
5. Decay trail map values

Simulation parameters such as texture resolution, sensor angle, sensor distance, decay speed, etc.
can be modiefied using the ImGui window. 
