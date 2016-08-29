#include "GameWindow.h"
#include <set>

#define maximum(x, y) x > y ? x : y
#define minimum(x, y) x < y ? x : y

string PythonMouseFilePath = "E:\\code\\mouse.py";

void GameWindow::Split() {
	BlockMetrix = new Block*[MetrixHeight];
	for(int i = 0; i < 11; i++) {
		BlockMetrix[i] = new Block[MetrixWidth];
	}

	Point leftTop(this->Vertex[1].x + 14, this->Vertex[1].y + 180);
	Point rightBottom(this->Vertex[1].x + 45, this->Vertex[1].y + 215);

	int startX = leftTop.x;
	int startY = leftTop.y;
	int sizeX = rightBottom.x - leftTop.x;
	int sizeY = rightBottom.y - leftTop.y;
	
	char* savePath = new char[100];
	int tempNullLabel = 0;
	int tempMaxLabelCount = 0;
	for(int i = 0; i < MetrixWidth; i++) {
		for(int j = 0; j < MetrixHeight; j++) {
			int tempX = startX + sizeX * i;
			int tempY = startY + sizeY * j;
			CImg<byte> part = this->Img.get_crop(tempX, tempY, tempX + sizeX, tempY + sizeY);
			int blockLabel = IsDuplicatedBlock(part);
			if (blockLabel == -1) {
				blockLabel = this->BlocksList.size();
				this->BlocksList.insert(part);
				this->Label2Point.insert ( std::pair<int, std::vector<Point>>(blockLabel, std::vector<Point>()) );
			}		
			this->Label2Point[blockLabel].push_back(Point(j, i));
			if(this->Label2Point[blockLabel].size() > tempMaxLabelCount) {
				tempMaxLabelCount = this->Label2Point[blockLabel].size();
				tempNullLabel = blockLabel;
			}
			this->BlockMetrix[j][i] = Block(blockLabel, Point(j, i), Point(tempX + 10, tempY + 10));
			
			
			sprintf(savePath, "E:\\code\\SpeedLinkup\\ScreenShoot\\parts\\%d_%d.bmp", j, i); 
			part.save(savePath);
		}
	}
	
	//this->BlocksList.save("E:\\code\\SpeedLinkup\\ScreenShoot\\list\\list.bmp");
	this->NullLabel = tempNullLabel;
	delete[] savePath;
}

int GameWindow::IsDuplicatedBlock(CImg<byte> newBlock) {
	if (this->BlocksList.size() == 0) {
		return -1;
	}
	int index = 0;
	for (CImgList<byte>::iterator it = this->BlocksList.begin(); it < this->BlocksList.end(); ++it) {
		CImg<byte> diff = newBlock.get_crop(5, 5, 27, 27) - (*it).get_crop(5, 5, 27, 27);	
		/*newBlock.get_crop(5, 5, 27, 27).save("E:\\code\\SpeedLinkup\\ScreenShoot\\new.bmp");
		(*it).get_crop(5, 5, 27, 27).save("E:\\code\\SpeedLinkup\\ScreenShoot\\it.bmp");*/
		//diff.display("diff");
		double sum = diff.sum();
		if (sum < 15000) {
			return index;
		}
		index += 1;
	}
	return -1;
}

