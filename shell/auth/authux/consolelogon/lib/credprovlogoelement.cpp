#include "pch.h"
#include "credprovlogoelement.h"
#include "duiutil.h"
#include "wicutil.h"

HRESULT GetBitmapFromRandomStream(
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStream> stream,
    HBITMAP* outBitmap);

static constexpr int LOGO_SIZE_RP = 40;

// ---------------------------------------------------------------------------
// DUI class boilerplate
// ---------------------------------------------------------------------------

DirectUI::IClassInfo* CDUICredProvLogoElement::Class = nullptr;

CDUICredProvLogoElement::~CDUICredProvLogoElement()
{
    DirectUI::Button::~Button();
}

DirectUI::IClassInfo* CDUICredProvLogoElement::GetClassInfoW()  { return Class; }
DirectUI::IClassInfo* CDUICredProvLogoElement::GetClassInfoPtr() { return Class; }

HRESULT CDUICredProvLogoElement::Create(DirectUI::Element*  pParent,
                                         unsigned long*      pdwDeferCookie,
                                         DirectUI::Element** ppElement)
{
    return DirectUI::CreateAndInit<CDUICredProvLogoElement, int>(
        3, pParent, pdwDeferCookie, ppElement);
}

HRESULT CDUICredProvLogoElement::Register()
{
    return DirectUI::ClassInfo<CDUICredProvLogoElement, DirectUI::Button>
        ::RegisterGlobal(HINST_THISCOMPONENT,
                         L"CredProvLogoElement",
                         nullptr, 0);
}

// ---------------------------------------------------------------------------
// Logo loading
// ---------------------------------------------------------------------------

HRESULT CDUICredProvLogoElement::SetCredentialProvider(
    const Microsoft::WRL::ComPtr<LCPD::ICredential>& credential)
{
    m_credential = credential;
    return _LoadLogo();
}

HRESULT CDUICredProvLogoElement::_LoadLogo()
{
    if (!m_credential)
        return S_OK;

    int sizePixels = DirectUI::RelPixToPixel(LOGO_SIZE_RP);
    HBITMAP hBmp   = nullptr;

    // Try to get the credential provider logo bitmap via the image field.
    // The logo field is typically CredentialFieldKind_TileImage with
    // IsLogoImageHidden == FALSE.
    Microsoft::WRL::ComPtr<LCPD::ICredentialImageField> imgField;
    if (SUCCEEDED(m_credential->QueryInterface(IID_PPV_ARGS(&imgField))))
    {
        Microsoft::WRL::ComPtr<
            ABI::Windows::Storage::Streams::IRandomAccessStream> stream;
        if (SUCCEEDED(imgField->get_BitmapStream(&stream)) && stream)
            GetBitmapFromRandomStream(stream, &hBmp);
    }

    if (!hBmp)
        return S_OK;  // no logo available – leave blank

    HRESULT hr = SetBackgroundFromHBITMAP(this, hBmp, sizePixels, sizePixels);
    DeleteObject(hBmp);
    return hr;
}

// ---------------------------------------------------------------------------
// Click: switch to this credential provider's credential entry view.
// The actual view switch is handled by the parent UserList / LogonFrame via
// standard DUI event bubbling – we just need to ensure the button fires
// the standard Click event (which Button::OnEvent already does).
// ---------------------------------------------------------------------------

void CDUICredProvLogoElement::OnEvent(DirectUI::Event* pEvent)
{
    DirectUI::Button::OnEvent(pEvent);
}
