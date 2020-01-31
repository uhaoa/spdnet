#include <stdlib.h>
#include <string.h>
#include <spdnet/base/buffer.h>


struct  buffer_s
{
    char*   data ; 
    size_t  data_len ; 
    size_t  write_pos ; 
    size_t  read_pos ; 
} ; 

void 
ox_buffer_delete(struct buffer_s* self)
{
    if(NULL == self)
        return ; 
    if(NULL != self->data)
    {
        free(self->data);
        self->data = NULL ; 
    }

    free(self);
    self = NULL;
}

struct buffer_s* 
ox_buffer_new(size_t buffer_size)
{
    struct buffer_s* ret = (struct buffer_s*)malloc(sizeof(struct buffer_s));
    if(ret == NULL)
        return NULL ; 

    if((ret->data = (char*)malloc(sizeof(char) * buffer_size)) != NULL)
    {
        ret->data_len  = buffer_size ; 
        ret->write_pos = 0 ; 
        ret->read_pos  = 0 ; 
    }
    else
    {
        ox_buffer_delete(ret);
        ret = NULL ; 
    }
    return  ret ; 
}

size_t 
ox_buffer_getreadvalidcount(struct buffer_s* self)
{
    return self->write_pos - self->read_pos ; 
}

void 
ox_buffer_adjustto_head(struct buffer_s* self)
{
    size_t len = 0  ; 
    if(self->read_pos <= 0)
        return ; 

    len = ox_buffer_getreadvalidcount(self);
    if(len > 0)
    {
        memmove(self->data , self->data + self->read_pos, len) ; 
    }

    self->write_pos = 0 ; 
    self->read_pos = 0 ; 
}


void 
ox_buffer_init(struct buffer_s* self)
{
    self->write_pos = 0 ; 
    self->read_pos = 0 ; 
}

size_t 
ox_buffer_get_read_pos(struct buffer_s* self)
{
    return self->read_pos ; 
}

size_t 
ox_buffer_get_write_pos(struct buffer_s* self)
{
    return self->write_pos ; 
}

void 
ox_buffer_addwritepos(struct buffer_s* self , size_t len)
{
    size_t tmp = self->write_pos + len ; 
    if(tmp <= self->data_len)
    {
        self->write_pos = tmp ;    
    }
}

void 
ox_buffer_addreadpos(struct buffer_s* self , size_t len)
{
    size_t tmp = self->read_pos  +len ; 
    if(tmp <= self->data_len)
    {
        self->read_pos = tmp ; 
    }
}

size_t ox_buffer_getwritevalidcount(struct buffer_s* self)
{
    return  self->data_len - self->write_pos ; 
}

char* 
ox_buffer_getwriteptr(struct buffer_s* self)
{
    if(self->write_pos < self->data_len)
        return self->data + self->write_pos ; 
    else
        return NULL ;     
}

char* 
ox_buffer_getreadptr(struct buffer_s* self)
{
    if(self->read_pos < self->data_len)
        return self->data + self->read_pos ; 
    else
        return NULL ;
}

bool  
ox_buffer_write(struct buffer_s* self , const char* data , size_t len)
{
    bool ret = true;
    if(ox_buffer_getwritevalidcount(self) >=  len)
    {
        memcpy(ox_buffer_getwriteptr(self) , data , len) ;
        ox_buffer_addwritepos(self , len);
    }
    else{
        size_t left_len = self->data_len - ox_buffer_getreadvalidcount(self) ; 
        if(left_len >= len)
        {
            ox_buffer_adjustto_head(self) ; 
            ox_buffer_write(self, data , len) ; 
        } 
        else{
            ret = false;
        }
    }

    return ret; 
}


size_t  
ox_buffer_getsize(struct buffer_s* self)
{
    return self->data_len ; 
}


