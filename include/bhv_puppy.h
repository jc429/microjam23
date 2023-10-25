#ifndef BHV_PUPPY_H
#define BHV_PUPPY_H

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bhv_entity.h"

namespace bhv
{
	class bhv_puppy : public bhv_entity
	{
	public:
		bhv_puppy();
		void update_anim() override;
		void set_wait_updates(int frames) override;

	private:
		bn::optional<bn::sprite_animate_action<4>> _anim_idle;
		bn::optional<bn::sprite_animate_action<5>> _anim_sing;
	};
}

#endif