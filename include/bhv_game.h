#ifndef BHV_GAME_H
#define BHV_GAME_H

#include "bn_regular_bg_ptr.h"

#include "mj/mj_game.h"

namespace bhv
{

class test_game : public mj::game
{

public:
    test_game(int completed_games, const mj::game_data& data);

    [[nodiscard]] bn::string<16> title() const final
    {
        return "TEST TITLE!";
    }

    [[nodiscard]] int total_frames() const final
    {
        return _total_frames;
    }

    void fade_in(const mj::game_data& data) final;

    [[nodiscard]] mj::game_result play(const mj::game_data& data) final;

    [[nodiscard]] bool victory() const final
    {
        return _victory;
    }

    void fade_out(const mj::game_data& data) final;

private:
    bn::regular_bg_ptr _bg;
    int _total_frames;
    int _show_result_frames = 60;
    bool _victory = false;
    bool _defeat = false;
};

}

#endif
