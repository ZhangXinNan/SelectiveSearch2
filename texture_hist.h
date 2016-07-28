/*
 * Copyright @ Surge Wong -- 2014/08/26
*/


#ifndef TEXTURE_HIST
#define TEXTURE_HIST

#include<cstdlib>
#include<algorithm>
#include"utilities.h"

#define TEXTURE_BINS 10
#define DIRECTION_NUM 8
#define TEXTURE_DIM TEXTURE_BINS*DIRECTION_NUM


class texture_hist{
public:
    texture_hist(){
    	for(int i = 0; i < TEXTURE_DIM;i++){
    	    r[i] = 0.0;
    	    g[i] = 0.0;
    	    b[i] = 0.0;
    	}
    }

    texture_hist operator=(texture_hist* T){
        for(int i = 0; i < TEXTURE_DIM; i++){
            this->r[i] = T->r[i];
            this->g[i] = T->g[i];
            this->b[i] = T->b[i];
        }

        return *this;
    }

    texture_hist operator*(double scale){
        for(int i = 0; i < TEXTURE_DIM; i++){
            this->r[i] *= scale;
            this->g[i] *= scale;
            this->b[i] *= scale;
        }

        return *this;
    }

    texture_hist operator/(double num){
        if(!num)
            return *this;
        return (*this * (1 / num));
    }

    friend texture_hist operator+(texture_hist T1, texture_hist T2){
        texture_hist* nth = new texture_hist;    // a new texture_hist;
        for(int i = 0; i < TEXTURE_DIM; i++){
            nth->r[i] = T1.r[i] + T2.r[i];
            nth->g[i] = T1.g[i] + T2.g[i];
            nth->b[i] = T1.b[i] + T2.b[i];
        }

        return *nth;
    }

    double ssSim(texture_hist* T){
        double sim = 0;
        for(int i = 0; i < TEXTURE_DIM; i++){
            sim += min(this->r[i],T->r[i]);
            sim += min(this->g[i],T->g[i]);
            sim += min(this->b[i],T->b[i]);
        }

        return sim;
    }

public:
    double r[TEXTURE_DIM];
    double g[TEXTURE_DIM];
    double b[TEXTURE_DIM];

};


#endif
