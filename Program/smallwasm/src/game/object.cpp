#include "object.h"

namespace shine { namespace game {

Object* Object::s_gc_head = nullptr;
unsigned int Object::s_pending_kill_count = 0;

} } // namespace shine::game

