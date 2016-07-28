/*
 * Copyright @ Surge Wong -- 2014/08/25
*/


#ifndef REGIONS_PARAMETERS
#define REGIONS_PARAMETERS


#include "FelZenSegment/segment-image.h"
#include "colour_hist.h"
#include "texture_hist.h"

#include "FelZenSegment/pnmfile.h"
#include "anigaussm/anigauss.c"



/*
 * Segment an image
 *
 * Matlab Wrapper around the code of Felzenszwalb and Huttenlocher created
 * by Jasper Uijlings, 2012
 *
 * Returns a color image representing the segmentation.
 * JASPER: Random is replaced by just an index.
 *
 * im: image to segment.
 * sigma: to smooth the image.
 * c: constant for treshold function.
 * min_size: minimum component size (enforced by post-processing stage).
 * num_ccs: number of connected components in the segmentation.
 */
int *segment_image_index(image<rgb> *im, float sigma, float c, int min_size, int *num_ccs) {
	  int width = im->width();
	  int height = im->height();

	  image<float> *r = new image<float>(width, height);
	  image<float> *g = new image<float>(width, height);
	  image<float> *b = new image<float>(width, height);

	  // smooth each color channel
	  for (int y = 0; y < height; y++) {
	    for (int x = 0; x < width; x++) {
	      imRef(r, x, y) = imRef(im, x, y).r;
	      imRef(g, x, y) = imRef(im, x, y).g;
	      imRef(b, x, y) = imRef(im, x, y).b;
	    }
	  }
	  image<float> *smooth_r = smooth(r, sigma);
	  image<float> *smooth_g = smooth(g, sigma);
	  image<float> *smooth_b = smooth(b, sigma);
	  delete r;
	  delete g;
	  delete b;

	  // build graph
	  edge *edges = new edge[width*height*4];
	  int num = 0;
	  for (int y = 0; y < height; y++) {
	    for (int x = 0; x < width; x++) {
	      if (x < width-1) {
		edges[num].a = y * width + x;
		edges[num].b = y * width + (x+1);
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y);
		num++;
	      }

	      if (y < height-1) {
		edges[num].a = y * width + x;
		edges[num].b = (y+1) * width + x;
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x, y+1);
		num++;
	      }

	      if ((x < width-1) && (y < height-1)) {
		edges[num].a = y * width + x;
		edges[num].b = (y+1) * width + (x+1);
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y+1);
		num++;
	      }

	      if ((x < width-1) && (y > 0)) {
		edges[num].a = y * width + x;
		edges[num].b = (y-1) * width + (x+1);
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y-1);
		num++;
	      }
	    }
	  }
	  delete smooth_r;
	  delete smooth_g;
	  delete smooth_b;

	  // segment
	  universe *u = segment_graph(width*height, num, edges, c);

	  // post process small components
	  for (int i = 0; i < num; i++) {
	    int a = u->find(edges[i].a);
	    int b = u->find(edges[i].b);
	    if ((a != b) && ((u->size(a) < min_size) || (u->size(b) < min_size)))
	      u->join(a, b);
	  }
	  delete [] edges;
	  *num_ccs = u->num_sets();

    // assign the each region with an index colour
    int *colorindex = new int[width*height];
    for(int i = 0; i < width*height; i++)
        colorindex[i] = 0;

    int idx = 1;
    int* indexmap = new int[width * height];
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int comp = u->find(y * width + x);
            if(!(colorindex[comp])){
                colorindex[comp] = idx;
                idx = idx + 1;
            }
        indexmap[y*width+x] = colorindex[comp];
        }
    }

    delete [] colorindex;
    delete u;

    return indexmap;
}


/*
 * Tack neighbors of the regions
 *
 * Returns an array (num_css * num_css) to label the realationship of neighborhood.
 *
 * imdexmap: indexmap of image.
 * height: height of image.
 * width: width of image.
 * num_ccs: number of connected components in the segmentation.
 */
