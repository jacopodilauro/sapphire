/*
 
 [x] prossimo punto da fare è la gravità, come avevo fatto tempo fa
 [] inserisco poi le colisioni 3D
 [] inseirsco il salto
 [] inseriamo il volo
 *[] inseriamo il piazzamento e togliemento blocchi
 [] inseriamo il sole, alberi, montagne
 [] inseriamo la terza persona e prima persona. 

*/

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <raymath.h>

#define WIDTH 1800
#define HEIGHT 1200

#define GRAVITY 0

#define CHUNK_SIZE 16
#define CHUNK_HEIGTH 128

#define HEIGHT_GROUND 80

#define LOCAL_WORLD_SIZE 9 // == # Chunks attorno al player
#define MAX_WORLD_SIZE 1024  // == # Chunks massimi di tutto il mondo

#define CHUNK_RADIUS (LOCAL_WORLD_SIZE / 2)

#define MAX_RAY_DISTANCE 6
#define STEP_RAY_SIZE 0.05

#define NINE_BLOCKS_SIZE 9

#define GEN_QUEUE_SIZE 64

// Generation Queue
int genQueueGX[GEN_QUEUE_SIZE];   
int genQueueGZ[GEN_QUEUE_SIZE];   
int genQueueWX[GEN_QUEUE_SIZE];   
int genQueueWZ[GEN_QUEUE_SIZE];
int genHead = 0;
int genTail = 0;

