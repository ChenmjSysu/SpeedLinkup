#include "CImg.h"
#include "Block.h"
#include "Common.h"
#include <vector>
#include <map>

using namespace cimg_library;

class GameWindow{
public:
	GameWindow(CImg<byte> src, std::vector<Point> vertex): Img(src), Vertex(vertex){
		GameArea = Img.get_crop(vertex[1].x, vertex[1].y, vertex[3].x, vertex[3].y);
		MetrixWidth = 19;
	    MetrixHeight = 11;
		//GameArea.display("gg");
	};
	void Split();
	int IsDuplicatedBlock(CImg<byte> newBlock);
	std::vector<Point> MatchBlock();
	bool IsNullBlock(Point p);

	bool IsEliminable(Point a, Point b);
	bool StraightEliminable(Point a, Point b);
	bool OneCornerEliminable(Point a, Point b);
	bool TwoCornersEliminable(Point a, Point b);

	void StartGame();
	void Eliminate(Point a);
	bool IsFinished();
	int GetRestCount();

	void Draw();
private:
	CImg<byte> Img;
	std::vector<Point> Vertex;
	CImg<byte> GameArea;
	CImgList<byte> BlocksList;
	Block** BlockMetrix;
	std::map<int, std::vector<Point>> Label2Point;
	int NullLabel;

	int MetrixWidth;
	int MetrixHeight;
};