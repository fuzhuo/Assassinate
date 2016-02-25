#include "HelloWorldScene.h"
#include "cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include "tinyxml2/tinyxml2.h"

USING_NS_CC;

using namespace cocostudio::timeline;

Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    //auto scene = Scene::create();
    auto scene = Scene::createWithPhysics();
    scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();
    layer->_world = scene->getPhysicsWorld();
    layer->_world->setGravity(Vec2(0.0, -980));
    layer->_showDebugDraw=true;

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Layer::init() )
    {
        return false;
    }
    winSize = Director::getInstance()->getVisibleSize();
    
    _ratio = Director::getInstance()->getContentScaleFactor();
    initMap();
    initHero();
//    createB2StaticRect(Point(20,15), Size(40,30), kBoxGround);
    return true;
}

HelloWorld::~HelloWorld()
{
    _map->release();
}

void HelloWorld::initMap()
{
    std::string path = FileUtils::getInstance()->fullPathForFilename("Assassinate_map01.tmx");
    _map = TMXTiledMap::create(path);
    _map->retain();
    _mapSize=_map->getMapSize();
    _tileSize=_map->getTileSize();
    _staticLayer = _map->getLayer(STATIC_LAYER);
    
    //xml to get collision data
    tinyxml2::XMLDocument *pDoc = new tinyxml2::XMLDocument();
    assert(pDoc!=nullptr);
    pDoc->LoadFile(path.c_str());
    tinyxml2::XMLElement *rootEle = pDoc->RootElement();
    auto tile = rootEle->FirstChildElement()->FirstChildElement()->NextSiblingElement();
    auto t = tile;
    int max=-1;
    while (t) {
        int id;
        t->QueryIntAttribute("id", &id);
        if (id>=max) max=id;
        t=t->NextSiblingElement();
    }
    _physicBlock.clear();
    _physicBlock=std::vector<Rect>(max);
    while (tile)
    {
        int gid;
        int id;
        float x,y,width,height;
        auto err = tile->QueryIntAttribute("id", &gid);
        auto tile_obj=tile->FirstChildElement()->FirstChildElement();
        if (tile_obj == nullptr) {
            tile=tile->NextSiblingElement();
            continue;
        }
        err = tile_obj->QueryIntAttribute("id", &id);
        err = tile_obj->QueryFloatAttribute("x", &x);
        err = tile_obj->QueryFloatAttribute("y", &y);
        err = tile_obj->QueryFloatAttribute("width", &width);
        err = tile_obj->QueryFloatAttribute("height", &height);
        log("ele gid[%d] id[%d],[%f, %f, %f, %f]", gid, id, x, y, width, height);
        //because center position is not 0,0, it is 0.5, 0.5
        _physicBlock[gid]=Rect(x/_ratio,(_tileSize.height-y-height/2)/_ratio,width/_ratio,height/_ratio);
        log("ele gid[%d] id[%d],[%f, %f, %f, %f]", gid, id,
            _physicBlock[gid].origin.x, _physicBlock[gid].origin.y,
            _physicBlock[gid].size.width, _physicBlock[gid].size.height);
        tile=tile->NextSiblingElement();
    }
    assert(pDoc!=nullptr);
    delete pDoc;
    
    this->addChild(_staticLayer);
    auto size = _map->getMapSize();
    log("Tile size[%f,%f]", size.width, size.height);
    for (int i=0; i<size.width; i++) {
        for (int j=0; j<size.height; j++) {
            auto gid = _staticLayer->getTileGIDAt(Vec2(i,j))-1;//why gid is not correct here
            auto p = _map->getPropertiesForGID(gid);
            bool isSensor=false;
            if (!p.isNull()) {
                auto valuemap = p.asValueMap();
                if (valuemap.size()!=0) {
                    auto config = valuemap["isSensor"].asString();
                    log("gid[%d], isSensor[%s]", gid, config.c_str());
                    if (config == "true") isSensor=true;
                }
            }
            Rect rect = _physicBlock[gid];
            if (rect.origin==Point::ZERO && rect.size.width==0 && rect.size.height==0) {log("skip (%d,%d)", i, j); continue;}
            log("at (%d,%d) - gid[%d] isSensor[%d] - rect[%f,%f,%f,%f]", i, j, gid, isSensor, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
            Point origin;
            origin.x = i*_tileSize.width/_ratio;
            origin.y = (_tileSize.height*(_mapSize.height-1)-j*_tileSize.height)/_ratio;
            createB2StaticRect(origin+rect.origin+Point(16,0), rect.size, isSensor? kBoxSensor:kBoxGround);
        }
    }
    //add edgebox to avoid out of the map
    //left
    createB2StaticRect(Point(-_tileSize.width/2/_ratio, _tileSize.height*_mapSize.height/2/_ratio),
                       Size(_tileSize.width/_ratio, _tileSize.height*_mapSize.height/_ratio), kBoxGround);
    //right
    createB2StaticRect(Point(_tileSize.width*_mapSize.width/_ratio+_tileSize.width/2/_ratio, _tileSize.height*_mapSize.height/2/_ratio),
                       Size(_tileSize.width/_ratio, _tileSize.height*_mapSize.height/_ratio), kBoxGround);
    //top
    createB2StaticRect(Point(_tileSize.width*_mapSize.width/2/_ratio, (_tileSize.height*_mapSize.height+_tileSize.height/2)/_ratio),
                       Size(_tileSize.width*_mapSize.width/_ratio, _tileSize.height/_ratio), kBoxGround);
    //bottom
    createB2StaticRect(Point(_tileSize.width*_mapSize.width/2/_ratio, -_tileSize.height/2/_ratio),
                       Size(_tileSize.width*_mapSize.width/_ratio, _tileSize.height/_ratio), kBoxGround);
}
void HelloWorld::createB2StaticRect(Point pos, Size size, CollisionType type)
{
    auto sp = Sprite::create();
    auto body = PhysicsBody::createEdgeBox(size, PHYSICSBODY_MATERIAL_DEFAULT, 1);
    sp->setPosition(pos);
    sp->setPhysicsBody(body);
    this->addChild(sp);
    for (auto shape:body->getShapes()) {
        shape->setContactTestBitmask(0x1);
        shape->setRestitution(1.0);
        shape->setMass(0.0);
        shape->setFriction(0.00);
        if (type==kBoxSensor) {
            shape->setSensor(true);
        }
    }
}

void HelloWorld::initHero()
{
    _hero = Sprite::create("hero_stand_01.png");
    _hero->setPosition(Point(48, 54));
    auto body = PhysicsBody::createBox(Size(25,50));//(Size(32, 64), PHYSICSBODY_MATERIAL_DEFAULT, 1);
    _hero->setPhysicsBody(body);
    this->addChild(_hero);
    body->setRotationEnable(false);
    for (auto shape:body->getShapes()) {
        shape->setContactTestBitmask(0x1);
        shape->setRestitution(0.0);
        shape->setMass(0.0);
        shape->setFriction(0.00);
    }
}

void HelloWorld::onEnterTransitionDidFinish()
{
    listener = EventListenerTouchAllAtOnce::create();
    listener->onTouchesBegan = CC_CALLBACK_2(HelloWorld::onTouchesBegan, this);
    listener->onTouchesMoved = CC_CALLBACK_2(HelloWorld::onTouchesMoved, this);
    listener->onTouchesEnded = CC_CALLBACK_2(HelloWorld::onTouchesEnded, this);
    listener->onTouchesCancelled = CC_CALLBACK_2(HelloWorld::onTouchesCancelled, this);
    auto dispatcher = Director::getInstance()->getEventDispatcher();
    dispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    //keyboard
    keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(HelloWorld::onKeyPressed, this);
    keyListener->onKeyReleased = CC_CALLBACK_2(HelloWorld::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
    
    this->scheduleUpdate();
}

void HelloWorld::update(float dt)
{
    if (_hero) {
        this->setViewpointCenter(_hero->getPosition());
    }
}
void HelloWorld::onExitTransitionDidStart()
{
    this->unscheduleUpdate();
    Director::getInstance()->getEventDispatcher()->removeEventListener(listener);
    Director::getInstance()->getEventDispatcher()->removeEventListener(keyListener);
}
     
void HelloWorld::onTouchesBegan(const std::vector<Touch*>& touches, Event *event)
{
    
}
void HelloWorld::onTouchesMoved(const std::vector<Touch*>& touches, Event *event)
{
    
}
void HelloWorld::onTouchesEnded(const std::vector<Touch*>& touches, Event *event)
{
    
}
void HelloWorld::onTouchesCancelled(const std::vector<Touch*>& touches, Event *event)
{
    
}
     
void HelloWorld::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* unused_event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_A) {
        log("left");
        _hero->getPhysicsBody()->setVelocity(Vec2(-80.0,0));
        //_hero->getPhysicsBody()->applyForce(Vec2(-30, 0));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_D) {
        log("right");
        _hero->getPhysicsBody()->setVelocity(Vec2(80.0,0));
        //_hero->getPhysicsBody()->applyForce(Vect(30, 0));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_J) {
        log("jump");
        _hero->getPhysicsBody()->applyImpulse(Vect(0, 380));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_M) {
        if (_showDebugDraw) _world->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_NONE);
        else _world->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
        _showDebugDraw=!_showDebugDraw;
    }
}
void HelloWorld::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_A) {
        auto vec = _hero->getPhysicsBody()->getVelocity();
        vec.x=0;
        _hero->getPhysicsBody()->setVelocity(vec);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_D) {
        auto vec = _hero->getPhysicsBody()->getVelocity();
        vec.x=0;
        _hero->getPhysicsBody()->setVelocity(vec);
    }
}

