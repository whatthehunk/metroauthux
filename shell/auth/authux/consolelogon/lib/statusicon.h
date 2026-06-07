#pragma once

#include "pch.h"
#include "DirectUI/DirectUI.h"

// Replicates authui.dll (build 7878) "statusicon" custom element.
// Uses DUser SimpleTimer (via GMA_ACTION/CreateAction) for refresh,
// which is the correct pattern for DUI elements - no HWND needed.

class CDUIStatusIcon : public DirectUI::Element
{
public:
    ~CDUIStatusIcon() override;

    static DirectUI::IClassInfo* Class;
    DirectUI::IClassInfo* GetClassInfoW()  override;
    static DirectUI::IClassInfo* GetClassInfoPtr();

    static HRESULT Create(DirectUI::Element* pParent,
                          unsigned long*     pdwDeferCookie,
                          DirectUI::Element** ppElement);

    static const DirectUI::PropertyInfo* WINAPI ProviderIdProp();
    HRESULT SetProviderId(const wchar_t* pszId);

    static HRESULT Register();

    void OnHosted(DirectUI::Element* peNewRoot) override;
    void OnDestroy() override;

private:
    void _Refresh();
    void _RefreshNetwork();
    void _RefreshBattery();
    HRESULT _SetIconFromImageRes(int iconIndex, int sizePixels);

    static void CALLBACK _ActionProc(GMA_ACTIONINFO* pInfo);

    HANDLE  m_hAction  = nullptr;
    bool    m_isNetwork = false;
};
