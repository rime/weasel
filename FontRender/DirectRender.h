#ifndef UI_GFX_WIN_DIRECT_WRITE_H_
#define UI_GFX_WIN_DIRECT_WRITE_H_

#include "common.h"

namespace gfx {
namespace win {
	class DwriteRender{
		public:
			DwriteRender();
			virtual ~DwriteRender();
		public:
			bool InitializeDirectWrite();
		protected:
		private:
			bool m_bDwState;
			IDWriteFactory* m_pDwfactory = nullptr;
	};

}  // namespace win
}  // namespace gfx	
#endif //UI_GFX_WIN_DIRECT_WRITE_H_