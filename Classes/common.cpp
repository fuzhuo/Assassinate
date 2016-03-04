//
//  common.cpp
//  Assassinate
//
//  Created by zfu on 3/2/16.
//
//

#include <stdio.h>
#include "common.h"

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
    "down",
    "down_round",
    "hero_stand",
    "hero_squat",
    "hero_hung",
    "hung_left",
    "hung_right",
    "hung_circle",
    "hero_hung_box"
};

const bool isSensor[] = {
    false,
    false,
    true,
    true,
    false,
    true
};

const std::string heroStateStr[] = {
    "stand",
    "hung",
    "squat",
    "hero_stand",
    "hero_squat",
    "hero_hung"
};