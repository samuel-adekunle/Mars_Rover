#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <bitset>
#include <random>

using namespace cv;
using namespace std;


#define imdisplay(b,a) imshow(b,(Bounding_Circle_Draw(*RLE_Decode(a), Bounding_Circle_Points(a))));waitKey(0);

constexpr int HEIGHT = 600;
constexpr int WIDTH = 600;

enum FilterType { DUMBFILTER = 0, DELETEFILTER = 1, MERGEFILTER = 2 };

void addNoise(Mat* im) {
	for (int i = 0; i < 5000; i++) {
		int randx = (rand() % 600) + 1;
		int randy = (rand() % 600) + 1; 
		int radius = (rand() % 100) == 1 ? 7 : 0;
		circle(*im, Point(randx, randy), radius, Scalar(255, 255, 255),-100);
	}
}

void addNegNoise(Mat* im) {
	for (int i = 0; i < 100; i++) {
		int randx = (rand() % 600) + 1;
		int randy = (rand() % 600) + 1;
		int radius = 12;
		circle(*im, Point(randx, randy), radius, Scalar(0, 0, 0), -100);
	}
}


vector<uint16_t>* RLE_Encode(Mat* im) { //returns pointer to vector<uint16_t> array
	static vector<uint16_t> RLE[HEIGHT]; //static to permemently allocate mem.
	for (uint16_t row = 0; row < HEIGHT; row++) {
		int tally = 0;
		uchar* ptr = im->ptr(row);
		uchar prev = 0;
		for (uint16_t col = 0; col < WIDTH; col++) {
			if (ptr[col] == prev) {
				tally++;
			}
			else {
				RLE[row].push_back(tally);
				tally = 1;
			}
			prev = ptr[col];
		}
		RLE[row].push_back(tally);
	}
	return RLE;
}

// Destroys all features below a threshold and then picks *only* the largest consecutive row of white bits to display
auto RLE_DumbFilter(vector<uint16_t>* RLE) { 
	constexpr int MIN_FEATURE_SIZE = 45;

	static vector<uint16_t> RLE_dumb[HEIGHT];
	for (uint16_t row = 0; row < HEIGHT; row++) {
		uint16_t streak = 0;
		uint16_t sum = 0;
		uint16_t streaksum = 0;
		for (uint16_t i = 0; i < RLE[row].size(); i++) {
			if (i % 2) { //false for black, true for white
				if (streak < RLE[row][i]) {
					streak = RLE[row][i];
					streaksum = sum;
				}
			}
			sum += RLE[row][i];
		}
		if (streak > MIN_FEATURE_SIZE) {
			RLE_dumb[row] = { streaksum,streak, uint16_t(HEIGHT - 1 - streaksum - streak) };
		}
		else {
			RLE_dumb[row] = { HEIGHT - 1 };
		}
	}
	return RLE_dumb;
}

// Each streak of white or black pixels attempts to eat the next one - if it's larger, it succeeds and merges with the streak further up, else it gets eaten.
auto RLE_DeleteFilter(vector<uint16_t>* RLE) {
	constexpr int MIN_FEATURE_SIZE = 45;

	static vector<uint16_t> RLE_delete[HEIGHT];
	for (uint16_t row = 0; row < HEIGHT; row++) {
		RLE_delete[row] = RLE[row];
		if (RLE_delete[row].size() <= 3) {
			continue;
		}
		for (int i = 0; i < RLE_delete[row].size() - 2; i++) {
			if (RLE_delete[row].size() <= 3) {
				break; //vector size changes -- gotta make sure we don't overreach
			}
			if (i > 0 && RLE_delete[row][i] < MIN_FEATURE_SIZE) {
				RLE_delete[row][i - 1] += RLE_delete[row][i] + RLE_delete[row][i + 1];
				RLE_delete[row].erase(RLE_delete[row].begin() + i);
				RLE_delete[row].erase(RLE_delete[row].begin() + i);
				i--;
			}
		}
	}
	return RLE_delete;
}