std::vector<Point> GameWindow::MatchBlock() {
	CImg<byte> tempImg = this->Img;
	std::vector<Point> matchedPoints;
	for(int i = 0; i < this->MetrixHeight; i++) {
		for(int j = 0; j < this->MetrixWidth; j++) {
			Block block = this->BlockMetrix[i][j];
			int label = block.Label;
			//ignore null label
			if(label == this->NullLabel) continue;
			std::vector<Point> blocksWithLabel = this->Label2Point[label];
			std::vector<Point>::iterator it = blocksWithLabel.begin();
			for(; it != blocksWithLabel.end(); it++) {
				//ignore the block on the left or the same block		
				if((*it).y < block.Position.y || ((*it).x == block.Position.x && (*it).y == block.Position.y)) continue;

				if(IsEliminable(block.Position, (*it))) {
					printf("Match %s %s\t", block.Position.ToString().c_str(), (*it).ToString().c_str());
					printf("Eliminable\tRemain %d\n", GetRestCount());
					Eliminate(block.Position);
					Eliminate((*it));

					Point click1 = block.ClickPosition;
					Point click2 = this->BlockMetrix[(*it).x][(*it).y].ClickPosition;
					char* cmd = new char[100];
					sprintf(cmd, "python %s %d %d", PythonMouseFilePath.c_str(), click1.x, click1.y);
					system(cmd);
					sprintf(cmd, "python %s %d %d", PythonMouseFilePath.c_str(), click2.x, click2.y);
					system(cmd);
					
					const unsigned char color[] = { 255,32,164 };

					tempImg.draw_rectangle(click1.x - 10, click1.y - 10, click1.x + 20, click1.y + 20, color, 1.0f);
					tempImg.draw_rectangle(click2.x - 10, click2.y - 10, click2.x + 20, click2.y + 20, color, 1.0f);				
					//tempImg.display("Eliminated");

					break;
				}
				else {
					//printf("Not Eliminable\n");
				}
			}
		}
	}
	return matchedPoints;
}

bool GameWindow::IsNullBlock(Point p) {
	return this->BlockMetrix[p.x][p.y].Label == this->NullLabel;
}

bool GameWindow::IsEliminable(Point a, Point b) {
	//Point leftTop = Point(minimum(a.x, b.x), minimum(a.y, b.y));
	//Point rightBottom = Point(maximum(a.x, b.x), maximum(a.y, b.y));

	return StraightEliminable(a, b) || OneCornerEliminable(a, b) || TwoCornersEliminable(a, b);
	
}

bool GameWindow::StraightEliminable(Point a, Point b) {
	bool flag = true;	
	if(a.x != b.x && a.y != b.y) flag = false;
	if(a.x == b.x) {
		int stepOrientation = (b.y - a.y) / abs(b.y - a.y);
		int tempY = a.y + stepOrientation;
		while(tempY != b.y) {		
			if(this->BlockMetrix[a.x][tempY].Label != this->NullLabel) {
				flag = false;
				break;
			}
			tempY += stepOrientation;
		}
	}
	else if (a.y == b.y) {
		int stepOrientation = (b.x - a.x) / abs(b.x - a.x);
		int tempX = a.x + stepOrientation;
		while(tempX != b.x) {			
			if(this->BlockMetrix[tempX][a.y].Label != this->NullLabel) {
				flag = false;
				break;
			}
			tempX += stepOrientation;
		}
	}


	return flag;
}

bool GameWindow::OneCornerEliminable(Point a, Point b) {
	bool flag = true;
	if(a.x == b.x || a.y == b.y) return false;
	Point corner1 = Point(a.x, b.y);
	Point corner2 = Point(b.x, a.y);
	
	if(this->BlockMetrix[corner1.x][corner1.y].Label != this->NullLabel 
		|| this->BlockMetrix[corner2.x][corner2.y].Label != this->NullLabel)
		return false;

	flag = (StraightEliminable(a, corner1) && StraightEliminable(b, corner1))
		|| (StraightEliminable(a, corner2) && StraightEliminable(b, corner2));

	return flag;
}

