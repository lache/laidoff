#include "lwtcp.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "battle.h"
#include "laidoff.h"
#include "render_admin.h"
#include "battle_result.h"
#include "field.h"
#include "mq.h"
#include "logic.h"
#include "lwbutton.h"
#include "puckgameupdate.h"
#include "puckgame.h"
#include "lwdirpad.h"
#include "lwtcpclient.h"

static void convert_touch_coord_to_ui_coord(LWCONTEXT* pLwc, float *x, float *y) {
    if (pLwc->height < pLwc->width) {
        *x *= (float)pLwc->width / pLwc->height;
    } else {
        *y *= (float)pLwc->height / pLwc->width;
    }
}

void lw_trigger_mouse_press(LWCONTEXT* pLwc, float x, float y, int pointer_id) {
    if (!pLwc) {
        return;
    }

    convert_touch_coord_to_ui_coord(pLwc, &x, &y);

    LOGI("mouse press ui coord x=%f, y=%f", x, y);

    if (field_network(pLwc->field)) {
        mq_publish_now(pLwc, pLwc->mq, 0);
    }

    pLwc->last_mouse_press_x = x;
    pLwc->last_mouse_press_y = y;

    int pressed_idx = lwbutton_press(&pLwc->button_list, x, y);
    if (pressed_idx >= 0) {
        const char* id = lwbutton_id(&pLwc->button_list, pressed_idx);
        logic_emit_ui_event_async(pLwc, id);
        // Should return here to prevent calling overlapped UI element behind buttons.
        return;
    }

    if (pLwc->game_scene == LGS_FIELD || pLwc->game_scene == LGS_PHYSICS) {
        const float sr = get_dir_pad_size_radius();

        float left_dir_pad_center_x = 0;
        float left_dir_pad_center_y = 0;
        get_left_dir_pad_original_center(pLwc->aspect_ratio, &left_dir_pad_center_x, &left_dir_pad_center_y);
        dir_pad_press(&pLwc->left_dir_pad, x, y, pointer_id, left_dir_pad_center_x, left_dir_pad_center_y, sr);

        if (pLwc->control_flags & LCF_PUCK_GAME_RIGHT_DIR_PAD) {
            float right_dir_pad_center_x = 0;
            float right_dir_pad_center_y = 0;
            get_right_dir_pad_original_center(pLwc->aspect_ratio, &right_dir_pad_center_x, &right_dir_pad_center_y);
            dir_pad_press(&pLwc->right_dir_pad, x, y, pointer_id, right_dir_pad_center_x, right_dir_pad_center_y, sr);
        }
    }

    const float fist_button_w = 0.75f;
    const float fist_button_h = 0.75f;
    const float fist_button_x_center = pLwc->aspect_ratio - 0.3f - fist_button_w / 2;
    const float fist_button_y_center = -1 + fist_button_h / 2;

    const float top_button_w = fist_button_w;
    const float top_button_h = fist_button_h;
    const float top_button_x_center = fist_button_x_center;
    const float top_button_y_center = -fist_button_y_center;


    if (pLwc->game_scene == LGS_PHYSICS
        && fabs(fist_button_x_center - x) < fist_button_w
        && fabs(fist_button_y_center - y) < fist_button_h
        && (!pLwc->left_dir_pad.dragging || (pLwc->left_dir_pad.pointer_id != pointer_id))) {
        // this event handled by lua script
        //puck_game_dash(pLwc, pLwc->puck_game);
    }

    if (pLwc->game_scene == LGS_FIELD
        && fabs(fist_button_x_center - x) < fist_button_w
        && fabs(fist_button_y_center - y) < fist_button_h) {
        //field_attack(pLwc);

        //pLwc->hide_field = !pLwc->hide_field;

        //LOGI("atk_pad_dragging ON");

        // Player combat mode...
        //pLwc->atk_pad_dragging = 1;
    }

    if (pLwc->game_scene == LGS_FIELD
        && fabs(top_button_x_center - x) < top_button_w
        && fabs(top_button_y_center - y) < top_button_h) {
        //pLwc->fps_mode = !pLwc->fps_mode;
    }

    if (pLwc->game_scene == LGS_PHYSICS
        && fabs(top_button_x_center - x) < top_button_w
        && fabs(top_button_y_center - y) < top_button_h) {
        // controlled by LWBUTTONLIST and lua script
        //puck_game_pull_puck_start(pLwc, pLwc->puck_game);
    }

    if (pLwc->game_scene == LGS_BATTLE && pLwc->battle_state != LBS_COMMAND_IN_PROGRESS && pLwc->player_turn_creature_index >= 0) {
        const float command_palette_pos = -0.5f;

        if (y > command_palette_pos) {
            exec_attack_p2e_with_screen_point(pLwc, x, y);
        } else {
            // command palette area
            int command_slot = (int)((x + pLwc->aspect_ratio) / (2.0f / 10 * pLwc->aspect_ratio)) - 2;
            if (command_slot >= 0 && command_slot < 6) {
                const LWSKILL* skill = pLwc->player[pLwc->player_turn_creature_index].skill[command_slot];
                if (skill && skill->valid) {
                    pLwc->selected_command_slot = command_slot;
                }
            }

            //printf("mouse press command slot %d\n", command_slot);
        }
    }
}