// Similar to Darwin filter, but weighted. Sums of adjacent black/white streaks attempt to merge, and they are able as long as their sums are larger than the size
// of the streak inbetween them, times an assymetric coefficient.
auto RLE_MergeFilter(vector<uint16_t>* RLE, int WHITE_CONST, int BLACK_CONST) {
	// CURRENTLY SEMI-FUNCTIONAL
	//constexpr int WHITE_CONST = 8; // How resistant black streaks are. Low values surpress negative noise.
	//constexpr int BLACK_CONST = 6; // How resistant white streaks are. Low values surpress positive noise.

	static vector<uint16_t> RLE_merge[HEIGHT];
	for (uint16_t row = 0; row < HEIGHT; row++) {
		RLE_merge[row] = RLE[row];
		if (RLE_merge[row].size() <= 3) { //need 2 checks
			continue;
		}
		for (int i = 0; i < RLE_merge[row].size() - 2; i++) {
			int MAGIC_CONST = (i % 2) ? WHITE_CONST : BLACK_CONST;
			if (RLE_merge[row].size()<=3) {
				break; //vector size changes -- gotta make sure we don't overreach
			}
			if (RLE_merge[row][i] + RLE_merge[row][i+2] > MAGIC_CONST * RLE_merge[row][i + 1]) {
				RLE_merge[row][i] = RLE_merge[row][i] + RLE_merge[row][i + 1] + RLE_merge[row][i + 2];
				RLE_merge[row].erase(RLE_merge[row].begin() + i + 1);
				RLE_merge[row].erase(RLE_merge[row].begin() + i + 1);
				i--;
			}
		}
	}
	return RLE_merge;
}



auto RLE_MasterFilter(vector<uint16_t>* RLE, int type, int var1 = 2, int var2 = 2) {
	switch (type) {
	case 0:
		return RLE_DumbFilter(RLE);
	case 1:
		return RLE_DeleteFilter(RLE);
	case 2:
		return RLE_MergeFilter(RLE, var1, var2);
	default:
		throw "Invalid Filter";
	}
}


Mat* RLE_Decode(vector<uint16_t>* vect) {
	Mat* im = new Mat;
	*im = Mat::zeros(HEIGHT, WIDTH, CV_8U); //gray-scale
	for (uint16_t row = 0; row < HEIGHT; row++) {
		int index = 0;
		bool symbol = 1;
		for (uint16_t i = 0; i < vect[row].size(); i++) {
			symbol = !symbol; // ensure we *always* have a black-white-black-.. sequence
			for (int j = 0; j < vect[row][i]; j++) {
				im->at<uchar>(Point(index, row)) = (255* symbol); // even grayscale is mapped to 0 and 255 - no way to get boolean maps onto opencv natively.
				index++;
				if (index > WIDTH) {
					break;
				}
			}
			if (index > WIDTH) {
				break;
			}
		}
	}
	return im;
}

tuple<Point,Point> Bounding_Circle_Points(vector<uint16_t>* RLE) {
	// we are looking to find the smallest initial black streak to find the left-most white pixel
	// and the smallest latest black streak to find the right-most white pixel
	int min = WIDTH; //coordinate of left-most
	int minrow = 0;
	int max = 0; // coordinate of right-most
	int maxrow = 0;
	for (uint16_t row = 0; row < HEIGHT; row++) {
		switch (RLE[row].size()) {
		case 0:
			throw "0 size matrix input"; // something's wrong
		case 1: // all black
			continue;
		case 2: // all white
			min = 0;
			max = WIDTH;
			minrow = row;
			maxrow = row;
			break;
		default: // black-white-black or more complicated
			if (!(RLE[row].size() % 2)) { // cleanly divided by 2, so black-white-black-white or equivalent. Ends with white anyway. Special case for right-most white pixel.
				max = WIDTH;
				maxrow = row;
			}
			else { // ends in black. More readable than else-if
				if (RLE[row].back() < WIDTH - max) {
					max = WIDTH - RLE[row].back();
					maxrow = row;
				}
			} // in any case, check white normally.
			if (RLE[row][0] < min) {
				min = RLE[row][0];
				minrow = row;
			}
		}
	}
	return make_tuple(Point(min, minrow), Point(max, maxrow));
}


