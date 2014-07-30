/* --------------------------------------------------------------------
 *  Version control log:
 * $Log:   H:/VCS/FCIT/SWAP.H_V  $
 * 
 *    Rev 1.38   06 May 1991 17:29:50   FJM
 *
 *     Rev 1.15   06 Jan 1991 12:46:40   FJM
 *  Porting, formating, fixes and misc changes.
 *
 *     Rev 1.14   27 Dec 1990 20:17:20   FJM
 *  Changes for porting.
 *
 *     Rev 1.3   23 Nov 1990 13:25:42   FJM
 *  Header change
 *
 *     Rev 1.1   17 Nov 1990 16:12:34   FJM
 *  Added version control log header
 * -------------------------------------------------------------------- */

/* This conditional extern statement is required when compiling with
   Turbo C++
 */



#ifdef __cplusplus
extern "C" {
#endif

    int swap(unsigned char *program_name,
    unsigned char *command_line,
    unsigned char *exec_return,
    unsigned char *swap_fname);

#ifdef __cplusplus
}

#endif

int ems4_installed(void);
int xms_installed(void);

/* The return code from swap() will be one of the following.  Codes other    */
/* than SWAP_OK (0) indicate that an error occurred, and thus the program    */
/* has NOT been swapped, and the new program has NOT been executed.          */

#define SWAP_OK         (0)
#define SWAP_NO_SHRINK  (1)
#define SWAP_NO_SAVE    (2)
#define SWAP_NO_EXEC    (3)


/* If swap() returns 3, SWAP_NO_EXEC, the byte/char pointed to by the        */
/* parameter exec_return will be one of the following standard DOS error     */
/* codes, as specified in the DOS technical reference manuals.               */

#define BAD_FUNC        (0x01)
#define FILE_NOT_FOUND  (0x02)
#define ACCESS_DENIED   (0x05)
#define NO_MEMORY       (0x08)
#define BAD_ENVIRON     (0x0A)
#define BAD_FORMAT      (0x0B)
