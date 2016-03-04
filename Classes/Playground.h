#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"
#define STATIC_LAYER "StaticLayer"
#include "Hero.hpp"
#include "common.h"

USING_NS_CC;

class Playground : public cocos2d::Layer
{
public:
    // there's no 'id' in cpp, so we recommend returning the class instance pointer
    static cocos2d::Scene* createScene();

    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool init();

    // implement the "static create()" method manually
    CREATE_FUNC(Playground);
    ~Playground();
public:
    //input
    virtual void onEnterTransitionDidFinish();
    virtual void onExitTransitionDidStart();
    //touch
    void onTouchesBegan(const std::vector<Touch*>& touches, Event *event);
    void onTouchesMoved(const std::vector<Touch*>& touches, Event *event);
    void onTouchesEnded(const std::vector<Touch*>& touches, Event *event);
    void onTouchesCancelled(const std::vector<Touch*>& touches, Event *event);
    
    //keyboard
    void onKeyPressed(EventKeyboard::KeyCode keyCode, Event* unused_event);
    void onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event);
    EventListenerTouchAllAtOnce *listener;
    EventListenerKeyboard *keyListener;
    
    virtual bool onContactBegin(const PhysicsContact &contact);
    virtual bool onContactSeparate(const PhysicsContact &contact);
private:
    Size winSize;
    PhysicsWorld *_world;
    TMXTiledMap *_map;
    TMXLayer *_staticLayer;
    LayerColor *_fadeLayer;
    void initMap();
    void createB2StaticRect(Point pos, Size size, int tag);
    float _ratio;
    std::vector<Rect> _physicBlock;
    std::vector<int> _physicBlockType;
    Size _mapSize;
    Size _tileSize;
    int **_visited;
    
    void initHero();
    Sprite *_heroSprite;
    Hero *_hero;
    bool _showDebugDraw;
    void setViewpointCenter(Point position);
    void update(float dt);
    unsigned int _key;
    PhysicsBody *_currentStandBody;
};

#endif // __HELLOWORLD_SCENE_H__
