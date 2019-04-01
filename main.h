#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "winmm.lib")//time

//detours
#include "detours.X86/detours.h"
#if defined _M_X64
#pragma comment(lib, "detours.X64/detours.lib")
#elif defined _M_IX86
#pragma comment(lib, "detours.X86/detours.lib")
#endif

//dx sdk dir
#include "DXSDK\d3dx9.h"
#if defined _M_X64
#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
#elif defined _M_IX86
#pragma comment(lib, "DXSDK/x86/d3dx9.lib")
#endif

using namespace std;

#pragma warning (disable: 4244) 
#define _CRT_SECURE_NO_DEPRECATE

//==========================================================================================================================

HMODULE Hand;
D3DVIEWPORT9 Viewport;
float ScreenCX;
float ScreenCY;
LPD3DXFONT Font;
UINT Stride;
IDirect3DVertexShader9* vShader;
UINT vSize;
bool InitOnce = true;
int countnum = -1;

//==========================================================================================================================

//features

//group states
int	esp_group = 1;
int	aim_group = 1;
int	misc_group = 1;

//item states
int wallhack = 1;				//wallhack
int distanceesp = 1;			//distance esp
int lineesp = 10;				//line esp
int boxesp = 1;					//box esp

//aimbot settings
int aimbot = 1;
int aimkey = 2;
DWORD Daimkey = VK_RBUTTON;		//aimkey
int aimsens = 2;				//aim sensitivity, makes aim smoother
int aimfov = 3;					//aim field of view in % 
int aimheight = 12;				//aim height value, high value aims higher
int preaim = 1;					//aim slightly in front of the target to hit moving targets better

//autoshoot settings
int autoshoot = 0;
unsigned int asdelay = 49;		//use x-999 (shoot for xx millisecs, looks more legit)
bool IsPressed = false;			//
DWORD astime = timeGetTime();	//autoshoot timer
//==========================================================================================================================

// getdir & log
char dlldir[320];
char* GetDirFile(char *name)
{
	static char pldir[320];
	strcpy_s(pldir, dlldir);
	strcat_s(pldir, name);
	return pldir;
}

void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirFile((PCHAR)"logg.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

//==========================================================================================================================

//calc distance
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct WeaponEspInfo_t
{
	float pOutX, pOutY, RealDistance;
	float CrosshairDistance;
};
std::vector<WeaponEspInfo_t>WeaponEspInfo;

//w2s
void AddModels(LPDIRECT3DDEVICE9 Device, UINT sr)
{
	D3DXMATRIX matrix;
	D3DXVECTOR4 position;
	D3DXVECTOR4 input;
	Device->GetVertexShaderConstantF(sr, matrix, 4);

	input.x = 0.0f;
	input.y = (float)aimheight;
	input.z = (float)preaim;
	input.w = 0.0f;

	D3DXVec4Transform(&position, &input, &matrix);

	position.x = input.x * matrix._11 + input.y * matrix._21 + input.z * matrix._31 + matrix._41;
	position.y = input.x * matrix._12 + input.y * matrix._22 + input.z * matrix._32 + matrix._42;
	position.z = input.x * matrix._13 + input.y * matrix._23 + input.z * matrix._33 + matrix._43;
	position.w = input.x * matrix._14 + input.y * matrix._24 + input.z * matrix._34 + matrix._44;

	float xx, yy;
	xx = ((position.x / position.w) * (Viewport.Width / 2)) + Viewport.X + (Viewport.Width / 2); 
	yy = Viewport.Y + (Viewport.Height / 2) - ((position.y / position.w) * (Viewport.Height / 2));

	WeaponEspInfo_t pWeaponEspInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(position.w*0.4f), };
	WeaponEspInfo.push_back(pWeaponEspInfo);
}

//==========================================================================================================================

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow, White, Black;
HRESULT GenerateTexture(IDirect3DDevice9 *pDevice, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}

