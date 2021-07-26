// A minimal pixel shader that inverts the colors

// The terminal graphics as a texture
Texture2D shaderTexture;
SamplerState samplerState;

// Terminal settings such as the resolution of the texture
cbuffer PixelShaderSettings {
  // The number of seconds since the pixel shader was enabled
  float  Time;
  // UI Scale
  float  Scale;
  // Resolution of the shaderTexture
  float2 Resolution;
  // Background color as rgba
  float4 Background;

  float2 GlyphSize;
  float2 CursorPosition;
  float2 BufferSize;
};

// A pixel shader is a program that given a texture coordinate (tex) produces a color.
// tex is an x,y tuple that ranges from 0,0 (top left) to 1,1 (bottom right).
// Just ignore the pos parameter.
float4 main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    // Read the color value at the current texture coordinate (tex)
    //  float4 is tuple of 4 floats, rgba
    float4 color = shaderTexture.Sample(samplerState, tex);

    // Inverts the rgb values (xyz) but don't touch the alpha (w)
    color.xyz = 1.0 - color.xyz;

    float2 cursorTopLeft = CursorPosition * GlyphSize;
    float2 cursorBottomRight = (CursorPosition + float2(1, 1)) * GlyphSize;
    float2 relativeTopLeft = cursorTopLeft / Resolution;
    float2 relativeBottomRight = cursorBottomRight / Resolution;

    float2 relativeCursorPos = CursorPosition / BufferSize;
    if ((tex.x >= relativeTopLeft.x && tex.x <= relativeBottomRight.x) &&
        (tex.y >= relativeTopLeft.y && tex.y <= relativeBottomRight.y)) {
        color.xy = relativeCursorPos;
        color.z = 0.0;
    }
    

    // Return the final color
    return color;
}