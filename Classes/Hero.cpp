//
//  Hero.cpp
//  Assassinate
//
//  Created by zfu on 3/2/16.
//
//

#include "Hero.hpp"
#include "loguru.hpp"


bool Hero::init()
{
    Director::getInstance()->getTextureCache()->addImage("hero_stand_01.png");
    Director::getInstance()->getTextureCache()->addImage("hero_hung_01.png");
    Director::getInstance()->getTextureCache()->addImage("hero_staydown_01.png");
    mSprite=Sprite::createWithTexture(Director::getInstance()->getTextureCache()->getTextureForKey("hero_stand_01.png"));
    _state=kHeroState_stand;
    mSprite->retain();
    
    
    Vec2 hero[6]={Vec2(13,25), Vec2(13,-23), Vec2(10,-25), Vec2(-10,-25), Vec2(-13,-23), Vec2(-13,25)};
    auto body = PhysicsBody::createPolygon(hero, 6);
    mSprite->setPhysicsBody(body);
    body->setRotationEnable(false);
    body->getShapes().at(0)->setTag(kShapeTag_hero_stand);
    
    Vec2 hero_squat[6]={Vec2(13,13), Vec2(13,-11), Vec2(10,-13), Vec2(-10,-13), Vec2(-13,-11), Vec2(-13,13)};
    auto shape_squat = PhysicsShapePolygon::create(hero_squat, 6, PHYSICSSHAPE_MATERIAL_DEFAULT, Vec2(0,-12));
    shape_squat->setTag(kShapeTag_hero_squat);
    body->addShape(shape_squat);
    shape_squat->setSensor(true);
    
    auto shape_hung_box = PhysicsShapeBox::create(Size(26, 10), PHYSICSSHAPE_MATERIAL_DEFAULT, Vec2(0, 38));
    shape_hung_box->setSensor(true);
    shape_hung_box->setTag(kShapeTag_hero_hung_box);
    body->addShape(shape_hung_box);
    for (auto shape:body->getShapes()) {
        shape->setContactTestBitmask(0x1);
        shape->setRestitution(0.0);
        shape->setMass(0.0);
        shape->setFriction(0.00);
    }
    return true;
}

Hero::~Hero()
{
    mSprite->release();
}

kHeroState Hero::getState()
{
    return _state;
}

void Hero::changeHeroState(kHeroState state)
{
    LOG_F(INFO,"try change hero state from[%s] --> [%s]", heroStateStr[_state].c_str(), heroStateStr[state].c_str());
    if (_state == state) return;
    LOG_F(INFO,"ok change hero state from[%s] --> [%s]", heroStateStr[_state].c_str(), heroStateStr[state].c_str());
    _state=state;
    if (_state==kHeroState_stand) {
        mSprite->getPhysicsBody()->getShape(kShapeTag_hero_stand)->setSensor(false);
        mSprite->getPhysicsBody()->getShape(kShapeTag_hero_squat)->setSensor(true);
        //mSprite->getPhysicsBody()->getShape(kShapeTag_hero_hung_box)->setSensor(true);
    }else if (_state==kHeroState_hung) {
        //mSprite->getPhysicsBody()->getShape(kShapeTag_hero_hung_box)->setSensor(false);
        mSprite->getPhysicsBody()->getShape(kShapeTag_hero_stand)->setSensor(false);
        mSprite->getPhysicsBody()->getShape(kShapeTag_hero_squat)->setSensor(true);
    } else if (_state==kHeroState_squat) {
        mSprite->getPhysicsBody()->getShape(kShapeTag_hero_squat)->setSensor(false);
        mSprite->getPhysicsBody()->getShape(kShapeTag_hero_stand)->setSensor(true);
        //mSprite->getPhysicsBody()->getShape(kShapeTag_hero_hung_box)->setSensor(true);
    }
    if (_state==kHeroState_hung) {
        mSprite->setTexture(Director::getInstance()->getTextureCache()->getTextureForKey("hero_hung_01.png"));
    } else if (_state==kHeroState_stand) {
        mSprite->setTexture(Director::getInstance()->getTextureCache()->getTextureForKey("hero_stand_01.png"));
    } else if (_state==kHeroState_squat) {
        mSprite->setTexture(Director::getInstance()->getTextureCache()->getTextureForKey("hero_staydown_01.png"));
    }
}