#include "Playground.h"
#include "cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include "tinyxml2/tinyxml2.h"

USING_NS_CC;

using namespace cocostudio::timeline;

#define LOGURU_IMPLEMENTATION
#include <loguru.hpp>

Scene* Playground::createScene()
{
    // 'scene' is an autorelease object
    //auto scene = Scene::create();
    auto scene = Scene::createWithPhysics();
    scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
    
    // 'layer' is an autorelease object
    auto layer = Playground::create();
    layer->_world = scene->getPhysicsWorld();
    layer->_world->setGravity(Vec2(0.0, -980));
    layer->_showDebugDraw=true;

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool Playground::init()
{
    if ( !Layer::init() )
    {
        return false;
    }
    _key=0;
    winSize = Director::getInstance()->getVisibleSize();
    
    //loguru::add_file("~/testlog.txt", loguru::Truncate, loguru::Verbosity_INFO);
    LOG_SCOPE_FUNCTION(INFO);
    loguru::g_stderr_verbosity = 1;
    _ratio = Director::getInstance()->getContentScaleFactor();
    initMap();
    initHero();
    return true;
}

Playground::~Playground()
{
    _map->release();
    _hero->release();
    for (int i=0; i<_mapSize.width; i++) {
        free(_visited[i]);
    }
    free(_visited);
}

void Playground::initMap()
{
    std::string path = FileUtils::getInstance()->fullPathForFilename("Assassinate_map01.tmx");
    _map = TMXTiledMap::create(path);
    _map->retain();
    _mapSize=_map->getMapSize();
    _tileSize=_map->getTileSize();
    _staticLayer = _map->getLayer(STATIC_LAYER);
    _visited = (int**)malloc(sizeof(int*)*_mapSize.width);
    for (int i=0; i<_mapSize.width; i++) {
        _visited[i]=(int*)malloc(sizeof(int)*_mapSize.height);
    }
    for (int i=0; i<_mapSize.width; i++)
        for (int j=0; j<_mapSize.height; j++)
            _visited[i][j]=kMapVisit_NO;
    
    
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
    max+=1;
    _physicBlock.clear();
    _physicBlock=std::vector<Rect>(max);
    _physicBlockType.clear();
    _physicBlockType=std::vector<int>(max);
    LOG_F(INFO,"size of _physicBlock is %lu", _physicBlock.size());
    while (tile)
    {
        int gid;
        int id;
        float x,y,width,height;
        auto err = tile->QueryIntAttribute("id", &gid);
        //LOG_F(INFO,"tile , query gid[%d]", gid);
        auto obj_or_prop=tile->FirstChildElement();
        //bool isSensor;
        if (obj_or_prop->FirstChildElement() == nullptr) {
            tile=tile->NextSiblingElement();
            continue;
        }
        //LOG_F(INFO,"first obj_or_prop[%s]", obj_or_prop->Name());
        tinyxml2::XMLElement *tile_obj=nullptr;
        if (strcmp(obj_or_prop->Name(), "objectgroup")==0) {
            tile_obj=obj_or_prop->FirstChildElement();
            assert(tile_obj!=nullptr);
        } else {
            auto prop = obj_or_prop->FirstChildElement();
            //LOG_F(INFO,"prop name[%s]", prop->Name());
            int type=false;
            prop->QueryIntAttribute("value", &type);
            //LOG_F(INFO,"gid[%d] type[%s]", gid, mapTypeStr[type].c_str());
            _physicBlockType[gid]=type;
            auto n = obj_or_prop->NextSiblingElement();
            if (n == nullptr) {
                tile=tile->NextSiblingElement();
                continue;
            }
            //LOG_F(INFO,"n name[%s]", n->Name());
            tile_obj=n->FirstChildElement();
            if (tile_obj == nullptr) {
                tile=tile->NextSiblingElement();
                continue;
            }
        }
        assert(tile_obj!=nullptr);
        err = tile_obj->QueryIntAttribute("id", &id);
        assert(err == tinyxml2::XML_NO_ERROR);
        err = tile_obj->QueryFloatAttribute("x", &x);
        assert(err == tinyxml2::XML_NO_ERROR);
        err = tile_obj->QueryFloatAttribute("y", &y);
        assert(err == tinyxml2::XML_NO_ERROR);
        err = tile_obj->QueryFloatAttribute("width", &width);
        assert(err == tinyxml2::XML_NO_ERROR);
        err = tile_obj->QueryFloatAttribute("height", &height);
        assert(err == tinyxml2::XML_NO_ERROR);
        //because center position is not 0,0, it is 0.5, 0.5
        _physicBlock[gid]=Rect(x/_ratio,(_tileSize.height-y-height/2)/_ratio,width/_ratio,height/_ratio);
        
        LOG_F(2,"ele gid[%d] id[%d],[%f, %f, %f, %f]", gid, id,
            _physicBlock[gid].origin.x, _physicBlock[gid].origin.y,
            _physicBlock[gid].size.width, _physicBlock[gid].size.height);
        tile=tile->NextSiblingElement();
    }
    assert(pDoc!=nullptr);
    delete pDoc;
    
    if (_staticLayer->getParent()) {
        _staticLayer->removeFromParent();
    }
    this->addChild(_staticLayer);
    auto size = _map->getMapSize();
    //LOG_F(INFO,"Tile size[%f,%f]", size.width, size.height);
    //LOG_F(INFO,"size of _physicBlock is %lu", _physicBlock.size());
    for (int i=0; i<size.width; i++) {
        for (int j=0; j<size.height; j++) {
            if (_visited[i][j]==kMapVisit_YES) continue;
            auto gid = _staticLayer->getTileGIDAt(Vec2(i,j))-1;//why gid is not correct here
            auto p = _map->getPropertiesForGID(gid);
            Rect rect = _physicBlock[gid];
            int type = _physicBlockType[gid];
            if (rect.origin==Point::ZERO && rect.size.width==0 && rect.size.height==0) {
                //LOG_F(INFO,"skip (%d,%d)", i, j);
                _visited[i][j]=kMapVisit_YES;
                continue;
            }
            //Need do merge here
            //merge ground
            if (type == kMapType_ground || type== kMapType_hung) { //right
                int right=0;
                int k=i+1;
                while (k<_mapSize.width) {
                    auto gid = _staticLayer->getTileGIDAt(Vec2(k,j))-1;//why gid is not correct here
                    int t = _physicBlockType[gid];
                    if (type==t) {
                        _visited[k][j]=kMapVisit_YES;
                        k++;right++;
                    }
                    else break;
                }
                /*
                LOG_F(INFO,"at (%d,%d) - gid[%d] type[%s] - right[%d] - rect[%f,%f,%f,%f]",
                    i, j,
                    gid, mapTypeStr[type].c_str(),
                    right,
                    rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
                 */
                //create the whole box
                Point origin;
                origin.x = i*_tileSize.width/_ratio;
                origin.y = (_tileSize.height*(_mapSize.height-1)-j*_tileSize.height)/_ratio;
                rect.size.width*=(right+1);
                createB2StaticRect(origin+rect.origin+Point(rect.size.width/2,0), rect.size, gid);
                continue;//merged ok
            } else if (type == kMapType_wall) { //down
                int down=0;
                int k=j+1;
                while (k<_mapSize.height) {
                    auto gid = _staticLayer->getTileGIDAt(Vec2(i,k))-1;//why gid is not correct here
                    int t = _physicBlockType[gid];
                    if (type==t) {
                        _visited[i][k]=kMapVisit_YES;
                        k++;down++;
                    }
                    else break;
                }
                /*
                LOG_F(INFO,"at (%d,%d) - gid[%d] type[%s] - right[%d] - rect[%f,%f,%f,%f]",
                    i, j,
                    gid, mapTypeStr[type].c_str(),
                    down,
                    rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
                 */
                //create the whole box
                Point origin;
                origin.x = i*_tileSize.width/_ratio;
                origin.y = (_tileSize.height*(_mapSize.height-1)-j*_tileSize.height)/_ratio;
                rect.size.height*=(down+1);
                createB2StaticRect(origin+rect.origin+Point(rect.size.width/2,-rect.size.height/2+16), rect.size, gid);
                continue;//merged ok
            }
            _visited[i][j]=kMapVisit_YES;
            /*
            LOG_F(INFO,"at (%d,%d) - gid[%d] type[%s] - rect[%f,%f,%f,%f]",
                i, j,
                gid, mapTypeStr[type].c_str(),
                rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
             */
            Point origin;
            origin.x = i*_tileSize.width/_ratio;
            origin.y = (_tileSize.height*(_mapSize.height-1)-j*_tileSize.height)/_ratio;
            createB2StaticRect(origin+rect.origin+Point(16,0), rect.size, gid);
        }
    }
    //add edgebox to avoid out of the map
    //left
    createB2StaticRect(Point(-_tileSize.width/2/_ratio, _tileSize.height*_mapSize.height/2/_ratio),
                       Size(_tileSize.width/_ratio, _tileSize.height*_mapSize.height/_ratio), kMapType_ground);
    //right
    createB2StaticRect(Point(_tileSize.width*_mapSize.width/_ratio+_tileSize.width/2/_ratio, _tileSize.height*_mapSize.height/2/_ratio),
                       Size(_tileSize.width/_ratio, _tileSize.height*_mapSize.height/_ratio), kMapType_ground);
    //top
    createB2StaticRect(Point(_tileSize.width*_mapSize.width/2/_ratio, (_tileSize.height*_mapSize.height+_tileSize.height/2)/_ratio),
                       Size(_tileSize.width*_mapSize.width/_ratio, _tileSize.height/_ratio), kMapType_ground);
    //bottom
    createB2StaticRect(Point(_tileSize.width*_mapSize.width/2/_ratio, -_tileSize.height/2/_ratio),
                       Size(_tileSize.width*_mapSize.width/_ratio, _tileSize.height/_ratio), kMapType_ground);
}
void Playground::createB2StaticRect(Point pos, Size size, int tag)
{
    auto sp = Sprite::create();
    sp->setTag(tag);
    auto body = PhysicsBody::createEdgeBox(size, PHYSICSBODY_MATERIAL_DEFAULT, 1);
    body->getShapes().at(0)->setTag(kShapeTag_normal);
    sp->setPosition(pos);
    sp->setPhysicsBody(body);
    if (_physicBlockType[tag]==kMapType_stage || _physicBlockType[tag]==kMapType_hung) {
        PhysicsShape *roundshape=PhysicsShapeBox::create(Size(size.width+10, size.height+16));
        roundshape->setTag(kShapeTag_round);
        roundshape->setSensor(true);
        body->addShape(roundshape);
    }
    this->addChild(sp);
    for (auto shape:body->getShapes()) {
        shape->setContactTestBitmask(0x1);
        shape->setRestitution(0.0);
        shape->setMass(0.0);
        shape->setFriction(0.00);
        if (isSensor[_physicBlockType[tag]]) {
            shape->setSensor(true);
        }
    }
}

void Playground::initHero()
{
    _hero = Hero::create();
    _hero->retain();
    _heroSprite = _hero->mSprite;
    _heroSprite->setPosition(Point(48, 54));
    _heroSprite->setTag(-1);
    this->addChild(_heroSprite);
}

void Playground::onEnterTransitionDidFinish()
{
    //_world->addJoint(_join);
    listener = EventListenerTouchAllAtOnce::create();
    listener->onTouchesBegan = CC_CALLBACK_2(Playground::onTouchesBegan, this);
    listener->onTouchesMoved = CC_CALLBACK_2(Playground::onTouchesMoved, this);
    listener->onTouchesEnded = CC_CALLBACK_2(Playground::onTouchesEnded, this);
    listener->onTouchesCancelled = CC_CALLBACK_2(Playground::onTouchesCancelled, this);
    auto dispatcher = Director::getInstance()->getEventDispatcher();
    dispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    //keyboard
    keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(Playground::onKeyPressed, this);
    keyListener->onKeyReleased = CC_CALLBACK_2(Playground::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
    
    //physical contact
    auto contact_listener = EventListenerPhysicsContact::create();
    contact_listener->onContactBegin = CC_CALLBACK_1(Playground::onContactBegin, this);
    contact_listener->onContactSeparate = CC_CALLBACK_1(Playground::onContactSeparate, this);
    dispatcher->addEventListenerWithSceneGraphPriority(contact_listener, this);
    this->scheduleUpdate();
}

bool Playground::onContactBegin(const PhysicsContact &contact)
{
    //LOG_SCOPE_FUNCTION(INFO);
    auto shapeA = contact.getShapeA();
    auto shapeB = contact.getShapeB();
    auto bodyA = shapeA->getBody();
    auto bodyB = shapeB->getBody();
    Sprite *spA = dynamic_cast<Sprite*>(contact.getShapeA()->getBody()->getNode());
    Sprite *spB = dynamic_cast<Sprite*>(contact.getShapeB()->getBody()->getNode());
    auto sizeA = spA->getTextureRect().size;
    auto sizeB = spB->getTextureRect().size;
    int gidA = spA->getTag();
    int gidB = spB->getTag();
    auto shapeTagA = shapeA->getTag();
    auto shapeTagB = shapeB->getTag();
    
    //skip hero sensor test, to limited verbose logs
    if (gidA == HERO_GID && shapeA->isSensor()) return true;
    if (gidB == HERO_GID && shapeB->isSensor()) return true;
    log("CONTACT gid[%2d %2d], shapeTagA[%6s %6s], blockType[%s %s], "
          "sensorA[%s %s], "
          "A[%f,%f] B[%f,%f]",
        gidA, gidB,
        (gidA==-1)? (shapeTypeStr[shapeTagA]+"_Hero").c_str():shapeTypeStr[shapeTagA].c_str(),
        (gidB==-1)? (shapeTypeStr[shapeTagB]+"_Hero").c_str():shapeTypeStr[shapeTagB].c_str(),
        (gidA==-1)? "Hero":mapTypeStr[_physicBlockType[gidA]].c_str(),
        (gidB==-1)? "Hero":mapTypeStr[_physicBlockType[gidB]].c_str(),
        shapeA->isSensor()? "true":"false",
        shapeB->isSensor()? "true":"false",
        spA->getPosition().x, spA->getPosition().y,
        spB->getPosition().x, spB->getPosition().y);
    
    //hero stand on ground
    if (gidA == HERO_GID
        && _physicBlockType[gidB]==kMapType_ground) {
        _currentStandBody=bodyB;
    }
    if (gidB == HERO_GID
        && _physicBlockType[gidA]==kMapType_ground) {
        _currentStandBody=bodyA;
    }
    
    //hero colission with hung normal from up direction
    if (gidA == HERO_GID && (_physicBlockType[gidB]==kMapType_stage || _physicBlockType[gidB]==kMapType_hung)
        && shapeTagB==kShapeTag_round
        && spA->getPosition().y-sizeA.height/2+8 > spB->getPosition().y) {
        LOG_F(INFO, "up contact round, set normal not sensor");
        bodyB->getShape(kShapeTag_normal)->setSensor(false);
        _currentStandBody=bodyB;
    }
    if (gidB == HERO_GID && (_physicBlockType[gidA]==kMapType_stage || _physicBlockType[gidA]==kMapType_hung)
        && shapeTagA==kShapeTag_round
        && spB->getPosition().y-sizeB.height/2+8 > spA->getPosition().y) {
        LOG_F(INFO, "up contact round, set normal not sensor");
        _currentStandBody=bodyA;
        bodyA->getShape(kShapeTag_normal)->setSensor(false);
    }
    //try if need hung
    /*
    if (gidA == HERO_GID && _physicBlockType[gidB] == kMapType_hung
        && shapeTagB==kShapeTag_round
        && spA->getPosition().y + sizeA.height/2 - 8< spB->getPosition().y) {
        if (_hero->getState() != kHeroState_hung) {
            LOG_F(INFO,"try hung A");
            bodyB->getShape(kShapeTag_down)->setSensor(false);
            bodyB->getShape(kShapeTag_normal)->setSensor(false);
            _currentStandBody=bodyB;
            _hero->changeHeroState(kHeroState_hung);
        }
    }
    if (gidB == HERO_GID && _physicBlockType[gidA] == kMapType_hung
        && shapeTagA==kShapeTag_round
        && spA->getPosition().y > spB->getPosition().y + sizeB.height/2 - 8) {
        if (_hero->getState() != kHeroState_hung) {
            LOG_F(INFO,"try hung B");
            bodyA->getShape(kShapeTag_down)->setSensor(false);
            bodyA->getShape(kShapeTag_normal)->setSensor(false);
            _currentStandBody=bodyA;
            _hero->changeHeroState(kHeroState_hung);
        }
    }
     */
    return true;
}

bool Playground::onContactSeparate(const PhysicsContact &contact)
{
    //LOG_SCOPE_FUNCTION(INFO);
    auto shapeA = contact.getShapeA();
    auto shapeB = contact.getShapeB();
    auto bodyA = shapeA->getBody();
    auto bodyB = shapeB->getBody();
    Sprite *spA = dynamic_cast<Sprite*>(contact.getShapeA()->getBody()->getNode());
    Sprite *spB = dynamic_cast<Sprite*>(contact.getShapeB()->getBody()->getNode());
    auto sizeA = spA->getTextureRect().size;
    auto sizeB = spB->getTextureRect().size;
    int gidA = spA->getTag();
    int gidB = spB->getTag();
    auto shapeTagA = shapeA->getTag();
    auto shapeTagB = shapeB->getTag();
    
    if (gidA == HERO_GID && shapeTagA!=kShapeTag_hero_hung_box && shapeA->isSensor()) return true;
    if (gidB == HERO_GID && shapeTagB!=kShapeTag_hero_hung_box && shapeB->isSensor()) return true;
    log("SEPARATE gid[%2d %2d], shapeTagA[%6s %6s], blockType[%s %s], "
          "sensorA[%s %s], "
          "A[%f,%f] B[%f,%f]",
        gidA, gidB,
        (gidA==-1)? (shapeTypeStr[shapeTagA]+"_Hero").c_str():shapeTypeStr[shapeTagA].c_str(),
        (gidB==-1)? (shapeTypeStr[shapeTagB]+"_Hero").c_str():shapeTypeStr[shapeTagB].c_str(),
        (gidA==-1)? "Hero":mapTypeStr[_physicBlockType[gidA]].c_str(),
        (gidB==-1)? "Hero":mapTypeStr[_physicBlockType[gidB]].c_str(),
        shapeA->isSensor()? "true":"false",
        shapeB->isSensor()? "true":"false",
        spA->getPosition().x, spA->getPosition().y,
        spB->getPosition().x, spB->getPosition().y);
    
    //hero seperate colission with hung round
    if (gidA == HERO_GID
        && (_physicBlockType[gidB]==kMapType_stage || _physicBlockType[gidB]==kMapType_hung)
        && shapeTagA==kShapeTag_hero_stand
        && shapeTagB==kShapeTag_round
        && shapeB->isSensor()
        && _hero->getState() != kHeroState_hung) {
        LOG_F(INFO, "leave a round");
        bodyB->getShape(kShapeTag_normal)->setSensor(true);
        _hero->changeHeroState(kHeroState_stand);
        return true;
    }
    if (gidB == HERO_GID
        && (_physicBlockType[gidA]==kMapType_stage || _physicBlockType[gidA]==kMapType_hung)
        && shapeTagB==kShapeTag_hero_stand
        && shapeTagA==kShapeTag_round
        && shapeA->isSensor()
        && _hero->getState() != kHeroState_hung) {
        LOG_F(INFO, "leave a round");
        bodyA->getShape(kShapeTag_normal)->setSensor(true);
        _hero->changeHeroState(kHeroState_stand);
        return true;
    }
    //seperate at hung state
    if (_hero->getState() == kHeroState_hung
        && gidA == HERO_GID
        && _physicBlockType[gidB]==kMapType_hung
        && shapeTagA==kShapeTag_hero_hung_box
        && shapeTagB==kShapeTag_round) {
        LOG_F(INFO, "separate from hung");
        bodyA->getShape(kShapeTag_hero_hung_box)->setSensor(true);
        bodyB->getShape(kShapeTag_normal)->setSensor(true);
        _hero->changeHeroState(kHeroState_stand);
        return true;
    }
    if (_hero->getState() == kHeroState_hung
        && gidB == HERO_GID
        && _physicBlockType[gidA]==kMapType_hung
        && shapeTagB==kShapeTag_hero_hung_box
        && shapeTagA==kShapeTag_round) {
        LOG_F(INFO, "separate from hung");
        bodyB->getShape(kShapeTag_hero_hung_box)->setSensor(true);
        bodyA->getShape(kShapeTag_normal)->setSensor(true);
        _hero->changeHeroState(kHeroState_stand);
        return true;
    }
    //do hung
    if (_hero->getState() != kHeroState_hung
        && gidA == HERO_GID
        && _physicBlockType[gidB]==kMapType_hung
        && shapeTagB==kShapeTag_normal
        && shapeTagA==kShapeTag_hero_stand
        && bodyB->getShape(kShapeTag_normal)->isSensor()
        && spA->getPosition().y < spB->getPosition().y) {
        LOG_F(INFO, "seperate and hung A");
        _hero->changeHeroState(kHeroState_hung);
        bodyB->getShape(kShapeTag_normal)->setSensor(false);
        bodyA->getShape(kShapeTag_hero_hung_box)->setSensor(false);
        _currentStandBody=bodyB;
        return true;
    }
    if (_hero->getState() != kHeroState_hung
        && gidB == HERO_GID
        && _physicBlockType[gidA]==kMapType_hung
        && shapeTagA==kShapeTag_normal
        && shapeTagB==kShapeTag_hero_stand
        && bodyA->getShape(kShapeTag_normal)->isSensor()
        && spB->getPosition().y < spA->getPosition().y) {
        LOG_F(INFO, "seperate and hung B");
        _hero->changeHeroState(kHeroState_hung);
        bodyA->getShape(kShapeTag_normal)->setSensor(false);
        bodyB->getShape(kShapeTag_hero_hung_box)->setSensor(false);
        _currentStandBody=bodyA;
        return true;
    }
    return true;
}

void Playground::update(float dt)
{
    if (_heroSprite) {
        this->setViewpointCenter(_heroSprite->getPosition());
    }
}
void Playground::onExitTransitionDidStart()
{
    this->unscheduleUpdate();
    Director::getInstance()->getEventDispatcher()->removeEventListener(listener);
    Director::getInstance()->getEventDispatcher()->removeEventListener(keyListener);
}
     
void Playground::onTouchesBegan(const std::vector<Touch*>& touches, Event *event)
{
    
}
void Playground::onTouchesMoved(const std::vector<Touch*>& touches, Event *event)
{
    
}
void Playground::onTouchesEnded(const std::vector<Touch*>& touches, Event *event)
{
    
}
void Playground::onTouchesCancelled(const std::vector<Touch*>& touches, Event *event)
{
    
}

void Playground::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* unused_event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_A) {
        LOG_F(INFO,"left");
        _key |= BUTTON_L;
        _heroSprite->getPhysicsBody()->setVelocity(Vec2(-80.0,0));
        _heroSprite->getPhysicsBody()->applyForce(Vec2(-80, 0));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_D) {
        LOG_F(INFO,"right");
        _key |= BUTTON_R;
        _heroSprite->getPhysicsBody()->setVelocity(Vec2(80.0,0));
        _heroSprite->getPhysicsBody()->applyForce(Vect(80, 0));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_W) {
        _key |= BUTTON_U;
        if (_hero->getState() == kHeroState_hung) {
            _heroSprite->getPhysicsBody()->applyImpulse(Vect(0, 380));
        } else {
            _hero->changeHeroState(kHeroState_stand);
        }
    } else if (keyCode == EventKeyboard::KeyCode::KEY_S) {
        _key |= BUTTON_D;
        LOG_F(INFO,"down");
        if (_hero->getState() == kHeroState_hung) {
            LOG_F(INFO, "exit hung and stand");
            _currentStandBody->getShape(kShapeTag_normal)->setSensor(true);
            _hero->changeHeroState(kHeroState_stand);
            _hero->mSprite->getPhysicsBody()->getShape(kShapeTag_hero_hung_box)->setSensor(true);
            _currentStandBody=nullptr;
        } else if (_hero->getState() == kHeroState_squat) {
            auto gid = _currentStandBody->getNode()->getTag();
            LOG_F(INFO, "exit from squat gid[%d]", gid);
            if (_physicBlockType[gid]==kMapType_stage) {
                LOG_F(INFO, "real exit from squat, down and standup");
                _currentStandBody->getShape(kShapeTag_normal)->setSensor(true);
                _hero->changeHeroState(kHeroState_stand);
            } else if (_physicBlockType[gid]==kMapType_hung) {
                LOG_F(INFO, "real exit from squat, down and hung");
                _currentStandBody->getShape(kShapeTag_normal)->setSensor(true);
                _hero->changeHeroState(kHeroState_stand);
            }
        } else {
            _hero->changeHeroState(kHeroState_squat);
        }
    } else if (keyCode == EventKeyboard::KeyCode::KEY_J) {
        LOG_F(INFO,"jump");
        _key |= BUTTON_A;
        if (_hero->getState() == kHeroState_hung) {
            LOG_F(INFO, "jump from hung");
            _currentStandBody->getShape(kShapeTag_normal)->setSensor(true);
            _heroSprite->getPhysicsBody()->applyImpulse(Vect(0, 400));
        } else if (_hero->getState() != kHeroState_squat) {
            LOG_F(INFO, "jump from none squat");
            _heroSprite->getPhysicsBody()->applyImpulse(Vect(0, 400));
        }
    } else if (keyCode == EventKeyboard::KeyCode::KEY_M) {
        if (_showDebugDraw) _world->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_NONE);
        else _world->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
        _showDebugDraw=!_showDebugDraw;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_K) {
        _key |= BUTTON_B;
    }
    //LOG_F(INFO,"press key[0x%x]", _key);
}
void Playground::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_A) {
        _key &= ~BUTTON_L;
        auto vec = _heroSprite->getPhysicsBody()->getVelocity();
        _heroSprite->getPhysicsBody()->resetForces();
        vec.x=0;
        _heroSprite->getPhysicsBody()->setVelocity(vec);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_D) {
        _key &= ~BUTTON_R;
        auto vec = _heroSprite->getPhysicsBody()->getVelocity();
        _heroSprite->getPhysicsBody()->resetForces();
        vec.x=0;
        _heroSprite->getPhysicsBody()->setVelocity(vec);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_W) {
        _key &= ~BUTTON_U;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_S) {
        _key &= ~BUTTON_D;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_J) {
        _key &= ~BUTTON_A;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_K) {
        _key &= ~BUTTON_B;
    }
    //LOG_F(INFO,"release key[0x%x]", _key);
}

void Playground::setViewpointCenter(Point position)
{
    float mapWidth = _mapSize.width * _tileSize.width/_ratio;
    float mapHeight = _mapSize.height * _tileSize.height/_ratio;
    float offsetX = _tileSize.width*0;
    float offsetY = _tileSize.height*0;
    
    //LOG_F(INFO,"offset[%f,%f]", offsetX, offsetY);
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
