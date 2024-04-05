#include "screenshot_layer.hpp"
#include "clipboard.hpp"

std::array<std::string, 2> ScreenshotLayer::generateScreenshotName() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::stringstream ss;
    ss << std::hex << std::uppercase << rand();
    std::string filename = "screenshot_" + std::to_string(timestamp) + "_" + ss.str() + ".png";
    std::string prePath = "geode\\mods\\zilko.better_screenshots\\" + filename;

    std::filesystem::path path = Mod::get()->getSaveDir().string() + "\\" + filename;

    return { path.string(), prePath };
}

void ScreenshotLayer::addBlackOverlay() {
    overlay = CCLayerColor::create(ccc4(0, 0, 0, 0), visibleSize.width, visibleSize.height);
    overlay->setZOrder(3);

    auto fadeInAction = CCFadeTo::create(0.3f, 110);
    overlay->runAction(fadeInAction);

    addChild(overlay);
}

void ScreenshotLayer::captureScene(bool isSnip) {
    HWND hwnd = GetForegroundWindow();
    RECT windowRect;
    GetClientRect(hwnd, &windowRect); 

    float width = windowRect.right - windowRect.left;
    float height = windowRect.bottom - windowRect.top;

    int tex = GameManager::sharedState()->m_texQuality;
    float scale = visibleSize.width / width * (tex == 3 ? 4 : tex);

    GLubyte* buffer = nullptr;
    buffer = new GLubyte[width * height * 3]; 

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

    CCTexture2D* texture = new CCTexture2D();
    texture->initWithData(buffer, kCCTexture2DPixelFormat_RGB888, width, height, CCSize(width, height));

    snapshotSprite = CCSprite::createWithTexture(texture);
    snapshotSprite->setScaleY(-scale);
    snapshotSprite->setScaleX(scale);
    snapshotSprite->setAnchorPoint({0,1});
    snapshotSprite->setPosition({0,0});
    addChild(snapshotSprite);
    
}

void ScreenshotLayer::flashAnim() {
    if (snapshotSprite) snapshotSprite->setVisible(false);

    overlay = CCLayerColor::create(ccc4(255, 255, 255, 20), visibleSize.width, visibleSize.height);
    overlay->setZOrder(3);

    auto fadeInAction = CCFadeTo::create(0.05f, 30);
    auto fadeOutAction = CCFadeTo::create(0.3f, 0);

    auto sequence = CCSequence::create(fadeInAction, CCDelayTime::create(0.05f), fadeOutAction, nullptr);

    overlay->runAction(sequence);

    addChild(overlay);
}

void ScreenshotLayer::updateSelection(CCPoint startPoint, CCPoint currentPoint) {
    if (drawNode) drawNode->removeFromParentAndCleanup(true);
    if (overlay) overlay->setVisible(false);

    drawNode = CCDrawNode::create();
    drawNode->drawRect(startPoint, currentPoint, ccColor4F(0, 0, 0, 0), 0.3f, ccColor4F(1, 1, 1, 1));
    drawNode->setZOrder(4);
    addChild(drawNode);

    CCSize visibleSize = CCDirector::sharedDirector()->getVisibleSize();

    if (overlayLeft) overlayLeft->removeFromParentAndCleanup(true);
    if (overlayRight) overlayRight->removeFromParentAndCleanup(true);
    if (overlayUp) overlayUp->removeFromParentAndCleanup(true);
    if (overlayDown) overlayDown->removeFromParentAndCleanup(true);

    overlayLeft = CCLayerColor::create(ccc4(0, 0, 0, 110), visibleSize.width, visibleSize.height);
    overlayLeft->setPosition({ startPoint.x < currentPoint.x ? currentPoint.x : startPoint.x, 0.f });
    overlayLeft->setAnchorPoint({ 0.f, 0.f });
    overlayLeft->setZOrder(4);
    addChild(overlayLeft);

    overlayRight = CCLayerColor::create(ccc4(0, 0, 0, 110), visibleSize.width, visibleSize.height);
    overlayRight->setPosition({ (startPoint.x < currentPoint.x ? currentPoint.x : startPoint.x) - visibleSize.width - abs(startPoint.x - currentPoint.x), 0.f });
    overlayRight->setAnchorPoint({ 1.f, 0.f });
    overlayRight->setZOrder(4);
    addChild(overlayRight);

    overlayUp = CCLayerColor::create(ccc4(0, 0, 0, 110), abs(startPoint.x - currentPoint.x), visibleSize.height);
    overlayUp->setPosition({ startPoint.x > currentPoint.x ? currentPoint.x : startPoint.x, startPoint.y < currentPoint.y ? currentPoint.y : startPoint.y });
    overlayUp->setAnchorPoint({ 1.f, 1.f });
    overlayUp->setZOrder(4);
    addChild(overlayUp);

    overlayDown = CCLayerColor::create(ccc4(0, 0, 0, 110), abs(startPoint.x - currentPoint.x), visibleSize.height);
    overlayDown->setPosition({ startPoint.x > currentPoint.x ? currentPoint.x : startPoint.x, (startPoint.y < currentPoint.y ? currentPoint.y : startPoint.y) - visibleSize.height - abs(startPoint.y - currentPoint.y) });
    overlayDown->setAnchorPoint({ 1.f, 1.f });
    overlayDown->setZOrder(4);
    addChild(overlayDown);
}

