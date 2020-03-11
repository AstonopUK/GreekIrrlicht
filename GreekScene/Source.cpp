//Include declaration
#include "Source.h"

//Namespace inclusion
using namespace irr;
using namespace core;
using namespace video;
using namespace scene;
using namespace io;
using namespace gui;
using namespace irrklang;

//Using define statement to prevent analytics from being repeatedly posted into cmd window
#ifdef _IRR_WINDOWS_
#pragma comment (linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#pragma comment(lib, "irrklang.lib") //Including Irrklang library for use
#endif

#pragma comment (lib, "irrlicht.lib") //Including Irrlicht library for use

//KEY EVENT CLASS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

s32 fps_then; //Variable to track past FPS
vector3df cam_target(0.0f, 0.0f, 10.0f); //Tracks FPS camera target
vector3df cam_lookdir(0.0f, 0.0f, 0.0f); //Tracks FPS camera look direction

//Declaring all GUI button elements as enums
enum
{

	GUI_ID_FPS_BUTTON,
	GUI_ID_QUIT_BUTTON

};

//Declaration of requirements for the key management class
struct SAppContext
{

	IrrlichtDevice *device;
	s32 counter;
	IGUIListBox* listbox;

};

//Key Event Receiver class | Used for management of GUI buttons and external key usage such as custom keyboard movement
class KeyEventReceiver : public IEventReceiver
{

public:
	//Sets all pressed keys on the keyboard to false
	KeyEventReceiver(SAppContext & context) : Context(context)
	{

		for (u32 i = 0; i < KEY_KEY_CODES_COUNT; ++i)
		{

			KeyIsDown[i] = false;

		}

	}

	//Relates the context modifier to the app context and sets an array of booleans to the key codes
	SAppContext & Context;
	bool KeyIsDown[KEY_KEY_CODES_COUNT];

	//Event handler for GUI and custom key events
	virtual bool OnEvent(const SEvent& event)
	{

		//Handles new key inputs via keyboard
		if (event.EventType == EET_KEY_INPUT_EVENT)
		{

			KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown; //Checks is the key to be pressed is in fact pressed down

		}

		//Handles GUI element interactions
		if (event.EventType == EET_GUI_EVENT)
		{

			//Adding ID context to each GUI call to keep track of all open GUI elements
			s32 id = event.GUIEvent.Caller->getID();
			IGUIEnvironment* env = Context.device->getGUIEnvironment();

			//Switch statement controls which button is being pressed - includes button decleration
			switch (event.GUIEvent.EventType)
			{

				//Switch case to track which button has been pressed
			case EGET_BUTTON_CLICKED:
				switch (id)
				{
					//Quit button, quits game when pressed
				case GUI_ID_QUIT_BUTTON:
					Context.device->closeDevice();
					return true;

					//FPS button, displays performance statistics when pressed
				case GUI_ID_FPS_BUTTON:
				{

					//Context.listbox->addItem(L"Window created");
					Context.counter += 30;
					if (Context.counter > 200) //Context adds to element counter. Debug line about will create listbox for all current open GUI elements
					{

						Context.counter = 0;

					}

					//Creates a string for the FPS and appends FPS to it, then when button pressed presents window with current FPS
					stringw str = L"Your current FPS is: ";
					str += fps_then;
					IGUIWindow* window = env->addWindow(rect<s32>(100 + Context.counter, 100 + Context.counter, 300 + Context.counter, 200 + Context.counter), false, L"Performance");
					env->addStaticText(str.c_str(), rect<s32>(5, 20, 195, 95), true, false, window);

				}
				return true;

				default:
					return false;

				}
				break;

			default:
				break;

			}

		}

		return false;

	}

	//Boolean for checking if key is pressed or not
	bool IsKeyDown(const EKEY_CODE &keyCode) const
	{

		return KeyIsDown[keyCode];

	}

private:

