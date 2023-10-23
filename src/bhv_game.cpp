#include "bhv_game.h"

#include "bn_fixed_point.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_sound_items.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_item.h"

#include "mj/mj_game_list.h"

#include "bn_regular_bg_items_bhv_bg.h"
#include "bn_regular_bg_items_tmg_you_lose.h"
#include "bn_regular_bg_items_tmg_you_win.h"

#include "bn_sprite_items_bhv_button_icons.h"
#include "bn_sprite_items_bhv_conductor.h"
#include "bn_sprite_items_bhv_conductor_hands.h"
#include "bn_sprite_items_bhv_instruction_bubble.h"
#include "bn_sprite_items_bhv_pumppy.h"

#define __BHV_PROMPT_DISAPPEAR__ false
#define __BHV_PLAYER_INPUTS_ALL__ false

MJ_GAME_LIST_ADD(bhv::bhv_game)

void play_tone(int btn);
bn::fixed_point get_puppy_pos(int pup_idx, int pup_count);

namespace
{
	constexpr bn::string_view code_credits[] = {"squishyfrogs"};
	constexpr bn::string_view graphics_credits[] = {"moawling"};
	constexpr bn::fixed_point puppy_pos_3[] = {
		{-80, 20},
		{-30, 40},
		{30, 40}
	};
	constexpr bn::fixed_point puppy_pos_4[] = {
		{-80, 20},
		{-30, 40},
		{30, 40},
		{80, 20}
	};
	constexpr bn::fixed_point puppy_pos_5[] = {
		{-80, 20},
		{-30, 40},
		{15, 50},
		{50, 40},
		{80, 20}
	};
	constexpr bn::fixed_point conductor_sprite_pos[] = {
		{20, -20},
		{4, -20},
		{36, -20}
	};
	constexpr bn::fixed_point prompt_base_pos = {-16,-36};

	constexpr bn::sound_item btn_tones[] = {
		bn::sound_items::bhv_c2,
		bn::sound_items::bhv_d2,
		bn::sound_items::bhv_e2,
		bn::sound_items::bhv_f2,
		bn::sound_items::bhv_g2,
		bn::sound_items::bhv_a2,
		bn::sound_items::bhv_b2,
		bn::sound_items::bhv_c3
	};
	
}

MJ_GAME_LIST_ADD_CODE_CREDITS(code_credits)
MJ_GAME_LIST_ADD_GRAPHICS_CREDITS(graphics_credits)
// MJ_GAME_LIST_ADD_MUSIC_CREDITS(music_credits)
// MJ_GAME_LIST_ADD_SFX_CREDITS(sfx_credits)

namespace bhv
{
	bhv_puppy::bhv_puppy()
	{
		_pos = bn::fixed_point(0,0);
		bn::sprite_builder builder(bn::sprite_items::bhv_pumppy);
		builder.set_bg_priority(1);
		builder.set_z_order(20);
		builder.set_position(_pos);
		_spr = builder.release_build();
		_anim_pup_idle = bn::create_sprite_animate_action_forever(*_spr.get(), 6, bn::sprite_items::bhv_pumppy.tiles_item(), 0, 1, 2, 3);
		_anim_pup_sing = bn::create_sprite_animate_action_once(*_spr.get(), 6, bn::sprite_items::bhv_pumppy.tiles_item(), 4, 5, 6, 7, 4);
	}

	void bhv_puppy::set_position(bn::fixed_point pos)
	{
		_pos = pos;
		if(_spr.has_value())
		{
			_spr.get()->set_position(pos);
		}
	}

	void bhv_puppy::update_anim()
	{
		if (_anim_pup_idle.has_value())
		{
			_anim_pup_idle.get()->update();
		}
	}

