/* -*- C++ -*-
 * 
 *  ONScripter_image.cpp - Image processing in ONScripter
 *
 *  Copyright (c) 2001-2015 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "pch.h"
#include "ONScripter.h"
#include <new>
#include "resize_image.h"
#include "Utils.h"
#ifdef USE_PARALLEL
#include "Parallel.h"
#elif defined(USE_OMP_PARALLEL)
#include <omp.h>
#endif

SDL_Surface *ONScripter::loadImage(char *filename, bool *has_alpha, int *location)
{
    if (!filename) return NULL;

    SDL_Surface *tmp = NULL;
    if (location) *location = BaseReader::ARCHIVE_TYPE_NONE;

    if (filename[0] == '>')
        tmp = createRectangleSurface(filename, has_alpha);
    else
        tmp = createSurfaceFromFile(filename, has_alpha, location);
    if (tmp == NULL) return NULL;

    SDL_Surface *ret;
    if((tmp->w * tmp->format->BytesPerPixel == tmp->pitch) &&
       (tmp->format->BitsPerPixel == image_surface->format->BitsPerPixel) && 
       (tmp->format->Rmask == image_surface->format->Rmask) &&
       (tmp->format->Gmask == image_surface->format->Gmask) &&
       (tmp->format->Bmask == image_surface->format->Bmask) &&
       (tmp->format->Amask == image_surface->format->Amask)){
        ret = tmp;
    }
    else{
        ret = SDL_ConvertSurface(tmp, image_surface->format, SDL_SWSURFACE);
        SDL_FreeSurface(tmp);
    }
    
    return ret;
}

SDL_Surface *ONScripter::createRectangleSurface(char *filename, bool *has_alpha)
{
    int c=1, w=0, h=0;
    while (filename[c] != 0x0a && filename[c] != 0x00){
        if (filename[c] >= '0' && filename[c] <= '9')
            w = w*10 + filename[c]-'0';
        if (filename[c] == ','){
            c++;
            break;
        }
        c++;
    }

    while (filename[c] != 0x0a && filename[c] != 0x00){
        if (filename[c] >= '0' && filename[c] <= '9')
            h = h*10 + filename[c]-'0';
        if (filename[c] == ','){
            c++;
            break;
        }
        c++;
    }
        
    while (filename[c] == ' ' || filename[c] == '\t') c++;
    int n=0, c2 = c;
    while(filename[c] == '#'){
        uchar3 col;
        readColor(&col, filename+c);
        n++;
        c += 7;
        while (filename[c] == ' ' || filename[c] == '\t') c++;
    }

    SDL_PixelFormat *fmt = image_surface->format;
    SDL_Surface *tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
                                            fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

    c = c2;
    for (int i=0 ; i<n ; i++){
        uchar3 col;
        readColor(&col, filename+c);
        c += 7;
        while (filename[c] == ' ' || filename[c] == '\t') c++;
        
        SDL_Rect rect;
        rect.x = w*i/n;
        rect.y = 0;
        rect.w = w*(i+1)/n - rect.x;
        if (i == n-1) rect.w = w - rect.x;
        rect.h = h;
        SDL_FillRect(tmp, &rect, SDL_MapRGBA( tmp->format, col[0], col[1], col[2], 0xff));
    }

    if (has_alpha) *has_alpha = false;
    
    return tmp;
}

SDL_Surface *ONScripter::createSurfaceFromFile(char *filename, bool *has_alpha, int *location)
{
	string s=string(filename);
    unsigned long length = script_h.cBR->getFileLength( filename );

    if (length == 0){

		utils::printError(" *** can't find file [%s] ***\n", filename);
        return NULL;
    }

    if (filelog_flag)
        script_h.findAndAddLog(script_h.log_info[ScriptHandler::FILE_LOG], filename, true);
	//utils::printInfo(" ... loading %s length %ld\n", filename, length );

    mean_size_of_loaded_images += length*6/5; // reserve 20% larger size
    num_loaded_images++;
    if (tmp_image_buf_length < mean_size_of_loaded_images/num_loaded_images){
        tmp_image_buf_length = mean_size_of_loaded_images/num_loaded_images;
        if (tmp_image_buf) delete[] tmp_image_buf;
        tmp_image_buf = NULL;
    }

    unsigned char *buffer = NULL;
    if (length > tmp_image_buf_length){
        buffer = new(std::nothrow) unsigned char[length];
        if (buffer == NULL){
			utils::printError("failed to load [%s] because file size [%lu] is too large.\n", filename, length);
            return NULL;
        }
    }
    else{
        if (!tmp_image_buf)
			tmp_image_buf = new unsigned char[tmp_image_buf_length];
        buffer = tmp_image_buf;
    }
        
    script_h.cBR->getFile(filename, buffer, location);
    char *ext = strrchr(filename, '.');
    SDL_RWops *src = SDL_RWFromMem(buffer, length);//SIG
	//string s(GAMEPATH);
	//s+=filename;
	//SDL_RWops *src=SDL_RWFromFile(s.data(),"rb");
    int is_png = IMG_isPNG(src);

    SDL_Surface *tmp = IMG_Load_RW(src, 0);
    if (!tmp && ext && (!strcmp(ext+1, "JPG") || !strcmp(ext+1, "jpg")))
	{
		utils::printError(" *** force-loading a JPG image [%s]\n", filename);
        tmp = IMG_LoadJPG_RW(src);
    }
	if (tmp && has_alpha) {
		if (tmp->format->Amask || is_png)
            *has_alpha = true;
        else
            *has_alpha = false;
    }
    SDL_RWclose(src);

    if (buffer != tmp_image_buf) delete[] buffer;

    if (!tmp)
		utils::printError(" *** can't load file [%s] %s ***\n", filename, IMG_GetError());

    return tmp;
}

// resize 32bit surface to 32bit surface
int ONScripter::resizeSurface( SDL_Surface *src, SDL_Surface *dst )
{
    SDL_LockSurface( dst );
    SDL_LockSurface( src );
    Uint32 *src_buffer = (Uint32 *)src->pixels;
    Uint32 *dst_buffer = (Uint32 *)dst->pixels;

    /* size of tmp_buffer must be larger than 16 bytes */
    size_t len = src->w * (src->h+1) * 4 + 4;
    if (resize_buffer_size < len){
        delete[] resize_buffer;
        resize_buffer = new unsigned char[len];
        resize_buffer_size = len;
    }
    resizeImage( (unsigned char*)dst_buffer, dst->w, dst->h, dst->w * 4,
                 (unsigned char*)src_buffer, src->w, src->h, src->w * 4,
                 4, resize_buffer, src->w * 4, false );

    SDL_UnlockSurface( src );
    SDL_UnlockSurface( dst );

    return 0;
}

