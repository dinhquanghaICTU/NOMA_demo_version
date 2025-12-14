#ifndef __GSM_URC_H__
#define __GSM_URC_H__ 

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum {
    URC_OK,
    URC_ERROR,
    URC_CME_ERROR,   
    URC_CPIN_READY,
    URC_CREG,        
    URC_CGREG,       
    URC_CSQ,        
    URC_PDP_DEACT,
    URC_CMTI,        
    URC_CMGR,        
    URC_CMGS,        
    URC_HTTPACTION,
    URC_HTTPREAD,
    URC_HTTPREAD_END,  // +HTTPREAD: 0 (kết thúc)
    URC_UNKNOWN
} urc_type_t;

typedef struct {
    urc_type_t type;
    int v1;
    int v2;
    int v3;
    char text[64];  
} urc_t;

static inline int parse_int(const char *s) { return atoi(s); }

bool at_parse_line(const char *line, urc_t *out);
bool raw_data_bin(const char *linebuff, urc_t *out);

#endif // __GSM_URC_H__