	bhv_game::bhv_game(int completed_games, const mj::game_data &data) : 
		_bg(bn::regular_bg_items::bhv_bg.create_bg((256 - 240) / 2, (256 - 160) / 2)),
		// TODO: Select final bgm CONTENDERS: TOTSNUK10, TOTSNUK01,
		_total_frames(play_jingle(mj::game_jingle_type::METRONOME_16BEAT, completed_games, data))
	{
		constexpr int frames_diff = maximum_frames - minimum_frames;
		constexpr int maximum_speed_completed_games = 30;

		completed_games = bn::min(completed_games, maximum_speed_completed_games);

		int frames_reduction = (frames_diff * completed_games) / maximum_speed_completed_games;
		_total_frames = maximum_frames - frames_reduction;
		_total_frames -= data.random.get_int(60);
		_total_frames = bn::clamp(_total_frames, minimum_frames, maximum_frames);

		_frames_per_reveal = _total_frames / 18; //TODO: fix this to match bpm

		// TODO: prefer longer patterns the more minigames have been cleared?
		// 3-5 notes seems right?
		_note_count = 3 + data.random.get_int(3);
		_prompt_index = 0;
		_recite_index = 0;
		_player_index = 2 + data.random.get_int(3);
		if(_player_index >= _note_count)
		{
			_player_index = _note_count - 1;
		}
		_player_pup.set_position(get_puppy_pos(_player_index, _note_count));

		init_sprites(data);
		_game_phase = BHV_PHASE_TEACHING;
	}

	void bhv_game::init_sprites(const mj::game_data &data)
	{
		clear();
		
		// BUILD BUTTON SPRITES
		{
			// init sprite builder
			bn::sprite_builder builder(bn::sprite_items::bhv_button_icons);
			builder.set_bg_priority(1);
			builder.set_z_order(10);

			for (int i = 0; i < _note_count; i++)
			{
				// identify button for pattern
				int button = data.random.get_int(8);
				_pattern_items.push_back(button);

				bn::sprite_tiles_item sheet_tiles = bn::sprite_items::bhv_button_icons.tiles_item();

				// sprite setup
				int x_off = ((i - _note_count) * 18) + 4;
				int y_off = (-2 * button) + 6;
				bn::fixed_point pos = {prompt_base_pos.x() + x_off, prompt_base_pos.y() + y_off};
				builder.set_position(pos);
				_btn_sprites.push_back(builder.build());
				int btn_spr = button * 2;
				_btn_sprites.back().set_tiles(sheet_tiles.create_tiles(btn_spr));
				_btn_sprites.back().set_visible(false);
			}
		}

		// BUILD CAT SPRITES
		{
			bn::sprite_builder builder(bn::sprite_items::bhv_pumppy);
			builder.set_bg_priority(1);
			builder.set_z_order(20);

			for (int i = 0; i < _note_count; i++)
			{
				// sprite setup
				builder.set_position(get_puppy_pos(i,_note_count));
				_pup_sprites.push_back(builder.build());
				_pup_sprites.back().set_horizontal_flip(i < 3);
				_anim_cats_idle.push_back(bn::create_sprite_animate_action_forever(_pup_sprites.back(), 6, bn::sprite_items::bhv_pumppy.tiles_item(), 0, 1, 2, 3));
				
			}
			// TODO: split pup and cat sprites
			// _anim_pup_sing = bn::create_sprite_animate_action_once(_pup_sprites.back(), 6, bn::sprite_items::bhv_pumppy.tiles_item(), 4, 5, 6, 7, 4);
		}


		// BUILD CONDUCTOR SPRITES
		{
			bn::sprite_builder builder(bn::sprite_items::bhv_conductor);
			builder.set_bg_priority(1);
			builder.set_z_order(50);

			builder.set_position(conductor_sprite_pos[0]);
			_conductor_sprites.push_back(builder.release_build());

			bn::sprite_builder arm_builder(bn::sprite_items::bhv_conductor_hands);
			bn::sprite_tiles_item arm_tiles = bn::sprite_items::bhv_conductor_hands.tiles_item();
			arm_builder.set_bg_priority(1);
			arm_builder.set_z_order(50);

			for (int i = 0; i < 2; i++)
			{
				arm_builder.set_position(conductor_sprite_pos[i + 1]);
				_conductor_sprites.push_back(arm_builder.build());
				_conductor_sprites.back().set_tiles(arm_tiles.create_tiles(i));
			}
		}

		// BUILD PROMPT SPEECH BUBBLE
		{
			bn::sprite_builder builder(bn::sprite_items::bhv_instruction_bubble);
			bn::sprite_tiles_item speech_bubble_tiles = bn::sprite_items::bhv_instruction_bubble.tiles_item();
			builder.set_bg_priority(1);
			builder.set_z_order(20);

			for (int i = 0; i < _note_count+2; i++)
			{
				bn::fixed_point pos = { prompt_base_pos.x() + (-16*i), prompt_base_pos.y()};
				builder.set_position(pos);
				_prompt_sprites.push_back(builder.build());
				if(i == 0)
				{
					_prompt_sprites.back().set_tiles(speech_bubble_tiles.create_tiles(0));
				}
				else if(i == _note_count+1)
				{
					_prompt_sprites.back().set_tiles(speech_bubble_tiles.create_tiles(2));
				}
				else
				{
					_prompt_sprites.back().set_tiles(speech_bubble_tiles.create_tiles(1));
				}
			}
		}
	}

