#pragma once
#include "pch.h"

std::vector<float> quad_vertices =
{
    -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f
};

std::vector<unsigned int> quad_indices =
{
    0, 1, 2,
    0, 2, 3
};