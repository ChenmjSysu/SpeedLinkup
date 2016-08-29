#include "CImg.h"
#include "Common.h"
#include <queue>

#define byte unsigned char
#define PI 3.14159265358979323846
//#define sigma 1.4
//#define gaussWidth 5
//#define gaussHeight 5
//#define maxThreshold 50
//#define minThreshold 20
float sigma;
int gaussWidth;
int gaussHeight;
int maxThreshold;
int minThreshold;
#define angleStep 2
#define angleVar 20

#define jiaodianThreshold 0.3

#define LINE_NUMBER 4

#define minimum(x, y) x < y ? x : y
#define maximum(x, y) x > y ? x : y
#define cot(t) cos(t) / sin(t)

using namespace cimg_library;

void swapPoint(Point &p1, Point &p2) {
    int x = p1.x;
	int y = p1.y;
	p1.x = p2.x;
	p1.y = p2.y;
	p2.x = x;
	p2.y = y;
}


//输出图像信息
void printInfo(CImg<byte> img, const char* filename) {
	int width = img.width();
	int height = img.height();
	int spectrum = img.spectrum();
	printf("open image %s\n", filename);
	printf("width: %d\nheight: %d\nspectrum: %d\n", width, height, spectrum);
	printf("maxThreshold: %d, minThreshold: %d\n", maxThreshold, minThreshold);
}

//获取一维高斯核
double* getGaussKernel(int windowSize) {
    double *gauss = new double[windowSize];
	for(int i = 0; i < windowSize; i++) gauss[i] = 0;
	double sum = 0;
	int middle = windowSize / 2;

	//求出高斯滤波器的每个值
	double sigma2 = 2 * sigma * sigma;
	double PI2 = 2 * PI;
	for(int i = 0; i < windowSize; i++) {
	    double distance = i - middle;
		double sqrtSigmaPi2 = (float)sqrt(PI2) * sigma ;  
		gauss[i] = (float)sqrt(1 / (2 * PI)) / (float)sigma * (float)exp(-(distance * distance) / (float)(2 * sigma * sigma));
		sum += gauss[i];
	}
	//对刚刚算出的滤波器做归一化
	for(int i = 0; i < windowSize; i++) {
	    gauss[i] /= sum;
	}
	return gauss;
}

//对X方向做高斯滤波
void XGaussFilter(CImg<byte> &img) {
	double *gauss = getGaussKernel(gaussWidth);
	cimg_forY(img, y) {
		cimg_forX(img, x) {
			double sum = 0;
			double gaussSum = 0;
			for(int i = -gaussWidth / 2; i < gaussWidth / 2; i++) {
				byte value = img(x, y);
				if(!(x + i < 0 || x + i >= img.width())) {
				    value = img(x + i, y);
				}
				else {
					/*int temp = (x + i) < 0 ? 0 : img.width();
				    value = (temp, y);*/
				}
				sum += value * gauss[i + gaussWidth / 2];
				gaussSum += gauss[i + gaussWidth / 2];
				//printf("%lf\n", gauss[i + gaussWidth / 2]);
			}
			cimg_forC(img, v) {
			    img(x, y, 0, v) = sum / gaussSum;
			}
		}
	}
}

//对Y方向做高斯滤波
void YGaussFilter(CImg<byte> &img) {
	double *gauss = getGaussKernel(gaussHeight);	
	cimg_forX(img, x) {
		cimg_forY(img, y) {
			double sum = 0;
			double gaussSum = 0;
			for(int i = -gaussWidth / 2; i < gaussWidth / 2; i++) {
				byte value = img(x, y);
				if(!(y + i < 0 || y + i >= img.height())) {
				    value = img(x, y + i);
				}
				else {
					/*int temp = (y + i) < 0 ? 0 : img.height();
				    value = (x, temp);*/
				}
				sum += value * gauss[i + gaussWidth / 2];
				gaussSum += gauss[i + gaussWidth / 2];
			}
            cimg_forC(img, v) {
			    img(x, y, 0, v) = sum / gaussSum;
			}
		}
	}
}

