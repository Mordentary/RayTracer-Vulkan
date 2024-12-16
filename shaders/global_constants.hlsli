#pragma once

struct SceneConstant
{
    uint vertexDataIndex;
};

#ifndef __cplusplus
ConstantBuffer<SceneConstant> SceneCB : register(b2, space0);

#endif