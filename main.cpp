#include "main.h"

typedef HRESULT(APIENTRY *SetStreamSource_t)(IDirect3DDevice9*, UINT, IDirect3DVertexBuffer9*, UINT, UINT);
SetStreamSource_t SetStreamSource_orig = 0;

typedef HRESULT(APIENTRY *DrawIndexedPrimitive_t)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DrawIndexedPrimitive_t DrawIndexedPrimitive_orig = 0;

typedef HRESULT(APIENTRY* Present_t) (IDirect3DDevice9*, const RECT *, const RECT *, HWND, const RGNDATA *);
Present_t Present_orig = 0;

typedef HRESULT(APIENTRY *Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
Reset_t Reset_orig = 0;

//==========================================================================================================================

HRESULT APIENTRY SetStreamSource_hook(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT sStride)
{
	if (StreamNumber == 0)
		Stride = sStride;

	return SetStreamSource_orig(pDevice, StreamNumber, pStreamData, OffsetInBytes, sStride);
}

//==========================================================================================================================

HRESULT APIENTRY DrawIndexedPrimitive_hook(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	//models
	//Stride == 40
	//bullet
	//Stride == 48 && NumVertices == 2005 && vSize == 840
	//outline
	//Stride == 48 && NumVertices == 220 && vSize == 1000
	//weapon
	//Stride == 48 && NumVertices == 433 && vSize == 1356
	//snowboard
	//Stride == 40 && NumVertices == 185 && vSize == 2740

	//get vSize
	if (SUCCEEDED(pDevice->GetVertexShader(&vShader)))
		if (vShader != NULL)
			if (SUCCEEDED(vShader->GetFunction(NULL, &vSize)))
				if (vShader != NULL) { vShader->Release(); vShader = NULL; }

	if (aimbot==1 || distanceesp == 1 || lineesp > 0 || boxesp == 1)
	//if (NumVertices != 185) //snowboard
	if (NumVertices > 1160)//treasure chest
	if (Stride == 40 && vSize > 2500) //compatibility
	//if (Stride == 24 && vSize == 320) //freeze with distance esp
	//if (vSize == 2884 || vSize == 3556) //2884near, 3556far
	{
		AddModels(pDevice, 189);
		AddModels(pDevice, 180);
	}

	if (wallhack==1)
	if ((Stride == 40) || (Stride == 48 && vSize == 840) || (Stride == 48 && vSize == 1000) || (Stride == 48 && vSize == 1356))
	{
		float bias = 1000.0f;
		float bias_float = static_cast<float>(-bias);
		bias_float /= 2000;//10000.0f;
		pDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias_float);
		DrawIndexedPrimitive_orig(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		pDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
	}

	/*
	//hold down P key until a texture disappears, press END to log values of those textures
	if (GetAsyncKeyState('O') & 1) //-
		countnum--;
	if (GetAsyncKeyState('P') & 1) //+
		countnum++;
	if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('9') & 1) //reset, set to -1
		countnum = -1;

	if (countnum == vSize / 100)
		if (GetAsyncKeyState(VK_END) & 1) //log
			Log("Stride == %d && NumVertices == %d && vSize == %d && pSize == %d numElements == %d && decl->Type == %d && mStartregister == %d && mVectorCount == %d",
				Stride, NumVertices, vSize, pSize, numElements, decl->Type, mStartregister, mVectorCount);

	if (countnum == vSize / 100)
		return D3D_OK;
	*/
	
	return DrawIndexedPrimitive_orig(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

//==========================================================================================================================

HRESULT APIENTRY Present_hook(IDirect3DDevice9* pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
	if (InitOnce)
	{
		InitOnce = false;

		//generate texture
		//GenerateTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 255, 0, 0));
		//GenerateTexture(pDevice, &Green, D3DCOLOR_RGBA(0, 255, 0, 255));
		//GenerateTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		//GenerateTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));

		//load settings
		LoadCfg();
	}


	//get viewport
	pDevice->GetViewport(&Viewport);
	ScreenCX = (float)Viewport.Width / 2.0f;
	ScreenCY = (float)Viewport.Height / 2.0f;

	//create font
	if (Font == NULL)
		D3DXCreateFont(pDevice, 14, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Italic"), &Font);

	//create dxLine
	if (!pLine)
		D3DXCreateLine(pDevice, &pLine);

	//draw background
	if (ShowMenu)
		DrawBox(pDevice, 71.0f, 86.0f, 200.0f, 275.0f, D3DCOLOR_ARGB(120, 30, 200, 200));//200 = left/right, 275 = up/down

	//draw menu
	if (Font)
		DrawMenu(pDevice);

	//Shift|RMouse|LMouse|Ctrl|Alt|Space|X|C
	if (aimkey == 0) Daimkey = 0;
	if (aimkey == 1) Daimkey = VK_SHIFT;
	if (aimkey == 2) Daimkey = VK_RBUTTON;
	if (aimkey == 3) Daimkey = VK_LBUTTON;
	if (aimkey == 4) Daimkey = VK_CONTROL;
	if (aimkey == 5) Daimkey = VK_MENU;
	if (aimkey == 6) Daimkey = VK_SPACE;
	if (aimkey == 7) Daimkey = 0x58; //X
	if (aimkey == 8) Daimkey = 0x43; //C

	//do distance esp
	if (distanceesp > 0 && WeaponEspInfo.size() != NULL)
	{
		for (unsigned int i = 0; i < WeaponEspInfo.size(); i++)
		{
			if (WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f && (float)WeaponEspInfo[i].RealDistance <= 200.0f) // 12 - 200 yellow
				DrawCenteredString(Font, (int)WeaponEspInfo[i].pOutX, (int)WeaponEspInfo[i].pOutY - 20, D3DCOLOR_ARGB(255, 255, 255, 0), "%.f", (float)WeaponEspInfo[i].RealDistance);
			else if (WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 200.0f && (float)WeaponEspInfo[i].RealDistance <= 1000.0f) //200 - 1000 white
				DrawCenteredString(Font, (int)WeaponEspInfo[i].pOutX, (int)WeaponEspInfo[i].pOutY - 20, D3DCOLOR_ARGB(255, 255, 255, 255), "%.f", (float)WeaponEspInfo[i].RealDistance);
			else if (WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 1000.0f) //> 1000 gray
				DrawCenteredString(Font, (int)WeaponEspInfo[i].pOutX, (int)WeaponEspInfo[i].pOutY - 20, D3DCOLOR_ARGB(255, 128, 128, 128), "%.f", (float)WeaponEspInfo[i].RealDistance);
		}
	}

	//do line esp
	if (lineesp > 0 && WeaponEspInfo.size() != NULL)
	{
		for (unsigned int i = 0; i < WeaponEspInfo.size(); i++)
		{
			if (lineesp == 2 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 0.2f, 1, D3DCOLOR_ARGB(255, 255, 255, 255), 0);//0.1up, 1.0middle, 2.0down
				//DrawLine2(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * ((float)esp * 0.2f), 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255));

			else if (lineesp == 3 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 0.4f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 4 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 0.6f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 5 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 0.8f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 6 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 1.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 7 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 1.2f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 8 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 1.4f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 9 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 1.6f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 10 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 1.8f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);

			else if (lineesp == 11 && WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawLine(pDevice, (float)WeaponEspInfo[i].pOutX, (float)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 2.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0);
		}
	}

	//do box esp
	if (boxesp > 0 && WeaponEspInfo.size() != NULL)
	{
		for (unsigned int i = 0; i < WeaponEspInfo.size(); i++)
		{
			//box esp
			if (WeaponEspInfo[i].pOutX > 1.0f && WeaponEspInfo[i].pOutY > 1.0f && (float)WeaponEspInfo[i].RealDistance > 12.0f)
				DrawCornerBox(pDevice, (int)WeaponEspInfo[i].pOutX + 2, (int)WeaponEspInfo[i].pOutY + 2 + 20, 20, 30, 1, D3DCOLOR_ARGB(255, 255, 255, 255));
		}
	}


	//do aim
	if (aimbot == 1 && WeaponEspInfo.size() != NULL)
	{
		UINT BestTarget = -1;
		DOUBLE fClosestPos = 99999;

		for (unsigned int i = 0; i < WeaponEspInfo.size(); i++)
		{
			//aimfov
			float radiusx = (aimfov*5.0f) * (ScreenCX / 100.0f);
			float radiusy = (aimfov*5.0f) * (ScreenCY / 100.0f);

			if (aimfov == 0)
			{
				radiusx = 5.0f * (ScreenCX / 100.0f);
				radiusy = 5.0f * (ScreenCY / 100.0f);
			}

			//get crosshairdistance
			WeaponEspInfo[i].CrosshairDistance = GetDistance(WeaponEspInfo[i].pOutX, WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY);

			//if in fov
			if (WeaponEspInfo[i].pOutX >= ScreenCX - radiusx && WeaponEspInfo[i].pOutX <= ScreenCX + radiusx && WeaponEspInfo[i].pOutY >= ScreenCY - radiusy && WeaponEspInfo[i].pOutY <= ScreenCY + radiusy)

				//get closest/nearest target to crosshair
				if (WeaponEspInfo[i].CrosshairDistance < fClosestPos)
				{
					fClosestPos = WeaponEspInfo[i].CrosshairDistance;
					BestTarget = i;
				}
		}


		//if nearest target to crosshair
		if (BestTarget != -1 && WeaponEspInfo[BestTarget].RealDistance > 12.0f)//do not aim at self
		{
			double DistX = WeaponEspInfo[BestTarget].pOutX - ScreenCX;
			double DistY = WeaponEspInfo[BestTarget].pOutY - ScreenCY;

			DistX /= (float)aimsens*0.5f;
			DistY /= (float)aimsens*0.5f;

			//aim
			if (GetAsyncKeyState(Daimkey) & 0x8000)
				mouse_event(MOUSEEVENTF_MOVE, (float)DistX, (float)DistY, 0, NULL);

			//autoshoot on
			if ((!GetAsyncKeyState(VK_LBUTTON) && (autoshoot == 1) && (GetAsyncKeyState(Daimkey) & 0x8000))) //
			{
				if (autoshoot == 1 && !IsPressed)
				{
					IsPressed = true;
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				}
			}
		}
	}
	WeaponEspInfo.clear();

	//autoshoot off
	if (autoshoot == 1 && IsPressed)
	{
		if (timeGetTime() - astime >= asdelay)
		{
			IsPressed = false;
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			astime = timeGetTime();
		}
	}

	/*
	//draw logger
	if (Font && countnum != 0)
	{
	char szString[255];
	sprintf_s(szString, "countnum = %d", countnum);
	DrawString(Font, 319, 99, D3DCOLOR_ARGB(255, 0, 0, 0), (char*)&szString[0]);
	DrawString(Font, 321, 101, D3DCOLOR_ARGB(255, 0, 0, 0), (char*)&szString[0]);
	DrawString(Font, 320, 100, D3DCOLOR_ARGB(255, 255, 255, 255), (char*)&szString[0]);
	DrawString(Font, 320, 110, D3DCOLOR_ARGB(255, 255, 255, 255), (PCHAR)"hold P to +");
	DrawString(Font, 320, 120, D3DCOLOR_ARGB(255, 255, 255, 255), (PCHAR)"hold O to -");
	}
	*/
	return Present_orig(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

//==========================================================================================================================

HRESULT APIENTRY Reset_hook(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	if (Font)
		Font->OnLostDevice();

	if (pLine)
		pLine->OnLostDevice();

	HRESULT ResetReturn = Reset_orig(pDevice, pPresentationParameters);

	if (SUCCEEDED(ResetReturn))
	{
		if (Font)
			Font->OnResetDevice();

		if (pLine)
			pLine->OnResetDevice();

		InitOnce = true;
	}

	return ResetReturn;
}

//==========================================================================================================================

DWORD WINAPI dRosD3D(LPVOID lpParameter)
{
	HMODULE dDll = NULL;
	while (!dDll)
	{
		dDll = GetModuleHandleA("d3d9.dll");
		Sleep(100);
	}
	CloseHandle(dDll);

	IDirect3D9* d3d = NULL;
	IDirect3DDevice9* d3ddev = NULL;

	HWND tmpWnd = CreateWindowA("BUTTON", "CD", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, Hand, NULL);
	if (tmpWnd == NULL)
	{
		//Log("[DirectX] Failed to create temp window");
		return 0;
	}

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == NULL)
	{
		DestroyWindow(tmpWnd);
		//Log("[DirectX] Failed to create temp Direct3D interface");
		return 0;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = tmpWnd;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
	if (result != D3D_OK)
	{
		d3d->Release();
		DestroyWindow(tmpWnd);
		MessageBox(0, L"Run the game first and inject dll later", L"Failed to create temp Direct3D device.", MB_OK);
		//MessageBox(0, "Run the game first and inject dll later", "Failed to create temp Direct3D device.", MB_OK);
		//Log("[DirectX] Failed to create temp Direct3D device");
		return 0;
	}

	// We have the device, so walk the vtable to get the address of all the dx functions in d3d9.dll
#if defined _M_X64
	DWORD64* dVtable = (DWORD64*)d3ddev;
	dVtable = (DWORD64*)dVtable[0];
#elif defined _M_IX86
	DWORD* dVtable = (DWORD*)d3ddev;
	dVtable = (DWORD*)dVtable[0]; // == *d3ddev
#endif
	//Log("[DirectX] dVtable: %x", dVtable);

	//for(int i = 0; i < 95; i++)
	//{
			//Log("[DirectX] vtable[%i]: %x, pointer at %x", i, dVtable[i], &dVtable[i]);
	//}

	SetStreamSource_orig = (SetStreamSource_t)dVtable[100];
	DrawIndexedPrimitive_orig = (DrawIndexedPrimitive_t)dVtable[82];
	Present_orig = (Present_t)dVtable[17];
	Reset_orig = (Reset_t)dVtable[16];

	// Detour functions
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)SetStreamSource_orig, (PBYTE)SetStreamSource_hook);
	DetourAttach(&(LPVOID&)DrawIndexedPrimitive_orig, (PBYTE)DrawIndexedPrimitive_hook);
	DetourAttach(&(LPVOID&)Present_orig, (PBYTE)Present_hook);
	DetourAttach(&(LPVOID&)Reset_orig, (PBYTE)Reset_hook);
	DetourTransactionCommit();
	
	//Log("[Detours] Detours attached\n");

	d3ddev->Release();
	d3d->Release();
	DestroyWindow(tmpWnd);

	return 1;
}

//==========================================================================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Hand = hModule;
		DisableThreadLibraryCalls(hModule); //disable unwanted thread notifications to reduce overhead
		GetModuleFileNameA(hModule, dlldir, 512);
		for (int i = (int)strlen(dlldir); i > 0; i--)
		{
			if (dlldir[i] == '\\')
			{
				dlldir[i + 1] = 0;
				break;
			}
		}
		CreateThread(0, 0, dRosD3D, 0, 0, 0); //init our hooks
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
