#include "HelloWorldScene.h"
#include "cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include "tinyxml2/tinyxml2.h"

USING_NS_CC;

using namespace cocostudio::timeline;

const std::string mapTypeStr[] = {
    "ground",
    "wall",
    "hung",
    "stage",
    "empty",
    "harm"
};

const std::string shapeTypeStr[] = {
    "normal",
    "round",
    "down"
};

const std::string heroStateStr[] = {
    "stand",
    "hung",
    "squat",
    "hero_stand",
    "hero_squat",
    "hero_hung"
};

const bool isSensor[] = {
    false,
    false,
    true,
    true,
    false,
    true
};

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
    if ( !Layer::init() )
    {
        return false;
    }
    _key=0;
    winSize = Director::getInstance()->getVisibleSize();
    
    _ratio = Director::getInstance()->getContentScaleFactor();
    initMap();
    initHero();
    return true;
}

HelloWorld::~HelloWorld()
{
    _map->release();
    for (int i=0; i<_mapSize.width; i++) {
        free(_visited[i]);
    }
    free(_visited);
}

void HelloWorld::initMap()
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
    log("size of _physicBlock is %lu", _physicBlock.size());
    while (tile)
    {
        int gid;
        int id;
        float x,y,width,height;
        auto err = tile->QueryIntAttribute("id", &gid);
        log("tile , query gid[%d]", gid);
        auto obj_or_prop=tile->FirstChildElement();
        //bool isSensor;
        if (obj_or_prop->FirstChildElement() == nullptr) {
            tile=tile->NextSiblingElement();
            continue;
        }
        log("first obj_or_prop[%s]", obj_or_prop->Name());
        tinyxml2::XMLElement *tile_obj=nullptr;
        if (strcmp(obj_or_prop->Name(), "objectgroup")==0) {
            tile_obj=obj_or_prop->FirstChildElement();
            assert(tile_obj!=nullptr);
        } else {
            auto prop = obj_or_prop->FirstChildElement();
            log("prop name[%s]", prop->Name());
            int type=false;
            prop->QueryIntAttribute("value", &type);
            log("gid[%d] type[%s]", gid, mapTypeStr[type].c_str());
            _physicBlockType[gid]=type;
            auto n = obj_or_prop->NextSiblingElement();
            if (n == nullptr) {
                tile=tile->NextSiblingElement();
                continue;
            }
            log("n name[%s]", n->Name());
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
        //log("ele gid[%d] id[%d],[%f, %f, %f, %f]", gid, id, x, y, width, height);
        //because center position is not 0,0, it is 0.5, 0.5
        _physicBlock[gid]=Rect(x/_ratio,(_tileSize.height-y-height/2)/_ratio,width/_ratio,height/_ratio);
        log("ele gid[%d] id[%d],[%f, %f, %f, %f]", gid, id,
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
    //log("Tile size[%f,%f]", size.width, size.height);
    //log("size of _physicBlock is %lu", _physicBlock.size());
    for (int i=0; i<size.width; i++) {
        for (int j=0; j<size.height; j++) {
            if (_visited[i][j]==kMapVisit_YES) continue;
            auto gid = _staticLayer->getTileGIDAt(Vec2(i,j))-1;//why gid is not correct here
            auto p = _map->getPropertiesForGID(gid);
            Rect rect = _physicBlock[gid];
            int type = _physicBlockType[gid];
            if (rect.origin==Point::ZERO && rect.size.width==0 && rect.size.height==0) {
                log("skip (%d,%d)", i, j);
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
                log("at (%d,%d) - gid[%d] type[%s] - right[%d] - rect[%f,%f,%f,%f]",
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
                log("at (%d,%d) - gid[%d] type[%s] - right[%d] - rect[%f,%f,%f,%f]",
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
            log("at (%d,%d) - gid[%d] type[%s] - rect[%f,%f,%f,%f]",
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
void HelloWorld::createB2StaticRect(Point pos, Size size, int tag)
{
    auto sp = Sprite::create();
    sp->setTag(tag);
    auto body = PhysicsBody::createEdgeBox(size, PHYSICSBODY_MATERIAL_DEFAULT, 1);
    body->getShapes().at(0)->setTag(kShapeTag_normal);
    sp->setPosition(pos);
    sp->setPhysicsBody(body);
    if (_physicBlockType[tag]==kMapType_stage || _physicBlockType[tag]==kMapType_hung) {
        PhysicsShape *roundshape=PhysicsShapeBox::create(Size(size.width+8, size.height+8));
        roundshape->setTag(kShapeTag_round);
        roundshape->setSensor(true);
        body->addShape(roundshape);
        
        if (_physicBlockType[tag]==kMapType_hung) {
            PhysicsShape *downshape=PhysicsShapeBox::create(Size(size.width, 8), PHYSICSBODY_MATERIAL_DEFAULT, Vec2(0, -60));
            downshape->setTag(kShapeTag_down);
            downshape->setSensor(true);
            body->addShape(downshape);
        }
    }
    this->addChild(sp);
    for (auto shape:body->getShapes()) {
        shape->setContactTestBitmask(0x1);
        shape->setRestitution(1.0);
        shape->setMass(0.0);
        shape->setFriction(0.00);
        if (isSensor[_physicBlockType[tag]]) {
            shape->setSensor(true);
        }
    }
}

void HelloWorld::initHero()
{
    _state=kHeroState_stand;
    _hero = Sprite::create("hero_stand_01.png");
    _hero->setPosition(Point(48, 54));
    _hero->setTag(-1);
    Vec2 hero[6]={Vec2(13,25), Vec2(13,-23), Vec2(10,-25), Vec2(-10,-25), Vec2(-13,-23), Vec2(-13,25)};
    auto body = PhysicsBody::createPolygon(hero, 6);
    _hero->setPhysicsBody(body);
    this->addChild(_hero);
    body->setRotationEnable(false);
    body->getShapes().at(0)->setTag(kShapeTag_hero_stand);
    
    Vec2 hero_squat[6]={Vec2(13,13), Vec2(13,-11), Vec2(10,-13), Vec2(-10,-13), Vec2(-13,-11), Vec2(-13,13)};
    auto shape_squat = PhysicsShapePolygon::create(hero_squat, 6, PHYSICSSHAPE_MATERIAL_DEFAULT, Vec2(0,-12));
    shape_squat->setTag(kShapeTag_hero_squat);
    body->addShape(shape_squat);
    shape_squat->setSensor(true);
    for (auto shape:body->getShapes()) {
        shape->setContactTestBitmask(0x1);
        shape->setRestitution(0.0);
        shape->setMass(0.0);
        shape->setFriction(0.00);
    }
}

void HelloWorld::onEnterTransitionDidFinish()
{
    //_world->addJoint(_join);
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
    
    //physical contact
    auto contact_listener = EventListenerPhysicsContact::create();
    contact_listener->onContactBegin = CC_CALLBACK_1(HelloWorld::onContactBegin, this);
    contact_listener->onContactSeparate = CC_CALLBACK_1(HelloWorld::onContactSeparate, this);
    dispatcher->addEventListenerWithSceneGraphPriority(contact_listener, this);
    this->scheduleUpdate();
}

bool HelloWorld::onContactBegin(const PhysicsContact &contact)
{
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
    
    log("contact begin gidA[%d] gidB[%d], shapeTagA[%s] shapeTagB[%s], blockTypeA[%s] blockTypeB[%s]"
        "PositionA[%f,%f], PositionB[%f,%f]",
        gidA, gidB,
        (gidA==-1)? "Hero":shapeTypeStr[shapeTagA].c_str(),
        (gidB==-1)? "Hero":shapeTypeStr[shapeTagB].c_str(),
        (gidA==-1)? "Hero":mapTypeStr[_physicBlockType[gidA]].c_str(),
        (gidB==-1)? "Hero":mapTypeStr[_physicBlockType[gidB]].c_str(),
        spA->getPosition().x, spA->getPosition().y,
        spB->getPosition().x, spB->getPosition().y);
    
    //hero colission with hung normal from up direction
    if (gidA == HERO_GID && (_physicBlockType[gidB]==kMapType_stage || _physicBlockType[gidB]==kMapType_hung)
        && shapeTagB==kShapeTag_normal
        && spA->getPosition().y-sizeA.height/2+8 > spB->getPosition().y) {
        bodyB->getShape(kShapeTag_normal)->setSensor(false);
    }
    if (gidB == HERO_GID && (_physicBlockType[gidA]==kMapType_stage || _physicBlockType[gidA]==kMapType_hung)
        && shapeTagA==kShapeTag_normal
        && spB->getPosition().y-sizeB.height/2+8 > spA->getPosition().y) {
        bodyA->getShape(kShapeTag_normal)->setSensor(false);
    }
    //try if need hung
    if (gidA == HERO_GID && _physicBlockType[gidB] == kMapType_hung
        && shapeTagB==kShapeTag_round
        && spA->getPosition().y < spB->getPosition().y) {
        log("try hung A");
        bodyB->getShape(kShapeTag_down)->setSensor(false);
    }
    if (gidB == HERO_GID && _physicBlockType[gidA] == kMapType_hung
        && shapeTagA==kShapeTag_round
        && spA->getPosition().y < spB->getPosition().y) {
        log("try hung B");
        bodyA->getShape(kShapeTag_down)->setSensor(false);
    }
    return true;
}

bool HelloWorld::onContactSeparate(const PhysicsContact &contact)
{
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
    
    log("contact seperate gidA[%d] gidB[%d], shapeTagA[%s] shapeTagB[%s], blockTypeA[%s] blockTypeB[%s], "
        "PositionA[%f,%f], PositionB[%f,%f]",
        gidA, gidB,
        (gidA==-1)? "Hero":shapeTypeStr[shapeTagA].c_str(),
        (gidB==-1)? "Hero":shapeTypeStr[shapeTagB].c_str(),
        (gidA==-1)? "Hero":mapTypeStr[_physicBlockType[gidA]].c_str(),
        (gidB==-1)? "Hero":mapTypeStr[_physicBlockType[gidB]].c_str(),
        spA->getPosition().x, spA->getPosition().y,
        spB->getPosition().x, spB->getPosition().y);
    
    //hero seperate colission with hung round
    if (gidA == HERO_GID
        && (_physicBlockType[gidB]==kMapType_stage || _physicBlockType[gidB]==kMapType_hung)
        && shapeTagB==kShapeTag_round
        && shapeB->isSensor()) {
        bodyB->getShape(kShapeTag_normal)->setSensor(true);
    }
    if (gidB == HERO_GID
        && (_physicBlockType[gidA]==kMapType_stage || _physicBlockType[gidA]==kMapType_hung)
        && shapeTagA==kShapeTag_round
        && shapeA->isSensor()) {
        bodyA->getShape(kShapeTag_normal)->setSensor(true);
    }
    return true;
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
        _key |= BUTTON_L;
        _hero->getPhysicsBody()->setVelocity(Vec2(-80.0,0));
        _hero->getPhysicsBody()->applyForce(Vec2(-80, 0));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_D) {
        log("right");
        _key |= BUTTON_R;
        _hero->getPhysicsBody()->setVelocity(Vec2(80.0,0));
        _hero->getPhysicsBody()->applyForce(Vect(80, 0));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_J) {
        log("jump");
        _key |= BUTTON_A;
        _hero->getPhysicsBody()->applyImpulse(Vect(0, 380));
    } else if (keyCode == EventKeyboard::KeyCode::KEY_M) {
        if (_showDebugDraw) _world->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_NONE);
        else _world->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
        _showDebugDraw=!_showDebugDraw;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_W) {
        _key |= BUTTON_U;
        changeHeroState(kHeroState_stand);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_S) {
        _key |= BUTTON_D;
        log("down");
        changeHeroState(kHeroState_squat);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_J) {
        _key |= BUTTON_A;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_K) {
        _key |= BUTTON_B;
    }
    log("press key[0x%x]", _key);
}
void HelloWorld::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_A) {
        _key &= ~BUTTON_L;
        auto vec = _hero->getPhysicsBody()->getVelocity();
        _hero->getPhysicsBody()->resetForces();
        vec.x=0;
        _hero->getPhysicsBody()->setVelocity(vec);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_D) {
        _key &= ~BUTTON_R;
        auto vec = _hero->getPhysicsBody()->getVelocity();
        _hero->getPhysicsBody()->resetForces();
        vec.x=0;
        _hero->getPhysicsBody()->setVelocity(vec);
    } else if (keyCode == EventKeyboard::KeyCode::KEY_W) {
        _key &= ~BUTTON_U;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_S) {
        _key &= ~BUTTON_D;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_J) {
        _key &= ~BUTTON_A;
    } else if (keyCode == EventKeyboard::KeyCode::KEY_K) {
        _key &= ~BUTTON_B;
    }
    log("release key[0x%x]", _key);
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

void HelloWorld::changeHeroState(kHeroState state)
{
    if (_state == state) return;
    log("change hero state from[%s] --> [%s]", heroStateStr[_state].c_str(), heroStateStr[state].c_str());
    _state=state;
    if (_state==kHeroState_stand) {
        _hero->getPhysicsBody()->getShape(kShapeTag_hero_stand)->setSensor(false);
        _hero->getPhysicsBody()->getShape(kShapeTag_hero_squat)->setSensor(true);
    } else if (_state==kHeroState_squat) {
        _hero->getPhysicsBody()->getShape(kShapeTag_hero_stand)->setSensor(true);
        _hero->getPhysicsBody()->getShape(kShapeTag_hero_squat)->setSensor(false);
    }
}
