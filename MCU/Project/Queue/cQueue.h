/****************************************************************
 * !\file cQueue.h
** \author SMFSW
** \copyright BSD 3-Clause License (c) 2017-2019, SMFSW
** \brief Queue handling library (designed in c on STM32)
** \details Queue handling library (designed in c on STM32)
**
**      https://github.com/SMFSW/cQueue
**
** My revision of library for MCU without dinamic memmory allocation
**
**/
/****************************************************************/
#ifndef __CQUEUE_H
	#define __CQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
/****************************************************************/


#define QUEUE_INITIALIZED	0x5AA5							//!< Queue initialized control value

#define Q_STATIC_INIT(name, type, size_rec, nb_recs, overwrite) \
    static uint8_t name##_buffer[(size_rec) * (nb_recs)]; \
    Queue_t name = { \
        .impl = type, \
        .ovw = overwrite, \
        .rec_nb = nb_recs, \
        .rec_sz = size_rec, \
        .queue_sz = (size_rec) * (nb_recs), \
        .queue = name##_buffer, \
        .in = 0, \
        .out = 0, \
        .cnt = 0, \
        .init = QUEUE_INITIALIZED, \
        .is_static = true \
    }

#define Q_STATIC_INIT_DEF(name, size_rec, nb_recs) \
    Q_STATIC_INIT(name, FIFO, size_rec, nb_recs, false)

/*!\enum enumQueueType
** \brief Queue behavior enumeration (FIFO, LIFO)
**/
typedef enum enumQueueType {
	FIFO = 0,	//!< First In First Out behavior
	LIFO = 1	//!< Last In First Out behavior
} QueueType;


/*!\struct Queue_t
** \brief Queue type structure holding all variables to handle the queue
**/
typedef struct Queue_t {
	QueueType	impl;		//!< Queue implementation: FIFO LIFO
	bool		ovw;		//!< Overwrite previous records when queue is full allowed
	uint16_t	rec_nb;		//!< number of records in the queue
	uint16_t	rec_sz;		//!< Size of a record
	uint32_t	queue_sz;	//!< Size of the full queue
	uint8_t *	queue;		//!< Queue start pointer (when allocated)

	uint16_t	in;			//!< number of records pushed into the queue
	uint16_t	out;		//!< number of records pulled from the queue (only for FIFO)
	uint16_t	cnt;		//!< number of records not retrieved from the queue
	uint16_t	init;		//!< set to QUEUE_INITIALIZED after successful init of the queue and reset when killing queue
	bool        is_static;
} Queue_t;


/*!	\brief Queue initialization
**	\param [in,out] q - pointer of queue to handle
**	\param [in] size_rec - size of a record in the queue
**	\param [in] nb_recs - number of records in the queue
**	\param [in] type - Queue implementation type: FIFO, LIFO
**	\param [in] overwrite - Overwrite previous records when queue is full
**	\return NULL when allocation not possible, Queue tab address when successful
**/
void * __attribute__((nonnull)) q_init(Queue_t * const q, const uint16_t size_rec, const uint16_t nb_recs, const QueueType type, const bool overwrite);

/*!	\brief Queue destructor: release dynamically allocated queue
**	\param [in,out] q - pointer of queue to handle
**/

/*! \brief Initialize with static buffer */
void q_init_static(Queue_t * const q, uint8_t * const buffer,
                   const uint16_t size_rec, const uint16_t nb_recs,
                   const QueueType type, const bool overwrite);

void __attribute__((nonnull)) q_kill(Queue_t * const q);

/*!	\brief Flush queue, restarting from empty queue
**	\param [in,out] q - pointer of queue to handle
**/
void __attribute__((nonnull)) q_flush(Queue_t * const q);

/*!	\brief get initialization state of the queue
**	\param [in] q - pointer of queue to handle
**	\return Queue initialization status
**	\retval true if queue is allocated
**	\retval false is queue is not allocated
**/
inline bool __attribute__((nonnull, always_inline)) q_isInitialized(const Queue_t * const q) {
	return (q->init == QUEUE_INITIALIZED) ? true : false; }

/*!	\brief get emptiness state of the queue
**	\param [in] q - pointer of queue to handle
**	\return Queue emptiness status
**	\retval true if queue is empty
**	\retval false is not empty
**/
inline bool __attribute__((nonnull, always_inline)) q_isEmpty(const Queue_t * const q) {
	return (!q->cnt) ? true : false; }

/*!	\brief get fullness state of the queue
**	\param [in] q - pointer of queue to handle
**	\return Queue fullness status
**	\retval true if queue is full
**	\retval false is not full
**/
inline bool __attribute__((nonnull, always_inline)) q_isFull(const Queue_t * const q) {
	return (q->cnt == q->rec_nb) ? true : false; }