Mat Bounding_Circle_Draw(Mat &im, tuple<Point,Point> tup) {
	auto [point1, point2] = tup;
	assert(point1.x < point2.x); // min < max
	int diff1 = round(abs(point1.x - point2.x));
	int diff2 = round(abs(point2.y - point1.y));
	int radius = round(sqrt((diff1 * diff1) + (diff2 + diff2))/2);
	Point centre = point1 + Point(round(diff1 / 2), round(diff2 / 2));

	Mat ColourImg = Mat::zeros(HEIGHT, WIDTH, CV_8UC3);
	cvtColor(im, ColourImg, COLOR_GRAY2BGR);
	circle(ColourImg, centre, radius, Scalar(0, 255, 0));
	circle(ColourImg, point1, 3, Scalar(0, 0, 255),-100);
	circle(ColourImg, point2, 3, Scalar(0, 0, 255),-100);
	return ColourImg;
}


void FilterDisplay(Mat image) { //legacy opencv filter display.
	Mat disp;
	image.copyTo(disp);
	putText(disp, "Unfiltered", Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,1, Scalar(255), 2);
	imshow("Display Window", disp);
	waitKey(0);
	Mat filter;
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3,3));
	int iter = 3;
	morphologyEx(image, filter, MORPH_OPEN, kernel, Point(-1, -1), iter);
	Mat disp1;
	filter.copyTo(disp1);
	putText(disp1, "Open, iterations = 3", Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, Scalar(255), 2);
	imshow("Display Window", disp1);
	waitKey(0);
	Mat filter2;
	int iter2 = 8;
	morphologyEx(image, filter2, MORPH_OPEN, kernel, Point(-1, -1), iter2);
	Mat disp2;
	filter2.copyTo(disp2);
	putText(disp2, "Open, iterations = 8", Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, Scalar(255), 2);
	imshow("Display Window", disp2);
	waitKey(0);
	Mat filter3;
	Mat kernel3 = getStructuringElement(MORPH_ELLIPSE, Size(7,7));
	int iter3 = 20;
	morphologyEx(filter2, filter3, MORPH_CLOSE, kernel3, Point(-1, -1), iter3);
	Mat disp3;
	filter3.copyTo(disp3);
	putText(disp3, "Open, iter =8, then Close, iter =20", Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, Scalar(255), 2);
	imshow("Display Window", disp3);
	waitKey(0);
}

int main()
{
	srand(1337);
	Mat image = Mat::zeros(HEIGHT, WIDTH, CV_8U);

	circle(image, Point(250, 150), 100, Scalar(255), -100);
	circle(image, Point(50, 250), 15, Scalar(255), -100);
	addNoise(&image);
	addNegNoise(&image);

	//rectangle(image, Point(100, 100), Point(200, 500), 255, -100);
	//rectangle(image, Point(400, 100), Point(500, 500), 255, -100);


	imshow("Im", image);
	waitKey(0);
	auto encode = RLE_Encode(&image);

	vector<uint16_t>* filtered = RLE_MasterFilter (encode,DUMBFILTER);
	imdisplay("Dumb",filtered); //^^ imdisplay(b, a) is imshow(b, (Bounding_Circle_Draw(*RLE_Decode(a), Bounding_Circle_Points(a)))); waitKey(0);
	filtered = RLE_MasterFilter(encode, DELETEFILTER, 8 , 15);
	imdisplay("Merge", filtered);
	filtered = RLE_MasterFilter(RLE_MasterFilter(encode,MERGEFILTER, 8 , 15), DUMBFILTER);
	imdisplay("Merge & Dumb", filtered);

	//FilterDisplay(image);
}