int* track_region_neighbors(int* indexmap, int height, int width, int num_css){
    int* nn = new int[num_css*num_css];
    for(int i = 0; i < num_css*num_css; i++){
    	nn[i] = 0;
    }

    int vcurr, hcurr, curr;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            curr = indexmap[y * width + x];           // current pixel's colour index

            if(x > 0){
            	hcurr = indexmap[y * width + x-1];  // previous horizontal pixel's colour index
            	nn[(hcurr-1)*num_css+curr-1] = 1;
            	nn[(curr-1)*num_css+hcurr-1] = 1;
            }

            if(y > 0){
            	vcurr = indexmap[(y-1)*width+x];	// previous vertical pixel's colour index
                nn[(vcurr-1)*num_css+curr-1] = 1;
                nn[(curr-1)*num_css+vcurr-1] = 1;
            }
        }
    }

    for(int i = 0; i < num_css; i++){
    	nn[i*num_css+i] = 0;
    }

    return nn;
}


/*
 * Get the minimum and maximum of region -- bounding box
 *
 * Returns an array (num_css * 4) for four parameters of bounding box
 *
 * imdexmap: indexmap of image.
 * height: height of image.
 * width: width of image.
 * num_ccs: number of connected components in the segmentation.
 */
int* get_region_bb(int* indexmap, int height, int width, int num_css)
{
    int* bb = new int[num_css*4];

    for(int i = 0; i < num_css; i++){
        bb[4*i+0] = width;  	 // min x
        bb[4*i+1] = height; 	 // min y
        bb[4*i+2] = -1;         // max x
        bb[4*i+3] = -1;         // max y

    }

    int mcurr;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            mcurr = indexmap[y*width+x] - 1;
            if(bb[4*mcurr+0] > x){
                bb[4*mcurr+0] = x;
            }
            if(bb[4*mcurr+1] > y){
                bb[4*mcurr+1] = y;
            }
            if(bb[4*mcurr+2] < x){
                bb[4*mcurr+2] = x;
            }
            if(bb[4*mcurr+3] < y){
                bb[4*mcurr+3] = y;
            }
        }
    }

    return bb;
}


/*
 * Get the number of pixels in each region
 *
 * Returns an array (num_css) for size of region
 *
 * imdexmap: indexmap of image.
 * height: height of image.
 * width: width of image.
 * num_ccs: number of connected components in the segmentation.
 */
int* get_region_size(int* indexmap, int height, int width, int num_css)
{
    int* sizes = new int[num_css];
    for(int i = 0; i < num_css;i++){
    	sizes[i] = 0;
    }

    int mcurr;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            mcurr = indexmap[y*width+x] - 1;
            sizes[mcurr]++;
        }
    }

    return sizes;
}


/*
 * Get the colour histogram of each region
 *
 * Returns an array (num_css) for colour histogram of region
 *
 * im: original image.
 * imdexmap: indexmap of image.
 * num_ccs: number of connected components in the segmentation.
 */
colour_hist* get_region_colour_hist(image<rgb> *im, int* indexmap, int num_css)
{
    colour_hist* chs = new colour_hist[num_css];
    for(int i = 0; i < num_css; i++){
    	for(int j = 0; j < COLOUR_BINS;j++){
    	    chs[i].r[j] = 0.0;
    	    chs[i].g[j] = 0.0;
    	    chs[i].b[j] = 0.0;
    	}
    }

    int width = im->width();
    int height = im->height();
    double step = 255.0/COLOUR_BINS + 0.005;    // divide the colour value into COLOUR_BINS intervals

    int mcurr;
    int num;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            mcurr = indexmap[y*width+x] - 1;
            num = int(imRef(im, x, y).r / step);
            chs[mcurr].r[num]++;
            num = int(imRef(im, x, y).g / step);
            chs[mcurr].g[num]++;
            num = int(imRef(im, x, y).b / step);
            chs[mcurr].b[num]++;
        }
    }

    return chs;
}


/*
 * Get the texture histogram of each region
 *
 * Returns an array (num_css) for size of region
 *
 * im: original image.
 * imdexmap: indexmap of image.
 * num_ccs: number of connected components in the segmentation.
 */
