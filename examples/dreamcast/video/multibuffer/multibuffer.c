/*  KallistiOS ##version##

    multibuffer.c
    Copyright (C) 2023 Donald Haase

*/

/*
    This example is based off a combination of the libdream video mode and 
    the bfont examples. It draws out four distinct framebuffers then rotates 
    between them until stopped.

 */

#include <kos.h>

int main(int argc, char **argv) {
    int x, y, mb, o;

    /* Press all buttons to exit */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);


    /* Set the video mode */
    vid_set_mode(DM_640x480 | DM_MULTIBUFFER, PM_RGB565);
    
    /* Set our starting offset to one letter height away from the 
       top of the screen and two widths from the left */
    o = (640 * BFONT_HEIGHT) + (BFONT_THIN_WIDTH * 2);

    /* Cycle through each frame buffer populating it with different 
        patterns and text labelling it. */
    for(mb = 0; mb < vid_mode->fb_count; mb++) {
        
        for(y = 0; y < 480; y++)
            for(x = 0; x < 640; x++) {
                int c = (x ^ y) & 0xff;
                vram_s[y * 640 + x] = ( ((c >> 3) << 12)
                                      | ((c >> 2) << 5)
                                      | ((c >> 3) << mb)) & 0xffff;
            }
            
        /* Drawing the special symbols is a bit convoluted. First we'll draw some
           standard text as above. */
        bfont_set_encoding(BFONT_CODE_ISO8859_1);
        bfont_draw_str(vram_s + o, 640, 1, "This is FB  ");

        /* Then we set the mode to raw to draw the special character. */
        bfont_set_encoding(BFONT_CODE_RAW);
        /* Adjust the writing to start after "This is FB  " and draw the one char */
        bfont_draw_wide(vram_s + o + (BFONT_THIN_WIDTH * 11), 640, 1, BFONT_ABUTTON + (mb*BFONT_WIDE_WIDTH*BFONT_HEIGHT/8));
            
        vid_flip(-1);
    }

    printf("\n\nPress all buttons simultaneously to exit.\n");
    fflush(stdout);

    /* Now flip through each frame until stopped */
    while(1) {
        vid_flip(-1);
        timer_spin_sleep(2000);
    }

    return 0;
}
