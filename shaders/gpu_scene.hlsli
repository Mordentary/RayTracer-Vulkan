#pragma once
#include "global_constants.hlsli"

struct InstanceData
{
    uint posBufferAddress;
    
};



struct Vertex
{
    float3 position;
};

#ifndef __cplusplus


template<typename T>
T LoadSceneStaticBuffer(uint bufferAddress, uint element_id)
{
    ByteAddressBuffer sceneBuffer = ResourceDescriptorHeap[SceneCB.sceneStaticBufferSRV];
    return sceneBuffer.Load < T > (bufferAddress + sizeof(T) * element_id);
}

template<typename T>
T LoadSceneConstantBuffer(uint bufferAddress)
{
    ByteAddressBuffer constantBuffer = ResourceDescriptorHeap[SceneCB.sceneConstantBufferSRV];
    return constantBuffer.Load < T > (bufferAddress);
}

InstanceData GetInstanceData(uint instance_id)
{
    return LoadSceneConstantBuffer <InstanceData> (SceneCB.instanceDataAddress + sizeof(InstanceData) * instance_id);
}


Vertex getVertex(uint index)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[SceneCB.vertexDataIndex];
    // Calculate the byte offset to the desired vertex
    uint byteOffset = index * sizeof(Vertex);
    // Load the vertex data from the buffer
    Vertex v;
    v = vertexBuffer.Load < Vertex > (byteOffset);
    
    return v;
}



#endif //__cplusplus