texture_hist* get_region_texture_hist(image<rgb> *im, int* indexmap, int num_css)
{
    texture_hist* ths = new texture_hist[num_css];
    for(int i = 0; i < num_css; i++){
        for(int j = 0; j < TEXTURE_DIM;j++){
            ths[i].r[j] = 0.0;
        	ths[i].g[j] = 0.0;
            ths[i].b[j] = 0.0;
        }
    }


    int width = im->width();
    int height = im->height();

    image<float> *r = new image<float>(width, height);
    image<float> *g = new image<float>(width, height);
    image<float> *b = new image<float>(width, height);

    // gaussian derivative
    for (int y = 0; y < height; y++) {
    	for (int x = 0; x < width; x++) {
    		imRef(r, x, y) = imRef(im, x, y).r;
    		imRef(g, x, y) = imRef(im, x, y).g;
    		imRef(b, x, y) = imRef(im, x, y).b;
    	}
    }

    image<float> *r_gauss = new image<float>(width, height);
    image<float> *g_gauss = new image<float>(width, height);
    image<float> *b_gauss = new image<float>(width, height);

    int theta = 360/DIRECTION_NUM;				// the unit angel
    for(int i = 0; i< DIRECTION_NUM; i++){
    	 anigauss(r->data, r_gauss->data, width, height, 1, 1, i* theta, 1, 1);
    	 anigauss(g->data, g_gauss->data, width, height, 1, 1, i* theta, 1, 1);
    	 anigauss(b->data, b_gauss->data, width, height, 1, 1, i* theta, 1, 1);

    	 float r_max_value = -1.0e10;
    	 float r_min_value = 1.0e10;
    	 float g_max_value = -1.0e10;
    	 float g_min_value = 1.0e10;
    	 float b_max_value = -1.0e10;
    	 float b_min_value = 1.0e10;

    	 for(int y = 0; y < height; y++) {
    	     for(int x = 0; x < width; x++) {
    	    	 if(imRef(r_gauss,x,y) > r_max_value){			// r channel
    	    		 r_max_value = imRef(r_gauss,x,y);
    	    	 }
    	    	 if(imRef(r_gauss,x,y) < r_min_value){
    	    		 r_min_value = imRef(r_gauss,x,y);
    	    	 }

    	    	 if(imRef(g_gauss,x,y) > g_max_value){			// g channel
    	    		 g_max_value = imRef(g_gauss,x,y);
    	    	 }
    	    	 if(imRef(g_gauss,x,y) < g_min_value){
    	    		 g_min_value = imRef(g_gauss,x,y);
    	    	 }

    	    	 if(imRef(b_gauss,x,y) > b_max_value){			// b channel
    	    		 b_max_value = imRef(b_gauss,x,y);
    	    	 }
    	    	 if(imRef(b_gauss,x,y) < b_min_value){
    	    		 b_min_value = imRef(b_gauss,x,y);
    	    	 }
    	     }
    	 }

    	 float r_step = (r_max_value - r_min_value)/(TEXTURE_BINS-0.05);
    	 float g_step = (g_max_value - g_min_value)/(TEXTURE_BINS-0.05);
    	 float b_step = (b_max_value - b_min_value)/(TEXTURE_BINS-0.05);

    	 int mcurr;
    	 int num;
    	 for(int y = 0; y < height; y++){
    	     for(int x = 0; x < width; x++){
    	         mcurr = indexmap[y*width+x] - 1;
    	         num = int((imRef(r_gauss, x, y) - r_min_value) / r_step);
    	         ths[mcurr].r[DIRECTION_NUM* i + num]++;
    	         num = int((imRef(g_gauss, x, y) - g_min_value) / g_step);
    	         ths[mcurr].g[DIRECTION_NUM* i + num]++;
    	         num = int((imRef(b_gauss, x, y) - b_min_value) / b_step);
    	         ths[mcurr].b[DIRECTION_NUM* i + num]++;
    	    }
    	}
    }

    delete r_gauss;
    delete g_gauss;
    delete b_gauss;

    delete r;
    delete g;
    delete b;

    return ths;
}

#endif