#if defined(BPP16)
#define BLEND_PIXEL_MASK(){\
    Uint32 s1 = (*src1_buffer | *src1_buffer << 16) & 0x07e0f81f; \
    Uint32 s2 = (*src2_buffer | *src2_buffer << 16) & 0x07e0f81f; \
    Uint32 mask_rb = (s1 + ((s2-s1) * mask2 >> 5)) & 0x07e0f81f; \
    *dst_buffer = mask_rb | mask_rb >> 16; \
}
#else
#define BLEND_PIXEL_MASK(){\
    Uint32 temp = *src1_buffer & 0xff00ff;\
    Uint32 mask_rb = (((((*src2_buffer & 0xff00ff) - temp ) * mask2 ) >> 8 ) + temp ) & 0xff00ff;\
    temp = *src1_buffer & 0x00ff00;\
    Uint32 mask_g  = (((((*src2_buffer & 0x00ff00) - temp ) * mask2 ) >> 8 ) + temp ) & 0x00ff00;\
    *dst_buffer = mask_rb | mask_g;\
}
// Originally, the above looks like this.
//    mask1 = mask2 ^ 0xff;
//    Uint32 mask_rb = (((*src1_buffer & 0xff00ff) * mask1 +
//                       (*src2_buffer & 0xff00ff) * mask2) >> 8) & 0xff00ff;
//    Uint32 mask_g  = (((*src1_buffer & 0x00ff00) * mask1 +
//                       (*src2_buffer & 0x00ff00) * mask2) >> 8) & 0x00ff00;
#endif

