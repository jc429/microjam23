#ifndef BHV_GAME_H
#define BHV_GAME_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sound_item.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "mj/mj_game.h"

namespace bhv
{
#define __BHV_NOTE_COUNT_MAX__ 6

	class bhv_puppy
	{
	public:
		bhv_puppy();
		void set_position(bn::fixed_point pos);
		void update_anim();

	private:
		bn::fixed_point _pos;
		bn::optional<bn::sprite_ptr> _spr;
		bn::optional<bn::sprite_animate_action<4>> _anim_pup_idle;
		bn::optional<bn::sprite_animate_action<5>> _anim_pup_sing;
	};

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
		bn::vector<bn::sprite_ptr, __BHV_NOTE_COUNT_MAX__> _btn_sprites;
		bn::vector<bn::sprite_ptr, __BHV_NOTE_COUNT_MAX__> _pup_sprites;
		bn::vector<bn::sprite_ptr, 3> _conductor_sprites;
		bn::vector<bn::sprite_ptr, 7> _prompt_sprites;

		bn::vector<bn::sprite_animate_action<4>, __BHV_NOTE_COUNT_MAX__> _anim_cats_idle;

		bhv_puppy _player_pup;

		int _total_frames;
		int _show_result_frames = 60;
		bool _victory = false;
		bool _defeat = false;
		int _show_prompt_frames = 30;

		bhv_game_phase _game_phase;
		int _note_count;

		bn::vector<int, __BHV_NOTE_COUNT_MAX__> _pattern_items;
		int _prompt_index;
		int _recite_index;
		int _player_index;
		int _frames_per_reveal;
		bool _player_input_allowed;

		void init_sprites(const mj::game_data &data);
		void clear();

		bool any_pressed_not_start_select();
		int get_pressed_button();
		bool check_pattern(int btn);
		
		void reveal_button();
		void reveal_all_buttons();
		void hide_prompt();
		void player_recite_button(int btn);
		void cpu_recite_button();
		void advance_recite_index();
		void set_phase(bhv_game_phase phase);
		void update_animations();
	};

}

#endif
