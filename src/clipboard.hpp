#include <stdlib.h>
#include <Windows.h>
#include <gdiplus.h>

#pragma comment (lib, "gdiplus.lib")

class Clipboard {
public:

    static bool bitmapToClipboard(HBITMAP hBM);

    static HBITMAP getBitmap(std::filesystem::path path);
};