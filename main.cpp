/*
 * Code made by Felipe López Valdez and Sergio Velázquez
 */
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

struct imageTo{
    Mat output;
    int umbral;
};

imageTo histogram(Mat, string, bool);
void initializeArray(int []);
void makeHistogram(int[], Mat, string);
bool decideExposure(float []);
void tresholdBinary(Mat, uchar, uchar, int,  string);
void cdf_pmf(float [], float [], int [], float, Mat, Mat);
int umbral (int []);

int main() {
    namedWindow("Cam", WINDOW_AUTOSIZE);
    namedWindow("Canny_1", WINDOW_AUTOSIZE);
    VideoCapture cap(0);
    Mat(500, 500, CV_8U);
    if (!cap.isOpened()){
        return -1;
    }
    while (true){
        Mat frame;
        cap >> frame;
        Mat out;
        Mat frameGray;
        Mat edges;
        cvtColor(frame, frameGray, COLOR_BGR2GRAY);
        Canny(frameGray, out, 20, 30);

        Canny(frameGray, edges, 60, 128);
        imshow("Cam", edges);
        imshow("Canny_1", out);
        imageTo im = histogram(frameGray, "NOT EQUALIZED", false);
        histogram(im.output, "EQUALIZED", true);
        waitKey(30);
        if (waitKey(30) >= 0){
            break;
        }
    }
    return 0;
}

imageTo histogram(Mat original, string name, bool equalized){
    imageTo im;
    Mat output;
    output = original.clone();
    int counts [256];
    float pmf[256], cdf[256];
    initializeArray(counts);
    float totalPixels = original.rows * original.cols;

    makeHistogram(counts, original, name);
    cdf_pmf(cdf, pmf, counts, totalPixels, output, original);
    decideExposure(cdf);
    im.output = output;
    im.umbral = 95;
    namedWindow(name, WINDOW_AUTOSIZE );
    imshow(name, output);
    umbral(counts);
    if (!equalized){
        tresholdBinary(original, (uchar)255, (uchar)0, umbral(counts), "Equalized");
    }

    return im;
}



void initializeArray(int array []){
    for (int rgb = 0; rgb < 255; rgb++) {
        array[rgb] = 0;
    }
}

void makeHistogram(int counts [], Mat original, string name){
    int greatest = 0;
    Mat histogram(768, 768, CV_8U);
    histogram.setTo(0);
    for (int j = 0; j < original.rows; j++){
        for (int i = 0; i < original.cols; i++) {
            counts[(int)original.at<uchar>(j,i)]++;
        }
    }

    for (int pos = 0; pos < 256; pos++){
        if (counts[pos] > greatest ){
            greatest = counts[pos];
        }

    }

    float max = 768.0f / greatest;
    for (int rgb = 0; rgb < 256; rgb++) {
        rectangle(histogram, Point(rgb * 3, histogram.rows), Point(rgb * 3 + 3, histogram.rows - ((max * counts[rgb]))), Scalar(255, 255, 255), FILLED, LINE_8, 0);
    }
    namedWindow(name + "Histogram", WINDOW_AUTOSIZE );
    imshow(name + "Histogram", histogram);
}

/*
 *  Checking if there's much light, or the image is obscure, or the light tends to be
 *  in a moderate proportion
 */

bool decideExposure(float rgb []){
    if (rgb[underExposure] > rgb[overExposure] - rgb[underExposure]){
        if (rgb[underExposure] > rgb[255] - rgb[overExposure]){
            cout << "Image is underexposure\n";
            return false;
        }

        else{
            cout << "Image is overexposure\n";
            return true;
        }


    }
    else if (rgb[overExposure] - rgb[underExposure] > rgb[255] - rgb[overExposure]){
        cout << "Image is well contrasted\n";
        return true;
    }
    else{
        cout << "Image is overexposure\n";
        return true;
    }
}

void tresholdBinary(Mat original, uchar color1, uchar color2, int umbral, string name){
    Mat newFromOriginal = original.clone();
    Mat other;
    for (int j = 0; j < newFromOriginal.rows; j++){
        for (int i = 0; i < newFromOriginal.cols; i++) {
            if (original.at<uchar>(j,i) < umbral){
                newFromOriginal.at<uchar>(j,i) = color2;
            }
            else{
                newFromOriginal.at<uchar>(j,i) = color1;
            }
        }
    }
    dilate(newFromOriginal, other, NULL);
    namedWindow(name, WINDOW_AUTOSIZE );
    imshow(name, other);
}

int umbral(int histogramValues[]){
    float acumVals1 = 0, acumVals2 = 0, media1 = 0, media2 = 0, segmentos = 2, tamano = 256;

    for (int i = 0; i < tamano; i++) {
        if (i < tamano / segmentos) {
            if (histogramValues[i] > 0) {
                acumVals1 += i;
            }
        }
        else {
            if (histogramValues[i] > 0) {
                acumVals2 += i;
            }
        }
    }

    media1 = acumVals1 / tamano;
    media2 = acumVals2 / tamano;
    int umbral = media1 + media2;
    return umbral;
}

void cdf_pmf(float cdf [], float pmf [], int counts [], float totalPixels, Mat output, Mat original ){
    int sumaTotal = 0;
    for (int rgb = 0; rgb < 255; rgb++) {
        pmf[rgb] = (float)counts[rgb] / totalPixels;
        cdf[rgb] = pmf[rgb];
        if (rgb >= 1){
            cdf[rgb] += cdf[rgb-1];
            if (rgb > underExposure && rgb < overExposure){
                sumaTotal += counts[rgb];
            }
        }
    }



    for (int j = 0; j < output.rows; j++){
        for (int i = 0; i < output.cols; i++) {
            output.at<uchar>(j,i) =  (uchar) (cdf[original.at<uchar>(j, i)] * 255);
        }
    }
}
