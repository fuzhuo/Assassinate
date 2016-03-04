//
//  Hero.hpp
//  Assassinate
//
//  Created by zfu on 3/2/16.
//
//

#ifndef Hero_hpp
#define Hero_hpp

#include <stdio.h>
#include <cocos2d.h>
#include "common.h"

USING_NS_CC;

typedef enum{
    kHeroState_stand,
    kHeroState_hung,
    kHeroState_squat
}kHeroState;

class Hero : public Ref
{
public:
    CREATE_FUNC(Hero);
    ~Hero();
    bool init();
    Sprite *mSprite;
    void changeHeroState(kHeroState state);
    kHeroState getState();
private:
    kHeroState _state;
};

#endif /* Hero_hpp */
