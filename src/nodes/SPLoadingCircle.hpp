#pragma once

#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/actions/CCActionInterval.h>

class SPLoadingCircle : public cocos2d::CCSprite
{
public:
	static SPLoadingCircle* create();

	bool init() override;

	void show();
	void hide();

private:
	enum ACTION_TAG : int
	{
		FADE_IN,
		FADE_OUT,
		ROTATE
	};
};