	void bhv_game::clear()
	{
		_btn_sprites.clear();
		_conductor_sprites.clear();
		_pup_sprites.clear();
		_pattern_items.clear();
		_prompt_sprites.clear();
		_anim_cats_idle.clear();
	}

	mj::game_result bhv_game::play(const mj::game_data &data)
	{
		mj::game_result result;
		result.exit = data.pending_frames == 0;
		int frames_elapsed = _total_frames - data.pending_frames;

		update_animations();

		if (!_victory && !_defeat)
		{
			// game beat
			if (frames_elapsed > 0 && (frames_elapsed % _frames_per_reveal == 0))
			{
				if (_game_phase == BHV_PHASE_TEACHING)
				{
					if (_prompt_index == _btn_sprites.size())
					{
						set_phase(BHV_PHASE_RECITING);
					}
					else
					{
						reveal_button();
					}
				}
				else if ((_game_phase == BHV_PHASE_RECITING) && !__BHV_PLAYER_INPUTS_ALL__)
				{
					cpu_recite_button();
				}
			}

			if (!__BHV_PLAYER_INPUTS_ALL__ && (_game_phase == BHV_PHASE_RECITING))
			{
						
			}
			else if(__BHV_PLAYER_INPUTS_ALL__)
			{
				// keep prompt around for a bit
				if (_game_phase == BHV_PHASE_RECITING && _show_prompt_frames > 0)
				{
					_show_prompt_frames--;
					if (_show_prompt_frames == 0)
					{
						hide_prompt();
					}
				}

				if (any_pressed_not_start_select())
				{
					// check vs pattern
					int btn = get_pressed_button();
					if (btn >= 0 && (_game_phase == BHV_PHASE_RECITING))
					{
						player_recite_button(btn);
					}
				}
			}
			
			
			// update conductor sprite pos
			{
				bn::fixed y_float = (3.5 * bn::sin(0.005*frames_elapsed));
				for (int i = 0; i < _conductor_sprites.size(); i++)
				{
					bn::fixed_point pos = conductor_sprite_pos[i];
					pos.set_y(pos.y() + y_float);
					_conductor_sprites[i].set_position(pos);
				}
			}
		}
		else
		{
			if (_show_result_frames)
			{
				--_show_result_frames;
			}
			else
			{
				result.exit = true;
				clear();
			}
		}

		return result;
	}

	bool bhv_game::any_pressed_not_start_select()
	{
		return bn::keypad::any_pressed() && !bn::keypad::start_pressed() && !bn::keypad::select_pressed();
	}

	int bhv_game::get_pressed_button()
	{
		if (bn::keypad::a_pressed())
		{
			return button_mapping::A;
		}
		if (bn::keypad::b_pressed())
		{
			return button_mapping::B;
		}
		if (bn::keypad::l_pressed())
		{
			return button_mapping::L;
		}
		if (bn::keypad::r_pressed())
		{
			return button_mapping::R;
		}
		if (bn::keypad::left_pressed())
		{
			return button_mapping::LEFT;
		}
		if (bn::keypad::right_pressed())
		{
			return button_mapping::RIGHT;
		}
		if (bn::keypad::up_pressed())
		{
			return button_mapping::UP;
		}
		if (bn::keypad::down_pressed())
		{
			return button_mapping::DOWN;
		}
		return -1;
	}

