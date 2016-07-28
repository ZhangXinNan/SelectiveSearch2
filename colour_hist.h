/*
 * Copyright @ Surge Wong -- 2014/08/26
*/


#ifndef COLOUR_HIST
#define COLOUR_HIST

#include<cstdlib>
#include<algorithm>
#include"utilities.h"

#define COLOUR_BINS 25

class colour_hist{
public:
    colour_hist(){
    	for(int i = 0; i < COLOUR_BINS;i++){
    		r[i] = 0.0;
    		g[i] = 0.0;
    		b[i] = 0.0;
    	}
    }

    colour_hist operator=(colour_hist* C){
        for(int i = 0; i < COLOUR_BINS; i++){
            this->r[i] = C->r[i];
            this->g[i] = C->g[i];
            this->b[i] = C->b[i];
        }

        return *this;
    }

    colour_hist operator*(double scale){
        for(int i = 0; i < COLOUR_BINS; i++){
            this->r[i] *= scale;
            this->g[i] *= scale;
            this->b[i] *= scale;
        }

        return *this;
    }

    colour_hist operator/(double num){
        if(!num)
            return *this;
        return (*this * (1 / num));
    }

    friend colour_hist operator+(colour_hist C1,colour_hist C2){
        colour_hist* nch = new colour_hist;    // a new colour_hist;
        for(int i = 0; i < COLOUR_BINS; i++){
            nch->r[i] = C1.r[i] + C2.r[i];
            nch->g[i] = C1.g[i] + C2.g[i];
            nch->b[i] = C1.b[i] + C2.b[i];
        }

        return *nch;
    }

    double ssSim(colour_hist* C){
        double sim = 0;
        for(int i = 0; i < COLOUR_BINS; i++){
            sim += min(this->r[i],C->r[i]);
            sim += min(this->g[i],C->g[i]);
            sim += min(this->b[i],C->b[i]);
        }

        return sim;
    }

public:
    double r[COLOUR_BINS];
    double g[COLOUR_BINS];
    double b[COLOUR_BINS];  
};

#endif