void HelloWorld::setViewpointCenter(Point position)
{
    float mapWidth = _mapSize.width * _tileSize.width/_ratio;
    float mapHeight = _mapSize.height * _tileSize.height/_ratio;
    float offsetX = _tileSize.width*0;
    float offsetY = _tileSize.height*0;
    
    //log("offset[%f,%f]", offsetX, offsetY);
    //x
    int x;
    if(mapWidth < winSize.width) x=mapWidth/2;
    else{
        if(position.x > mapWidth-winSize.width/2+offsetX){
            x = MIN(position.x, mapWidth-winSize.width/2+offsetX);
        }else if(position.x < winSize.width/2-offsetX){
            x = MAX(position.x, winSize.width/2-offsetX);
        }else x=position.x;
    }
    
    //y
    int y;
    if(mapHeight < winSize.height) y=mapHeight/2;
    else{
        if(position.y > mapHeight-winSize.height/2+offsetY){
            y = MIN(position.y, mapHeight-winSize.height/2+offsetY);
        }else if(position.y < winSize.height/2-offsetY){
            y = MAX(position.y, winSize.height/2-offsetY);
        }else y=position.y;
    }
    
    Point actualPosition = Point(x, y);
    Point centerOfView = Point(winSize.width/2, winSize.height/2);
    Point viewPoint = centerOfView-actualPosition;
    this->setPosition(viewPoint);
}