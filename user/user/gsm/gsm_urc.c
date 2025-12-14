#include "gsm_urc.h"

bool at_parse_line(const char *line, urc_t *out){
    if (!line || !out) return false;
    memset(out, 0, sizeof(*out));
    out->type = URC_UNKNOWN;


    size_t n = strlen(line);
    while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) {
        n--;
    }
    if (n == 0) return false;

    if ((n >= 2 && line[0] == 'A' && line[1] == 'T') ||
        (n >= 3 && strncmp(line, "AT+", 3) == 0)) {
        return false;
    }

  
    if (n == 2 && line[0] == 'O' && line[1] == 'K') {
        out->type = URC_OK;
        return true;
    }
    if (n == 5 && strncmp(line, "ERROR", 5) == 0) {
        out->type = URC_ERROR;
        return true;
    }

    if (n >= 11 && strncmp(line, "+CME ERROR:", 11) == 0) {
        const char *p = line + 11;
        while (*p == ' ' || *p == '\t') p++;
        out->v1 = parse_int(p);
        out->type = URC_CME_ERROR;
        return true;
    }

  
    if (n >= 6 && strncmp(line, "+CPIN:", 6) == 0) {
        if (n >= 12 && strncmp(line + 7, "READY", 5) == 0) {
            out->type = URC_CPIN_READY;
            return true;
        }
    }

    if (n >= 6 && strncmp(line, "+CREG:", 6) == 0) {
        const char *p = line + 6;
        out->v1 = -1; out->v2 = -1;
        
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        out->v1 = parse_int(p);
        const char *comma = strchr(p, ',');
        if (comma) {
            out->v2 = parse_int(comma + 1);
        }
        out->type = URC_CREG;
        return true;
    }

  
    if (n >= 7 && strncmp(line, "+CGREG:", 7) == 0) {
        const char *p = line + 7;
        out->v1 = -1; out->v2 = -1;
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        out->v1 = parse_int(p);
        const char *comma = strchr(p, ',');
        if (comma) {
            out->v2 = parse_int(comma + 1);
        }
        out->type = URC_CGREG;
        return true;
    }


    if (n >= 5 && strncmp(line, "+CSQ:", 5) == 0) {
        const char *p = line + 5;
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        out->v1 = parse_int(p);
        const char *comma = strchr(p, ',');
        if (comma) out->v2 = parse_int(comma + 1);
        out->type = URC_CSQ;
        return true;
    }

  
    if (n >= 10 && strncmp(line, "+PDP: DEACT", 11) == 0) {
        out->type = URC_PDP_DEACT;
        return true;
    }


    if (n >= 6 && strncmp(line, "+CMTI:", 6) == 0) {
        const char *p = strchr(line, '"');
        if (p) {
            const char *q = strchr(p + 1, '"');
            if (q) {
                size_t len = q - (p + 1);
                if (len >= sizeof(out->text)) len = sizeof(out->text) - 1;
                memcpy(out->text, p + 1, len);
                out->text[len] = '\0';
                const char *idx = strchr(q, ',');
                if (idx) out->v1 = parse_int(idx + 1);
            }
        }
        out->type = URC_CMTI;
        return true;
    }
    
// parser cho sms_recive
    if (n >= 6 && strncmp(line, "+CMGR:", 6) == 0) {
        const char *p = line + 6;
        int quote_count = 0;
        const char *num_start = NULL;
        const char *num_end = NULL;
        
        while (*p) {
            if (*p == '"') {
                quote_count++;
                if (quote_count == 3) {
                    num_start = p + 1;
                } else if (quote_count == 4 && num_start) {
                    num_end = p;
                    break;
                }
            }
            p++;
        }
        
        if (num_start && num_end) {
            size_t len = num_end - num_start;
            if (len >= sizeof(out->text)) len = sizeof(out->text) - 1;
            memcpy(out->text, num_start, len);
            out->text[len] = '\0';
        }
        out->type = URC_CMGR;
        return true;
    }

    if (n >= 6 && strncmp(line, "+CMGS:", 6) == 0) {
        const char *p = line + 6;
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        out->v1 = parse_int(p);
        out->type = URC_CMGS;
        return true;
    }

   
    if (n >= 12 && strncmp(line, "+HTTPACTION:", 12) == 0) {
        const char *p = line + 12;
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        out->v1 = parse_int(p); // method: 0=GET, 1=POST, ...
        const char *c1 = strchr(p, ',');
        if (c1) {
            out->v2 = parse_int(c1 + 1); // status code
            const char *c2 = strchr(c1 + 1, ',');
            if (c2) {
                out->v3 = parse_int(c2 + 1); // data length
            }
        }
        out->type = URC_HTTPACTION;
        return true;
    }

    return false;
}


bool raw_data_bin(const char *linebuff, urc_t *out) {
    if (!linebuff || !out) return false;
    
    size_t n = strlen(linebuff);
    

    while (n > 0 && (linebuff[n - 1] == '\r' || linebuff[n - 1] == '\n')) {
        n--;
    }
    if (n == 0) return false;
    
    if (n >= 10 && strncmp(linebuff, "+HTTPREAD:", 10) == 0) {
        const char *p = linebuff + 10;
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        
        // Check xem có "DATA," không: +HTTPREAD: DATA,<len>
        if (strncmp(p, "DATA,", 5) == 0) {
            p += 5;
        }
        
        int len = parse_int(p);
        
        // Nếu len = 0, đó là +HTTPREAD: 0 (kết thúc)
        if (len == 0) {
            out->v1 = 0;
            out->type = URC_HTTPREAD_END;  // Cần thêm type này
        } else {
            out->v1 = len;
            out->type = URC_HTTPREAD;
        }
        return true;
    }
    
    return false; 
}