/*
 * Copyright @ Surge Wong -- 2014/08/25
*/


#ifndef REGION
#define REGION

#include "colour_hist.h"
#include "texture_hist.h"
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include"utilities.h"

class region{
public:
    region(){
        size = 0;

        bb[0] = 0;
        bb[1] = 0;
        bb[2] = 0;
        bb[3] = 0;

        subregion_a = NULL;
        subregion_b = NULL;
        im_size = 1;
    }

    region(int im_size){
        size = 0;

        bb[0] = 0;
        bb[1] = 0;
        bb[2] = 0;
        bb[3] = 0;

        subregion_a = NULL;
        subregion_b = NULL;
        this->im_size = im_size;
    }

    region operator=(region* R){
        this->size = R->size;
        for(int i = 0; i < 4; i++)
            this->bb[i] = R->bb[i];
        this->c = R->c;
        this->t = R->t;
        this->subregion_a = R->subregion_a;
        this->subregion_b = R->subregion_b;
        this->im_size = R->im_size;

        return *this;
    }

    region mergeRegion(region* R1, region* R2){
        region* nR = new region(R1->im_size);

        // size
        nR->size = R1->size + R2->size;

        // bounding box
        nR->bb[0] = min(R1->bb[0],R2->bb[0]);
        nR->bb[1] = min(R1->bb[1],R2->bb[1]);
        nR->bb[2] = max(R1->bb[2],R2->bb[2]);
        nR->bb[3] = max(R1->bb[3],R2->bb[3]);

        // colour_hist
        colour_hist* c1 = new colour_hist;
        colour_hist* c2 = new colour_hist;

        *c1 = R1->c;
        *c2 = R2->c;
        nR->c = (*c1 * R1->size + *c2 * R2->size)/(nR->size);

        
        delete c1;
        delete c2;


        // texture_hist
        texture_hist* t1 = new texture_hist;
        texture_hist* t2 = new texture_hist;

        *t1 = R1->t;
        *t2 = R2->t;
        nR->t = (*t1 * R1->size + *t2 * R2->size)/(nR->size);
        
        delete t1;
        delete t2;


        // subregion
        nR->subregion_a = R1;
        nR->subregion_b = R2;

        *this = *nR;
        delete nR;
        return *this;
    }

    double ssSim(region* R){
        double sim_colour = this->c.ssSim(&(R->c))/3.0;
        double sim_texture = this->t.ssSim(&(R->t))/(DIRECTION_NUM*3.0);
        double sim_size = this->im_size - this->size - R->size;
        sim_size /= this->im_size;
        int rect = (this->bb[2] - this->bb[0])*(this->bb[3] - this->bb[1]);
        double sim_fill = this->im_size - ( rect - this->size - R->size);
        sim_fill /= this->im_size;

        return sim_colour + sim_texture + sim_size + sim_fill;
    }

public:
    int size;
    int bb[4];
    colour_hist c;
    texture_hist t;
    region* subregion_a;
    region* subregion_b;
    int im_size;
};

#endif
