#define USE_TBB

#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <fstream>

#include <chrono>

#ifdef USE_TBB
#include "tbb/tbb.h"
#endif


    /* clockwise
    90: 0, -1
        1, 0
        x + rows
    180:-1, 0
        0, -1
        x + cols
        y + rows
    270:0, 1
        -1,0
        y + cols
    */

using namespace std;

inline cv::Mat RotateImageInternal(const cv::Mat &image, int type)
{
    cv::Mat rotated(image.t());
    cv::flip(rotated, rotated, type);
    return rotated;

}

inline cv::Mat RotateImage90(const cv::Mat &image)
{
    return RotateImageInternal(image, 1);
}


int main(int argc, char *argv[])
{
    string srcFile, destFile;
    cv::Mat srcImg;
    if (argc != 4)
    {
        cout << "Wrong input arguments." << endl;
        cout << "Usage like below:" << endl;
        cout << "Rotate a image anti-clockwise." << endl;
        cout << "Syntax: Rotate $src_image_file $degree $dest_image_file" << endl;
        cout << "$degree is enum, 1, 2, 3 mean 90, 180, 270 degree." << endl;
        return 1;
    }

    srcFile = argv[1];
    int degreeEnum = 0;
    try
    {
        degreeEnum = stoi(argv[2]);
        if (1 != degreeEnum
            && 2 !=  degreeEnum
            && 3 != degreeEnum)
            throw invalid_argument("Argument 2, only [1,2,3] permitted.");
    }
    catch (invalid_argument& excpt)
    {
        cout << excpt.what() << endl;
        return 2;
    }
    destFile = argv[3];

    srcImg = cv::imread(srcFile);
    if (srcImg.empty())
    {
        cout << "Argument 1: invalid source image file." << endl;
        return 2;
    }

    auto t0 = chrono::high_resolution_clock::now();

    auto sRows = srcImg.rows;
    auto sCols = srcImg.cols;
    int channels = srcImg.channels();

#ifndef USE_TBB
    vector<uchar> destData(sRows*sCols*channels);
#else
    tbb::concurrent_vector<uchar> destData(sRows*sCols*channels);
#endif

    
    cv::Mat destImg;
    if (3 == degreeEnum
        || 1 == degreeEnum)
    {
        destImg = cv::Mat(sCols, sRows, CV_8UC3, &destData[0]);
    }
    else if (2 == degreeEnum)
    {
        destImg = cv::Mat(sRows, sCols, CV_8UC3, &destData[0]);
    }

#ifndef USE_TBB
    for (int i = 0; i < sRows; ++ i)
#else
    tbb::parallel_for(0, sRows, [&](int i)
#endif
    {        
        if (3 == degreeEnum)// 90 degree, anti-clockwise
        {
            for (int j = 0; j < sCols; ++ j)
            {            
                int x = 0, y = 0;
                auto& val = srcImg.at<cv::Vec3b>(i, j);
                x = j * 0 - i * 1;
                y = j * 1 + i * 0;
                x += sRows - 1;                
                destData[(y*sRows+x)*channels] = val[0];
                destData[(y*sRows+x)*channels+1] = val[1];
                destData[(y*sRows+x)*channels+2] = val[2];
            }
        }
        else if (2 == degreeEnum)// 180 degree, anti-clockwise
        {
            for (int j = 0; j < sCols; ++ j)
            {
                int x = 0, y = 0;
                auto& val = srcImg.at<cv::Vec3b>(i, j);
                x = j * -1 + i * 0;
                y = j * 0 + i * -1;
                x += sCols - 1;
                y += sRows - 1;
                destData[(y*sCols+x)*channels] = val[0];
                destData[(y*sCols+x)*channels+1] = val[1];
                destData[(y*sCols+x)*channels+2] = val[2];
            }
        }
        else if (1 == degreeEnum)// 270 degree, anti-clockwise
        {
            for (int j = 0; j < sCols; ++ j)
            {
                int x = 0, y = 0;
                auto& val = srcImg.at<cv::Vec3b>(i, j);
                x = j * 0 + i * 1;
                y = j * -1 + i * 0;
                y += sCols - 1;
                destData[(y*sRows+x)*channels] = val[0];
                destData[(y*sRows+x)*channels+1] = val[1];
                destData[(y*sRows+x)*channels+2] = val[2];
            }
        }
#ifndef USE_TBB
    }
#else
    });
#endif

    auto t1 = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << "ms" << endl;

    if (!cv::imwrite(destFile, destImg))
    {
        cout << "Cannot write rotated file." << endl;
        return 3;
    }

    t0 = chrono::high_resolution_clock::now();
    cv::Mat destImg2 = RotateImage90(srcImg);
    t1 = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << "ms" << endl;

    auto idx = destFile.rfind(".");
    destFile.insert(idx, "+");
    if (!cv::imwrite(destFile, destImg2))
    {
        cout << "Cannot write rotated file." << endl;
        return 3;
    }
    
    /* verification of 270 degrees with built-in version
    [&]{
        for (int i = 0; i < destImg.rows; ++ i)
            for (int j = 0; j <destImg.cols; ++ j)
            {
                if (destImg.at<cv::Vec3b>(j, i) != destImg2.at<cv::Vec3b>(j, i))
                {
                    cout << "Result is wrong." << endl;
                    return;
                }
            }
    }();
    */
    return 0;
}