/*!	\brief get size of queue
**	\remark Size in bytes (like sizeof)
**	\param [in] q - pointer of queue to handle
**	\return Size of queue in bytes
**/
inline uint32_t __attribute__((nonnull, always_inline)) q_sizeof(const Queue_t * const q) {
	return q->queue_sz; }

/*!	\brief get number of records in the queue
**	\param [in] q - pointer of queue to handle
**	\return Number of records stored in the queue
**/
inline uint16_t __attribute__((nonnull, always_inline)) q_getCount(const Queue_t * const q) {
	return q->cnt; }

/*!	\brief get number of records left in the queue
**	\param [in] q - pointer of queue to handle
**	\return Number of records left in the queue
**/
inline uint16_t __attribute__((nonnull, always_inline)) q_getRemainingCount(const Queue_t * const q) {
	return q->rec_nb - q->cnt; }

/*!	\brief Push record to queue
**	\warning If using q_push, q_pop, q_peek, q_drop, q_peekItem and/or q_peekPrevious in both interrupts and main application,
**				you shall disable interrupts in main application when using these functions
**	\param [in,out] q - pointer of queue to handle
**	\param [in] record - pointer to record to be pushed into queue
**	\return Push status
**	\retval true if successfully pushed into queue
**	\retval false if queue is full
**/
bool __attribute__((nonnull)) q_push(Queue_t * const q, const void * const record);

/*!	\brief Pop record from queue
**	\warning If using q_push, q_pop, q_peek, q_drop, q_peekItem and/or q_peekPrevious in both interrupts and main application,
**				you shall disable interrupts in main application when using these functions
**	\param [in] q - pointer of queue to handle
**	\param [in,out] record - pointer to record to be popped from queue
**	\return Pop status
**	\retval true if successfully pulled from queue
**	\retval false if queue is empty
**/
bool __attribute__((nonnull)) q_pop(Queue_t * const q, void * const record);

/*!	\brief Peek record from queue
**	\warning If using q_push, q_pop, q_peek, q_drop, q_peekItem and/or q_peekPrevious in both interrupts and main application,
**				you shall disable interrupts in main application when using these functions
**	\note This function is most likely to be used in conjunction with q_drop
**	\param [in] q - pointer of queue to handle
**	\param [in,out] record - pointer to record to be peeked from queue
**	\return Peek status
**	\retval true if successfully peeked from queue
**	\retval false if queue is empty
**/
bool __attribute__((nonnull)) q_peek(const Queue_t * const q, void * const record);

/*!	\brief Drop current record from queue
**	\warning If using q_push, q_pop, q_peek, q_drop, q_peekItem and/or q_peekPrevious in both interrupts and main application,
**				you shall disable interrupts in main application when using these functions
**	\note This function is most likely to be used in conjunction with q_peek
**	\param [in,out] q - pointer of queue to handle
**	\return drop status
**	\retval true if successfully dropped from queue
**	\retval false if queue is empty
**/
bool __attribute__((nonnull)) q_drop(Queue_t * const q);

/*!	\brief Peek record at index from queue
**	\warning If using q_push, q_pop, q_peek, q_drop, q_peekItem and/or q_peekPrevious in both interrupts and main application,
**				you shall disable interrupts in main application when using these functions
**	\note This function is only useful if searching for a duplicate record and shouldn't be used in conjunction with q_drop
**	\param [in] q - pointer of queue to handle
**	\param [in,out] record - pointer to record to be peeked from queue
**	\param [in] idx - index of the record to pick
**	\return Peek status
**	\retval true if successfully peeked from queue
**	\retval false if index is out of range
**/
bool __attribute__((nonnull)) q_peekIdx(const Queue_t * const q, void * const record, const uint16_t idx);

/*!	\brief Peek previous record from queue
**	\warning If using q_push, q_pop, q_peek, q_drop, q_peekItem and/or q_peekPrevious in both interrupts and main application,
**				you shall disable interrupts in main application when using these functions
**	\note This inline is only useful with FIFO implementation, use q_peek instead with a LIFO (will lead to the same result)
**	\param [in] q - pointer of queue to handle
**	\param [in,out] record - pointer to record to be peeked from queue
**	\return Peek status
**	\retval true if successfully peeked from queue
**	\retval false if queue is empty
**/
inline bool __attribute__((nonnull, always_inline)) q_peekPrevious(const Queue_t * const q, void * const record) {
	const uint16_t idx = q_getCount(q) - 1;	// No worry about count - 1 when queue is empty, test is done by q_peekIdx
	return q_peekIdx(q, record, idx); }


/****************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* __CQUEUE_H */
/****************************************************************/
