/* Find a string within another string     */
/* and report it's position                */

int index(char *str, char *sub,int occur)
{
	int length, sublen, i1=0, i2=0, pos=0, ok=0, occur_hold=1;

	length = strlen(str);
	sublen = strlen(sub);

	for(i1=0;i1<length;i1++) {
		if(str[i1]==sub[i2]) {
			ok = 1;
         pos = i1-i2;
			i2++;
		}
		else {
			ok = 0;
			i2 = 0;
		}
		if(i2==sublen) {
			if(ok) {
				if(occur>occur_hold) {
					occur_hold++;
					i2=0;
					ok=0;
				}
				else {
					if(ok) break;
				}
			}
		}
   }
	if(i2==sublen) {
		pos++;   /* Needed for PICK syntax understanding  */
		return(pos);
	}
	else return(0);
}

