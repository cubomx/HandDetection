#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <stdio.h>

#define underExposure 85
#define overExposure 170


using namespace std;
using namespace cv;

void histogram(Mat);
void initializeArray(int []);
void makeHistogram(int[], Mat);
void decideExposure(float []);
void tresholdBinary(Mat, uchar, uchar, int, int, string, int [], int, int);

int main() {
    namedWindow("Cam", WINDOW_AUTOSIZE);
    VideoCapture cap(0);
    Mat(500, 500, CV_8U);
    if (!cap.isOpened()){
        return -1;
    }
    while (true){
        Mat frame;
        cap >> frame;

        Mat frameGray;
        cvtColor(frame, frameGray, COLOR_BGR2GRAY);
        imshow("Cam", frameGray);
        histogram(frameGray);
        waitKey(30);
        if (waitKey(30) >= 0){
            break;
        }
    }
    return 0;
}

void histogram(Mat original){


    Mat output;
    output = original.clone();

    int counts [256];
    float pmf[256], cdf[256];
    initializeArray(counts);

    float totalPixels = original.rows * original.cols;
    makeHistogram(counts, original);
    string imageVeredict = "Image is ";
    for (int rgb = 0; rgb < 255; rgb++) {
        pmf[rgb] = counts[rgb] / totalPixels;
        cdf[rgb] = pmf[rgb];
        if (rgb > 1)
            cdf[rgb] += cdf[rgb-1];
    }
    decideExposure(cdf);

    for (int j = 0; j < output.rows; j++){
        for (int i = 0; i < output.cols; i++) {
            output.at<uchar>(j,i) =  (uchar) (cdf[original.at<uchar>(j, i)] * 255);
        }
    }

    namedWindow("NEW", WINDOW_AUTOSIZE );
    imshow("NEW", output);
}

void initializeArray(int array []){
    for (int rgb = 0; rgb < 255; rgb++) {
        array[rgb] = 0;
    }
}

void makeHistogram(int counts [], Mat original){
    int greatest = 0, less = -1, posMinor = 0;
    Mat histogram(768, 768, CV_8U);
    histogram.setTo(0);
    for (int j = 0; j < original.rows; j++){
        for (int i = 0; i < original.cols; i++) {
            counts[(int)original.at<uchar>(j,i)]++;
        }
    }

    for (int pos = 0; pos < 256; pos++){
        if (counts[pos] > greatest) {
            greatest = counts[pos];
        }
        if (pos > underExposure && pos < overExposure){
            if (counts[pos] < less || less == -1){
                posMinor = pos;
                less = counts[pos];
            }
        }

    }
    tresholdBinary(original, (uchar)0, (uchar) 255, posMinor, 250, "Detection", counts, less, ceil((float)less / (float)greatest * 100000));
    float max = 768.0f / greatest;
    for (int rgb = 0; rgb < 256; rgb++) {
        rectangle(histogram, Point(rgb * 3, histogram.rows), Point(rgb * 3 + 3, histogram.rows - ((max * counts[rgb]))), Scalar(255, 255, 255), FILLED, LINE_8, 0);
    }
    namedWindow("Histogram", WINDOW_AUTOSIZE );
    imshow("Histogram", histogram);

}

/*
 *  Checking if there's much light, or the image is obscure, or the light tends to be
 *  in a moderate proportion
 */

void decideExposure(float rgb []){
    if (rgb[underExposure] > rgb[overExposure] - rgb[underExposure]){
        if (rgb[underExposure] > rgb[255] - rgb[overExposure])
            cout << "Image is underexposure\n";
        else
            cout << "Image is overexposure\n";
    }
    else if (rgb[overExposure] - rgb[underExposure] > rgb[255] - rgb[overExposure]){
        cout << "Image is well contrasted\n";
    }
    else{
        cout << "Image is overexposure\n";
    }
}

void tresholdBinary(Mat original, uchar color1, uchar color2, int umbral, int upUmbral, string name, int rgb [], int upper, int offset){
    Mat newFromOriginal = original.clone();

    for (int j = 0; j < newFromOriginal.rows; j++){
        for (int i = 0; i < newFromOriginal.cols; i++) {
            if (original.at<uchar>(j,i) >= umbral || (original.at<uchar>(j,i) < upUmbral && (int) rgb[original.at<uchar>(j,i)] < upper - offset) ){
                newFromOriginal.at<uchar>(j,i) = color2;
            }
            else{
                newFromOriginal.at<uchar>(j,i) = color1;
            }
        }
    }
    namedWindow(name, WINDOW_AUTOSIZE );
    imshow(name, newFromOriginal);
}


