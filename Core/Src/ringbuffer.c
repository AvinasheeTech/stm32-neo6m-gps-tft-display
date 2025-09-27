/* @file  ringbuffer.c
   @brief source file to handle gps buffer basic functions
   @author Shyam Jha (Avinashee Tech)
*/


#include "ringbuffer.h"

/*Buffer Structure and Variables*/
typedef struct{
	char gps_buffer[Buffer_Length];
	int head_index;
	int tail_index;
}ring_buffer_t;

ring_buffer_t ring_buffer;
ring_buffer_t *ring_buffer_ptr = NULL;


/**
 * @brief  init buffer
 * @param  None
 * @retval None
 * @note   initializes head and tail index and resets buffer
 */
void buffer_init(void){
	ring_buffer_ptr = &ring_buffer;

    memset(ring_buffer_ptr->gps_buffer,0,Buffer_Length);
    ring_buffer_ptr->head_index = 0;
    ring_buffer_ptr->tail_index = 0;
}


/**
 * @brief  insert buffer data
 * @param  element data to be added to buffer
 * @retval return 1 if successfully added else return 0
 * @note   insert data in the buffer until head index falls behind tail index
 */
uint8_t buffer_insert(char element){

	int current_head = ring_buffer_ptr->head_index;
    int upcoming_head = upcoming_index(current_head,Buffer_Length);
    if(upcoming_head!=ring_buffer_ptr->tail_index){            //if next head not equivalent to tail
    	ring_buffer_ptr->gps_buffer[current_head] = element;   //add data
		ring_buffer_ptr->head_index = upcoming_head;           //update head
		return 1;
    }
    return 0;
}

/**
 * @brief  remove buffer data
 * @param  element pointer to data to be removed from buffer
 * @retval return 0 if buffer empty else return 1 for successfully removing element
 */
uint8_t buffer_remove(char *element){
    if(isbuffer_empty()){
    	return 0;
    }
    int current_tail = ring_buffer_ptr->tail_index;
    int upcoming_tail = upcoming_index(current_tail,Buffer_Length);
	*element = ring_buffer_ptr->gps_buffer[current_tail];
	ring_buffer_ptr->tail_index = upcoming_tail;

	return 1;

}

/**
 * @brief  peek buffer data
 * @param  element pointer to data to be looked in buffer
 * @retval return 0 if buffer empty else return 1 for finding data
 * @note   helper function to peek into the data at the tail
 *         index without removing it
 */
uint8_t buffer_peek(char *element){
	if(isbuffer_empty()){
		return 0;
	}
	int current_tail = ring_buffer_ptr->tail_index;
	*element = ring_buffer_ptr->gps_buffer[current_tail];

	return 1;
}

/**
 * @brief  check if buffer empty
 * @param  None
 * @retval return 1 if buffer empty else return 0
 */
uint8_t isbuffer_empty(void){
	return (ring_buffer_ptr->head_index==ring_buffer_ptr->tail_index);
}

/**
 * @brief  check if buffer full
 * @param  None
 * @retval return 1 if buffer full else return 0
 */
uint8_t isbuffer_full(void){
	return ((ring_buffer_ptr->head_index-ring_buffer_ptr->tail_index)==(Buffer_Length-1));
}

/**
 * @brief  check if buffer data is present
 * @param  None
 * @retval return 1 if buffer data present else return 0
 */
uint8_t isdata_available(void){
	return (((ring_buffer_ptr->head_index-ring_buffer_ptr->tail_index)>1)?1:0);
}
