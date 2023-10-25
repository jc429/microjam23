#ifndef BHV_ENTITY_H
#define BHV_ENTITY_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

namespace bhv
{
	class bhv_entity
	{
	public:
		virtual ~bhv_entity() = default;
		virtual void set_position(bn::fixed_point pos)
		{
			_pos = pos;
			if (_spr.has_value())
			{
				_spr.get()->set_position(pos);
			}
		}
		bn::fixed_point get_position() { return _pos; }
		virtual void update_anim() = 0;
		void set_flip(bool flip)
		{
			if (_spr.has_value())
			{
				_spr.get()->set_horizontal_flip(flip);
			}
		}
		virtual void set_wait_updates(int frames) = 0;

	protected:
		bn::fixed_point _pos;
		bn::optional<bn::sprite_ptr> _spr;
	};
}

#endif