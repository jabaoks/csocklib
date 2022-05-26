/*
 * csocktrgt.h
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */

#ifndef CSOCKTRGT_H_
#define CSOCKTRGT_H_

/*
 * target dependent mutex implementation
 * */
void *cs_init(void);
void cs_lock(void *);
void cs_unlock(void *);
void cs_release(void *);

#endif /* CSOCKTRGT_H_ */
