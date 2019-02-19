#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "paintshop.h"


void order_paint(paint_can *can);
void go_home(void);
void * take_order(void);
void fill_order(void *v);
void serve_order(void *v);
void paintshop_open(void);
void paintshop_close(void);

/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * paintshop_open()
 *
 * Perform any initialisation you need prior to opening the paint shop to
 * staff and customers
 */

void paintshop_open()
{
    // time intervals for the stuffs to leave
    wait_time = 10000;

    remaining_customers = NCUSTOMERS;

    /* binary semaphores for accessing critical region */
    access_orders = sem_create("access_orders", 1);
    access_shipments   = sem_create("access_shipments", 1);
    access_tints  = sem_create("access_tints", 1);
    stuff_exit = sem_create("stuff_exit", 1);

    /* counting semaphores for controlling access to full or empty buffer */

    full_shipment = sem_create("full_shipment", 0);
    full_order = sem_create("full_order", 0);

    empty_shipment = sem_create("empty_shipment", NCUSTOMERS);
    empty_order = sem_create("empty_order", NCUSTOMERS);

    /* buffer initialization */
    int i;
    for(i = 0; i < NCUSTOMERS; i++)
    {
        order_buffer[i]=NULL;
        shipment_buffer[i] = NULL;
    }
}

/*
 * paintshop_close()
 *
 * Perform any cleanup after the paint shop has closed and everybody
 * has gone home.
 */

void paintshop_close()
{
    // cleanup
    sem_destroy(access_orders);
    sem_destroy(access_shipments);

    sem_destroy(full_shipment);
    sem_destroy(full_order);

    sem_destroy(empty_order);
    sem_destroy(empty_shipment);

    sem_destroy(access_tints);
    sem_destroy(stuff_exit);
}


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_paint()
 *
 * Takes one argument specifying the can to be filled. The function
 * makes the can available to staff threads and then blocks until the staff
 * have filled the can with the appropriately tinted paint.
 *
 * The can itself contains an array of requested tints.
 */

void order_paint(paint_can *can)
{
    /* decrement number of empty slots in order buffer */
    P(empty_order);

    /* enter critical region */
    P(access_orders);

    // try to put the can in a random available space in the buffer
    int rand= random()%NCUSTOMERS, randIndex, i;
    for(i = 0; i < NCUSTOMERS; i++)
    {
        randIndex=(rand+i)%NCUSTOMERS;
        if(order_buffer[randIndex] == NULL)
        {
            order_buffer[randIndex] = can;
            break;
        }
    }
    V(access_orders);
    /* exit critical region */

    V(full_order);

    bool found= false;
    while(1)
    {

        /* decrement number of ready cans that the customer can take back */
        P(full_shipment);

        /* enter critical region */
        P(access_shipments);

        // go through the shipment buffer looking for his paint_can
        for(i = 0; i < NCUSTOMERS; i++)
        {
            if((paint_can *)shipment_buffer[i] == can)
            {
                shipment_buffer[i] = NULL;
                found=true;
                break;
            }
        }
        V(access_shipments);
        /* exit critical region */

        V(full_shipment); /* done */


        if(found){
            /* increment number of empty slots in shipment buffer */
            V(empty_shipment);
            break;
        }

    }

}



/*
 * go_home()
 *
 * This function is called by customers when they go home. It could be
 * used to keep track of the number of remaining customers to allow
 * paint shop staff threads to exit when no customers remain.
 */

void go_home()
{
    remaining_customers--;
}


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY PAINT SHOP STAFF THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it records the details, and returns a
 * pointer to something representing the order.
 *
 * The return pointer type is void * to allow freedom of representation
 * of orders.
 *
 * The function can return NULL to signal the staff thread it can now
 * exit as their are no customers nor orders left.
 */


void * take_order()
{
    void *can;

    bool found=false;
    while(1) {
        //no customer left -> stuff departs
        if(remaining_customers == 0)
        {
            /* enter critical region */
            P(stuff_exit);
            int i=0;
            for(i=0; i<wait_time; i++);
            can=NULL;
            V(stuff_exit); /* exit critical region */
            return can;
        }


        /* decrement the number of ordered cans */
        P(full_order);
        /* enter critical region */
        P(access_orders);

        // look for a can
        int i;
        for(i = 0; i < NCUSTOMERS; i++)
        {
            if(order_buffer[i]!=NULL)
            {
                can= (void *)order_buffer[i];
                order_buffer[i] = NULL;
                found = true;
                break;
            }
        }
        V(access_orders);
        /* exit critical region */

        V(full_order); /* done */

        if(found){
            /* increment number of empty slots in order buffer */
            V(empty_order);
            break;
        }
    }

    return can;
}




/*
 * fill_order()
 *
 * This function takes an order generated by take order and fills the
 * order using the mix() function to tint the paint.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE TINTS
 * IT NEEDS TO USE TO FILE THE ORDER.
 */

void fill_order(void *v)
{
    /* enter critical region */
    P(access_tints);
    mix(v);
    V(access_tints);
    /* exit critical region */
}



/*
 * serve_order()
 *
 * Takes a filled order and makes it available to the waiting customer.
 */

void serve_order(void *v)
{
    int i;

    /* decremnet number of empty slots in shipment buffer */
    P(empty_shipment);

    /* enter critical region */
    P(access_shipments);

    // put the can in an empty space in shipment buffer
    for(i = 0; i < NCUSTOMERS; i++)
    {
        if(shipment_buffer[i] == NULL)
        {
            shipment_buffer[i] = v;
            break;
        }
    }
    V(access_shipments);
    /* exit critical region */

    /* increment number of ready cans that the customer can take back */
    V(full_shipment);
}

