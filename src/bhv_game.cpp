#include "bhv_game.h"

#include "bn_fixed_point.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_item.h"

#include "mj/mj_game_list.h"

#include "bn_regular_bg_items_bhv_bg.h"
#include "bn_regular_bg_items_tmg_you_lose.h"
#include "bn_regular_bg_items_tmg_you_win.h"

#include "bn_sprite_items_bhv_button_icons.h"
#include "bn_sprite_items_bhv_pumppy.h"
#include "bn_sprite_items_bhv_conductor.h"
#include "bn_sprite_items_bhv_conductor_hands.h"

MJ_GAME_LIST_ADD(bhv::bhv_game)

namespace
{
	constexpr bn::string_view code_credits[] = {"squishyfrogs"};
	constexpr bn::string_view graphics_credits[] = {"moawling"};
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
		_total_frames(play_jingle(mj::game_jingle_type::TOTSNUK10, completed_games, data)) 
	{
		constexpr int frames_diff = maximum_frames - minimum_frames;
		constexpr int maximum_speed_completed_games = 30;

		completed_games = bn::min(completed_games, maximum_speed_completed_games);

		int frames_reduction = (frames_diff * completed_games) / maximum_speed_completed_games;
		_total_frames = maximum_frames - frames_reduction;
		_total_frames -= data.random.get_int(60);
		_total_frames = bn::clamp(_total_frames, minimum_frames, maximum_frames);

		_frames_per_reveal = _total_frames / 10; //TODO: fix this to match bpm

		init(data);
		_game_phase = BHV_PHASE_TEACHING;
	}

	void bhv_game::fade_in([[maybe_unused]] const mj::game_data &data)
	{
	}

	mj::game_result bhv_game::play(const mj::game_data &data)
	{
		mj::game_result result;
		result.exit = data.pending_frames == 0;
		int frames_elapsed = _total_frames - data.pending_frames;

		if (!_victory && !_defeat)
		{

			// if (data.pending_frames + (_frames_per_reveal * (1+_pattern_index)) == _total_frames)
			if (frames_elapsed > 0 && (frames_elapsed % _frames_per_reveal == 0))
			{
				game_tick();
			}

			if (bn::keypad::any_pressed() && !bn::keypad::start_pressed() && !bn::keypad::select_pressed())
			{
				// check vs pattern
				bool success = check_pattern();

				if (success)
				{
					advance_index();
				}
				else
				{
					lose();
				}
			}

			// update conductor sprite pos
			{
				
				bn::fixed y_float = (3.5 * bn::sin(0.005*frames_elapsed));
				for (int i = 0; i < _conductor_sprites.size(); i++)
				{
					bn::fixed_point pos = _conductor_sprite_pos[i];
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

	void bhv_game::fade_out([[maybe_unused]] const mj::game_data &data)
	{
	}

	void bhv_game::init(const mj::game_data &data)
	{
		clear();
		// TODO: prefer longer patterns the more minigames have been cleared?
		_item_count = 3 + data.random.get_int(5);
		_pattern_index = 0;
		_player_index = 0;

		// BUILD BUTTON SPRITES
		{
			// init sprite builder
			bn::sprite_builder builder(bn::sprite_items::bhv_button_icons);
			builder.set_bg_priority(1);
			builder.set_z_order(10);

			const int btn_spacing = 20;
			int btns_width = _item_count * btn_spacing;
			bn::fixed_point btn_offset(10 + (btns_width/-2),-20);

			for (int i = 0; i < _item_count; i++)
			{
				// identify button for pattern
				int button = data.random.get_int(8);
				_pattern_items.push_back(button);

				bn::sprite_tiles_item sheet_tiles = bn::sprite_items::bhv_button_icons.tiles_item();

				// sprite setup
				builder.set_position(bn::fixed_point(btn_offset.x() + (i * btn_spacing), btn_offset.y()));
				_btn_sprites.push_back(builder.build());
				int btn_spr = button*2;
				_btn_sprites.back().set_tiles(sheet_tiles.create_tiles(btn_spr));
				_btn_sprites.back().set_visible(false);
			}
		}

		// BUILD PUMPPY SPRITES
		{
			bn::sprite_builder builder(bn::sprite_items::bhv_pumppy);
			builder.set_bg_priority(1);
			builder.set_z_order(20);

			_pup_spr_pos[0] = {-80, 20};
			_pup_spr_pos[1] = {-30, 40};
			_pup_spr_pos[2] = {30, 40};
			_pup_spr_pos[3] = {80, 20};

			for (int i = 0; i < __PUMPPY_COUNT__; i++)
			{
				// sprite setup
				builder.set_position(_pup_spr_pos[i]);
				_pup_sprites.push_back(builder.build());
			}
		}

		// BUILD CONDUCTOR SPRITES
		{
			bn::sprite_builder builder(bn::sprite_items::bhv_conductor);
			builder.set_bg_priority(1);
			builder.set_z_order(50);

			_conductor_sprite_pos[0] = {20, -20};

			builder.set_position(_conductor_sprite_pos[0]);
			_conductor_sprites.push_back(builder.release_build());

			bn::sprite_builder arm_builder(bn::sprite_items::bhv_conductor_hands);
			bn::sprite_tiles_item arm_tiles = bn::sprite_items::bhv_conductor_hands.tiles_item();
			_conductor_sprite_pos[1] = {_conductor_sprite_pos[0].x() - 16, _conductor_sprite_pos[0].y() + 0};
			_conductor_sprite_pos[2] = {_conductor_sprite_pos[0].x() + 16, _conductor_sprite_pos[0].y() + 0};
			arm_builder.set_bg_priority(1);
			arm_builder.set_z_order(50);

			for(int i = 0; i < 2; i++)
			{
				arm_builder.set_position(_conductor_sprite_pos[i+1]);
				_conductor_sprites.push_back(arm_builder.build());
				_conductor_sprites.back().set_tiles(arm_tiles.create_tiles(i));
			}
			
		}
	}

	void bhv_game::clear()
	{
		_btn_sprites.clear();
		_conductor_sprites.clear();
		_pup_sprites.clear();
		_pattern_items.clear();
	}

	bool bhv_game::check_pattern()
	{
		if (_player_index >= _pattern_items.size())
		{
			return false;
		}

		switch (_pattern_items[_player_index])
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
			break;
		}

		return false;
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
			win();
		}
	}

	void bhv_game::win()
	{
		_bg.set_item(bn::regular_bg_items::tmg_you_win);
		// result.remove_title = true;
		_victory = true;
	}

	void bhv_game::lose()
	{
		_bg.set_item(bn::regular_bg_items::tmg_you_lose);
		// result.remove_title = true;
		_defeat = true;
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
		_pattern_index++;
		if(_pattern_index == _btn_sprites.size())
		{
			set_phase(BHV_PHASE_RECITING);
		}
	}

	void bhv_game::recite_button()
	{
		if (_pattern_index >= __PUMPPY_COUNT__)
		{
			return;
		}

		bn::fixed_point pos = _pup_spr_pos[_pattern_index];
		pos.set_y(pos.y() - 10); // TODO: TEMP
		_pup_sprites[_pattern_index].set_position(pos);

		_pattern_index++;
		if (_pattern_index == _btn_sprites.size())
		{
			// RESULTS
		}

	}

	void bhv_game::set_phase(bhv_game_phase phase)
	{
		_game_phase = phase;
		_pattern_index = 0;
	}
}
