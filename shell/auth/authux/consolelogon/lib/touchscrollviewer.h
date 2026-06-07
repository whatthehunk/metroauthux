#pragma once

#include "pch.h"
#include "DirectUI/DirectUI.h"
#include "DirectUI/Control/TouchScrollViewer.h"

// Thin wrapper around dui70's DirectUI::TouchScrollViewer.
// dui70 already provides full DirectManipulation-backed kinetic scrolling
// with the exact 0.85x inertia decay.  We only need to register our own
// class name ("TouchScrollViewer") so the UIFILE parser finds it, and
// expose the DUI class boilerplate AuthUX expects.

class CDUITouchScrollViewer : public DirectUI::TouchScrollViewer
{
public:
    ~CDUITouchScrollViewer() override;

    static DirectUI::IClassInfo* Class;
    DirectUI::IClassInfo* GetClassInfoW()  override;
    static DirectUI::IClassInfo* GetClassInfoPtr();

    static HRESULT Create(DirectUI::Element*  pParent,
                          unsigned long*      pdwDeferCookie,
                          DirectUI::Element** ppElement);

    static HRESULT Register();
};
