/*************************************************************************************
*								 File Name	: Thread.h		   			 	         *
*								Usage : Used for threads creation                    *
**************************************************************************************/
#ifndef THREAD_HPP
#define THREAD_HPP

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

/* Define the Stack Size and define the methods for thread class */
#define	STKSIZE	 16536
class Thread
{
	public:
		Thread()
		{}
		virtual ~Thread()
		{}
		static void * pthread_callback (void *ptrThis);		/* Thread creation */
		virtual void run () = 0 ;							/* Start the Thread */
		void  start();										/* Thread initialization*/
};
#endif