void lw_trigger_mouse_move(LWCONTEXT* pLwc, float x, float y, int pointer_id) {
    if (!pLwc) {
        return;
    }

    convert_touch_coord_to_ui_coord(pLwc, &x, &y);

    pLwc->last_mouse_move_delta_x = x - pLwc->last_mouse_move_x;
    pLwc->last_mouse_move_delta_y = y - pLwc->last_mouse_move_y;

    pLwc->last_mouse_move_x = x;
    pLwc->last_mouse_move_y = y;

    if (pLwc->game_scene == LGS_FIELD || pLwc->game_scene == LGS_PHYSICS) {
        const float sr = get_dir_pad_size_radius();

        float left_dir_pad_center_x = 0;
        float left_dir_pad_center_y = 0;
        get_left_dir_pad_original_center(pLwc->aspect_ratio, &left_dir_pad_center_x, &left_dir_pad_center_y);
        dir_pad_move(&pLwc->left_dir_pad, x, y, pointer_id, left_dir_pad_center_x, left_dir_pad_center_y, sr);

        if (pLwc->control_flags & LCF_PUCK_GAME_RIGHT_DIR_PAD) {
            float right_dir_pad_center_x = 0;
            float right_dir_pad_center_y = 0;
            get_right_dir_pad_original_center(pLwc->aspect_ratio, &right_dir_pad_center_x, &right_dir_pad_center_y);
            dir_pad_move(&pLwc->right_dir_pad, x, y, pointer_id, right_dir_pad_center_x, right_dir_pad_center_y, sr);
        }
    }
}

void lw_trigger_mouse_release(LWCONTEXT* pLwc, float x, float y, int pointer_id) {
    if (!pLwc) {
        return;
    }

    convert_touch_coord_to_ui_coord(pLwc, &x, &y);

    LOGI("mouse release ui coord x=%f, y=%f (last press ui coord x=%f, y=%f) (width %f) (height %f)\n",
         x, y,
         pLwc->last_mouse_press_x, pLwc->last_mouse_press_y,
         fabsf(x - pLwc->last_mouse_press_x),
         fabsf(y - pLwc->last_mouse_press_y));

    if (field_network(pLwc->field)) {
        mq_publish_now(pLwc, pLwc->mq, 1);
    }

    const float fist_button_x_center = pLwc->aspect_ratio - 0.3f - 0.75f / 2;
    const float fist_button_y_center = -1 + 0.75f / 2;
    const float fist_button_w = 0.75f;
    const float fist_button_h = 0.75f;

    const float top_button_x_center = fist_button_x_center;
    const float top_button_y_center = -fist_button_y_center;
    const float top_button_w = 0.75f;
    const float top_button_h = 0.75f;

    // Touch left top corner of the screen
    if (pLwc->game_scene != LGS_ADMIN
        && x < -pLwc->aspect_ratio + 0.25f
        && y > 1.0f - 0.25f) {

        change_to_admin(pLwc);
        return;
    }

    // Touch right top corner of the screen
    if (pLwc->game_scene != LGS_ADMIN
        && x > pLwc->aspect_ratio - 0.25f
        && y > 1.0f - 0.25f) {

        reset_time(pLwc);
        return;
    }

    if (pLwc->game_scene == LGS_PHYSICS
        && fabs(top_button_x_center - x) < top_button_w
        && fabs(top_button_y_center - y) < top_button_h) {
        // controlled by LWBUTTONLIST and lua script
        //puck_game_pull_puck_stop(pLwc, pLwc->puck_game);
    }

    dir_pad_release(&pLwc->left_dir_pad, pointer_id);
    float puck_fire_dx, puck_fire_dy, puck_fire_dlen;
    lw_get_normalized_dir_pad_input(pLwc, &pLwc->right_dir_pad, &puck_fire_dx, &puck_fire_dy, &puck_fire_dlen);
    if (pLwc->puck_game->player_no != 2) {
        puck_fire_dx *= -1;
        puck_fire_dy *= -1;
    }
    if (dir_pad_release(&pLwc->right_dir_pad, pointer_id)) {
        puck_game_fire(pLwc, pLwc->puck_game, puck_fire_dx, puck_fire_dy, puck_fire_dlen);
    }

    if (pLwc->game_scene == LGS_ADMIN) {
        touch_admin(pLwc, pLwc->last_mouse_press_x, pLwc->last_mouse_press_y, x, y);
    }
}