//IDirect3DPixelShader9* oldsShader;
void DrawBox(IDirect3DDevice9 *pDevice, float x, float y, float w, float h, D3DCOLOR Color)
{
	struct Vertex
	{
		float x, y, z, ht;
		DWORD Color;
	}
	V[4] = { { x, y + h, 0.0f, 0.0f, Color },{ x, y, 0.0f, 0.01f, Color },
	{ x + w, y + h, 0.0f, 0.0f, Color },{ x + w, y, 0.0f, 0.0f, Color } };
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	//pDevice->GetPixelShader(&oldsShader);

	pDevice->SetTexture(0, NULL);
	pDevice->SetPixelShader(0);

	//pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	//pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	//pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	//pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(Vertex));

	//pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);

	//pDevice->SetPixelShader(oldsShader);
}

void DrawP(LPDIRECT3DDEVICE9 Device, int baseX, int baseY, int baseW, int baseH, D3DCOLOR Cor)
{
	D3DRECT BarRect = { baseX, baseY, baseX + baseW, baseY + baseH };
	Device->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, Cor, 0, 0);
}

void DrawCornerBox(LPDIRECT3DDEVICE9 Device, int x, int y, int w, int h, int borderPx, DWORD borderColor)
{
	DrawP(Device, x - (w / 2), (y - h + borderPx), w / 3, borderPx, borderColor); //bottom 
	DrawP(Device, x - (w / 2) + w - w / 3, (y - h + borderPx), w / 3, borderPx, borderColor); //bottom 
	DrawP(Device, x - (w / 2), (y - h + borderPx), borderPx, w / 3, borderColor); //left 
	DrawP(Device, x - (w / 2), (y - h + borderPx) + h - w / 3, borderPx, w / 3, borderColor); //left 
	DrawP(Device, x - (w / 2), y, w / 3, borderPx, borderColor); //top 
	DrawP(Device, x - (w / 2) + w - w / 3, y, w / 3, borderPx, borderColor); //top 
	DrawP(Device, (x + w - borderPx) - (w / 2), (y - h + borderPx), borderPx, w / 3, borderColor);//right 
	DrawP(Device, (x + w - borderPx) - (w / 2), (y - h + borderPx) + h - w / 3, borderPx, w / 3, borderColor);//right 
}

class D3DTLVERTEX
{
public:
	FLOAT X, Y, X2, Y2;
	DWORD Color;
};

//IDirect3DPixelShader9* oldlShader;
void DrawLine(IDirect3DDevice9* pDevice, float X, float Y, float X2, float Y2, float Width, D3DCOLOR Color, bool AntiAliased)
{
	D3DTLVERTEX qV[2] = {
		{ (float)X , (float)Y, 0.0f, 1.0f, Color },
	{ (float)X2 , (float)Y2 , 0.0f, 1.0f, Color },
	};
	const DWORD D3DFVF_TL = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

	pDevice->SetFVF(D3DFVF_TL);

	//pDevice->GetPixelShader(&oldlShader);

	//pDevice->SetTexture(0, Yellow);
	pDevice->SetTexture(0, NULL);
	pDevice->SetPixelShader(0);

	//pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, (AntiAliased ? TRUE : FALSE));

	pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 2, qV, sizeof(D3DTLVERTEX));

	pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
	//pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);

	//pDevice->SetPixelShader(oldlShader);
}

LPD3DXLINE pLine;
VOID DrawLine2(IDirect3DDevice9* pDevice, FLOAT startx, FLOAT starty, FLOAT endx, FLOAT endy, FLOAT width, D3DCOLOR dColor)
{
	D3DXVECTOR2 lines[] = { D3DXVECTOR2(startx, starty), D3DXVECTOR2(endx, endy) };

	pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	pLine->SetAntialias(TRUE);

	pLine->SetWidth(width);
	pLine->Begin();
	pLine->Draw(lines, 2, dColor);
	pLine->End();

	pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
}

