#ifndef SPDNET_BASE_BUFFER_H
#define SPDNET_BASE_BUFFER_H 

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

struct buffer_s;

void ox_buffer_delete(struct buffer_s* self);
struct buffer_s* ox_buffer_new(size_t buffer_size);
void ox_buffer_adjustto_head(struct buffer_s* self);
size_t ox_buffer_getreadvalidcount(struct buffer_s* self);
size_t ox_buffer_getwritevalidcount(struct buffer_s* self);
void ox_buffer_init(struct buffer_s* self);
size_t ox_buffer_get_read_pos(struct buffer_s* self);
size_t ox_buffer_get_write_pos(struct buffer_s* self);
void ox_buffer_addwritepos(struct buffer_s* self , size_t len);
void ox_buffer_addreadpos(struct buffer_s* self , size_t len);
char* ox_buffer_getwriteptr(struct buffer_s* self);
char* ox_buffer_getreadptr(struct buffer_s* self);
bool  ox_buffer_write(struct buffer_s* self , const char* data , size_t len);
size_t  ox_buffer_getsize(struct buffer_s* self);
#ifdef __cplusplus
}
#endif

#endif //SPDNET_BASE_BUFFER_H