//灰度化
void toGray(CImg<byte> &img, CImg<byte> src) {	
	cimg_forXY(img, x, y) {
		byte value = 0;
		int spectrum = src.spectrum();
		cimg_forC(src, v) {
		    value += src(x, y, 0, v) / spectrum;
			//printf("%d\n", v);
		}
	    cimg_forC(img, v) {
		    img(x, y, 0, v) = value;
		}
	}
}

//做高斯滤波
void GaussFilter(CImg<byte> &img) {
	//img.display();
	XGaussFilter(img);
	YGaussFilter(img);
}

//计算梯度大小和方向
void calcGradient(CImg<byte> &gradient, CImg<double> &gradientAngle, 
				  CImg<int> &gradientX, CImg<int> &gradientY, CImg<byte> src) {
	CImg_2x2(I, byte);
	cimg_for2x2(src, x, y, 0, 0, I, byte) {
	    gradientX(x, y) = ((Inc + Inn - Icc - Icn) / 2);
		gradientY(x, y) = ((Icc + Inc - Icn - Inn) / 2);
	}

	cimg_forXY(gradient, x, y) {
		if(y >= gradient.height() * 0.9) {
			gradient(x, y) = 0;
			continue;
		}
	    gradient(x, y) = sqrt(gradientX(x, y) * gradientX(x, y) + gradientY(x, y) * gradientY(x, y)) + 0.5;	
		float angle = atan2(gradientY(x, y), gradientX(x, y)) * 53.7;
		//printf("%lf\n", angle);
		angle = angle < 0 ? angle + 180 : angle;
		gradientAngle(x, y) = angle;
	}
}

//非最大值抑制
void nonMaxSuppression(CImg<byte> &gradient, CImg<double> &gradientAngle, 
					   CImg<int> &gradientX, CImg<int> &gradientY, CImg<byte> &result) {
	float weight;
	float temp1, temp2; //插值后的像素值
	CImg_3x3(I, byte);
	cimg_for3x3(gradient, x, y, 0, 0, I, byte) {
	    double angle = gradientAngle(x, y);
		if(gradient(x, y) <= minThreshold) {
		    result(x, y) = 0;
			continue;
		}
		if(angle > 180) angle -= 180;
        if(angle < 0) angle += 180;
		if((angle <= 45)) {
			weight = angle / 45.0;
			temp1 = weight * Inp + (1 - weight) * Inc;
			temp2 = weight * Ipn + (1 - weight) * Ipc;
		}
		else if((angle <= 90)) {
		    weight = (angle - 45) / 45.0;
			temp1 = weight * Icp + (1 - weight) * Inp;
			temp2 = weight * Icn + (1 - weight) * Ipn;
		}
		else if((angle <= 135)) {
		    weight = (angle - 90) / 45.0;
			temp1 = weight * Ipp + (1 - weight) * Icp;
			temp2 = weight * Inn + (1 - weight) * Icn;
		}
		else if((angle <= 180)) {
		    weight = (angle - 135) / 45.0;
			temp1 = weight * Ipc + (1 - weight) * Ipp;
			temp2 = weight * Inc + (1 - weight) * Inn;
		}
		if(gradient(x, y) >= temp1 && gradient(x, y) >= temp2) {
			if(gradient(x, y) >= maxThreshold) {
				result(x, y) = 255;
			}
			else {
			    result(x, y) = 128;
			}
		}
		else {
		    result(x, y) = 0;
		}
	}
}

//获取点的8邻域中的潜在边缘点
void getPotentialEdgePoints(CImg<byte> &src, const Point point, std::queue<Point> &points) {
	int x = point.x, y = point.y;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			//点本身或者他的8邻域不存在（在界外）则忽略
			if((i == j && i == 0) || x + i < 0 || x + i >= src.width() || y + j < 0 || y + j >= src.height()) continue;
			if(src(x + i, y + j) > 0) {
			    points.push(Point(x + i, y + j));
				src(x + i, y + j) = 0;
				getPotentialEdgePoints(src, Point(x + i, y + j), points);
			}
		}
	}
}

