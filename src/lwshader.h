#pragma once

#include "lwgl.h"

typedef struct _LWSHADER
{
	int valid;

	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;

	// Vertex attributes
	GLint vpos_location;
	GLint vcol_location;
	GLint vuv_location;
	GLint vs9_location;
	GLint vbweight_location;
	GLint vbmat_location;
	GLint theta_location;

	// Uniforms
	GLint mvp_location;
	GLint vuvoffset_location;
	GLint vuvscale_location;
	GLint vs9offset_location;
	GLint alpha_multiplier_location;
	GLint overlay_color_location;
	GLint overlay_color_ratio_location;
	GLint diffuse_location;
	GLint alpha_only_location;
	GLint glyph_color_location;
	GLint outline_color_location;
	GLint bone_location;
	GLint rscale_location;
	GLint thetascale_location;
	GLint projection_matrix_location;
	GLint k_location;

} LWSHADER;
