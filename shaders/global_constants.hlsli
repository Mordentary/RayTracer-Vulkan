#pragma once

struct SceneConstant
{
    uint instanceDataAddress;
    uint sceneConstantBufferSRV;
    uint sceneStaticBufferSRV;

};

#ifndef __cplusplus
ConstantBuffer<SceneConstant> SceneCB : register(b2);

#endif