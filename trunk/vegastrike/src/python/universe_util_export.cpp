
#ifndef USE_BOOST_128

#include "cmd/container.h"
#include <string>
#include "init.h"
#include "gfx/vec.h"
#include "cmd/unit_generic.h"
#include "python_class.h"
#ifndef USE_BOOST_128
#include <boost/python.hpp>
#else
#include <boost/python/objects.hpp>
#endif
#include "universe_util.h"
#include "cmd/unit_util.h"
#include "faction_generic.h"
#include "cmd/ai/fire.h"
#include "audilib.h"

void StarSystemExports () {
#define EXPORT_GLOBAL(name,aff) PYTHON_DEFINE_GLOBAL(VS,&name,#name);
#define voidEXPORT_GLOBAL(name) EXPORT_GLOBAL(name,0)
#define EXPORT_UTIL(name,aff) PYTHON_DEFINE_GLOBAL(VS,&UniverseUtil::name,#name);
#define voidEXPORT_UTIL(name) EXPORT_UTIL(name,0)
#undef EXPORT_FACTION
#undef voidEXPORT_FACTION
#define EXPORT_FACTION(name,aff) PYTHON_DEFINE_GLOBAL(VS,&FactionUtil::name,#name);
#define voidEXPORT_FACTION(name) EXPORT_FACTION(name,0)

	#include "star_system_exports.h"

}
#endif