//双阈值
void hysteresisThreshold(CImg<byte> src, CImg<byte> &result) {
	cimg_forXY(result, x, y) {
	    result(x, y) = 0;
	}
	cimg_forXY(src, x, y) {
		if(x == 0 || x == src.width() - 1 || y == 0 || y == src.height() - 1) continue;
		if(src(x, y) == 255) {
			Point point = Point(x, y);
			result(x, y) = 255;
			src(x, y) = 0;
			std::queue<Point> points;
			//从point这个点往外扩展找到的128或255的点
			getPotentialEdgePoints(src, point, points);
			while(!points.empty()) {
				Point temp = points.front();
				points.pop();
				result(temp.x, temp.y) = 255;
			}
		}
	}
	//src.display("原图 后");
}

//求出最大的四个数（四条边）
std::vector<Point> getEdgeLine(CImg<long> src) {
	//src.display("hough begin");
	std::vector<Point> points;
	Point temp;
	bool searchAngle = false;
	for(int i = 0; i < LINE_NUMBER; i++) {
		//src.display();
		long max = 0;
		if(i < LINE_NUMBER / 2) {
			cimg_forXY(src, x, y) {
				//取较平缓的直线
				//if(x > 180 / angleStep / 4 && x < 180 / angleStep * 3 / 4) continue;
				if(max < src(x, y)) {
					max = src(x, y);
					temp.x = x;
					temp.y = y;					
				}
			}
		}
		else {			
		    cimg_forXY(src, x, y) {
				//if(x <= 180 / angleStep / 4 || x >= 180 / angleStep * 3 / 4) continue;
				if(max < src(x, y)) {
					max = src(x, y);
					temp.x = x;
					temp.y = y;
				}
			}
		}
		printf("(%d, %d) count: %d\n", temp.x, temp.y, max);
		//将最值附近的值设为0,避免检测相同的直线
		int distanceVar = src.height() / 15;
		for(int m = -angleVar; m <= angleVar; m++) {
			for(int n = -distanceVar; n <= distanceVar; n++) {
				int houghWidth = src.width() - (180 - (180 / angleStep));
			    int x_ = temp.x + m;
				int y_ = temp.y + n;
				if(x_ < 0) {
				    x_ = houghWidth + x_;
				}
				else if(x_ >= houghWidth) {
				    x_ = x_ - houghWidth;
				}
				y_ = y_ < 0 ? 0 : y_;
				y_ = y_ >= src.height() ? src.height() - 1 : y_;
				src(x_, y_) = 0;
			}
		}		
		//src.display("hough");
		points.push_back(Point(temp.x, temp.y));
	}
	return points;
}

