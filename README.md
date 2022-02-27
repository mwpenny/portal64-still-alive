# N64 Realtime Shadows

This showcases a tenique to render realtime shadows on native N64 hardware.

It works as a variation of shadow volumes and works by rendering 3 passes on any object in shadow and 2 passes for the shadow volume.

* First it renders the subjects lit.
* Next it renders the back faces of the shadow volume fully trasparent but still updates the depth buffer
* Next it redraws all the subjects unlit using decal mode
* Next it draws the front faces of the shadow volume also fully transparent
* Last it redraws the subjects lit again

![Shadows](./images/Example.png)

The shadows cast by Suzanne just renders her to an offscreen 64x64 8 bit buffer then projects it onto the ground.

## N64 Toon Shading

![Toon Shading](./images/Toon.png)

The palletecb demo shows a few uses of rending to an 8 bit color buffer. It then uses a color pallete to translate it to the final image. The most useful technique showcased here is toon shading. To achieve this effect I first render the 3D objects to a 8 bit color buffer. The N64 does this by only rendering the full 8 bits of the red channel. The result is a grayscale image that cannot be displayed on its own. It first needs to be converted to a 16 bit image. This is done by assigning each grayscale value to a unqiue color using a color pallete. To get the toon effect, the dark and lit value for each color are put right next to each other with the dark color coming first. When an object is rendered, it uses this blending mode `(SHADE - 0) * ENVIRONMENT + PRIMITIVE` where `PRIMITIVE` is assigned the index of the dark color and `ENVIRONMENT` is given the value of `1`. The hardware interprets the value of `1` as `1 / 255` so the end result is the part of the model in shadow gets the dark index and the part that is lit is given one more than the dark index which is the light color. Up to 128 color pairs can be stored in color pallete that is 256 entries long.