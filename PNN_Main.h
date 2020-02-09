//PNN_Main.h                                               (c) DIR, 2019
//
//      Main header file for the PeerNetNode application
//
////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
 #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CPeerNetNodeApp:
// See PeerNetNode.cpp for the implementation of this class
//

class CPeerNetNodeApp : public CWinAppEx
{
public:
  CPeerNetNodeApp();

// Overrides
public:
  virtual BOOL  InitInstance();

// Implementation
  DECLARE_MESSAGE_MAP()
};

extern CPeerNetNodeApp  theApp;
