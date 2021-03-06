# SPARKi
[![demo video](https://user-images.githubusercontent.com/10673999/30955755-69b6c7f2-a43d-11e7-86bd-150a9512f3ee.png)](https://youtu.be/q0ZhWul9hFs)

The main goals of the project are self-education and experiments. The demo shows a simple scene (an object and an environment light source) and material ui that allows to edit object’s material parameters in runtime. The challenge with complex objects is that they might be composed of several unique materials. So the requirement is to be able to generate a single texture that will contain parameters for each material. 

One possible solution is to provide a mask texture with ```N``` unique rgb-colors (each color represents a single material). The demo takes such a mask texture, retrieves all the unique colors from it and generates the material property texture according to the color mask. Then the renderer gets the generated texture and uses it to draw the object. It is possible to alter material properties for any unique color in runtime. 

The textures used in video are below.


[![base_color](https://user-images.githubusercontent.com/10673999/30958084-bcc119f0-a444-11e7-9104-42ee6a52cca3.png) ](https://raw.githubusercontent.com/ref2401/SPARKi/master/data/material_base_color.png)|
[![reflect_color](https://user-images.githubusercontent.com/10673999/30958099-cbf2d10c-a444-11e7-887f-c597ee021eb3.png) ](https://raw.githubusercontent.com/ref2401/SPARKi/master/data/material_reflect_color.png)|
[![normal_map](https://user-images.githubusercontent.com/10673999/30958088-bdf336b4-a444-11e7-9ec1-8fdf109542ff.png) ](https://raw.githubusercontent.com/ref2401/SPARKi/master/data/material_normal_map.png)|
[![property_mask](https://user-images.githubusercontent.com/10673999/30958134-e28dca66-a444-11e7-84e6-686e96a7ea7b.png) ](https://raw.githubusercontent.com/ref2401/SPARKi/master/data/material_property_mask.png)|
| --- | --- | --- | --- |

```.geo``` & ```.tex``` are SPARKi internal geometry & texture formats respectively.

## Dependencies & Build
1) DirectX SDK
2) Visul Studio 2017 (Community Edition will do)
3) [FBX SDK](https://www.autodesk.com/products/fbx/overview). After the installation is complete add FBX_SDK_DIR environmanet variable. It must point to the path where your fbx sdk has been installed. For example: ```c:\Program Files\Autodesk\FBX\FBX SDK\2018.1.1\```
4) Clone the repository
5) Get all the submodules (```git submodule init```, ```git submodule update```)
6) Open the solution ```/msvc/SPARKi.sln```
7) Go to SPARKi project properties > Debugging and set Working Directory to ```$(ProjectDir)..\bin\$(Configuration)\```

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
