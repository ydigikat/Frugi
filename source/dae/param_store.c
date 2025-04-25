/*
  ------------------------------------------------------------------------------
   DAE
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
  ------------------------------------------------------------------------------
*/
#include <stdbool.h>
#include "FreeRTOS.h"

#include "param_store.h"
#include "dsp_core.h"
#include "dsp_math.h"

#include "trace.h"


/* Allocate new store space in blocks of this size*/
#define DAE_PARAM_ALLOC_SIZE (64)

/* Maximum size to which store can grow */
#define DAE_PARAM_MAX_CAPACITY (DAE_PARAM_ALLOC_SIZE * 4)

static float *param_store = NULL;
static size_t capacity;

/* flag indicating that store is dirty */
bool dae_param_changed = false;

/**
 * \brief allocate memory for the data store
 */
void dae_param_init(void)
{
    param_store = pvPortMalloc(DAE_PARAM_ALLOC_SIZE * sizeof(float));
    if (!param_store)
    {
        return;
    }
    capacity = DAE_PARAM_ALLOC_SIZE;    

    memset(param_store, 0, sizeof(float) * DAE_PARAM_ALLOC_SIZE);
}

/**
 * \brief grows the data store by DAE_PARAM_ALLOC_SIZE 
 * \note the new_size is multipled by 4 to hold float values.
 */
static void expand_store(size_t new_size)
{
    if (new_size > DAE_PARAM_MAX_CAPACITY)
        return;

    float *new_store = pvPortMalloc(new_size * sizeof(float));
    if (!new_store)
        return;

    memcpy(new_store, param_store, capacity * sizeof(float));
    memset(new_store + capacity, 0, (new_size - capacity) * sizeof(float));

    vPortFree(param_store);
    param_store = new_store;
    capacity = new_size;
}

/* Call this in the sets to auto expand store */
static void ensure_capacity(uint16_t param)
{
    if (param >= capacity)
    {
        size_t new_size = param + 1;
        if (new_size < capacity * 2)
            new_size = capacity * 2;
        expand_store(new_size);
    }
}

/* Setters always convert to unipolar normalised floats */
void param_set(uint16_t id, float norm_value)
{ 
    RTT_ASSERT(norm_value >= 0.0f && norm_value <= 1.0f);

#ifdef DAE_PARAM_STORE_AUTOSIZE    
    ensure_capacity(id) 
#endif    
    
    param_store[id] = norm_value;
    dae_param_changed = true;
}

void param_set_midi(uint16_t id, uint8_t value)
{    
    RTT_ASSERT(value <= 127);
    
#ifdef DAE_PARAM_STORE_AUTOSIZE    
    ensure_capacity(id) 
#endif        
    
    param_store[id] =(float)value / 127.0f;     
    dae_param_changed = true;
}

float param_get(uint16_t id)
{
    return param_store[id];
}


