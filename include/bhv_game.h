#ifndef BHV_GAME_H
#define BHV_GAME_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "mj/mj_game.h"

namespace bhv
{
	
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
		bn::vector<bn::sprite_ptr, 24> _sprites;
		int _total_frames;
		int _show_result_frames = 60;
		bool _victory = false;
		bool _defeat = false;

		int _item_count;
		bn::vector<int, 24> _pattern_items;
		int _pattern_index;

		void init(const mj::game_data &data);
		void clear();
		bool check_pattern();
		void advance_index();

		void win();
		void lose();
	};

}

#endif