void lw_trigger_touch(LWCONTEXT* pLwc, float x, float y, int pointer_id) {
    if (!pLwc) {
        return;
    }

    convert_touch_coord_to_ui_coord(pLwc, &x, &y);

    pLwc->dialog_move_next = 1;

    if (pLwc->game_scene == LGS_FIELD) {

    } else if (pLwc->game_scene == LGS_DIALOG) {

    } else if (pLwc->game_scene == LGS_BATTLE) {

    } else if (pLwc->game_scene == LGS_ADMIN) {

    } else if (pLwc->game_scene == LGS_BATTLE_RESULT) {
        process_touch_battle_result(pLwc, x, y);
    }
}

void lw_trigger_reset(LWCONTEXT* pLwc) {
    reset_runtime_context_async(pLwc);
}

void lw_trigger_play_sound(LWCONTEXT* pLwc) {
}

void lw_trigger_key_right(LWCONTEXT* pLwc) {

    // battle

    if (pLwc->battle_state == LBS_SELECT_COMMAND) {
        if (pLwc->selected_command_slot != -1 && pLwc->player_turn_creature_index >= 0) {
            int new_selected_command_slot = -1;
            for (int i = pLwc->selected_command_slot + 1; i < MAX_COMMAND_SLOT; i++) {
                const LWSKILL* s = pLwc->player[pLwc->player_turn_creature_index].skill[i];
                if (s && s->valid) {

                    new_selected_command_slot = i;
                    break;
                }
            }

            if (new_selected_command_slot != -1) {
                pLwc->selected_command_slot = new_selected_command_slot;
            }
        }
    } else if (pLwc->battle_state == LBS_SELECT_TARGET) {

        if (pLwc->selected_enemy_slot != -1) {
            int new_selected_enemy_slot = -1;
            for (int i = pLwc->selected_enemy_slot + 1; i < MAX_ENEMY_SLOT; i++) {
                if (pLwc->enemy[i].valid
                    && pLwc->enemy[i].c.hp > 0) {

                    new_selected_enemy_slot = i;
                    break;
                }
            }

            if (new_selected_enemy_slot != -1) {
                pLwc->selected_enemy_slot = new_selected_enemy_slot;
            }
        }
    }
}

void lw_trigger_key_left(LWCONTEXT* pLwc) {

    // battle

    if (pLwc->battle_state == LBS_SELECT_COMMAND) {
        if (pLwc->selected_command_slot != -1 && pLwc->player_turn_creature_index >= 0) {
            int new_selected_command_slot = -1;
            for (int i = pLwc->selected_command_slot - 1; i >= 0; i--) {
                const LWSKILL* s = pLwc->player[pLwc->player_turn_creature_index].skill[i];
                if (s && s->valid) {

                    new_selected_command_slot = i;
                    break;
                }
            }

            if (new_selected_command_slot != -1) {
                pLwc->selected_command_slot = new_selected_command_slot;
            }
        }
    } else if (pLwc->battle_state == LBS_SELECT_TARGET) {
        if (pLwc->selected_enemy_slot != -1) {
            int new_selected_enemy_slot = -1;
            for (int i = pLwc->selected_enemy_slot - 1; i >= 0; i--) {
                if (pLwc->enemy[i].valid
                    && pLwc->enemy[i].c.hp > 0) {

                    new_selected_enemy_slot = i;
                    break;
                }
            }

            if (new_selected_enemy_slot != -1) {
                pLwc->selected_enemy_slot = new_selected_enemy_slot;
            }
        }
    }
}

