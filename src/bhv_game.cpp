#include "bhv_game.h"

#include "bn_fixed_point.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_sound_items.h"
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

MJ_GAME_LIST_ADD(bhv::bhv_game)

namespace
{
	constexpr bn::string_view code_credits[] = {"squishyfrogs"};
	constexpr bn::string_view graphics_credits[] = {"moawling"};
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
		_pattern_index = 0;
		_player_index = 0;

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

			const int btn_spacing = 20;
			int btns_width = _note_count * btn_spacing;
			bn::fixed_point btn_offset((btns_width / -2) - 40, -30);

			for (int i = 0; i < _note_count; i++)
			{
				// identify button for pattern
				int button = data.random.get_int(8);
				_pattern_items.push_back(button);

				bn::sprite_tiles_item sheet_tiles = bn::sprite_items::bhv_button_icons.tiles_item();

				// sprite setup
				int y_off = -2 * button;
				builder.set_position(bn::fixed_point(btn_offset.x() + (i * btn_spacing), btn_offset.y() + y_off));
				_btn_sprites.push_back(builder.build());
				int btn_spr = button * 2;
				_btn_sprites.back().set_tiles(sheet_tiles.create_tiles(btn_spr));
				_btn_sprites.back().set_visible(false);
			}
		}

		// BUILD PUMPPY SPRITES
		{
			bn::sprite_builder builder(bn::sprite_items::bhv_pumppy);
			builder.set_bg_priority(1);
			builder.set_z_order(20);

			for (int i = 0; i < _note_count; i++)
			{
				// sprite setup
				builder.set_position(puppy_pos_5[i]);
				_pup_sprites.push_back(builder.build());
			}
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
	}

	void bhv_game::clear()
	{
		_btn_sprites.clear();
		_conductor_sprites.clear();
		_pup_sprites.clear();
		_pattern_items.clear();
		_prompt_sprites.clear();
	}

	mj::game_result bhv_game::play(const mj::game_data &data)
	{
		mj::game_result result;
		result.exit = data.pending_frames == 0;
		int frames_elapsed = _total_frames - data.pending_frames;

		if (!_victory && !_defeat)
		{

			if (frames_elapsed > 0 && (frames_elapsed % _frames_per_reveal == 0) && (_game_phase == BHV_PHASE_TEACHING))
			{
				// game_tick();
				reveal_button();
			}

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
				// jump to reciting phase if player presses buttons early
				if (_game_phase == BHV_PHASE_TEACHING)
				{
					//TODO: reveal all remaining notes instantly & keep on screen for a couple frames (20?)
					reveal_all_buttons();
					set_phase(BHV_PHASE_RECITING);
				}

				// check vs pattern
				int btn = get_pressed_button();
				if(btn >= 0)
				{
					play_tone(btn);
				}

				bool success = check_pattern(_pattern_items[_player_index]);

				if (success)
				{
					advance_index();
				}
				else
				{
					end_game(false);
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

	void bhv_game::end_game(bool victory)
	{
		_bg.set_item(victory ? bn::regular_bg_items::tmg_you_win : bn::regular_bg_items::tmg_you_lose);
		_victory = victory;
		_defeat = !victory;
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

	void bhv_game::play_tone(int btn)
	{
		bn::optional<bn::sound_item> tone = get_tone(btn);
		if (tone.get() != NULL)
		{
			tone.get()->play();
		}
	}

	bn::optional<bn::sound_item> bhv_game::get_tone(int btn)
	{
		if(btn < 0 || btn >= 8)
		{
			return bn::nullopt;
		}
		return btn_tones[btn];
	}

	void bhv_game::advance_index()
	{
		{
			bn::sprite_tiles_item sheet_tiles = bn::sprite_items::bhv_button_icons.tiles_item();
			int btn_spr = (_pattern_items[_player_index] * 2) + 1;
			_btn_sprites[_player_index].set_tiles(sheet_tiles.create_tiles(btn_spr));
		}
		_player_index++;
		if (_player_index >= _pattern_items.size())
		{
			end_game(true);
		}
	}

	void bhv_game::game_tick()
	{
		switch (_game_phase)
		{
		case BHV_PHASE_TEACHING:
			reveal_button();
			break;
		case BHV_PHASE_RECITING:
			recite_button();
			break;
		
		default:
			break;
		}
		
	}

	void bhv_game::reveal_button()
	{
		if(_pattern_index >= _btn_sprites.size())
		{
			return;
		}

		_btn_sprites[_pattern_index].set_visible(true);
		play_tone(_pattern_items[_pattern_index]);

		_pattern_index++;
		if(_pattern_index == _btn_sprites.size())
		{
			set_phase(BHV_PHASE_RECITING);
		}
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
		for (int i = 0; i < _btn_sprites.size(); i++)
		{
			_btn_sprites[i].set_visible(false);
		}
	}

	void bhv_game::recite_button()
	{
		if (_pattern_index >= __NOTE_COUNT_MAX__)
		{
			return;
		}

		bn::fixed_point pos = puppy_pos_4[_pattern_index];
		pos.set_y(pos.y() - 10); // TODO: TEMP
		_pup_sprites[_pattern_index].set_position(pos);

		play_tone(_pattern_items[_pattern_index]);

		_pattern_index++;
		if (_pattern_index == _btn_sprites.size())
		{
			// RESULTS
			set_phase(BHV_PHASE_RESULTS);
		}

	}

	void bhv_game::set_phase(bhv_game_phase phase)
	{
		_game_phase = phase;
		_pattern_index = 0;
	}
}
