#pragma once
#include "global_constants.hlsli"

struct Vertex
{
 float3 position;
};

#ifndef __cplusplus

Vertex getVertex(uint index)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[2];

    // Calculate the byte offset to the desired vertex
    uint byteOffset = index * sizeof(Vertex);

    // Load the vertex data from the buffer
    Vertex v;
    v = vertexBuffer.Load<Vertex>(byteOffset);

    return v;
}



#endif //__cplusplus