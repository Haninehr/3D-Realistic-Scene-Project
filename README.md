# 3D Realistic Scene Project

![Screenshot 1](./screenshots/Image1.png)
![Screenshot 2](./screenshots/image2.png)
![Screenshot 3](./screenshots/image3.png)

## Overview
This project is a realistic 3D scene renderer built using OpenGL in C++. It simulates a detailed bedroom environment with interactive elements such as furniture, lighting, doors, windows, and camera controls. The scene includes dynamic lighting (point lights, directional sunlight), texture mapping, and basic animations for doors and windows. Models are loaded from OBJ files, and textures enhance realism.
The project demonstrates concepts in computer graphics, including shader-based rendering, matrix transformations, lighting models (ambient, diffuse, specular), and user interaction via keyboard and mouse.

## Features

•Interactive Camera Modes:<br>
Auto-orbit mode for smooth circling around the scene.<br>
Free-FPS mode with mouse-look and WASD movement (clamped within room bounds).<br>

•Dynamic Lighting:<br>
Sunlight (directional) influenced by window/door openness.<br>
Point lights: Desk lamp, ceiling light, and door light.<br>
Toggle lights and adjust intensity.<br>

•Animations and Interactions:<br>
Open/close doors and windows with sound effects (Windows-only).<br>
Toggle PC screen, lamps, and other elements.<br>
Smooth animations for door/window movements.<br>

•3D Models and Textures:
Loaded OBJ models: Bed, desk, chair, lamp, PC, wardrobe, bike, etc.<br>
Textures for walls, floor, ceiling, and objects (e.g., wood, metal, posters).<br>

•HUD and UI:
On-screen FPS display (smoothed and calibrated).<br>
Help menu with controls and status (toggle with 'H').<br>
Fullscreen support (F11).<br>

•Performance:
FPS calculation with smoothing for stable display.<br>
Clamped camera to prevent clipping through walls.<br>


## Requirements

•Operating System: Windows (due to sound playback via PlaySound). Can be adapted for other OS.
•Libraries:<br>
OpenGL (core profile via GLEW).<br>
GLUT/FreeGLUT for windowing and input.<br>
GLM for matrix math.<br>
STB_IMAGE for texture loading.<br>
TinyOBJLoader for OBJ model loading.<br>

•Compiler: Visual Studio or any C++ compiler supporting C++11 (e.g., g++).<br>
•Hardware: GPU supporting OpenGL 3.3+ for shaders.<br>


## Controls

•Camera:<br>
'C': Toggle between Auto-Orbit and Free-FPS modes.<br>
Arrow Keys (Auto): Rotate/zoom orbit.<br>
WASD (Free): Move forward/back/left/right.<br>
Mouse (Free): Look around (capture with 'B').<br>

•Lights:<br>
'L': Toggle desk lamp.<br>
'G': Toggle ceiling light.<br>
'+/-': Adjust global light intensity.<br>

•Interactions:<br>
'U': Open/close door (with sound).<br>
'F': Open/close window.<br>
'N': Toggle PC screen.<br>
'P': Pause animations.<br>

•Other:<br>
'H': Show/hide help menu.<br>
F11: Toggle fullscreen.<br>
'B': Toggle mouse capture in Free mode.<br>


## Project Structure

Source: Single main.cpp file (modularized with functions for drawing, input, etc.).<br>
Resources:<br>
Ressources/textures/: JPG/PNG textures (floor, walls, objects).<br>
Ressources/objects/: OBJ models (bed.obj, lamp.obj, etc.).<br>
Ressources/shaders/: Vertex (shader.vert) and fragment (shader.frag) shaders.<br>
Ressources/sounds/: WAV files for door open/close.<br>
Ressources/images/: Posters and additional images.<br>


## Known Issues

Sound playback is Windows-specific; adapt for cross-platform.<br>
Some models/textures may fail to load if paths are incorrect—check console errors.<br>
FPS display is calibrated over time; initial values may fluctuate.<br>
No collision detection beyond basic clamping.<br>

## Credits

Author: Rouibah Hanine (2025).<br>
Libraries: STB_Image (Sean Barrett), TinyOBJLoader, GLM, GLEW, FreeGLUT.<br>
Inspired by computer graphics tutorials on lighting and scene rendering.<br>

## License
This project is licensed under the MIT License - see the LICENSE file for details.