// alphaBlend
// dst: accumulation_surface
// src1: effect_src_surface
// src2: effect_dst_surface
void ONScripter::alphaBlend( SDL_Surface *mask_surface,
                                  int trans_mode, Uint32 mask_value, SDL_Rect *clip )
{
    SDL_Rect rect = {0, 0, screen_width, screen_height};

    /* ---------------------------------------- */
    /* clipping */
    if ( clip ){
        if ( AnimationInfo::doClipping( &rect, clip ) ) return;
    }

    /* ---------------------------------------- */

    SDL_LockSurface( effect_src_surface );
    SDL_LockSurface( effect_dst_surface );
    SDL_LockSurface( accumulation_surface );
    if ( mask_surface ) SDL_LockSurface( mask_surface );
    
    SDL_PixelFormat *fmt = accumulation_surface->format;
    Uint32 lowest_mask;
    Uint8  lowest_loss;
    if (fmt->Rmask < fmt->Bmask){
        lowest_mask = fmt->Rmask; // ABGR8888
        lowest_loss = fmt->Rloss;
    }
    else{
        lowest_mask = fmt->Bmask; // ARGB8888 or RGB565
        lowest_loss = fmt->Bloss;
    }

    Uint32 overflow_mask;
    if ( trans_mode == ALPHA_BLEND_FADE_MASK )
        overflow_mask = 0xffffffff;
    else
        overflow_mask = ~lowest_mask;

    mask_value >>= lowest_loss;

    if ( (trans_mode == ALPHA_BLEND_FADE_MASK ||
          trans_mode == ALPHA_BLEND_CROSSFADE_MASK) && mask_surface ){
#ifdef USE_PARALLEL
		struct Data {
			ONSBuf *const stsrc1_buffer, *const stsrc2_buffer, *const stdst_buffer;
			int screen_width;
			SDL_Surface *mask_surface;
			SDL_Rect *rect;
			Uint32 mask_value, lowest_mask, overflow_mask;
		} data = { (ONSBuf *)effect_src_surface->pixels + effect_src_surface->w * rect.y + rect.x,
			(ONSBuf *)effect_dst_surface->pixels + effect_dst_surface->w * rect.y + rect.x,
			(ONSBuf *)accumulation_surface->pixels + accumulation_surface->w * rect.y + rect.x,
			screen_width, mask_surface, &rect, mask_value, lowest_mask, overflow_mask };
		parallel::For<Data> parafor;
		parafor.run([](int i, const Data *data) {
			const int &screen_width = data->screen_width;
			const SDL_Surface *mask_surface = data->mask_surface;
			const SDL_Rect &rect = *(data->rect);
			ONSBuf *src1_buffer = data->stsrc1_buffer + screen_width * i;
			ONSBuf *src2_buffer = data->stsrc2_buffer + screen_width * i;
			ONSBuf *dst_buffer = data->stdst_buffer + screen_width * i;
			const ONSBuf *mask_buffer = (ONSBuf *)mask_surface->pixels + mask_surface->w * ((rect.y + i) % mask_surface->h);


            int j2 = rect.x;
			for (int j = 0; j < rect.w; j++) {
				Uint32 mask2 = 0;
				Uint32 mask = *(mask_buffer + j2) & data->lowest_mask;
				if (data->mask_value > mask) {
					mask2 = data->mask_value - mask;
					if (mask2 & data->overflow_mask) mask2 = data->lowest_mask;
				}
				BLEND_PIXEL_MASK();
				src1_buffer++; src2_buffer++; dst_buffer++;

				if (j2 >= mask_surface->w) j2 = 0;
				else                       j2++;
			}
			src1_buffer += screen_width - rect.w;
			src2_buffer += screen_width - rect.w;
			dst_buffer += screen_width - rect.w;
		}, 0, rect.h, &data);
#else
#ifdef USE_OMP_PARALLEL
#pragma omp parallel for
#endif //USE_OMP_PARALLEL
		for (int i = 0; i < rect.h; i++) {
			ONSBuf *src1_buffer = (ONSBuf *)effect_src_surface->pixels + effect_src_surface->w * rect.y + rect.x + screen_width * i;
			ONSBuf *src2_buffer = (ONSBuf *)effect_dst_surface->pixels + effect_dst_surface->w * rect.y + rect.x + screen_width * i;
			ONSBuf *dst_buffer = (ONSBuf *)accumulation_surface->pixels + accumulation_surface->w * rect.y + rect.x + screen_width * i;
			const ONSBuf *mask_buffer = (ONSBuf *)mask_surface->pixels + mask_surface->w * ((rect.y + i) % mask_surface->h);

			int j2 = rect.x;
			for (int j = 0; j < rect.w; j++) {
                Uint32 mask2 = 0;
                Uint32 mask = *(mask_buffer + j2) & lowest_mask;
                if ( mask_value > mask ){
                    mask2 = mask_value - mask;
                    if ( mask2 & overflow_mask ) mask2 = lowest_mask;
                }
                BLEND_PIXEL_MASK();
                src1_buffer++; src2_buffer++; dst_buffer++;

                if (j2 >= mask_surface->w) j2 = 0;
                else                       j2++;
            }
            src1_buffer += screen_width - rect.w;
            src2_buffer += screen_width - rect.w;
            dst_buffer  += screen_width - rect.w;
        }
#endif //USE_PARALLEL
    }else{ // ALPHA_BLEND_CONST
        Uint32 mask2 = mask_value & lowest_mask;

#ifdef USE_PARALLEL
		struct Data {
			ONSBuf *const stsrc1_buffer, *const stsrc2_buffer, *const stdst_buffer;
			Uint32 mask2;
			int screen_width, rect_w;
		} data = { (ONSBuf *)effect_src_surface->pixels + effect_src_surface->w * rect.y + rect.x,
			(ONSBuf *)effect_dst_surface->pixels + effect_dst_surface->w * rect.y + rect.x,
			(ONSBuf *)accumulation_surface->pixels + accumulation_surface->w * rect.y + rect.x,
			mask2, screen_width, rect.w };
		parallel::For<Data> parafor;
		parafor.run([](int i, const Data *data) {
			const int &screen_width = data->screen_width;
			const int &rect_w = data->rect_w;
			ONSBuf *src1_buffer = data->stsrc1_buffer + screen_width * i;
			ONSBuf *src2_buffer = data->stsrc2_buffer + screen_width * i;
			ONSBuf *dst_buffer = data->stdst_buffer + screen_width * i;
			const Uint32 &mask2 = data->mask2;
			for (int j = rect_w; j != 0; j--) {
				BLEND_PIXEL_MASK();
				src1_buffer++; src2_buffer++; dst_buffer++;
			}
			src1_buffer += screen_width - rect_w;
			src2_buffer += screen_width - rect_w;
			dst_buffer += screen_width - rect_w;
		}, 0, rect.h, &data);
#else
#ifdef USE_OMP_PARALLEL
#pragma omp parallel for
#endif //USE_OMP_PARALLEL
		for (int i = 0; i < rect.h; i++) {
			ONSBuf *src1_buffer = (ONSBuf *)effect_src_surface->pixels + effect_src_surface->w * rect.y + rect.x + screen_width * i;
			ONSBuf *src2_buffer = (ONSBuf *)effect_dst_surface->pixels + effect_dst_surface->w * rect.y + rect.x + screen_width * i;
			ONSBuf *dst_buffer = (ONSBuf *)accumulation_surface->pixels + accumulation_surface->w * rect.y + rect.x + screen_width * i;
			for (int j = rect.w; j != 0; j--) {
                BLEND_PIXEL_MASK();
                src1_buffer++; src2_buffer++; dst_buffer++;
            }
            src1_buffer += screen_width - rect.w;
            src2_buffer += screen_width - rect.w;
            dst_buffer  += screen_width - rect.w;
        }
#endif //USE_PARALLEL
    }
    
    if ( mask_surface ) SDL_UnlockSurface( mask_surface );
    SDL_UnlockSurface( accumulation_surface );
    SDL_UnlockSurface( effect_dst_surface );
    SDL_UnlockSurface( effect_src_surface );
}

