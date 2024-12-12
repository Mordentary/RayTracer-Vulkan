// A simple HLSL shader for testing:
// This example consists of a basic vertex shader and pixel shader.
// The vertex shader just passes through the position and color.
// The pixel shader returns the input color, producing a solid color.

struct VSInput
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	// Transform the vertex position. For a simple test,
	// just convert it to a 4D vector with w=1
	output.position = float4(input.position, 1.0f);
	// Pass the color to the pixel shader
	output.color = input.color;
	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	// Simply return the color passed in by the vertex shader
	return input.color;
}