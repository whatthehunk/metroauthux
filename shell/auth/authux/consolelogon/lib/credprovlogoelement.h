#pragma once

#include "pch.h"
#include "DirectUI/DirectUI.h"

// Replicates authui.dll (build 7878) "CredProvLogoElement".
//
// Displays the active credential provider's small logo as a 40x40 button.
// When selected (i.e. when its owning credential provider is the active one)
// the LogonUserTileStyle stylesheet enlarges it to 45x45 via the pressed/
// selected pseudo-class – no extra code needed for that, the UIFILE handles it.
//
// UIFILE reference:
//   <CredProvLogoElement resid="CredProvButton" sheet="CredProvLogoStyle"
//       tooltip="true" borderthickness="rect(1,1,1,1)"
//       bordercolor="rgb(255,255,255)"
//       margin="rect(0rp,0rp,5rp,0rp)" accessible="true"/>
//
// <style resid="CredProvLogoStyle">
//   <CredProvLogoElement width="40" height="40"/>
//   <if selected="false"><CredProvLogoElement width="40" height="40"/></if>
//   <if selected="true"> <CredProvLogoElement width="45" height="45"/></if>
// </style>

class CDUICredProvLogoElement : public DirectUI::Button
{
public:
    ~CDUICredProvLogoElement() override;

    static DirectUI::IClassInfo* Class;
    DirectUI::IClassInfo* GetClassInfoW()  override;
    static DirectUI::IClassInfo* GetClassInfoPtr();

    static HRESULT Create(DirectUI::Element*  pParent,
                          unsigned long*      pdwDeferCookie,
                          DirectUI::Element** ppElement);

    static HRESULT Register();

    // Supply the credential provider whose logo we should show.
    // Called by the logon frame after credential providers are loaded.
    HRESULT SetCredentialProvider(
        const Microsoft::WRL::ComPtr<LCPD::ICredential>& credential);

    void OnEvent(DirectUI::Event* pEvent) override;

private:
    HRESULT _LoadLogo();

    Microsoft::WRL::ComPtr<LCPD::ICredential> m_credential;
};
