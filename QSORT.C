	qsort(logTab, (unsigned) cfg->MAXLOGTAB,
    (unsigned) sizeof(*logTab), logSort);

}

/************************************************************************/
/*      logSort() Sorts 2 entries in logTab                             */
/************************************************************************/
int logSort(const void *p1, const void *p2)
/* struct lTable *s1, *s2;     */
{
    struct lTable *s1, *s2;

    s1 = (struct lTable *) p1;
    s2 = (struct lTable *) p2;
    if (s1->ltnmhash == 0 && (struct lTable *) (s2)->ltnmhash == 0)
        return 0;
    if (s1->ltnmhash == 0 && s2->ltnmhash != 0)
        return 1;
    if (s1->ltnmhash != 0 && s2->ltnmhash == 0)
        return -1;
    if (s1->ltcallno < s2->ltcallno)
        return 1;
    if (s1->ltcallno > s2->ltcallno)
        return -1;
    return 0;
}

