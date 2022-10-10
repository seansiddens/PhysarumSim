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

## Examples

![slimy-gif](https://user-images.githubusercontent.com/11508260/194922974-f143b03a-6204-45b5-9a04-f73fdc884bec.gif)
![slimy1](https://user-images.githubusercontent.com/11508260/194921409-b586ff44-a5c4-4174-9de3-2ea480aa3dfe.png)
 ![slimy2](https://user-images.githubusercontent.com/11508260/194921386-fb35622e-38f7-4845-a432-63269ae84ec4.png)