#if defined(BPP16)
#define BLEND_PIXEL_TEXT()\
{\
    Uint32 mask2 = *src_buffer >> 3;                                    \
    if (mask2 != 0){                                                    \
        Uint32 d1   = (*dst_buffer | *dst_buffer << 16) & 0x07e0f81f;   \
        Uint32 mask = (d1 + ((src_color-d1) * mask2 >> 5)) & 0x07e0f81f; \
        *dst_buffer = mask | mask >> 16;                                \
    }                                                                   \
}
#else
#define BLEND_PIXEL_TEXT()\
{\
    Uint32 mask2 = *src_buffer;                                     \
    if (mask2 == 255){\
    	*dst_buffer = src_color3;\
    }\
    else if (mask2 != 0){                                           \
        Uint32 mask1   = mask2 ^ 0xff;                              \
        Uint32 mask_rb = (((*dst_buffer & 0xff00ff) * mask1 +       \
                           src_color1 * mask2) >> 8) & 0xff00ff;    \
        Uint32 mask_g  = (((*dst_buffer & 0x00ff00) * mask1 +       \
                           src_color2 * mask2) >> 8) & 0x00ff00;    \
        *dst_buffer    = 0xff000000 | mask_rb | mask_g;             \
    }                                                               \
}
#endif        

