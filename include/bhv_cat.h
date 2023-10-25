#ifndef BHV_CAT_H
#define BHV_CAT_H

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bhv_entity.h"

namespace bhv
{
	class bhv_cat : public bhv_entity
	{
	public:
		bhv_cat(bn::fixed_point pos);
		void update_anim() override;
		void set_wait_updates(int frames) override;

	private:
		bn::optional<bn::sprite_animate_action<6>> _anim_idle;
		bn::optional<bn::sprite_animate_action<5>> _anim_sing;
	};
}

#endif