//Hough变换
void houghDetector(CImg<byte> &src, CImg<long> &hough, CImg<byte> &lines, bool showHough) {
	//初始化
	cimg_forXY(hough, x, y) {
	    hough(x, y) = 0;
	}
	cimg_forXY(src, x, y) {
	    if(src(x, y) == 0) continue;
		for(int i = 0; i < 180 / angleStep; i++) {
		    int p = fabs((float)x * cos((float)angleStep * (float)i / (180.0f/PI)) + (float)y * sin((float)angleStep * (float)i / (180.0f/PI)));
			//if(x < 100 || x > 450 || y < 120 || y > 127) continue;
			hough(i, p) += 1;	
			//printf("(%d, %d) to (%d, %d)\n", x, y, i, p);
		}
	}	
	if(showHough) {
		hough.display("Hough");
	}
	//计算最值
	std::vector<Point> points = getEdgeLine(hough);
	//用一个图像来显示检测到的直线
	
	//初始化
	cimg_forXY(lines, x, y) {
	    lines(x, y) = 0;
	}

	std::vector<Point>::iterator it = points.begin();
	///将线段转换到xy坐标轴上，0表示没有线段经过，1表示有较陡的直线，2表示有较缓的直线，10表示较陡和较缓的线段的交点
	for(; it != points.end(); it++) {
		float radius = angleStep * (it->x) / (180/PI);
		float sin_ = sin(radius);
		float cos_ = fabs(cos(radius));
		float cot_ = (cot(radius));
		float tan_ = (tan(radius));
		int p = it->y;
		printf("angle=%d  p=%d  radius=%f  sin=%f  cos=%f  cot=%f ", (it->x) * angleStep, it->y, radius, sin_, cos_, cot_);
		int jiaodian = 50;
		//垂直线，斜率无限大
		if(it->x == 0) {
			printf("chuizhi\n", it->x * angleStep);
			int x = p * cos_;
		    cimg_forY(lines, y) {
		        if(lines(x, y) % 2 == 0 && lines(x, y) != 0) lines(x, y) = jiaodian;
				if(lines(x, y) < jiaodian && lines(x, y) % 2 == 1) lines(x, y) += 2;
				else if(lines(x, y) == 0) lines(x, y) = 1;
				if(x != src.width() - 1) {  //增加直线宽度，避免漏点
					int xx = x + 1;
				    if(lines(xx, y) % 2 == 0 && lines(xx, y) != 0) lines(xx, y) = jiaodian;
					if(lines(xx, y) < jiaodian && lines(xx, y) % 2 == 1) lines(xx, y) += 2;
					else if(lines(xx, y) == 0) lines(xx, y) = 1;
				}
				//lines(x, y)++;
				//if(src(x, y) == 255) lines(x, y)++;
			}
		}
		//非垂直线
		else {
			//较平的直线，以X为步进单位
			if(it->x <= 135 / angleStep && it->x >= 45 / angleStep) {
				printf("X\n");
				cimg_forX(lines, x) {					
					int y = (int)(p / sin_ - x * cot_);
					//if(x < 20) printf("%d\n", y);
					if(y >= 0 && y < lines.height()) {
						/*if(lines(x, y) == 1) lines(x, y) = 100;
				        else lines(x, y) = 2;*/
						if(lines(x, y) % 2 == 1) lines(x, y) = jiaodian;
						if(lines(x, y) < jiaodian && lines(x, y) % 2 == 0) lines(x, y) += 2;
						else if(lines(x, y) == 0) lines(x, y) = 2;
						if(y != src.height() - 1) {    //增加直线宽度，避免漏点
							int yy = y + 1;
							if(lines(x, yy) % 2 == 1) lines(x, yy) = jiaodian;
							if(lines(x, yy) < jiaodian && lines(x, yy) % 2 == 0) lines(x, yy) += 2;
							else if(lines(x, yy) == 0) lines(x, yy) = 2;
						}
						//lines(x, y)++;
						//if(src(x, y) == 255) lines(x, y)++;
					}
				}
			}
			//较陡的直线，以Y为步进单位
			else {
				printf("Y\n", it->x * angleStep);
			    cimg_forY(lines, y) {					
					int x = (int)(p / cos_ - y * tan_);
					if(x >= 0 && x < lines.width()) {
						if(lines(x, y) % 2 == 0 && lines(x, y) != 0) lines(x, y) = jiaodian;
						if(lines(x, y) < jiaodian && lines(x, y) % 2 == 1) lines(x, y) += 2;
						else if(lines(x, y) == 0) lines(x, y) = 1;
						if(x != src.width() - 1) {     //增加直线宽度，避免漏点
							int xx = x + 1;
							if(lines(xx, y) % 2 == 0 && lines(xx, y) != 0) lines(xx, y) = jiaodian;
							if(lines(xx, y) < jiaodian && lines(xx, y) % 2 == 1) lines(xx, y) += 2;
							else if(lines(xx, y) == 0) lines(xx, y) = 1;
						}
						//lines(x, y)++;
						//if(src(x, y) == 255) lines(x, y)++;
					}
				}
			}
		}
		//printf("paint point(%d, %d)\n", it->x, it->y);
		//lines.display("lines");
	}
	//lines.display("lines");
}

