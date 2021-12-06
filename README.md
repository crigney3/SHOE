# SHOE

## What is SHOE?

SHOE, the Sorta Helpful Open Engine (and many other acronyms), is a C++ app designed to provide convenient access to efficient graphics features needed to render a 3D environment. In its current version of V0.2, SHOE supports:

- 3D Object Rendering
- Basic lighting, including point, spot, and directional lights
- Skybox rendering
- Terrain
- Blend maps
- Easy creation of new GameObjects, lights, models, materials, and cameras
- Easy editing of GameObjects, lights, models, materials, and cameras
- Multiple render targets
- Shadow mapping
- PBR shading
- Image-Based Lighting (for lighting that emulates the sky)
- Rendering with multiple targets
- Screen-space ambient occlusion
- Particles
- Sound through [FMOD Core](https://www.fmod.com/)

SHOE will continue to gain features over time, and I hope to turn it into a functional engine that I can build games on top of.

## Can I contribute/build on top of SHOE?

Yes! The goal is, eventually, for this to become a collaborative project that can be used to build games or other software. This project will always be FOSS.

Currently, this program only runs on Windows, but before reaching V1.0 multiplatform functionality will be added.

To help develop SHOE, simply clone the repo and launch the SLN file in the SHOE directory. You will also need the Assets folder, which holds test assets for development, from skyboxes to models. This folder can be downloaded [here.](https://github.com/crigney3/SHOE)