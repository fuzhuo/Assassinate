//
//  common.h
//  Assassinate
//
//  Created by zfu on 3/2/16.
//
//

#ifndef common_h
#define common_h

extern const std::string mapTypeStr[];
extern const std::string shapeTypeStr[];
extern const bool isSensor[];
extern const std::string heroStateStr[];

#define HERO_GID -1

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
    kShapeTag_hero_hung,
    kShapeTag_hung_left,
    kShapeTag_hung_right,
    kShapeTag_hung_circle,
    kShapeTag_hero_hung_box
}kShapeTag;

typedef enum{
    BUTTON_U=1<<0,
    BUTTON_D=1<<1,
    BUTTON_L=1<<2,
    BUTTON_R=1<<3,
    BUTTON_A=1<<4,
    BUTTON_B=1<<5
}kButton;

#endif /* common_h */
