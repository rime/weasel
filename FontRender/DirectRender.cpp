#include "DirectRender.h"

namespace gfx {
namespace win {

DwriteRender::DwriteRender(){
    m_bDwState = false;
    m_pDwfactory = nullptr;
}
DwriteRender::~DwriteRender(){
    if(m_bDwState && m_pDwfactory)
        m_pDwfactory->Release();
}

bool DwriteRender::InitializeDirectWrite() {
    HRESULT hr = DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&m_pDwfactory)
                );
    if (FAILED(hr)){
        D_INFO("DwriteRender::InitializeDirectWrite failed"); 
        m_pDwfactory=nullptr; 
        return false;  
    }
    m_pDwfactory->AddRef();
    m_bDwState=true;
    return m_bDwState;
}

}  // namespace win
}  // namespace gfx
