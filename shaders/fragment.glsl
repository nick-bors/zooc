// Opengl version >= 1.3
#version 130 

out mediump vec4 color;         // resultant color of pixel

in mediump  vec2 texcoord;      // texture coordinates after zooming/panning

uniform sampler2D tex;          // texture (screenshot)
uniform vec2      cursorPos;    // cursor position in X11 coordinate space
uniform vec2      windowSize;   // X11 window size

uniform float flShadow;         // Percentage to dim around flashlight
uniform float flRadius;         // Radius of flashlight
uniform float cameraScale;      // Camera scaling

void main()
{
    // Opengl counts y differently, so we have to take the position from the 
    // bottom of the screen (windowSize.y - cursorPos.y).
    vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);

    color = mix(
        texture(tex, texcoord), vec4(0.0, 0.0, 0.0, 0.0), 
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow
    );
}
