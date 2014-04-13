/*
 * Copyright (c) 2007 Riccardo Lucchese, riccardo.lucchese at gmail.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include <pthread.h>
#include <malloc.h>

static int mem_usage_size;
static pthread_mutex_t mem_usage_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* (*_std_malloc_func)(size_t size, const void* caller);
static void* (*_std_realloc_func)(void* mem, size_t size, const void* caller);
static void (*_std_free_func)(void* mem, const void* caller);

void* mem_usage_malloc (size_t size, const void* caller);
void* mem_usage_realloc (void* mem, size_t size, const void* caller);
void mem_usage_free (void* mem, const void *caller);

void* mem_usage_malloc (size_t size, const void *caller)
{
	void* mem;
	pthread_mutex_lock(&mem_usage_mutex);
	/* Restore std hooks */
	__malloc_hook = _std_malloc_func;
	__realloc_hook = _std_realloc_func;
	__free_hook = _std_free_func;

	if (size)
	{
		mem = malloc(size);
		if (mem)
			mem_usage_size += malloc_usable_size(mem);
	}
	else
		mem = malloc(0);

	/* Restore our own hooks */
	__malloc_hook = mem_usage_malloc;
	__realloc_hook = mem_usage_realloc;
	__free_hook = mem_usage_free;
	pthread_mutex_unlock(&mem_usage_mutex);
	return mem;
}

void* mem_usage_realloc (void* src, size_t size, const void *caller)
{
	void* mem; 
	pthread_mutex_lock(&mem_usage_mutex);
	/* Restore std hooks */
	__malloc_hook = _std_malloc_func;
	__realloc_hook = _std_realloc_func;
	__free_hook = _std_free_func;

	mem_usage_size -= malloc_usable_size(src);
	mem = realloc (src, size);
	mem_usage_size += malloc_usable_size(mem);

	/* Restore our own hooks */
	__malloc_hook = mem_usage_malloc;
	__realloc_hook = mem_usage_realloc;
	__free_hook = mem_usage_free;
	pthread_mutex_unlock(&mem_usage_mutex);
	return mem;
}
 
     
void mem_usage_free (void *ptr, const void *caller)
{
	pthread_mutex_lock(&mem_usage_mutex);
	/* Restore std hooks */
	__malloc_hook = _std_malloc_func;
	__realloc_hook = _std_realloc_func;
	__free_hook = _std_free_func;

	mem_usage_size -= malloc_usable_size(ptr); 
	free (ptr);
  
	/* Restore our own hooks */
	__malloc_hook = mem_usage_malloc;
	__realloc_hook = mem_usage_realloc;
	__free_hook = mem_usage_free;
	pthread_mutex_unlock(&mem_usage_mutex);
}

static int mem_usage_get_size(void)
{
	int size;
	pthread_mutex_lock(&mem_usage_mutex);
	size = mem_usage_size;
	pthread_mutex_unlock(&mem_usage_mutex);
	return size;
}

void mem_usage_set_ref(void)
{
	pthread_mutex_lock(&mem_usage_mutex);
	mem_usage_size = 0;
	pthread_mutex_unlock(&mem_usage_mutex);
}


#include "Python.h"

static PyObject *
allocstats_setref(PyObject* a, PyObject* b)
{
	mem_usage_set_ref();
	Py_RETURN_NONE;
}


static PyObject *
allocstats_size(PyObject* a, PyObject* b)
{
	return Py_BuildValue("l", mem_usage_get_size());
}

static PyMethodDef allocstats_methods[] = {
	{"setref", allocstats_setref, METH_NOARGS, PyDoc_STR("Set the current allocated bytes value to zero")},
	{"size", allocstats_size, METH_NOARGS, PyDoc_STR("Get allocated bytes since module import")},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initallocstats(void)
{
	PyObject* mod;

	/* TODO: are there multiple threads running at this point ??? */
	mem_usage_size = 0;
  
	/* save standard hooks */
	_std_malloc_func = __malloc_hook;
	_std_realloc_func = __realloc_hook;
	_std_free_func = __free_hook;

	/* set our ones */
	__malloc_hook = mem_usage_malloc;
	__realloc_hook = mem_usage_realloc;
	__free_hook = mem_usage_free;

	mod = Py_InitModule("allocstats", allocstats_methods);
}
