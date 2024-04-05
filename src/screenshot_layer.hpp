#include <Geode/Geode.hpp>
#include <vector>
#include <chrono>

using namespace geode::prelude;

struct lists {
    CCNode* list;
    CCNode* parent;
};

class ScreenshotLayer : public CCLayer {
public:
    bool snipping = true;
    bool cursorHidden;

    CCLayerColor* overlay = nullptr;
    CCLayerColor* overlayLeft = nullptr;
    CCLayerColor* overlayRight = nullptr;
    CCLayerColor* overlayUp = nullptr;
    CCLayerColor* overlayDown = nullptr;

    CCSprite* snapshotSprite = nullptr;

    CCScene* scene = nullptr;

    CCSize visibleSize;

    CCDrawNode* drawNode = nullptr;

    static ScreenshotLayer* create() {
        auto layer = new ScreenshotLayer;
        layer->init();
        return layer;
    }

    static std::array<std::string, 2> generateScreenshotName();

    void changeCursor(bool crossCursor) {
        HWND consoleWindow = GetConsoleWindow();
    	HCURSOR cursor = LoadCursor(NULL, crossCursor ? IDC_CROSS : IDC_ARROW);
    	SetCursor(cursor);
    }

    void addBlackOverlay();

    void captureScene(bool isSnip);

    void captureSnip(CCPoint startPoint, CCPoint currentPoint);

    void captureFullScreenshot(bool isFake);

    void flashAnim();

    void updateSelection(CCPoint startPoint, CCPoint currentPoint);

};
