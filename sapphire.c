/*								
 [x] inseriamo il volo																	|
 [x] inseriamo il piazzamento e togliemento blocchi										|
 [x] ottimizzare la generazione chunk													|05/08/26 <note: da fare con i threads>
 [x] gui blocchetti nell inventario														|
 [x] poter selzionare blocchi da piazzare												|
 [x] inserisco poi le colisioni 3D														|
 [x] gravità e il salto	 																|
 [x] inseriamo il sole, luna, alberi, montagne											|
 [x] inserisco biomi																	|
 [] inseriamo la terza persona e prima persona. 										|
 [] sezioni																				|
 [] ottimizzazione meshing
 [] update texture
*/

#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <raymath.h>
#include <stdbool.h>

// GLOBAL
#define TICKS_PER_DAY 24000
#define TICK_RATE     200.0f

#define DATA_PER_FRAME 2
#define MESH_PER_FRAME 2

#define GEN_QUEUE_SIZE 128
// SCREEN
#define WIDTH 1600
#define HEIGHT 990

#define GRAVITY 28

// GUI
#define GUI_SCALE 3.5
#define ITEM_BAR_SIZE 9
#define FONT_SIZE 20

#define SKY_DAY     CLITERAL (Color){ 102, 191, 255, 255 }  	// azzurro
#define SKY_SUNSET  CLITERAL (Color){ 252,  93,  46, 255 }  	// arancio caldo
#define SKY_SUNRISE CLITERAL (Color){ 255, 140,  80, 255 }  	// rosato
#define SKY_NIGHT  	CLITERAL (Color){   8,  10,  32, 255 }	 	// blu scuro
#define XPGREEN 	CLITERAL (Color){ 128, 255,  32, 255 } 		// XP Green

//CHUNK
#define CHUNK_SIZE 16 // ATTENZIONE!!!!!!!: changes ? modify -> GetBlockGlobal
#define CHUNK_HEIGTH 384//128
#define MAX_CHUNK_FACES (CHUNK_SIZE * CHUNK_HEIGTH * CHUNK_SIZE * 6)

// LAND
#define HEIGHT_GROUND 126
#define MIN_MOUNTAIN 35    // min height of land
#define MAX_MOUNTAIN 300   // max height
#define WATER_LEVEL_FIXED 45

#define LOCAL_WORLD_SIZE 21
#define CHUNK_RADIUS (LOCAL_WORLD_SIZE / 2)

// PLAYER
#define PLAYER_WIDTH   0.6f
#define PLAYER_HEIGHT  1.8f
#define PLAYER_EYE 	   1.6f
#define PLAYER_SPEED   4.3f
#define JUMP_SPEED     8.4f
#define SKIN_PX (PLAYER_HEIGHT / 32.0f)

#define SENS_MOUSE 	  0.15f

// RAY
#define MAX_RAY_DISTANCE 6
#define STEP_RAY_SIZE 0.05

// Mesh Generation Queue
int genQueueGX[GEN_QUEUE_SIZE];   
int genQueueGZ[GEN_QUEUE_SIZE];   
int genQueueWX[GEN_QUEUE_SIZE];   
int genQueueWZ[GEN_QUEUE_SIZE];
int genHead = 0;
int genTail = 0;

// Data Generation Queue
int dataQueueGX[GEN_QUEUE_SIZE];   
int dataQueueGZ[GEN_QUEUE_SIZE];   
int dataQueueWX[GEN_QUEUE_SIZE];   
int dataQueueWZ[GEN_QUEUE_SIZE];
int dataHead = 0;
int dataTail = 0;

// calculus for global generation
float temp_vertici[MAX_CHUNK_FACES * 4 * 3];
unsigned short temp_indici[MAX_CHUNK_FACES * 6];
float temp_texcoords[MAX_CHUNK_FACES * 4 * 2];

enum Layer {
	LAYER_SOLID = 0,
	LAYER_CUTOUT,
	LAYER_WATER,
	LAYER_COUNT
};

enum BlockType {
	AIR 		= 0,
	SAND 		= 1,
	DIRT 		= 2,
	GRASS 		= 3,
	ROCK 		= 4,
	WATER 		= 5,
	SNOW 		= 6,
	BADROCK 	= 7,
	LEAF 		= 8,
	LOG 		= 9,
	LEAF_OPAQUE = 10,
	SNOW_GRASS 	= 11,
	MAX_BLOCK_TYPES
};

enum BiomeType {
	PLAINS			= 0,
	FOREST			= 1,
	BEACH			= 2,
	MOUNTAINS_PEAKS	= 3,
	OCEAN			= 4,
	DESERT			= 5,
	MAX_BIOME_TYPES
};

char *biomeName[] = { 
	"Plains",
	"Forest",
	"Beach",
	"Mountain Peaks",
	"Ocean",
	"Desert"
};

typedef struct Time{
    long long totTicks;
    int   timeOfDay;
    float accumulator;
	unsigned int dailyPhase : 2;
}Time;

typedef struct Chunk{
    int gridX;
    int gridZ;
    char Map[CHUNK_SIZE][CHUNK_HEIGTH][CHUNK_SIZE];
    Model layers[LAYER_COUNT];
    Vector3 position;
	bool needRemesh;
}Chunk; 

typedef struct Game{
	Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE];
	Time time;
	unsigned int mode  	 	: 1; // CREATIVE (0)/ SURVIVOL(1)
	unsigned int opt_mode 	: 1; // FAST (0)/ NORMAL (1)
	unsigned int isKeyF1	: 1;
	unsigned int isKeyF2	: 1;
	unsigned int isKeyF3	: 1;
}Game;

typedef struct CameraController{
	float yaw;
	float pitch;
	float sensitivity;
}CameraController;

typedef struct BodyPart{
	Model model;
    Vector3 pivot; // articolazione
    Vector3 rot;  
}BodyPart;

typedef struct Skin{
	BodyPart head, body, armR, armL, legR, legL;
	Vector3 pos;
}Skin;

typedef struct Player{
	Vector3 position;
	Vector3 velocity;
	BoundingBox playerBox;
	Camera3D camera;
	CameraController view;
	Vector3 lookDir;
	Vector3 ray;
	float threshold_fovy;	
	Skin skin;
	
	int biome; 
	// Bit field
	unsigned int xp : 7; 
	
	unsigned int hungry 	: 5;
	unsigned int heal 		: 5;
	unsigned int xpBar 		: 5;
	int selectedSlotItemBar : 5;
	
	unsigned int isInWater 		: 1;
	unsigned int isTakingDamage : 1;
	unsigned int isCollisioning : 1;
	unsigned int isOnGround 	: 1;
	unsigned int isFlying		: 1;
	unsigned int changeThirdPerson	: 2;

	unsigned int modeOfMovement	: 2; 
	// 0:static 1:walking 2:running 
	
	unsigned int blocksInHand[ITEM_BAR_SIZE];
	RenderTexture2D blockIcons[MAX_BLOCK_TYPES];
	
}Player;

typedef struct CustomCamera{
	Camera3D *camera;
	Vector3 position; 
	float yaw; // orizzontale
	float pitch; // verticale
	float speed; 
	float ds; // sensibilità
}CustomCamera;

const int BLOCK_TEXTURE[MAX_BLOCK_TYPES] = {
			[AIR]  =   0, [SAND] = 18, [DIRT] 	=  2, [GRASS]= 3, [ROCK]= 1,
			[WATER]= 207, [SNOW] = 66, [BADROCK]= 17, [LEAF] =53, [LOG] =20,
			[LEAF_OPAQUE] = 54, [SNOW_GRASS] = 68
};


Model BuildSkinModel(Texture2D skinTex, float sx, float sy, float sz, float ox, float oy){
	Mesh mesh = {0};
    mesh.vertexCount = 24;
    mesh.triangleCount = 12;
    mesh.vertices = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)malloc(mesh.triangleCount * 3 * sizeof(unsigned short));
    mesh.texcoords = (float*)malloc(mesh.vertexCount * 2 * sizeof(float));
    
    float hx = sx/2, hy = sy/2, hz = sz/2;
    
    const float facce[6][4][3] = {
        {{-hx,-hy, hz},{ hx,-hy, hz},{ hx, hy, hz},{-hx, hy, hz}}, // FRONT
        {{ hx,-hy,-hz},{-hx,-hy,-hz},{-hx, hy,-hz},{ hx, hy,-hz}}, // BACK
        {{ hx,-hy, hz},{ hx,-hy,-hz},{ hx, hy,-hz},{ hx, hy, hz}}, // LEFT
        {{-hx,-hy,-hz},{-hx,-hy, hz},{-hx, hy, hz},{-hx, hy,-hz}}, // RIGHT
        {{-hx, hy, hz},{ hx, hy, hz},{ hx, hy,-hz},{-hx, hy,-hz}}, // TOP
        {{-hx,-hy,-hz},{ hx,-hy,-hz},{ hx,-hy, hz},{-hx,-hy, hz}}  // BOTTOM
    };