// alphaBlendText
// dst: ONSBuf surface (accumulation_surface)
// src: 8bit surface (TTF_RenderGlyph_Shaded())
void ONScripter::alphaBlendText( SDL_Surface *dst_surface, SDL_Rect dst_rect,
                                 SDL_Surface *src_surface, SDL_Color &color, SDL_Rect *clip, bool rotate_flag )
{
    int x2=0, y2=0;
    SDL_Rect clipped_rect;

    /* ---------------------------------------- */
    /* 1st clipping */
    if ( clip ){
        if ( AnimationInfo::doClipping( &dst_rect, clip, &clipped_rect ) ) return;

        x2 += clipped_rect.x;
        y2 += clipped_rect.y;
    }

    /* ---------------------------------------- */
    /* 2nd clipping */
    SDL_Rect clip_rect = {0, 0, dst_surface->w, dst_surface->h};
    if ( AnimationInfo::doClipping( &dst_rect, &clip_rect, &clipped_rect ) ) return;
    
    x2 += clipped_rect.x;
    y2 += clipped_rect.y;

    /* ---------------------------------------- */

    SDL_LockSurface( dst_surface );
    SDL_LockSurface( src_surface );

    SDL_PixelFormat *fmt = accumulation_surface->format;
#if defined(BPP16)    
    Uint32 src_color = (((color.r >> fmt->Rloss) << fmt->Rshift) |
                        ((color.g >> fmt->Gloss) << fmt->Gshift) |
                        ((color.b >> fmt->Bloss) << fmt->Bshift));
    src_color = (src_color | src_color << 16) & 0x07e0f81f;
#else
    Uint32 src_color1 = (color.r << fmt->Rshift) | (color.b << fmt->Bshift);
    Uint32 src_color2 = (color.g << fmt->Gshift);
    Uint32 src_color3 = (0xff << fmt->Ashift) | src_color1 | src_color2;
#endif    

    ONSBuf *dst_buffer = (AnimationInfo::ONSBuf *)dst_surface->pixels + dst_surface->w * dst_rect.y + dst_rect.x;

    if (!rotate_flag){
        unsigned char *src_buffer = (unsigned char*)src_surface->pixels + src_surface->pitch * y2 + x2;
        for ( int i=0 ; i<dst_rect.h ; i++ ){
            for ( int j=dst_rect.w ; j!=0 ; j-- ){
                BLEND_PIXEL_TEXT();
                src_buffer++;
                dst_buffer++;
            }
            src_buffer += src_surface->pitch - dst_rect.w;
            dst_buffer += dst_surface->w - dst_rect.w;
        }
    }
    else{
        for ( int i=0 ; i<dst_rect.h ; i++ ){
            unsigned char *src_buffer = (unsigned char*)src_surface->pixels + src_surface->pitch*(src_surface->h - x2 - 1) + y2 + i;
            for ( int j=dst_rect.w ; j!=0 ; j-- ){
                BLEND_PIXEL_TEXT();
                src_buffer -= src_surface->pitch;
                dst_buffer++;
            }
            dst_buffer += dst_surface->w - dst_rect.w;
        }
    }
    
    SDL_UnlockSurface( src_surface );
    SDL_UnlockSurface( dst_surface );
}

void ONScripter::makeNegaSurface( SDL_Surface *surface, SDL_Rect &clip )
{
    SDL_LockSurface( surface );
    ONSBuf *buf = (ONSBuf *)surface->pixels + clip.y * surface->w + clip.x;

    ONSBuf mask = surface->format->Rmask | surface->format->Gmask | surface->format->Bmask;
    for ( int i=clip.y ; i<clip.y + clip.h ; i++ ){
        for ( int j=clip.x ; j<clip.x + clip.w ; j++ )
            *buf++ ^= mask;
        buf += surface->w - clip.w;
    }

    SDL_UnlockSurface( surface );
}

