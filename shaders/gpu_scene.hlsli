#pragma once
#include "global_constants.hlsli"

struct InstanceData
{
    uint posBufferAddress;
    uint indexBufferAddress;
};



struct Vertex
{
    float3 position;
};

#ifndef __cplusplus


template<
typename T>
T LoadSceneStaticBuffer(uint bufferAddress, uint element_id)
{
    ByteAddressBuffer sceneBuffer = ResourceDescriptorHeap[SceneCB.sceneStaticBufferSRV];
    return sceneBuffer.Load < T > (bufferAddress + sizeof(T) * element_id);



}

template<
typename T>
T LoadSceneConstantBuffer(uint bufferAddress)
{
    ByteAddressBuffer constantBuffer = ResourceDescriptorHeap[SceneCB.sceneConstantBufferSRV];
    return constantBuffer.Load < T > (bufferAddress);
}

InstanceData GetInstanceData(uint instance_id)
{
    return LoadSceneConstantBuffer < InstanceData > (SceneCB.instanceDataAddress + sizeof(InstanceData) * instance_id);
}


//Vertex GetVertex(uint instance_id, uint vertex_id)
//{
//    InstanceData instanceData = GetInstanceData(instance_id);

//    Vertex v;
//    v.position = LoadSceneStaticBuffer <

//    float3 > (instanceData.posBufferAddress, vertex_id);

//    return v;
//}

Vertex GetVertex(uint instance_id, uint vertex_id)
{
    InstanceData instanceData = GetInstanceData(instance_id);

    Vertex v;
    v.position = LoadSceneStaticBuffer <

    float3 > (instanceData.posBufferAddress, LoadSceneStaticBuffer <
    uint > (instanceData.indexBufferAddress, vertex_id));

    return v;
}


#endif //__cplusplus