void ScreenshotLayer::captureSnip(CCPoint startPoint, CCPoint finalPoint) {
    auto path = ScreenshotLayer::generateScreenshotName();

    if (drawNode) drawNode->setVisible(false);
    if (overlayUp) overlayUp->setVisible(false);
    if (overlayDown) overlayDown->setVisible(false);
    if (overlayLeft) overlayLeft->setVisible(false);
    if (overlayRight) overlayRight->setVisible(false);

    float scale = snapshotSprite->getScaleX();
    float spriteWidth = snapshotSprite->getContentSize().width * scale;
    float spriteHeight = snapshotSprite->getContentSize().height * scale;

    CCRenderTexture* sceneRender = CCRenderTexture::create(spriteWidth, spriteHeight);
    sceneRender->begin();
    snapshotSprite->visit();
    sceneRender->end();

    auto snipSprite = CCSprite::createWithTexture(sceneRender->getSprite()->getTexture());

    float x = startPoint.x < finalPoint.x ? startPoint.x : finalPoint.x;
    float y = startPoint.y < finalPoint.y ? startPoint.y : finalPoint.y;

    float width = abs(startPoint.x - finalPoint.x);
    float height = abs(startPoint.y - finalPoint.y);

    CCRect cropArea(x, y, width, height);

    snipSprite->setTextureRect(cropArea);
    snipSprite->setPosition({ 0,0 });
    snipSprite->setAnchorPoint({ 0,1 });
    snipSprite->setScaleY(-1.f);

    CCRenderTexture* snipRender = CCRenderTexture::create(width, height);
    snipRender->begin();
    snipSprite->visit();
    snipRender->end();

    snipRender->saveToFile(path[1].c_str(), tCCImageFormat::kCCImageFormatPNG);

    if (Mod::get()->getSettingValue<bool>("save_to_clipboard")) {
        HBITMAP imgBitmap = Clipboard::getBitmap(path[0]);
        if (imgBitmap != NULL) Clipboard::bitmapToClipboard(imgBitmap);
    }

    if (!Mod::get()->getSettingValue<bool>("save_file"))
        std::remove(path[0].c_str());

    flashAnim();
    snipping = false;

    if (!Mod::get()->getSettingValue<bool>("auto_pause"))
        cocos2d::CCEGLView::sharedOpenGLView()->showCursor(!cursorHidden);
}

void ScreenshotLayer::captureFullScreenshot(bool isFake) {
    auto path = ScreenshotLayer::generateScreenshotName();

    float scale = snapshotSprite->getScaleX();
    float width = snapshotSprite->getContentSize().width * scale;
    float height = snapshotSprite->getContentSize().height * scale;

    CCRenderTexture* finalScreenshot = CCRenderTexture::create(width, height);
    finalScreenshot->begin();
    snapshotSprite->visit();
    finalScreenshot->end();

    finalScreenshot->saveToFile(path[1].c_str(), tCCImageFormat::kCCImageFormatPNG);
    
    if (!isFake) {
        if (Mod::get()->getSettingValue<bool>("save_to_clipboard")) {
            HBITMAP imgBitmap = Clipboard::getBitmap(path[0]);
            if (imgBitmap != NULL) Clipboard::bitmapToClipboard(imgBitmap);
        }
        flashAnim();
    } 
    
    if (isFake || !Mod::get()->getSettingValue<bool>("save_file"))
        std::remove(path[0].c_str());
    
}