
#include "paintshop_driver.h"

typedef struct paintcan paint_can;
typedef struct semaphore _semaphore;

int remaining_customers;
int wait_time;

/* buffers */
void *shipment_buffer[NCUSTOMERS];
paint_can *order_buffer[NCUSTOMERS];

/* binary semaphores for accessing critical region */
_semaphore *access_orders;
_semaphore *access_shipments;
_semaphore *access_tints;
_semaphore *stuff_exit;

/* counting semaphores for controlling access to full or empty buffer */
_semaphore *full_shipment;
_semaphore *full_order;
_semaphore *empty_shipment;
_semaphore *empty_order;