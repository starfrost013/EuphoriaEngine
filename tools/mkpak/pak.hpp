/* pak.h - Zombono engine (Euphoria) archive format structs
 * licensed under the MIT license 
 * original author wishes to remain unnamed
 */
#ifndef PAK_H_
#define PAK_H_
#include <stdint.h>

#define PAK_HEADER_SZ 12
typedef struct {
    uint8_t magic[4];
    uint32_t offset;
    uint32_t size;
} pak_header;

#define FILE_HEADER_SZ 64
typedef struct {
    uint8_t name[56];
    uint32_t offset;
    uint32_t size;
} file_header;

#endif /* PAK_H_ */
