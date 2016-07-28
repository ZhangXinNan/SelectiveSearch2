/*
 * Copyright @ Surge Wong -- 2014/08/25
*/


#ifndef GENERATE_BB
#define GENERATE_BB

#include "colour_hist.h"
#include "texture_hist.h"
#include "region.h"
#include "regions_parameters.h"

#include <cstdlib>
#include<cstdio>
#include <algorithm>


//typedef struct {
//	int x1;
//	int y1;
//	int x2;
//	int y2;
//}bbox;

/*
 * Generate bounding boxes for input image
 *
 * Returns a series of bonudng boxing parameters, two point (xmin,ymin)(xmax,ymax).
 *
 * im: input image.
 * sigma: to smooth the image.
 * c: constant for treshold function.
 * min_size: minimum component size (enforced by post-processing stage).
 */
int *generate_bounding_boxes(image<rgb> *im, float sigma, float c, int min_size,int *nums){
    int width = im->width();
    int height = im->height();

    int num_css;

    printf("Preprocessing...........\n");
    int* segIndices = segment_image_index(im, sigma, c, min_size, &num_css);   // height * width
    int* nn = track_region_neighbors(segIndices, height, width, num_css);      // num_css * num_css
    int* bb = get_region_bb(segIndices, height, width, num_css);               // num_css * 4
    int* sizes = get_region_size(segIndices, height, width, num_css);          // num_css
    colour_hist* chs =  get_region_colour_hist(im, segIndices, num_css);       // num_css
    texture_hist* ths = get_region_texture_hist(im, segIndices, num_css);      // num_css

    printf("Save the segments .........\n");
	image<rgb> *output = new image<rgb>(width, height);
	char s[200];
	rgb* colors = new rgb[num_css];
	for(int i = 0; i < num_css; i++){
		colors[i]= random_rgb();
	}

	// Save the coloured image regions
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			imRef(output, x, y) = colors[segIndices[y*width+x]];
	    }
	}
	sprintf(s,"results/regions.PPM");
	savePPM(output,s);

	// Save the components
	for(int i = 0; i < num_css; i++){
	    for(int y = 0; y < height; y++) {
	        for(int x = 0; x < width; x++) {
	        	if(segIndices[y*width+x] == i+1){
	        		imRef(output,x,y) = imRef(im,x,y);
	      	    }
	      	    else{
	      	    	imRef(output,x,y).r = (uchar)0;
	      	    	imRef(output,x,y).g = (uchar)0;
	      	    	imRef(output,x,y).b = (uchar)0;
	      	    }
	      	}
	    }
	    sprintf(s,"results/region_%d.PPM",i);
	    savePPM(output,s);
	}



    int region_nums = 2 * num_css - 1;
    *nums = region_nums;
    region* R = new region[region_nums];
    int* NN = new int[region_nums*region_nums];
    double* region_sim = new double[region_nums*region_nums];

    // Initialize the regions
    int im_size = height * width;
    for(int i = 0; i < region_nums; i++)
    {
        R[i].im_size = im_size;
        if( i < num_css){
            R[i].size = sizes[i];
            R[i].bb[0] = bb[4*i+0];
            R[i].bb[1] = bb[4*i+1];
            R[i].bb[2] = bb[4*i+2];
            R[i].bb[3] = bb[4*i+3];
            R[i].c = chs[i];
            R[i].c/sizes[i];
            R[i].t = ths[i];
            R[i].t/sizes[i];
        }
        else{
            R[i].size = 0;
            R[i].bb[0] = 0;
            R[i].bb[1] = 0;
            R[i].bb[2] = 0;
            R[i].bb[3] = 0;
            for(int j = 0; j < COLOUR_BINS; j++){
            	R[i].c.r[j] = 0.0;
            	R[i].c.g[j] = 0.0;
            	R[i].c.b[j] = 0.0;
            }
            for(int j = 0; j < TEXTURE_DIM; j++){
            	R[i].t.r[j] = 0.0;
            	R[i].t.g[j] = 0.0;
            	R[i].t.b[j] = 0.0;
            }
        }
    }

    // copy the values in nn to NN (expand num_css * num_css to region_nums * region_nums)
    for(int i = 0; i < region_nums; i++)
    {
        for(int j = i; j < region_nums; j++){
        	if( i < num_css && j < num_css){
        		NN[i*region_nums+j] = nn[i*num_css+j];
        		NN[j*region_nums+i] = nn[j*num_css+i];
        	}
        	else{
        		NN[i*region_nums+j] = 0;
        		NN[j*region_nums+i] = 0;
        	}
        }
    }





    // calculate the similarities between neighbor regions
    for(int i = 0; i < region_nums; i++)
    {
        for(int j = i; j < region_nums; j++){
            if(NN[i*region_nums+j] == 1){
                region_sim[i*region_nums+j] = R[i].ssSim(&R[j]);
                region_sim[j*region_nums+i] = region_sim[i*region_nums+j];
            }
            else{
                region_sim[i*region_nums+j] = 0.0;
                region_sim[j*region_nums+i] = 0.0;
            }
        }
    }


    delete[] nn;
    delete[] bb;
    delete[] sizes;

    printf("Start to merge the segments ........\n");
    for(int k = num_css; k < region_nums; k++)
    {
        // find the pairs (R[i],R[j])with max similarity
        double max_sim = 0;
        int sim_index_i = 0;
        int sim_index_j = 0;
        for(int i = 0; i < region_nums; i++){
            for(int j = i+1; j < region_nums; j++){
                int idx = i * region_nums + j;
                if(region_sim[idx] > max_sim){
                    max_sim = region_sim[idx];
                    sim_index_i = i;
                    sim_index_j = j;
                }
            }
        }

        printf("[%4d,(%4d,%4d;%4d,%4d):%6d] [%4d,(%4d,%4d;%4d,%4d),%6d]--->(%4d,%.2lf).\n",
        		sim_index_i,R[sim_index_i].bb[0],R[sim_index_i].bb[1],R[sim_index_i].bb[2],R[sim_index_i].bb[3],R[sim_index_i].size,
        		sim_index_j,R[sim_index_j].bb[0],R[sim_index_j].bb[1],R[sim_index_j].bb[2],R[sim_index_j].bb[3],R[sim_index_j].size,
        		k,region_sim[sim_index_i*region_nums+sim_index_j]);

        // merge these two regions
        R[k].mergeRegion(&R[sim_index_i],&R[sim_index_j]);

	    for(int y = 0; y < height; y++) {
	        for(int x = 0; x < width; x++) {
	        	if((segIndices[y*width+x] == sim_index_i+1) ||(segIndices[y*width+x] == sim_index_j+1)){
	      	  	    imRef(output,x,y) = imRef(im,x,y);
	      	  	    segIndices[y*width+x] = k + 1;
	      	    }
	      	    else{
	      	  	    imRef(output,x,y).r = (uchar)0;
	      	    	imRef(output,x,y).g = (uchar)0;
	      	    	imRef(output,x,y).b = (uchar)0;
	      	    }
	        }
	    }
	    sprintf(s,"results/region_%d.PPM",k);
	    savePPM(output,s);



        // update the neighbors of new Region and the similarity matrix
        for(int i = 0; i < region_nums; i++){
            if(!(i == sim_index_i || i == sim_index_j)&&((NN[sim_index_i* region_nums+i] == 1) || (NN[sim_index_j* region_nums+i] == 1))){
            	NN[k* region_nums+i] = 1;
                NN[i* region_nums+k] = 1;
                region_sim[k*region_nums+i] = R[k].ssSim(&R[i]);
                region_sim[i*region_nums+k] = region_sim[k*region_nums+i];
            }
        }


        for(int i = 0; i < region_nums; i++){
            NN[sim_index_i*region_nums+i] = 0;
            NN[i*region_nums+sim_index_i] = 0;
            region_sim[sim_index_i*region_nums+i] = 0;
            region_sim[i*region_nums+sim_index_i] = 0;

            NN[sim_index_j*region_nums+i] = 0;
            NN[i*region_nums+sim_index_j] = 0;
            region_sim[sim_index_j*region_nums+i] = 0;
            region_sim[i*region_nums+sim_index_j] = 0;
        }

    }

    int* BB = new int[region_nums*4];

    for(int i = 0; i < region_nums; i++){
        BB[4*i+0] = R[i].bb[0];
        BB[4*i+1] = R[i].bb[1];
        BB[4*i+2] = R[i].bb[2];
        BB[4*i+3] = R[i].bb[3];
    }

    delete output;
    delete segIndices;
    delete[] R;
    delete[] NN;
    delete[] region_sim;

    return BB;
}

#endif