/*    
    <HEAD>
    0	8	 16	   24	32
    +----+----+----+----+
    |	   T    Bo		|
    +-------------------+
    | R		F	 L	 Ba	|
    +----+----+----+----+

*/ 
    const float uv[6][4] = {
        {ox+sz,        oy+sz, sx, sy},  // FRONT
        {ox+2*sz+sx,   oy+sz, sx, sy},  // BACK
        {ox+sz+sx,     oy+sz, sz, sy},  // LEFT
        {ox,           oy+sz, sz, sy},  // RIGHT
        {ox+sz,        oy,    sx, sz},  // TOP
        {ox+sz+sx,     oy,    sx, sz}   // BOTTOM
    };

    int vCount = 0, iCount = 0, tCount = 0;
    for(int f = 0; f < 6; f++) {
        for(int v = 0; v < 4; v++) {
            mesh.vertices[vCount*3+0] = facce[f][v][0] * SKIN_PX;
            mesh.vertices[vCount*3+1] = facce[f][v][1] * SKIN_PX;
            mesh.vertices[vCount*3+2] = facce[f][v][2] * SKIN_PX;
            vCount++;
        }

        int vBase = f * 4;
        mesh.indices[iCount+0] = vBase + 0;
        mesh.indices[iCount+1] = vBase + 1;
        mesh.indices[iCount+2] = vBase + 2;
        mesh.indices[iCount+3] = vBase + 0;
        mesh.indices[iCount+4] = vBase + 2;
        mesh.indices[iCount+5] = vBase + 3;
        iCount += 6;

        float u0 = (uv[f][0] + 0.01f) / 64.0f;
        float u1 = (uv[f][0] + uv[f][2] - 0.01f) / 64.0f;
        float v0 = (uv[f][1] + 0.01f) / 64.0f;
        float v1 = (uv[f][1] + uv[f][3] - 0.01f) / 64.0f;
        float uvs[8] = {u0,v1, u1,v1, u1,v0, u0,v0};

        for(int i = 0; i < 8; i++) mesh.texcoords[tCount++] = uvs[i];
    }

    UploadMesh(&mesh, false);
    Model model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = skinTex;
    return model;
}

void InitPlayer(struct Player *p, Texture2D skinTex){
	p->position = (Vector3){ 0.0f, 150.0f, 0.0f };
	p->velocity = (Vector3){0};
	p->isCollisioning 	= 0;
	p->isOnGround 		= 0;
	p->isFlying 		= 1;
	p->changeThirdPerson	= 0;

	p->view.yaw 		= 180.0f;
	p->view.pitch 		= 0.0f;
	p->view.sensitivity = 0.15f;

	p->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
	p->camera.fovy = 72.0f;
	p->threshold_fovy = cosf((p->camera.fovy / 2) * DEG2RAD);
	p->camera.projection = CAMERA_PERSPECTIVE;
	p->camera.position = (Vector3){ p->position.x, p->position.y + PLAYER_EYE, p->position.z };
	p->camera.target = Vector3Add(p->camera.position, (Vector3){0.0f, 0.0f, -1.0f});
	
    p->xp = 4;
    p->hungry = 20;
    p->heal = 20;
    p->xpBar = 0;
    p->selectedSlotItemBar = 0;
    p->isInWater = 0;
    p->isTakingDamage = 0;

	p->biome = -1;

	p->blocksInHand[0] = GRASS;
	p->blocksInHand[1] = AIR;
	p->blocksInHand[2] = SAND;
	p->blocksInHand[3] = LEAF;
	p->blocksInHand[4] = SNOW;
	p->blocksInHand[5] = WATER;
	p->blocksInHand[6] = ROCK;
	p->blocksInHand[7] = BADROCK;
	p->blocksInHand[8] = LOG;
	
	// SKIN
	//p->skin = (Skin)malloc(sizeof(Skin));
	p->skin.head.model = BuildSkinModel(skinTex, 8, 8, 8, 0, 0);
	p->skin.body.model = BuildSkinModel(skinTex, 8, 12, 4, 16, 16);
	p->skin.armR.model = BuildSkinModel(skinTex, 4, 12, 4, 40, 16);
	p->skin.armL.model = BuildSkinModel(skinTex,4, 12, 4, 32, 48);
	p->skin.legR.model = BuildSkinModel(skinTex, 4, 12, 4,  0, 16);
	p->skin.legL.model = BuildSkinModel(skinTex, 4, 12, 4, 16, 48);
	
	p->skin.body.pivot = (Vector3){               0,				  0, 0 };   
	p->skin.head.pivot = (Vector3){               0,	24.0f * SKIN_PX, 0 };  
	p->skin.armR.pivot = (Vector3){ -5.0f * SKIN_PX, 	22.0f * SKIN_PX, 0 };
	p->skin.armL.pivot = (Vector3){  5.0f * SKIN_PX, 	22.0f * SKIN_PX, 0 };
	p->skin.legR.pivot = (Vector3){ -2.0f * SKIN_PX, 	12.0f * SKIN_PX, 0 }; 
	p->skin.legL.pivot = (Vector3){  2.0f * SKIN_PX, 	12.0f * SKIN_PX, 0 };
	
	p->skin.pos = p->position;
}

BoundingBox PosToBox(Vector3 *pos){
	float dw = PLAYER_WIDTH / 2;
	return (BoundingBox){ 
			    	{ pos->x - dw, pos->y,			pos->z - dw},
			    	{ pos->x + dw, pos->y + PLAYER_HEIGHT,	pos->z + dw}
			    };
}

int IsTransparent(int blockID) {
    return (blockID == AIR || blockID == WATER || blockID == LEAF);
}

void DrawGUI(Texture2D gui, Texture2D ascii, Player *player){
	// Draw central cross 
	Rectangle rec_cross = {617.0f, 480.0f, 9.0f, 9.0f};
	Rectangle pos_cross = {WIDTH/2 - (4.5 * GUI_SCALE), HEIGHT/2 - (4.5 * GUI_SCALE), 9.0f * GUI_SCALE, 9.0f * GUI_SCALE};
	DrawTexturePro(gui, rec_cross, pos_cross, (Vector2) {0}, 0.0f, WHITE);
	
	// Draw item bar
	Rectangle rec_item_bar = {72.0f, 457.0f, 182.0f, 22.0f};
	Rectangle pos_item_bar = {WIDTH/2 - (91 * GUI_SCALE), HEIGHT - (23*GUI_SCALE), 182.0f * GUI_SCALE, 22.0f * GUI_SCALE};
	DrawTexturePro(gui, rec_item_bar, pos_item_bar, (Vector2) {0}, 0.0f, WHITE);
	
	// Draw texture items in item bar
	for(int i = 0; i < ITEM_BAR_SIZE; i++){
		int id_block = player->blocksInHand[i];
		if(id_block != AIR){
			float icon_size = 16 * GUI_SCALE;
			int posX = WIDTH/2 - ((91-3 - (i * 20)) * GUI_SCALE);
			int posY = HEIGHT - ((23- 3)*GUI_SCALE);	
			Rectangle src = {0.0f, 0.0f,
							player->blockIcons[id_block].texture.width, 
							-player->blockIcons[id_block].texture.height};
			Rectangle dest = {posX, posY, icon_size, icon_size};
			DrawTexturePro(player->blockIcons[id_block].texture, src, dest,(Vector2){0,0}, 0.0f, WHITE);  
		}
	}
	
	// Draw item square GetMouseWheelMove
	Rectangle rec_item_box = {48.0f, 457.0f, 24.0f, 24.0f};
	Rectangle pos_item_box = {WIDTH/2 - ((92 - (player->selectedSlotItemBar * 20)) * GUI_SCALE), HEIGHT - (24*GUI_SCALE), 24.0f * GUI_SCALE, 24.0f * GUI_SCALE};
	DrawTexturePro(gui, rec_item_box, pos_item_box, (Vector2) {0}, 0.0f, WHITE);
	
	
	// Draw experience bar
	Rectangle rec_exp_bar = {822.0f, 110.0f, 182.0f, 5.0f};
	Rectangle pos_exp_bar = {WIDTH/2 - (91 * GUI_SCALE), HEIGHT - ((23+7)*GUI_SCALE), 182.0f * GUI_SCALE, 5.0f * GUI_SCALE};
	DrawTexturePro(gui, rec_exp_bar, pos_exp_bar, (Vector2) {0}, 0.0f, WHITE);	

	// Draw healt heart
	for(int i = 0; i < 10; i++){
		// Draw edge healt
		Rectangle rec_heart_edge = {225.0f, 501.0f, 9.0f, 9.0f};
		Rectangle pos_heart_edge = {WIDTH/2 - ((92-(i * 8)) * GUI_SCALE), HEIGHT - ((23 + 17)*GUI_SCALE), 9.0f * GUI_SCALE, 9.0f * GUI_SCALE};
		DrawTexturePro(gui, rec_heart_edge, pos_heart_edge, (Vector2) {0}, 0.0f, WHITE);
		// ---------------
		Rectangle rec_heart = {82.0f, 502.0f, 7.0f, 7.0f};
		Rectangle pos_heart = {WIDTH/2 - ((91-(i * 8)) * GUI_SCALE), HEIGHT - ((23 + 16)*GUI_SCALE), 7.0f * GUI_SCALE, 7.0f * GUI_SCALE};
		DrawTexturePro(gui, rec_heart, pos_heart, (Vector2) {0}, 0.0f, WHITE);
	}
	
	// Draw cosciotti, hunger
	for(int i = 0; i < 10; i++){
		// Draw edge hunger
		Rectangle rec_hunger_edge = {177.0f, 242.0f, 9.0f, 9.0f};
		Rectangle pos_hunger_edge = {WIDTH/2 + ((81-(i * 8)) * GUI_SCALE), HEIGHT - ((23 + 17)*GUI_SCALE), 9.0f * GUI_SCALE, 9.0f * GUI_SCALE};
		DrawTexturePro(gui, rec_hunger_edge, pos_hunger_edge, (Vector2) {0}, 0.0f, WHITE);
		// ---------------		
		Rectangle rec_hunger = {196.0f, 243.0f, 7.0f, 7.0f};
		Rectangle pos_hunger = {WIDTH/2 + ((82-(i * 8)) * GUI_SCALE), HEIGHT - ((23 + 16)*GUI_SCALE), 7.0f * GUI_SCALE, 7.0f * GUI_SCALE};
		DrawTexturePro(gui, rec_hunger, pos_hunger, (Vector2) {0}, 0.0f, WHITE);
	}
	
	// Draw ascii number
	float dx = player->xp * 8;
	Rectangle rec_number_edge = {dx, 24.0f, 5.0f, 7.0f};
	Rectangle pos_number_edge = {WIDTH/2 - (2 * GUI_SCALE), HEIGHT - (((23 + 12+0.5)*GUI_SCALE)), 5.0f * GUI_SCALE, 7.0f * GUI_SCALE};
	DrawTexturePro(ascii, rec_number_edge, pos_number_edge, (Vector2) {0}, 0.0f, BLACK);
	// ---------------		
	Rectangle rec_number = {dx, 24.0f, 5.0f, 7.0f};
	Rectangle pos_number = {WIDTH/2 - ((2.5) * GUI_SCALE), HEIGHT - (((23 + 12)*GUI_SCALE)), 5.0f * GUI_SCALE, 7.0f * GUI_SCALE};
	DrawTexturePro(ascii, rec_number, pos_number, (Vector2) {0}, 0.0f, XPGREEN);
	
}

