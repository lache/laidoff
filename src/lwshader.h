#pragma once

#include "lwgl.h"

typedef struct _LWSHADER {
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
	GLint shade_location;
	GLint     a_pID;
	GLint     a_pRadiusOffset;
	GLint     a_pVelocityOffset;
	GLint     a_pDecayOffset;
	GLint     a_pSizeOffset;
	GLint     a_pColorOffset;

	// Uniforms
	GLint mvp_location;
	GLint vuvoffset_location;
	GLint vuvscale_location;
	GLint vs9offset_location;
	GLint m_location;
	GLint alpha_multiplier_location;
	GLint overlay_color_location;
	GLint overlay_color_ratio_location;
	GLint diffuse_location;
    GLint diffuse_arrow_location;
	GLint alpha_only_location;
	GLint glyph_color_location;
	GLint outline_color_location;
	GLint bone_location;
	GLint rscale_location;
	GLint thetascale_location;
	GLint projection_matrix_location;
	GLint k_location;
	GLint color_location;
	GLint time_location;
	GLint texture_location;
	GLint u_ProjectionViewMatrix;
	GLint u_ModelMatrix;
	GLint u_Gravity;
	GLint u_Time;
	GLint u_eRadius;
	GLint u_eVelocity;
	GLint u_eDecay;
	GLint u_eSizeStart;
	GLint u_eSizeEnd;
	GLint u_eScreenWidth;
	GLint u_eColorStart;
	GLint u_eColorEnd;
	GLint u_Texture;
	GLint u_TextureAlpha;
	GLint time;
	GLint resolution;
	GLint sphere_pos;
	GLint sphere_col;
	GLint sphere_col_ratio;
	GLint sphere_speed;
	GLint sphere_move_rad;
    GLint arrow_center;
    GLint arrow_angle;
    GLint arrow_scale;
    GLint arrowRotMat2;
} LWSHADER;