//改进的hough，直接求角点
void houghDetectorVertex(CImg<byte> &src, std::vector<Point> &points) {
	int winWidth = 7;
	cimg_forXY(src, x, y) {
		if(src(x, y) == 0) continue;
		int pixCount = 0;
		CImg<byte> temp(180);
		cimg_forX(temp, x) { temp(x) = 0; }
		for(int i = -winWidth / 2; i <= winWidth / 2; i++) {
			for(int j = -winWidth / 2; j <= winWidth / 2; j++) {
			    int x_ = x + i;
				int y_ = y + j;
				if(x_ < 0 || x_ >= src.width() || y_ < 0 || y >= src.height()) continue;
				if(src(x_, y_) == 0) continue;
				int angle = (int)atan2(-j, i) * ((float)180 / PI);
				angle = angle < 0 ? angle + 180 : angle;
				angle = angle == 180 ? 0 : angle;				
				temp(angle)++;
				pixCount++;
			}
		}
		int lineCount = 0;
		cimg_forX(temp, x) {
			if((float)temp(x) / (float)pixCount >= jiaodianThreshold) {
			    lineCount++;
			}
		}
		//清空周围的边缘点
		if(lineCount >= 2) {
			printf("(%d, %d) lineCount:%d\n", x, y, lineCount);
			
			temp.display("temp");
			//src.display("src");
			points.push_back(Point(x, y));
		    for(int i = -winWidth / 2; i <= winWidth / 2; i++) {
				for(int j = -winWidth / 2; j <= winWidth / 2; j++) {
					int x_ = x + i;
					int y_ = y + j;
					if(x_ < 0 || x_ >= src.width() || y_ < 0 || y >= src.height()) continue;
					src(x_, y_) = 0;
				}
			}
		}
	}
}

//将顶点以左下角开始，顺时针排序
void sortVertex(std::vector<Point> &vertex) {
    int temp = 0;
	Point p1, p2, p3, p4;
	p1 = vertex[0];
	vertex.erase(vertex.begin());
	int min = 1 << 16;
	for(int i = 1; i < 3; i++) {
		if(min > abs(p1.x - vertex[i].x)) {
		    min = abs(p1.x - vertex[i].x);
			temp = i;
		}
	}
	p2 = vertex[temp];
	vertex.erase(vertex.begin() + temp);
	p3 = vertex[0];
	p4 = vertex[1];
	if(p1.y < p2.y) swapPoint(p1, p2);
	if(p3.y < p4.y) swapPoint(p3, p4);
	if(p1.x > p3.x) {
	    swapPoint(p1, p3);
		swapPoint(p2, p4);
	}
	vertex.clear();
	vertex.push_back(p1);
	vertex.push_back(p2);
	vertex.push_back(p4);
	vertex.push_back(p3);
}


