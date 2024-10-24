#pragma once

#ifdef SINGULARITY_ENGINE_EXPORTS
#define SINGULARITY_API __declspec(dllexport)
#else
#define SINGULARITY_API __declspec(dllimport)
#endif