bool GameWindow::TwoCornersEliminable(Point a, Point b) {
	std::set<int> setLeftTop, setRightBottom;
	//horizontal first
	Point left, right;
	if(a.y < b.y) {
		left = a;
		right = b;
	}
	else {
		left = b;
		right = a;
	}
	//find max null block list for left
	for(int tempY = left.y + 1; tempY < this->MetrixWidth; tempY++) {
		if(IsNullBlock(Point(left.x, tempY))) {
			setLeftTop.insert(tempY);
		}
		else {
			break;
		}
	}
	for(int tempY = left.y -1 ; tempY >=0; tempY--) {
		if(IsNullBlock(Point(left.x, tempY))) {
			setLeftTop.insert(tempY);
		}
		else {
			break;
		}
	}
	//find max null block list for right
	for(int tempY = right.y + 1; tempY < this->MetrixWidth; tempY++) {
		if(IsNullBlock(Point(right.x, tempY))) {
			setRightBottom.insert(tempY);
		}
		else {
			break;
		}
	}
	for(int tempY = right.y -1 ; tempY >= 0; tempY--) {
		if(IsNullBlock(Point(right.x, tempY))) {
			setRightBottom.insert(tempY);
		}
		else {
			break;
		}
	}

	for(std::set<int>::iterator it = setLeftTop.begin(); it != setLeftTop.end(); it++) {
		if (setRightBottom.count(*it) != 0 && StraightEliminable(Point(left.x, *it), Point(right.x, *it))) {
			return true;
		}
	}

	//vertical first
	setLeftTop.clear();
	setRightBottom.clear();
	Point top, bottom;
	if(a.x < b.x) {
		top = a;
		bottom = b;
	}
	else {
		top = b;
		bottom = a;
	}
	//find max null block list for top
	for(int tempX = top.x + 1; tempX < this->MetrixHeight; tempX++) {
		if(IsNullBlock(Point(tempX, top.y))) {
			setLeftTop.insert(tempX);
		}
		else {
			break;
		}
	}
	for(int tempX = top.x -1; tempX >= 0; tempX--) {
		if(IsNullBlock(Point(tempX, top.y))) {
			setLeftTop.insert(tempX);
		}
		else {
			break;
		}
	}

	//find max null block list for bottom
	for(int tempX = bottom.x + 1; tempX < this->MetrixHeight; tempX++) {
		if(IsNullBlock(Point(tempX, bottom.y))) {
			setRightBottom.insert(tempX);
		}
		else {
			break;
		}
	}
	for(int tempX = bottom.x -1; tempX >= 0; tempX--) {
		if(IsNullBlock(Point(tempX, bottom.y))) {
			setRightBottom.insert(tempX);
		}
		else {
			break;
		}
	}

	for(std::set<int>::iterator it = setLeftTop.begin(); it != setLeftTop.end(); it++) {
		if (setRightBottom.count(*it) != 0 && StraightEliminable(Point(*it, top.y), Point(*it, bottom.y))) {
			return true;
		}
	}

	return false;
}

void GameWindow::Eliminate(Point a) {
	//remove the point from original vector
	std::vector<Point>::iterator it = this->Label2Point[this->BlockMetrix[a.x][a.y].Label].begin();
	for(; it != this->Label2Point[this->BlockMetrix[a.x][a.y].Label].end(); it++) {
		if((*it).x == a.x && (*it).y == a.y) break;
	}
	this->Label2Point[this->BlockMetrix[a.x][a.y].Label].erase(it);

	//modify the label to nulllabel and push it into the nulllabel vector
	this->BlockMetrix[a.x][a.y].Label = this->NullLabel;
	this->Label2Point[this->NullLabel].push_back(a);
}

void GameWindow::Draw() {
	CImg<byte> draw = this->Img;
	char* text = new char[100];

	for(int i = 0; i < MetrixWidth; i++) {
		for(int j = 0; j < MetrixHeight; j++) {
			int blockLabel = this->BlockMetrix[j][i].Label;
			int tempX = this->BlockMetrix[j][i].ClickPosition.x;
			int tempY = this->BlockMetrix[j][i].ClickPosition.y;
			sprintf(text, "%d", blockLabel);
			const unsigned char color[] = { 66,32,164 };
			draw.draw_text(tempX, tempY, text, color, 1, 1.1f, 20);
		}
	}
	//draw.display("drawn");
}

bool GameWindow::IsFinished() {
	return this->Label2Point[this->NullLabel].size() == this->MetrixHeight * this->MetrixWidth;
}

void GameWindow::StartGame() {
	while(!IsFinished()) {
		MatchBlock();
	}
}

int GameWindow::GetRestCount() {
	return this->MetrixHeight * this->MetrixWidth - this->Label2Point[this->NullLabel].size();
}