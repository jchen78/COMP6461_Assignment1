#define COMMON_API __declspec(dllexport)

#pragma once
/*************************************************************************************
*								 File Name	: Thread.h		   			 	         *
*								Usage : Used for threads creation                    *
**************************************************************************************/
#ifndef THREAD_HPP
#define THREAD_HPP

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "Common.h"

/* Define the Stack Size and define the methods for thread class */
#define	STKSIZE	 16536

namespace Common
{
	class COMMON_API Thread
	{
	public:
		Thread() {}
		static void * pthread_callback(void *ptrThis);		/* Thread creation */
		virtual void run() = 0;
		void  start();										/* Thread initialization*/
	};
}

#endif
