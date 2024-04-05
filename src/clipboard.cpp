#include "clipboard.hpp"

bool Clipboard::bitmapToClipboard(HBITMAP hBM) {
	if (!::OpenClipboard(NULL))
		return false;
	::EmptyClipboard();

	BITMAP bm;
	::GetObject(hBM, sizeof(bm), &bm);

	BITMAPINFOHEADER bi;
	::ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = bm.bmBitsPixel;
	bi.biCompression = BI_RGB;
	if (bi.biBitCount <= 1)
		bi.biBitCount = 1;
	else if (bi.biBitCount <= 4)
		bi.biBitCount = 4;
	else if (bi.biBitCount <= 8)
		bi.biBitCount = 8;
	else
		bi.biBitCount = 24;

	SIZE_T dwColTableLen = (bi.biBitCount <= 8) ? (1 << bi.biBitCount) * sizeof(RGBQUAD) : 0;

	HDC hDC = ::GetDC(NULL);
	HPALETTE hPal = static_cast<HPALETTE>(::GetStockObject(DEFAULT_PALETTE));
	HPALETTE hOldPal = ::SelectPalette(hDC, hPal, FALSE);
	::RealizePalette(hDC);

	::GetDIBits(hDC, hBM, 0, static_cast<UINT>(bi.biHeight), NULL,
		reinterpret_cast<LPBITMAPINFO>(&bi), DIB_RGB_COLORS);
	if (0 == bi.biSizeImage)
		bi.biSizeImage = ((((bi.biWidth * bi.biBitCount) + 31) & ~31) / 8) * bi.biHeight;

	HGLOBAL hDIB = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + dwColTableLen + bi.biSizeImage);
	if (hDIB)
	{
		union tagHdr_u
		{
			LPVOID             p;
			LPBYTE             pByte;
			LPBITMAPINFOHEADER pHdr;
			LPBITMAPINFO       pInfo;
		} Hdr;

		Hdr.p = ::GlobalLock(hDIB);
		::CopyMemory(Hdr.p, &bi, sizeof(BITMAPINFOHEADER));
		int nConv = ::GetDIBits(hDC, hBM, 0, static_cast<UINT>(bi.biHeight),
			Hdr.pByte + sizeof(BITMAPINFOHEADER) + dwColTableLen,
			Hdr.pInfo, DIB_RGB_COLORS);
		::GlobalUnlock(hDIB);
		if (!nConv)
		{
			::GlobalFree(hDIB);
			hDIB = NULL;
		}
	}
	if (hDIB)
		::SetClipboardData(CF_DIB, hDIB);
	::CloseClipboard();
	::SelectPalette(hDC, hOldPal, FALSE);
	::ReleaseDC(NULL, hDC);
	return NULL != hDIB;
}

HBITMAP Clipboard::getBitmap(std::filesystem::path path) {
	int bufferSize = MultiByteToWideChar(CP_UTF8, 0, path.string().c_str(), -1, nullptr, 0);
	if (bufferSize == 0)
		return NULL;

	WCHAR* wideString = new WCHAR[bufferSize];
	if (MultiByteToWideChar(CP_UTF8, 0, path.string().c_str(), -1, wideString, bufferSize) == 0) {
		delete[] wideString;
		return NULL;
	}

	using namespace Gdiplus;
	GdiplusStartupInput gpStartupInput;
	ULONG_PTR gpToken;
	GdiplusStartup(&gpToken, &gpStartupInput, NULL);
	HBITMAP result = NULL;
	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wideString, false);
	if (bitmap) {
		bitmap->GetHBITMAP(Color(255, 255, 255), &result);
		delete bitmap;
	}

	GdiplusShutdown(gpToken);

	delete[] wideString;
	return result;
}