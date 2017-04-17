#pragma once 

typedef struct
{
	float x, y, z;
	float r, g, b;
	float u, v;
	float s9x, s9y;
} LWVERTEX;

static const LWVERTEX full_square_vertices[] =
{
	{ -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
	{ +1.f, -1.f, 0, 0, 0, 0, 1, 1, 0, 0 },
	{ +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ -1.f, +1.f, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
};

#define VERTEX_COUNT_PER_ARRAY (6)
