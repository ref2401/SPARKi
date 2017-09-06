# SPARKi

The main goal of SPARKi is education and experiments. Currently the demo represents a simple scene with one object & environment light source. The demo allows to edit object’s material params in runtime.

```.geo``` & ```.tex``` are SPARKi internal geometry & texture formats respectively.

## Dependencies & Build
1) Visul Studio 2017 (Community Edition will do)
2) [FBX SDK](https://www.autodesk.com/products/fbx/overview). After the installation is complete add FBX_SDK_DIR environmanet variable. It must point to the path where your fbx sdk has been installed. For example: ```c:\Program Files\Autodesk\FBX\FBX SDK\2018.1.1\```
3) Clone the repository
4) Get all the submodules (```git submodule init```, ```git submodule update```)
5) Open the solution ```/msvc/SPARKi.sln```
6) Go to SPARKi project properties > Debugging and set Working Directory to ```$(ProjectDir)..\bin\$(Configuration)\```

# Bibliography
- PBR
	- [Physically-Based Shading at Disney](https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf)
	- [Real Shading in Unreal Engine 4](http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
	- [Moving Frostbite to Physically Based Rendering 3.0](https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf)
	- [Implementation Notes: Runtime Environment Map Filtering for Image Based Lighting](https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/)
- FXAA
	- [FXAA paper by Timothy Lottes](https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf)
	- [NVIDIAGameWorks - D3DSamples - fxaa.hlsl](https://github.com/NVIDIAGameWorks/D3DSamples/blob/master/samples/FXAA/media/FXAA.hlsl)
	- [20161027 - FXAA Pixel Width Contrast Reduction](https://timothylottes.github.io/20161027.html)
	- [Implementing FXAA](http://blog.simonrodriguez.fr/articles/30-07-2016_implementing_fxaa.html)