//==========================================================================================================================

void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirFile((PCHAR)"cdd3d.ini"), ios::trunc);
	fout << "esp_group " << esp_group << endl;
	fout << "aim_group " << aim_group << endl;
	fout << "misc_group " << misc_group << endl;
	fout << "wallhack " << wallhack << endl;
	fout << "distanceesp " << distanceesp << endl;
	fout << "lineesp " << lineesp << endl;
	fout << "boxesp " << boxesp << endl;
	fout << "aimbot " << aimbot << endl;
	fout << "aimkey " << aimkey << endl;
	fout << "aimsens " << aimsens << endl;
	fout << "aimfov " << aimfov << endl;
	fout << "aimheight " << aimheight << endl;
	fout << "preaim " << preaim << endl;
	fout << "autoshoot " << autoshoot << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirFile((PCHAR)"cdd3d.ini"), ifstream::in);
	fin >> Word >> esp_group;
	fin >> Word >> aim_group;
	fin >> Word >> misc_group;
	fin >> Word >> wallhack;
	fin >> Word >> distanceesp;
	fin >> Word >> lineesp;
	fin >> Word >> boxesp;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimsens;
	fin >> Word >> aimfov;
	fin >> Word >> aimheight;
	fin >> Word >> preaim;
	fin >> Word >> autoshoot;
	fin.close();
}

//==========================================================================================================================

// menu stuff
HRESULT DrawString(LPD3DXFONT Font, INT X, INT Y, DWORD dColor, const char* cString, ...)
{
	HRESULT hRet;

	CHAR buf[512] = { NULL };
	va_list ArgumentList;
	va_start(ArgumentList, cString);
	_vsnprintf_s(buf, sizeof(buf), sizeof(buf) - strlen(buf), cString, ArgumentList);
	va_end(ArgumentList);

	RECT rc[2];
	SetRect(&rc[0], X, Y, X, 0);
	SetRect(&rc[1], X, Y, X + 50, 50);

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		Font->DrawTextA(NULL, buf, -1, &rc[0], DT_NOCLIP, 0xFF000000);
		hRet = Font->DrawTextA(NULL, buf, -1, &rc[1], DT_NOCLIP, dColor);
	}

	return hRet;
}

HRESULT DrawCenteredString(LPD3DXFONT Font, INT X, INT Y, DWORD dColor, const char* cString, ...)
{
	HRESULT hRet;

	CHAR buf[512] = { NULL };
	va_list ArgumentList;
	va_start(ArgumentList, cString);
	_vsnprintf_s(buf, sizeof(buf), sizeof(buf) - strlen(buf), cString, ArgumentList);
	va_end(ArgumentList);

	RECT rc[2];
	SetRect(&rc[0], X, Y, X, 0);
	SetRect(&rc[1], X, Y, X + 2, 2);

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		Font->DrawTextA(NULL, buf, -1, &rc[0], DT_NOCLIP | DT_CENTER, 0xFF000000);
		hRet = Font->DrawTextA(NULL, buf, -1, &rc[1], DT_NOCLIP | DT_CENTER, dColor);
	}

	return hRet;
}

int menuselect = 0;
int Current = true;

int PosX = 30;
int PosY = 27;

int ShowMenu = false; //off by default

POINT Pos;

int CheckTab(int x, int y, int w, int h)
{
	if (ShowMenu)
	{
		GetCursorPos(&Pos);
		ScreenToClient(GetForegroundWindow(), &Pos);
		if (Pos.x > x && Pos.x < x + w && Pos.y > y && Pos.y < y + h)
		{
			if (GetAsyncKeyState(VK_LBUTTON) & 1)
			{
				//return 1; //disabled mouse selection in menu
			}
			return 2;
		}
	}
	return 0;
}

void WriteText(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	Font->DrawTextA(0, text, -1, &rect, DT_NOCLIP | DT_LEFT, color);
}

