#include "laidoff.h"
#include "sound.h"

static void handle_move_key_press_release(LWCONTEXT* pLwc, int key, int action) {
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		lw_press_key_right(pLwc);
	}

	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		lw_press_key_left(pLwc);
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		lw_press_key_up(pLwc);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		lw_press_key_down(pLwc);
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
		lw_release_key_right(pLwc);
	}

	if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
		lw_release_key_left(pLwc);
	}

	if (key == GLFW_KEY_UP && action == GLFW_RELEASE) {
		lw_release_key_up(pLwc);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE) {
		lw_release_key_down(pLwc);
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	LWCONTEXT* pLwc = (LWCONTEXT*)glfwGetWindowUserPointer(window);

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		lw_trigger_touch(pLwc, 0, 0);
	}

	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		lw_trigger_reset(pLwc);
	}

	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		lw_trigger_play_sound(pLwc);
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		lw_trigger_key_right(pLwc);
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		lw_trigger_key_left(pLwc);
	}

	handle_move_key_press_release(pLwc, key, action);

	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
		lw_trigger_key_enter(pLwc);
	}

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
		change_to_field(pLwc);
	}

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
		change_to_dialog(pLwc);
	}

	if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
		change_to_battle(pLwc);
	}

	if (key == GLFW_KEY_F4 && action == GLFW_PRESS) {
		change_to_font_test(pLwc);
	}

	if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
		change_to_admin(pLwc);
	}

	if (key == GLFW_KEY_F6 && action == GLFW_PRESS) {
		change_to_battle_result(pLwc);
	}

	if (key == GLFW_KEY_F7 && action == GLFW_PRESS) {
		change_to_skin(pLwc);
	}

	if (key == GLFW_KEY_F8 && action == GLFW_PRESS) {
		change_to_physics(pLwc);
	}

	if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
		toggle_font_texture_test_mode(pLwc);
	}

	if (key == GLFW_KEY_KP_1 && action == GLFW_PRESS) {
		play_sound(LWS_DIE);
		lw_set_kp(pLwc, 1);
	}

	if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS) {
		play_sound(LWS_HIT);
		lw_set_kp(pLwc, 2);
	}

	if (key == GLFW_KEY_KP_3 && action == GLFW_PRESS) {
		play_sound(LWS_POINT);
		lw_set_kp(pLwc, 3);
	}

	if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS) {
		play_sound(LWS_SWOOSHING);
		lw_set_kp(pLwc, 4);
	}

	if (key == GLFW_KEY_KP_5 && action == GLFW_PRESS) {
		play_sound(LWS_WING);
		lw_set_kp(pLwc, 5);
	}
}
