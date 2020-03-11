//#include "KeyEventReceiver.h"
//
//using namespace irr;
//using namespace core;
//using namespace video;
//using namespace scene;
//using namespace io;
//using namespace gui;
//
//KeyEventReceiver::KeyEventReceiver()
//{
//
//	for (u32 i = 0; i < KEY_KEY_CODES_COUNT; ++i)
//	{
//
//		KeyIsDown[i] = false;
//
//	}
//
//}
//
//bool KeyEventReceiver::OnEvent(const irr::SEvent& event)
//{
//
//	// Remember whether each key is down or up
//	if (event.EventType == irr::EET_KEY_INPUT_EVENT)
//		KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
//
//	return false;
//
//}
//
//// This is used to check whether a key is being held down
//bool KeyEventReceiver::IsKeyDown(irr::EKEY_CODE keyCode) const
//{
//
//	return KeyIsDown[keyCode];
//
//}
//
//void KeyEventReceiver::Checkkeys(KeyEventReceiver receiver, const irr::f32 frameDeltaTime, float *rotation)
//{
//
//	if (receiver.IsKeyDown(irr::KEY_KEY_D))
//	{
//
//		*rotation -= 50 * frameDeltaTime;
//
//	}
//	if (receiver.IsKeyDown(irr::KEY_KEY_A))
//	{
//
//		*rotation += 50 * frameDeltaTime;
//
//	}
//
//}