#include "pch.h"
#include "statusicon.h"
#include "duiutil.h"

// Do NOT include netioapi.h or wininet.h here - they conflict with
// winsock.h already pulled in by pch.h -> windows.h.
// Instead we declare only the types we need manually.

#pragma comment(lib, "iphlpapi.lib")

// Minimal declarations from netioapi.h for GetNetworkConnectivityHint
typedef enum _NL_NETWORK_CONNECTIVITY_LEVEL_HINT {
    NetworkConnectivityLevelHintUnknown         = 0,
    NetworkConnectivityLevelHintNone            = 1,
    NetworkConnectivityLevelHintLocalAccess     = 2,
    NetworkConnectivityLevelHintInternetAccess  = 3,
    NetworkConnectivityLevelHintConstrainedInternetAccess = 4,
    NetworkConnectivityLevelHintHidden          = 5,
} NL_NETWORK_CONNECTIVITY_LEVEL_HINT;

typedef enum _NL_NETWORK_CONNECTIVITY_COST_HINT {
    NetworkConnectivityCostHintUnknown    = 0,
    NetworkConnectivityCostHintUnrestricted = 1,
    NetworkConnectivityCostHintFixed      = 2,
    NetworkConnectivityCostHintVariable   = 3,
} NL_NETWORK_CONNECTIVITY_COST_HINT;

typedef struct _NL_NETWORK_CONNECTIVITY_HINT {
    NL_NETWORK_CONNECTIVITY_LEVEL_HINT ConnectivityLevel;
    NL_NETWORK_CONNECTIVITY_COST_HINT  ConnectivityCost;
    BOOLEAN                            ApproachingDataLimit;
    BOOLEAN                            OverDataLimit;
    BOOLEAN                            Roaming;
} NL_NETWORK_CONNECTIVITY_HINT;

typedef DWORD (NETIOAPI_API_ *PFN_GetNetworkConnectivityHint)(
    NL_NETWORK_CONNECTIVITY_HINT* ConnectivityHint);

static constexpr int   ICON_SIZE_RP      = 40;
static constexpr float REFRESH_PERIOD_MS = 5000.0f;

static constexpr int IMAGERES_NETWORK_CONNECTED    = 25;
static constexpr int IMAGERES_NETWORK_DISCONNECTED = 26;
static constexpr int IMAGERES_BATTERY_FULL         = 100;
static constexpr int IMAGERES_BATTERY_MEDIUM       = 101;
static constexpr int IMAGERES_BATTERY_LOW          = 102;
static constexpr int IMAGERES_BATTERY_CHARGING     = 103;
static constexpr int IMAGERES_BATTERY_NONE         = 104;

// ---------------------------------------------------------------------------
// DUI class boilerplate
// ---------------------------------------------------------------------------

DirectUI::IClassInfo* CDUIStatusIcon::Class = nullptr;

CDUIStatusIcon::~CDUIStatusIcon()
{
    DirectUI::Element::~Element();
}

DirectUI::IClassInfo* CDUIStatusIcon::GetClassInfoW()   { return Class; }
DirectUI::IClassInfo* CDUIStatusIcon::GetClassInfoPtr() { return Class; }

HRESULT CDUIStatusIcon::Create(DirectUI::Element*  pParent,
                                unsigned long*      pdwDeferCookie,
                                DirectUI::Element** ppElement)
{
    return DirectUI::CreateAndInit<CDUIStatusIcon, int>(
        0, pParent, pdwDeferCookie, ppElement);
}

// ---------------------------------------------------------------------------
// Custom property: providerId
// ---------------------------------------------------------------------------

static DirectUI::PropertyInfoData dataProviderIdProp;
static const DirectUI::PropertyInfo impProviderIdProp =
{
    L"providerId",
    0xA,
    0,
    nullptr,
    DirectUI::Value::GetNull,
    &dataProviderIdProp
};

const DirectUI::PropertyInfo* CDUIStatusIcon::ProviderIdProp()
{
    return &impProviderIdProp;
}

HRESULT CDUIStatusIcon::SetProviderId(const wchar_t* pszId)
{
    if (!pszId) return E_INVALIDARG;
    m_isNetwork = (wcscmp(pszId, L"networkstatus") == 0);
    DirectUI::Value* pv = DirectUI::Value::CreateString(pszId, nullptr);
    if (!pv) return E_OUTOFMEMORY;
    HRESULT hr = SetValue(ProviderIdProp(), 0, pv);
    pv->Release();
    return hr;
}

