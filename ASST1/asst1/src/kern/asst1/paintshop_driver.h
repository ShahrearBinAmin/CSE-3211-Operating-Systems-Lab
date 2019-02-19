/*
 * **********************************************************************
 *
 * Define function prototypes, types, and constants needed by both the
 * driver (paintshop_driver.c) and the code you need to write
 * (paintshop.c)
 *
 * YOU SHOULD NOT RELY ON ANY CHANGES YOU MAKE TO THIS FILE
 *
 * We will use our own version of this file for testing
 */



/*
 * Define the number of paint tinting colours available and their
 * symbolic constants.
 *
 */
#define BLUE    1
#define GREEN  2
#define YELLOW   3
#define MAGENTA 4
#define ORANGE   5
#define CYAN    6
#define BLACK 7
#define RED 8
#define WHITE 9
#define BROWN 10
#define NCOLOURS 10


/*
 * The maximum number of tints that can be mixed in a single can
 */
#define PAINT_COMPLEXITY 3

/*
 * The data type representing a paintcan
 */
struct paintcan {
  /* the tints requested for this paint */
  unsigned int requested_colours[PAINT_COMPLEXITY];
  /* the actual contents of the can */
  unsigned int contents[PAINT_COMPLEXITY];
};


/*
 * FUNCTION PROTOTYPES FOR THE FUNCTIONS YOU MUST WRITE
 *
 * YOU CANNOT MODIFY THESE PROTOTYPES
 *
 */

/* Customer functions */
extern void order_paint(struct paintcan *);
extern void go_home(void);


/* Paintshop staff functions */
extern void * take_order();
extern void fill_order(void *);
extern void serve_order(void *);


/* Paintshop  opening and closing functions */
extern void paintshop_open();
extern void paintshop_close();


/*
 * Function prototype for the supplied routine that mixes the various
 * paint tints into a can.
 *
 * YOU MUST USE THIS FUNCTION FOR MIXING
 *
 */
extern void mix(struct paintcan *);


/*
 * THESE PARAMETERS CAN BE CHANGED BY US, so you should test various
 * combinations. NOTE: We will only ever set these to something
 * greater than zero.
 */


#define NCUSTOMERS 10     /* The number of customers painting today*/
#define NPAINTSHOPSTAFF 3 /* The number of paint shop staff working today */

