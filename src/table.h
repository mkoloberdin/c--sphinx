/* some defines for extracting instruction bit fields from bytes */
#define MOD(a)	  (((a)>>6)&3)
#define REG(a)	  (((a)>>3)&7)
#define RM(a)	  ((a)&7)
#define SCALE(a)  (((a)>>6)&3)
#define INDEX(a)  (((a)>>3)&7)
#define BASE(a)   ((a)&7)

#include <cstdint>

typedef union {
    struct {
        uint16_t ofs;
        uint16_t seg;
    } w;
    uint32_t dword;
} WORD32;

/* prototypes */
void ua_str(const char *, fs::ofstream &OFS);
unsigned char getByte(fs::ofstream &OFS);
int modrm(fs::ofstream &OFS);
int sib(fs::ofstream &OFS);
void uprintf(const char *, ...);
void uputchar(char );
int bytes(char );
void outhex(char, int, int, int, int, fs::ofstream &OFS);
void reg_name(int , char );
void do_sib(int, fs::ofstream &OFS);
void do_modrm(char, fs::ofstream &OFS);
void floating_point(int, fs::ofstream &OFS);
void percent(char, char, fs::ofstream &OFS);
void undata(unsigned offset, unsigned long len, unsigned int type, fs::ofstream &OFS);