void UpgradeTime(Time *t, float dt){
	t->accumulator += dt;
	while(t->accumulator >= 1.0f / TICK_RATE){
		t->accumulator -= 1.0f / TICK_RATE;
		t->totTicks++;
		t->timeOfDay = (t->timeOfDay + 1) % TICKS_PER_DAY;
	}
}

void DrawSun(Vector3 *position, Time *t, Model sunModel, Model moonModel){
	float phase = (float)t->timeOfDay / (float)TICKS_PER_DAY; // 0 sunrise 0.25 mezzogiorno
	t->dailyPhase = (int)phase;
	float angle = phase * 2.0f * PI;
	float sunRadius = 2000.f;
	float moonRadius = 2000.0f;
	
	float cx = position->x;
    //float cy = position->y;
    float cz = position->z;

    Vector3 sunPos = {
        cx + cosf(angle) * sunRadius,
        64 + sinf(angle) * sunRadius,
        cz
    };
    Vector3 moonPos = {
        cx - cosf(angle) * moonRadius,
        64 - sinf(angle) * moonRadius,
        cz
    };

    Vector3 rotationAx = { 0.0f, 0.0f, 1.0f };
    float rotationAngle = (angle * RAD2DEG) + 90.0f;

    DrawModelEx(sunModel,  sunPos,  rotationAx, rotationAngle,
                (Vector3){1.0f, 1.0f, 1.0f}, WHITE);
    DrawModelEx(moonModel, moonPos, rotationAx, rotationAngle + 180.0f,
                (Vector3){1.0f, 1.0f, 1.0f}, WHITE);
}

void UpdateCustomCamera(struct CustomCamera *cam, float dt){
    // MOUSE LOOK
    Vector2 mouseDelta = GetMouseDelta();
    cam->yaw -= mouseDelta.x * cam->ds;
    cam->pitch -= mouseDelta.y * cam->ds;

    // limit orizzontal view
    if (cam->pitch > 89.0f)
        cam->pitch = 89.0f;

    if (cam->pitch < -89.0f)
        cam->pitch = -89.0f;
	
    // DIREZIONE TELECAMERA
    Vector3 forward = {
        cosf(DEG2RAD * cam->pitch) * sinf(DEG2RAD * cam->yaw),
        sinf(DEG2RAD * cam->pitch),
        cosf(DEG2RAD * cam->pitch) * cosf(DEG2RAD * cam->yaw)
    };
	
    forward = Vector3Normalize(forward);

    // Destra della camera
    Vector3 right = Vector3Normalize(
        Vector3CrossProduct(forward, (Vector3){0,1,0})
    );

    // MOVING WSDA
    float currentSpeed = cam->speed;

    if (IsKeyDown(KEY_LEFT_CONTROL))
        currentSpeed *= 2.0f;

    if (IsKeyDown(KEY_W))
		cam->position = Vector3Add( cam->position,
							Vector3Scale(forward, currentSpeed * dt));

    if (IsKeyDown(KEY_S))
        cam->position = Vector3Subtract( cam->position,
							Vector3Scale(forward, currentSpeed * dt));

    if (IsKeyDown(KEY_D))
        cam->position = Vector3Add( cam->position,
							Vector3Scale(right, currentSpeed * dt));

    if (IsKeyDown(KEY_A)){
        cam->position = Vector3Subtract(cam->position, 
							Vector3Scale(right, currentSpeed * dt));}
	
	if (IsKeyDown(KEY_SPACE))
        cam->position.y += currentSpeed * dt;

    if (IsKeyDown(KEY_LEFT_SHIFT))
        cam->position.y -= currentSpeed * dt; 

    // Update raylib Camera
    cam->camera->position = cam->position;
    cam->camera->target = Vector3Add(cam->position, forward);	
	
}

