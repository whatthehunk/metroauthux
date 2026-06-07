#include "pch.h"
#include "logonusertilement.h"

#include "duiutil.h"
#include "logonguids.h"
#include "userlist.h"
#include "wicutil.h"

// Forward declarations for helpers defined in usertileelement.cpp
HRESULT GetBitmapFromRandomStream(
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStream> stream,
    HBITMAP* outBitmap);
HRESULT GetBitmapFromUserSID(CoTaskMemNativeString& SID, HBITMAP* outBitmap);

// Tile image dimensions matching LogonUserTileStyle in the UIFILE
static constexpr int TILE_SIZE_RP    = 199;  // inner picture button
static constexpr int TILE_PRESS_RP   = 205;  // pressed size

// ---------------------------------------------------------------------------
// DUI class boilerplate
// ---------------------------------------------------------------------------

DirectUI::IClassInfo* CDUILogonUsertileElement::Class = nullptr;

CDUILogonUsertileElement::~CDUILogonUsertileElement()
{
    DirectUI::Button::~Button();
}

DirectUI::IClassInfo* CDUILogonUsertileElement::GetClassInfoW()  { return Class; }
DirectUI::IClassInfo* CDUILogonUsertileElement::GetClassInfoPtr() { return Class; }

HRESULT CDUILogonUsertileElement::Create(DirectUI::Element*  pParent,
                                          unsigned long*      pdwDeferCookie,
                                          DirectUI::Element** ppElement)
{
    return DirectUI::CreateAndInit<CDUILogonUsertileElement, int>(
        3, pParent, pdwDeferCookie, ppElement);
}

HRESULT CDUILogonUsertileElement::Register()
{
    return DirectUI::ClassInfo<CDUILogonUsertileElement, DirectUI::Button>
        ::RegisterGlobal(HINST_THISCOMPONENT,
                         L"LogonUsertileElement",
                         nullptr, 0);
}

// ---------------------------------------------------------------------------
// Configuration – called by UserList after the UIFILE template is inflated
// ---------------------------------------------------------------------------

HRESULT CDUILogonUsertileElement::Configure(
    const Microsoft::WRL::ComPtr<LCPD::IUser>&       user,
    const Microsoft::WRL::ComPtr<LCPD::ICredential>& credential,
    DirectUI::DUIXmlParser* /*pParser*/)
{
    m_dataSourceUser       = user;
    m_dataSourceCredential = credential;

    RETURN_IF_FAILED(_LoadDisplayName());
    RETURN_IF_FAILED(_LoadUserPicture());

    return S_OK;
}

// ---------------------------------------------------------------------------
// User picture loading
// ---------------------------------------------------------------------------

HRESULT CDUILogonUsertileElement::_LoadUserPicture()
{
    // The TileImage button is a direct child inside the template element.
    DirectUI::Element* pTileImage =
        FindDescendent(DirectUI::StrToID(L"TileImage"));
    if (!pTileImage)
        return S_OK;  // UIFILE may not have inflated yet – harmless

    int sizePixels = DirectUI::RelPixToPixel(TILE_SIZE_RP);
    HBITMAP hBmp   = nullptr;

    // 1. Try to get the user's account picture via IUserTileStore
    if (m_dataSourceUser)
    {
        Microsoft::WRL::Wrappers::HString sid;
        if (SUCCEEDED(m_dataSourceUser->get_Sid(sid.ReleaseAndGetAddressOf())))
        {
            CoTaskMemNativeString nativeSID;
            nativeSID.Initialize(sid.GetRawBuffer(nullptr));

            Microsoft::WRL::ComPtr<IUserTileStore> tileStore;
            if (SUCCEEDED(CoCreateInstance(CLSID_UserTileStore, nullptr,
                                           CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARGS(&tileStore))))
            {
                // Ignore failure; fall through to default PFP below
                tileStore->GetLargePicture(nativeSID.Get(), &hBmp);
            }
        }
    }

    // 2. Try the credential's tile image stream
    if (!hBmp && m_dataSourceCredential)
    {
        Microsoft::WRL::ComPtr<LCPD::ICredentialImageField> imgField;
        if (SUCCEEDED(m_dataSourceCredential->QueryInterface(
                IID_PPV_ARGS(&imgField))))
        {
            Microsoft::WRL::ComPtr<
                ABI::Windows::Storage::Streams::IRandomAccessStream> stream;
            if (SUCCEEDED(imgField->get_BitmapStream(&stream)) && stream)
            {
                // Reuse existing WIC helper from usertileelement.cpp
                GetBitmapFromRandomStream(stream, &hBmp);
            }
        }
    }

    // 3. Fall back to the built-in default profile picture
    if (!hBmp)
        hBmp = LoadBitmapW(HINST_THISCOMPONENT,
                           MAKEINTRESOURCEW(IDB_DEFAULTPFP));

    if (!hBmp)
        return S_OK;  // no picture at all – leave background empty

    HRESULT hr = SetBackgroundFromHBITMAP(pTileImage, hBmp,
                                          sizePixels, sizePixels);
    DeleteObject(hBmp);
    return hr;
}

// ---------------------------------------------------------------------------
// Display name
// ---------------------------------------------------------------------------

HRESULT CDUILogonUsertileElement::_LoadDisplayName()
{
    DirectUI::Element* pName =
        FindDescendent(DirectUI::StrToID(L"DisplayName"));
    if (!pName)
        return S_OK;

    if (m_dataSourceUser)
    {
        Microsoft::WRL::Wrappers::HString displayName;
        RETURN_IF_FAILED(
            m_dataSourceUser->get_DisplayName(
                displayName.ReleaseAndGetAddressOf()));

        const wchar_t* pszName = displayName.GetRawBuffer(nullptr);
        if (pszName && *pszName)
            RETURN_IF_FAILED(pName->SetContentString(pszName));
    }

    return S_OK;
}

// ---------------------------------------------------------------------------
// Click handling – delegate to UserList to zoom/select the matching UserTile
// ---------------------------------------------------------------------------

void CDUILogonUsertileElement::OnEvent(DirectUI::Event* pEvent)
{
    if (pEvent->uidType   == DirectUI::Button::Click() &&
        pEvent->nStage    == DirectUI::GMF_DIRECT &&
        pEvent->peTarget  == this)
    {
        // Find the CDUIUserTileElement inside the UserList that corresponds
        // to our credential and tell the list to zoom it.
        if (m_owningUserList && m_dataSourceCredential)
        {
            CDUIUserTileElement* pTile =
                m_owningUserList->FindTileByCredential(m_dataSourceCredential);
            if (pTile)
                m_owningUserList->ZoomTile(pTile);
        }
    }

    DirectUI::Button::OnEvent(pEvent);
}