void DrawGUI(Texture2D gui){
	
	Rectangle rec = {617.0f, 480.0f, 9.0f, 9.0f};
	Rectangle posGUI = {WIDTH/2, HEIGHT/2, 20.0f, 20.0f};
	DrawTexturePro(gui, rec, posGUI, (Vector2) {10, 10}, 0.0f, WHITE);
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

typedef struct Chunk{
    int gridX;
    int gridZ;
    char Map[CHUNK_SIZE][CHUNK_HEIGTH][CHUNK_SIZE];
    Model model;
    Vector3 position;
}Chunk; 

char GetBlockGlobal(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int gx, int gy, int gz){
    if(gy < 0 || gy >= CHUNK_HEIGTH) return 0;
	
	int chunkX = (int) floor((float)gx / CHUNK_SIZE);
	int chunkZ = (int) floor((float)gz / CHUNK_SIZE);
	
	int wx = (chunkX % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
    int wz = (chunkZ % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
	
	if (world[wx][wz].gridX != chunkX || world[wx][wz].gridZ != chunkZ) {
        return 0; 
    }
	
	int lx = (gx % CHUNK_SIZE  + CHUNK_SIZE) % CHUNK_SIZE;
    int lz = (gz % CHUNK_SIZE  + CHUNK_SIZE) % CHUNK_SIZE;
	
	return world[wx][wz].Map[lx][gy][lz];
}

void BuildChunkData(Chunk *c, int gX, int gZ){
	c->gridX = gX;
	c->gridZ = gZ;
	c->position = (Vector3){gX * CHUNK_SIZE, 0.0f, gZ * CHUNK_SIZE};
	
	int octaves = 6;
	float persistence = 0.5f;
	float lacunarity = 2.0f;
	float scale = 100.0f;
	
	memset(c->Map, 0, sizeof(c->Map));
	//MAP / floor
    for(int x = 0; x < CHUNK_SIZE; x++){
        for(int z = 0; z < CHUNK_SIZE; z++){
        	float globalX = (gX * CHUNK_SIZE) + x;
        	float globalZ = (gZ * CHUNK_SIZE) + z;
            //float noise = PerlinNoise(globalX * 0.09f, 0.0f, globalZ * 0.09f);
			float noise = FractalBrownianMotion(globalX, globalZ, octaves, persistence, lacunarity, scale);
            float noiseNorm = pow((noise + 1.0f) / 2.0f, 3.0f);
            int heightGround = (int)(noiseNorm * HEIGHT_GROUND) + 2;
            for(int y = 0; y < heightGround; y++){
                if (heightGround == 4) {
                    c->Map[x][y][z] = 1; 
                }else if (heightGround < 4){
					c->Map[x][y][z] = 5; // Acqua	
				}
				if (y == heightGround - 1) {
					c->Map[x][y][z] = 3; // Erba
				} else if (y > heightGround - 4) {
					c->Map[x][y][z] = 2; // Terra
				} else{
					c->Map[x][y][z] = 4; // Roccia
				}
            }
        }
    }
}

void BuildChunkMesh(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], Chunk *c, int gX, int gZ){
    //Algoritmo Mesh optimizated, I use only visible faces
    int MAX_FACES = CHUNK_SIZE * CHUNK_HEIGTH * CHUNK_SIZE * 6;
    float *vertici = (float*)malloc(MAX_FACES * 4 * 3 * sizeof(float));
    unsigned short *indici = (unsigned short*)malloc(MAX_FACES * 6 * sizeof(unsigned short));
    float *texcoords = (float*)malloc(MAX_FACES * 4 * 2 * sizeof(float));
    
    int vCount = 0;
    int iCount = 0;
    int tCount = 0;

    for(int x = 0; x < CHUNK_SIZE; x++){
        for(int y = 0; y < CHUNK_HEIGTH; y++){
            for(int z = 0; z < CHUNK_SIZE; z++){  
                
				if(c->Map[x][y][z] == 0) continue;
                
				int globalX = (gX * CHUNK_SIZE) + x;
				int globalZ = (gZ * CHUNK_SIZE) + z;
				
				
				
                int textureID = 0;
                if(c->Map[x][y][z] == 1){ textureID = 18; } //sabbia
                else if(c->Map[x][y][z] == 2){ textureID = 2; } // terra
                else if(c->Map[x][y][z] == 3){ textureID = 3; } // erba
                else if(c->Map[x][y][z] == 4){ textureID = 1; } // roccia
				else if(c->Map[x][y][z] == 5){ textureID = 207; } // acqua
                
				//Upper Face
                if(GetBlockGlobal(world, globalX, y+1, globalZ) == 0){
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
                if(y == 0 || GetBlockGlobal(world, globalX, y-1, globalZ) == 0){
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
                if(GetBlockGlobal(world, globalX + 1, y, globalZ) == 0){
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
                if(GetBlockGlobal(world, globalX-1, y, globalZ) == 0){
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
                if(GetBlockGlobal(world, globalX, y, globalZ+1) == 0){
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
                if(GetBlockGlobal(world, globalX, y, globalZ-1) == 0){
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
	//MESH - MODEL
    Mesh ChunkMesh = {0};
    Mesh *ptrMesh = &ChunkMesh;
    ptrMesh -> vertexCount = vCount;
    ptrMesh -> triangleCount = iCount / 3;
    
    ptrMesh -> vertices = (float*)realloc(vertici, vCount * 3 * sizeof(float));
    ptrMesh -> indices = (unsigned short*)realloc(indici, iCount * sizeof(unsigned short));
    ptrMesh -> texcoords = (float*)realloc(texcoords, tCount * sizeof(float));
    UploadMesh(&ChunkMesh, false);
    
    //MODEL    
    c -> model = LoadModelFromMesh(ChunkMesh);               
}

void BuildChunk(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], Vector3 playerPosition, int *lastChunkPlayerX, int *lastChunkPlayerZ, Texture2D fnTerrain){
		int chunkPlayerX = (int) floorf(playerPosition.x / CHUNK_SIZE); 
		int chunkPlayerZ = (int) floorf(playerPosition.z / CHUNK_SIZE); 
        
        if (chunkPlayerX != *lastChunkPlayerX || chunkPlayerZ != *lastChunkPlayerZ) {
            for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
                for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
                    
                    int gx = chunkPlayerX + dx;
                    int gz = chunkPlayerZ + dz;
                    
                    // Calcoliamo in quale slot dell'array DOVREBBE trovarsi questo chunk
                    int wx = (gx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
                    int wz = (gz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
                    
                    // Se lo slot non contiene le coordinate corrette, significa che c'è
                    // un vecchio chunk ormai lontano. Lo sovrascriviamo!
                    if (world[wx][wz].gridX != gx || world[wx][wz].gridZ != gz) {
                        if (world[wx][wz].model.meshCount > 0) UnloadModel(world[wx][wz].model);
                        world[wx][wz].model = (Model){0};
                        
                        // Generiamo i DATI all'istante, fuori dalla coda, così i chunk vicini li vedono subito!
						BuildChunkData(&world[wx][wz], gx, gz);

                        // Mettiamo in coda la generazione
                        genQueueGX[genTail] = gx;
                        genQueueGZ[genTail] = gz;
                        genQueueWX[genTail] = wx;
                        genQueueWZ[genTail] = wz;
                        genTail = (genTail + 1) % GEN_QUEUE_SIZE;
                    }
                }
            }
            *lastChunkPlayerX = chunkPlayerX;
            *lastChunkPlayerZ = chunkPlayerZ;
        }
		
		if (genHead != genTail) {
 			int wx = genQueueWX[genHead];
    		int wz = genQueueWZ[genHead];
    		int gx = genQueueGX[genHead];
    		int gz = genQueueGZ[genHead];
    		genHead = (genHead + 1) % GEN_QUEUE_SIZE;
    		BuildChunkMesh(world, &world[wx][wz], gx, gz);
    		world[wx][wz].model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = fnTerrain;
		}
}

void UpdateChunkGraph(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int cx, int cz, Texture2D fnTerrain) {
    int wx = (cx % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
    int wz = (cz % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;

    if (world[wx][wz].gridX == cx && world[wx][wz].gridZ == cz) {
        if (world[wx][wz].model.meshCount > 0) {
            UnloadModel(world[wx][wz].model);
        }
        BuildChunkMesh(world, &world[wx][wz], cx, cz);
        world[wx][wz].model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = fnTerrain;
    }
}

void InizializeWorld(Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE], int chunkPlayerX, int chunkPlayerZ, Texture2D fnTerrain){
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
            world[wx][wz].model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = fnTerrain;
        }
    }
}

int main(){
    
    InitWindow(WIDTH, HEIGHT, "Minecraft"); 
    Texture2D fnTerrain = LoadTexture("terrain.png");
    SetTextureFilter(fnTerrain, TEXTURE_FILTER_POINT);
	Texture2D gui = LoadTexture("gui.png");
    
    //CAMERA
    Camera3D camera = {10.0f, 40.0f, -10.0f, 
                       9.0f, 3.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       60.0f,
                       CAMERA_PERSPECTIVE};
    
	//RAY COLLISION
	Vector3 dist = Vector3Subtract(camera.target, camera.position);
	Vector3 lookDir = Vector3Normalize(dist);
	Vector3 ray = camera.position;
	
    //PLAYER
    static BoundingBox BoxPlayer;
    static BoundingBox BoxCollisionPlayer;
    
    int chunkPlayerX = (int) floorf(camera.position.x / CHUNK_SIZE); 
    int chunkPlayerZ = (int) floorf(camera.position.z / CHUNK_SIZE); 

    int lastChunkPlayerX = chunkPlayerX;
    int lastChunkPlayerZ = chunkPlayerZ;
    
    // Build Initialize World 
    static Chunk world[LOCAL_WORLD_SIZE][LOCAL_WORLD_SIZE];
    InizializeWorld(world, chunkPlayerX, chunkPlayerZ, fnTerrain);

    SetTargetFPS(240); 
    DisableCursor();
	SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
    float dt; Vector3 velocityPlayer = {0};
    while(!WindowShouldClose())
    {
		//GRAVITY
		dt = GetFrameTime();
		velocityPlayer.y -= GRAVITY * dt;
		camera.position.y += velocityPlayer.y * dt;
		//CAMERA
        UpdateCamera(&camera, CAMERA_FREE);
        BoxPlayer.min =  (Vector3){ camera.position.x - 0.5f, camera.position.y - 2.0f, camera.position.z - 0.5f };
		BoxPlayer.max =  (Vector3){ camera.position.x + 0.5f, camera.position.y - 0.15f, camera.position.z + 0.5f };
		
		BuildChunk(world, camera.position, &lastChunkPlayerX, &lastChunkPlayerZ, fnTerrain);

		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
			dist = Vector3Subtract(camera.target, camera.position);
			lookDir = Vector3Normalize(dist);
			ray = camera.position;
			
			for(float i = 0.0; i < MAX_RAY_DISTANCE; i+= STEP_RAY_SIZE){
				ray.x += lookDir.x * STEP_RAY_SIZE;
				ray.y += lookDir.y * STEP_RAY_SIZE;
				ray.z += lookDir.z * STEP_RAY_SIZE;
				
				int gx = (int)floorf(ray.x);
				int gy = (int)floorf(ray.y);
				int gz = (int)floorf(ray.z);
				
				if(GetBlockGlobal(world, gx, gy, gz) != 0){ // I hit it
					int chunkX = (int)floorf((float)gx / CHUNK_SIZE);
					int chunkZ = (int)floorf((float)gz / CHUNK_SIZE);
					int wx = (chunkX % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
					int wz = (chunkZ % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;

					int lx = (gx % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
					int lz = (gz % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
					
					world[wx][wz].Map[lx][gy][lz] = 0;
					UpdateChunkGraph(world, chunkX, chunkZ, fnTerrain);
					if (lx == 0) { UpdateChunkGraph(world, chunkX - 1, chunkZ, fnTerrain);}
					else if (lx == CHUNK_SIZE - 1) { UpdateChunkGraph(world, chunkX + 1, chunkZ, fnTerrain);}
					if (lz == 0) { UpdateChunkGraph(world, chunkX, chunkZ - 1, fnTerrain);}
					else if (lz == CHUNK_SIZE - 1) { UpdateChunkGraph(world, chunkX, chunkZ + 1, fnTerrain);}
					
					break;
				}
			}
		}
		if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
			dist = Vector3Subtract(camera.target, camera.position);
			lookDir = Vector3Normalize(dist);
			ray = camera.position;
			
			int prec_gx = (int)floorf(ray.x);
			int prec_gy = (int)floorf(ray.y);
			int prec_gz = (int)floorf(ray.z);
			
			for(float i = 0.0; i < MAX_RAY_DISTANCE; i+= STEP_RAY_SIZE){
				ray.x += lookDir.x * STEP_RAY_SIZE;
				ray.y += lookDir.y * STEP_RAY_SIZE;
				ray.z += lookDir.z * STEP_RAY_SIZE;
				
				int gx = (int)floorf(ray.x);
				int gy = (int)floorf(ray.y);
				int gz = (int)floorf(ray.z);
				
				if(GetBlockGlobal(world, gx, gy, gz) != 0){ // I hit it
					int chunkX = (int)floorf((float)prec_gx / CHUNK_SIZE);
					int chunkZ = (int)floorf((float)prec_gz / CHUNK_SIZE);
					int wx = (chunkX % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;
					int wz = (chunkZ % LOCAL_WORLD_SIZE + LOCAL_WORLD_SIZE) % LOCAL_WORLD_SIZE;

					int lx = (prec_gx % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
					int lz = (prec_gz % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
					
					world[wx][wz].Map[lx][prec_gy][lz] = 1;
					UpdateChunkGraph(world, chunkX, chunkZ, fnTerrain);
					if (lx == 0) { UpdateChunkGraph(world, chunkX - 1, chunkZ, fnTerrain);}
					else if (lx == CHUNK_SIZE - 1) { UpdateChunkGraph(world, chunkX + 1, chunkZ, fnTerrain);}
					if (lz == 0) { UpdateChunkGraph(world, chunkX, chunkZ - 1, fnTerrain);}
					else if (lz == CHUNK_SIZE - 1) { UpdateChunkGraph(world, chunkX, chunkZ + 1, fnTerrain);}
					
					break;
				}
				prec_gx = gx;
				prec_gy = gy;
				prec_gz = gz;
			}
		}

        BeginDrawing();
            ClearBackground(SKYBLUE);
            DrawFPS(10, 10);
			
            BeginMode3D(camera);
              	//DrawBoundingBox(BoxPlayer, RED);
            	for (int wx = 0; wx < LOCAL_WORLD_SIZE; wx++) {
        		    for (int wz = 0; wz < LOCAL_WORLD_SIZE; wz++) {
            			DrawModel(world[wx][wz].model, world[wx][wz].position, 1.0f, WHITE);
        			}
    			}   
            EndMode3D();
			
			DrawGUI(gui);
			
			
        EndDrawing();
    }
    for (int wx = 0; wx < LOCAL_WORLD_SIZE; wx++) {
        for (int wz = 0; wz < LOCAL_WORLD_SIZE; wz++) {
      		UnloadModel(world[wx][wz].model);
     	}
    }  
    UnloadTexture(fnTerrain);
    CloseWindow(); 

    return 0;
}
