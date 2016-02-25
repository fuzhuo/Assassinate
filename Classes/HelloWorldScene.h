#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"
#define STATIC_LAYER "StaticLayer"

typedef enum {
    kBoxGround,
    kBoxSensor
}CollisionType;

USING_NS_CC;
class HelloWorld : public cocos2d::Layer
{
public:
    // there's no 'id' in cpp, so we recommend returning the class instance pointer
    static cocos2d::Scene* createScene();

    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool init();

    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);
    ~HelloWorld();
public:
    //input
    virtual void onEnterTransitionDidFinish();
    virtual void onExitTransitionDidStart();
    //touch
    virtual void onTouchesBegan(const std::vector<Touch*>& touches, Event *event);
    virtual void onTouchesMoved(const std::vector<Touch*>& touches, Event *event);
    virtual void onTouchesEnded(const std::vector<Touch*>& touches, Event *event);
    virtual void onTouchesCancelled(const std::vector<Touch*>& touches, Event *event);
    
    //keyboard
    virtual void onKeyPressed(EventKeyboard::KeyCode keyCode, Event* unused_event);
    virtual void onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event);
    EventListenerTouchAllAtOnce *listener;
    EventListenerKeyboard *keyListener;
    
private:
    Size winSize;
    PhysicsWorld *_world;
    TMXTiledMap *_map;
    TMXLayer *_staticLayer;
    LayerColor *_fadeLayer;
    void initMap();
    void createB2StaticRect(Point pos, Size size, CollisionType type);
    float _ratio;
    std::vector<Rect> _physicBlock;
    Size _mapSize;
    Size _tileSize;
    
    void initHero();
    Sprite *_hero;
    bool _showDebugDraw;
    void setViewpointCenter(Point position);
    void update(float dt);
};

#endif // __HELLOWORLD_SCENE_H__
