#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "paintshop_driver.h"


/*
 * DEFINE THIS MACRO TO SWITCH ON MORE PRINTING
 *
 * Note: Your solution should work whether printing is on or off
 *  
 */

/* #define PRINT_ON */

/* this semaphore is for cleaning up at the end. */
static struct semaphore *alldone;
struct semaphore *print_sem;

/*
 * Data type used to track number of doses each tint performs 
 */

struct paint_tint {
  int doses; 
};

struct paint_tint paint_tints[NCOLOURS];



/*
 * **********************************************************************
 * CUSTOMERS
 *
 * Customers are rather simple, they arrive with their paint can,
 * write their requested colour on the can, submit their can as an
 * order to the paint shop staff and wait. 
 *
 * Eventually their can returns with the requested contents (exactly
 * as requested), they paint until the can is empty, take a short
 * break, and do it all again until they have emptied the desired
 * number of cans.
 *
 */

void customer(void *unusedpointer, unsigned long customernum)
{
  struct paintcan can;
  int i,j;
  
  (void) unusedpointer; /* avoid compiler warning */
  
  
  i = 0; /* count number of interations */
  do {
    

#ifdef PRINT_ON
    kprintf("C %ld is ordering\n", customernum);
#endif

    /* erase ingredients list on can and select a colour in terms of tints */
    for (j = 0; j < PAINT_COMPLEXITY; j++) {
      can.requested_colours[j] = 0;
    }
    can.requested_colours[0] = RED;
    

    /* order the paint, this blocks until the order is forfilled */
    order_paint(&can);
    
    
#ifdef PRINT_ON
    kprintf("C %ld painting with the following %d, %d, %d\n", customernum,
	    can.contents[0],
	    can.contents[1],
	    can.contents[2]);
#endif

    /* empty the paint can */
    for (j = 0; j < PAINT_COMPLEXITY; j++) {
      can.contents[j] = 0;
    }
    
    
    /* I needed that break.... */
    thread_yield();
    
    i++;
  } while (i < 10); /* keep going until .... */ 

#ifdef PRINT_ON  
  kprintf("C %ld going home\n", customernum);
#else
  (void)customernum;
#endif
  
  /*
   * Now we go home. 
   */
  go_home();
  V(alldone);
}


/*
 * **********************************************************************
 * PAINT SHOP STAFF
 * paint shop staff are only slightly more complicated than the customers.
 * They take_orders, and if valid, they fill them and serve them.
 * When all the customers have left, the staff members go home.
 *
 * An invalid order signals that the staffer should go home.
 *
 */

void paintshop_staff(void *unusedpointer, unsigned long staff)
{
  
  void *o;
  int i;
  (void)unusedpointer; /* avoid compiler warning */

  i = 0; /* count orders filled for stats */
  while (1) {
    
#ifdef PRINT_ON
    kprintf("S %ld taking order\n", staff);
#endif
    
    o = take_order();
    
    if (o != NULL) {
      
#ifdef PRINT_ON
      kprintf("S %ld filling\n", staff);
#endif
      

      i++;
      fill_order(o);
      
#ifdef PRINT_ON
      kprintf("S %ld serving\n", staff);
#endif
      
      serve_order(o);
    }
    else {
      break;
    }
    
  };
  P(print_sem);
  kprintf("S %ld going home after mixing %d orders\n", staff, i);
  V(print_sem);
  V(alldone);
}


/*
 * **********************************************************************
 * RUN THE PAINT SHOP
 * This routine sets up the paint shop prior to opening and cleans up
 * after closing.
 *
 * It calls two routines (paintshop_open() and paintshop_close()) in
 * which you can insert your own initialisation code.
 *
 * It also prints some statistics at the end.
 *
 */

int runpaintshop(int nargs, char **args)
{
  int i, result;
  
  (void) nargs; /* avoid compiler warnings */
  (void) args;

  /* this semaphore indicates everybody has gone home */
  alldone = sem_create("alldone", 0);
  if (alldone==NULL) {
    panic("runpaintshop: out of memory\n");
  }
  print_sem=sem_create("print",1);
  /* initialise the tint doses to 0 */ 
  for (i =0 ; i < NCOLOURS; i++) {
    paint_tints[i].doses = 0;
  }

  /***********************************************************************
   * call your routine that initialises the rest of the paintshop 
   */
  paintshop_open();

  /* Start the paint shop staff */
  for (i=0; i<NPAINTSHOPSTAFF; i++) {
    result = thread_fork("paint shop staff thread", NULL,
			 paintshop_staff, NULL, i);
    if (result) {
      panic("runpaintshop: thread_fork failed: %s\n",
	    strerror(result));
    }
  }
  
  /* Start the customers */
  for (i=0; i<NCUSTOMERS; i++) {
    result = thread_fork("customer thread", NULL,
			 customer, NULL, i);
    if (result) {
      panic("runpaintshop: thread_fork failed: %s\n",
	    strerror(result));
    }
  }
  
  /* Wait for everybody to finish. */
  for (i=0; i< NCUSTOMERS+NPAINTSHOPSTAFF; i++) {
    P(alldone);
  }
  
  for (i =0 ; i < NCOLOURS; i++) {
    kprintf("Tint %d used for %d doses\n", i+1, paint_tints[i].doses);
  }
  
  /***********************************************************************
   * Call your paint shop clean up routine
   */
  paintshop_close();
  
  sem_destroy(alldone);
  kprintf("The paint shop is closed, bye!!!\n");
  return 0;
}



/*
 * **********************************************************************
 * MIX
 *
 * This function take a can with an tint list and mixing the
 * tints together as required. It does it such that the contents
 * EXACTLY matches the requested tints.
 *
 * MIX NEEDS THE ROUTINE THAT CALLS IT TO ENSURE THAT MIX HAS EXCLUSIVE 
 * ACCESS TO THE TINTS IT NEEDS.
 *
 * YOU MUST USE THIS MIX FUNCTION TO FILL PAINT CANS
 *
 */

void mix(struct paintcan *c)
{
  int i;

  /* add tints to can in order given and increment number of 
     doses from particular tint */

  for (i = 0; i < PAINT_COMPLEXITY;i++){
    int col;
    col = c->requested_colours[i];
    c->contents[i] = col;
    
    if (col > NCOLOURS) {
      panic("Unknown colour");
    }
    if (col > 0) {
      paint_tints[col-1].doses++;
    }
  }
}

  
  
