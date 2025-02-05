#include "global_constants.hlsli"
#include "gpu_scene.hlsli"

struct PSInput
{
	float4 position : SV_Position;
	float3 color : COLOR;
};

// Vertex Shader
PSInput VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
	PSInput output;

	CameraConstant cb = GetCameraCB();
	Vertex v = GetVertex(instanceId, vertexId);

	float4 pos = mul(float4(v.position, 1.0f), cb.viewProjection);
	output.position = pos;

	output.color = v.position * 0.5f + 0.5f;

	return output;
}

// Pixel Shader
float4 PSMain(PSInput input) : SV_Target
{
	return float4(input.color, 1.0f);
}