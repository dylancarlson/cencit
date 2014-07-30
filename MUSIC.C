/*------------------------------------------------------------
* music.c
*
*  Å
*     Å   Centauri Enterprises           Copyright (c) 1991
*   Å
*
*  The Citadel Routines to play the music on the console
*------------------------------------------------------------
* History:
*
*  09/03/91 BLJ Moved soundsys.c functions in here
*------------------------------------------------------------
*/

#include <string.h>
#include "ctdl.h"

#include "proto.h"
#include "global.h"

/*--- CentWin ------------------------------------------------*/
/* Do the sound effect										  */
/*------------------------------------------------------------*/
void sound_effect()
{
	int i, l, pos;
	unsigned int tone, duration;
	char ch, work[20];

	l = strlen(Sound_Buffer);
	i = 0;

	if(debug)
		cPrintf("(Sound Effect)");

	while( i < l ) {
		switch(Sound_Buffer[i]) {
			case 'F' : /* Frequency */
				pos = 0;
                do {
					ch = Sound_Buffer[++i];
                    work[pos++] = ch;
                }while(isdigit(ch));
                work[pos] = 0;
				tone = atoi(work);
                break;
			case 'D' : /* Duration  */
				pos = 0;
                do {
					ch = Sound_Buffer[++i];
                    work[pos++] = ch;
                }while(isdigit(ch));
                work[pos] = 0;
				duration = atoi(work);
				submit_sound(tone,duration);
				break;
			default :
				i++;
				break;
		}
	}
	submit_sound(0,0);
}

