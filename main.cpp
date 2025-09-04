#include<iostream>
#include<Windows.h>
#include<chrono>
#include<vector>
#include<utility>
#include<algorithm>

using namespace std;

int nScreenWidth = 120;		//Console screen x(cols)
int nScreenHeight = 40;		//Console screen y(rows)

float fPlayerX = 8.0f;		//X position of player
float fPlayerY = 8.0f;		//Y position of player
float fPlayerA = 0.0f;		//Angle at which player is looking

int nMapHeight = 16;		//Map array
int nMapWidth = 16;

float fFOV = 3.14159 / 4.0;
float fDepth = 16.0f;

int main() {
	
	//Create Screen Buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#...........#..#";
	map += L"#######........#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"####...........#";
	map += L"#..............#";
	map += L"#.........######";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	//Game Loop
	while (1) {

		//FPS like system
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		//Controls
		//Handle rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000){
			fPlayerA -= 0.8f * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000){
			fPlayerA += 0.8f * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
			//Collision detection
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerA) * 0.5f * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * 0.5f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; x++) {
			//For each column, calculate projected ray angle into space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
			
			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundary = false;

			float fEyeX = sinf(fRayAngle);		//Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth) {
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				//Test if ray is out of bound
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					bHitWall = true;		//set the distance to max depth
					fDistanceToWall = fDepth;
				}
				else {
					//If ray is inbound then test if the raycell is a wall block
					if (map[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true;

						vector<pair<float, float>> p;
						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);		//Calculate how far is the corner
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d); //dot product between unit vector
								p.push_back(make_pair(d, dot));
							}
						}
						//Sort from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first;});
						
						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						//if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}

			//Calculate distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nFloorShade = ' ';
			short nWallShade = ' ';	

			if (fDistanceToWall <= fDepth / 4.0f) {
				nWallShade = 0x2588; //Very Close
			}
			else if (fDistanceToWall < fDepth / 3.0f) {
				nWallShade = 0x2593;
			}
			else if (fDistanceToWall < fDepth / 2.0f) {
				nWallShade = 0x2592;
			}
			else if(fDistanceToWall < fDepth){
				nWallShade = 0x2591;
			}
			else {
				nWallShade = ' '; //Far Away
			}

			if (bBoundary) { nWallShade = ' '; }

			for (int y = 0; y < nScreenHeight; y++) {
				if (y < nCeiling) {
					screen[y * nScreenWidth + x] = ' ';
				}
				else if(y > nCeiling && y<=nFloor) {
					screen[y * nScreenWidth + x] = nWallShade;
				}
				else {
					//Shade floor
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)		nFloorShade = '#';
					else if (b < 0.5)	nFloorShade = 'x';
					else if (b < 0.75)	nFloorShade = '.';
					else if (b < 0.9)	nFloorShade = '-';
					else				nFloorShade = ' ';
					screen[y * nScreenWidth + x] = nFloorShade;
				}
			}
		}

		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

		// Display Map
		for (int nx = 0; nx < nMapWidth; nx++)
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}
	

	return 0;
}
