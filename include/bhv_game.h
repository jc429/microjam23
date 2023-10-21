#ifndef BHV_GAME_H
#define BHV_GAME_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sound_item.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "mj/mj_game.h"

namespace bhv
{
#define __NOTE_COUNT_MAX__ 6

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

		enum bhv_game_phase
		{
			BHV_PHASE_TEACHING,
			BHV_PHASE_RECITING,
			BHV_PHASE_RESULTS
		};

	public:
		bhv_game(int completed_games, const mj::game_data &data);
		[[nodiscard]] mj::game_result play(const mj::game_data &data) final;

		[[nodiscard]] bn::string<16> title() const final
		{
			return "SING!";
		}

		[[nodiscard]] int total_frames() const final
		{
			return _total_frames;
		}

		[[nodiscard]] bool victory() const final
		{
			return _victory;
		}

		void fade_in(const mj::game_data &data) final {}
		void fade_out(const mj::game_data &data) final {}

	private:
		bn::regular_bg_ptr _bg;
		bn::vector<bn::sprite_ptr, __NOTE_COUNT_MAX__> _btn_sprites;
		bn::vector<bn::sprite_ptr, __NOTE_COUNT_MAX__> _pup_sprites;
		bn::vector<bn::sprite_ptr, 3> _conductor_sprites;
		bn::vector<bn::sprite_ptr, 5> _prompt_sprites;

		int _total_frames;
		int _show_result_frames = 60;
		bool _victory = false;
		bool _defeat = false;
		int _show_prompt_frames = 30;

		bhv_game_phase _game_phase;
		int _note_count;
		bn::vector<int, __NOTE_COUNT_MAX__> _pattern_items;
		int _pattern_index;
		int _player_index;
		int _frames_per_reveal;

		void init_sprites(const mj::game_data &data);
		void clear();
		void end_game(bool victory);

		bool any_pressed_not_start_select();
		int get_pressed_button();
		bool check_pattern(int btn);
		void play_tone(int btn);
		bn::optional<bn::sound_item> get_tone(int btn);
		void advance_index();

		void game_tick();
		void reveal_button();
		void reveal_all_buttons();
		void hide_prompt();
		void recite_button();
		void set_phase(bhv_game_phase phase);

	};

}

#endif
