/*************************************************************************************
*								 File Name	: Thread.cpp		   			 	     *
*	Usage : Used for threads creation                                                *
**************************************************************************************/
#include "stdafx.h"
#include <cstdio>
#include <iostream>
#include "Thread.h"
using namespace std;

namespace Common
{
	/**
	* Function - pthread_callback
	* Usage: This is the callback needed by the Thread class
	*
	* @arg: void *
	*/
	void * Thread::pthread_callback(void *ptrThis)
	{

		if (ptrThis == NULL)
			return NULL;
		Thread  * ptr_this = (Thread *)(ptrThis);
		ptr_this->run(); /* Starts the created thread */
		return NULL;
	}

	/**
	* Function - start
	* Usage: Thread Creation
	*
	* @arg: void
	*/
	void Thread::start()
	{
		int result;
		/* Create the thread specifying the stack size and the callback function */
		if ((result = _beginthread((void(*)(void *))Thread::pthread_callback, STKSIZE, this)) < 0)
		{
			cerr << "_beginthread error " << endl;
			exit(1);
		}
	}
}