	//Private level variable decleration


};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Collision detector for sound collisions
bool detectCollisionBetweenNodes(ISceneNode *node1, ISceneNode *node2) 
{

	if (node1->getTransformedBoundingBox().intersectsWithBox(node2->getTransformedBoundingBox())) //Checks if two bounding boxes have collided. If so, returns true
	{

		return true;

	}
	else 
	{

		return false;

	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{

	//CONTEXT & KEY EVENT RECEIVER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SAppContext context;

	//Setup of display device, event receiver and gui environment
	KeyEventReceiver ker(context); //Binds Key Event Receiver to context
	IrrlichtDevice *device = createDevice(EDT_DIRECT3D9, dimension2d<u32>(650, 600), 32, false, false, false, &ker); //Sets up device using DX9 in a 800x600 resolution screen

	//Setting up the context device for GUI tracking
	context.device = device;
	context.counter = 0;

	//Establishing the GUI environment to render on
	IGUIEnvironment *guienv = device->getGUIEnvironment();

	//Setting up sound engine and playing main theme
	ISoundEngine *soueng = createIrrKlangDevice();
	ISound *music = soueng->play2D("./media/Audio/Sardana.mp3", true); //This will play the song once the program is launched
	ISound *menuOpen = NULL; //This will contain the menu opening sound
	ISound *menuClose = NULL; //This will contain the menu closing sound
	ISound *swordCol = NULL; //This will contain the sword collision sound

	ISceneNode *colSoundNode = NULL; //Variable for object collision detection for sounds
	ISceneNode *colLastNode = NULL; //Variable for last sound object collided with

	ISound *birds = soueng->play3D("./media/Audio/Birds.wav", vector3df(6200, 125, 7800), true, false, true); //Initiates the 3D sound of birds
	birds->setMinDistance(50.0f); //Sets bird listen distance to whole map but limits loud birds to near source

	//Adding button elements to the scene with relevant interactions
	guienv->addButton(rect<s32>(10, 20, 110, 20 + 32), 0, GUI_ID_FPS_BUTTON, L"FPS", L"Further information on performance."); //This adds the FPS button
	guienv->addButton(rect<s32>(10, 450, 110, 450 + 32), 0, GUI_ID_QUIT_BUTTON, L"QUIT", L"Exit the application."); //This button closes the game

	//Static text that was used as a placeholder for finding coordinates
	IGUIStaticText *PosX = 0;
	stringw XString = L"X:";
	PosX = guienv->addStaticText(L"0", core::rect<s32>(500, 40, 600, 120), false);

	//If device engine doesn't launch, close with error code -1
	if (device == nullptr)
	{

		return -1;

	}

	//If sound engine doesn't launch, close with error code -2
	if (soueng == nullptr)
	{

		return -2;

	}

	//Set scene header caption
	device->setWindowCaption(L"Mythos Scene");

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//SCENE MESH & COMPONENTS
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Instantiation of cameras and relevant video and scene drivers to allow rendering of the scene
	ISceneManager *scnmgr = device->getSceneManager(); //Creates the scene manager
	IVideoDriver *driver = device->getVideoDriver(); //Creates the video driver
	ICameraSceneNode *FPScamera = scnmgr->addCameraSceneNodeFPS(NULL, 100, 0.4); //FPS camera for default navigation
	ICameraSceneNode *TPScamera = scnmgr->addCameraSceneNode(NULL, vector3df(500, 300, 400), vector3df(0, 0, 0)); //Third person camera for behind the head perspective
	ICameraSceneNode *MapCamera = scnmgr->addCameraSceneNode(NULL, vector3df(300, 600, 300), vector3df(FPScamera->getPosition())); //Map camera to get birdseye view of location
	FPScamera->setPosition(vector3df(300, 200, 300)); //Setting camera start location

	scnmgr->setActiveCamera(FPScamera); //Sets FPS camera as the default camera

	//Creation of skybox
	driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false); //Disabled mipmaps
	ISceneNode *skybox = scnmgr->addSkyBoxSceneNode(
		driver->getTexture("./media/lilacisles_up.tga"), //Top texture
		driver->getTexture("./media/lilacisles_dn.tga"), //Bottom texture
		driver->getTexture("./media/lilacisles_lf.tga"), //Left texture
		driver->getTexture("./media/lilacisles_rt.tga"), //Right texture
		driver->getTexture("./media/lilacisles_bk.tga"), //Back texture
		driver->getTexture("./media/lilacisles_ft.tga")); //Front texture
	driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, true); //Reenabling mipmaps once loaded

	//Creating the fog mechanic that will allow for distant items to be cast in a foggy shadow
	driver->setFog(SColor(255, 255, 255, 255), EFT_FOG_LINEAR, 1500.0f, 2250.0f, 0.001f, false, false); 

	//All misc basic meshes declared here

	IMesh *sandMesh = scnmgr->getMesh("./media/Sand.obj"); //Sand mesh
	IMesh *treestMesh = scnmgr->getMesh("./media/TreeStem.obj"); //Tree stump mesh
	IMesh *potMesh = scnmgr->getMesh("./media/vazy 3.3ds"); //Pot selection mesh
	IMesh *pegMesh = scnmgr->getMesh("./media/PegasusNew.obj"); //Pegasus statue mesh
	IAnimatedMesh *rockMesh = scnmgr->getMesh("./media/RocksNew.obj"); //Big rock mesh
	IAnimatedMesh *wolf = scnmgr->getMesh("./media/Only_Spider.x"); //Wolf spider mesh
	IAnimatedMesh *alien = scnmgr->getMesh("./media/AncientVase.obj"); //Big vase mesh
	IAnimatedMesh *AncientPot = scnmgr->getMesh("./media/AncientPot.obj"); //Old amphora mesh
	IAnimatedMesh *Column = scnmgr->getMesh("./media/OldColumn.obj"); //Column set mesh
	IAnimatedMesh *Sword = scnmgr->getMesh("./media/romansword.obj"); //Sword mesh

	//All important objects declared here
	IAnimatedMesh *house1 = scnmgr->getMesh("./media/RuinHouse.obj"); //House of ruins mesh
	IAnimatedMesh *house2 = scnmgr->getMesh("./media/lowpoly_house.obj"); //Small shed mesh
	IAnimatedMesh *house3 = scnmgr->getMesh("./media/pierre ele 5fixfix.obj"); //Broken cobbled ruin mesh
	IAnimatedMesh *house4 = scnmgr->getMesh("./media/pierre ele 6fix.obj"); //Broken cobbled ruin mesh
	IAnimatedMesh *house5 = scnmgr->getMesh("./media/pierre ele 1fix.obj"); //Broken cobbled ruin mesh
	IAnimatedMesh *house6 = scnmgr->getMesh("./media/pantheon.obj"); //Pantheon mesh
	IAnimatedMesh *house7 = scnmgr->getMesh("./media/free_arena.obj"); //Large arena mesh
	IAnimatedMesh *house8 = scnmgr->getMesh("./media/gazebo3.obj"); // Ampitheatrea mesh
	IAnimatedMesh *house9 = scnmgr->getMesh("./media/nike.obj"); //Templemesh
	IAnimatedMesh *house10 = scnmgr->getMesh("./media/Tower.obj"); //Destroyed watchtower mesh

	//All animated models declared here
	IAnimatedMesh *mesh = scnmgr->getMesh("./media/mintoor.x"); //Minotaur mesh
	IAnimatedMesh *felguard = scnmgr->getMesh("./media/felguard-X.x"); //Lesser minotaur mesh
	IAnimatedMesh *yeti = scnmgr->getMesh("./media/yeti-monster-animated-X.x"); //War beast mesh
	IAnimatedMesh *scorpion = scnmgr->getMesh("./media/scorpid-monster-X-animated.x"); //Sand scorpion mesh
	IAnimatedMesh *achMesh = scnmgr->getMesh("./media/achilles.x"); //Achilles mesh
	IAnimatedMesh *allFourMonster = scnmgr->getMesh("./media/monster-animated-character-X.x"); //Lion-thing mesh
	IAnimatedMesh *goblin = scnmgr->getMesh("./media/goblin-sapper-animated-X.x"); //Impling mesh
	IAnimatedMesh *knight = scnmgr->getMesh("./media/knight_low.x"); //Powerful knight mesh

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Creating the water plane for near the ampitheatre
	IAnimatedMesh* waterMesh = scnmgr->getMesh(0); //Creating an empty mesh
	ISceneNode* waterMeshNode = 0; //Designating the mesh a node

	//Creating the hill plane node that will ripple to create the water effect
	waterMesh = scnmgr->addHillPlaneMesh("pool",
		dimension2d<f32>(80, 80),
		dimension2d<u32>(40, 40), 0, 0,
		dimension2d<f32>(0, 0),
		dimension2d<f32>(10, 10));

	waterMeshNode = scnmgr->addWaterSurfaceSceneNode(waterMesh->getMesh(0), 3.0f, 300.0f, 30.0f); //Adds the water surface effect
	waterMeshNode->setPosition(vector3df(6200, 175, 7800)); //Sets the location of the water

	waterMeshNode->setMaterialTexture(0, driver->getTexture("./media/stones.jpg")); //Sets the primary texture to stones
	waterMeshNode->setMaterialTexture(1, driver->getTexture("./media/water.jpg")); //Sets the secondary texture to faux light scattering
	waterMeshNode->setMaterialType(EMT_REFLECTION_2_LAYER); //Enables the reflection layer
	waterMeshNode->setMaterialFlag(EMF_FOG_ENABLE, true); //Enables the fog on the water

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Set mouse visibility to false
	device->getCursorControl()->setVisible(false);

	//SCENE NODES
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Test lighting
	ILightSceneNode *light = scnmgr->addLightSceneNode(nullptr, vector3df(6200, 175, 7800)); //Creation of a light node
	SLight lightData; //Creating data relevant to the light
	lightData.DiffuseColor = SColor(255, 255, 255, 255); //Setting the diffuse colour of the light it emits
	lightData.AmbientColor = SColor(255, 255, 255, 255); //Setting ambient colour that effects surrounding objects
	lightData.Falloff = 5.0f;
	lightData.Radius = 5000.0f;
	light->setLightData(lightData); //Assigning the light data to the light node in order to apply changes made in the light data

	//Instantiating model nodes in scene
	ISceneNode *pCube = scnmgr->addCubeSceneNode(); //Cube to represent current player location
	IMeshSceneNode *trstMeshNode = scnmgr->addMeshSceneNode(treestMesh); //Tree stump node
	IMeshSceneNode *potMeshNode = scnmgr->addMeshSceneNode(potMesh); //Pot node
	IMeshSceneNode *pegMeshNode = scnmgr->addMeshSceneNode(pegMesh); //Pegasus statue node
	IAnimatedMeshSceneNode *rockMeshNode = scnmgr->addAnimatedMeshSceneNode(rockMesh); //Rock node
	IAnimatedMeshSceneNode *achNode = scnmgr->addAnimatedMeshSceneNode(achMesh); //Achilles node
	IAnimatedMeshSceneNode *wolfNode = scnmgr->addAnimatedMeshSceneNode(wolf); //Spider node
	IAnimatedMeshSceneNode *aliNode = scnmgr->addAnimatedMeshSceneNode(alien); //Vase node
	IAnimatedMeshSceneNode *ancpotNode = scnmgr->addAnimatedMeshSceneNode(AncientPot); //Amphora node
	IAnimatedMeshSceneNode *colNode = scnmgr->addAnimatedMeshSceneNode(Column); //Columns node
	IAnimatedMeshSceneNode *swordNode = scnmgr->addAnimatedMeshSceneNode(Sword); //Sword node

	//Houses and buildings
	IAnimatedMeshSceneNode *house1Node = scnmgr->addAnimatedMeshSceneNode(house1); //Ruined house node
	IAnimatedMeshSceneNode *house2Node = scnmgr->addAnimatedMeshSceneNode(house2); //Hut node
	IAnimatedMeshSceneNode *house3Node = scnmgr->addAnimatedMeshSceneNode(house3); //Ruins node
	IAnimatedMeshSceneNode *house4Node = scnmgr->addAnimatedMeshSceneNode(house4); //Ruins node
	IAnimatedMeshSceneNode *house5Node = scnmgr->addAnimatedMeshSceneNode(house5); //Ruins node
	IAnimatedMeshSceneNode *house6Node = scnmgr->addAnimatedMeshSceneNode(house6); //Pantheon node
	IAnimatedMeshSceneNode *house7Node = scnmgr->addAnimatedMeshSceneNode(house7); //Arena node
	IAnimatedMeshSceneNode *house8Node = scnmgr->addAnimatedMeshSceneNode(house8); //Ampitheatre node
	IAnimatedMeshSceneNode *house9Node = scnmgr->addAnimatedMeshSceneNode(house9); //Temple node
	IAnimatedMeshSceneNode *house10Node = scnmgr->addAnimatedMeshSceneNode(house10); //Watchtower node
	
	//Animated creatures
	IAnimatedMeshSceneNode *allMonNode = scnmgr->addAnimatedMeshSceneNode(allFourMonster); //Lion-thing node
	IAnimatedMeshSceneNode *mintoorNode = scnmgr->addAnimatedMeshSceneNode(mesh); //Minotaur node
	IAnimatedMeshSceneNode *felguard1Node = scnmgr->addAnimatedMeshSceneNode(felguard); //Lesser node
	IAnimatedMeshSceneNode *felguard2Node = scnmgr->addAnimatedMeshSceneNode(felguard); //Lesser node
	IAnimatedMeshSceneNode *yeti1Node = scnmgr->addAnimatedMeshSceneNode(yeti); //Beast node
	IAnimatedMeshSceneNode *yeti2Node = scnmgr->addAnimatedMeshSceneNode(yeti); //Beast node 
	IAnimatedMeshSceneNode *yeti3Node = scnmgr->addAnimatedMeshSceneNode(yeti); //Beast node
	IAnimatedMeshSceneNode *scorpNode = scnmgr->addAnimatedMeshSceneNode(scorpion); //Scorpion node
	IAnimatedMeshSceneNode *gobNode = scnmgr->addAnimatedMeshSceneNode(goblin); //Impling node
	IAnimatedMeshSceneNode *kniNode = scnmgr->addAnimatedMeshSceneNode(knight); //Knight node

	//Checks if the minotaur node has loaded. If not, exits program
	//All the models have been tested individually for compatability and all work. The reason we test only one node is to check if they will all load correctly, as if once doesn't
	//the rest will not either. This saves on processing power as we do not have to check every single node over and over again.
	if (mintoorNode == nullptr)
	{

		device->drop();
		return -3;

	}

	//Terrain item modification, setting position, scale and rotation on the map
	rockMeshNode->setScale(vector3df(2.5, 2.5, 2.5));
	rockMeshNode->setPosition(vector3df(1000, 0, 1000));
	trstMeshNode->setPosition(vector3df(1000, 0, 1000));

	//House node modification, setting position, scale and rotation on the map
	house1Node->setPosition(vector3df(2300, 50, 2100));
	house1Node->setScale(vector3df(50, 50, 50));
	house2Node->setPosition(vector3df(10, 60, 2100));
	house2Node->setRotation(vector3df(0, 180, 0));
	house3Node->setPosition(vector3df(8300, 80, 700));
	house3Node->setScale(vector3df(90, 90, 90));
	house3Node->setRotation(vector3df(0, 270, 0));
	house4Node->setPosition(vector3df(1100, 73, 4700));
	house4Node->setScale(vector3df(90, 90, 90));
	house4Node->setRotation(vector3df(0, 90, 0));
	house5Node->setPosition(vector3df(4100, 580, 3100));
	house5Node->setScale(vector3df(90, 90, 90));
	house5Node->setRotation(vector3df(0, 90, 0));
	house6Node->setPosition(vector3df(6200, 150, 7000));
	house6Node->setScale(vector3df(4000, 4000, 4000));
	house6Node->setRotation(vector3df(0, 0, 0));
	house7Node->setPosition(vector3df(900, 10, 11500));
	house7Node->setScale(vector3df(50, 50, 50));
	house7Node->setRotation(vector3df(0, 0, 0));
	house8Node->setPosition(vector3df(2200, 400, 14000));
	house8Node->setScale(vector3df(50, 50, 50));
	house8Node->setRotation(vector3df(0, 0, 0));
	house9Node->setPosition(vector3df(1370, 500, 10000));
	house9Node->setScale(vector3df(7, 7, 7));
	house9Node->setRotation(vector3df(0, 0, 0));
	house10Node->setPosition(vector3df(8900, 200, 4200));
	house10Node->setScale(vector3df(1.2, 1.2, 1.2));
	house10Node->setRotation(vector3df(0, 0, 0));

	//Animation node modifications, setting position, scale, animation frame loops and rotation on the map
	mintoorNode->setScale(vector3df(1100, 1100, 1100));
	mintoorNode->setFrameLoop(0, 24);
	allMonNode->setScale(vector3df(10, 10, 10));
	allMonNode->setRotation(vector3df(0, 0, 270));
	allMonNode->setPosition(vector3df(3500, 700, 3300));
	allMonNode->setFrameLoop(250, 333);
	felguard1Node->setScale(vector3df(40, 40, 40));
	felguard1Node->setRotation(vector3df(0, 0, 0));
	felguard1Node->setPosition(vector3df(7900, 100, 680));
	felguard1Node->setFrameLoop(150, 200);
	felguard2Node->setScale(vector3df(40, 40, 40));
	felguard2Node->setRotation(vector3df(0, 0, 0));
	felguard2Node->setPosition(vector3df(8700, 100, 690));
	felguard2Node->setFrameLoop(260, 350);
	yeti1Node->setScale(vector3df(40, 40, 40));
	yeti1Node->setRotation(vector3df(0, 0, 0));
	yeti1Node->setPosition(vector3df(7000, 480, 4800));
	yeti1Node->setFrameLoop(0, 200);
	yeti2Node->setScale(vector3df(40, 40, 40));
	yeti2Node->setRotation(vector3df(0, 90, 0));
	yeti2Node->setPosition(vector3df(650, 105, 2000));
	yeti2Node->setFrameLoop(350, 450);
	yeti3Node->setScale(vector3df(40, 40, 40));
	yeti3Node->setRotation(vector3df(0, 180, 0));
	yeti3Node->setPosition(vector3df(600, 100, 10700));
	yeti3Node->setFrameLoop(450, 525);
	scorpNode->setScale(vector3df(20, 20, 20));
	scorpNode->setRotation(vector3df(0, 0, 0));
	scorpNode->setPosition(vector3df(2000, 75, 3300));
	scorpNode->setFrameLoop(168, 220);
	gobNode->setScale(vector3df(30, 30, 30));
	gobNode->setRotation(vector3df(0, 90, 0));
	gobNode->setPosition(vector3df(1430, 85, 450));
	gobNode->setFrameLoop(0, 95);
	kniNode->setScale(vector3df(30, 30, 30));
	kniNode->setRotation(vector3df(0, 0, 0));
	kniNode->setPosition(vector3df(2300, 225, 2100));
	kniNode->setFrameLoop(1380, 1530);

	//Misc node modification, setting position, scale and rotation on the map
	aliNode->setScale(vector3df(200, 200, 200));
	aliNode->setRotation(vector3df(0, 0, 0));
	aliNode->setPosition(vector3df(2000, 70, 2100));
	achNode->setScale(vector3df(0.6, 0.6, 0.6));
	achNode->setPosition(vector3df(1100, 90, 1200));
	achNode->setRotation(vector3df(0, 0, 0));
	achNode->setFrameLoop(0, 48);
	potMeshNode->setScale(vector3df(50, 50, 50));
	potMeshNode->setPosition(vector3df(585, 96, 2050));
	potMeshNode->setRotation(vector3df(2, 0, 0));
	pegMeshNode->setScale(vector3df(15, 15, 15));
	pegMeshNode->setPosition(vector3df(1090, 87, 1200));
	pegMeshNode->setRotation(vector3df(-90, 0, 0));
	wolfNode->setScale(vector3df(1, 1, 1));
	wolfNode->setRotation(vector3df(0, 90, -25));
	wolfNode->setPosition(vector3df(300, 350, 2100));
	ancpotNode->setScale(vector3df(100, 100, 100));
	ancpotNode->setRotation(vector3df(0, 90, -25));
	ancpotNode->setPosition(vector3df(930, 100, 1100));
	colNode->setScale(vector3df(10, 10, 10));
	colNode->setRotation(vector3df(0, 90, -25));
	colNode->setPosition(vector3df(2130, 300, 4500));
	swordNode->setScale(vector3df(5, 5, 5));
	swordNode->setRotation(vector3df(0, 90, 180));
	swordNode->setPosition(vector3df(1130, 170, 1500));

	//Editing lighting settings, applying fog and applying textures to building nodes
	//vector<IAnimatedMeshSceneNode> Buildings;
	house1Node->setMaterialFlag(EMF_LIGHTING, false);
	house1Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house1Node->setMaterialTexture(0, driver->getTexture("./media/86d942c9.png"));
	house2Node->setMaterialFlag(EMF_LIGHTING, false);
	house2Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house2Node->setMaterialTexture(0, driver->getTexture("./media/defuse.jpg"));
	house3Node->setMaterialFlag(EMF_LIGHTING, false);
	house3Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house3Node->setMaterialTexture(0, driver->getTexture("./media/mossy_rock_by_cl_stock-d605xoz.jpg"));
	house4Node->setMaterialFlag(EMF_LIGHTING, false);
	house4Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house4Node->setMaterialTexture(0, driver->getTexture("./media/mossy_rock_by_cl_stock-d605xoz.jpg"));
	house5Node->setMaterialFlag(EMF_LIGHTING, false);
	house5Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house5Node->setMaterialTexture(0, driver->getTexture("./media/mossy_rock_by_cl_stock-d605xoz.jpg"));
	house6Node->setMaterialFlag(EMF_LIGHTING, false);
	house6Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house6Node->setMaterialTexture(0, driver->getTexture("./media/mercury.jpg"));
	house7Node->setMaterialFlag(EMF_LIGHTING, false);
	house7Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house7Node->setMaterialTexture(0, driver->getTexture("./media/mercury.jpg"));
	house8Node->setMaterialFlag(EMF_LIGHTING, false);
	house8Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house8Node->setMaterialTexture(0, driver->getTexture("./media/gazebo.jpg"));
	house9Node->setMaterialFlag(EMF_LIGHTING, false);
	house9Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house9Node->setMaterialTexture(0, driver->getTexture("./media/Maps/snd1.jpg"));
	house10Node->setMaterialFlag(EMF_LIGHTING, false);
	house10Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	house10Node->setMaterialTexture(0, driver->getTexture("./media/Cobblestone.png"));

	//Editing lighting settings, applying fog and applying textures to model nodes
	rockMeshNode->setMaterialFlag(EMF_LIGHTING, false);
	rockMeshNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	rockMeshNode->setMaterialTexture(0, driver->getTexture("./media/Maps/rc1.jpg"));
	trstMeshNode->setMaterialFlag(EMF_LIGHTING, false);
	trstMeshNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	trstMeshNode->setMaterialTexture(0, driver->getTexture("./media/Maps/plvs1c.jpg"));
	achNode->setMaterialFlag(EMF_LIGHTING, false);
	achNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	achNode->setMaterialTexture(0, driver->getTexture("./media/AchillesTexture.png"));
	potMeshNode->setMaterialFlag(EMF_LIGHTING, false);
	potMeshNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	potMeshNode->setMaterialTexture(0, driver->getTexture("./media/PotTex.png"));
	pegMeshNode->setMaterialFlag(EMF_LIGHTING, false);
	pegMeshNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	pegMeshNode->setMaterialTexture(0, driver->getTexture("./media/Maps/snd1.jpg"));
	wolfNode->setMaterialFlag(EMF_LIGHTING, false);
	wolfNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	wolfNode->setMaterialTexture(0, driver->getTexture("./media/Spinnen_Bein_tex.jpg"));
	aliNode->setMaterialFlag(EMF_LIGHTING, false);
	aliNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	aliNode->setMaterialTexture(0, driver->getTexture("./media/Maps/snd1.jpg"));
	ancpotNode->setMaterialFlag(EMF_LIGHTING, false);
	ancpotNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	ancpotNode->setMaterialTexture(0, driver->getTexture("./media/Stone01.jpg"));
	colNode->setMaterialFlag(EMF_LIGHTING, false);
	colNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	colNode->setMaterialTexture(0, driver->getTexture("./media/Stone01.jpg"));
	swordNode->setMaterialFlag(EMF_LIGHTING, false);
	swordNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	swordNode->setMaterialTexture(0, driver->getTexture("./media/mercury.jpg"));

	//Editing lighting settings, applying fog and applying textures to animation nodes
	mintoorNode->setMaterialFlag(EMF_LIGHTING, false);
	mintoorNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	allMonNode->setMaterialFlag(EMF_LIGHTING, false);
	allMonNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	allMonNode->setMaterialTexture(0, driver->getTexture("./media/monster.jpg"));
	felguard1Node->setMaterialFlag(EMF_LIGHTING, false);
	felguard1Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	felguard1Node->setMaterialTexture(0, driver->getTexture("./media/fguard.jpg"));
	felguard2Node->setMaterialFlag(EMF_LIGHTING, false);
	felguard2Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	felguard2Node->setMaterialTexture(0, driver->getTexture("./media/fguard2.jpg"));
	yeti1Node->setMaterialFlag(EMF_LIGHTING, false);
	yeti1Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	yeti1Node->setMaterialTexture(0, driver->getTexture("./media/yeti1.jpg"));
	yeti2Node->setMaterialFlag(EMF_LIGHTING, false);
	yeti2Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	yeti2Node->setMaterialTexture(0, driver->getTexture("./media/yeti2.jpg"));
	yeti3Node->setMaterialFlag(EMF_LIGHTING, false);
	yeti3Node->setMaterialFlag(EMF_FOG_ENABLE, true);
	yeti3Node->setMaterialTexture(0, driver->getTexture("./media/yeti3.jpg"));
	scorpNode->setMaterialFlag(EMF_LIGHTING, false);
	scorpNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	scorpNode->setMaterialTexture(0, driver->getTexture("./media/scorp-01.jpg"));
	gobNode->setMaterialFlag(EMF_LIGHTING, false);
	gobNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	gobNode->setMaterialTexture(0, driver->getTexture("./media/goblin.jpg"));
	kniNode->setMaterialFlag(EMF_LIGHTING, false);
	kniNode->setMaterialFlag(EMF_FOG_ENABLE, true);
	kniNode->setMaterialTexture(0, driver->getTexture("./media/knight_01.jpg"));
	
	//Applying animations to certain model nodes
	ISceneNodeAnimator *spintoor = scnmgr->createFlyCircleAnimator(vector3df(1100, 97, 1070), 200, 0.001);

	//Attributing current FPS to a variable for later comparison
	fps_then = driver->getFPS();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//BILLBOARDS
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Random float store
	float random;

	/*
	vector<IBillboardSceneNode> Trees[4];
	Trees[0].reserve(105);
	Trees[1].reserve(105);
	Trees[2].reserve(105);
	Trees[3].reserve(105);
	*/

	//For loop to instantiate a row of trees to cover the perimeter of the map, illusion of barrier
	for (f32 row = 40; row < 15750; row = row + 150)
	{

		random = rand() % 70 + 180;

		IBillboardSceneNode *billboardT1 = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(500, 500), vector3df(30, random, row));
		if (billboardT1)
		{

			billboardT1->setMaterialFlag(EMF_LIGHTING, false);
			billboardT1->setMaterialFlag(EMF_FOG_ENABLE, true);
			billboardT1->setMaterialTexture(0, driver->getTexture("./media/Treee.png")); //Creates the row of trees set at the left of the map
			billboardT1->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

		}

	}

	//For loop to instantiate a row of trees to cover the perimeter of the map, illusion of barrier
	for (f32 row = 40; row < 15750; row = row + 150)
	{

		random = rand() % 70 + 180;

		IBillboardSceneNode *billboardT2 = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(500, 500), vector3df(row, random, 30));
		if (billboardT2)
		{

			billboardT2->setMaterialFlag(EMF_LIGHTING, false);
			billboardT2->setMaterialFlag(EMF_FOG_ENABLE, true);
			billboardT2->setMaterialTexture(0, driver->getTexture("./media/Treee.png")); //Creates the row of trees set at the bottom of the map
			billboardT2->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

		}

	}

	//For loop to instantiate a row of trees to cover the perimeter of the map, illusion of barrier
	for (f32 row = 40; row < 15750; row = row + 150)
	{

		random = rand() % 70 + 180;

		IBillboardSceneNode *billboardT3 = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(500, 500), vector3df(row, random, 15750));
		if (billboardT3)
		{

			billboardT3->setMaterialFlag(EMF_LIGHTING, false);
			billboardT3->setMaterialFlag(EMF_FOG_ENABLE, true);
			billboardT3->setMaterialTexture(0, driver->getTexture("./media/Treee.png")); //Creates the row of trees set at the top of the map
			billboardT3->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

		}

	}

	//For loop to instantiate a row of trees to cover the perimeter of the map, illusion of barrier
	for (f32 row = 40; row < 15750; row = row + 150)
	{

		random = rand() % 70 + 180;

		IBillboardSceneNode *billboardT4 = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(500, 500), vector3df(15750, random, row));
		if (billboardT4)
		{

			billboardT4->setMaterialFlag(EMF_LIGHTING, false);
			billboardT4->setMaterialFlag(EMF_FOG_ENABLE, true);
			billboardT4->setMaterialTexture(0, driver->getTexture("./media/Treee.png")); //Creates the row of trees set at the right of the map
			billboardT4->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

		}

	}

	//Billboard For Loop creating a small army of greek soldiers. Effective and easy on performance
	for (f32 row = 900; row < 1350; row = row + 75)
	{

		for (f32 col = 3500; col < 3950; col = col + 75)
		{

			IBillboardSceneNode *billboardGW = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(100, 100), vector3df(col, 110, row));
			if (billboardGW)
			{

				billboardGW->setMaterialFlag(EMF_LIGHTING, false);
				billboardGW->setMaterialFlag(EMF_FOG_ENABLE, true);
				billboardGW->setMaterialTexture(0, driver->getTexture("./media/GreekWarrior.png"));
				billboardGW->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

			}

		}

	}

	//Billboard for signpost
	IBillboardSceneNode *billboardSign = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(100, 140), vector3df(1300, 140, 900));
	if (billboardSign)
	{

		billboardSign->setMaterialFlag(EMF_LIGHTING, false);
		billboardSign->setMaterialFlag(EMF_FOG_ENABLE, true);
		billboardSign->setMaterialTexture(0, driver->getTexture("./media/WoodSign.png"));
		billboardSign->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

	}

	//Billboard for big tree
	IBillboardSceneNode *billboardBT = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(900, 600), vector3df(1600, 340, 1900));
	if (billboardBT)
	{

		billboardBT->setMaterialFlag(EMF_LIGHTING, false);
		billboardBT->setMaterialFlag(EMF_FOG_ENABLE, true);
		billboardBT->setMaterialTexture(0, driver->getTexture("./media/BigTree.png"));
		billboardBT->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

	}

	//Billboard for clouds - NB use of different alpha channel to simulate 'fluffy' effect
	IBillboardSceneNode *billboardClouds = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(2700, 2000), vector3df(1300, 2950, 900));
	if (billboardClouds)
	{

		billboardClouds->setMaterialFlag(EMF_LIGHTING, false);
		billboardClouds->setMaterialTexture(0, driver->getTexture("./media/Clouds.png"));
		billboardClouds->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);

	}

	//Billboard for clouds - NB use of different alpha channel to simulate 'fluffy' effect
	IBillboardSceneNode *billboardClouds2 = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(3500, 2000), vector3df(3300, 2950, 4900));
	if (billboardClouds2)
	{

		billboardClouds2->setMaterialFlag(EMF_LIGHTING, false);
		billboardClouds2->setMaterialTexture(0, driver->getTexture("./media/Cloud2.png"));
		billboardClouds2->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);

	}

	//Billboard for dust cloud
	IBillboardSceneNode *billboardDustClouds = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(3500, 2000), vector3df(2300, 200, 1900));
	if (billboardDustClouds)
	{

		billboardDustClouds->setMaterialFlag(EMF_LIGHTING, false);
		billboardDustClouds->setMaterialTexture(0, driver->getTexture("./media/DustCloud.png"));
		billboardDustClouds->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);

	}

	//Billboard for pillar
	IBillboardSceneNode *billboardPillar = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(100, 200), vector3df(1600, 120, 900));
	if (billboardPillar)
	{

		billboardPillar->setMaterialFlag(EMF_LIGHTING, false);
		billboardPillar->setMaterialTexture(0, driver->getTexture("./media/Pillar.png"));
		billboardPillar->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

	}

	//Billboard for statue
	IBillboardSceneNode *billboardStatue = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(120, 200), vector3df(300, 150, 1600));
	if (billboardStatue)
	{

		billboardStatue->setMaterialFlag(EMF_LIGHTING, false);
		billboardStatue->setMaterialTexture(0, driver->getTexture("./media/GreekStatue.png"));
		billboardStatue->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

	}

	//Billboard for mask
	IBillboardSceneNode *billboardMask = scnmgr->addBillboardSceneNode(NULL, dimension2d<f32>(50, 80), vector3df(500, 120, 4600));
	if (billboardMask)
	{

		billboardMask->setMaterialFlag(EMF_LIGHTING, false);
		billboardMask->setMaterialTexture(0, driver->getTexture("./media/Mask.png"));
		billboardMask->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//TERRAIN CREATION
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Creation of terrain through custom heightmap
	ITerrainSceneNode *terrain = scnmgr->addTerrainSceneNode("./media/height.jpg", 0, -1, vector3df(0, 0, 0), vector3df(0, 0, 0), vector3df(20, 5, 20), SColor(255, 255, 255, 255), 5, ETPS_17, 4);

	//Editing terrain material properaties to add sand and stone texture
	terrain->setMaterialFlag(EMF_LIGHTING, false); //Setting lighting to false
	terrain->setMaterialFlag(EMF_FOG_ENABLE, true); //Enabling fog on the terrain
	terrain->setMaterialTexture(0, driver->getTexture("./media/Maps/snd1.jpg")); //Applying base sand texture
	terrain->setMaterialTexture(1, driver->getTexture("./media/Maps/rc1.jpg")); //Applying top rock texture
	terrain->setMaterialType(EMT_DETAIL_MAP); //Setting the detail map

	//Scaling terrain to correct size
	terrain->scaleTexture(1, 20);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//COLLISION DETECTION
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Creating triangle collision detectors for houses, rocks and terrain
	ITriangleSelector *rocksSelect = scnmgr->createTriangleSelector(rockMeshNode); //Collider for the rocks
	ITriangleSelector *colSelect = scnmgr->createTriangleSelector(colNode); //Column collider
	ITriangleSelector *house1Select = scnmgr->createTriangleSelector(house1Node); //Ruin house collider
	ITriangleSelector *house2Select = scnmgr->createTriangleSelector(house2Node); //Shed collider
	ITriangleSelector *house3Select = scnmgr->createTriangleSelector(house3Node); //Ruins collider
	ITriangleSelector *house4Select = scnmgr->createTriangleSelector(house4Node); //Ruins collider
	ITriangleSelector *house5Select = scnmgr->createTriangleSelector(house5Node); //Ruins collider
	ITriangleSelector *house6Select = scnmgr->createTriangleSelector(house6Node); //Pantheon collider
	ITriangleSelector *house7Select = scnmgr->createTriangleSelector(house7Node); //Arena collider
	ITriangleSelector *house8Select = scnmgr->createTriangleSelector(house8Node); //Ampitheatre collider
	ITriangleSelector *house9Select = scnmgr->createTriangleSelector(house9Node); //Temple collider
	ITriangleSelector *house10Select = scnmgr->createTriangleSelector(house10Node); //Watchtower collider
	ITriangleSelector *selector = scnmgr->createTerrainTriangleSelector(terrain, 0); //Terrain collider

	//Applying triangle selectors to correct nodes
	terrain->setTriangleSelector(selector);
	rockMeshNode->setTriangleSelector(rocksSelect);
	colNode->setTriangleSelector(colSelect);
	house1Node->setTriangleSelector(house1Select);
	house2Node->setTriangleSelector(house2Select);
	house3Node->setTriangleSelector(house3Select);
	house4Node->setTriangleSelector(house4Select);
	house5Node->setTriangleSelector(house5Select);
	house6Node->setTriangleSelector(house6Select);
	house7Node->setTriangleSelector(house7Select);
	house8Node->setTriangleSelector(house8Select);
	house9Node->setTriangleSelector(house9Select);
	house10Node->setTriangleSelector(house10Select);

	//Creating collision between camera and triangle collider
	ISceneNodeAnimator *TerAnim = scnmgr->createCollisionResponseAnimator(selector, FPScamera, vector3df(5, 5, 5), vector3df(0, -0.75, 0), vector3df(0, 30, 0), f32(0.05));
	ISceneNodeAnimator *RockAnim = scnmgr->createCollisionResponseAnimator(rocksSelect, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *ColAnim = scnmgr->createCollisionResponseAnimator(colSelect, FPScamera, vector3df(15, 15, 15), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House1Anim = scnmgr->createCollisionResponseAnimator(house1Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House2Anim = scnmgr->createCollisionResponseAnimator(house2Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House3Anim = scnmgr->createCollisionResponseAnimator(house3Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House4Anim = scnmgr->createCollisionResponseAnimator(house4Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House5Anim = scnmgr->createCollisionResponseAnimator(house5Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House6Anim = scnmgr->createCollisionResponseAnimator(house6Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House7Anim = scnmgr->createCollisionResponseAnimator(house7Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House8Anim = scnmgr->createCollisionResponseAnimator(house8Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House9Anim = scnmgr->createCollisionResponseAnimator(house9Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));
	ISceneNodeAnimator *House10Anim = scnmgr->createCollisionResponseAnimator(house10Select, FPScamera, vector3df(5, 5, 5), vector3df(0, 0, 0), vector3df(0, 30, 0), f32(0.09));

	//Adding collision animator to all relevant nodes
	if (TerAnim)
	{

		selector->drop();
		FPScamera->addAnimator(TerAnim);
		TerAnim->drop();

	}

	if (RockAnim)
	{

		rocksSelect->drop();
		FPScamera->addAnimator(RockAnim);
		RockAnim->drop();

	}

	if (House1Anim)
	{

		house1Select->drop();
		FPScamera->addAnimator(House1Anim);
		House1Anim->drop();

	}

	if (House2Anim)
	{

		house2Select->drop();
		FPScamera->addAnimator(House2Anim);
		House2Anim->drop();

	}

	if (House3Anim)
	{

		house3Select->drop();
		FPScamera->addAnimator(House3Anim);
		House3Anim->drop();

	}

	if (House4Anim)
	{

		house4Select->drop();
		FPScamera->addAnimator(House4Anim);
		House4Anim->drop();

	}

	if (House5Anim)
	{

		house5Select->drop();
		FPScamera->addAnimator(House5Anim);
		House5Anim->drop();

	}

	if (House6Anim)
	{

		house6Select->drop();
		FPScamera->addAnimator(House6Anim);
		House6Anim->drop();

	}

	if (House7Anim)
	{

		house7Select->drop();
		FPScamera->addAnimator(House7Anim);
		House7Anim->drop();

	}

	if (House8Anim)
	{

		house8Select->drop();
		FPScamera->addAnimator(House8Anim);
		House8Anim->drop();

	}

	if (House9Anim)
	{

		house9Select->drop();
		FPScamera->addAnimator(House9Anim);
		House9Anim->drop();

	}

	if (House10Anim)
	{

		house10Select->drop();
		FPScamera->addAnimator(House10Anim);
		House10Anim->drop();

	}

	if (ColAnim)
	{

		colSelect->drop();
		FPScamera->addAnimator(ColAnim);
		ColAnim->drop();

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//RUNTIME LOOP
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	while (device->run())
	{

		//Custom controls for movement based on world geometry - used for TP
		//Gets current position and applies it to variable
		vector3df nodeFPSPosition = FPScamera->getPosition();
		vector3df nodeTPSPosition = TPScamera->getPosition();

		//Checks if keys are pressed and applies movement based on key pressed
		if (ker.IsKeyDown(KEY_KEY_A))
		{
			//Tracks left movement
			nodeFPSPosition.Z -= 2;
			nodeTPSPosition.Z -= 2;

		}
		if (ker.IsKeyDown(KEY_KEY_D))
		{
			//Tracks right movement
			nodeFPSPosition.Z += 2;
			nodeTPSPosition.Z += 2;

		}
		if (ker.IsKeyDown(KEY_KEY_W))
		{
			//Tracks forward movement
			nodeFPSPosition.X -= 2;
			nodeTPSPosition.X -= 2;

		}
		if (ker.IsKeyDown(KEY_KEY_S))
		{
			//Tracks backward movement
			nodeFPSPosition.X += 2;
			nodeTPSPosition.X += 2;

		}
		if (ker.IsKeyDown(KEY_KEY_E))
		{
			//Track upward movement
			nodeFPSPosition.Y += 1;
			nodeTPSPosition.Y += 1;

		}
		if (ker.IsKeyDown(KEY_KEY_Q))
		{
			//Tracks downward movement
			nodeFPSPosition.Y -= 2;
			nodeTPSPosition.Y -= 2;

		}

		//XString += nodeFPSPosition.X;
		//PosX->setText(XString.c_str());

		//Sets new position based on keys pressed
		FPScamera->setPosition(nodeFPSPosition);
		TPScamera->setPosition(nodeTPSPosition);
		TPScamera->setTarget(nodeFPSPosition); //Sets the target of the third person camera to constantly be the new location of the first person camera

		//cam_target = FPScamera->getTarget();
		//cam_lookdir = cam_target - nodeFPSPosition;

		//Minimap decleration and positioning
		vector3df mapPosition = FPScamera->getPosition(); //Sets minimap position variable to be edited
		MapCamera->setPosition(vector3df(mapPosition.X, mapPosition.Y + 800, mapPosition.Z)); //Map camera set above the FPS camera location
		MapCamera->setTarget(nodeFPSPosition); //Always targets FPS camera location

		pCube->setPosition(vector3df(nodeFPSPosition.X, nodeFPSPosition.Y - 20, nodeFPSPosition.Z)); //Sets the location of the cube that represents the player underneath the FPS camera

		driver->beginScene(true, true, SColor(255, 50, 50, 50)); //Begins the scene

		//If statement checks which camera is currently selected and renders relevant camera

		switch (activeCamera)
		{

			//This case controls the FPS camera and sets the location of the TPS camera behing it while moving and not rendering
		case (0):
			scnmgr->setActiveCamera(FPScamera);
			driver->setViewPort(rect<s32>(0, 0, 650, 600));
			if (isPaused == false)
			{

				scnmgr->drawAll();

			}
			if (isPaused == true)
			{

				guienv->drawAll();

			}
			TPScamera->setPosition(vector3df(nodeFPSPosition.X - 50, nodeFPSPosition.Y + 50, nodeFPSPosition.Z - 50));
			break;

			//This case controls the TPS camera but does not move the FPS camera as it is handled by the custom key bindings
		case (1):
			scnmgr->setActiveCamera(TPScamera);
			driver->setViewPort(rect<s32>(0, 0, 650, 600));
			if (isPaused == false)
			{

				scnmgr->drawAll();

			}
			if (isPaused == true)
			{

				guienv->drawAll();

			}
			break;

			//This case controls the map camera which will always render above the FPS camera
		case (2):
			scnmgr->setActiveCamera(MapCamera);
			driver->setViewPort(rect<s32>(0, 0, 650, 600));
			if (isPaused == false)
			{

				scnmgr->drawAll();

			}
			if (isPaused == true)
			{

				guienv->drawAll();

			}
			break;

		default:
			break;

		}

		scnmgr->setActiveCamera(FPScamera); //FPS camera set by default

		s32 fps_now = driver->getFPS(); //Variable to hold the current FPS and only display during changes

		//Checks for FPS change. If change occurs, updates application header
		if (fps_now != fps_then)
		{

			stringw msg = "FPS = ";
			msg += fps_now;

			device->setWindowCaption(msg.c_str());

			fps_then = fps_now;

		}

		//Checks if player wants to change camera
		if (ker.IsKeyDown(KEY_KEY_U))
		{

			//If the game is not paused, draw FPS camera. If paused, draw GUI
			activeCamera = 0;
			scnmgr->setActiveCamera(FPScamera);
			if (isPaused == false)
			{

				scnmgr->drawAll();

			}
			if (isPaused == true)
			{

				guienv->drawAll();

			}

		}

		if (ker.IsKeyDown(KEY_KEY_I))
		{

			//If the game is not paused, draw TPS camera. If paused, draw GUI
			activeCamera = 1;
			scnmgr->setActiveCamera(TPScamera);
			if (isPaused == false)
			{

				scnmgr->drawAll();

			}
			if (isPaused == true)
			{

				guienv->drawAll();

			}

		}

		if (ker.IsKeyDown(KEY_KEY_M))
		{

			//If the game is not paused, draw minimap camera. If paused, draw GUI
			activeCamera = 2;
			scnmgr->setActiveCamera(MapCamera);
			if (isPaused == false)
			{

				scnmgr->drawAll();

			}
			if (isPaused == true)
			{

				guienv->drawAll();

			}

		}

		//Pauses the players game and presents button options
		if (ker.IsKeyDown(KEY_KEY_P) && isPaused == false)
		{

			//When P is pressed, prevents all scenes from rendering and only presents GUI
			device->getCursorControl()->setVisible(true);
			if (menuOpen == NULL)
			{

				menuOpen = soueng->play2D("./media/Audio/open.wav", false, false); //Plays sound when menu opens

			}
			if (menuOpen != NULL && menuOpen->isFinished())
			{

				menuOpen->drop();
				menuOpen = NULL;

			}

			isPaused = true;

		}

		//Unpauses the game and resumes render
		if (ker.IsKeyDown(KEY_KEY_O) && isPaused == true)
		{

			//When O is pressed, renders last camera to be used
			device->getCursorControl()->setVisible(false);
			if (menuClose == NULL)
			{

				menuClose = soueng->play2D("./media/Audio/close.wav", false, false); //Plays sound when menu closes

			}
			if (menuClose != NULL && menuClose->isFinished())
			{

				menuClose->drop();
				menuClose = NULL;

			}

			isPaused = false;

		}

		//Checking for sound collisions
		if (detectCollisionBetweenNodes(pCube, swordNode)) //Checks for collision between player and sword 
		{

			colSoundNode = swordNode;

		}

		if (colSoundNode != NULL && colSoundNode != colLastNode) //If collided and hasn't collided before, play sword sound
		{

			if (colSoundNode == swordNode)
			{

				swordCol = soueng->play2D("./media/Audio/Sword.wav"); //Plays sword sound

			}

			colLastNode = colSoundNode;

		}

		//Final step is to update the listener location for the sound engine, meaning the 3D sound will change on location
		soueng->setListenerPosition(vec3df(nodeFPSPosition.X, nodeFPSPosition.Y, nodeFPSPosition.Z), vec3df(nodeFPSPosition.X, nodeFPSPosition.Y, nodeFPSPosition.Z));

		//Applying animators
		mintoorNode->addAnimator(spintoor);
		spintoor->drop();

		driver->endScene();

	}

	soueng->drop(); //Drops the sound engine
	device->drop(); //Drops the device

	return 0;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}