	bool bhv_game::check_pattern(int btn)
	{
		switch (btn)
		{
		case button_mapping::A:
			return bn::keypad::a_pressed();
			break;
		case button_mapping::B:
			return bn::keypad::b_pressed();
			break;
		case button_mapping::L:
			return bn::keypad::l_pressed();
			break;
		case button_mapping::R:
			return bn::keypad::r_pressed();
			break;
		case button_mapping::UP:
			return bn::keypad::up_pressed();
			break;
		case button_mapping::DOWN:
			return bn::keypad::down_pressed();
			break;
		case button_mapping::LEFT:
			return bn::keypad::left_pressed();
			break;
		case button_mapping::RIGHT:
			return bn::keypad::right_pressed();
			break;
		default:
			return false;
			break;
		}
	}

	void bhv_game::reveal_button()
	{
		if(_prompt_index >= _btn_sprites.size())
		{
			return;
		}

		_btn_sprites[_prompt_index].set_visible(true);
		play_tone(_pattern_items[_prompt_index]);

		_prompt_index++;
		
	}

	void bhv_game::reveal_all_buttons()
	{
		for (int i = 0; i < _btn_sprites.size(); i++)
		{
			_btn_sprites[i].set_visible(true);
		}
	}

	void bhv_game::hide_prompt()
	{
		if (!__BHV_PROMPT_DISAPPEAR__)
		{
			return;
		}

		for (int i = 0; i < _btn_sprites.size(); i++)
		{
			_btn_sprites[i].set_visible(false);
		}

		for (int i = 0; i < _prompt_sprites.size(); i++)
		{
			_prompt_sprites[i].set_visible(false);
		}
	}

	void bhv_game::player_recite_button(int btn)
	{
		bool success = check_pattern(_pattern_items[_recite_index]);

		bn::fixed_point pos = get_puppy_pos(_recite_index,_note_count);
		pos.set_y(pos.y() - 10); // TODO: TEMP
		_pup_sprites[_recite_index].set_position(pos);

		play_tone(btn);

		if (success)
		{
			advance_recite_index();
		}
		else
		{
			set_phase(BHV_PHASE_RESULTS);
			_defeat = true;
			_bg.set_item(bn::regular_bg_items::tmg_you_lose); // TODO: TEMP
		}	
	}

	void bhv_game::cpu_recite_button()
	{
		if(_recite_index == _player_index)
		{
			advance_recite_index();
			return;
		}

		int btn = _pattern_items[_recite_index];

		bn::fixed_point pos = get_puppy_pos(_recite_index, _note_count);
		pos.set_y(pos.y() - 10); // TODO: TEMP
		_pup_sprites[_recite_index].set_position(pos);

		play_tone(btn);

		advance_recite_index();
	}

	void bhv_game::advance_recite_index()
	{
		{
			bn::sprite_tiles_item sheet_tiles = bn::sprite_items::bhv_button_icons.tiles_item();
			int btn_spr = (_pattern_items[_recite_index] * 2) + 1;
			_btn_sprites[_recite_index].set_tiles(sheet_tiles.create_tiles(btn_spr));
		}
		_recite_index++;
		if (_recite_index >= _pattern_items.size())
		{
			set_phase(BHV_PHASE_RESULTS);
			_victory = true;
			_bg.set_item(bn::regular_bg_items::tmg_you_win); //TODO: TEMP
		}
	}

	void bhv_game::set_phase(bhv_game_phase phase)
	{
		_game_phase = phase;
		_prompt_index = 0;
	}

	void bhv_game::update_animations()
	{

		// cats
		for (auto it = _anim_cats_idle.begin(), end = _anim_cats_idle.end(); it != end;)
		{
			bn::sprite_animate_action<4> &action = *it;
			action.update();

			if (action.done())
			{
				it = _anim_cats_idle.erase(it);
				end = _anim_cats_idle.end();
			}
			else
			{
				++it;
			}
		}

		_player_pup.update_anim();
	}

}

void play_tone(int btn)
{
	if (btn < 0 || btn >= 8)
	{
		return;
	}
	btn_tones[btn].play();
}

bn::fixed_point get_puppy_pos(int pup_idx, int pup_count)
{
	if (pup_idx >= pup_count || pup_idx < 0)
	{
		return bn::fixed_point();
	}
	switch (pup_count)
	{
	case 3:
		return puppy_pos_3[pup_idx];
		break;
	case 4:
		return puppy_pos_4[pup_idx];
		break;
	case 5:
		return puppy_pos_5[pup_idx];
		break;
	default:
		break;
	}
	return bn::fixed_point();
}