// not taken from https://github.com/SpaghettDev/GD-Roulette/blob/geode/src/custom_nodes/RLLoadingCircle.cpp :D
#include "SPLoadingCircle.hpp"

SPLoadingCircle* SPLoadingCircle::create()
{
	auto ret = new SPLoadingCircle();

	if (ret && ret->init())
		ret->autorelease();
	else
	{
		delete ret;
		ret = nullptr;
	}

	return ret;
}

bool SPLoadingCircle::init()
{
	if (!this->initWithFile("loadingCircle.png")) return false;

	this->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
	this->setOpacity(0);
	this->setZOrder(105);

	return true;
}

void SPLoadingCircle::show()
{
	auto* fadeInAction = cocos2d::CCFadeTo::create(.3f, 200);
	fadeInAction->setTag(ACTION_TAG::FADE_IN);

	this->runAction(fadeInAction);

    auto* rotateAction = cocos2d::CCRepeatForever::create(
		cocos2d::CCRotateBy::create(1.f, 360.f)
	);
	rotateAction->setTag(ACTION_TAG::ROTATE);

	this->runAction(rotateAction);
}

void SPLoadingCircle::hide()
{
    this->stopActionByTag(ACTION_TAG::ROTATE);
    this->stopActionByTag(ACTION_TAG::FADE_IN);

	auto* fadeOutAction = cocos2d::CCFadeTo::create(.4f, 0);
	fadeOutAction->setTag(ACTION_TAG::FADE_OUT);

	this->runAction(fadeOutAction);
}
