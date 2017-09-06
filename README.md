# SPARKi

The main goal of SPARKi is education and experiments. Currently the demo represents a simple scene with one object & environment light source. The demo allows to edit object’s material params in runtime.

```.geo``` & ```.tex``` are SPARKi internal geometry & texture formats respectively.

## Printscreens
 [ ![r01m0](https://user-images.githubusercontent.com/10673999/30104334-00c54f0c-92fe-11e7-835e-f111043df981.png) ](https://user-images.githubusercontent.com/10673999/30104323-f953a2dc-92fd-11e7-9cd4-3b5c36193feb.png) | [ ![r041m0](https://user-images.githubusercontent.com/10673999/30104337-0502d03a-92fe-11e7-8b64-44559049f660.png) ](https://user-images.githubusercontent.com/10673999/30104340-06664b6e-92fe-11e7-96bf-d8da0be54ca2.png) | [ ![r1m0](https://user-images.githubusercontent.com/10673999/30104346-0853e9cc-92fe-11e7-8aa0-d24bcb5c4c55.png) ](https://user-images.githubusercontent.com/10673999/30104350-094038a4-92fe-11e7-9a69-8103310e75d4.png) |  [ ![gold_like](https://user-images.githubusercontent.com/10673999/30104352-0b58c07a-92fe-11e7-9573-6e2bc20baadc.png) ](https://user-images.githubusercontent.com/10673999/30104358-0d524522-92fe-11e7-957c-c31b4c7a123f.png) |
| --- | --- | --- | --- |

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