void lWriteText(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	Font->DrawTextA(0, text, -1, &rect, DT_NOCLIP | DT_RIGHT, color);
}

void AddItem(LPDIRECT3DDEVICE9 pDevice, char *text, int &var, char **opt, int MaxValue)
{
	if (ShowMenu)
	{
		int Check = CheckTab(PosX + 44, (PosY + 51) + (Current * 15), 190, 10);
		DWORD ColorText;

		if (var)
		{
			//DrawBox(pDevice, PosX+44, PosY+51 + (Current * 15), 10, 10, D3DCOLOR_ARGB(255, 0, 255, 0));
			ColorText = D3DCOLOR_ARGB(255, 0, 255, 0);
		}
		if (var == 0)
		{
			//DrawBox(pDevice, PosX+44, PosY+51 + (Current * 15), 10, 10, D3DCOLOR_ARGB(255, 255, 0, 0));
			ColorText = D3DCOLOR_ARGB(255, 255, 0, 0);
		}

		if (Check == 1)
		{
			var++;
			if (var > MaxValue)
				var = 0;
		}

		if (Check == 2)
			ColorText = D3DCOLOR_ARGB(255, 255, 255, 255);

		if (menuselect == Current)
		{
			static ULONGLONG lasttick_right = GetTickCount64();
			static ULONGLONG lasttick_left = GetTickCount64();
			if (GetAsyncKeyState(VK_RIGHT) && GetTickCount64() - lasttick_right > 100)
			{
				lasttick_right = GetTickCount64();
				var++;
				if (var > MaxValue)
					var = 0;
			}
			else if (GetAsyncKeyState(VK_LEFT) && GetTickCount64() - lasttick_left > 100)
			{
				lasttick_left = GetTickCount64();
				var--;
				if (var < 0)
					var = MaxValue;
			}
		}

		if (menuselect == Current)
			ColorText = D3DCOLOR_ARGB(255, 255, 255, 255);


		WriteText(PosX + 44, PosY + 50 + (Current * 15) - 1, D3DCOLOR_ARGB(255, 50, 50, 50), text);
		WriteText(PosX + 45, PosY + 51 + (Current * 15) - 1, ColorText, text);

		lWriteText(PosX + 236, PosY + 50 + (Current * 15) - 1, D3DCOLOR_ARGB(255, 100, 100, 100), opt[var]);
		lWriteText(PosX + 237, PosY + 51 + (Current * 15) - 1, ColorText, opt[var]);
		Current++;
	}
}

void AddGroup(LPDIRECT3DDEVICE9 pDevice, char *txt, int &var, char **opt, int maxval)
{
	AddItem(pDevice, txt, var, opt, maxval);
}

//==========================================================================================================================

// menu part
char *opt_OnOff[] = { (PCHAR)"[OFF]", (PCHAR)"[On]" };
char *opt_WhChams[] = { (PCHAR)"[OFF]", (PCHAR)"[On]", (PCHAR)"[Color]" };
char *opt_ZeroFive[] = { (PCHAR)"[0]", (PCHAR)"[1]", (PCHAR)"[2]", (PCHAR)"[3]", (PCHAR)"[4]", (PCHAR)"[5]", (PCHAR)"[6]" };
char *opt_ZeroTen[] = { (PCHAR)"[0]", (PCHAR)"[1]", (PCHAR)"[2]", (PCHAR)"[3]", (PCHAR)"[4]", (PCHAR)"[5]", (PCHAR)"[6]", (PCHAR)"[7]", (PCHAR)"[8]", (PCHAR)"[9]", (PCHAR)"[10]", (PCHAR)"[11]" };
char *opt_ZeroFifteen[] = { (PCHAR)"[0]", (PCHAR)"[1]", (PCHAR)"[2]", (PCHAR)"[3]", (PCHAR)"[4]", (PCHAR)"[5]", (PCHAR)"[6]", (PCHAR)"[7]", (PCHAR)"[8]", (PCHAR)"[9]", (PCHAR)"[10]", (PCHAR)"[11]", (PCHAR)"[12]", (PCHAR)"[13]", (PCHAR)"[14]", (PCHAR)"[15]" };
char *opt_Keys[] = { (PCHAR)"[OFF]", (PCHAR)"[Shift]", (PCHAR)"[RMouse]", (PCHAR)"[LMouse]", (PCHAR)"[Ctrl]", (PCHAR)"[Alt]", (PCHAR)"[Space]", (PCHAR)"[X]", (PCHAR)"[C]" };
char *opt_aimfov[] = { (PCHAR)"[0]", (PCHAR)"[5%]", (PCHAR)"[10%]", (PCHAR)"[15%]", (PCHAR)"[20%]", (PCHAR)"[25%]", (PCHAR)"[30%]", (PCHAR)"[35%]", (PCHAR)"[40%]", (PCHAR)"[45%]" };
char *opt_autoshoot[] = { (PCHAR)"[OFF]", (PCHAR)"[OnKeyDown]" };

