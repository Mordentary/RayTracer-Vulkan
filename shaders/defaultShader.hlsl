#include "global_constants.hlsli"
#include "gpu_scene.hlsli"

struct PSInput
{
	float4 position : SV_Position;
	float3 color : COLOR;
};

// Vertex Shader
PSInput VSMain(uint vertex_id : SV_VertexID)
{
	PSInput output;

	// Get the vertex data using the provided vertex ID
	Vertex v = getVertex(vertex_id);

	// Apply a simple transformation (identity for now)
	output.position = float4(v.position, 1.0f);

	// Assign a color based on position (for visualization)
	output.color = v.position * 0.5f + 0.5f; // Normalize to [0, 1]

	return output;
}

// Pixel Shader
float4 PSMain(PSInput input) : SV_Target
{
	return float4(input.color, 1.0f);
}