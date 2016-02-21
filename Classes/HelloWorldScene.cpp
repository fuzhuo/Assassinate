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
    layer->_world->setGravity(Vec2(0.0, -200.0));

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
    
//    createB2StaticRect(Point(20,15), Size(40,30), kBoxGround);
    return true;
}

void HelloWorld::initMap()
{
    std::string path = FileUtils::getInstance()->fullPathForFilename("Assassinate_map01.tmx");
    _map = TMXTiledMap::create(path);
    _map->retain();
    _mapSize=_map->getMapSize();
    _tileSize=_map->getTileSize();
    _staticLayer = _map->getLayer(STATIC_LAYER);
    
    //xml
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
        _physicBlock[gid]=Rect((x+width/2)/_ratio,(_tileSize.height-y-height/2)/_ratio,width/_ratio,height/_ratio);
        tile=tile->NextSiblingElement();
    }
    assert(pDoc!=nullptr);
    delete pDoc;
    
    this->addChild(_staticLayer);
    auto size = _map->getMapSize();
    log("Tile size[%f,%f]", size.width, size.height);
    for (int i=0; i<size.width; i++) {
        for (int j=0; j<size.height; j++) {
            auto gid = _staticLayer->getTileGIDAt(Vec2(i,j))-1;
            Rect rect = _physicBlock[gid];
            if (rect.origin==Point::ZERO && rect.size.width==0 && rect.size.height==0) {log("skip (%d,%d)", i, j); continue;}
            log("at (%d,%d) - gid[%d] - rect[%f,%f,%f,%f]", i, j, gid, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
            Point origin;
            origin.x = i*_tileSize.width/_ratio;
            origin.y = (_tileSize.height*_mapSize.height-j*_tileSize.height)/_ratio;
            createB2StaticRect(origin+rect.origin-Point(0,32), rect.size, kBoxGround);
        }                  
    }
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
        shape->setRestitution(0.0);
        shape->setMass(0.0);
        shape->setFriction(0.0);
        if (type==kBoxSensor) {
            shape->setSensor(true);
        }
    }
}
HelloWorld::~HelloWorld()
{
    _map->release();
}