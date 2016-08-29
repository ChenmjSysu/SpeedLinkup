#include <string>
#include "CImg.h"
#include "GetGameWindows.h"
#include "GameWindow.h"
#include <stdlib.h>

using std::string;
using namespace cimg_library;

int main(int argc, char* argv[]) {
	string pythonScreenShotFilePath = "E:\\code\\screenshot.py";
	string screenShotFilePath = "E:\\code\\SpeedLinkup\\ScreenShoot\\test.bmp";
	char* screenShotCmd = new char[100];
	sprintf(screenShotCmd, "python %s %s", pythonScreenShotFilePath.c_str(), screenShotFilePath.c_str());
	
	getchar();
	Sleep(2000);
	printf("Capture Screen\n");	
	system(screenShotCmd);
	CImg<byte> src;
	src.assign(screenShotFilePath.c_str());
	std::vector<Point> vertex = GetGameWindows(src);

	GameWindow gameWindow(src, vertex);
 	gameWindow.Split();
	//gameWindow.Draw();

	gameWindow.StartGame();

	return 0;
}