void lw_trigger_key_enter(LWCONTEXT* pLwc) {

    // battle

    if (pLwc->battle_state == LBS_SELECT_COMMAND && pLwc->player_turn_creature_index >= 0) {
        pLwc->battle_state = LBS_SELECT_TARGET;

        for (int i = 0; i < ARRAY_SIZE(pLwc->enemy); i++) {
            if (pLwc->enemy[i].valid && pLwc->enemy[i].c.hp > 0) {
                pLwc->selected_enemy_slot = i;
                break;
            }
        }
    } else if (pLwc->battle_state == LBS_SELECT_TARGET) {

        exec_attack_p2e(pLwc, pLwc->selected_enemy_slot);
    }
}

static void simulate_dir_pad_touch_input(LWCONTEXT* pLwc) {
    const int simulate_pointer_id = 10;
    pLwc->left_dir_pad.pointer_id = simulate_pointer_id;
    pLwc->left_dir_pad.dragging = pLwc->player_move_left || pLwc->player_move_right || pLwc->player_move_down || pLwc->player_move_up;

    float dir_pad_center_x = 0;
    float dir_pad_center_y = 0;
    get_left_dir_pad_original_center(pLwc->aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

    pLwc->left_dir_pad.x = dir_pad_center_x + (pLwc->player_move_right - pLwc->player_move_left) / 5.0f;
    pLwc->left_dir_pad.y = dir_pad_center_y + (pLwc->player_move_up - pLwc->player_move_down) / 5.0f;
    pLwc->left_dir_pad.start_x = dir_pad_center_x;
    pLwc->left_dir_pad.start_y = dir_pad_center_y;
    pLwc->left_dir_pad.touch_began_x = dir_pad_center_x;
    pLwc->left_dir_pad.touch_began_y = dir_pad_center_y;
}

void lw_press_key_left(LWCONTEXT* pLwc) {
    pLwc->player_move_left = 1;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_right(LWCONTEXT* pLwc) {
    pLwc->player_move_right = 1;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_up(LWCONTEXT* pLwc) {
    pLwc->player_move_up = 1;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_down(LWCONTEXT* pLwc) {
    pLwc->player_move_down = 1;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_space(LWCONTEXT* pLwc) {
    pLwc->player_space = 1;
}

void lw_press_key_a(LWCONTEXT* pLwc) {
    puck_game_jump(pLwc, pLwc->puck_game);
}

void lw_press_key_z(LWCONTEXT* pLwc) {
    puck_game_dash_and_send(pLwc, pLwc->puck_game);
}

void lw_press_key_x(LWCONTEXT* pLwc) {
    puck_game_pull_puck_start(pLwc, pLwc->puck_game);
}

void lw_press_key_q(LWCONTEXT* pLwc) {
    if (pLwc->tcp && pLwc->puck_game) {
        tcp_send_suddendeath(pLwc->tcp, pLwc->puck_game->battle_id, pLwc->puck_game->token);
    }
}

void lw_release_key_left(LWCONTEXT* pLwc) {
    pLwc->player_move_left = 0;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_right(LWCONTEXT* pLwc) {
    pLwc->player_move_right = 0;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_up(LWCONTEXT* pLwc) {
    pLwc->player_move_up = 0;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_down(LWCONTEXT* pLwc) {
    pLwc->player_move_down = 0;
    simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_space(LWCONTEXT* pLwc) {
    pLwc->player_space = 0;
}

void lw_release_key_a(LWCONTEXT* pLwc) {
}

void lw_release_key_z(LWCONTEXT* pLwc) {
}

void lw_release_key_x(LWCONTEXT* pLwc) {
    puck_game_pull_puck_stop(pLwc, pLwc->puck_game);
}

void lw_release_key_q(LWCONTEXT* pLwc) {
}

void lw_go_back(LWCONTEXT* pLwc, void* native_context) {
    if (pLwc->puck_game) {
        if (pLwc->puck_game->game_state == LPGS_PRACTICE) {
            puck_game_roll_to_main_menu(pLwc->puck_game);
        } else if (pLwc->puck_game->game_state == LPGS_MAIN_MENU) {
            lw_app_quit(pLwc, native_context);
        }
    } else {
        lw_app_quit(pLwc, native_context);
    }
}
