//
//  lwios.m
//  laidoff
//
//  Created by 김거엽 on 2017. 4. 15..
//  Copyright © 2017년 KIMGEO YEOB. All rights reserved.
//
			
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <ImageIO/ImageIO.h>

#include "lwbitmapcontext.h"
#include "constants.h"
#include "lwmacro.h"

char * create_string_from_file(const char * filename) {
    
    NSString *pathname = [[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"assets/%s", filename] ofType:@""];
    
    NSString *str = [NSString stringWithContentsOfFile:pathname encoding:NSUTF8StringEncoding error:nil];
    
    return strdup((char *)[str UTF8String]);
}

void release_string(char * d) {
    free((void*)d);
}

char* create_binary_from_file(const char* filename, size_t* size) {
    
    NSString *pathname = [[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"assets/%s", filename] ofType:@""];
    
    NSData* data = [NSData dataWithContentsOfFile:pathname];

    *size = data.length;
    
    char* d = (char*)malloc(data.length);
    memcpy(d, [data bytes], data.length);

    return d;
}

void release_binary(char * d) {
    free(d);
}

void play_sound(enum LW_SOUND lws) {
    
}

void stop_sound(enum LW_SOUND lws) {
    
}

HRESULT init_ext_sound_lib() {
    return 0;
}

int request_get_today_played_count() {
    return 1;
}

int request_get_today_playing_limit_count() {
    return 1;
}

int request_get_highscore() {
    return 0;
}


CGImageRef CGImageRef_load(const char *filename) {
    NSString *pathname = [[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"assets/%s", filename] ofType:@""];
    
    UIImage *someImage = [UIImage imageWithContentsOfFile:pathname];
    
    return [someImage CGImage];
}

unsigned char* CGImageRef_data(CGImageRef image, int* w, int* h) {
    NSInteger width = CGImageGetWidth(image);
    NSInteger height = CGImageGetHeight(image);
    unsigned char *data = (unsigned char*)calloc(1, width*height*4);
    
    CGContextRef context = CGBitmapContextCreate(data,
                                                 width, height,
                                                 8, width * 4,
                                                 CGImageGetColorSpace(image),
                                                 kCGImageAlphaPremultipliedLast);
    
    CGContextDrawImage(context,
                       CGRectMake(0.0, 0.0, (float)width, (float)height),
                       image);
    CGContextRelease(context);
    
    *w = (int)width;
    *h = (int)height;
    
    return data;
}

const unsigned char* load_png_ios(const char* filename, LWBITMAPCONTEXT* pBitmapContext) {
    
    CGImageRef r = CGImageRef_load(filename);
    unsigned char* d = 0;
    if (r) {
        d = CGImageRef_data(r, &pBitmapContext->width, &pBitmapContext->height);
    }
    
    pBitmapContext->lock = 0;
    pBitmapContext->data = (char*)d;
    
    return d;
}

void unload_png_ios(LWBITMAPCONTEXT* pBitmapContext) {
    free(pBitmapContext->data);
    pBitmapContext->data = 0;
}
