#include "pch.h"
#include "touchscrollviewer.h"

// dui70's TouchScrollViewer::Initialize takes only 2 args:
//   HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie)
// so we cannot use CreateAndInit<T, int> which passes 3 args.
// Instead we implement Create manually.

DirectUI::IClassInfo* CDUITouchScrollViewer::Class = nullptr;

CDUITouchScrollViewer::~CDUITouchScrollViewer()
{
    DirectUI::TouchScrollViewer::~TouchScrollViewer();
}

DirectUI::IClassInfo* CDUITouchScrollViewer::GetClassInfoW()   { return Class; }
DirectUI::IClassInfo* CDUITouchScrollViewer::GetClassInfoPtr() { return Class; }

HRESULT CDUITouchScrollViewer::Create(DirectUI::Element*  pParent,
                                       unsigned long*      pdwDeferCookie,
                                       DirectUI::Element** ppElement)
{
    if (!ppElement) return E_POINTER;
    *ppElement = nullptr;

    CDUITouchScrollViewer* p = DirectUI::HNew<CDUITouchScrollViewer>();
    if (!p) return E_OUTOFMEMORY;

    HRESULT hr = p->Initialize(pParent, pdwDeferCookie);
    if (FAILED(hr))
    {
        p->Destroy(false);
        return hr;
    }

    *ppElement = p;
    return S_OK;
}

HRESULT CDUITouchScrollViewer::Register()
{
    return DirectUI::ClassInfo<CDUITouchScrollViewer, DirectUI::TouchScrollViewer>
        ::RegisterGlobal(HINST_THISCOMPONENT, L"TouchScrollViewer", nullptr, 0);
}
