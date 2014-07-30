/* -------------------------------------------------------------------- */
/*  PROTO.H  -  Function Prototypes for Citadel.                        */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *
 *  History:
 *
 *  03/03/90    GREN    Add DRAGON.C, lets the user abuse routine work.
 *  03/07/90    {zm}    Add ismostlyupper() to EDIT.C
 *  03/13/90    FJM     Made oChar() take unsigned arg for filtering
 *                      Converted to use prototypes (was extern decl's)
 *  03/19/90    FJM     Linted & partial cleanup
 *  06/06/90    FJM     Changed strftime to cit_strftime
 *	09/03/91	BLJ 	Linted & more cleanup
 *
 * -------------------------------------------------------------------- */

/* CTDL.C */
void main(int argc, char *argv[]);

/* COMMAND.C */
unsigned char mAbort(void);
void doAd(int force);
void updatebalance(void);
void doReader(void);

/* CRON.C */
int do_cron(int why_called);
void cron_commands(void);
void did_net(char *callnode);
void readCrontab(void);
void readcron(void);
void writeCrontab(void);

/* DOCHAT.C */
void chat(void);
void ringSysop(void);

/* ACCOUNT.C */
int logincheck(void);
void calc_trans(long time1, long time2, char trans);
void clearthisAccount(void);
void logincrement(void);
void negotiate(void);
void newaccount(void);
void unlogthisAccount(void);

/* APLIC.C */
void readDoorFiles(int door);
void roomFmtRun(char *cmd);
void extFmtRun(char *cmd, char *file);
void ExeAplic(void);
void apsystem(char *stuff);
void shellescape(char super);

/* CONFIG.C */
void buildroom(void);
void configcit(void);
void illegal(char *errorstring);
void readaccount(void);
void readconfig(int cfg_ctdl);
void readprotocols(void);
void readchatbell(void);
void rules_read(void);
void showtypemsg(unsigned long here);

/* EDIT.C */
char *matchString(char *buf, char *pattern, char *bufEnd, char ver);
int editText(char *buf, int lim, uchar sound_in_message);
int ismostlyupper(char *s, int n);
unsigned char getText(void);
void fakeFullCase(char *text);
void putheader(void);
void replaceString(char *buf, int lim, char ver);
void wordcount(char *buf);
void xPutStr(FILE * fl, char *str);

/* FILES.C */
int ambig(char *filename);
int ambigUnlink(char *filename, char change);
int checkfilename(char *filename, char xtype);
int dumpf(char *filename);
void attributes(void);
void dir(char *filename, char verbose);
void download(char c);
void entertextfile(void);
void enterwc(void);
void filldirectory(char *filename, char verbose);
void goodbye(void);
void hello(void);
void nextblurb(char *name, int *count, int showhelp);
void readdirectory(char verbose);
void readtextfile(void);
void readwc(void);
void renamefile(void);
void tutorial(char *filename, int showhelp);
void unlinkfile(void);
void upload(char c);
void wcdown(char *filename);
void wcup(char *filename);
void wxrcv(char *path, char *file, char trans);
void wxsnd(char *path, char *file, char trans);

/* FORMAT.C */
int parse_it(char **words, char *input);
int qtext(char *buf, char *line, char end);
int strpos(int ch, char *str);
int u_match(char *s, char *p);
void normalizeString(char *s);
void sformat(char *str, char *fmt, char *val,...);

/* GRPHALL.C */
int accesshall(int slot);
int groupexists(char *groupname);
int groupseeshall(int hallslot);
int groupseesroom(int groupslot);
int hallexists(char *hallname);
int ingroup(int groupslot);
int iswindow(int roomslot);
int partialgroup(char *groupname);
int partialhall(char *hallname);
int roominhall(int roomslot);
void cleargroupgen(void);
void enterhall(void);
void getgroup(void);
void gotodefaulthall(void);
void knownhalls(void);
void readhalls(void);
void setgroupgen(void);
void stephall(int direction);

/* IDLE.C */
void do_idle(int idleflag);

/* INFO.C */
void entercomment(char *filename, char *uploader, char *comment);
void setfileinfo(void);

/* INFOFILE.C */
int batchinfo(int askuser);
void killinfo(char *filename);
void moveFile(void);
void readinfofile(char verbose);
void showinfo(char verbose);
void updateinfo(void);

/* INIT.C */
int readMsgTab(void);
void writeMsgTab(void);
void crashout(char *message);
void exitcitadel(void);
void initCitadel(void);
void asciitable(void);
int readTables(void);
void freeTables(void);
void allocateTables(void);
void writeTables(void);

/* INPUT.C */
int BBSCharReady(void);
int getYesNo(char *prompt, char dfault);
int iChar(void);
long getNumber(char *prompt, long bottom, long top, long dfaultnum);
void cGetString(char *buffer, int length, char *dfault, char *prompt);
void getASCString(char *prompt, char *buf, int lim, char QuestIsSpecial,
	char doEcho, char *dfault, char ASCIIOnly);
void getNormStr(char *prompt, char *s, int size, char doEcho);
void getString(char *prompt, char *buf, int lim, char QuestIsSpecial, char doEcho, char *dfault);
void setio(char whichio, char echo, char outflag);

/* LIBRARY.C */
void getGroup(void);
void getHall(void);
void getLog(struct logBuffer * lBuf, int n);
void getRoom(int rm);
void putGroup(void);
void putHall(void);
void putLog(struct logBuffer * lBuf, int n);
void putRoom(int rm);

/* LOG.C */
int findPerson(char *name, struct logBuffer * lbuf);
int personexists(char *name);
int pwexists(char *pw);
void displaypw(char *name, char *in, char *pw);
void normalizepw(char *InitPw, char *Initials, char *passWord, char *semicolon);
void setdefaultconfig(void);
void setlogconfig(void);
void setsysconfig(void);
void showconfig(struct logBuffer * lbuf);
void slideLTab(int slot);
void storeLog(void);

/* LOG2.C */
int pwslot(char *in, char *pw);
void Readlog(char verbose);
void forwardaddr(void);
void killuser(void);
void login(char *initials, char *password);
void minibin(void);
void newPW(void);
void setalloldrooms(void);
void setbackrooms(int num_visits);
void setlbvisit(void);
void setroomgen(void);
void showuser(void);
void terminate(char discon, char verbose);

/* LOG3.C */
void userEdit2(char *who);
int newlog(char *fullnm, char *in, char *pw);
int newslot(void);
void configure(unsigned char new);
void initroomgen(void);
void newUser(char *initials, char *password);
void newUserFile(void);
void userEdit(void);

/* MACHINE.C */
long bytesfree(void);
unsigned char getattr(char far * filename);
void setattr(char far * filename, unsigned char attr);
int cit_herror_handler(int errval,int ax,int bp,int si);

/* MISC.C */
int changedir(char *path);
unsigned char filexists(char *filename);
unsigned int hash(char *str);
void SaveAideMess(void);
void amPrintf(char *fmt,...);
void amZap(void);
void flipFileLan(char *path, int Up);
void changedisk(char disk);
void ctrl_c(void);
char *strip_ansi(char *msg);
void h2memcpy(char huge * to, char huge * from, long size);
void hmemcpy(void huge * xto, void huge * xfrom, long size);
void openFile(char *filename, FILE * *fd);
void trap(char *string, int what);
void GetStr(FILE * fl, char *str, int mlen);
void PutStr(FILE * fl, char *str);

/* MISC2.C */
void dial_out(void);
void logo(void);
void ringSystemREQ(void);
void systat(void);

/* MODEM.C */
int carrier(void);
int findbaud(void);
int getModStr(char *str);
unsigned char KBReady(void);
unsigned char checkCR(void);
void carrdetect(void);
void carrloss(void);
void doccr(void);
void domcr(void);
int fkey(void);
void offhook(void);
void outCon(char c);
void outstring(char *string);
void ringdetectbaud(void);
void verbosebaud(void);

/* MSG.C */
unsigned char putMessage(void);
void aideMessage(void);
void clearmsgbuf(void);
void getMessage(void);
void specialMessage(void);

/* MSG2.C */
int getMsgChar(void);
int indexslot(unsigned long msgno);
unsigned char dGetWord(char *dest, int lim);
unsigned char mayseeindexmsg(int slot);
unsigned char mayseemsg(void);
unsigned int sizetable(void);
void changeheader(unsigned long id, unsigned char roomno, unsigned char attr);
void copyindex(int dest, int source);
void crunchmsgTab(int howmany);
void dPrintf(char *fmt,...);
void getMsgStr(char *dest, int lim);
void indexmessage(unsigned long here);
void noteMessage(void);
void notelogmessage(char *name);
void overwrite(int bytes);
void putMsgChar(unsigned char c);
void putMsgStr(char *string);

/* MSG3.C */
unsigned char makeMessage(void);
unsigned char markIt(void);
unsigned char pullIt(void);
unsigned char stepMessage(unsigned long *at, int dir);
void copymessage(unsigned long id, unsigned char roomno);
void deleteMessage(void);
void insert(void);
void markmsg(void);
void printMessage(unsigned long id, char verbose);
void printheader(unsigned long id, char verbose, int slot);
void showMessages(char whichMess, char revOrder, char verbose);
void saveMessageText(char *filename);

/* MUSIC.C */
void sound_effect(void);
void play(void);
void submit_note(int note, float note_len, int tempo);
void init_sound(void);       /* Proto for init sound ISR */
void restore_sound(void);    /* Proto for restore sound */
void submit_sound(int freq,int delay); /* Proto for submit sound */


/* NET.C */
unsigned char alias(char *str);
unsigned char getnode(char *nodename);
unsigned char master(void);
unsigned char net_callout(char *node);
unsigned char net_slave(void);
unsigned char readnode(void);
unsigned char route(char *str);
unsigned char save_mail(void);
unsigned char slave(void);
void cleanup(void);

/* OUTPUT.C */
int getWord(char *dest, char *source, int offset, int lim);
void cCPrintf(char *fmt,...);
void cPrintf(char *fmt,...);
void doBS(void);
void doCR(void);
void doTAB(void);
void dospCR(void);
void echocharacter(char c);
void mFormat(char *string);
void mPrintf(char *fmt,...);
void mpPrintf(char *fmt,...);
void mtPrintf(char attr, char *fmt,...);
void oChar(unsigned char c);
void putWord(char *st);
void termCap(char c);
void updcrtpos(char c);

/* PORT.C */
int MIReady(void);
int MOReady(void);
int getMod(void);
void Mflush(void);
int gotCarrier(void);
int ringdetect(void);
void Hangup(void);
void Initport(void);
void baud(int baudrate);
void drop_dtr(void);
void outMod(unsigned char ch);
void portExit(void);
void portInit(void);

/* ROOMA.C */
int gotoRoom(char *roomname);
int roomExists(char *room);
void RoomStatus(void);
void givePrompt(void);
void indexRooms(void);
void listRooms(unsigned int what, char verbose, char numMess);
void noteRoom(void);
void roomtalley(void);
void stepRoom(int direction);
void unGotoRoom(void);

/* ROOMB.C */
int findRoom(void);
int renameRoom(void);
void formatSummary(char *buffer);
void killempties(void);
void killroom(void);
void makeRoom(void);
void massdelete(void);
void readbymsgno(void);
int directory_l(char *str);

/* SYSOP1.C */
int xhallfunc(unsigned char roomslot, unsigned char xyn, int fl);
void defaulthall(void);
void force(void);
void globalgroup(void);
void globalhall(void);
void globaluser(void);
void globalgroup2(char *groupname, unsigned char add);
void globaluser2(char *who, unsigned char add);
void globalverify(void);
void groupfunc(void);
void hallfunc(void);
void killgroup(void);
void killhall(void);
void listgroup(void);
void listhalls(void);
void newgroup(void);
void newhall(void);
void renamegroup(void);
void renamehall(void);
void sysopunlink(void);
void windowfunc(void);

/* TIMEDATE.C */
int dayofweek(void);
int hour(void);
int quarter(void);
int systimeout(long timer);
void changeDate(void);
void cit_strftime(char *outstr, int maxsize, char *formatstr, long tnow);
void diffstamp(long oldtime);
void getdstamp(char *buffer, unsigned int stamp);
void gettstamp(char *buffer, unsigned int stamp);
void pause(int ptime);
void set_date(void);
int chkdate();
int parse_tm_rules(char *rules, char day, int wday, char month, int year);

/* VERSION.C */

/* WINDOW.C */
char ansi(char c);
int gmode(void);
unsigned char screenheight(void);
unsigned char screenwidth(void);
void bioschar(char ch, unsigned char attr);
void biosstring(unsigned int row, char *str, unsigned char attr);
void clearline(unsigned int row, unsigned char attr);
void cls(void);
void connectcls(void);
void cursoff(void);
void curson(void);
void directchar(char ch, unsigned char attr);
void directstring(unsigned int row, char *str, unsigned char attr);
void help(void);
void position(unsigned char row, unsigned char column);
void readpos(unsigned char *row, unsigned char *column);
void restore_screen(void);
void save_screen(void);
void scroll(unsigned char row, unsigned char howmany, unsigned char attr);
void setscreen(void);
void update25(void);
void updatehelp(void);
void wscroll(int left, int top, int right, int bottom, int lines, unsigned char attr);

/* XMODEM.C */
unsigned int stringcrc(char *buff);
void rxfile(char *name);
void trans(char *in_name);

/* ZIPFILE.C */
char zipview(char *filename, char verbose);
void readzip(char verbose);

/* EOF */
