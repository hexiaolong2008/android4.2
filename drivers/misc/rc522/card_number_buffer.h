#ifndef __CARD_NUMBER_BUFFER__H
#define __CARD_NUMBER_BUFFER__H

#define CARD_NUMBER_BUF_LEN  64
#define CARD_NUMBER_LEN      4

int isEmpty(void);
int isFull(void);
void putVal(unsigned char *val);
unsigned char *getVal(unsigned char *val);

#endif //__CARD_NUMBER_BUFFER_H
