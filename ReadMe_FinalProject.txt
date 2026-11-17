EN.605.767.8VL.SP26
Bryan Garry
Final Project

========================================

Files Added / Modified:

- SampleProject/main.cpp
- SampleProject/planet_shader_node.hpp/.cpp
- SampleProject/water_shader_node.hpp/.cpp
- SampleProject/cloud_shader_node.hpp/.cpp
- SampleProject/atmosphere_shader_node.hpp/.cpp
- SampleProject/planet.vert/frag
- SampleProject/water.vert/.frag
- SampleProject/cloud.vert/.frag
- SampleProject/atmosphere.vert/.frag
- SampleProject/spacecraft.hpp/.cpp

- scene/icosphere.hpp/.cpp
- scene/water_node.hpp/.cpp

- scene/transform_node.hpp/.cpp
- scene/camera_node.hpp/.cpp


Files Ported Over From Last Semester

- SampleProject/skybox.hpp/cpp
- SampleProject/skybox_shader_node.hpp/.cpp
- SampleProject/skybox_shader.vert/.frag


========================================

The inspiration for this project came from many sources, from YouTube videos about planet
creation to some of my favorite video games such as Outer Wilds and Kerbal Space Program.
The idea was to create a simplistic planet with varying additional layers of detail to help
create a semi-realistic depiction of a celestial body. Those layers of detail start with
randomly generated terrain done in the vertex shader of the planet, to layers of water, clouds,
and atmosphere to complete the creation.

The planet terrain was the first part of the project and it is built based on the reference
provided in my presentation slides. It is a procedural terrain implementation built on a
subdivided icosphere where the terrain displacement happens entirely in the vertex shader.
Doing the terrain this way resulted in some great looking planets but provided challenging for 
me when I went to implement terrain collision. I had attempted to sample the terrain data from
the shader in order to get the values in world space of the planet terrain for collision
calculations, but something about my approach was not fully correct and I had to resort to a
basic spherical collision for the final submission.

The ocean, clouds, and atmosphere where then built one after another using various different
methods to get the desired effect. The ocean layer uses a basic icosphere for the water level
but then the vertex normals are perturbed so that specular sparkles reflect off of the water
surface. Ideally there is also some fresnel reflectance and depth based coloring, although
the effort here resulted in minimal visual returns in my opinion. The clouds where the most
similar to the terrain in that I was able to sample fBm noise at a certain radius around the 
center of the planet and then offset that over time by a speed and drift direction to help
give the illusion of wind. I think this effect looks the best out of the different environmental
effects included here. The atmosphere was the most challenging aspect as it was an attempt to
take what we learned during our ray-tracing assignment and take a ray-marching approach to the
atmosphere in a fragment shader.I have also cited a paper I used for reference in my slide
presentation, but this part was most difficult because even with the correct ray tracking, the
tweaking of the various coefficients that go into creating the desired effect had me second
guessing some shader logic here and there. 

A moon was added using the same planet shader with terrain deformation, the coloring of the
terrain was omitted so that a natural grey tone would dominate. This moon orbits the central
planet and each body has a gravitational sphere of influence that effects a spacecraft that
can be toggled on in the controls. The spacecraft was two sphere combined together as I had
trouble finding a free model online that I was able to load into the scene. There are many
controls listed out for the spacecraft, which can also be toggled to go from third person
to first person. The spacecraft will also collide with the surface of the planet, although not
perfectly for reasons I have already mentioned above. The spacecraft can get carried along by
the orbiting moon if positioned correctly as well!

The skybox was carried over from last semester and was added to improve the feel of the scene
since it exists in a space setting.

Overall, I am happy with the result I got and if more time allowed I would have liked to generate a multitude of different planets of different colors and properties to see what the shaders I have built are truly capable of. I would have also liked to possibly add some small
asteroids or even fireworks from the spacecraft to help demonstrate the collision or gravity
in the scene as well.

For full transparency, the use of GenAI was included in this project. I have used it
to assist with the formatting of code and comments, as well as help with some tedious
hard-coded values such as the triangle winding in the Icosphere or for testing various
tweak-able coefficients in the atmosphere to get the best visual results without spending
hours manually testing different combinations.

(Using MacOS)
