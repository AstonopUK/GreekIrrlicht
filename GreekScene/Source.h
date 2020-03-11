#pragma once

#ifndef SOURCE_H
#define SOURCE_H

#include <irrlicht.h>
#include <stdlib.h>
#include <vector>
#include <irrklang.h>
#include <ITimer.h>
#include <cstdlib>

using namespace irr;
using namespace core;
using namespace video;
using namespace scene;
using namespace io;
using namespace gui;
using namespace irrklang;
using namespace std;

//Booleans controls current active camera
int activeCamera = 0;
bool isPaused = false;

//vector<IBillboardSceneNode> BillboardEast[1];
//BillboardEast.reserve(200);

#endif