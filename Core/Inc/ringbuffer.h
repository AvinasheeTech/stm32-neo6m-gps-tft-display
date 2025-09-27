/* @file  ringbuffer.h
   @brief header file to handle gps buffer basic functions
   @author Shyam Jha (Avinashee Tech)
*/


#ifndef INC_RINGBUFFER_H_
#define INC_RINGBUFFER_H_

#include <string.h>
#include <stdint.h>

#define Buffer_Length   512                 //size of circular buffer
#define upcoming_index(a,b)  (((a)+1)%b)    //helps find the upcoming index taking in account the buffer size

/*function declarations*/
void buffer_init(void);
uint8_t buffer_insert(char element);
uint8_t buffer_remove(char *element);
uint8_t buffer_peek(char *element);
uint8_t isbuffer_empty(void);
uint8_t isbuffer_full(void);
uint8_t isdata_available(void);


#endif /* INC_RINGBUFFER_H_ */
