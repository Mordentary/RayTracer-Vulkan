#pragma once

struct CameraConstant
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
};


struct SceneConstant
{
    CameraConstant cameraCB;
   
    uint instanceDataAddress;
    uint sceneConstantBufferSRV;
    uint sceneStaticBufferSRV;

};

#ifndef __cplusplus
ConstantBuffer<SceneConstant> SceneCB : register(b2);

CameraConstant GetCameraCB()
{
    return SceneCB.cameraCB;
}
#endif