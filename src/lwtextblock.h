#pragma once

typedef enum _LW_TEXT_BLOCK_ALIGN {
	LTBA_LEFT_TOP,		LTBA_CENTER_TOP,		LTBA_RIGHT_TOP,
	LTBA_LEFT_CENTER,	LTBA_CENTER_CENTER,		LTBA_RIGHT_CENTER,
	LTBA_LEFT_BOTTOM,	LTBA_CENTER_BOTTOM,		LTBA_RIGHT_BOTTOM,
} LW_TEXT_BLOCK_ALIGN;

typedef struct _LWTEXTBLOCK
{
	const char* text;
	int text_bytelen;
	int begin_index;
	int end_index;
	float text_block_x;
	float text_block_y;
	float text_block_width;
	float text_block_line_height;
	float size;
	float color_normal_glyph[4];
	float color_normal_outline[4];
	float color_emp_glyph[4];
	float color_emp_outline[4];
	LW_TEXT_BLOCK_ALIGN align;
} LWTEXTBLOCK;


#define DEFAULT_TEXT_BLOCK_WIDTH (2.0f)