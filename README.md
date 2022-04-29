# SHOE

## What is SHOE?

SHOE, the Sorta Helpful Open Engine (and many other acronyms), is a C++ app designed to provide convenient access to efficient graphics features needed to render a 3D environment. In its current version of V0.5, SHOE supports:

- 3D Object Rendering
- Basic lighting, including point, spot, and directional lights
- Skybox rendering
- Dynamic terrain and terrain materials
- Blend maps
- Multiple render targets
- Shadow mapping
- PBR shading
- Image-Based Lighting (for lighting that emulates the sky)
- Rendering with multiple targets
- Screen-space ambient occlusion
- GPU-based particles through compute shaders
- Sound through [FMOD Core](https://www.fmod.com/)
- Efficient live UI through IMGUI
- Asset component system designed for quick addition and removal of complex types
- Colliders and triggerboxes
- Internal messaging system for assets to communicate efficiently
- Easy creation and dynamic editing of GameEntities, lights, models, materials, colliders, terrain, particle systems, and cameras through code or the UI
- Transparency and refraction
- Complex model loading through assimp
- Multithreaded loading screens with asset load tracking displayed
- Custom input management
- Scene saving and loading of all available data
- Code that focuses on human readability and accessibility through lots of comments, defines, and avoidance of difficult types like wstring
- Aggressive GPU threading designed to provide the quickest GPU calls that DX11 can do
- Easy access to asset management and the component system both through code and the UI
- Ability to import custom assets of nearly any type (only partially implemented - importing assets from outside the Assets/ dir may result in undefined behavior until a future patch.)
- Error tracking and handling (mostly unimplemented - only occurs during loading screens)

SHOE will continue to gain features over time, and I hope to turn it into a functional engine that I can build games on top of.

## Can I contribute/build on top of SHOE?

Yes! The goal is, eventually, for this to become a collaborative project that can be used to build games or other software. This project will always be FOSS.

Currently, this program only runs on Windows, but before reaching V1.0 multiplatform functionality will be added.

To help develop SHOE, simply clone the repo and launch the SLN file in the SHOE directory. You will also need the Assets folder, which holds test assets for development, from skyboxes to models. This folder can be downloaded [here.](https://github.com/crigney3/SHOE)