void DrawMenu(LPDIRECT3DDEVICE9 pDevice)
{
	static ULONGLONG lasttick_insert = GetTickCount64();
	if (GetAsyncKeyState(VK_INSERT) && GetTickCount64() - lasttick_insert > 150)
	{
		lasttick_insert = GetTickCount64();
		ShowMenu = !ShowMenu;
		//save settings
		SaveCfg();
	}

	if (ShowMenu)
	{
		static ULONGLONG lasttick_up = GetTickCount64();
		if (GetAsyncKeyState(VK_UP) && GetTickCount64() - lasttick_up > 100)
		{
			lasttick_up = GetTickCount64();
			menuselect--;
		}

		static ULONGLONG lasttick_down = GetTickCount64();
		if (GetAsyncKeyState(VK_DOWN) && GetTickCount64() - lasttick_down > 100)
		{
			lasttick_down = GetTickCount64();
			menuselect++;
		}

		Current = 1;

		AddGroup(pDevice, (PCHAR)"[-=ESP SETTINGS=-]", esp_group, opt_OnOff, 1);
		if (esp_group) {
			AddItem(pDevice, (PCHAR)"Distance Esp", distanceesp, opt_OnOff, 1);
			AddItem(pDevice, (PCHAR)"Line Esp", lineesp, opt_ZeroTen, 11);
			AddItem(pDevice, (PCHAR)"Box Esp", boxesp, opt_OnOff, 1);
		}
		AddGroup(pDevice, (PCHAR)"[-=AIM SETTINGS=-]", aim_group, opt_OnOff, 1);
		if (aim_group) {
			AddItem(pDevice, (PCHAR)"Aimbot", aimbot, opt_OnOff, 1);
			AddItem(pDevice, (PCHAR)"Aimkey", aimkey, opt_Keys, 8);
			AddItem(pDevice, (PCHAR)"Aimsens", aimsens, opt_ZeroTen, 10);
			AddItem(pDevice, (PCHAR)"Aimfov", aimfov, opt_aimfov, 9);
			AddItem(pDevice, (PCHAR)"Aimheight", aimheight, opt_ZeroFifteen, 14);
			AddItem(pDevice, (PCHAR)"Preaim", preaim, opt_ZeroFive, 4);
		}
		AddGroup(pDevice, (PCHAR)"[-=MISC SETTINGS=-]", misc_group, opt_OnOff, 1);
		if (misc_group) {
			AddItem(pDevice, (PCHAR)"Wallhack", wallhack, opt_WhChams, 2);
			AddItem(pDevice, (PCHAR)"Autoshoot", autoshoot, opt_autoshoot, 1);
		}

		if (menuselect >= Current)
			menuselect = 1;

		if (menuselect < 1)
			menuselect = 14;//Current;
	}
}