float PerlinNoise(float x, float y, float z) {
	static const int p[512] = { 151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
                      140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
                      247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
                       57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
                       74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
                       60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
                       65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
                      200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
                       52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
                      207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
                      119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
                      129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
                      218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
                       81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
                      184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
                      222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180, 
                    
                    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
                      140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
                      247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
                       57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
                       74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
                       60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
                       65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
                      200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
                       52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
                      207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
                      119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
                      129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
                      218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
                       81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
                      184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
                      222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180
                    };

float Fadealg(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float Lerpalg(float t, float a, float b) { return a + t * (b - a); }
float Grad(int hash, float dx, float dy, float dz) {
    int h = hash & 15; 
    float u = h < 8 ? dx : dy; 
    float v = h < 4 ? dy : (h == 12 || h == 14 ? dx : dz); 
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

	
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;
    int Z = (int)floorf(z) & 255;

    x -= floorf(x);
    y -= floorf(y);
    z -= floorf(z);

    float u = Fadealg(x);
    float v = Fadealg(y);
    float w = Fadealg(z);

    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    float mediaAsseZ_Fronte = Lerpalg(v, 
        Lerpalg(u, Grad(p[AA], x, y, z),           Grad(p[BA], x - 1, y, z)),
        Lerpalg(u, Grad(p[AB], x, y - 1, z),       Grad(p[BB], x - 1, y - 1, z))
    );
    float mediaAsseZ_Retro = Lerpalg(v, 
        Lerpalg(u, Grad(p[AA + 1], x, y, z - 1), Grad(p[BA + 1], x - 1, y, z - 1)),
        Lerpalg(u, Grad(p[AB + 1], x, y - 1, z - 1), Grad(p[BB + 1], x - 1, y - 1, z - 1))
    );
    return Lerpalg(w, mediaAsseZ_Fronte, mediaAsseZ_Retro);
}

float FractalBrownianMotion(float x, float z, int octaves, float persistence, float lacunarity, float scale) {
    float total = 0.0f;
    float frequency = 1.0f / scale;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for(int i = 0; i < octaves; i++) {
        total += PerlinNoise(x * frequency, 0.0f, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return total / maxValue; 
}

// makes no sense that humidity and temperature influence BEACH AND OCEAN but now idc
int GetBiome(float gx, float gz){
	float temp  = PerlinNoise(gx*0.0015f + 500.0f,  0.0f, gz*0.0015f + 500.0f);
    float humid = PerlinNoise(gx*0.0015f + 9000.0f, 0.0f, gz*0.0015f + 9000.0f);

	if(temp < -0.20f) return (humid < 0.0f) ? MOUNTAINS_PEAKS : FOREST;
    if(temp > 0.20f) return (humid < 0.0f) ? BEACH : DESERT;
    return (humid > 0.4) ? OCEAN : PLAINS; 
}

void UpdatePlayerStats(Player *player){
	
	int move = -(int)GetMouseWheelMove();
    if (move != 0) {
		int newSlot = player->selectedSlotItemBar + move;
		player -> selectedSlotItemBar = (newSlot % ITEM_BAR_SIZE + ITEM_BAR_SIZE) % ITEM_BAR_SIZE;
	}
	// BIOME
	player-> biome = GetBiome(player->position.x, player->position.z);
}

void GetCoordinatesFromAtlas(int textureID, int vertexID, float *u_out, float *v_out){
	int img_col = 16;
	int img_row = 16;
	
	int tex_col = textureID % img_col;
	int tex_row = textureID / img_row;
	
	float img_percent_width = 1.0f / img_col;
	float img_percent_height = 1.0f / img_row;
	
	float topX = img_percent_width * tex_col;
	float topY = img_percent_height * tex_row;
	
	if(vertexID == 0) { *u_out = topX; 						*v_out = topY; }
	if(vertexID == 1) { *u_out = topX; 						*v_out = topY + img_percent_height; }
	if(vertexID == 2) { *u_out = topX + img_percent_width; 	*v_out = topY; }
	if(vertexID == 3) { *u_out = topX + img_percent_width; 	*v_out = topY + img_percent_height; } 
}

static void DrawPart(Model m, Vector3 base, float ox, float oy, float yaw){
	Vector3 off = Vector3RotateByAxisAngle((Vector3){ ox*SKIN_PX, oy*SKIN_PX, 0.0f },
	                                       (Vector3){0,1,0}, DEG2RAD * yaw);
	DrawModelEx(m, Vector3Add(base, off), (Vector3){0,1,0}, yaw, (Vector3){1,1,1}, WHITE);
}

void DrawSkin(Skin *skin, float yaw){
	DrawPart(skin->head.model, skin->pos,  0, 28, yaw);
	DrawPart(skin->body.model, skin->pos,  0, 18, yaw);
	DrawPart(skin->armL.model, skin->pos,  6, 18, yaw);
	DrawPart(skin->armR.model, skin->pos, -6, 18, yaw);
	DrawPart(skin->legL.model, skin->pos,  2,  6, yaw);
	DrawPart(skin->legR.model, skin->pos, -2,  6, yaw);
}

Model BuildItemModel(int block) {
    Mesh mesh = {0};
    mesh.vertexCount = 24;
    mesh.triangleCount = 12;
    mesh.vertices = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)malloc(mesh.triangleCount * 3 * sizeof(unsigned short));
    mesh.texcoords = (float*)malloc(mesh.vertexCount * 2 * sizeof(float));

    int vCount = 0, iCount = 0, tCount = 0;
	int textureID = BLOCK_TEXTURE[(int) block];

    // Veritci coordinates for a (-0.5 0.5) cube
    const float facce[6][4][3] = {
        {{-0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}, // Upper
        {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}}, // Under
        {{0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}}, // DX
        {{-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}}, // SX
        {{-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}}, // BACK
        {{0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}}  // FRONT
    };

    for(int f = 0; f < 6; f++) {
        // vertices
        for(int v = 0; v < 4; v++) {
            mesh.vertices[vCount*3+0] = facce[f][v][0];
            mesh.vertices[vCount*3+1] = facce[f][v][1];
            mesh.vertices[vCount*3+2] = facce[f][v][2];
            vCount++;
        }

        // index
        int vBase = f * 4;
        mesh.indices[iCount+0] = vBase + 0;
        mesh.indices[iCount+1] = vBase + 1;
        mesh.indices[iCount+2] = vBase + 2;
        mesh.indices[iCount+3] = vBase + 1;
        mesh.indices[iCount+4] = vBase + 3;
        mesh.indices[iCount+5] = vBase + 2;
        iCount += 6;

        int tmpFaceID = textureID;
        if(block == GRASS && f == 0) tmpFaceID = 0;
        if(block == GRASS && f == 1) tmpFaceID = 2;

        for(int nVer = 0; nVer < 4; nVer++){
            float u, v;
            GetCoordinatesFromAtlas(tmpFaceID, nVer, &u, &v);
            mesh.texcoords[tCount+0] = u;
            mesh.texcoords[tCount+1] = v;
            tCount += 2;
        }
    }

    UploadMesh(&mesh, false);
    return LoadModelFromMesh(mesh);
}

void InitTextureInventary(Texture2D terrain, Player *player){
	Camera3D Texturecam = {	{2.5f, 2.5f, 2.5f},	// position
							{0.0f, 0.0f, 0.0f},	// target
							{0.0f, 1.0f, 0.0f},	// up
							2.2f,				// fovy
							CAMERA_ORTHOGRAPHIC};// projection

	for(int i = 0; i < MAX_BLOCK_TYPES; i++){
		player->blockIcons[i] = LoadRenderTexture(128, 128);
			
		if (i != AIR) {
			Model itemModel = BuildItemModel(i);
			itemModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = terrain;

			BeginTextureMode(player->blockIcons[i]);
				ClearBackground(BLANK);
				
				BeginMode3D(Texturecam);
					DrawModel(itemModel, (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
					//DrawCubeWires(Vector3Zero(), 1.0f, 1.0f, 1.0f, BLACK);
				EndMode3D();
			EndTextureMode();

			UnloadModel(itemModel); 
		}
	}
}

void BuildTree(Chunk *c, int x, int y, int z){
    for(int j = y + 1; j < y + 7 && j < CHUNK_HEIGTH; j++) {
        c->Map[x][j][z] = LOG;
    }
    int top = y + 7;
    for(int kx = x - 2; kx <= x + 2; kx++){
        for(int kz = z - 2; kz <= z + 2; kz++){
            for(int ky = top - 2; ky <= top; ky++){
                if(kx >= 0 && kx < CHUNK_SIZE && 
                   kz >= 0 && kz < CHUNK_SIZE && 
                   ky >= 0 && ky < CHUNK_HEIGTH) {
                    if(c->Map[kx][ky][kz] == AIR) {
                        c->Map[kx][ky][kz] = LEAF;
                    }
                }
            }
        }
    }
}

/*
 * 
 *	BuildChunkMesh   "devo disegnare questa faccia?" pieno — non la disegno
 *	BoxColliderWorld "ci sbatto contro?"	aria — non ci sbatti
 *	Delete/PlaceBlockRay "il raggio ha colpito qualcosa?"	aria — non hai colpito
*/
char GetBlockMain(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int gx, int gy, int gz, char OUT){
    if(gy >= CHUNK_HEIGTH) 	return AIR;
    if(gy < 0) 				return OUT;
	
	int chunkX = gx >> 4; // Sarebbe come dividere per 16, anche per numeri negativi
	int chunkZ = gz >> 4;
	
	int wx = (chunkX % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
    int wz = (chunkZ % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
	
	if (world[wx][wz].gridX != chunkX || world[wx][wz].gridZ != chunkZ) {
        return OUT; 
    }
	
	int lx = gx & 15; // Identico a %16 anche con negativi 
    int lz = gz & 15;
	
	return world[wx][wz].Map[lx][gy][lz];
}

char GetBlockGlobal(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int gx, int gy, int gz){
	return GetBlockMain(world, gx, gy, gz, AIR);
}

static char GetBlockForMesh(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int gx, int gy, int gz){
	return GetBlockMain(world, gx, gy, gz, ROCK);
}

void BuildChunkData(Chunk *c, int gX, int gZ){
	int HEIGHTGROUND;
	int WATER_LEVEL;
	c->gridX = gX;
	c->gridZ = gZ;
	c->position = (Vector3){gX * CHUNK_SIZE, 0.0f, gZ * CHUNK_SIZE};
	
	int octaves = 6;
	float persistence = 0.5f;
	float lacunarity = 2.0f; 
	float scale = 150.0f; // durezza e morbidezza
	
	memset(c->Map, 0, sizeof(c->Map));
	//MAP / floor
    for(int x = 0; x < CHUNK_SIZE; x++){
        for(int z = 0; z < CHUNK_SIZE; z++){
        	float globalX = (gX * CHUNK_SIZE) + x;
        	float globalZ = (gZ * CHUNK_SIZE) + z;
            //float noise = PerlinNoise(globalX * 0.09f, 0.0f, globalZ * 0.09f);
			// NOISE Ground Level
			float noise = FractalBrownianMotion(globalX, globalZ, octaves, persistence, lacunarity, scale);
			/*float baseNoise = PerlinNoise(globalX * 0.002f + 1000.0f, 0.0f, globalZ * 0.002f + 1000.0f);
            int mountain_off = (int) (baseNoise * 25);*/
            float noiseNorm = pow(noise + 0.5f, 2.0f);
            if(noiseNorm > 1.0f) noiseNorm = 1.0f;
			if(noiseNorm < 0.0f) noiseNorm = 0.0f;
            HEIGHTGROUND = (int)(noiseNorm * HEIGHT_GROUND) + 2; //+ mountain_off*/;
            /*if (HEIGHTGROUND < 3) HEIGHTGROUND = 3;
            if (HEIGHTGROUND > CHUNK_HEIGTH - 10) HEIGHTGROUND = CHUNK_HEIGTH - 10;*/
            
			float waterNoiseNorm = pow((-0.2f + 1.0f) / 2.0f, 2.0f);
            WATER_LEVEL = (int)(waterNoiseNorm * HEIGHT_GROUND) + 2;

/*
 * 	float t = noise / 0.5f;              // il rumore FBM sta circa in [-0.5, 0.5]
	if(t >  1.0f) t =  1.0f;
	if(t < -1.0f) t = -1.0f;
	t = (t + 1.0f) * 0.5f;               // ora t sta in [0, 1]

	HEIGHTGROUND = MIN_MOUNTAIN + (int)(t * (MAX_MOUNTAIN - MIN_MOUNTAIN));
	WATER_LEVEL  = WATER_LEVEL_FIXED;
 * */

            for(int y = 0; y < HEIGHTGROUND; y++){
				
				if(y == 0) {c->Map[x][y][z] = BADROCK;}
				else if (noise < -0.3f) { 
                    if (y >= HEIGHTGROUND - 2) c->Map[x][y][z] = SAND;
                    else c->Map[x][y][z] = ROCK;
                } 
                else if (noise < -0.15f) { 
                    if (y >= HEIGHTGROUND - 3) c->Map[x][y][z] = SAND; 
                    else c->Map[x][y][z] = ROCK; 
                } 
				else {
                    if (HEIGHTGROUND > WATER_LEVEL + 28) { 
			int snowLine = (int)(PerlinNoise(globalX*0.04f + 9000.0f, 0.5f, globalZ*0.04f + 9000.0f) * 24.0f);
                        if (y >= HEIGHTGROUND - 5 && y >= 61 + snowLine) c->Map[x][y][z] = SNOW;
                        else c->Map[x][y][z] = ROCK;
                    } 
                    else if (HEIGHTGROUND > WATER_LEVEL + 20) { 
                        c->Map[x][y][z] = ROCK;
                    } 
                    else { 
                        if (y == HEIGHTGROUND - 1){ 
							c->Map[x][y][z] = GRASS;
							if(y == HEIGHTGROUND - 1) { if(x == z && x == CHUNK_SIZE/2)BuildTree( c, x, y, z); }
						}
                        else if (y > HEIGHTGROUND - 4) c->Map[x][y][z] = DIRT;
                        else c->Map[x][y][z] = ROCK;
                    }
                }
            }

            if (HEIGHTGROUND <= WATER_LEVEL) {
                for (int y = HEIGHTGROUND; y <= WATER_LEVEL; y++) {
                    c->Map[x][y][z] = WATER;
                }
            }
        }    
    }
}

int LayerOfBlock(char block){
    if (block == WATER) return LAYER_WATER;
    if (block == LEAF)  return LAYER_CUTOUT;
    return LAYER_SOLID;
}

void BuildFaces(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], 
						Chunk *c, int gX, int gZ, int *vC, int * iC, int *tC, int layer){

	float *vertici = temp_vertici;
	unsigned short *indici = temp_indici;
	float *texcoords = temp_texcoords;
        
    int vCount = 0;
    int iCount = 0;
    int tCount = 0;

    for(int x = 0; x < CHUNK_SIZE; x++){
        for(int y = 0; y < CHUNK_HEIGTH; y++){
            for(int z = 0; z < CHUNK_SIZE; z++){  
			
                char block = c->Map[x][y][z];
				if(block == AIR) continue;
                
				if (LayerOfBlock(block) != layer) continue;
				
				int globalX = (gX * CHUNK_SIZE) + x;
				int globalZ = (gZ * CHUNK_SIZE) + z;
								
                int textureID = BLOCK_TEXTURE[(int)block];
				char neighbor = (y == CHUNK_HEIGTH - 1) ? AIR : c->Map[x][y+1][z];				
				//Upper Face
                if(IsTransparent(neighbor) && neighbor != block){
                    vertici[vCount*3+0] = x + 0.0f; vertici[vCount*3+1] = y + 1.0f; vertici[vCount*3+2] = z + 0.0f;
                    vertici[vCount*3+3] = x + 0.0f; vertici[vCount*3+4] = y + 1.0f; vertici[vCount*3+5] = z + 1.0f;
                    vertici[vCount*3+6] = x + 1.0f; vertici[vCount*3+7] = y + 1.0f; vertici[vCount*3+8] = z + 0.0f;
                    vertici[vCount*3+9] = x + 1.0f; vertici[vCount*3+10] = y + 1.0f; vertici[vCount*3+11] = z + 1.0f;

                    indici[iCount+0] = vCount + 0; indici[iCount+1] = vCount + 1; indici[iCount+2] = vCount + 2;
                    indici[iCount+3] = vCount + 1; indici[iCount+4] = vCount + 3; indici[iCount+5] = vCount + 2;
                    
                    int tmpFaceID = textureID == 3 ? 0 : textureID;
                    
                    for(int nVer = 0; nVer < 4; nVer++){
                        float u, v;
                        GetCoordinatesFromAtlas(tmpFaceID, nVer, &u, &v);
                        texcoords[tCount+0] = u;
                        texcoords[tCount+1] = v;
                        tCount += 2;
                    }

                    vCount += 4; 
                    iCount += 6;
                }
                //Under Face    
				neighbor = (y == 0) ? AIR : c->Map[x][y-1][z];
				if(IsTransparent(neighbor) && neighbor != block){
                    vertici[vCount*3+0] = x + 0.0f; vertici[vCount*3+1] = y + 0.0f; vertici[vCount*3+2] = z + 0.0f;
                    vertici[vCount*3+3] = x + 1.0f; vertici[vCount*3+4] = y + 0.0f; vertici[vCount*3+5] = z + 0.0f;
                    vertici[vCount*3+6] = x + 0.0f; vertici[vCount*3+7] = y + 0.0f; vertici[vCount*3+8] = z + 1.0f;
                    vertici[vCount*3+9] = x + 1.0f; vertici[vCount*3+10] = y + 0.0f; vertici[vCount*3+11] = z + 1.0f;

                    indici[iCount+0] = vCount + 0; indici[iCount+1] = vCount + 1; indici[iCount+2] = vCount + 2;
                    indici[iCount+3] = vCount + 1; indici[iCount+4] = vCount + 3; indici[iCount+5] = vCount + 2;

					int tmpFaceID = textureID == 3 ? 2 : textureID;

                    for(int nVer = 0; nVer < 4; nVer++){
                        float u, v;
                        GetCoordinatesFromAtlas(tmpFaceID, nVer, &u, &v);
                        texcoords[tCount+0] = u;
                        texcoords[tCount+1] = v;
                        tCount += 2;
                    }

                    vCount += 4; 
                    iCount += 6;
                }                
                //DX Face ->
				neighbor = (x == CHUNK_SIZE - 1) ? GetBlockForMesh(world, globalX + 1, y, globalZ) : c->Map[x + 1][y][z];
                if(IsTransparent(neighbor) && neighbor != block){
                    vertici[vCount*3+0] = x + 1.0f; vertici[vCount*3+1] = y + 1.0f; vertici[vCount*3+2] = z + 1.0f; // Top-Left
                    vertici[vCount*3+3] = x + 1.0f; vertici[vCount*3+4] = y + 0.0f; vertici[vCount*3+5] = z + 1.0f; // Bottom-Left
                    vertici[vCount*3+6] = x + 1.0f; vertici[vCount*3+7] = y + 1.0f; vertici[vCount*3+8] = z + 0.0f; // Top-Right
                    vertici[vCount*3+9] = x + 1.0f; vertici[vCount*3+10] = y + 0.0f; vertici[vCount*3+11] = z + 0.0f; // Bottom-Right

                    indici[iCount+0] = vCount + 0; indici[iCount+1] = vCount + 1; indici[iCount+2] = vCount + 2;
                    indici[iCount+3] = vCount + 1; indici[iCount+4] = vCount + 3; indici[iCount+5] = vCount + 2;

                    for(int nVer = 0; nVer < 4; nVer++){
                        float u, v;
                        GetCoordinatesFromAtlas(textureID, nVer, &u, &v);
                        texcoords[tCount+0] = u; texcoords[tCount+1] = v; tCount += 2;
                    }
                    vCount += 4; iCount += 6;
                }
                //SX Face <-
				neighbor = (x == 0) ? GetBlockForMesh(world, globalX - 1, y, globalZ) : c->Map[x - 1][y][z];
                if(IsTransparent(neighbor) && neighbor != block){
                    vertici[vCount*3+0] = x + 0.0f; vertici[vCount*3+1] = y + 1.0f; vertici[vCount*3+2] = z + 0.0f;
                    vertici[vCount*3+3] = x + 0.0f; vertici[vCount*3+4] = y + 0.0f; vertici[vCount*3+5] = z + 0.0f;
                    vertici[vCount*3+6] = x + 0.0f; vertici[vCount*3+7] = y + 1.0f; vertici[vCount*3+8] = z + 1.0f;
                    vertici[vCount*3+9] = x + 0.0f; vertici[vCount*3+10] = y + 0.0f; vertici[vCount*3+11] = z + 1.0f;

                    indici[iCount+0] = vCount + 0; indici[iCount+1] = vCount + 1; indici[iCount+2] = vCount + 2;
                    indici[iCount+3] = vCount + 1; indici[iCount+4] = vCount + 3; indici[iCount+5] = vCount + 2;

                    for(int nVer = 0; nVer < 4; nVer++){
                        float u, v;
                        GetCoordinatesFromAtlas(textureID, nVer, &u, &v);
                        texcoords[tCount+0] = u; texcoords[tCount+1] = v; tCount += 2;
                    }
                    vCount += 4; iCount += 6;
                }
                //BACK Face
				neighbor = (z == CHUNK_SIZE - 1) ? GetBlockForMesh(world, globalX, y, globalZ + 1) : c->Map[x][y][z + 1];
                if(IsTransparent(neighbor) && neighbor != block){
                    vertici[vCount*3+0] = x + 0.0f; vertici[vCount*3+1] = y + 1.0f; vertici[vCount*3+2] = z + 1.0f; 
                    vertici[vCount*3+3] = x + 0.0f; vertici[vCount*3+4] = y + 0.0f; vertici[vCount*3+5] = z + 1.0f; 
                    vertici[vCount*3+6] = x + 1.0f; vertici[vCount*3+7] = y + 1.0f; vertici[vCount*3+8] = z + 1.0f; 
                    vertici[vCount*3+9] = x + 1.0f; vertici[vCount*3+10] = y + 0.0f; vertici[vCount*3+11] = z + 1.0f;

                    indici[iCount+0] = vCount + 0; indici[iCount+1] = vCount + 1; indici[iCount+2] = vCount + 2;
                    indici[iCount+3] = vCount + 1; indici[iCount+4] = vCount + 3; indici[iCount+5] = vCount + 2;

                    for(int nVer = 0; nVer < 4; nVer++){
                        float u, v;
                        GetCoordinatesFromAtlas(textureID, nVer, &u, &v);
                        texcoords[tCount+0] = u; texcoords[tCount+1] = v; tCount += 2;
                    }
                    vCount += 4; iCount += 6;
                }
                //FRONT Face
				neighbor = (z == 0) ? GetBlockForMesh(world, globalX, y, globalZ - 1) : c->Map[x][y][z - 1];
                if(IsTransparent(neighbor) && neighbor != block){
                    vertici[vCount*3+0] = x + 1.0f; vertici[vCount*3+1] = y + 1.0f; vertici[vCount*3+2] = z + 0.0f; 
                    vertici[vCount*3+3] = x + 1.0f; vertici[vCount*3+4] = y + 0.0f; vertici[vCount*3+5] = z + 0.0f; 
                    vertici[vCount*3+6] = x + 0.0f; vertici[vCount*3+7] = y + 1.0f; vertici[vCount*3+8] = z + 0.0f; 
                    vertici[vCount*3+9] = x + 0.0f; vertici[vCount*3+10] = y + 0.0f; vertici[vCount*3+11] = z + 0.0f;

                    indici[iCount+0] = vCount + 0; indici[iCount+1] = vCount + 1; indici[iCount+2] = vCount + 2;
                    indici[iCount+3] = vCount + 1; indici[iCount+4] = vCount + 3; indici[iCount+5] = vCount + 2;

                    for(int nVer = 0; nVer < 4; nVer++){
                        float u, v;
                        GetCoordinatesFromAtlas(textureID, nVer, &u, &v);
                        texcoords[tCount+0] = u; texcoords[tCount+1] = v; tCount += 2;
                    }
                    vCount += 4; iCount += 6;
                }
            }
		}
	}
	
	*vC = vCount;
	*iC = iCount;
	*tC = tCount;	
}

Model BuildModelFromMesh(int *vCount, int * iCount, int *tCount){
    if (*vCount == 0) return (Model){0};
	//MESH - MODEL
    Mesh ChunkMesh = {0};
    Mesh *ptrMesh = &ChunkMesh;
    ptrMesh -> vertexCount = *vCount;
    ptrMesh -> triangleCount = *iCount / 3;
    
	// Allocate only necessary memory for this psecific chunk mesh
	ptrMesh->vertices = (float*)malloc(*vCount * 3 * sizeof(float));
    ptrMesh->indices = (unsigned short*)malloc(*iCount * sizeof(unsigned short));
    ptrMesh->texcoords = (float*)malloc(*tCount * sizeof(float));
    
    // Copy form buffer -> allocated memory
    memcpy(ptrMesh->vertices, temp_vertici, *vCount * 3 * sizeof(float));
    memcpy(ptrMesh->indices, temp_indici, *iCount * sizeof(unsigned short));
    memcpy(ptrMesh->texcoords, temp_texcoords, *tCount * sizeof(float));
    UploadMesh(&ChunkMesh, false);
    
    //MODEL    
    return LoadModelFromMesh(ChunkMesh); 	
}

void BuildChunkMesh(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], 
												Chunk *c, int gX, int gZ){
	for(int l = 0; l < LAYER_COUNT; l++){
		int vC, iC, tC;
		BuildFaces(world, c, gX, gZ, &vC, &iC, &tC, l);
		c->layers[l] = BuildModelFromMesh(&vC, &iC, &tC);
	}
}

Model BuildModel(Texture2D tex, float size, float cutX, float cutY, float cutW, float cutH) {
    Mesh mesh = {0};
    mesh.vertexCount = 4;
    mesh.triangleCount = 2;
    mesh.vertices = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)malloc(mesh.triangleCount * 3 * sizeof(unsigned short));
    mesh.texcoords = (float*)malloc(mesh.vertexCount * 2 * sizeof(float));

    float hs = size / 2.0f;

	float v[12] = {
			-hs, 0.0f, -hs, // Alto-Sinistra
			-hs, 0.0f,  hs, // Basso-Sinistra
			 hs, 0.0f,  hs, // Basso-Destra
			 hs, 0.0f, -hs  // Alto-Destra
		};
    memcpy(mesh.vertices, v, sizeof(v));

    // Ordine per i due triangoli del quadrato
    unsigned short i[6] = { 0, 1, 2, 0, 2, 3 };
    memcpy(mesh.indices, i, sizeof(i));

    float imgW = (float)tex.width;
    float imgH = (float)tex.height;

    // Convertiamo i pixel (da 0.0f a 1.0f) richieste dal motore grafico
    float norm[8] = {
        cutX/imgW,       (cutY+cutH)/imgH, 
        cutX/imgW,       cutY/imgH,     
        (cutX+cutW)/imgW,   cutY/imgH,     
        (cutX+cutW)/imgW,   (cutY+cutH)/imgH  
    };
    memcpy(mesh.texcoords, norm, sizeof(norm));

    UploadMesh(&mesh, false);
    Model model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = tex;
    return model;
}

void UnloadChunkLayers(Chunk *c){
    for (int l = 0; l < LAYER_COUNT; l++){
        if (c->layers[l].meshCount > 0) UnloadModel(c->layers[l]);
        c->layers[l] = (Model){0};
    }
}

void SetChunkTexture(Chunk *c, Texture2D tex){
    for (int l = 0; l < LAYER_COUNT; l++){
        if (c->layers[l].meshCount > 0)
            c->layers[l].materials[0].maps[MATERIAL_MAP_ALBEDO].texture = tex;
    }
}

void UpdateChunkGraph(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int cx, int cz, Texture2D fnTerrain) {
    int wx = (cx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
    int wz = (cz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;

    if (world[wx][wz].gridX == cx && world[wx][wz].gridZ == cz) {
        UnloadChunkLayers(&world[wx][wz]);
        
        BuildChunkMesh(world, &world[wx][wz], cx, cz);
        SetChunkTexture(&world[wx][wz], fnTerrain);
    }
}

void BuildChunk(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], Vector3 playerPosition, int *lastChunkPlayerX, int *lastChunkPlayerZ, Texture2D fnTerrain){
	int chunkPlayerX = (int) floorf(playerPosition.x / CHUNK_SIZE); 
	int chunkPlayerZ = (int) floorf(playerPosition.z / CHUNK_SIZE); 
	
	if (chunkPlayerX != *lastChunkPlayerX || chunkPlayerZ != *lastChunkPlayerZ) {
		for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
			for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
				
				int gx = chunkPlayerX + dx;
				int gz = chunkPlayerZ + dz;
				
				int wx = (gx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
				int wz = (gz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
				
				// Se lo slot non contiene le coordinate corrette, significa che c'è
				// un vecchio chunk ormai lontano
				if (world[wx][wz].gridX != gx || world[wx][wz].gridZ != gz) {
					UnloadChunkLayers(&world[wx][wz]);
					world[wx][wz].needRemesh = false;
					
					dataQueueGX[dataTail] = gx;
					dataQueueGZ[dataTail] = gz;
					dataQueueWX[dataTail] = wx;
					dataQueueWZ[dataTail] = wz;
					dataTail = (dataTail + 1) % GEN_QUEUE_SIZE;
					
				}
			}
		}
		*lastChunkPlayerX = chunkPlayerX;
		*lastChunkPlayerZ = chunkPlayerZ;
	}
	
	/*
	 * I switch if -> while bc daa is fast and I wanna 
	*/
	int dataBudget = DATA_PER_FRAME;
	while (dataHead != dataTail && dataBudget-- > 0) {
		int wx = dataQueueWX[dataHead];
		int wz = dataQueueWZ[dataHead];
		int gx = dataQueueGX[dataHead];
		int gz = dataQueueGZ[dataHead];
		dataHead = (dataHead + 1) % GEN_QUEUE_SIZE;

		BuildChunkData(&world[wx][wz], gx, gz);
	
		genQueueGX[genTail] = gx;
		genQueueGZ[genTail] = gz;
		genQueueWX[genTail] = wx;
		genQueueWZ[genTail] = wz;
		genTail = (genTail + 1) % GEN_QUEUE_SIZE;
		
		/*
		 * nborX and nborZ are used for help me to find the 4 neighbor of an chunk
		 * ngx and ngz are the global X,Z coordinate of the neighbor 
		 * nwx and nwz are the local one
		 * nb the neighbor
		*/
		int nborX[4] = {1, -1, 0, 0};
		int nborZ[4] = {0, 0, 1, -1};
		for (int n = 0; n < 4; n++) {
			int ngx = gx + nborX[n];
			int ngz = gz + nborZ[n];
			int nwx = (ngx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
			int nwz = (ngz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
			Chunk *nb = &world[nwx][nwz];
			if (nb->gridX == ngx && nb->gridZ == ngz && nb->layers[LAYER_SOLID].meshCount > 0) {
				nb->needRemesh = true;
			}
		}
	}
	
	int meshBudget = MESH_PER_FRAME;
	if (genHead != genTail && meshBudget-- > 0) {
		int wx = genQueueWX[genHead];
		int wz = genQueueWZ[genHead];
		int gx = genQueueGX[genHead];
		int gz = genQueueGZ[genHead];
		genHead = (genHead + 1) % GEN_QUEUE_SIZE;

		BuildChunkMesh(world, &world[wx][wz], gx, gz);
		
		SetChunkTexture(&world[wx][wz], fnTerrain);
	}
			
	for (int wx = 0; wx < LOCAL_WORLD_SIZE && meshBudget > 0; wx++) {
		for (int wz = 0; wz < LOCAL_WORLD_SIZE && meshBudget > 0; wz++) {
			if (world[wx][wz].needRemesh) {
				UnloadChunkLayers(&world[wx][wz]);
				BuildChunkMesh(world, &world[wx][wz], world[wx][wz].gridX, world[wx][wz].gridZ);
				SetChunkTexture(&world[wx][wz], fnTerrain);
				world[wx][wz].needRemesh = false;
				meshBudget--;
			}
		}
	}
}

void InitWorldTime(Time *wt){
    wt->totTicks  = 0;
    wt->timeOfDay   = 0;
    wt->accumulator = 0.0f;
	wt->dailyPhase = 0;
}

void InizializeWorld(Game *game, int chunkPlayerX, int chunkPlayerZ, Texture2D fnTerrain){
	
	InitWorldTime(&game->time);
	game -> mode = 0;
	game -> opt_mode = 0;
	game -> isKeyF1 = 0;
	game -> isKeyF2 = 0;
	game -> isKeyF3 = 0;
	
	Chunk (*world)[LOCAL_WORLD_SIZE] = game->world;
	for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
        for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
            // Coordinate globali del chunk desiderato
            int gx = chunkPlayerX + dx;
            int gz = chunkPlayerZ + dz;
            int wx = (gx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
            int wz = (gz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
			BuildChunkData(&world[wx][wz], gx, gz);
        }
    }

	for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
        for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
            // Coordinate globali del chunk desiderato
            int gx = chunkPlayerX + dx;
            int gz = chunkPlayerZ + dz;
            int wx = (gx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
            int wz = (gz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
            
            BuildChunkMesh(world, &world[wx][wz], gx, gz);
            SetChunkTexture(&world[wx][wz], fnTerrain);
        }
    }
}

void DeleteBlockRay(Player *player, Game *game, Texture2D fnTerrain){
	player->lookDir = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
	player->ray = (Vector3){ player->position.x,
	                         player->position.y + PLAYER_EYE,
	                         player->position.z };
	
	for(float i = 0.0; i < MAX_RAY_DISTANCE; i+= STEP_RAY_SIZE){
		player->ray.x += player->lookDir.x * STEP_RAY_SIZE;
		player->ray.y += player->lookDir.y * STEP_RAY_SIZE;
		player->ray.z += player->lookDir.z * STEP_RAY_SIZE;
		
		int gx = (int)floorf(player->ray.x);
		int gy = (int)floorf(player->ray.y);
		int gz = (int)floorf(player->ray.z);

	 	char hit = GetBlockGlobal(game->world, gx, gy, gz);
                if(hit != AIR && hit != WATER){ // I hit it	
			int chunkX = (int)floorf((float)gx / CHUNK_SIZE);
			int chunkZ = (int)floorf((float)gz / CHUNK_SIZE);
			int wx = (chunkX % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
			int wz = (chunkZ % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;

			int lx = (gx % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
			int lz = (gz % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
			
			game->world[wx][wz].Map[lx][gy][lz] = 0;
			UpdateChunkGraph(game->world, chunkX, chunkZ, fnTerrain);
			if (lx == 0) { UpdateChunkGraph(game->world, chunkX - 1, chunkZ, fnTerrain);}
			else if (lx == CHUNK_SIZE - 1) { UpdateChunkGraph(game->world, chunkX + 1, chunkZ, fnTerrain);}
			if (lz == 0) { UpdateChunkGraph(game->world, chunkX, chunkZ - 1, fnTerrain);}
			else if (lz == CHUNK_SIZE - 1) { UpdateChunkGraph(game->world, chunkX, chunkZ + 1, fnTerrain);}
			
			break;
		}
	}
}

int isAroundPlayer(Player *player, int bx, int by, int bz){
	
    BoundingBox pb = player->playerBox;
    const float eps = 1e-4f;

    return (pb.min.x < bx + 1.0f - eps && pb.max.x > bx + eps) &&
           (pb.min.y < by + 1.0f - eps && pb.max.y > by + eps) &&
           (pb.min.z < bz + 1.0f - eps && pb.max.z > bz + eps);
}


void PlaceBlockRay(Player *player, Game *game, Texture2D fnTerrain){
	int block_selected = player->blocksInHand[(int)player->selectedSlotItemBar];
	player->lookDir = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
	player->ray = (Vector3){ player->position.x,
	                         player->position.y + PLAYER_EYE,
	                         player->position.z };
	
	int prec_gx = (int)floorf(player->ray.x);
	int prec_gy = (int)floorf(player->ray.y);
	int prec_gz = (int)floorf(player->ray.z);

	
	for(float i = 0.0; i < MAX_RAY_DISTANCE; i+= STEP_RAY_SIZE){
		player->ray.x += player->lookDir.x * STEP_RAY_SIZE;
		player->ray.y += player->lookDir.y * STEP_RAY_SIZE;
		player->ray.z += player->lookDir.z * STEP_RAY_SIZE;
		
		int gx = (int)floorf(player->ray.x);
		int gy = (int)floorf(player->ray.y);
		int gz = (int)floorf(player->ray.z);
		
		char hit = GetBlockGlobal(game->world, gx, gy, gz);
		if(hit != AIR && hit != WATER){ // I hit it			
			// check block validity 
			if(prec_gy < 0 || prec_gy >= CHUNK_HEIGTH) break;
			if(block_selected != WATER && isAroundPlayer(player, prec_gx, prec_gy, prec_gz)) break;
			
			int chunkX = (int)floorf((float)prec_gx / CHUNK_SIZE);
			int chunkZ = (int)floorf((float)prec_gz / CHUNK_SIZE);
			int wx = (chunkX % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
			int wz = (chunkZ % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;

			int lx = (prec_gx % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
			int lz = (prec_gz % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
			
			game->world[wx][wz].Map[lx][prec_gy][lz] = block_selected;
			UpdateChunkGraph(game->world, chunkX, chunkZ, fnTerrain);
			if (lx == 0) { UpdateChunkGraph(game->world, chunkX - 1, chunkZ, fnTerrain);}
			else if (lx == CHUNK_SIZE - 1) { UpdateChunkGraph(game->world, chunkX + 1, chunkZ, fnTerrain);}
			if (lz == 0) { UpdateChunkGraph(game->world, chunkX, chunkZ - 1, fnTerrain);}
			else if (lz == CHUNK_SIZE - 1) { UpdateChunkGraph(game->world, chunkX, chunkZ + 1, fnTerrain);}
			
			break;
		}
		prec_gx = gx;
		prec_gy = gy;
		prec_gz = gz;
	}
}

int IsInRange(Player *player, Game *game, int wx, int wz){
	if (game->world[wx][wz].layers[LAYER_SOLID].meshCount == 0) return 0;

	Vector3 chunkCenter = { game->world[wx][wz].position.x + (CHUNK_SIZE / 2.0f),
							player->camera.position.y,
							game->world[wx][wz].position.z + (CHUNK_SIZE / 2.0f)
						};
	Vector3 dirToChunk = Vector3Subtract(chunkCenter, player->camera.position);
	float dist = Vector3Length(dirToChunk);
	if(dist < (CHUNK_SIZE * 2.0f)) return 1;
	
	dirToChunk = Vector3Normalize(dirToChunk);

	Vector3 cameraDir = Vector3Normalize(
		Vector3Subtract(player->camera.target, player->camera.position)
		);
	
	if(Vector3DotProduct(cameraDir, dirToChunk) > -player->threshold_fovy) {return 1;}
	
	return 0;
}

Color GetSkyColor(int t){ // https://minecraft.wiki/w/Daylight_cycle#Daytime
	if(t < 11500) return SKY_DAY; 
	else if(t < 12500) return ColorLerp(SKY_DAY, SKY_SUNSET, (t - 11500) / 1000.0f);
	else if(t < 13500) return ColorLerp(SKY_SUNSET, SKY_NIGHT, (t - 12500) / 1000.0f);
	else if(t < 22500) return SKY_NIGHT;
	else if(t < 23500) return ColorLerp(SKY_NIGHT, SKY_SUNRISE, (t - 22500) / 1000.0f);
	else return ColorLerp(SKY_SUNRISE, SKY_DAY, (t - 23500) / 1000.0f);
}

bool BoxColliderWorld(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], BoundingBox *b){
	const float exp_bug = 1e-4f;
	int minX = (int)(floorf) (b->min.x + exp_bug);
	int maxX = (int)(floorf) (b->max.x - exp_bug);
	int minY = (int)(floorf) (b->min.y + exp_bug);
	int maxY = (int)(floorf) (b->max.y - exp_bug);
	int minZ = (int)(floorf) (b->min.z + exp_bug);
	int maxZ = (int)(floorf) (b->max.z - exp_bug);
	
    for(int x = minX; x <= maxX; x++){
        for(int y = minY; y <= maxY; y++){
            for(int z = minZ; z <= maxZ; z++){
                char id = GetBlockGlobal(world, x, y, z);
                if(id != AIR && id != WATER) return true;
            }
        }
    }
	
	return false;
}

void TryMoveAxis(Player *p, Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], Vector3 delta){
    Vector3 tryPos = Vector3Add(p->position, delta);
	BoundingBox b = PosToBox(&tryPos);
    if(!BoxColliderWorld(world, &b)){
        p->position = tryPos;
        return;
    }

    if(delta.y != 0.0f){
        if(delta.y < 0.0f) p->isOnGround = true;
        p->velocity.y = 0.0f;
    }
}

void UpdatePlayer(Game *game, Player *p, float dt){
	if(dt > 0.05f) dt = 0.05f;

	// MOUSE LOOK
	Vector2 md = GetMouseDelta();
	p->view.yaw   -= md.x * p->view.sensitivity;
	p->view.pitch -= md.y * p->view.sensitivity;
	if(p->view.pitch >  89.0f) p->view.pitch =  89.0f;
	if(p->view.pitch < -89.0f) p->view.pitch = -89.0f;

	Vector3 forward = Vector3Normalize((Vector3){
		cosf(DEG2RAD * p->view.pitch) * sinf(DEG2RAD * p->view.yaw),
		sinf(DEG2RAD * p->view.pitch),
		cosf(DEG2RAD * p->view.pitch) * cosf(DEG2RAD * p->view.yaw)
	});

	// direzione di camminata: orizzontale, senza Y
	Vector3 flat  = Vector3Normalize((Vector3){ forward.x, 0.0f, forward.z });
	Vector3 right = Vector3Normalize(Vector3CrossProduct(flat, (Vector3){0.0f, 1.0f, 0.0f}));

	// WASD -> direzione desiderata
	Vector3 moveDir = {0};
	if(IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, flat);
	if(IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, flat);
	if(IsKeyDown(KEY_D)) moveDir = Vector3Add(moveDir, right);
	if(IsKeyDown(KEY_A)) moveDir = Vector3Subtract(moveDir, right);
	if(Vector3Length(moveDir) > 0.0f) moveDir = Vector3Normalize(moveDir);

	float speed = IsKeyDown(KEY_LEFT_CONTROL) ? PLAYER_SPEED * 2.0f : PLAYER_SPEED;

	// SALTO E GRAVITA
	if(IsKeyPressed(KEY_F)) p->isFlying = !p->isFlying;

	float dy;
	if(p->isFlying){
		p->velocity.y = 0.0f;
		dy = 0.0f;
		if(IsKeyDown(KEY_SPACE))      dy +=  speed * dt;
		if(IsKeyDown(KEY_LEFT_SHIFT)) dy -=  speed * dt;
	} else {
		if(p->isOnGround && IsKeyDown(KEY_SPACE)) p->velocity.y = JUMP_SPEED;
		p->velocity.y -= GRAVITY * dt;
		dy = p->velocity.y * dt;
	}

	p->isOnGround = 0;
	TryMoveAxis(p, game->world, (Vector3){ moveDir.x * speed * dt, 0.0f, 0.0f });
	TryMoveAxis(p, game->world, (Vector3){ 0.0f, 0.0f, moveDir.z * speed * dt });
	TryMoveAxis(p, game->world, (Vector3){ 0.0f, dy, 0.0f });

	// SETTING
	p->playerBox = PosToBox(&p->position);
	p->isCollisioning = BoxColliderWorld(game->world, &p->playerBox);
	p->lookDir = forward;

	Vector3 eye = (Vector3){ p->position.x,
							 p->position.y + PLAYER_EYE,
							 p->position.z };

	if(p->changeThirdPerson == 1){
		p->camera.position = Vector3Subtract(eye, Vector3Scale(forward, 4.0f));
		p->camera.target   = eye;
	}else if(p->changeThirdPerson == 2){
		p->camera.position = Vector3Subtract(eye, Vector3Scale(forward, -5.0f));
		p->camera.target   = eye;
		
	}else {
		eye = Vector3Add(eye, Vector3Scale(flat, 3.0f * SKIN_PX));
		p->camera.position = eye;
		p->camera.target   = Vector3Add(eye, forward);
	}
	// SKIN
	p->skin.pos = (Vector3)p->position;
	
}

void Printplayer(Player *player, Game *game, char f){
	char char_dir[128];
	sprintf(char_dir, "View direction: %f / %f / %f\n", player->lookDir.x, player->lookDir.y, player->lookDir.z);
	DrawText(char_dir, 10, 50, FONT_SIZE, WHITE);
	
	char char_xp[128];
	sprintf(char_xp, "Level XP: %d / XP Bar: %.2f%%\n", player->xp, player->xpBar * 3.125);
	DrawText(char_xp, 10, 70, FONT_SIZE, WHITE);	
	
	char char_heal[128];
	sprintf(char_heal, "Heal: %d%%\n", player->heal * 5);
	DrawText(char_heal, 10, 90, FONT_SIZE, WHITE);	
	
	char char_hungry[128];
	sprintf(char_hungry, "Hungry: %d%%\n", player->hungry * 5);
	DrawText(char_hungry, 10, 110, FONT_SIZE, WHITE);
	
	char char_slotItemBar[128];
	sprintf(char_slotItemBar, "Slot Item Bar: %d\n", player->selectedSlotItemBar);
	DrawText(char_slotItemBar, 10, 130, FONT_SIZE, WHITE);

	char char_collision[128];
	sprintf(char_collision, "Collisioning: %s\n", player->isCollisioning ? "true" : "false");
	DrawText(char_collision, 10, 150, FONT_SIZE, WHITE);

	char char_fly[128];
	sprintf(char_fly, "Flying: %s\n", player->isFlying ? "true" : "false");
	DrawText(char_fly, 10, 170, FONT_SIZE, WHITE);
	
	char char_ground[128];
	sprintf(char_ground, "On Ground: %s\n", player->isOnGround ? "true" : "false");
	DrawText(char_ground, 10, 190, FONT_SIZE, WHITE);
	
	char char_biome[128];
	sprintf(char_biome, "Biome: %s\n", biomeName[player->biome]);
	DrawText(char_biome, 10, 210, FONT_SIZE, WHITE);

	// Global game's variables
	DrawText("Global game's variables:\n", WIDTH * 0.75, 30, FONT_SIZE, WHITE);
		char char_tick[1024];
		sprintf(char_tick, "Total Ticks: %lld / Phase: %.2f\n"
							"Day: %d / Dailytime: %d", 
							game->time.totTicks, (float)game->time.timeOfDay / (float)TICKS_PER_DAY,
							(int)game->time.totTicks / TICKS_PER_DAY, (int)game->time.totTicks % TICKS_PER_DAY); 
		DrawText(char_tick, WIDTH * 0.75, 50, FONT_SIZE, WHITE);
	
	// Temporary variables
	char char_tmp[1024];
	sprintf(char_tmp, "Tmp Var: In water = %d, Taking damage = %d", 
				player->isInWater, player->isTakingDamage);
	DrawText(char_tmp, WIDTH * 0.75, 90, FONT_SIZE, WHITE);
	
	// CLOCK
	char char_clock[16]; // 6 is enough, but I'm put 16 to avoid warnings
	int timeOfDay = game->time.timeOfDay;
	int hours = timeOfDay / 1000;
	int min_k = timeOfDay % 1000;
	int min = 60 * min_k / 1000;
	sprintf(char_clock, "%02d:%02d", hours, min);
	DrawText(char_clock, (WIDTH / 2) - MeasureText(char_clock, 40), 10, 40, WHITE);  
}

void UnloadSkin(Skin *skin){
	skin->head.model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = (Texture2D){0};
	skin->body.model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = (Texture2D){0};
	skin->armL.model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = (Texture2D){0};
	skin->armR.model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = (Texture2D){0};
	skin->legL.model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = (Texture2D){0};
	skin->legR.model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = (Texture2D){0};
	
	UnloadModel(skin->head.model);
	UnloadModel(skin->body.model);
	UnloadModel(skin->armL.model);
	UnloadModel(skin->armR.model);
	UnloadModel(skin->legL.model);
	UnloadModel(skin->legR.model);
}

void DrawLayer(Player *player, Game *game, int layer){
    for (int wx = 0; wx < LOCAL_WORLD_SIZE; wx++){
        for (int wz = 0; wz < LOCAL_WORLD_SIZE; wz++){
            if (IsInRange(player, game, wx, wz) && game->world[wx][wz].layers[layer].meshCount > 0)
                DrawModel(game->world[wx][wz].layers[layer], game->world[wx][wz].position, 1.0f, WHITE);
        }
    }
}

int main(){

/*
 * 	Texture2D fnTerrain = LoadTexture("texture/atlas/atlas_terrain.png");
 *	GenTextureMipmaps(&fnTerrain); // <--- AGGIUNGI QUESTA RIGA PER IL TRILINEAR
 *	SetTextureFilter(fnTerrain, TEXTURE_FILTER_TRILINEAR);
 * */

	double start_game = GetTime();
	
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "Sapphire"); 
    
    Texture2D fnTerrain = LoadTexture("texture/atlas/atlas_terrain.png");
	Texture2D gui = LoadTexture("texture/atlas/atlas_gui.png");
	Texture2D ascii = LoadTexture("texture/atlas/atlas_ascii.png");
	Texture2D cielo = LoadTexture("texture/atlas/atlas_celestials.png");
	Texture2D skinTex_TD = LoadTexture("texture/skin-player/adventure_guy.png");

	SetTextureFilter(fnTerrain, TEXTURE_FILTER_POINT);
	SetTextureFilter(gui, TEXTURE_FILTER_POINT);
	SetTextureFilter(ascii, TEXTURE_FILTER_POINT);
	SetTextureFilter(cielo, TEXTURE_FILTER_POINT);
	SetTextureFilter(skinTex_TD, TEXTURE_FILTER_POINT);

	Model sunModel = BuildModel(cielo, 400, 175, 47, 8, 8);
	Model moonModel = BuildModel(cielo, 400, 79, 13, 8, 8);
	
    // Initialize World, Texture Inventary, Player, Skin
	Player *player = (Player*)malloc(sizeof(Player));
	InitPlayer(player, skinTex_TD);

	int chunkPlayerX = (int)floorf(player->position.x / CHUNK_SIZE);
	int chunkPlayerZ = (int)floorf(player->position.z / CHUNK_SIZE);

    int lastChunkPlayerX = chunkPlayerX;
    int lastChunkPlayerZ = chunkPlayerZ;

	
	Game *game = (Game*)malloc(sizeof(Game));
	InizializeWorld(game, chunkPlayerX, chunkPlayerZ, fnTerrain);
	InitTextureInventary(fnTerrain, player);
		
    SetTargetFPS(540); 
    DisableCursor();
	char textCordinates[128];
    float dt = {0};
    while(!WindowShouldClose()){
		dt = GetFrameTime();

		// TIME
		UpgradeTime(&game->time, dt);
		
		// PLAYER CAMERA
        UpdatePlayer(game, player, dt);
		sprintf(textCordinates, "XYZ: %.2f / %.2f / %.2f | ground %d",
				player->position.x, player->position.y, player->position.z, player->isOnGround);

		BuildChunk(game->world, player->position, &lastChunkPlayerX, &lastChunkPlayerZ, fnTerrain);

		// Sarebbe da fare i Command check
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
			DeleteBlockRay(player, game, fnTerrain);
		}
		if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
			PlaceBlockRay(player, game, fnTerrain);
		}
		if(IsKeyPressed(KEY_F3)){ game->isKeyF3 = !(game->isKeyF3); }
		if(IsKeyPressed(KEY_F2)){ game->isKeyF2 = !(game->isKeyF2); }
		if(IsKeyPressed(KEY_F1)){ game->isKeyF1 = !(game->isKeyF1); }
		if(IsKeyPressed(KEY_F5)){player->changeThirdPerson = (player->changeThirdPerson + 1) % 3;}
		

		//if(IsKeyDown(KEY_G)) TryMoveAxis(player, game->world, (Vector3){0, -2.0f * dt, 0});
		if(IsKeyDown(KEY_G)){ 
			DrawText("G PREMUTO", 10, 200, FONT_SIZE, RED);
			TryMoveAxis(player, game->world, (Vector3){0, -2.0f * dt, 0});
		}

        BeginDrawing();
            ClearBackground(GetSkyColor(game->time.timeOfDay));
			
            BeginMode3D(player->camera);
            	// Draw Terrain
            	DrawLayer(player, game, LAYER_SOLID);   
				
				//DrawSkin(&player->skin, player->view.yaw);				
				DrawSun(&player->camera.position, &game->time, sunModel, moonModel);			
				if(game->isKeyF3)
				{ 
					DrawBoundingBox(player->playerBox, RED);
				}
				if(!game->isKeyF1)
				{
					DrawSkin(&player->skin, player->view.yaw);				
				}
				
				// Draw LEAF & Water
				rlDisableBackfaceCulling();					
					DrawLayer(player, game, LAYER_CUTOUT);
					DrawLayer(player, game, LAYER_WATER);
				rlEnableBackfaceCulling();
			EndMode3D();
			
			if(!game->isKeyF3)
			{
				Printplayer(player, game, 0); 
			}
			if(!game->isKeyF1)
			{
				DrawGUI(gui, ascii, player); 
			}
			DrawFPS(10, 10);
			DrawText(textCordinates, 10, 30, FONT_SIZE, WHITE);
			UpdatePlayerStats(player);
			
        EndDrawing();
    }
	
	// FINAL PRINT
	double end_game = GetTime();
	float time_played = (float) (end_game - start_game);
	printf("Time Played: %.4fs\n", time_played);
	
	// UNLOAD FEATURES	
	UnloadSkin(&player->skin);
	
	for (int wx = 0; wx < LOCAL_WORLD_SIZE; wx++)
		for (int wz = 0; wz < LOCAL_WORLD_SIZE; wz++)
			UnloadChunkLayers(&game->world[wx][wz]);

	for(int i = 0; i < MAX_BLOCK_TYPES; i++)
		UnloadRenderTexture(player->blockIcons[i]);
	
    UnloadTexture(fnTerrain);
	UnloadTexture(gui);
	UnloadTexture(ascii);
	UnloadTexture(cielo);
	UnloadTexture(skinTex_TD);
	UnloadModel(sunModel);
	UnloadModel(moonModel);
	
    CloseWindow(); 
	
	free(player);
	free(game);
    return 0;
}