/*--- CentWin ------------------------------------------------*/
/* Play the musical score									  */
/*------------------------------------------------------------*/
void play()
{
/*-----------------------------------------------------------
* This frequency table contains the 8 octaves of a piano keyboard
* including the sharps and flats
*  NOTE:  this does not have perfect pitch because we are currently
*		  using integers.
*------------------------------------------------------------
*/
/*		Octaves    0  1  2	 3	 4	 5	  6 */
int c_notes[] = { 33,66,131,262,523,1046,2093 };
int csnotes[] = { 35,69,139,277,554,1108,2217 };
int dfnotes[] = { 35,69,139,277,554,1108,2217 };
int d_notes[] = { 37,73,147,294,587,1175,2349 };
int dsnotes[] = { 39,78,156,311,622,1245,2489 };
int efnotes[] = { 39,78,156,311,622,1245,2489 };
int e_notes[] = { 41,82,165,330,659,1329,2637 };
int f_notes[] = { 44,87,175,349,698,1397,2794 };
int fsnotes[] = { 46,93,185,370,740,1480,2960 };
int gfnotes[] = { 46,93,185,370,740,1480,2960 };
int g_notes[] = { 49,98,196,392,784,1568,3136 };
int gsnotes[] = { 52,104,208,415,831,1661,3322 };
int afnotes[] = { 52,104,208,415,831,1661,3322 };
int a_notes[] = { 55,110,220,440,880,1760,3520 };
int asnotes[] = { 58,117,233,466,923,1865,3729 };
int bfnotes[] = { 58,117,233,466,923,1865,3729 };
int b_notes[] = { 62,123,247,494,988,1976,3951 };

	int have_note;
	unsigned char this_octave;
    unsigned char tempo;
    float note_len, this_note_len;


	int i, l, pos, last_i;
    int this_note;
    char ch, work[20], hdr[50];

	l = strlen(Sound_Buffer);
    have_note = 0;
	this_note = 0;
	this_octave = 4;
	tempo = 120;
	note_len = 1;

	i = last_i = 0;

	if(debug) cPrintf("(Play Sound)");

	while( i < l ) {
		switch(Sound_Buffer[i]) {
            case 'T' :  /* Set the tempo */
                pos = 0;
                do {
					ch = Sound_Buffer[++i];
                    work[pos++] = ch;
                }while(isdigit(ch));
                work[pos] = 0;
                tempo = atoi(work);
                break;

            case 'L' :  /* Set the length of the notes (default 1) */
                pos = 0;
                do {
					ch = Sound_Buffer[++i];
                    work[pos++] = ch;
                }while(isdigit(ch));
                work[pos] = 0;
                note_len = 1 / (float) atoi(work);
                this_note_len = note_len;
                break;

            case 'P' :  /* Set up a pause */
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
				this_note = 0;
                have_note = 1;
                i++;
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;

            case 'O' :  /* Set up the octave */
                i++;
				this_octave = Sound_Buffer[i]-'0';
                i++;
                if(this_octave < 0 || this_octave > 7) {
					mPrintf("Octave out of range..");
                    this_octave = 4;
                }
                break;

            case '<' :  /* drop our octave for the next note */
                this_octave--;
                i++;
                break;

            case '>' :  /* raise our octave for the next note */
                this_octave++;
                i++;
                break;

            case '.' :  /* increase this notes playing by 3/2 */
                this_note_len = (this_note_len * 3) / 2;
                i++;
                break;

            case 'A' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '#' || Sound_Buffer[i] == '+') {
                    this_note = asnotes[this_octave];
                    have_note = 1;
                    i++;
                }
				else if(Sound_Buffer[i] == '-') {
                    this_note = afnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = a_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;
            case 'B' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '-') {
                    this_note = bfnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = b_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;
            case 'C' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '#' || Sound_Buffer[i] == '+') {
                    this_note = csnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = c_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;
            case 'D' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '#' || Sound_Buffer[i] == '+') {
                    this_note = dsnotes[this_octave];
                    have_note = 1;
                    i++;
                }
				else if(Sound_Buffer[i] == '-') {
                    this_note = dfnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = d_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;
            case 'E' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '-') {
                    this_note = efnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = e_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;
            case 'F' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '#' || Sound_Buffer[i] == '+') {
                    this_note = fsnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = f_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;
            case 'G' :
                if(have_note) {
                    submit_note(this_note,this_note_len, tempo);
                    this_note_len = note_len;
                }
                i++;
				if(Sound_Buffer[i] == '#' || Sound_Buffer[i] == '+') {
                    this_note = gsnotes[this_octave];
                    have_note = 1;
                    i++;
                }
				else if(Sound_Buffer[i] == '-') {
                    this_note = gfnotes[this_octave];
                    have_note = 1;
                    i++;
                }
                else {
                    this_note = g_notes[this_octave];
                    have_note = 1;
                }
				if(isdigit(Sound_Buffer[i])) {
                    pos = 0;
					ch = Sound_Buffer[i];
                    do {
                        work[pos++] = ch;
						ch = Sound_Buffer[++i];
                    }while(isdigit(ch));
                    work[pos] = 0;
                    this_note_len = 1 / (float) atoi(work);
                }
                break;

            case 'M' :  /* Music style - next char is FBNL or S */
                i += 2;
                break;

            case 14  :
                i = l;
                break;
            case 27  :
            case '[' :
            case ' ' :  /* a space for readbility - ignore     */
            default  :  /* same thing */
				ch = Sound_Buffer[i];
				if(debug) {
					if(ch != '"' && ch != ' ' && ch != '\n' &&
						ch != 27 && ch != '[') {
						cPrintf(hdr,"Skipping \"%c\" at %d...",ch, i);
					}
                }
                i++;
                break;
        }
        if(i == last_i) {
            i++;
			cPrintf("Adjusting..\n");
        }
        last_i = i;
    }
    if(have_note) {
        submit_note(this_note,this_note_len, tempo);
        this_note_len = note_len;
    }
	submit_sound(0,0);
}

void submit_note(int note, float note_len, int tempo)
{
	int duration, pause;

	if(tempo == 0) tempo = 120;

	duration = (((float) 2000 * (float) 120) / (float) tempo) * note_len;
    pause = (duration * 1) / 8;
    duration = (duration * 7) / 8;

	if(debug)
		cPrintf("\nNote %d - D %3d  Len %f",note, duration, note_len);

	submit_sound(note, duration);
	submit_sound(0, pause);
}

void init_sound(void)
{

}

/* -------------------------------------------- */
/* Call this routine before returning to DOS... */
/* -------------------------------------------- */

void restore_sound(void)
{
	nosound();						/* Turn off sound		   */
}

/* --------------------------------------------------------- */
/* This routine is used to submit sounds to the sound queue. */
/* ----------------------------------------------------------*/

void submit_sound(int freq,int dly)
{
	if(!cfg->Sound) freq = 0;
	sound(freq);
	delay(dly);
	nosound();
}

/* EOF */
