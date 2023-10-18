#ifndef BHV_GAME_H
#define BHV_GAME_H

#include "bn_fixed_point.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "mj/mj_game.h"

namespace bhv
{
#define __PATTERN_LEN_MAX__ 8
#define __PUMPPY_COUNT__ 4

	class bhv_game : public mj::game
	{
		enum button_mapping
		{
			A = 0,
			B = 1,
			L = 2,
			R = 3,
			UP = 4,
			DOWN = 5,
			LEFT = 6,
			RIGHT = 7
		};

		enum game_phase
		{
			TEACHING,
			RECITING
		};

	public:
		bhv_game(int completed_games, const mj::game_data &data);

		[[nodiscard]] bn::string<16> title() const final
		{
			return "TEST TITLE!";
		}

		[[nodiscard]] int total_frames() const final
		{
			return _total_frames;
		}

		void fade_in(const mj::game_data &data) final;

		[[nodiscard]] mj::game_result play(const mj::game_data &data) final;

		[[nodiscard]] bool victory() const final
		{
			return _victory;
		}

		void fade_out(const mj::game_data &data) final;

	private:
		bn::regular_bg_ptr _bg;
		bn::vector<bn::sprite_ptr, __PATTERN_LEN_MAX__> _btn_sprites;
		bn::vector<bn::sprite_ptr, __PUMPPY_COUNT__> _pup_sprites;
		bn::vector<bn::sprite_ptr, 3> _conductor_sprites;
		bn::fixed_point _conductor_sprite_pos[3];
		int _total_frames;
		int _show_result_frames = 60;
		bool _victory = false;
		bool _defeat = false;

		game_phase _game_phase;
		int _item_count;
		bn::vector<int, __PATTERN_LEN_MAX__> _pattern_items;
		int _pattern_index;
		int _player_index;
		int _frames_per_reveal;


		void init(const mj::game_data &data);
		void clear();
		bool check_pattern();
		void advance_index();

		void win();
		void lose();

		void reveal_button();

	};

}

#endif
