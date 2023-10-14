#include "bhv_game.h"

#include "bn_fixed_point.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_item.h"

#include "mj/mj_game_list.h"

#include "bn_regular_bg_items_tmg_press_a.h"
#include "bn_regular_bg_items_tmg_you_lose.h"
#include "bn_regular_bg_items_tmg_you_win.h"

#include "bn_sprite_items_bhv_button_icons.h"

MJ_GAME_LIST_ADD(bhv::bhv_game)

namespace
{
	constexpr bn::string_view code_credits[] = {"squishyfrogs"};
	constexpr bn::string_view graphics_credits[] = {"moaw"};
}

namespace bhv
{

	bhv_game::bhv_game(int completed_games, const mj::game_data &data) : _bg(bn::regular_bg_items::tmg_press_a.create_bg((256 - 240) / 2, (256 - 160) / 2))
	{
		constexpr int frames_diff = maximum_frames - minimum_frames;
		constexpr int maximum_speed_completed_games = 30;

		completed_games = bn::min(completed_games, maximum_speed_completed_games);

		int frames_reduction = (frames_diff * completed_games) / maximum_speed_completed_games;
		_total_frames = maximum_frames - frames_reduction;
		_total_frames -= data.random.get_int(60);
		_total_frames = bn::clamp(_total_frames, minimum_frames, maximum_frames);

		init(data);
	}

	void bhv_game::fade_in([[maybe_unused]] const mj::game_data &data)
	{
	}

	mj::game_result bhv_game::play(const mj::game_data &data)
	{
		mj::game_result result;
		result.exit = data.pending_frames == 0;

		if (!_victory && !_defeat)
		{

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
		_item_count = 3 + data.random.get_int(6);
		_pattern_index = 0;

		{
			// init sprite builder
			bn::sprite_builder builder(bn::sprite_items::bhv_button_icons);
			builder.set_bg_priority(1);
			builder.set_z_order(0);

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
				_sprites.push_back(builder.build());
				int btn_spr = button*2;
				_sprites.back().set_tiles(sheet_tiles.create_tiles(btn_spr));
			}

		}
	}

	void bhv_game::clear()
	{
		_sprites.clear();
		_pattern_items.clear();
	}

	bool bhv_game::check_pattern()
	{
		if (_pattern_index >= _pattern_items.size())
		{
			return false;
		}

		switch (_pattern_items[_pattern_index])
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
			int btn_spr = (_pattern_items[_pattern_index] * 2) + 1;
			_sprites[_pattern_index].set_tiles(sheet_tiles.create_tiles(btn_spr));
		}
		_pattern_index++;
		if (_pattern_index >= _pattern_items.size())
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
}