HRESULT CDUIStatusIcon::Register()
{
    static const DirectUI::PropertyInfo* const s_rgProps[] = { &impProviderIdProp };
    return DirectUI::ClassInfo<CDUIStatusIcon, DirectUI::Element>
        ::RegisterGlobal(HINST_THISCOMPONENT, L"statusicon",
                         s_rgProps, ARRAYSIZE(s_rgProps));
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CDUIStatusIcon::OnHosted(DirectUI::Element* peNewRoot)
{
    DirectUI::Element::OnHosted(peNewRoot);

    DirectUI::Value* pv = GetValue(ProviderIdProp(), 0, nullptr);
    if (pv)
    {
        const wchar_t* pszId = pv->GetString();
        if (pszId)
            m_isNetwork = (wcscmp(pszId, L"networkstatus") == 0);
        pv->Release();
    }

    _Refresh();

    GMA_ACTION ga = {};
    ga.cbSize     = sizeof(ga);
    ga.flPeriod   = REFRESH_PERIOD_MS;
    ga.cRepeat    = -1;
    ga.pvData     = this;
    ga.pfnProc    = _ActionProc;
    m_hAction     = CreateAction(&ga);
}

void CDUIStatusIcon::OnDestroy()
{
    if (m_hAction)
    {
        DeleteHandle(m_hAction);
        m_hAction = nullptr;
    }
    DirectUI::Element::OnDestroy();
}

// static
void CALLBACK CDUIStatusIcon::_ActionProc(GMA_ACTIONINFO* pInfo)
{
    if (!pInfo->fFinished)
        reinterpret_cast<CDUIStatusIcon*>(pInfo->pvData)->_Refresh();
}

// ---------------------------------------------------------------------------
// Icon helpers
// ---------------------------------------------------------------------------

HRESULT CDUIStatusIcon::_SetIconFromImageRes(int iconIndex, int sizePixels)
{
    static HMODULE s_hImageRes = nullptr;
    if (!s_hImageRes)
    {
        WCHAR szPath[MAX_PATH];
        GetSystemDirectoryW(szPath, ARRAYSIZE(szPath));
        wcsncat_s(szPath, L"\\imageres.dll", _TRUNCATE);
        s_hImageRes = LoadLibraryExW(szPath, nullptr,
            LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    }
    if (!s_hImageRes) return HRESULT_FROM_WIN32(GetLastError());

    HICON hIcon = reinterpret_cast<HICON>(
        LoadImageW(s_hImageRes, MAKEINTRESOURCEW(iconIndex),
                   IMAGE_ICON, sizePixels, sizePixels, LR_DEFAULTCOLOR));
    if (!hIcon) return HRESULT_FROM_WIN32(GetLastError());

    HDC     hdcScreen = GetDC(nullptr);
    HDC     hdcMem    = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp      = nullptr;

    BITMAPINFO bmi              = {};
    bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth       = sizePixels;
    bmi.bmiHeader.biHeight      = -sizePixels;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (hBmp && pBits)
    {
        HBITMAP hOld = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));
        DrawIconEx(hdcMem, 0, 0, hIcon, sizePixels, sizePixels,
                   0, nullptr, DI_NORMAL);
        SelectObject(hdcMem, hOld);
    }

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    DestroyIcon(hIcon);

    if (!hBmp) return E_OUTOFMEMORY;

    HRESULT hr = SetBackgroundFromHBITMAP(this, hBmp, sizePixels, sizePixels);
    DeleteObject(hBmp);
    return hr;
}

void CDUIStatusIcon::_RefreshNetwork()
{
    bool connected = false;

    // Load GetNetworkConnectivityHint dynamically to avoid header conflicts
    static auto pfnHint = reinterpret_cast<PFN_GetNetworkConnectivityHint>(
        GetProcAddress(GetModuleHandleW(L"iphlpapi.dll"),
                       "GetNetworkConnectivityHint"));

    if (pfnHint)
    {
        NL_NETWORK_CONNECTIVITY_HINT hint = {};
        if (pfnHint(&hint) == 0 /* NO_ERROR */)
            connected = (hint.ConnectivityLevel >=
                         NetworkConnectivityLevelHintLocalAccess);
    }
    else
    {
        // Fallback: check if any network interface is up
        DWORD dwForwardEntries = 0;
        connected = (GetNumberOfInterfaces(&dwForwardEntries) == NO_ERROR &&
                     dwForwardEntries > 1);
    }

    _SetIconFromImageRes(
        connected ? IMAGERES_NETWORK_CONNECTED : IMAGERES_NETWORK_DISCONNECTED,
        DirectUI::RelPixToPixel(ICON_SIZE_RP));
}

void CDUIStatusIcon::_RefreshBattery()
{
    SYSTEM_POWER_STATUS ps = {};
    GetSystemPowerStatus(&ps);

    int idx;
    if (ps.BatteryFlag == 255)            idx = IMAGERES_BATTERY_NONE;
    else if (ps.ACLineStatus == 1)        idx = IMAGERES_BATTERY_CHARGING;
    else if (ps.BatteryLifePercent >= 66) idx = IMAGERES_BATTERY_FULL;
    else if (ps.BatteryLifePercent >= 33) idx = IMAGERES_BATTERY_MEDIUM;
    else                                  idx = IMAGERES_BATTERY_LOW;

    _SetIconFromImageRes(idx, DirectUI::RelPixToPixel(ICON_SIZE_RP));
}

void CDUIStatusIcon::_Refresh()
{
    if (m_isNetwork) _RefreshNetwork();
    else             _RefreshBattery();
}