void ONScripter::makeMonochromeSurface( SDL_Surface *surface, SDL_Rect &clip )
{
    SDL_LockSurface( surface );
    ONSBuf *buf = (ONSBuf *)surface->pixels + clip.y * surface->w + clip.x, c;

    SDL_PixelFormat *fmt = surface->format;
    for ( int i=clip.y ; i<clip.y + clip.h ; i++ ){
        for ( int j=clip.x ; j<clip.x + clip.w ; j++ ){
            c = ((((*buf & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss) * 77 +
                 (((*buf & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss) * 151 +
                 (((*buf & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss) * 28 ) >> 8; 
            *buf++ = ((monocro_color_lut[c][0] >> fmt->Rloss) << surface->format->Rshift |
                      (monocro_color_lut[c][1] >> fmt->Gloss) << surface->format->Gshift |
                      (monocro_color_lut[c][2] >> fmt->Bloss) << surface->format->Bshift);
        }
        buf += surface->w - clip.w;
    }

    SDL_UnlockSurface( surface );
}

void ONScripter::refreshSurface( SDL_Surface *surface, SDL_Rect *clip_src, int refresh_mode)
{
    if (refresh_mode == REFRESH_NONE_MODE) return;

    SDL_Rect clip = {0, 0, surface->w, surface->h};
    if (clip_src) if ( AnimationInfo::doClipping( &clip, clip_src ) ) return;

    int i, top;
    SDL_BlitSurface( bg_info.image_surface, &clip, surface, &clip );
    
    if ( !all_sprite_hide_flag ){
        if ( z_order < 10 && refresh_mode & REFRESH_SAYA_MODE )
            top = 9;
        else
            top = z_order;
        for ( i=MAX_SPRITE_NUM-1 ; i>top ; i-- ){
            if ( sprite_info[i].image_surface && sprite_info[i].visible )
                drawTaggedSurface( surface, &sprite_info[i], clip );
        }
    }

    if ( !all_sprite_hide_flag ){
        for ( i=0 ; i<3 ; i++ ){
            if (human_order[2-i] >= 0 && tachi_info[human_order[2-i]].image_surface)
                drawTaggedSurface( surface, &tachi_info[human_order[2-i]], clip );
        }
    }

    if ( windowback_flag ){
        if ( nega_mode == 1 ) makeNegaSurface( surface, clip );
        if ( monocro_flag )   makeMonochromeSurface( surface, clip );
        if ( nega_mode == 2 ) makeNegaSurface( surface, clip );

        if (!all_sprite2_hide_flag){
            for ( i=MAX_SPRITE2_NUM-1 ; i>=0 ; i-- ){
                if ( sprite2_info[i].image_surface && sprite2_info[i].visible )
                    drawTaggedSurface( surface, &sprite2_info[i], clip );
            }
        }
    
        if (refresh_mode & REFRESH_SHADOW_MODE)
            shadowTextDisplay( surface, clip );
        if (refresh_mode & REFRESH_TEXT_MODE)
            text_info.blendOnSurface( surface, 0, 0, clip );
    }

    if ( !all_sprite_hide_flag ){
        if ( refresh_mode & REFRESH_SAYA_MODE )
            top = 10;
        else
            top = 0;
        for ( i=z_order ; i>=top ; i-- ){
            if ( sprite_info[i].image_surface && sprite_info[i].visible )
                drawTaggedSurface( surface, &sprite_info[i], clip );
        }
    }

    if ( !windowback_flag ){
        if (!all_sprite2_hide_flag){
            for ( i=MAX_SPRITE2_NUM-1 ; i>=0 ; i-- ){
                if ( sprite2_info[i].image_surface && sprite2_info[i].visible )
                    drawTaggedSurface( surface, &sprite2_info[i], clip );
            }
        }

        if ( nega_mode == 1 ) makeNegaSurface( surface, clip );
        if ( monocro_flag )   makeMonochromeSurface( surface, clip );
        if ( nega_mode == 2 ) makeNegaSurface( surface, clip );
    }
    
    if ( !( refresh_mode & REFRESH_SAYA_MODE ) ){
        for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
            if ( bar_info[i] )
                drawTaggedSurface( surface, bar_info[i], clip );
        }
        for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
            if ( prnum_info[i] )
                drawTaggedSurface( surface, prnum_info[i], clip );
        }
    }

    if ( !windowback_flag ){
        if (refresh_mode & REFRESH_SHADOW_MODE)
            shadowTextDisplay( surface, clip );
        if (refresh_mode & REFRESH_TEXT_MODE)
            text_info.blendOnSurface( surface, 0, 0, clip );
    }
    if ( refresh_mode & REFRESH_CURSOR_MODE && !textgosub_label ){
        if ( clickstr_state == CLICK_WAIT )
            drawTaggedSurface( surface, &cursor_info[0], clip );
        else if ( clickstr_state == CLICK_NEWPAGE )
            drawTaggedSurface( surface, &cursor_info[1], clip );
    }
	
    if (show_dialog_flag)

        drawTaggedSurface( surface, &dialog_info, clip );
    ButtonLink *bl = root_button_link.next;
    while( bl ){
        if (bl->show_flag > 0)
            drawTaggedSurface( surface, bl->anim[bl->show_flag-1], clip );
        bl = bl->next;
    }
	//if (rePaint)
	//SDL_UpdateTexture(texture,NULL,surface->pixels,surface->pitch);
	//SDL_RenderCopy(renderer,texture,NULL,NULL);
	//SDL_RenderPresent(renderer);
}

void ONScripter::refreshSprite( int sprite_no, bool active_flag, int cell_no,
                                SDL_Rect *check_src_rect, SDL_Rect *check_dst_rect )
{
    if ( sprite_info[sprite_no].image_surface && 
         ( sprite_info[ sprite_no ].visible != active_flag ||
           (cell_no >= 0 && sprite_info[ sprite_no ].current_cell != cell_no ) ||
           AnimationInfo::doClipping(check_src_rect, &sprite_info[ sprite_no ].pos) == 0 ||
           AnimationInfo::doClipping(check_dst_rect, &sprite_info[ sprite_no ].pos) == 0) )
    {
        if ( cell_no >= 0 )
            sprite_info[ sprite_no ].setCell(cell_no);

        sprite_info[ sprite_no ].visible = active_flag;

        dirty_rect.add( sprite_info[ sprite_no ].pos );
    }
}

void ONScripter::createBackground()
{
    bg_info.num_of_cells = 1;
    bg_info.trans_mode = AnimationInfo::TRANS_COPY;
    bg_info.pos.x = 0;
    bg_info.pos.y = 0;
    bg_info.allocImage( screen_width, screen_height, texture_format );

    if ( !strcmp( bg_info.file_name, "white" ) ){
        bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0xff;
    }
    else if ( !strcmp( bg_info.file_name, "black" ) ||
              !strcmp( bg_info.file_name, "*bgcpy" ) ){
        bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0x00;
    }
    else if ( bg_info.file_name[0] == '#' ){
            readColor( &bg_info.color, bg_info.file_name );
    }
    else{
        AnimationInfo anim;
        setStr( &anim.image_name, bg_info.file_name );
        parseTaggedString( &anim );
        anim.trans_mode = AnimationInfo::TRANS_COPY;
        setupAnimationInfo( &anim );

        bg_info.fill(0, 0, 0, 0xff);
        if (anim.image_surface){
            SDL_Rect src_rect = {0, 0, anim.image_surface->w, anim.image_surface->h};
            SDL_Rect dst_rect = {0, 0};
            if (screen_width >= anim.image_surface->w){
                dst_rect.x = (screen_width - anim.image_surface->w) / 2;
            }
            else{
                src_rect.x = (anim.image_surface->w - screen_width) / 2;
                src_rect.w = screen_width;
            }

            if (screen_height >= anim.image_surface->h){
                dst_rect.y = (screen_height - anim.image_surface->h) / 2;
            }
            else{
                src_rect.y = (anim.image_surface->h - screen_height) / 2;
                src_rect.h = screen_height;
            }
            bg_info.copySurface(anim.image_surface, &src_rect, &dst_rect);
        }
        return;
    }

    bg_info.fill(bg_info.color[0], bg_info.color[1], bg_info.color[2], 0xff);
}
