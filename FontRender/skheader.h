#ifndef UI_GFX_SK_HEADER_H_
#define UI_GFX_SK_HEADER_H_

/* #ifdef _DEBUG
	#include <crtdbg.h>

	inline void EnableMemLeakCheck()
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	}

	#define new  new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif */

#include <icu.h>
#include "no_destructor.h"

#include "mru_cache.h"
#include "SkFont.h"
#include "SkTypeface.h"
#include "SkRefCnt.h"
#include "SkScalar.h"

//#include "ui/gfx/font_render_params.h"
//#include "ubidi.h"
//#include "uscript.h"

//#include "base/macros.h"

//#include "ui/gfx/render_text.h"

#define GFX_EXPORT
#define DCHECK_LE(x,y){}
#define DCHECK_LT(x,y){}
#define DCHECK_NE(x,y){}
#define DCHECK_GT(x,y){}
#define DCHECK_GE(x,y){}
#define DCHECK_EQ(x,y){}
#define DCHECK(x) {}
#define TRACE_EVENT0(i,m){}

#define DISALLOW_COPY(TypeName) \
  TypeName(const TypeName&) = delete
// DEPRECATED: See above. Makes a class unassignable.
#define DISALLOW_ASSIGN(TypeName) TypeName& operator=(const TypeName&) = delete
// DEPRECATED: See above. Makes a class uncopyable and unassignable.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName);                 \
  DISALLOW_ASSIGN(TypeName)



#endif //UI_GFX_SK_HEADER_H_