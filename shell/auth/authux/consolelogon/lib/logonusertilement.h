#pragma once

#include "pch.h"
#include "DirectUI/DirectUI.h"
#include "usertileelement.h"

// Replicates authui.dll (build 7878) "LogonUsertileElement".
//
// In the Win8 beta metro logon, this is the deselected-state tile:
// a 205x205 square with a 199x199 white-bordered avatar photo, the user's
// display name below it, and an animated press-scale effect (199->205 px).
//
// When clicked it triggers the normal credential-entry flow that
// CDUIUserTileElement already handles (zoom/select).  We therefore keep a
// back-pointer to the parent UserList and delegate selection to it.
//
// UIFILE template reference:
//   <LogonUsertileElement resid="LogonUserTileTemplate" width="205" ...>
//     <element width="210" height="210" layout="flowlayout(0,2,2,2)">
//       <button id="atom(TileImage)" sheet="LogonUserTileStyle" ... />
//     </element>
//     <element id="atom(Subtitle)" ...>
//       <element id="atom(DisplayName)" ... />
//       <element id="atom(LogonStatus)" layoutpos="none" ... />
//     </element>
//   </LogonUsertileElement>

class UserList;

class CDUILogonUsertileElement : public DirectUI::Button
{
public:
    ~CDUILogonUsertileElement() override;

    static DirectUI::IClassInfo* Class;
    DirectUI::IClassInfo* GetClassInfoW()  override;
    static DirectUI::IClassInfo* GetClassInfoPtr();

    static HRESULT Create(DirectUI::Element*  pParent,
                          unsigned long*      pdwDeferCookie,
                          DirectUI::Element** ppElement);

    static HRESULT Register();

    // Called by UserList after creation to supply user data.
    HRESULT Configure(
        const Microsoft::WRL::ComPtr<LCPD::IUser>&       user,
        const Microsoft::WRL::ComPtr<LCPD::ICredential>& credential,
        DirectUI::DUIXmlParser*                          pParser);

    void OnEvent(DirectUI::Event* pEvent) override;

    UserList*                                m_owningUserList  = nullptr;
    Microsoft::WRL::ComPtr<LCPD::IUser>      m_dataSourceUser;
    Microsoft::WRL::ComPtr<LCPD::ICredential> m_dataSourceCredential;

private:
    HRESULT _LoadUserPicture();
    HRESULT _LoadDisplayName();
};
