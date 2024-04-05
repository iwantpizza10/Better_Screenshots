#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GameManager.hpp>

#include <geode.custom-keybinds/include/Keybinds.hpp>

#include "screenshot_layer.hpp"

ScreenshotLayer* screenshotLayer = nullptr;
bool isFullScreenshot = false;
bool isScreenshotting = false;

void screenshot(bool isSnip, bool isFake) {
	if (!Mod::get()->getSettingValue<bool>("save_to_clipboard") && !Mod::get()->getSettingValue<bool>("save_file") && !isFake)
		return;

	PlayLayer* pl = PlayLayer::get();
	isFullScreenshot = !isSnip;
	isScreenshotting = true;

	auto scene = CCDirector::sharedDirector()->getRunningScene();

	screenshotLayer = ScreenshotLayer::create();
	if (isFake) screenshotLayer->setVisible(false);

	CCSize visibleSize = CCDirector::sharedDirector()->getVisibleSize();
	screenshotLayer->visibleSize = visibleSize;
	screenshotLayer->scene = scene;
	screenshotLayer->cursorHidden = cocos2d::CCEGLView::sharedOpenGLView()->getShouldHideCursor();

	screenshotLayer->captureScene(isSnip);

	scene->addChild(screenshotLayer, std::numeric_limits<int>::max());

	if (isSnip) {
		if (pl) {
			if (Mod::get()->getSettingValue<bool>("auto_pause"))
				pl->pauseGame(true);
			else if (screenshotLayer->cursorHidden)
				cocos2d::CCEGLView::sharedOpenGLView()->showCursor(true);
		}
		screenshotLayer->changeCursor(true);
		screenshotLayer->addBlackOverlay();
	}
	else {
		screenshotLayer->captureFullScreenshot(isFake);
		isScreenshotting = false;
	}
}

class $modify(CCTouchDispatcher) {

	void touches(cocos2d::CCSet * set, cocos2d::CCEvent * v2, unsigned int action) {
		if (!screenshotLayer) CCTouchDispatcher::touches(set, v2, action);

		if (screenshotLayer) {
			if (!screenshotLayer->snipping || !isScreenshotting) {
				if (screenshotLayer) {
					screenshotLayer->removeFromParentAndCleanup(true);
					screenshotLayer = nullptr;
				}
				return CCTouchDispatcher::touches(set, v2, action);
			}
		}

		if (action == 1) {
			if (!screenshotLayer || isFullScreenshot) {
				if (screenshotLayer) {
					screenshotLayer->removeFromParentAndCleanup(true);
					screenshotLayer = nullptr;
				}
				return;
			}
			auto it = set->begin();
			CCTouch* touch = dynamic_cast<CCTouch*>(*it);
			screenshotLayer->updateSelection(touch->getStartLocation(), touch->getLocation());
		}
		else if (action == 2) {
			if (!screenshotLayer || isFullScreenshot) {
				if (screenshotLayer) {
					screenshotLayer->removeFromParentAndCleanup(true);
					screenshotLayer = nullptr;
				}
				return;
			}
			auto it = set->begin();
			CCTouch* touch = dynamic_cast<CCTouch*>(*it);

			CCPoint diff = touch->getStartLocation() - touch->getLocation();
			if (abs(diff.x) < 2 || abs(diff.y) < 2) {
				if (screenshotLayer->drawNode)
					screenshotLayer->drawNode->setVisible(false);

				if (screenshotLayer->overlayLeft) screenshotLayer->overlayLeft->removeFromParentAndCleanup(true);
    			if (screenshotLayer->overlayRight) screenshotLayer->overlayRight->removeFromParentAndCleanup(true);
    			if (screenshotLayer->overlayUp) screenshotLayer->overlayUp->removeFromParentAndCleanup(true);
    			if (screenshotLayer->overlayDown) screenshotLayer->overlayDown->removeFromParentAndCleanup(true);

				screenshotLayer->overlayLeft = nullptr;
				screenshotLayer->overlayRight = nullptr;
				screenshotLayer->overlayUp = nullptr;
				screenshotLayer->overlayDown = nullptr;

				if (screenshotLayer->overlay) screenshotLayer->overlay->setVisible(true);
				return;
			}

			screenshotLayer->captureSnip(touch->getStartLocation(), touch->getLocation());
			screenshotLayer->changeCursor(false);
			isScreenshotting = false;
		}
	}
};

class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat) {
		if (key == enumKeyCodes::KEY_Escape) {
			if (screenshotLayer) {
				if (screenshotLayer->snipping) screenshotLayer->changeCursor(false);
				screenshotLayer->removeFromParentAndCleanup(true);
				screenshotLayer = nullptr;
				if (isScreenshotting) {
					isScreenshotting = false;
					return false;
				}
			}
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat);

	}
};

class $modify(CCScheduler) {
	void update(float dt) {
		CCScheduler::update(dt);
		if (!isScreenshotting) return;
		if (screenshotLayer) {
				if (screenshotLayer->snipping)
					screenshotLayer->changeCursor(true);
		}
	}
};

 // some weird bug with the first screenshot of every session or after reloading
class $modify(GameManager) {
    void reloadAllStep5() {
		GameManager::reloadAllStep5();
		screenshot(false, true);
	}
	
};

$on_mod(Loaded) {
	screenshot(false, true);
}

using namespace keybinds;

$execute{
	BindManager::get()->registerBindable({
		"full_screenshot"_spr,
		"Full Screenshot",
		"Take a screenshot of the whole screen.",
		{ Keybind::create(KEY_F, Modifier::Control) },
		"Better Screenshots/Keybinds"
	});
	new EventListener([=](InvokeBindEvent* event) {
		if (event->isDown()) {
			if (screenshotLayer) {
				screenshotLayer->removeFromParentAndCleanup(true);
				screenshotLayer = nullptr;
			}
			screenshot(false, false);
		}
	return ListenerResult::Propagate;
	}, InvokeBindFilter(nullptr, "full_screenshot"_spr));

	BindManager::get()->registerBindable({
		"snip_screenshot"_spr,
		"Snip Screenshot",
		"Take a screenshot of a part of the screen.",
		{ Keybind::create(KEY_S, Modifier::Control) },
		"Better Screenshots/Keybinds"
	});
	new EventListener([=](InvokeBindEvent* event) {
		if (event->isDown() && !isScreenshotting) {
			if (screenshotLayer) {
				screenshotLayer->removeFromParentAndCleanup(true);
				screenshotLayer = nullptr;
			}
			screenshot(true, false);
		}
	return ListenerResult::Propagate;
	}, InvokeBindFilter(nullptr, "snip_screenshot"_spr));

	BindManager::get()->registerBindable({
		"hide_pause_menu"_spr,
		"Hide Pause Menu",
		"Hide the pause menu to take a better screenshot.",
		{ Keybind::create(KEY_V, Modifier::Control) },
		"Better Screenshots/Keybinds"
	});
	new EventListener([=](InvokeBindEvent* event) {
		if (event->isDown() && PlayLayer::get()) {
			CCObject* obj;
			CCARRAY_FOREACH(CCDirector::sharedDirector()->getRunningScene()->getChildren(), obj) {
				PauseLayer* pauseLayer = dynamic_cast<PauseLayer*>(obj);
				if (pauseLayer) {
					pauseLayer->setVisible(!pauseLayer->isVisible());
					break;
				}
			}
		}
	return ListenerResult::Propagate;
	}, InvokeBindFilter(nullptr, "hide_pause_menu"_spr));
}
