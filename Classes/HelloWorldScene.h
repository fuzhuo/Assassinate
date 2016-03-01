#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"
#define STATIC_LAYER "StaticLayer"

typedef enum {
    kMapType_ground,
    kMapType_wall,
    kMapType_hung,
    kMapType_stage,
    kMapType_empty,
    kMapType_harm
}CollisionType;

typedef enum{
    kMapVisit_NO=0,
    kMapVisit_YES
}kMapVisit;

typedef enum{
    kShapeTag_normal=0,
    kShapeTag_round,
    kShapeTag_down,
    kShapeTag_down_round,
    kShapeTag_hero_stand,
    kShapeTag_hero_squat,
    kShapeTag_hero_hung
}kShapeTag;

typedef enum{
    kHeroState_stand,
    kHeroState_hung,
    kHeroState_squat
}kHeroState;

typedef enum{
    BUTTON_U=1<<0,
    BUTTON_D=1<<1,
    BUTTON_L=1<<2,
    BUTTON_R=1<<3,
    BUTTON_A=1<<4,
    BUTTON_B=1<<5
}kButton;

#define HERO_GID -1
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
    Sprite *_hero;
    bool _showDebugDraw;
    void setViewpointCenter(Point position);
    void update(float dt);
    unsigned int _key;
    void changeHeroState(kHeroState state);
    kHeroState _state;
    PhysicsBody *_currentStandBody;
};

#endif // __HELLOWORLD_SCENE_H__
