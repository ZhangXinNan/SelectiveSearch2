/*
 *  Copyright @ Surge Wong -- 2014/09/05
 */

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/legacy/legacy.hpp"
#include "opencv2/legacy/compat.hpp"

#include <iostream>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include "FelZenSegment/image.h"
#include "FelZenSegment/misc.h"
#include "FelZenSegment/pnmfile.h"
#include "generate_bounding_boxes.h"

#include "type_convert.h"

using namespace std;
using namespace cv;

int main(int argc, char **argv) {

//	if(argc < 3){
//		fprintf(stderr,"Usage:%s inputimage(input) bounding-box(output) sigma k min\n",argv[0]);
//		fprintf(stderr,"For example:\n%s lena.jpg BB.txt\n");
//		exit(1);
//	}

//    const char* imagename = argv[1];
//    const char* outBB = argv[2];

    const char imagename[100] = "4.PPM";
    const char outBB[100] = "BoundingBoxes.txt";

    float sigma = 0.8;
    float k = 200;
    int min_size = 100;


    // read image
    Mat img = imread(imagename);
    Mat img_rgb;
    if (img.channels() == 1) {
        cvtColor(img, img_rgb, COLOR_GRAY2BGR);
    } else if (img.channels() == 4) {
        cvtColor(img, img_rgb, COLOR_BGRA2BGR);
    } else if (img.channels() == 3) {
        img_rgb = img.clone();
    } else {
        return 0;
    }

    // convert to image type
    image<rgb>* imginput = matToImage(img_rgb);

    std::cout << "generate_bounding_boxes   before ______________" << std::endl;
    int region_nums;
    int* BB = generate_bounding_boxes(imginput, sigma, k, min_size, &region_nums);
    std::cout << "generate_bounding_boxes   after  ______________" << std::endl;

    FILE* f;
    f = fopen(outBB,"w");
    for(int i = 0; i < region_nums;i++){
        fprintf(f,"%d %d %d %d \n",BB[4*i+0],BB[4*i+1],BB[4*i+2],BB[4*i+3]);
    }
    fclose(f);

    for(int i = 0; i < region_nums; i++){
    	rgb color = random_rgb();
    	for(int j = BB[4*i+0]; j <= BB[4*i+2]; j++){
    		imRef(imginput,j,BB[4*i+1]) = color;
    		imRef(imginput,j,BB[4*i+3]) = color;
    	}
    	for(int j = BB[4*i+1]; j <= BB[4*i+3]; j++){
    	    imRef(imginput,BB[4*i+0],j) = color;
    	    imRef(imginput,BB[4*i+2],j) = color;
    	}
    }

    img = *imageToMat(imginput);
    imwrite("testmat.jpg",img);
    printf("That is a hard work..\n");

    delete imginput;
    return 0;
}