//求出四个顶点
void caclTheVertex(CImg<byte> &src, std::vector<Point> &vertex) {
	//src.display("original");
	int countWidth = 70;
	//CImg<> I(countWidth, countWidth);
	CImg_5x5(I, byte);
	for(int i = 0; i < 4; i++) {
		int centerMax = 0;
		int neighborMax = 0;
		Point maxPoint;
		cimg_for5x5(src, x, y, 0, 0, I, byte) {
		    int centerCount = src(x, y);
			int sum = Ibb + Ipb + Icb + Inb + Iab
				    + Ibp + Ipp + Icp + Inp + Iap
					+ Ibc + Ipc + Icc + Inc + Iac
					+ Ibn + Ipn + Icn + Inn + Ian
					+ Iba + Ipa + Ica + Ina + Iaa;
			int neighborCount = sum - centerCount;
			if((centerCount > centerMax && neighborCount > neighborMax)) {
			    centerMax = centerCount;
				neighborMax = neighborCount;
				maxPoint.x = x;
				maxPoint.y = y;				
				//printf("sum=%d\n", sum);
				//printf("center=%d neighbor=%d centerMax=%d, neighborMax=%d\n", centerCount, neighborCount, centerMax, neighborMax);
			}
		}
		//printf("(%d, %d)\n", maxPoint.x, maxPoint.y);
		//将附近点设为0
		for(int m = -countWidth; m <= countWidth; m++) {
			for(int n = -countWidth; n <= countWidth; n++) {
				int x = maxPoint.x + m;
				int y = maxPoint.y + n;
				//防止越界
				if(x < 0 || x >= src.width() || y < 0 || y >= src.height()) continue;
				src(x, y) = 0;
			}
		}
		//src.display("aaa");
		vertex.push_back(Point(maxPoint.x, maxPoint.y));
	}
	//对顶点排序
	sortVertex(vertex);
}

std::vector<Point> GetGameWindows(CImg<byte> src) {

	bool flag = false;
	const bool showSrc = flag;
	const bool showGray = flag;
	const bool showGauss = flag;
	const bool showGradient = flag;
	const bool showEdge = flag;
	const bool showHough = flag;
	const bool showLines = flag;
	sigma = 1.5;
	gaussWidth = 11;
	gaussHeight = 11;
	maxThreshold = 50;
	minThreshold = 10;


	CImg<byte> img;
	try{
		img.assign(src);
	}
	catch (CImgException &e) {
		printf("Open file error!\n");		
	    return std::vector<Point>();
	}
	CImg<byte> gray(img);
	// printInfo(img, imageFilePath);
	if(showSrc) {
		//CImgDisplay display(img, "原图", true);
		img.display("原图");
	}
	//灰度化
	toGray(gray, img);
	if(showGray) {
		gray.display("灰度图");
	}
	
	//高斯模糊并显示
	CImg<byte> gaussBlur(gray);
	//GaussFilter(gaussBlur);
	gaussBlur = gray.get_blur(1.5, 1.5);
	
	if(showGauss) {
		//CImgDisplay gaussDisplay(gaussBlur, "高斯模糊", true);
		gaussBlur.display("高斯模糊");
	}
	
	//求梯度
	CImg<byte> gradient(gray.width(), gray.height());
	CImg<int> gradientX(gradient.width(), gradient.height()), gradientY(gradientX);
	CImg<double> gradientAngle(gray.width(), gray.height());
	calcGradient(gradient, gradientAngle, gradientX, gradientY, gaussBlur);
	
	if(showGradient) {
		//CImgDisplay gradientDisplay(gradient, "梯度图", true);
		gradient.display("梯度图");
	}
	
	//非最大值抑制
	CImg<byte> suppression(gradient);
	nonMaxSuppression(gradient, gradientAngle, gradientX, gradientY, suppression);
	///CImgDisplay suppressionDisplay(suppression, "非最大值抑制");
	//suppression.display(suppressionDisplay, true);
	//双阈值
	CImg<byte> hysteresis(suppression.width(), suppression.height());
	hysteresisThreshold(suppression, hysteresis);
	
	if(showEdge) {		
		//CImgDisplay hysteresisDisplay(hysteresis, "双阈值");
		hysteresis.display("双阈值");
	}
	//hough检测角点
	/*std::vector<Point> vertex;
	houghDetectorVertex(hysteresis, vertex);*/
	//hough变换检测直线
	CImg<long> hough(180 / 1, sqrt(hysteresis.width() * hysteresis.width() + hysteresis.height() * hysteresis.height()));
	CImg<byte> lines(hysteresis.width(), hysteresis.height());
	houghDetector(hysteresis, hough, lines, showHough);
	if(showLines) {
		lines.display("Hough检测到的直线");
	}

	//求四个顶点
	std::vector<Point> vertex;
	caclTheVertex(lines, vertex);

	
	return vertex;
}