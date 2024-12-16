#pragma once
#include "global_constants.hlsli"

struct Vertex
{
 float3 position;
};

#ifndef __cplusplus

Vertex getVertex(uint index)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[SceneCB.vertexDataIndex];

    // Calculate the byte offset to the desired vertex
    uint byteOffset = index * sizeof(Vertex);

    // Load the vertex data from the buffer
    Vertex v;
    v.position.x = vertexBuffer.Load(byteOffset);
    v.position.y = vertexBuffer.Load(byteOffset + 4);
    v.position.z = vertexBuffer.Load(byteOffset + 8);

    return v;
}



#endif //__cplusplus