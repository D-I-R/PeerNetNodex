
// PeerNetN.h: главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "включить pch.h до включения этого файла в PCH"
#endif

#include "resource.h"		// основные символы


// CPeerNetNApp:
// Сведения о реализации этого класса: PeerNetN.cpp
//

class CPeerNetNApp : public CWinApp
{
public:
	CPeerNetNApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern CPeerNetNApp theApp;
