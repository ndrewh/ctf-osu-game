#define CHECK_IDX 42
#define CHAR_TO_IDX(i) ((i >= 'A' && i <= 'Z') ? i - 'A' : ((i >= '0' && i <= '9') ? i - '0' + 26 : ( i == '{' ? 36 : (i == '}' ? 37 : (i == '_' ? 38 : ((i >= 'a' && i <= 'z') ? (i - 'a' + 43) : (i == ':' ? 39 : (i == ' ' ? 40 : (i == '-' ? 41 : 0))))) ))))
